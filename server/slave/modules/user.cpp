#include <modules/user.h>

#include <core/global.h>
#include <core/context.h>

#include <common/types.h>
#include <server/tcp.h>
#include <server/syseventloop.h>
#include <server/misc_utils.h>
#include <common/packets/slave_user.h>

#include <libconfig.h++>

YAIC_NAMESPACE

UserModule::UserModule(Context *context) :
    m_context(context)
{
    m_config.publicAddress = "127.0.0.1";
    m_config.publicPort = 31411;
    m_config.heartbeatInterval = 1;
    m_config.timeout = 10;
    m_config.capacity = 100;
    m_config.listen.push_back("0.0.0.0:31411");
}

UserModule::~UserModule()
{

}

void UserModule::loadConfig(const libconfig::Setting &section)
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

    if (section.exists("capacity")) {
        const libconfig::Setting &sub = section.lookup("capacity");

        if (sub.isScalar())
            m_config.capacity = sub;
    }
}

bool UserModule::init()
{
    if (!initTcp())
        return false;

    if (!initPackets())
        return false;

    if (!initTimeout())
        return false;

    return true;
}

void UserModule::dispatchPacket(EventPacket *ev)
{
    {
        MutexLock lock(m_lastPacketMutex);
        m_lastPackets[ev->clientid()] = SteadyClock::now();
    }

    m_context->dispatcher->dispatch(ev->clientid(), ev->packet());
}

void UserModule::dispatchTimer(EventTimer *ev)
{
    m_timerDispatcher.dispatch(ev->timer());
}

void UserModule::dispatchSimple(EventSimple *ev)
{
    UNUSED(ev);
}

const String &UserModule::publicAddress() const
{
    return m_config.publicAddress;
}

u16 UserModule::publicPort() const
{
    return m_config.publicPort;
}

uint UserModule::capacity() const
{
    return m_config.capacity;
}

bool UserModule::initPackets()
{
    return true;
}

bool UserModule::initTcp()
{
    ListenTcpPool *pool = new ListenTcpPool(
        BIND_TCP_STATE(this, &UserModule::tcpState),
        BIND_TCP_NEW(this, &UserModule::tcpNew),
        BIND_TCP_RECV(this, &UserModule::tcpReceive)
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

    m_context->tcp->createPool("user-server", pool);

    return true;
}

bool UserModule::initTimeout()
{
    m_timeoutTimer = m_context->sysLoop->addTimer(m_config.timeout);
    m_heartbeatTimer = m_context->sysLoop->addTimer(m_config.heartbeatInterval);

    if (m_timeoutTimer == -1 || m_heartbeatTimer == -1) {
        m_context->log->error("Unable to initialize user module's timers");
        return false;
    }

    m_timerDispatcher.append(m_timeoutTimer, BIND_TIMER(this, &UserModule::timeoutHandler));
    m_timerDispatcher.append(m_heartbeatTimer, BIND_TIMER(this, &UserModule::heartbeatHandler));

    return true;
}

bool UserModule::tcpNew(SharedPtr<Client> &client)
{
    if (!m_context->master->isSynced())
        return false;

    {
        MutexLock lock(m_connectionsMutex);

        auto it = m_connections.find(client->id());
        if (it == m_connections.end())
            m_connections[client->id()] = client;
    }

    m_context->log << Logger::Line::Start
                   << "User connection: [ID: " << client->id() << ", IP: " << client->address() <<"]"
                   << Logger::Line::End;

    return true;
}

void UserModule::tcpState(uint clientid, TcpClientState state, int error)
{
    if (state != TCSDisconnected)
        return;

    {
        MutexLock lock(m_connectionsMutex);
        m_connections.erase(clientid);
    }

    // TODO: Handle user disconnection (jako osobny event rzucić bo dużo tego będzie)
    m_context->log << Logger::Line::Start
                   << "User disconnect: [ID: " << clientid << "]: " << MiscUtils::systemError(error)
                   << Logger::Line::End;
}

void UserModule::tcpReceive(uint clientid, PacketHeader header, const Vector<char> &data)
{
    Packet *packet;

    // jeśli błąd to instant disconnect
    if (!Packet::checkDirection(header.type, Packet::Direction::UserToSlave) ||
            (packet = Packet::factory(header, data)) == nullptr) {
        MutexLock lock(m_connectionsMutex);
        auto it = m_connections.find(clientid);
        if (it != m_connections.end())
            m_context->tcp->disconnect(it->second, true);

        return;
    }

    m_context->eventQueue->append(new EventPacket(packet, clientid, SLAVE_APP_SOURCE_USER));
}

bool UserModule::heartbeatHandler(int timer)
{
    UNUSED(timer);

    SlaveUserPackets::SlaveHeartbeat packet;

    {
        MutexLock lock(m_connectionsMutex);

        Vector<SharedPtr<Client>*> clients;

        for (auto &x : m_connections)
            clients.push_back(&x.second);

        m_context->tcp->sendTo(clients, &packet);
    }

    return true;
}

bool UserModule::timeoutHandler(int timer)
{
    UNUSED(timer);

    auto now = SteadyClock::now();

    MutexLock lock(m_lastPacketMutex);

    for (auto &x : m_lastPackets) {
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now - x.second).count();

        if (seconds >= m_config.timeout) {
            m_context->log << Logger::Line::Start
                           << "User connection timeout [CID:" << x.first << "]: " << seconds << "s"
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

END_NAMESPACE
