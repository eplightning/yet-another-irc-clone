#include <modules/user.h>

#include <common/types.h>
#include <server/socket_utils.h>

#include <libconfig.h++>

YAIC_NAMESPACE

UserModule::UserModule(Context *context) :
    m_context(context)
{

}

UserModule::~UserModule()
{

}

void UserModule::loadFromLibconfig(const libconfig::Setting &section)
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
}

void UserModule::init()
{
    initTcp();

    m_context->userDispatcher.append(Packet::Type::RequestServers,
                                       BIND_DISPATCH(this, &UserModule::serversRequest));
}

void UserModule::initTcp()
{
    ListenPool *pool = new ListenPool;

    pool->droppedConnection = std::bind(&UserModule::tcpDropped, this, std::placeholders::_1);
    pool->lostConnection = std::bind(&UserModule::tcpLost, this, std::placeholders::_1);
    pool->newConnection = std::bind(&UserModule::tcpNew, this, std::placeholders::_1);
    pool->receive = std::bind(&UserModule::tcpReceive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    pool->sockets.reserve(m_config.listen.size());

    int i = 0;

    for (auto &x : m_config.listen) {
        ConnectionProto proto;
        int sock = SocketUtils::createListenSocket(x, proto);

        if (sock >= 0) {
            pool->sockets.resize(i + 1);
            pool->sockets[i].protocol = proto;
            pool->sockets[i].socket = sock;
            pool->sockets[i].pool = pool;

            i = i + 1;
        }
    }

    if (i == 0) {
        delete pool;
        throw Exception("Couldn't create any client listen sockets");
    }

    m_context->tcp.createPool("client", pool);
}

void UserModule::tcpDropped(uint clientid)
{
    {
        std::lock_guard<std::mutex> lock(m_context->usersMutex);

        auto it = m_context->users.find(clientid);
        if (it != m_context->users.end())
            m_context->users.erase(it);
    }

    *m_context->log << Log::Line::Start
                    << "Client connection dropped: [ID: " << clientid << "]"
                    << Log::Line::End;
}

void UserModule::tcpLost(uint clientid)
{
    *m_context->log << Log::Line::Start
                    << "Client connection lost: [ID: " << clientid << "]"
                    << Log::Line::End;
}

bool UserModule::tcpNew(SharedPtr<Client> client)
{
    {
        std::lock_guard<std::mutex> lock(m_context->usersMutex);

        auto it = m_context->users.find(client->id());
        if (it == m_context->users.end())
            m_context->users[client->id()] = std::make_shared<User>(client);
    }

    *m_context->log << Log::Line::Start
                    << "Client connection: [ID: " << client->id() << ", IP: " << client->address() <<"]"
                    << Log::Line::End;

    return true;
}

void UserModule::tcpReceive(uint clientid, PacketHeader header, const Vector<char> &data)
{
    Packet *packet;

    // jeśli błąd to instant disconnect
    if (!Packet::checkDirection(header.type, Packet::Direction::UserToMaster) ||
            (packet = Packet::factory(header, data)) == nullptr) {
        SharedPtr<User> user = m_context->user(clientid);

        if (user)
            m_context->tcp.disconnect(user->client(), true);

        return;
    }

    m_context->eventQueue.append(new EventPacket(packet, clientid, 1));
}

PacketDispatcher::Result UserModule::serversRequest(uint clientid, Packet *packet)
{
    *m_context->log << Log::Line::Start
                    << "Server Request"
                    << Log::Line::End;

    return PacketDispatcher::Result::Continue;
}



END_NAMESPACE
