#include <server/tcp_manager.h>

#include <common/types.h>
#include <common/packet.h>
#include <server/selector.h>
#include <server/tcp.h>
#include <server/misc_utils.h>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>

YAIC_NAMESPACE

static const int TCP_SELECTOR_TYPE_LISTEN = 1;
static const int TCP_SELECTOR_TYPE_CLIENT = 2;
static const int TCP_SELECTOR_TYPE_PIPE = 3;

enum TcpManagerMessageType {
    TMMTSendBufferReceived,
    TMMTDisconnect,
    TMMTTerminateLoop,
    TMMTDisconnectAll,
    TMMTConnect
};

struct TcpManagerConnectInfo {
    int socket;
    sockaddr_storage saddr;
    TcpPool *pool;
};

struct TcpManagerMessage {
    TcpManagerMessageType type;
    uint clientid;
    union {
        bool flag;
        void *ptr;
    } options;
};

Client::Client(uint id, int fd, const sockaddr *addr, TcpPool *pool) :
    m_id(id), m_socket(fd), m_pool(pool), m_state(TCSConnected), m_writeError(0), m_readError(0),
    m_connectError(0), m_dead(false), m_sendBuffer(nullptr),
    m_sendBufferQueue(), m_sendBufferMutex(), m_receiveBuffer()
{
    if (addr->sa_family == AF_INET) {
        m_proto = CPIpv4;
        const sockaddr_in *addr4 = reinterpret_cast<const sockaddr_in*>(addr);
        m_port = ntohs(addr4->sin_port);

        char ipv4[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr4->sin_addr, ipv4, INET_ADDRSTRLEN);
        m_address.assign(ipv4);

        memcpy(&m_sockaddr, addr4, sizeof(sockaddr_in));
    } else {
        m_proto = CPIpv6;
        const sockaddr_in6 *addr6 = reinterpret_cast<const sockaddr_in6*>(addr);
        m_port = ntohs(addr6->sin6_port);

        char ipv6[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &addr6->sin6_addr, ipv6, INET6_ADDRSTRLEN);
        m_address.assign(ipv6);

        memcpy(&m_sockaddr, addr6, sizeof(sockaddr_in6));
    }
}

Client::~Client()
{
    if (m_sendBuffer != nullptr)
        delete m_sendBuffer;

    while (!m_sendBufferQueue.empty()) {
        TcpSendBuffer *buf = m_sendBufferQueue.front();
        delete buf;
        m_sendBufferQueue.pop();
    }
}

TcpSendBuffer *Client::sendBuffer()
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

bool Client::operator==(sockaddr_storage &saddr) const
{
    if (saddr.ss_family != m_sockaddr.ss_family)
        return false;

    if (saddr.ss_family == AF_INET) {
        sockaddr_in *left = reinterpret_cast<sockaddr_in*>(&saddr);
        const sockaddr_in *right = reinterpret_cast<const sockaddr_in*>(&m_sockaddr);

        if (left->sin_addr.s_addr != right->sin_addr.s_addr)
            return false;
    } else if (saddr.ss_family == AF_INET6) {
        sockaddr_in6 *left = reinterpret_cast<sockaddr_in6*>(&saddr);
        const sockaddr_in6 *right = reinterpret_cast<const sockaddr_in6*>(&m_sockaddr);

        if (memcmp(left->sin6_addr.s6_addr, right->sin6_addr.s6_addr, 16) != 0)
            return false;
    }

    return true;
}

void Client::attachSendBuffer(TcpSendBuffer *buffer)
{
    m_sendBufferMutex.lock();
    m_sendBufferQueue.push(buffer);
    m_sendBufferMutex.unlock();
}

TcpPool::TcpPool(ClientStateDelegate clientState, NewConnectionDelegate newConnection, ReceiveDataDelegate receive)
    : m_clientState(clientState), m_newConnection(newConnection), m_receive(receive)
{

}

TcpPool::~TcpPool()
{

}

void TcpPool::callClientState(uint clientid, TcpClientState state, int error) const
{
    m_clientState(clientid, state, error);
}

bool TcpPool::callNewConnection(SharedPtr<Client> &client) const
{
    return m_newConnection(client);
}

void TcpPool::callReceive(uint clientid, PacketHeader header, const Vector<char> &data) const
{
    m_receive(clientid, header, data);
}

Vector<ListenTcpPoolSocket> *TcpPool::listenSockets()
{
    return nullptr;
}

ListenTcpPool::ListenTcpPool(ClientStateDelegate clientState, NewConnectionDelegate newConnection, ReceiveDataDelegate receive)
    : TcpPool(clientState, newConnection, receive)
{

}

ListenTcpPool::~ListenTcpPool()
{
    for (auto &sock : m_sockets)
        close(sock.socket);
}

Vector<ListenTcpPoolSocket> *ListenTcpPool::listenSockets()
{
    return &m_sockets;
}

void ListenTcpPool::appendListenSocket(ConnectionProtocol proto, int socket)
{
    m_sockets.emplace_back(ListenTcpPoolSocket{proto, socket, this});
}

TcpManager::TcpManager(int rounds)
    : m_pools(), m_clients(), m_nextClientId(1), m_rrRounds(rounds)
{
    pipe(m_pipe);
}

TcpManager::~TcpManager()
{
    for (auto &x : m_pools)
        delete x.second;

    for (auto &x : m_clients)
        close(x.second->socket());

    close(m_pipe[0]);
    close(m_pipe[1]);
}

bool TcpManager::connect(ConnectionProtocol proto, const String &ip, u16 port, const String &pool)
{
    auto it = m_pools.find(pool);
    if (it == m_pools.end())
        return false;

    sockaddr_storage saddr;
    if (!SocketUtils::fillSockaddr(&saddr, ip, port, proto))
        return false;

    int socket = ::socket((proto == CPIpv4) ? AF_INET : AF_INET6, SOCK_STREAM, 0);

    if (socket == -1)
        return false;

    if (!SocketUtils::makeNonBlocking(socket))
        return false;

    if (::connect(socket, (sockaddr*) &saddr, (proto == CPIpv4) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)) == -1
            && errno != EINPROGRESS)
        return false;

    TcpManagerConnectInfo *info = new TcpManagerConnectInfo;
    info->socket = socket;
    info->pool = (*it).second;
    memcpy(&info->saddr, &saddr, sizeof(sockaddr_storage));

    TcpManagerMessage notify{};
    notify.type = TMMTConnect;
    notify.clientid = 0;
    notify.options.ptr = info;
    write(m_pipe[1], &notify, sizeof(notify));

    return true;
}

bool TcpManager::connect(const String &address, const String &pool)
{
    u16 port;
    String ip;
    ConnectionProtocol proto = SocketUtils::readAddress(address, port, ip);
    if (proto == CPUnknown)
        return false;

    return connect(proto, ip, port, pool);
}

void TcpManager::createPool(const String &name, TcpPool *pool)
{
    auto it = m_pools.find(name);

    if (it != m_pools.end())
        delete (*it).second;

    m_pools[name] = pool;
}

void TcpManager::disconnect(SharedPtr<Client> &client, bool force)
{
    TcpManagerMessage notify{};
    notify.type = TMMTDisconnect;
    notify.clientid = client->id();
    notify.options.flag = force;
    write(m_pipe[1], &notify, sizeof(notify));
}

void TcpManager::disconnectAll(bool force)
{
    TcpManagerMessage notify{};
    notify.type = TMMTDisconnectAll;
    notify.clientid = 0;
    notify.options.flag = force;
    write(m_pipe[1], &notify, sizeof(notify));
}

void TcpManager::sendTo(SharedPtr<Client> &client, TcpSendBuffer *buffer)
{
    client->attachSendBuffer(buffer);

    TcpManagerMessage notify{};
    notify.type = TMMTSendBufferReceived;
    notify.clientid = client->id();
    notify.options.ptr = 0;
    write(m_pipe[1], &notify, sizeof(notify));
}

void TcpManager::sendTo(SharedPtr<Client> &client, const Packet *packet)
{
    TcpSendBuffer *buffer = new TcpSendBuffer;

    buffer->sent = 0;
    packet->encode(buffer->data);

    sendTo(client, buffer);
}

void TcpManager::sendTo(const Vector<SharedPtr<Client>*> &clients, const Packet *packet)
{
    Vector<char> encoded;
    packet->encode(encoded);

    for (auto *client : clients) {
        TcpSendBuffer *buffer = new TcpSendBuffer;

        buffer->sent = 0;
        buffer->data = encoded;

        sendTo(*client, buffer);
    }
}

void TcpManager::sendTo(const Vector<SharedPtr<Client>> &clients, const Packet *packet)
{
    Vector<char> encoded;
    packet->encode(encoded);

    for (auto client : clients) {
        TcpSendBuffer *buffer = new TcpSendBuffer;

        buffer->sent = 0;
        buffer->data = encoded;

        sendTo(client, buffer);
    }
}

void TcpManager::runLoop()
{
    UniquePtr<Selector> selectScope(Selector::factory());
    Selector *select = selectScope.get();

    select->add(m_pipe[0], TCP_SELECTOR_TYPE_PIPE, 0, SelectorInfo::ReadEvent);

    for (auto &pool : m_pools) {
        Vector<ListenTcpPoolSocket> *sockets = pool.second->listenSockets();

        if (sockets) {
            for (auto &sock : *sockets)
                select->add(sock.socket, TCP_SELECTOR_TYPE_LISTEN, &sock, SelectorInfo::ReadEvent);
        }
    }

    Vector<SelectorEvent> events;
    bool stopLooping = false;

    while (!stopLooping && select->wait(events)) {
        Vector<Client*> eraseList;

        for (auto &event : events) {
            if (event.info()->type() == TCP_SELECTOR_TYPE_LISTEN) {
                newConnection(static_cast<ListenTcpPoolSocket*>(event.info()->data()), select);
            } else if (event.info()->type() == TCP_SELECTOR_TYPE_PIPE) {
                if (event.type() != SelectorInfo::ReadEvent)
                    continue;

                stopLooping = stopLooping || pipeNotification(select, eraseList);
            } else if (event.info()->type() == TCP_SELECTOR_TYPE_CLIENT) {
                Client *client = static_cast<Client*>(event.info()->data());

                if (client->state() == TCSDisconnected && !connectEvent(client, eraseList))
                    continue;

                // writeEvent sam sprawdza czy jest błąd dlatego tutaj nie ma drugiego warunku
                if (event.type() == SelectorInfo::WriteEvent)
                    writeEvent(client, select, eraseList);
                else if (event.type() == SelectorInfo::ReadEvent && client->readError() == 0)
                    readEvent(client, event, select, eraseList);
            }
        }

        // na końcu pozbywamy się klientów usuniętych w tej iteracji
        for (auto *x : eraseList) {
            select->close(x->socket());
            close(x->socket());

            int sockError = x->connectError();
            if (!sockError) {
                sockError = x->writeError();
                if (!sockError)
                    sockError = x->readError();
            }

            x->pool()->callClientState(x->id(), TCSDisconnected, sockError);
            m_clients.erase(x->id());
        }
    }
}

void TcpManager::stopLoop()
{
    TcpManagerMessage notify;
    notify.type = TMMTTerminateLoop;
    notify.clientid = 0;
    notify.options.ptr = 0;
    write(m_pipe[1], &notify, sizeof(notify));
}

SharedPtr<Client> TcpManager::client(uint clientid) const
{
    auto it = m_clients.find(clientid);

    if (it == m_clients.end())
        return nullptr;

    return it->second;
}

void TcpManager::newConnection(ListenTcpPoolSocket *listen, Selector *select)
{
    // przyjmujemy połączenie (ipv6 lub ipv4) oraz ustawiamy socket na tryb nieblokujący
    sockaddr_storage addr;
    socklen_t addrLen = sizeof(sockaddr_storage);
    int sock = accept(listen->socket, (sockaddr*) &addr, &addrLen);

    if (sock == -1)
        return;

    if (!SocketUtils::makeNonBlocking(sock))
        return;

    // tworzymy obiekt klienta i pytamy czy przyjąć połączenie
    SharedPtr<Client> client = std::make_shared<Client>(m_nextClientId++, sock, (sockaddr*) &addr, listen->pool);

    if (listen->pool->callNewConnection(client)) {
        m_clients[client->id()] = client;
        select->add(sock, TCP_SELECTOR_TYPE_CLIENT, client.get(), SelectorInfo::ReadEvent);
    } else {
        close(sock);
    }
}

bool TcpManager::pipeNotification(Selector *select, Vector<Client*> &eraseList)
{
    TcpManagerMessage notify;
    read(m_pipe[0], &notify, sizeof(TcpManagerMessage));

    if (notify.type == TMMTTerminateLoop) {
        return true;
    } else if (notify.type == TMMTConnect) {
        TcpManagerConnectInfo *info = static_cast<TcpManagerConnectInfo*>(notify.options.ptr);

        SharedPtr<Client> client = std::make_shared<Client>(m_nextClientId++, info->socket, (sockaddr*) &info->saddr,
                                                            info->pool);

        client->setState(TCSDisconnected);
        m_clients[client->id()] = client;
        select->add(info->socket, TCP_SELECTOR_TYPE_CLIENT, client.get(), SelectorInfo::ReadEvent | SelectorInfo::WriteEvent);

        delete info;
    } else if (notify.type == TMMTDisconnectAll) {
        for (auto &x : m_clients) {
            Client *client = x.second.get();

            if (notify.options.flag) {
                if (!client->isDead()) {
                    eraseList.push_back(client);
                    client->setDead(true);
                }
            } else if (client->state() == TCSConnected) {
                client->setState(TCSDisconnecting);
            }
        }
    } else {
        auto clientIt = m_clients.find(notify.clientid);
        if (clientIt == m_clients.end())
            return false;
        Client *client = (*clientIt).second.get();

        if (notify.type == TMMTDisconnect || notify.type == TMMTSendBufferReceived) {
            // odpowiednia modyfikacja stanu przy poleceniu rozłączenia
            if (notify.type == TMMTDisconnect) {
                // wymuszenie
                if (notify.options.flag) {
                    if (!client->isDead()) {
                        eraseList.push_back(client);
                        client->setDead(true);
                    }
                } else if (client->state() == TCSConnected) {
                    client->setState(TCSDisconnecting);
                }
            }

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

void TcpManager::readEvent(Client *client, SelectorEvent &event, Selector *select, Vector<Client *> &eraseList, int rounds)
{
    TcpReceiveBuffer &buffer = client->receiveBuffer();
    char *data = buffer.data.data();
    int received = 0;

    // odczyt nagłówka
    if (buffer.received < Packet::HeaderSize) {
        received = read(client->socket(), data + buffer.received, Packet::HeaderSize - buffer.received);

        // kończymy czytam jeśli mamy błąd lub koniec socketa (jeśli druga strona już nam nic nie wyśle)
        if (received == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            return stopReading(client, select, eraseList, errno);
        } else if (received < static_cast<int>(Packet::HeaderSize - buffer.received) &&
                      (event.info()->closed() || client->state() == TCSWritingClosed)) {
            return stopReading(client, select, eraseList, EPIPE);
        } else if (received > 0) {
            buffer.received += received;
        }

        if (buffer.received < Packet::HeaderSize) {
            return;
        } else {
            buffer.header.type = ntohs(*(reinterpret_cast<u16*>(data)));
            buffer.header.payloadSize = ntohl(*(reinterpret_cast<u32*>(data + 2)));

            // pusty pakiet?
            if (buffer.header.payloadSize == 0) {
                client->pool()->callReceive(client->id(), buffer.header, buffer.data);
                buffer.received = 0;
                if (rounds < m_rrRounds)
                    readEvent(client, event, select, eraseList, ++rounds);
                return;
            }
        }
    }

    // nieprawidłowe pakiety
    if (buffer.header.payloadSize > Packet::MaxPayloadSize)
        return stopReading(client, select, eraseList, EYAICINVPACKET);

    received = read(client->socket(), data + buffer.received,
                    buffer.header.payloadSize - (buffer.received - Packet::HeaderSize));

    if (received == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        return stopReading(client, select, eraseList, errno);
    } else if (received < static_cast<int>(buffer.header.payloadSize - (buffer.received - Packet::HeaderSize)) &&
                  (event.info()->closed() || client->state() == TCSWritingClosed)) {
        return stopReading(client, select, eraseList, EPIPE);
    } else if (received > 0) {
        buffer.received += received;
    }

    if (buffer.received >= buffer.header.payloadSize + Packet::HeaderSize) {
        client->pool()->callReceive(client->id(), buffer.header, buffer.data);
        buffer.received = 0;

        // odczytujemy max m_rrRounds zamiast po max 1 pakiecie
        // optymalizacja, troche mniej spamujemy kqueue, epolla waitami
        if (rounds < m_rrRounds)
            readEvent(client, event, select, eraseList, ++rounds);
    }
}

void TcpManager::writeEvent(Client *client, Selector *select, Vector<Client *> &eraseList, int rounds)
{
    TcpSendBuffer *buffer = client->sendBuffer();

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

bool TcpManager::connectEvent(Client *client, Vector<Client *> &eraseList)
{
    // nie powinno się raczej wydarzyć
    if (client->isDead())
        return false;

    int err;
    socklen_t len = sizeof(err);
    getsockopt(client->socket(), SOL_SOCKET, SO_ERROR, &err, &len);

    if (err == 0) {
        client->setState(TCSConnected);
        client->pool()->callClientState(client->id(), TCSConnected, 0);
    } else {
        if (!client->isDead()) {
            eraseList.push_back(client);
            client->setDead(true);
        }

        client->setConnectError(err);
        return false;
    }

    return true;
}

void TcpManager::stopReading(Client *client, Selector *select, Vector<Client *> &eraseList, int error)
{
    client->setReadError(error);
    client->pool()->callClientState(client->id(), TCSDisconnecting, error);

    if (client->sendBuffer() != nullptr)
        select->modify(client->socket(), SelectorInfo::WriteEvent);
    else if (!client->isDead()) {
        eraseList.push_back(client);
        client->setDead(true);
    }
}

void TcpManager::stopWriting(Client *client, Selector *select, Vector<Client *> &eraseList, int error)
{
    if (client->readError() != 0) {
        if (!client->isDead()) {
            eraseList.push_back(client);
            client->setDead(true);
        }
    } else {
        select->modify(client->socket(), SelectorInfo::ReadEvent);

        if (client->state() == TCSDisconnecting) {
            // przy 'lekkim' rozłączeniu generalnie chcemy żeby klient dostał wszystkie nasze wiadomości
            // niestety oznacza to że nie może sobie od razu close() zrobić
            // najlepsze rozwiązanie to zamknięcie strumienia wyjściowego i czekanie z zamknięciem socketa aż klient zamknie własnego
            // dlatego niestety bez heartbeatów się nie obejdzie, w razie timeouta trzeba ostrego disconnecta żeby bezwarunkowo zamknął połączenie
            // info z http://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable
            shutdown(client->socket(), SHUT_WR);

            if (error == 0)
                error = EYAICDISCONNECT;

            client->setState(TCSWritingClosed);
        }
    }

    client->setWriteError(error);
}


END_NAMESPACE
