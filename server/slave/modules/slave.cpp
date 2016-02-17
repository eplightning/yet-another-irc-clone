#include <modules/slave.h>

#include <core/global.h>
#include <core/context.h>

#include <common/types.h>
#include <server/tcp.h>
#include <server/syseventloop.h>
#include <server/misc_utils.h>
#include <common/packets/master_slave.h>
#include <common/packets/slave_slave.h>

#include <libconfig.h++>

YAIC_NAMESPACE

SlaveServer::SlaveServer(u32 id, const String &name, bool elder, const String &address, u16 port) :
    m_id(id), m_name(name), m_clientid(0), m_elder(elder), m_address(address), m_port(port)
{

}

u32 SlaveServer::id() const
{
    return m_id;
}

const String &SlaveServer::name() const
{
    return m_name;
}

bool SlaveServer::isElder() const
{
    return m_elder;
}

const String &SlaveServer::address() const
{
    return m_address;
}

u16 SlaveServer::port() const
{
    return m_port;
}

SharedPtr<Client> SlaveServer::client()
{
    MutexLock lock(m_mutex);

    return m_client;
}

u32 SlaveServer::clientid() const
{
    return m_clientid.load();
}

bool SlaveServer::isConnected()
{
    MutexLock lock(m_mutex);

    return m_client.get() != nullptr;
}

void SlaveServer::replaceClient(SharedPtr<Client> &client)
{
    MutexLock lock(m_mutex);

    m_clientid.store(client->id());
    m_client = client;
}

void SlaveServer::replaceClient()
{
    MutexLock lock(m_mutex);

    m_clientid.store(0);
    m_client.reset();
}

SlaveModule::SlaveModule(Context *context) :
    m_context(context)
{
    m_config.publicAddress = "127.0.0.1";
    m_config.publicPort = 31413;
    m_config.heartbeatInterval = 1;
    m_config.reconnectInterval = 5;
    m_config.timeout = 10;
    m_config.listen.push_back("0.0.0.0:31413");
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

    if (section.exists("public-address")) {
        const libconfig::Setting &sub = section.lookup("public-address");

        if (sub.isScalar()) {
            String address;
            u16 port;

            if (SocketUtils::readAddress(sub.c_str(), port, address) != CPUnknown) {
                m_config.publicAddress = address;
                m_config.publicPort = port;
            }
        }
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

    if (section.exists("reconnect")) {
        const libconfig::Setting &sub = section.lookup("reconnect");

        if (sub.isScalar())
            m_config.reconnectInterval = sub;
    }
}

bool SlaveModule::init()
{
    if (!initTcp())
        return false;

    if (!initPackets())
        return false;

    if (!initTimeout())
        return false;

    return true;
}

void SlaveModule::dispatchPacket(EventPacket *ev)
{
    {
        MutexLock lock(m_lastPacketMutex);
        m_lastPackets[ev->clientid()] = SteadyClock::now();
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

const String &SlaveModule::publicAddress() const
{
    return m_config.publicAddress;
}

u16 SlaveModule::publicPort() const
{
    return m_config.publicPort;
}

SharedPtr<SlaveServer> SlaveModule::getSlaveByClientId(u32 clientid)
{
    MutexLock lock(m_slavesMutex);

    for (auto it = m_slaves.begin(); it != m_slaves.end(); ++it) {
        if (it->second->clientid() == clientid)
            return it->second;
    }

    return nullptr;
}

SharedPtr<SlaveServer> SlaveModule::getSlave(u32 id)
{
    MutexLock lock(m_slavesMutex);

    auto it = m_slaves.find(id);

    if (it == m_slaves.end())
        return nullptr;

    return it->second;
}

SharedPtr<Client> SlaveModule::getConnection(u32 clientid)
{
    MutexLock lock(m_connectionsMutex);

    auto it = m_connections.find(clientid);

    if (it == m_connections.end())
        return nullptr;

    return it->second;
}

bool SlaveModule::initPackets()
{
    // from master
    m_context->dispatcher->append(Packet::Type::NewSlave,
                              BIND_DISPATCH(this, &SlaveModule::newSlave));

    m_context->dispatcher->append(Packet::Type::RemoveSlave,
                              BIND_DISPATCH(this, &SlaveModule::removeSlave));

    // from slave
    m_context->dispatcher->append(Packet::Type::SlaveHello,
                              BIND_DISPATCH(this, &SlaveModule::hello));

    m_context->dispatcher->append(Packet::Type::SlaveHelloResponse,
                              BIND_DISPATCH(this, &SlaveModule::helloResponse));

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

bool SlaveModule::initTimeout()
{
    m_timeoutTimer = m_context->sysLoop->addTimer(m_config.timeout);
    m_heartbeatTimer = m_context->sysLoop->addTimer(m_config.heartbeatInterval);
    m_reconnectTimer = m_context->sysLoop->addTimer(m_config.reconnectInterval);

    if (m_timeoutTimer == -1 || m_heartbeatTimer == -1 || m_reconnectTimer == -1) {
        m_context->log->error("Unable to initialize slave module's timers");
        return false;
    }

    m_timerDispatcher.append(m_timeoutTimer, BIND_TIMER(this, &SlaveModule::timeoutHandler));
    m_timerDispatcher.append(m_heartbeatTimer, BIND_TIMER(this, &SlaveModule::heartbeatHandler));
    m_timerDispatcher.append(m_reconnectTimer, BIND_TIMER(this, &SlaveModule::reconnectHandler));

    return true;
}

void SlaveModule::tcpState(uint clientid, TcpClientState state, int error)
{
    if (state == TCSConnected) {
        SharedPtr<Client> client = m_context->tcp->client(clientid);

        {
            MutexLock lock(m_connectionsMutex);
            m_connections[client->id()] = client;
        }

        // jeśli łączyliśmy się z jakimkolwiek slave'm to połączenie z masterem już było
        SlaveSlavePackets::Hello packet;
        packet.setAuthPassword(m_context->master->getAuthPassword());
        packet.setId(m_context->master->getSlaveId());
        packet.setName(m_context->slaveName);

        m_context->tcp->sendTo(client, &packet);

        m_context->log << Logger::Line::Start
                       << "Slave connection established: [ID: " << clientid << "]"
                       << Logger::Line::End;

        return;
    } else if (state == TCSDisconnected) {
        {
            MutexLock lock(m_lastPacketMutex);
            m_lastPackets.erase(clientid);
        }

        {
            MutexLock lock(m_connectionsMutex);
            m_connections.erase(clientid);
        }

        SharedPtr<SlaveServer> srv = getSlaveByClientId(clientid);

        if (!srv)
            return;

        srv->replaceClient();

        m_context->log << Logger::Line::Start
                       << "Slave connection dropped / reconnection failed: [ID: " << srv->id() << ", Name: " << srv->name() <<
                       "]: (" << MiscUtils::systemError(error) << ")"
                       << Logger::Line::End;

        if (srv->isElder())
            return;

        {
            MutexLock lock(m_reconnectMutex);
            m_reconnectQueue.push(srv->id());
        }
    }
}

bool SlaveModule::tcpNew(SharedPtr<Client> &client)
{
    if (!m_context->master->isAuthed())
        return false;

    {
        MutexLock lock(m_connectionsMutex);

        auto it = m_connections.find(client->id());
        if (it == m_connections.end())
            m_connections[client->id()] = client;
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
    if (!Packet::checkDirection(header.type, Packet::Direction::SlaveToSlave) ||
            (packet = Packet::factory(header, data)) == nullptr) {
        MutexLock lock(m_connectionsMutex);
        auto it = m_connections.find(clientid);
        if (it != m_connections.end())
            m_context->tcp->disconnect(it->second, true);

        return;
    }

    m_context->eventQueue->append(new EventPacket(packet, clientid, SLAVE_APP_SOURCE_SLAVE));
}

void SlaveModule::establishConnection(SharedPtr<SlaveServer> &slave)
{
    if (!m_context->tcp->connect(SocketUtils::getProtoFromIp(slave->address()), slave->address(), slave->port(), "slave-server")) {
        m_context->log << Logger::Line::StartError
                       << "Slave connection attempt failed: [ID: " << slave->id() << ", Name: " << slave->name() <<"]"
                       << Logger::Line::End;

        {
            MutexLock lock(m_reconnectMutex);
            m_reconnectQueue.push(slave->id());
        }
    } else {
        m_context->log << Logger::Line::Start
                       << "Slave connection attempt ... : [ID: " << slave->id() << ", Name: " << slave->name() <<"]"
                       << Logger::Line::End;
    }
}

void SlaveModule::synchronize(SharedPtr<Client> &client)
{
    // TODO:
}

bool SlaveModule::heartbeatHandler(int timer)
{
    UNUSED(timer);

    Vector<SharedPtr<Client>> clients;

    {
        MutexLock lock(m_slavesMutex);

        for (auto &x : m_slaves) {
            SharedPtr<Client> client = x.second->client();

            if (client)
                clients.push_back(client);
        }
    }

    SlaveSlavePackets::Heartbeat packet;

    m_context->tcp->sendTo(clients, &packet);

    return true;
}

bool SlaveModule::timeoutHandler(int timer)
{
    UNUSED(timer);

    auto now = SteadyClock::now();

    MutexLock lock(m_lastPacketMutex);

    for (auto &x : m_lastPackets) {
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now - x.second).count();

        if (seconds >= m_config.timeout) {
            m_context->log << Logger::Line::Start
                           << "Slave connection timeout [CID:" << x.first << "]: " << seconds << "s"
                           << Logger::Line::End;

            {
                MutexLock lock2(m_connectionsMutex);

                auto it = m_connections.find(x.first);

                if (it != m_connections.end())
                    m_context->tcp->disconnect(it->second, true);
            }
        }
    }

    return true;
}

bool SlaveModule::reconnectHandler(int timer)
{
    UNUSED(timer);

    std::queue<u32> queueCopy;

    {
        MutexLock lock(m_reconnectMutex);
        m_reconnectQueue.swap(queueCopy);
    }

    while (!queueCopy.empty()) {
        u32 slaveid = queueCopy.front();
        queueCopy.pop();

        SharedPtr<SlaveServer> srv = getSlave(slaveid);

        if (!srv)
            continue;

        establishConnection(srv);
    }

    return true;
}

bool SlaveModule::newSlave(uint clientid, Packet *packet)
{
    UNUSED(clientid);

    if (!m_context->master->isAuthed())
        return false;

    SharedPtr<Client> master = m_context->master->getMaster();
    if (!master)
        return false;

    MasterSlavePackets::NewSlave *request = static_cast<MasterSlavePackets::NewSlave*>(packet);

    SharedPtr<SlaveServer> srv = std::make_shared<SlaveServer>(request->id(), request->name(), false, request->address(),
                                                               request->port());

    {
        MutexLock lock(m_slavesMutex);
        m_slaves[request->id()] = srv;
    }

    establishConnection(srv);

    return true;
}

bool SlaveModule::removeSlave(uint clientid, Packet *packet)
{
    UNUSED(clientid);

    if (!m_context->master->isAuthed())
        return false;

    MasterSlavePackets::RemoveSlave *request = static_cast<MasterSlavePackets::RemoveSlave*>(packet);

    SharedPtr<SlaveServer> srv;
    {
        MutexLock lock(m_slavesMutex);

        auto it = m_slaves.find(request->id());

        if (it == m_slaves.end())
            return true;

        srv = it->second;

        m_slaves.erase(it);
    }

    // TODO: Handle removal

    SharedPtr<Client> client = srv->client();
    if (client)
        m_context->tcp->disconnect(client, true);

    m_context->log << Logger::Line::Start
                   << "Slave removed, as requested by the master server: [ID: " << request->id() << ", Name: "
                   << srv->name() << "]"
                   << Logger::Line::End;

    return true;
}

bool SlaveModule::hello(uint clientid, Packet *packet)
{
    SharedPtr<Client> master = m_context->master->getMaster();
    if (!master)
        return false;

    if (!m_context->master->isAuthed())
        return false;

    SharedPtr<Client> client = getConnection(clientid);
    if (!client)
        return false;

    SlaveSlavePackets::Hello *request = static_cast<SlaveSlavePackets::Hello*>(packet);

    if (request->authPassword() != m_context->master->getAuthPassword()) {
        m_context->log << Logger::Line::Start
                       << "Slave hello refused: [ID: " << request->id() << "]: Invalid auth password"
                       << Logger::Line::End;

        return false;
    }

    bool synchronizeNeeded = false;

    {
        MutexLock lock(m_slavesMutex);

        auto it = m_slaves.find(request->id());

        if (it != m_slaves.end()) {
            if (!it->second->isElder()) {
                m_context->log << Logger::Line::Start
                               << "Slave hello refused: [ID: " << request->id() << "]: Hello reconnect from not elder"
                               << Logger::Line::End;

                return false;
            } else {
                synchronizeNeeded = true;
                it->second->replaceClient(client);
            }
        } else {
            SharedPtr<SlaveServer> srv = std::make_shared<SlaveServer>(request->id(), request->name(), true, client->address(),
                                                                       client->port());

            srv->replaceClient(client);
            m_slaves[request->id()] = srv;
        }
    }

    SlaveSlavePackets::HelloResponse response(m_context->master->getSlaveId());
    response.setAuthPassword(m_context->master->getAuthPassword());
    m_context->tcp->sendTo(client, &response);

    if (synchronizeNeeded)
        synchronize(client);

    return true;
}

bool SlaveModule::helloResponse(uint clientid, Packet *packet)
{
    UNUSED(packet);

    SharedPtr<Client> master = m_context->master->getMaster();
    if (!master)
        return false;

    if (!m_context->master->isAuthed())
        return false;

    SharedPtr<Client> client = getConnection(clientid);
    if (!client)
        return false;

    SlaveSlavePackets::HelloResponse *request = static_cast<SlaveSlavePackets::HelloResponse*>(packet);

    if (request->authPassword() != m_context->master->getAuthPassword())
        return false;

    SharedPtr<SlaveServer> slave = getSlave(request->id());
    if (!slave)
        return false;

    if (slave->isElder())
        return false;

    slave->replaceClient(client);

    synchronize(client);

    MasterSlavePackets::NewAck ack(slave->id());
    m_context->tcp->sendTo(master, &ack);

    m_context->log << Logger::Line::Start
                   << "Slave acknowledged: " << slave->id()
                   << Logger::Line::End;

    return true;
}

END_NAMESPACE
