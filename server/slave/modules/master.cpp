#include <modules/master.h>

#include <core/global.h>
#include <core/context.h>

#include <common/types.h>
#include <server/tcp.h>
#include <server/syseventloop.h>
#include <server/misc_utils.h>
#include <common/packets/master_user.h>

#include <libconfig.h++>

YAIC_NAMESPACE

MasterModule::MasterModule(Context *context) :
    m_context(context)
{
    m_config.address = "";
    m_config.tries = 5;
}

MasterModule::~MasterModule()
{

}

void MasterModule::loadConfig(const libconfig::Setting &section)
{
    if (section.exists("server")) {
        const libconfig::Setting &sub = section.lookup("server");

        if (sub.isScalar())
            m_config.address = sub.c_str();
    }

    if (section.exists("max-tries")) {
        const libconfig::Setting &sub = section.lookup("max-tries");

        if (sub.isScalar())
            m_config.tries = sub;
    }
}

bool MasterModule::init()
{
    if (!initTcp())
        return false;

    if (!initPackets())
        return false;

    return true;
}

void MasterModule::dispatchPacket(EventPacket *ev)
{
    m_packetDispatcher.dispatch(ev->clientid(), ev->packet());
}

void MasterModule::dispatchTimer(EventTimer *ev)
{
    m_timerDispatcher.dispatch(ev->timer());
}

void MasterModule::dispatchSimple(EventSimple *ev)
{
    UNUSED(ev);
}

bool MasterModule::initPackets()
{
    //m_packetDispatcher.append(Packet::Type::RequestServers,
    //                          BIND_DISPATCH(this, &MasterModule::serversRequest));

    return true;
}

bool MasterModule::initTcp()
{
    TcpPool *pool = new TcpPool(
        BIND_TCP_STATE(this, &MasterModule::tcpState),
        BIND_TCP_NEW(this, &MasterModule::tcpNew),
        BIND_TCP_RECV(this, &MasterModule::tcpReceive)
    );

    m_context->tcp->createPool("master-client", pool);

    bool result = m_context->tcp->connect(m_config.address, "master-client");

    if (!result)
        m_context->log->error("Unable to initialize connection to master server");

    return result;
}

void MasterModule::tcpState(uint clientid, TcpClientState state, int error)
{
    std::lock_guard<std::mutex> lock(m_masterMutex);

    switch (state) {
    case TCSConnected:
        m_master = m_context->tcp->client(clientid);
        break;

    case TCSDisconnected:
        m_context->log << Logger::Line::Start
                       << "Master server disconnected! Reason: " << MiscUtils::systemError(error)
                       << Logger::Line::End;
        // todo: reconnect
        break;

    case TCSDisconnecting:
    case TCSWritingClosed:
        break;
    }
}

bool MasterModule::tcpNew(SharedPtr<Client> &client)
{
    UNUSED(client);

    return true;
}

void MasterModule::tcpReceive(uint clientid, PacketHeader header, const Vector<char> &data)
{
    Packet *packet;

    // jeśli błąd to instant disconnect
    if (!Packet::checkDirection(header.type, Packet::Direction::MasterToSlave) ||
            (packet = Packet::factory(header, data)) == nullptr) {
        std::lock_guard<std::mutex> lock(m_masterMutex);

        if (m_master)
            m_context->tcp->disconnect(m_master, true);

        return;
    }

    m_context->eventQueue->append(new EventPacket(packet, clientid, SLAVE_APP_SOURCE_MASTER));
}

END_NAMESPACE

