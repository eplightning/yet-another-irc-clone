#include <server/tcp_server.h>

#include <common/types.h>
#include <common/packet.h>
#include <server/selector.h>

#include <set>

#include <fcntl.h>
#include <unistd.h>

YAIC_NAMESPACE

static const int TCP_SELECTOR_TYPE_LISTEN = 1;
static const int TCP_SELECTOR_TYPE_CLIENT = 2;
static const int TCP_SELECTOR_TYPE_PIPE = 3;

enum TcpPipeNotificationType {
    TcpPipeTypeSendBufferReceived,
    TcpPipeTypeDisconnect,
    TcpPipeTypeTerminateLoop
};

struct TcpPipeNotification {
    TcpPipeNotificationType type;
    uint clientid;
    int options;
};

Client::Client(uint id, int fd, const sockaddr *addr, ListenPool *pool) :
    m_id(id), m_socket(fd), m_pool(pool), m_state(State::Connected), m_writeError(0), m_readError(0), m_sendBuffer(nullptr),
    m_sendBufferQueue(), m_sendBufferMutex(), m_receiveBuffer()
{
    if (addr->sa_family == PF_INET) {
        m_proto = ConnectionProtoIpv4;
        const sockaddr_in *addr4 = reinterpret_cast<const sockaddr_in*>(addr);
        m_port = ntohs(addr4->sin_port);

        char ipv4[INET_ADDRSTRLEN];
        inet_ntop(PF_INET, &addr4->sin_addr, ipv4, INET_ADDRSTRLEN);
        m_address.assign(ipv4);
    } else {
        m_proto = ConnectionProtoIpv6;
        const sockaddr_in6 *addr6 = reinterpret_cast<const sockaddr_in6*>(addr);
        m_port = ntohs(addr6->sin6_port);

        char ipv6[INET6_ADDRSTRLEN];
        inet_ntop(PF_INET6, &addr6->sin6_addr, ipv6, INET6_ADDRSTRLEN);
        m_address.assign(ipv6);
    }
}

Client::~Client()
{

}

ClientSendBuffer *Client::sendBuffer()
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

void Client::attachSendBuffer(ClientSendBuffer *buffer)
{
    m_sendBufferMutex.lock();
    m_sendBufferQueue.push(buffer);
    m_sendBufferMutex.unlock();
}

TcpServer::TcpServer(int rounds) : m_pools(), m_clients(), m_nextClientId(1), m_rrRounds(rounds)
{
    pipe(m_pipe);
}

TcpServer::~TcpServer()
{
    for (auto &x : m_pools) {
        for (auto &y : x.second->sockets)
            close(y.socket);

        delete x.second;
    }

    close(m_pipe[0]);
    close(m_pipe[1]);
}

void TcpServer::createPool(String name, ListenPool *pool)
{
    // todo: remove existing

    m_pools[name] = pool;
}

void TcpServer::disconnect(SharedPtr<Client> client, bool force)
{
    TcpPipeNotification notify;
    notify.type = TcpPipeTypeDisconnect;
    notify.clientid = client->id();
    notify.options = static_cast<int>(force);
    write(m_pipe[1], &notify, sizeof(notify));
}

void TcpServer::sendTo(SharedPtr<Client> client, ClientSendBuffer *buffer)
{
    client->attachSendBuffer(buffer);

    TcpPipeNotification notify;
    notify.type = TcpPipeTypeSendBufferReceived;
    notify.clientid = client->id();
    notify.options = 0;
    write(m_pipe[1], &notify, sizeof(notify));
}

void TcpServer::sendTo(SharedPtr<Client> client, const Packet *packet)
{
    ClientSendBuffer *buffer = new ClientSendBuffer;

    buffer->sent = 0;
    packet->encode(buffer->data);

    sendTo(client, buffer);
}

void TcpServer::sendTo(const Vector<SharedPtr<Client> > &clients, const Packet *packet)
{
    Vector<char> encoded;
    packet->encode(encoded);

    for (auto &client : clients) {
        ClientSendBuffer *buffer = new ClientSendBuffer;

        buffer->sent = 0;
        buffer->data = encoded;

        sendTo(client, buffer);
    }
}

void TcpServer::runLoop()
{
    UniquePtr<Selector> selectScope(Selector::factory());
    Selector *select = selectScope.get();

    select->add(m_pipe[0], TCP_SELECTOR_TYPE_PIPE, 0, SelectorInfo::ReadEvent);

    for (auto &pool : m_pools) {
        for (auto &sock : pool.second->sockets)
            select->add(sock.socket, TCP_SELECTOR_TYPE_LISTEN, &sock, SelectorInfo::ReadEvent);
    }

    Vector<SelectorEvent> events;
    bool stopLooping = false;

    while (!stopLooping && select->wait(events) == Selector::WaitRetval::Success) {
        Vector<Client*> eraseList;

        for (auto &event : events) {
            if (event.info()->type() == TCP_SELECTOR_TYPE_LISTEN) {
                newConnection(static_cast<ListenPoolSocket*>(event.info()->data()), select);
            } else if (event.info()->type() == TCP_SELECTOR_TYPE_PIPE) {
                if (event.type() != SelectorInfo::ReadEvent)
                    continue;

                stopLooping = stopLooping || pipeNotification(select, eraseList);
            } else if (event.info()->type() == TCP_SELECTOR_TYPE_CLIENT) {
                Client *client = static_cast<Client*>(event.info()->data());

                if (event.type() == SelectorInfo::WriteEvent) {
                    writeEvent(client, select, eraseList);
                } else if (event.type() == SelectorInfo::ReadEvent && client->readError() == 0) {
                    readEvent(client, event, select, eraseList);
                }
            }
        }

        // na końcu pozbywamy się klientów usuniętych w tej iteracji
        for (auto *x : eraseList) {
            select->close(x->socket());
            close(x->socket());
            x->pool()->droppedConnection(x->id());
            m_clients.erase(x->id());
        }
    }
}

void TcpServer::stopLoop()
{
    TcpPipeNotification notify;
    notify.type = TcpPipeTypeTerminateLoop;
    notify.clientid = 0;
    notify.options = 0;
    write(m_pipe[1], &notify, sizeof(notify));
}

void TcpServer::newConnection(ListenPoolSocket *listen, Selector *select)
{
    sockaddr_in6 addr;
    socklen_t addrLen = sizeof(sockaddr_in6);
    int sock = accept(listen->socket, (sockaddr*) &addr, &addrLen);
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);

    SharedPtr<Client> client = std::make_shared<Client>(m_nextClientId++, sock, (sockaddr*) &addr, listen->pool);

    NewConnectionResponse res = listen->pool->newConnection(client);

    if (res == NewConnectionResponseAccept) {
        m_clients[client->id()] = client;
        select->add(sock, TCP_SELECTOR_TYPE_CLIENT, client.get(), SelectorInfo::ReadEvent);
    } else {
        close(sock);
    }
}

bool TcpServer::pipeNotification(Selector *select, Vector<Client*> &eraseList)
{
    TcpPipeNotification notify;
    read(m_pipe[0], &notify, sizeof(TcpPipeNotification));

    if (notify.type == TcpPipeTypeTerminateLoop) {
        return true;
    } else {
        auto clientIt = m_clients.find(notify.clientid);
        if (clientIt == m_clients.end())
            return false;
        Client *client = (*clientIt).second.get();

        if (notify.type == TcpPipeTypeDisconnect) {
            // wysyłamy wszystkie nasze pakiety w kolejce, ale już absolutnie nic nie odbieramy
            stopReading(client, select, eraseList, -2);
        } else if (notify.type == TcpPipeTypeSendBufferReceived) {
            int eventType = 0;
            if (client->readError() == 0)
                eventType |= SelectorInfo::ReadEvent;
            if (client->writeError() == 0)
                eventType |= SelectorInfo::WriteEvent;

            if (eventType != 0)
                select->modify(client->socket(), eventType);
        }
    }

    return false;
}

void TcpServer::readEvent(Client *client, SelectorEvent &event, Selector *select, Vector<Client *> &eraseList, int rounds)
{
    ClientReceiveBuffer &buffer = client->receiveBuffer();

    int received = 0;

    // odczyt nagłówka
    if (buffer.received < PACKET_HEADER_SIZE) {
        received = read(client->socket(), buffer.data + buffer.received, PACKET_HEADER_SIZE - buffer.received);

        if (received == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            return stopReading(client, select, eraseList, errno);
        } else if (received < static_cast<int>(PACKET_HEADER_SIZE - buffer.received) && event.info()->closed()) {
            return stopReading(client, select, eraseList, EPIPE);
        } else if (received > 0) {
            buffer.received += received;
        }

        if (buffer.received < PACKET_HEADER_SIZE) {
            return;
        } else {
            buffer.header.type = ntohs(*(reinterpret_cast<u16*>(buffer.data)));
            buffer.header.payloadSize = ntohs(*(reinterpret_cast<u16*>(buffer.data + 2)));

            // pusty pakiet?
            if (buffer.header.payloadSize == 0) {
                client->pool()->receive(client->id(), buffer.header, buffer.data);
                buffer.received = 0;
                return;
            }
        }
    }

    // nieprawidłowe pakiety
    if (buffer.header.payloadSize > PACKET_MAX_SIZE - PACKET_HEADER_SIZE) {
        return stopReading(client, select, eraseList, -1);
    }

    received = read(client->socket(), buffer.data + buffer.received,
                    buffer.header.payloadSize - buffer.received - PACKET_HEADER_SIZE);

    if (received == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        return stopReading(client, select, eraseList, errno);
    } else if (received < static_cast<int>(buffer.header.payloadSize - buffer.received - PACKET_HEADER_SIZE)
                    && event.info()->closed()) {
        return stopReading(client, select, eraseList, EPIPE);
    } else if (received > 0) {
        buffer.received += received;
    }

    if (buffer.received >= buffer.header.payloadSize + PACKET_HEADER_SIZE) {
        client->pool()->receive(client->id(), buffer.header, buffer.data);
        buffer.received = 0;

        if (rounds < m_rrRounds)
            readEvent(client, event, select, eraseList, ++rounds);
    }
}

void TcpServer::writeEvent(Client *client, Selector *select, Vector<Client *> &eraseList, int rounds)
{
    ClientSendBuffer *buffer = client->sendBuffer();

    if (buffer == nullptr)
        return stopWriting(client, select, eraseList, client->writeError());

    int written = write(client->socket(), &(buffer->data[buffer->sent]), buffer->data.size() - buffer->sent);
    if (written == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            return stopWriting(client, select, eraseList, errno);
    } else {
        buffer->sent += written;
    }

    // sprawdzamy czy coś jeszcze jest w kolejce do wysłania
    if (client->sendBuffer() == nullptr)
        return stopWriting(client, select, eraseList, client->writeError());
    else if (rounds < m_rrRounds)
        writeEvent(client, select, eraseList, ++rounds);
}

void TcpServer::stopReading(Client *client, Selector *select, Vector<Client *> &eraseList, int error)
{
    client->setReadError(error);
    client->setState(Client::State::Disconnecting);
    client->pool()->lostConnection(client->id());

    if (client->sendBuffer() != nullptr)
        select->modify(client->socket(), SelectorInfo::WriteEvent);
    else
        eraseList.push_back(client);
}

void TcpServer::stopWriting(Client *client, Selector *select, Vector<Client *> &eraseList, int error)
{
    client->setWriteError(error);

    if (client->state() == Client::State::Disconnecting || client->readError() != 0)
        eraseList.push_back(client);
    else
        select->modify(client->socket(), SelectorInfo::ReadEvent);
}


END_NAMESPACE
