#include <modules/user.h>

#include <core/global.h>
#include <core/context.h>

#include <common/types.h>
#include <server/tcp.h>
#include <server/syseventloop.h>
#include <server/misc_utils.h>
#include <common/packets/slave_user.h>
#include <common/packets/slave_slave.h>
#include <components/users.h>
#include <components/channels.h>

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

        auto it = m_lastPackets.find(ev->clientid());

        if (it != m_lastPackets.end())
            it->second = SteadyClock::now();
    }

    m_context->dispatcher->dispatch(ev->clientid(), ev->packet());
}

void UserModule::dispatchTimer(EventTimer *ev)
{
    m_timerDispatcher.dispatch(ev->timer());
}

void UserModule::dispatchSimple(EventSimple *ev)
{
    if (ev->id() == EventSimple::EventId::UserDisconnected || ev->id() == EventSimple::EventId::UserForeignDisconnected) {
        u64 id;

        if (ev->id() == EventSimple::EventId::UserDisconnected)
            id = m_users.getFullId(ev->param().clientid);
        else
            id = ev->param().id;

        SharedPtr<User> user = m_users.findById(id);

        if (!user)
            return;

        m_context->log << Logger::Line::Start
                       << "Removing user: [Nick: " << user->nick() << "]"
                       << Logger::Line::End;

        // Remove user from list and channels
        m_users.removeUser(id);

        for (auto &x : m_channels.list())
            x.second->removeUser(user);

        // Tell users about disconnect
        SlaveUserPackets::UserDisconnected dc(id);
        Vector<SharedPtr<Client>*> clients;

        for (auto &x : m_users.list()) {
            if (x.second->isLocal())
                clients.push_back(&x.second->client());
        }

        m_context->tcp->sendTo(clients, &dc);

        if (ev->id() == EventSimple::EventId::UserDisconnected) {
            SlaveSlavePackets::UserDisconnect dc(id);
            m_context->slave->broadcast(&dc);
        }
    }
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

uint UserModule::load() const
{
    return m_users.count();
}

void UserModule::slaveIdReceived(u32 id)
{
    m_users.setSlaveId(id);
    m_channels.setSlaveId(id);
}

SharedPtr<Client> UserModule::connection(u32 clientid)
{
    MutexLock lock(m_connectionsMutex);

    auto it = m_connections.find(clientid);

    if (it == m_connections.end())
        return nullptr;

    return it->second;
}

void UserModule::cleanupSlave(u32 slave)
{
    // All users
    Vector<SharedPtr<Client>*> allUsers;
    for (auto &x : m_users.list()) {
        if (x.second->isLocal())
            allUsers.push_back(&x.second->client());
    }

    // User disconnects
    for (auto it = m_users.list().begin(); it != m_users.list().end();) {
        if (it->second->slaveId() == slave) {
            SlaveUserPackets::UserDisconnected dc(it->second->id());
            m_context->tcp->sendTo(allUsers, &dc);

            it = m_users.list().erase(it);
        } else {
            ++it;
        }
    }

    // Channel removals
    for (auto it = m_channels.list().begin(); it != m_channels.list().end();) {
        if (it->second->slaveId() == slave) {
            SlaveUserPackets::ChannelParted parted;
            parted.setId(it->second->id());
            parted.setReason(SlaveUserPackets::ChannelParted::Reason::Unknown);
            parted.setStatus(SlaveUserPackets::ChannelParted::Status::Ok);

            Vector<SharedPtr<Client>*> allChannelUsers;
            for (auto &x : it->second->users()) {
                if (x.second->user()->isLocal())
                    allChannelUsers.push_back(&x.second->user()->client());
            }
            m_context->tcp->sendTo(allChannelUsers, &parted);

            it = m_channels.list().erase(it);
        } else {
            ++it;
        }
    }
}

void UserModule::syncSlave(SharedPtr<Client> &client)
{
    SlaveSlavePackets::SyncUsers users;

    for (auto &x : m_users.list()) {
        if (x.second->isLocal())
            users.addUser(x.second->id(), x.second->nick());
    }

    m_context->tcp->sendTo(client, &users);

    SlaveSlavePackets::SyncChannels chans;

    for (auto &x : m_channels.list()) {
        if (x.second->isLocal()) {
            Vector<SlaveSlavePackets::SyncChannelUser> chanusers;

            for (auto &y : x.second->users())
                chanusers.emplace_back(y.second->user()->id(), y.second->flags());

            chans.addChannel(x.second->id(), x.second->name(), chanusers);
        }
    }

    m_context->tcp->sendTo(client, &chans);
}

bool UserModule::initPackets()
{
    // user
    m_context->dispatcher->append(Packet::Type::Handshake,
                                  BIND_DISPATCH(this, &UserModule::handshake));

    m_context->dispatcher->append(Packet::Type::ListChannels,
                                  BIND_DISPATCH(this, &UserModule::channelList));

    m_context->dispatcher->append(Packet::Type::JoinChannel,
                                  BIND_DISPATCH(this, &UserModule::joinChannel));

    m_context->dispatcher->append(Packet::Type::PartChannel,
                                  BIND_DISPATCH(this, &UserModule::partChannel));

    m_context->dispatcher->append(Packet::Type::SendChannelMessage,
                                  BIND_DISPATCH(this, &UserModule::messageChannel));

    m_context->dispatcher->append(Packet::Type::SendPrivateMessage,
                                  BIND_DISPATCH(this, &UserModule::privateMessage));

    // slave
    m_context->dispatcher->append(Packet::Type::SlaveSyncUsers,
                                  BIND_DISPATCH(this, &UserModule::slaveSyncUsers));

    m_context->dispatcher->append(Packet::Type::SlaveSyncChannels,
                                  BIND_DISPATCH(this, &UserModule::slaveSyncChannels));

    m_context->dispatcher->append(Packet::Type::SlaveUserConnect,
                                  BIND_DISPATCH(this, &UserModule::slaveUserConnect));

    m_context->dispatcher->append(Packet::Type::SlaveUserDisconnect,
                                  BIND_DISPATCH(this, &UserModule::slaveUserDisconnect));

    m_context->dispatcher->append(Packet::Type::SlaveChannelNew,
                                  BIND_DISPATCH(this, &UserModule::slaveChannelNew));

    m_context->dispatcher->append(Packet::Type::SlaveChannelRemove,
                                  BIND_DISPATCH(this, &UserModule::slaveChannelRemove));

    m_context->dispatcher->append(Packet::Type::SlaveChannelUser,
                                  BIND_DISPATCH(this, &UserModule::slaveChannelUser));

    m_context->dispatcher->append(Packet::Type::SlaveChannelUserPart,
                                  BIND_DISPATCH(this, &UserModule::slaveChannelUserPart));

    m_context->dispatcher->append(Packet::Type::SlaveChannelMessage,
                                  BIND_DISPATCH(this, &UserModule::slaveChannelMessage));

    m_context->dispatcher->append(Packet::Type::SlavePrivateMessage,
                                  BIND_DISPATCH(this, &UserModule::slavePrivateMessage));

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
        m_context->log->error("Couldn't create any listen socket for user module");
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

    {
        MutexLock lock(m_lastPacketMutex);
        m_lastPackets[client->id()] = SteadyClock::now();
    }

    m_context->log << Logger::Line::Start
                   << "User connection: [ID: " << client->id() << ", IP: " << client->address() <<"]"
                   << Logger::Line::End;

    return true;
}

void UserModule::tcpState(u32 clientid, TcpClientState state, int error)
{
    if (state != TCSDisconnected)
        return;

    {
        MutexLock lock(m_connectionsMutex);
        m_connections.erase(clientid);
    }

    {
        MutexLock lock(m_lastPacketMutex);
        m_lastPackets.erase(clientid);
    }

    m_context->log << Logger::Line::Start
                   << "User disconnect: [CID: " << clientid << "]: " << MiscUtils::systemError(error)
                   << Logger::Line::End;

    m_context->eventQueue->append(new EventSimple(EventSimple::EventId::UserDisconnected, clientid));
}

void UserModule::tcpReceive(u32 clientid, PacketHeader header, const Vector<char> &data)
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

void UserModule::heartbeatHandler(int timer)
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
}

void UserModule::timeoutHandler(int timer)
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
}

void UserModule::handshake(u32 clientid, Packet *packet)
{
    SharedPtr<Client> client = connection(clientid);

    if (!client)
        return;

    SlaveUserPackets::Handshake *request = static_cast<SlaveUserPackets::Handshake*>(packet);

    SlaveUserPackets::HandshakeAck ack;
    ack.setUserId(0);

    if (m_users.count() >= m_config.capacity) {
        ack.setStatus(SlaveUserPackets::HandshakeAck::Status::Full);
        m_context->tcp->sendTo(client, &ack);
        return;
    }

    if (!m_users.isValidNick(request->nick()) || m_users.findByNick(request->nick())) {
        ack.setStatus(SlaveUserPackets::HandshakeAck::Status::InvalidNick);
        m_context->tcp->sendTo(client, &ack);
        return;
    }

    SharedPtr<User> user = m_users.addUser(client->id(), request->nick(), client);

    ack.setStatus(SlaveUserPackets::HandshakeAck::Status::Ok);
    ack.setUserId(user->id());

    m_context->tcp->sendTo(client, &ack);

    m_context->log << Logger::Line::Start
                   << "User handshake handled [User: " << user->nick() << "]"
                   << Logger::Line::End;

    SlaveSlavePackets::UserConnect notification;
    notification.setId(user->id());
    notification.setNick(user->nick());
    m_context->slave->broadcast(&notification);
}

void UserModule::channelList(u32 clientid, Packet *packet)
{
    UNUSED(packet);

    SharedPtr<Client> client = connection(clientid);

    if (!client)
        return;

    SharedPtr<User> user = m_users.findById(clientid);

    if (!user)
        return;

    SlaveUserPackets::Channels response;

    for (auto &x : m_channels.list())
        response.addChannel(x.second->name());

    m_context->tcp->sendTo(client, &response);

    m_context->log << Logger::Line::Start
                   << "Channel list handled [User: " << user->nick() << "]"
                   << Logger::Line::End;
}

void UserModule::joinChannel(u32 clientid, Packet *packet)
{
    SharedPtr<Client> client = connection(clientid);

    if (!client)
        return;

    SharedPtr<User> user = m_users.findById(clientid);

    if (!user)
        return;

    SlaveUserPackets::JoinChannel *request = static_cast<SlaveUserPackets::JoinChannel*>(packet);

    if (request->name().empty())
        return;

    SharedPtr<Channel> chan = m_channels.findByName(request->name());

    SlaveUserPackets::ChannelJoined response;
    response.setId(0);
    response.setName(request->name());
    response.setUserFlags(0);
    response.setStatus(SlaveUserPackets::ChannelJoined::Status::Ok);

    s32 flags = 0;

    if (chan) {
        if (!chan->user(user->id())) {
            response.setId(chan->id());

            SlaveUserPackets::ChannelUserJoined notification;
            notification.user().flags = 0;
            notification.user().id = user->id();
            notification.user().nick = user->nick();
            notification.setChannel(chan->id());

            Vector<SharedPtr<Client>*> clients;

            for (auto &x : chan->users()) {
                ChannelUser *chanuser = x.second.get();
                User *chanuser2 = chanuser->user().get();

                response.addUser(chanuser2->id(), chanuser->flags(), chanuser2->nick());

                if (chanuser2->isLocal())
                    clients.push_back(&chanuser2->client());
            }

            m_context->tcp->sendTo(clients, &notification);

            chan->addUser(user);
        } else {
            response.setStatus(SlaveUserPackets::ChannelJoined::Status::UnknownError);
        }
    } else {
        chan = m_channels.create(request->name(), user);

        if (!chan) {
            response.setStatus(SlaveUserPackets::ChannelJoined::Status::UnknownError);
        } else {
            response.setId(chan->id());
            response.setUserFlags(SlaveUserPackets::ChanUser::FlagOperator);
            flags = SlaveUserPackets::ChanUser::FlagOperator;

            SlaveSlavePackets::ChannelNew notification;
            notification.setId(chan->id());
            notification.setName(chan->name());
            m_context->slave->broadcast(&notification);
        }
    }

    m_context->tcp->sendTo(client, &response);

    m_context->log << Logger::Line::Start
                   << "Channel join handled [From: " << user->nick() << ", To: "
                   << chan->name() << " (" << chan->id() << "), Local: " << chan->isLocal() << "]"
                   << Logger::Line::End;

    SlaveSlavePackets::ChannelUser notification;
    notification.setChannel(chan->id());
    notification.setUser(user->id());
    notification.setFlags(flags);
    m_context->slave->broadcast(&notification);
}

void UserModule::partChannel(u32 clientid, Packet *packet)
{
    SharedPtr<Client> client = connection(clientid);

    if (!client)
        return;

    SharedPtr<User> user = m_users.findById(clientid);

    if (!user)
        return;

    SlaveUserPackets::PartChannel *request = static_cast<SlaveUserPackets::PartChannel*>(packet);

    SlaveUserPackets::ChannelParted response;
    response.setId(request->id());
    response.setReason(SlaveUserPackets::ChannelParted::Reason::Requested);
    response.setStatus(SlaveUserPackets::ChannelParted::Status::Ok);

    SharedPtr<Channel> chan = m_channels.findById(request->id());

    if (!chan || !chan->user(user->id())) {
        response.setStatus(SlaveUserPackets::ChannelParted::Status::UnknownError);
        m_context->tcp->sendTo(client, &response);

        return;
    }

    chan->removeUser(user);

    SlaveUserPackets::ChannelUserParted notification;
    notification.setChannel(chan->id());
    notification.setUser(user->id());

    Vector<SharedPtr<Client>*> clients;

    for (auto &x : chan->users()) {
        if (x.second->user()->isLocal())
            clients.push_back(&x.second->user()->client());
    }

    m_context->tcp->sendTo(clients, &notification);
    m_context->tcp->sendTo(client, &response);

    m_context->log << Logger::Line::Start
                   << "Channel part handled [From: " << user->nick() << ", To: "
                   << chan->name() << ", Local: " << chan->isLocal() << "]"
                   << Logger::Line::End;

    SlaveSlavePackets::ChannelUserPart notification2;
    notification2.setChannel(chan->id());
    notification2.setUser(user->id());
    m_context->slave->broadcast(&notification2);
}

void UserModule::messageChannel(u32 clientid, Packet *packet)
{
    SharedPtr<Client> client = connection(clientid);

    if (!client)
        return;

    SharedPtr<User> user = m_users.findById(clientid);

    if (!user)
        return;

    SlaveUserPackets::SendChannelMessage *request = static_cast<SlaveUserPackets::SendChannelMessage*>(packet);

    SharedPtr<Channel> chan = m_channels.findById(request->id());

    if (!chan || !chan->user(user->id()))
        return;

    SlaveUserPackets::ChannelMessage notification;
    notification.setMessage(request->message());
    notification.setChannel(chan->id());
    notification.setUser(user->id());

    Vector<SharedPtr<Client>*> clients;

    for (auto &x : chan->users()) {
        if (x.first != user->id() && x.second->user()->isLocal())
            clients.push_back(&x.second->user()->client());
    }

    m_context->tcp->sendTo(clients, &notification);

    SlaveSlavePackets::ChannelMessage notification2;
    notification2.setChannel(chan->id());
    notification2.setUser(user->id());
    notification2.setMessage(request->message());
    m_context->slave->broadcast(&notification2);

    m_context->log << Logger::Line::Start
                   << "Channel message handled [From: " << user->nick() << ", To: "
                   << chan->name() << ", Local: " << chan->isLocal() << "]"
                   << Logger::Line::End;
}

void UserModule::privateMessage(u32 clientid, Packet *packet)
{
    SharedPtr<Client> client = connection(clientid);

    if (!client)
        return;

    SharedPtr<User> user = m_users.findById(clientid);

    if (!user)
        return;

    SlaveUserPackets::SendPrivateMessage *request = static_cast<SlaveUserPackets::SendPrivateMessage*>(packet);

    SharedPtr<User> recipient = m_users.findById(request->user());

    if (!recipient) {
        m_context->log->error("Private message recipient not found");
        return;
    }

    if (recipient->isLocal()) {
        SlaveUserPackets::PrivateMessageReceived notification;
        notification.setMessage(request->message());
        notification.setUser(user->id());
        notification.setNick(user->nick());

        m_context->tcp->sendTo(recipient->client(), &notification);
    } else {
        SharedPtr<SlaveServer> slave = m_context->slave->get(recipient->slaveId());

        if (!slave) {
            m_context->log->error("Private message recipient's slave not found");
            return;
        }

        SlaveSlavePackets::PrivateMessage notification;
        notification.setUser(user->id());
        notification.setRecipient(recipient->id());
        notification.setMessage(request->message());

        SharedPtr<Client> slaveClient = slave->client();

        if (slaveClient)
            m_context->tcp->sendTo(slaveClient, &notification);
    }

    m_context->log << Logger::Line::Start
                   << "Private message handled [From: " << user->nick() << ", To: "
                   << recipient->nick() << ", Local: " << recipient->isLocal() << "]"
                   << Logger::Line::End;
}

void UserModule::slaveSyncUsers(u32 clientid, Packet *packet)
{
    SharedPtr<SlaveServer> slave = m_context->slave->getByClientId(clientid);

    if (!slave)
        return;

    SlaveSlavePackets::SyncUsers *request = static_cast<SlaveSlavePackets::SyncUsers*>(packet);

    // TODO: W sumie to może też przyjść jak slave'a rozłączyło i wysyła od nowa sync
    // TODO: Więc trzeba by było zrobić jakąś delte i doinformować użytkowników ...
    // TODO: Ale mi się nie chce już

    for (auto &x : request->users()) {
        m_users.addUser(x.id, x.nick);
    }
}

void UserModule::slaveSyncChannels(u32 clientid, Packet *packet)
{
    SharedPtr<SlaveServer> slave = m_context->slave->getByClientId(clientid);

    if (!slave)
        return;

    SlaveSlavePackets::SyncChannels *request = static_cast<SlaveSlavePackets::SyncChannels*>(packet);

    // TODO: To samo co wyżej

    for (auto &x : request->channels()) {
        SharedPtr<Channel> chan = m_channels.create(x.id, x.name);

        for (auto &y: x.users) {
            SharedPtr<User> usr = m_users.findById(y.id);

            if (usr)
                chan->addUser(usr, y.flags);
        }
    }
}

void UserModule::slaveUserConnect(u32 clientid, Packet *packet)
{
    SharedPtr<SlaveServer> slave = m_context->slave->getByClientId(clientid);

    if (!slave)
        return;

    SlaveSlavePackets::UserConnect *request = static_cast<SlaveSlavePackets::UserConnect*>(packet);

    m_users.addUser(request->id(), request->nick());
}

void UserModule::slaveUserDisconnect(u32 clientid, Packet *packet)
{
    SharedPtr<SlaveServer> slave = m_context->slave->getByClientId(clientid);

    if (!slave)
        return;

    SlaveSlavePackets::UserDisconnect *request = static_cast<SlaveSlavePackets::UserDisconnect*>(packet);

    SharedPtr<User> usr = m_users.findById(request->id());

    if (!usr)
        return;

    // Remove user from list and channels
    m_users.removeUser(usr->id());

    for (auto &x : m_channels.list())
        x.second->removeUser(usr);

    // Tell users about disconnect
    SlaveUserPackets::UserDisconnected dc(usr->id());
    Vector<SharedPtr<Client>*> clients;

    for (auto &x : m_users.list()) {
        if (x.second->isLocal())
            clients.push_back(&x.second->client());
    }

    m_context->tcp->sendTo(clients, &dc);
}

void UserModule::slaveChannelNew(u32 clientid, Packet *packet)
{
    SharedPtr<SlaveServer> slave = m_context->slave->getByClientId(clientid);

    if (!slave)
        return;

    SlaveSlavePackets::ChannelNew *request = static_cast<SlaveSlavePackets::ChannelNew*>(packet);

    SharedPtr<Channel> chan = m_channels.create(request->id(), request->name());
    UNUSED(chan);
}

void UserModule::slaveChannelRemove(u32 clientid, Packet *packet)
{
    SharedPtr<SlaveServer> slave = m_context->slave->getByClientId(clientid);

    if (!slave)
        return;

    SlaveSlavePackets::ChannelRemove *request = static_cast<SlaveSlavePackets::ChannelRemove*>(packet);

    SharedPtr<Channel> chan = m_channels.findById(request->id());

    if (!chan)
        return;

    SlaveUserPackets::ChannelParted parted;
    parted.setId(chan->id());
    parted.setReason(SlaveUserPackets::ChannelParted::Reason::Unknown);
    parted.setStatus(SlaveUserPackets::ChannelParted::Status::Ok);

    Vector<SharedPtr<Client>*> allChannelUsers;
    for (auto &x : chan->users()) {
        if (x.second->user()->isLocal())
            allChannelUsers.push_back(&x.second->user()->client());
    }

    m_context->tcp->sendTo(allChannelUsers, &parted);

    m_channels.remove(chan->id());
}

void UserModule::slaveChannelUser(u32 clientid, Packet *packet)
{
    SharedPtr<SlaveServer> slave = m_context->slave->getByClientId(clientid);

    if (!slave)
        return;

    SlaveSlavePackets::ChannelUser *request = static_cast<SlaveSlavePackets::ChannelUser*>(packet);

    SharedPtr<User> user = m_users.findById(request->user());
    SharedPtr<Channel> chan = m_channels.findById(request->channel());

    if (!user || !chan)
        return;

    if (user->isLocal()) {
        m_context->log->error("UNIMPLEMENTED: Local user joined by slave");
        return;
    }

    SharedPtr<ChannelUser> userChan = chan->user(user->id());

    if (userChan) {
        // TODO: Obsłużyć zmiany flag ..
        m_context->log->error("UNIMPLEMENTED: User changes");
        return;
    }

    SlaveUserPackets::ChannelUserJoined notification;
    notification.user().flags = request->flags();
    notification.user().id = user->id();
    notification.user().nick = user->nick();
    notification.setChannel(chan->id());

    Vector<SharedPtr<Client>*> clients;

    for (auto &x : chan->users()) {
        ChannelUser *chanuser = x.second.get();
        User *chanuser2 = chanuser->user().get();

        if (chanuser2->isLocal())
            clients.push_back(&chanuser2->client());
    }

    chan->addUser(user, request->flags());

    m_context->tcp->sendTo(clients, &notification);
}

void UserModule::slaveChannelUserPart(u32 clientid, Packet *packet)
{
    SharedPtr<SlaveServer> slave = m_context->slave->getByClientId(clientid);

    if (!slave)
        return;

    SlaveSlavePackets::ChannelUserPart *request = static_cast<SlaveSlavePackets::ChannelUserPart*>(packet);

    SharedPtr<User> user = m_users.findById(request->user());
    SharedPtr<Channel> chan = m_channels.findById(request->channel());

    if (user->isLocal()) {
        m_context->log->error("UNIMPLEMENTED: Local user parted by slave");
        return;
    }

    if (!user || !chan)
        return;

    SharedPtr<ChannelUser> userChan = chan->user(user->id());

    if (!userChan)
        return;

    chan->removeUser(user);

    SlaveUserPackets::ChannelUserParted notification;
    notification.setChannel(chan->id());
    notification.setUser(user->id());

    Vector<SharedPtr<Client>*> clients;

    for (auto &x : chan->users()) {
        if (x.second->user()->isLocal())
            clients.push_back(&x.second->user()->client());
    }

    m_context->tcp->sendTo(clients, &notification);
}

void UserModule::slaveChannelMessage(u32 clientid, Packet *packet)
{
    SharedPtr<SlaveServer> slave = m_context->slave->getByClientId(clientid);

    if (!slave)
        return;

    SlaveSlavePackets::ChannelMessage *request = static_cast<SlaveSlavePackets::ChannelMessage*>(packet);

    SharedPtr<User> user = m_users.findById(request->user());
    SharedPtr<Channel> chan = m_channels.findById(request->channel());

    if (!user || !chan)
        return;

    if (!chan->user(user->id()))
        return;

    SlaveUserPackets::ChannelMessage notification;
    notification.setMessage(request->message());
    notification.setChannel(chan->id());
    notification.setUser(user->id());

    Vector<SharedPtr<Client>*> clients;

    for (auto &x : chan->users()) {
        if (x.first != user->id() && x.second->user()->isLocal())
            clients.push_back(&x.second->user()->client());
    }

    m_context->tcp->sendTo(clients, &notification);
}

void UserModule::slavePrivateMessage(u32 clientid, Packet *packet)
{
    SharedPtr<SlaveServer> slave = m_context->slave->getByClientId(clientid);

    if (!slave)
        return;

    SlaveSlavePackets::PrivateMessage *request = static_cast<SlaveSlavePackets::PrivateMessage*>(packet);

    SharedPtr<User> user = m_users.findById(request->user());
    SharedPtr<User> recipient = m_users.findById(request->recipient());

    if (!user || !recipient)
        return;

    if (!recipient->isLocal())
        return;

    SlaveUserPackets::PrivateMessageReceived notification;
    notification.setMessage(request->message());
    notification.setUser(user->id());
    notification.setNick(user->nick());

    m_context->tcp->sendTo(recipient->client(), &notification);
}

END_NAMESPACE
