#include <modules/master.h>

#include <core/global.h>
#include <core/context.h>

#include <common/types.h>
#include <server/tcp.h>
#include <server/syseventloop.h>
#include <server/misc_utils.h>
#include <common/packets/master_slave.h>

#include <libconfig.h++>

YAIC_NAMESPACE

MasterModule::MasterModule(Context *context) :
    m_context(context)
{
    m_config.address = "";
    m_config.timeout = 10;

    m_lastPacket = SteadyClock::now();
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

bool MasterModule::init()
{
    if (!initTcp())
        return false;

    if (!initPackets())
        return false;

    if (!initTimeout())
        return false;

    return true;
}

void MasterModule::dispatchPacket(EventPacket *ev)
{
    {
        MutexLock lock(m_lastPacketMutex);
        m_lastPacket = SteadyClock::now();
    }

    m_context->dispatcher->dispatch(ev->clientid(), ev->packet());
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
    //m_context->dispatcher->append(Packet::Type::RequestServers,
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

bool MasterModule::initTimeout()
{
    m_timeoutTimer = m_context->sysLoop->addTimer(m_config.timeout);
    m_heartbeatTimer = m_context->sysLoop->addTimer(m_config.heartbeatInterval);

    if (m_timeoutTimer == -1 || m_heartbeatTimer == -1) {
        m_context->log->error("Unable to initialize timers");
        return false;
    }

    m_timerDispatcher.append(m_timeoutTimer, BIND_TIMER(this, &MasterModule::timeoutHandler));
    m_timerDispatcher.append(m_heartbeatTimer, BIND_TIMER(this, &MasterModule::heartbeatHandler));

    return true;
}

SharedPtr<Client> MasterModule::getMaster()
{
    MutexLock lock(m_masterMutex);

    return m_master;
}

void MasterModule::tcpState(uint clientid, TcpClientState state, int error)
{
    MutexLock lock(m_masterMutex);

    switch (state) {
    case TCSConnected:
        m_master = m_context->tcp->client(clientid);
        break;

    case TCSDisconnected:
        m_context->log << Logger::Line::Start
                       << "Master server disconnected! Reason: " << MiscUtils::systemError(error)
                       << Logger::Line::End;

        m_master.reset();
        m_context->eventQueue->append(new EventSimple(EventSimple::EventId::MasterDisconnected));

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
        MutexLock lock(m_masterMutex);

        if (m_master)
            m_context->tcp->disconnect(m_master, true);

        return;
    }

    m_context->eventQueue->append(new EventPacket(packet, clientid, SLAVE_APP_SOURCE_MASTER));
}

bool MasterModule::heartbeatHandler(int timer)
{
    UNUSED(timer);

    SharedPtr<Client> master = getMaster();

    if (!master)
        return false;

    // TODO: get slave load
    MasterSlavePackets::SlaveHeartbeat packet(0);

    m_context->tcp->sendTo(master, &packet);

    return true;
}

bool MasterModule::timeoutHandler(int timer)
{
    UNUSED(timer);

    std::chrono::time_point<SteadyClock> time;

    {
        MutexLock lock(m_lastPacketMutex);
        time = m_lastPacket;
    }

    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(SteadyClock::now() - time).count();

    if (seconds >= m_config.timeout) {
        m_context->log << Logger::Line::Start
                       << "Master server timeout: " << seconds << "s"
                       << Logger::Line::End;

        MutexLock lock(m_masterMutex);
        m_context->tcp->disconnect(m_master, true);
    }

    return true;
}

END_NAMESPACE

