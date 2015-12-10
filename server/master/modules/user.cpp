#include <modules/user.h>

#include <modules/slave.h>

#include <common/types.h>
#include <server/tcp.h>
#include <server/syseventloop.h>
#include <server/misc_utils.h>
#include <common/packets/master_user.h>

#include <libconfig.h++>
#include <algorithm>

YAIC_NAMESPACE

User::User(SharedPtr<Client> &client) :
    m_client(client), m_connectedAt(SteadyClock::now())
{

}

SharedPtr<Client> &User::client()
{
    return m_client;
}

const SteadyClock::time_point &User::connectedAt() const
{
    return m_connectedAt;
}

UserModule::UserModule(Context *context) :
    m_context(context)
{
    m_config.timeout = 10;
    m_config.timeoutGranularity = 1;
}

UserModule::~UserModule()
{

}

void UserModule::loadConfig(const libconfig::Setting &section)
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

    if (section.exists("timeout")) {
        const libconfig::Setting &sub = section.lookup("timeout");

        if (sub.isNumber()) {
            m_config.timeout = sub;
            m_config.timeout = m_config.timeout > 0 ? m_config.timeout : 10;
        }
    }

    if (section.exists("timeout-granularity")) {
        const libconfig::Setting &sub = section.lookup("timeout-granularity");

        if (sub.isNumber()) {
            m_config.timeoutGranularity = sub;
            m_config.timeoutGranularity = m_config.timeoutGranularity > 0 ? m_config.timeoutGranularity : 1;
        }
    }
}

bool UserModule::init()
{
    if (!initTcp())
        return false;

    if (!initTimeout())
        return false;

    if (!initPackets())
        return false;

    return true;
}

void UserModule::dispatchPacket(EventPacket *ev)
{
    m_packetDispatcher.dispatch(ev->clientid(), ev->packet());
}

void UserModule::dispatchTimer(EventTimer *ev)
{
    m_timerDispatcher.dispatch(ev->timer());
}

void UserModule::dispatchSimple(EventSimple *ev)
{
    UNUSED(ev);
}

HashMap<uint, SharedPtr<User> > &UserModule::users()
{
    return m_users;
}

std::mutex &UserModule::usersMutex()
{
    return m_usersMutex;
}

SharedPtr<User> UserModule::getUser(uint clientid)
{
    std::lock_guard<std::mutex> lock(m_usersMutex);

    auto it = m_users.find(clientid);

    if (it == m_users.end())
        return nullptr;

    return it->second;
}

bool UserModule::initPackets()
{
    m_packetDispatcher.append(Packet::Type::RequestServers,
                              BIND_DISPATCH(this, &UserModule::serversRequest));

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
    m_timerTimeout = m_context->sysLoop->addTimer(m_config.timeoutGranularity);

    if (m_timerTimeout < 0) {
        m_context->log->error("Couldn't create timer for user timeout");
        return false;
    }

    m_timerDispatcher.append(m_timerTimeout, BIND_TIMER(this, &UserModule::timeoutHandler));

    return true;
}

bool UserModule::timeoutHandler(int timer)
{
    UNUSED(timer);

    auto now = SteadyClock::now();

    std::lock_guard<std::mutex> lock(m_usersMutex);

    for (auto &x : m_users) {
        // timeout liczony od momentu połączenia jako że klient jedyne co powinien zrobić to zapytać się o serwery...
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now - x.second->connectedAt()).count();

        if (seconds >= m_config.timeout) {
            m_context->log << Logger::Line::Start
                           << "Client timeout: " << seconds << "s"
                           << Logger::Line::End;

            m_context->tcp->disconnect(x.second->client(), true);
        }
    }

    return true;
}

void UserModule::tcpState(uint clientid, TcpClientState state, int error)
{
    if (state == TCSDisconnecting) {
        m_context->log << Logger::Line::Start
                       << "Client connection lost: [ID: " << clientid << "]"
                       << Logger::Line::End;

        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_usersMutex);

        auto it = m_users.find(clientid);
        if (it != m_users.end())
            m_users.erase(it);
    }

    m_context->log << Logger::Line::Start
                   << "Client connection dropped: [ID: " << clientid << "] (" << error << ")"
                   << Logger::Line::End;
}

bool UserModule::tcpNew(SharedPtr<Client> &client)
{
    {
        std::lock_guard<std::mutex> lock(m_usersMutex);

        auto it = m_users.find(client->id());
        if (it == m_users.end())
            m_users[client->id()] = std::make_shared<User>(client);
    }

    m_context->log << Logger::Line::Start
                   << "Client connection: [ID: " << client->id() << ", IP: " << client->address() <<"]"
                   << Logger::Line::End;

    return true;
}

void UserModule::tcpReceive(uint clientid, PacketHeader header, const Vector<char> &data)
{
    Packet *packet;

    // jeśli błąd to instant disconnect
    if (!Packet::checkDirection(header.type, Packet::Direction::UserToMaster) ||
            (packet = Packet::factory(header, data)) == nullptr) {
        SharedPtr<User> usr = getUser(clientid);

        if (usr)
            m_context->tcp->disconnect(usr->client(), true);

        return;
    }

    m_context->eventQueue->append(new EventPacket(packet, clientid, MASTER_APP_SOURCE_USER));
}

bool UserModule::serversRequest(uint clientid, Packet *packet)
{
    SharedPtr<User> user = getUser(clientid);
    if (!user)
        return false;

    MasterUserPackets::RequestServers *request = static_cast<MasterUserPackets::RequestServers*>(packet);

    // zbieramy serwery i je sortujemy
    Vector<SharedPtr<SlaveServer>> servers;
    {
        std::lock_guard<std::mutex> lock(m_context->slave->slavesMutex());

        for (auto &x : m_context->slave->slaves()) {
            ConnectionProtocol proto = x.second->client()->proto();

            if (request->flags() & MasterUserPackets::RequestServers::FlagIpv4Only && proto != CPIpv4)
                continue;
            else if (request->flags() & MasterUserPackets::RequestServers::FlagIpv6Only && proto != CPIpv6)
                continue;

            servers.push_back(x.second);
        }

        std::sort(servers.begin(), servers.end(), [] (const SharedPtr<SlaveServer> &a, const SharedPtr<SlaveServer> &b) {
            return a->load() <= b->load();
        });
    }

    // budujemy pakiet
    MasterUserPackets::ServerList response;

    response.servers().reserve(std::min(request->max(), static_cast<u8>(servers.size())));
    for (auto &x : servers) {
        if (response.servers().size() >= request->max())
            break;

        response.servers().emplace_back(x->client()->address(), x->port());
    }

    // wysyłamy
    m_context->tcp->sendTo(user->client(), &response);

    m_context->log << Logger::Line::Start
                   << "Server request served [ID: " << clientid << "]"
                   << Logger::Line::End;

    return true;
}

END_NAMESPACE
