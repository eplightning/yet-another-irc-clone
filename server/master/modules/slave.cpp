#include <modules/slave.h>

#include <core/global.h>
#include <core/context.h>
#include <modules/user.h>

#include <common/types.h>
#include <server/tcp.h>
#include <server/syseventloop.h>
#include <server/misc_utils.h>
#include <common/packets/master_slave.h>

#include <libconfig.h++>

YAIC_NAMESPACE

SlaveServer::SlaveServer(SharedPtr<Client> &client, u32 id)
    : m_client(client), m_id(id), m_state(SSUnauthed)
{

}

SharedPtr<Client> &SlaveServer::client()
{
    return m_client;
}

u32 SlaveServer::id() const
{
    return m_id;
}

u16 SlaveServer::port() const
{
    return m_port;
}

uint SlaveServer::capacity() const
{
    return m_capacity;
}

uint SlaveServer::connections() const
{
    return m_load;
}

uint SlaveServer::load() const
{
    if (m_capacity == 0)
        return 0;

    return (m_load * 100) / m_capacity;
}

SlaveState SlaveServer::state() const
{
    return m_state;
}

const String &SlaveServer::name() const
{
    return m_name;
}

long long SlaveServer::lastPacketSeconds(const std::chrono::time_point<SteadyClock> &now)
{
    return std::chrono::duration_cast<std::chrono::seconds>(now - m_lastPacket).count();
}

void SlaveServer::authenticate(uint capacity, u16 port, const String &name)
{
    m_capacity = capacity;
    m_port = port;
    m_name = name;
}

void SlaveServer::setConnections(uint connections)
{
    m_load = connections;
}

void SlaveServer::setState(SlaveState state)
{
    m_state = state;
}

void SlaveServer::updateLastPacket()
{
    m_lastPacket = SteadyClock::now();
}

SlaveModule::SlaveModule(Context *context)
    : m_context(context)
{
    m_config.authMode = SAMNone;
    m_config.plainTextPassword = "";
    m_config.timeout = 10;
    m_config.heartbeatInterval = 1;
}

SlaveModule::~SlaveModule()
{

}

void SlaveModule::loadConfig(const libconfig::Setting &section)
{
    if (section.exists("listen")) {
        const libconfig::Setting &listenSection = section.lookup("listen");

        if (listenSection.isArray()) {
            for (auto &listener : listenSection)
                m_config.listen.push_back(listener);
        } else if (listenSection.isScalar()) {
            m_config.listen.push_back(listenSection);
        }
    }

    if (section.exists("auth-mode")) {
        const libconfig::Setting &sub = section.lookup("auth-mode");

        if (sub.isScalar()) {
            String mode = sub;

            if (mode == "plaintext") {
                m_config.authMode = SAMPlainText;
            } else {
                m_config.authMode = SAMNone;
            }
        }
    }

    if (section.exists("plaintext-password")) {
        const libconfig::Setting &sub = section.lookup("plaintext-password");

        if (sub.isScalar())
            m_config.plainTextPassword = sub.c_str();
    }

    if (section.exists("timeout")) {
        const libconfig::Setting &sub = section.lookup("timeout");

        if (sub.isScalar())
            m_config.timeout = sub;
    }

    if (section.exists("heartbeat")) {
        const libconfig::Setting &sub = section.lookup("heartbeat");

        if (sub.isScalar())
            m_config.heartbeatInterval = sub;
    }
}

bool SlaveModule::init()
{
    if (!initTcp())
        return false;

    if (!initPackets())
        return false;

    if (!initTimers())
        return false;

    return true;
}

void SlaveModule::dispatchPacket(EventPacket *ev)
{
    // TODO: troche ostro blokująca, może osobna strukturka dla czasów
    {
        MutexLock lock(m_slavesMutex);

        auto it = m_slaves.find(ev->clientid());

        if (it != m_slaves.end())
            it->second->updateLastPacket();
    }

    m_context->dispatcher->dispatch(ev->clientid(), ev->packet());
}

void SlaveModule::dispatchTimer(EventTimer *ev)
{
    m_timerDispatcher.dispatch(ev->timer());
}

void SlaveModule::dispatchSimple(EventSimple *ev)
{
    UNUSED(ev);
}

void SlaveModule::dispatchGeneric(Event *ev)
{
    UNUSED(ev);
}

bool SlaveModule::initPackets()
{
    m_context->dispatcher->append(Packet::Type::SlaveHeartbeat,
                                  BIND_DISPATCH(this, &SlaveModule::updateLoad));

    return true;
}

bool SlaveModule::initTcp()
{
    ListenTcpPool *pool = new ListenTcpPool(
        BIND_TCP_STATE(this, &SlaveModule::tcpState),
        BIND_TCP_NEW(this, &SlaveModule::tcpNew),
        BIND_TCP_RECV(this, &SlaveModule::tcpReceive)
    );

    pool->listenSockets()->reserve(m_config.listen.size());

    for (auto &x : m_config.listen) {
        ConnectionProtocol proto;
        int sock = SocketUtils::createListenSocket(x, proto);

        if (sock >= 0)
            pool->appendListenSocket(proto, sock);
        else
            m_context->log << Logger::Line::StartError
                           << "Error while creating listen socket for " << x
                           << Logger::Line::End;
    }

    if (pool->listenSockets()->empty()) {
        delete pool;
        m_context->log->error("Couldn't create any listen socket");
        return false;
    }

    m_context->tcp->createPool("slave-server", pool);

    return true;
}

bool SlaveModule::initTimers()
{
    m_timeoutTimer = m_context->sysLoop->addTimer(m_config.timeout);
    m_heartbeatTimer = m_context->sysLoop->addTimer(m_config.heartbeatInterval);

    if (m_timeoutTimer == -1 || m_heartbeatTimer == -1) {
        m_context->log->error("Unable to initialize timers");
        return false;
    }

    m_timerDispatcher.append(m_timeoutTimer, BIND_TIMER(this, &SlaveModule::timeoutHandler));
    m_timerDispatcher.append(m_heartbeatTimer, BIND_TIMER(this, &SlaveModule::heartbeatHandler));

    return true;
}

SharedPtr<SlaveServer> SlaveModule::getSlave(uint clientid)
{
    MutexLock lock(m_slavesMutex);

    auto it = m_slaves.find(clientid);

    if (it == m_slaves.end())
        return nullptr;

    return it->second;
}

Vector<SharedPtr<SlaveServer>> SlaveModule::getSlaves(bool ipv4, bool ipv6)
{
    Vector<SharedPtr<SlaveServer>> servers;

    MutexLock lock(m_slavesMutex);

    for (auto &x : m_slaves) {
        if (x.second->state() != SSActive)
            continue;

        ConnectionProtocol proto = x.second->client()->proto();

        if ((ipv4 && proto == CPIpv4) || (ipv6 && proto == CPIpv6))
            servers.push_back(x.second);
    }

    std::sort(servers.begin(), servers.end(), [] (const SharedPtr<SlaveServer> &a, const SharedPtr<SlaveServer> &b) {
        return a->load() <= b->load();
    });

    return servers;
}

void SlaveModule::tcpState(uint clientid, TcpClientState state, int error)
{
    if (state == TCSDisconnecting) {
        m_context->log << Logger::Line::Start
                       << "Slave connection lost: [CID: " << clientid << "]"
                       << Logger::Line::End;

        return;
    } else if (state == TCSConnected) {
        return;
    }

    {
        MutexLock lock(m_slavesMutex);

        Vector<SharedPtr<Client>*> clients;

        for (auto it = m_slaves.begin(); it != m_slaves.end();) {
            if (it->first == clientid) {
                it = m_slaves.erase(it);
            } else {
                if (it->second->state() == SSUnauthed || it->second->state() == SSClosing)
                    continue;

                clients.push_back(&it->second->client());
                ++it;
            }
        }

        // TODO: DO IT
        Packet *packet = nullptr;

        m_context->tcp->sendTo(clients, packet);
    }

    m_context->log << Logger::Line::Start
                   << "Slave connection dropped: [CID: " << clientid << "] (" << MiscUtils::systemError(error) << ")"
                   << Logger::Line::End;
}

bool SlaveModule::tcpNew(SharedPtr<Client> &client)
{
    {
        MutexLock lock(m_slavesMutex);

        auto it = m_slaves.find(client->id());
        if (it == m_slaves.end())
            m_slaves[client->id()] = std::make_shared<SlaveServer>(client, client->id());
    }

    m_context->log << Logger::Line::Start
                   << "Slave connection: [ID: " << client->id() << ", IP: " << client->address() <<"]"
                   << Logger::Line::End;

    return true;
}

void SlaveModule::tcpReceive(uint clientid, PacketHeader header, const Vector<char> &data)
{
    Packet *packet;

    // jeśli błąd to instant disconnect
    if (!Packet::checkDirection(header.type, Packet::Direction::SlaveToMaster) ||
            (packet = Packet::factory(header, data)) == nullptr) {
        SharedPtr<SlaveServer> slave = getSlave(clientid);

        if (slave)
            m_context->tcp->disconnect(slave->client(), true);

        return;
    }

    m_context->eventQueue->append(new EventPacket(packet, clientid, MASTER_APP_SOURCE_SLAVE));
}

bool SlaveModule::heartbeatHandler(int timer)
{
    UNUSED(timer);

    MutexLock lock(m_slavesMutex);

    Vector<SharedPtr<Client>*> clients;

    for (auto &x : m_slaves) {
        if (x.second->state() == SSUnauthed || x.second->state() == SSClosing)
            continue;

        clients.push_back(&x.second->client());
    }

    MasterSlavePackets::MasterHeartbeat packet;

    m_context->tcp->sendTo(clients, &packet);

    return true;
}

bool SlaveModule::timeoutHandler(int timer)
{
    UNUSED(timer);

    auto now = SteadyClock::now();

    MutexLock lock(m_slavesMutex);

    for (auto &x : m_slaves) {
        // timeout liczony od momentu połączenia jako że klient jedyne co powinien zrobić to zapytać się o serwery...
        auto seconds = x.second->lastPacketSeconds(now);

        if (seconds >= m_config.timeout) {
            m_context->log << Logger::Line::Start
                           << "Slave timeout: " << seconds << "s"
                           << Logger::Line::End;

            // wymuszony disconnect, slave'a usuwamy w tcpState
            m_context->tcp->disconnect(x.second->client(), true);
        }
    }

    return true;
}

bool SlaveModule::updateLoad(uint clientid, Packet *packet)
{
    MutexLock lock(m_slavesMutex);

    auto it = m_slaves.find(clientid);

    if (it == m_slaves.end())
        return false;

    MasterSlavePackets::SlaveHeartbeat *heartbeat = static_cast<MasterSlavePackets::SlaveHeartbeat*>(packet);

    it->second->setConnections(heartbeat->connections());

    return true;
}


END_NAMESPACE
