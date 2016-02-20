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

#include <random>
#include <algorithm>

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

u16 SlaveServer::userPort() const
{
    return m_userPort;
}

void SlaveServer::setUserPort(u16 userPort)
{
    m_userPort = userPort;
}

u16 SlaveServer::slavePort() const
{
    return m_slavePort;
}

void SlaveServer::setSlavePort(u16 slavePort)
{
    m_slavePort = slavePort;
}

const String &SlaveServer::userAddress() const
{
    return m_userAddress;
}

void SlaveServer::setUserAddress(const String &userAddress)
{
    m_userAddress = userAddress;
}

const String& SlaveServer::slaveAddress() const
{
    return m_slaveAddress;
}

void SlaveServer::setSlaveAddress(const String &slaveAddress)
{
    m_slaveAddress = slaveAddress;
}

void SlaveServer::setCapacity(uint capacity)
{
    m_capacity = capacity;
}

void SlaveServer::setName(const String &name)
{
    m_name.assign(name);
}

SlaveModule::SlaveModule(Context *context)
    : m_context(context)
{
    m_config.authMode = MasterSlavePackets::Auth::Mode::None;
    m_config.plainTextPassword = "";
    m_config.timeout = 10;
    m_config.heartbeatInterval = 1;
    m_config.listen.push_back("0.0.0.0:31412");
}

SlaveModule::~SlaveModule()
{

}

void SlaveModule::loadConfig(const libconfig::Setting &section)
{
    if (section.exists("listen")) {
        const libconfig::Setting &listenSection = section.lookup("listen");

        // delete default configuration
        m_config.listen.clear();

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
                m_config.authMode = MasterSlavePackets::Auth::Mode::Plaintext;
            } else {
                m_config.authMode = MasterSlavePackets::Auth::Mode::None;
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

        m_config.timeout = m_config.timeout > 0 ? m_config.timeout : 10;
    }

    if (section.exists("heartbeat")) {
        const libconfig::Setting &sub = section.lookup("heartbeat");

        if (sub.isScalar())
            m_config.heartbeatInterval = sub;

        m_config.heartbeatInterval = m_config.heartbeatInterval > 0 ? m_config.heartbeatInterval : 1;
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

    // auth password
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<u64> dist;
    m_authPassword = dist(rng);

    return true;
}

void SlaveModule::dispatchPacket(EventPacket *ev)
{
    //  troche ostro blokująca, może osobna strukturka dla czasów
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

bool SlaveModule::initPackets()
{
    m_context->dispatcher->append(Packet::Type::SlaveHeartbeat,
                                  BIND_DISPATCH(this, &SlaveModule::updateLoad));

    m_context->dispatcher->append(Packet::Type::SlaveAuth,
                                  BIND_DISPATCH(this, &SlaveModule::auth));

    m_context->dispatcher->append(Packet::Type::SlaveSyncStart,
                                  BIND_DISPATCH(this, &SlaveModule::syncStart));

    m_context->dispatcher->append(Packet::Type::SlaveNewAck,
                                  BIND_DISPATCH(this, &SlaveModule::newAck));

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
        m_context->log->error("Unable to initialize slave module's timers");
        return false;
    }

    m_timerDispatcher.append(m_timeoutTimer, BIND_TIMER(this, &SlaveModule::timeoutHandler));
    m_timerDispatcher.append(m_heartbeatTimer, BIND_TIMER(this, &SlaveModule::heartbeatHandler));

    return true;
}

SharedPtr<SlaveServer> SlaveModule::get(u32 clientid)
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

void SlaveModule::tcpState(u32 clientid, TcpClientState state, int error)
{
    if (state != TCSDisconnected)
        return;

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

        MasterSlavePackets::RemoveSlave packet(clientid);

        m_context->tcp->sendTo(clients, &packet);
    }

    m_context->log << Logger::Line::Start
                   << "Slave connection dropped: [ID: " << clientid << "] (" << MiscUtils::systemError(error) << ")"
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

void SlaveModule::tcpReceive(u32 clientid, PacketHeader header, const Vector<char> &data)
{
    Packet *packet;

    // jeśli błąd to instant disconnect
    if (!Packet::checkDirection(header.type, Packet::Direction::SlaveToMaster) ||
            (packet = Packet::factory(header, data)) == nullptr) {
        SharedPtr<SlaveServer> slave = get(clientid);

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

bool SlaveModule::updateLoad(u32 clientid, Packet *packet)
{
    MutexLock lock(m_slavesMutex);

    auto it = m_slaves.find(clientid);

    if (it == m_slaves.end())
        return false;

    MasterSlavePackets::SlaveHeartbeat *heartbeat = static_cast<MasterSlavePackets::SlaveHeartbeat*>(packet);

    it->second->setConnections(heartbeat->connections());

    return true;
}

bool SlaveModule::auth(u32 clientid, Packet *packet)
{
    MutexLock lock(m_slavesMutex);

    auto it = m_slaves.find(clientid);

    if (it == m_slaves.end() || it->second->state() != SSUnauthed)
        return false;

    SlaveServer *srv = it->second.get();

    MasterSlavePackets::Auth *request = static_cast<MasterSlavePackets::Auth*>(packet);

    // authentication
    if (m_config.authMode != request->mode()) {
        MasterSlavePackets::AuthResponse response;

        response.setId(0);
        response.setAuthPassword(0);
        response.setStatus(MasterSlavePackets::AuthResponse::Status::InvalidMode);

        m_context->tcp->sendTo(srv->client(), &response);

        m_context->log << Logger::Line::Start
                       << "Slave not authenticated: [ID: " << srv->id() << "] (Invalid auth mode)"
                       << Logger::Line::End;

        return false;
    } else if (m_config.authMode == MasterSlavePackets::Auth::Mode::Plaintext) {
        if (m_config.plainTextPassword != request->plaintextPassword()) {
            MasterSlavePackets::AuthResponse response;

            response.setId(0);
            response.setAuthPassword(0);
            response.setStatus(MasterSlavePackets::AuthResponse::Status::InvalidPlaintextPassword);

            m_context->tcp->sendTo(srv->client(), &response);

            m_context->log << Logger::Line::Start
                           << "Slave not authenticated: [ID: " << srv->id() << "] (Invalid password)"
                           << Logger::Line::End;

            return false;
        }
    }

    // save info
    srv->setCapacity(request->capacity());
    srv->setSlaveAddress(request->slaveAddress());
    srv->setSlavePort(request->slavePort());
    srv->setUserAddress(request->userAddress());
    srv->setUserPort(request->userPort());
    srv->setName(request->name());
    srv->setState(SSAuthed);

    // response
    MasterSlavePackets::AuthResponse response;
    response.setId(srv->id());
    response.setAuthPassword(m_authPassword);
    response.setStatus(MasterSlavePackets::AuthResponse::Status::Ok);
    m_context->tcp->sendTo(srv->client(), &response);

    m_context->log << Logger::Line::Start
                   << "Slave authenticated: [ID: " << srv->id() << ", Name: " << srv->name() << "]"
                   << Logger::Line::End;

    return true;
}

bool SlaveModule::syncStart(u32 clientid, Packet *packet)
{
    UNUSED(packet);

    MutexLock lock(m_slavesMutex);

    auto it = m_slaves.find(clientid);
    if (it == m_slaves.end() || it->second->state() != SSAuthed)
        return false;

    Vector<SharedPtr<Client>*> clients;

    for (auto it2 = m_slaves.begin(); it2 != m_slaves.end(); ++it2) {
        if (it2->first != clientid) {
            if (it2->second->state() != SSActive && it2->second->state() != SSSyncing)
                continue;

            clients.push_back(&it2->second->client());
        }
    }

    if (clients.empty()) {
        MasterSlavePackets::SyncEnd response;
        m_context->tcp->sendTo(it->second->client(), &response);

        it->second->setState(SSActive);

        m_context->log << Logger::Line::Start
                       << "Slave synchronized and ready to accept users: [ID: " << it->second->id() << ", Name: " << it->second->name() << "]"
                       << Logger::Line::End;
    } else {
        MasterSlavePackets::NewSlave msg;
        msg.setAddress(it->second->slaveAddress());
        msg.setPort(it->second->slavePort());
        msg.setId(it->first);
        msg.setName(it->second->name());

        m_context->tcp->sendTo(clients, &msg);

        it->second->setState(SSSyncing);

        // jakiś lepszy mechanizm, bo ten się wyłoży jak np. jakiś slave się rozłączy zanim to wyśle i nowy slave nigdy nie będzie gotowy
        // jakaś lista slave'ów i tyle
        m_syncProgress[it->first] = static_cast<int>(clients.size());

        m_context->log << Logger::Line::Start
                       << "Slave starting synchronization: [ID: " << it->second->id() << ", Name: " << it->second->name() << "]"
                       << Logger::Line::End;
    }

    return true;
}

bool SlaveModule::newAck(u32 clientid, Packet *packet)
{
    MutexLock lock(m_slavesMutex);

    auto it = m_slaves.find(clientid);

    if (it == m_slaves.end() || it->second->state() == SSAuthed || it->second->state() == SSUnauthed)
        return false;

    MasterSlavePackets::NewAck *request = static_cast<MasterSlavePackets::NewAck*>(packet);

    // find syncing slave
    auto it2 = m_slaves.find(request->id());
    if (it2 == m_slaves.end() || it2->second->state() != SSSyncing)
        return false;

    m_context->log << Logger::Line::Start
                   << "Old slave acknowledged new slave: [Old ID: " << it->second->id() << ", New ID: " << it2->second->id() << "]"
                   << Logger::Line::End;

    // find sync progress
    auto it3 = m_syncProgress.find(request->id());

    if (it3 == m_syncProgress.end() || it3->second <= 1) {
        m_syncProgress.erase(request->id());

        MasterSlavePackets::SyncEnd response;
        m_context->tcp->sendTo(it2->second->client(), &response);

        it2->second->setState(SSActive);

        m_context->log << Logger::Line::Start
                       << "Slave synchronized and ready to accept users: [ID: " << it2->second->id() << ", Name: " << it2->second->name() << "]"
                       << Logger::Line::End;
    } else {
        it3->second--;
    }

    return true;
}


END_NAMESPACE
