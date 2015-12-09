#include <server/tcp_client.h>

#include <common/types.h>
#include <common/packet.h>
#include <server/selector.h>
#include <server/tcp.h>

#include <functional>
#include <queue>
#include <mutex>

#include <arpa/inet.h>
#include <unistd.h>

YAIC_NAMESPACE

static const int TCP_SELECTOR_TYPE_PIPE = 1;
static const int TCP_SELECTOR_TYPE_SOCKET = 2;

enum TcpClientMessageType {
    TCMTSendBufferReceived,
    TCMTDisconnect,
    TCMTTerminateLoop
};

TcpClient::TcpClient(const String &address) :
    m_socket(-1), m_state(TCSDisconnected), m_closing(false),
    m_readError(0), m_writeError(0), m_connectError(0), m_sendBuffer(nullptr)
{
    pipe(m_pipe);

    m_proto = SocketUtils::readAddress(address, m_port, m_address);
}

TcpClient::~TcpClient()
{
    close(m_pipe[0]);
    close(m_pipe[1]);

    if (m_socket != -1)
        close(m_socket);

    if (m_sendBuffer != nullptr)
        delete m_sendBuffer;

    while (!m_sendBufferQueue.empty()) {
        TcpSendBuffer *buf = m_sendBufferQueue.front();
        delete buf;
        m_sendBufferQueue.pop();
    }
}

void TcpClient::disconnect()
{
    TcpClientMessageType notify = TCMTDisconnect;
    write(m_pipe[1], &notify, sizeof(notify));
}

void TcpClient::send(TcpSendBuffer *buffer)
{
    {
        std::lock_guard<std::mutex> lock(m_sendBufferMutex);
        m_sendBufferQueue.push(buffer);
    }

    TcpClientMessageType notify = TCMTSendBufferReceived;
    write(m_pipe[1], &notify, sizeof(notify));
}

void TcpClient::send(const Packet *packet)
{
    TcpSendBuffer *buffer = new TcpSendBuffer;

    buffer->sent = 0;
    packet->encode(buffer->data);

    send(buffer);
}

void TcpClient::stopLoop()
{
    TcpClientMessageType notify = TCMTTerminateLoop;
    write(m_pipe[1], &notify, sizeof(notify));
}

bool TcpClient::runLoop()
{
    if (!isValid() || m_socket != -1)
        return false;

    // select
    UniquePtr<Selector> selectScope(Selector::factory());
    Selector *select = selectScope.get();

    // address
    sockaddr_storage saddr;
    socklen_t slen = (m_proto == CPIpv4) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

    if (!SocketUtils::fillSockaddr(&saddr, m_address, m_port, m_proto))
        return false;

    // socket
    m_socket = socket((m_proto == CPIpv4) ? AF_INET : AF_INET6, SOCK_STREAM, 0);

    if (m_socket == -1)
        return false;

    if (!SocketUtils::makeNonBlocking(m_socket))
        return false;

    if (connect(m_socket, (sockaddr*) &addr, slen) == -1 && errno != EINPROGRESS)
        return false;

    select->add(m_pipe[0], TCP_SELECTOR_TYPE_PIPE, 0, SelectorInfo::ReadEvent);
    select->add(m_socket, TCP_SELECTOR_TYPE_SOCKET, 0, SelectorInfo::ReadEvent | SelectorInfo::WriteEvent);

    Vector<SelectorEvent> events;

    while (!m_closing && select->wait(events)) {
        for (auto &event : events) {
            if (event.info()->type() == TCP_SELECTOR_TYPE_PIPE) {
                if (event.type() != SelectorInfo::ReadEvent)
                    continue;

                pipeNotification(select);

                // nie ma sensu dalej przetwarzać jeśli nie było się połączonym i tak
                if (m_closing && m_state == TCSDisconnected)
                    break;
            } else if (event.info()->type() == TCP_SELECTOR_TYPE_SOCKET) {
                // writeEvent sam sprawdza czy jest błąd dlatego tutaj nie ma drugiego wraunku
                if (event.type() == SelectorInfo::WriteEvent)
                    writeEvent(select);
                else if (event.type() == SelectorInfo::ReadEvent && m_readError == 0)
                    readEvent(event, select);
            }
        }
    }

    m_stateDelegate(TCSDisconnected, m_connectError ? m_connectError : (m_writeError ? m_writeError : m_readError));
    close(m_socket);
    m_socket = -1;

    return true;
}

void TcpClient::setStateDelegate(ClientStateDelegate stateDelegate)
{
    m_stateDelegate = stateDelegate;
}

void TcpClient::setDataDelegate(ClientReceiveDataDelegate dataDelegate)
{
    m_dataDelegate = dataDelegate;
}

bool TcpClient::isValid() const
{
    return m_proto != CPUnknown;
}

void TcpClient::pipeNotification(Selector *select)
{
    TcpClientMessageType notify;
    read(m_pipe[0], &notify, sizeof(TcpClientMessageType));

    if (notify == TCMTTerminateLoop) {
        m_closing = true;
        return;
    }

    if (notify == TCMTDisconnect && m_state == TCSConnected)
        m_state = TCSDisconnecting;

    int eventType = 0;
    if (m_readError == 0)
        eventType |= SelectorInfo::ReadEvent;
    if (m_writeError == 0)
        eventType |= SelectorInfo::WriteEvent;

    if (eventType != 0)
        select->modify(m_socket, eventType);
}

void TcpClient::readEvent(SelectorEvent &event, Selector *select)
{
    TcpReceiveBuffer &buffer = m_receiveBuffer;
    char *data = m_receiveBuffer.data.data();
    int received = 0;

    // odczyt nagłówka
    if (buffer.received < Packet::HeaderSize) {
        received = read(m_socket, data + buffer.received, Packet::HeaderSize - buffer.received);

        // kończymy czytam jeśli mamy błąd lub koniec socketa (jeśli druga strona już nam nic nie wyśle)
        if (received == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            return stopReading(select, errno);
        } else if (received < static_cast<int>(Packet::HeaderSize - buffer.received) &&
                      (event.info()->closed() || m_state == TCSWritingClosed)) {
            return stopReading(select, EPIPE);
        } else if (received > 0) {
            buffer.received += received;
        }

        if (buffer.received < Packet::HeaderSize) {
            return;
        } else {
            buffer.header.type = ntohs(*(reinterpret_cast<u16*>(data)));
            buffer.header.payloadSize = ntohs(*(reinterpret_cast<u16*>(data + 2)));

            // pusty pakiet?
            if (buffer.header.payloadSize == 0) {
                m_dataDelegate(buffer.header, buffer.data);
                buffer.received = 0;
                return;
            }
        }
    }

    // nieprawidłowe pakiety
    if (buffer.header.payloadSize > Packet::MaxPayloadSize)
        return stopReading(select, -1);

    received = read(m_socket, data + buffer.received,
                    buffer.header.payloadSize - (buffer.received - Packet::HeaderSize));

    if (received == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        return stopReading(select, errno);
    } else if (received < static_cast<int>(buffer.header.payloadSize - (buffer.received - Packet::HeaderSize)) &&
                  (event.info()->closed() || m_state == TCSWritingClosed)) {
        return stopReading(select, EPIPE);
    } else if (received > 0) {
        buffer.received += received;
    }

    if (buffer.received >= buffer.header.payloadSize + Packet::HeaderSize) {
        m_dataDelegate(buffer.header, buffer.data);
        buffer.received = 0;
    }
}

void TcpClient::writeEvent(Selector *select)
{
    TcpSendBuffer *buffer = sendBuffer();

    if (buffer == nullptr)
        return stopWriting(select, m_writeError);

    int written = write(m_socket, &(buffer->data[buffer->sent]), buffer->data.size() - buffer->sent);
    if (written == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            return stopWriting(select, errno);
    } else {
        buffer->sent += written;
    }

    // sprawdzamy czy coś jeszcze jest w kolejce do wysłania
    if (sendBuffer() == nullptr)
        return stopWriting(select, m_writeError);
}

void TcpClient::stopReading(Selector *select, int error)
{
    m_readError = error;
    m_stateDelegate(TCSDisconnecting, error);

    if (sendBuffer() != nullptr)
        select->modify(m_socket, SelectorInfo::WriteEvent);
    else
        m_closing = true;
}

void TcpClient::stopWriting(Selector *select, int error)
{
    if (m_readError != 0) {
        m_closing = true;
    } else {
        select->modify(m_socket, SelectorInfo::ReadEvent);

        if (m_state == TCSDisconnecting) {
            // przy 'lekkim' rozłączeniu generalnie chcemy żeby klient dostał wszystkie nasze wiadomości
            // niestety oznacza to że nie może sobie od razu close() zrobić
            // najlepsze rozwiązanie to zamknięcie strumienia wyjściowego i czekanie z zamknięciem socketa aż klient zamknie własnego
            // dlatego niestety bez heartbeatów się nie obejdzie, w razie timeouta trzeba ostrego disconnecta żeby bezwarunkowo zamknął połączenie
            // info z http://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable
            shutdown(m_socket, SHUT_WR);

            if (error == 0)
                error = -1;

            m_state = TCSWritingClosed;
        }
    }

    m_writeError = error;
}

TcpSendBuffer *TcpClient::sendBuffer()
{
    if (m_writeError != 0)
        return nullptr;

    if (m_sendBuffer != nullptr) {
        if (m_sendBuffer->data.size() > m_sendBuffer->sent)
            return m_sendBuffer;

        delete m_sendBuffer;
        m_sendBuffer = nullptr;
    }

    m_sendBufferMutex.lock();

    if (!m_sendBufferQueue.empty()) {
        m_sendBuffer = m_sendBufferQueue.front();
        m_sendBufferQueue.pop();
    }

    m_sendBufferMutex.unlock();

    return m_sendBuffer;
}

END_NAMESPACE
