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
    m_context(context), m_authed(false), m_synced(false)
{
    m_config.address = "127.0.0.1:31412";
    m_config.timeout = 10;
    m_config.heartbeatInterval = 1;
    m_config.authMode = MasterSlavePackets::Auth::Mode::None;
    m_config.plainTextPassword = "";

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
    m_context->dispatcher->append(Packet::Type::SlaveAuthResponse,
                              BIND_DISPATCH(this, &MasterModule::authResponse));

    m_context->dispatcher->append(Packet::Type::SlaveSyncEnd,
                              BIND_DISPATCH(this, &MasterModule::syncEnd));

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

bool MasterModule::isAuthed()
{
    return m_authed.load();
}

bool MasterModule::isSynced()
{
    return m_synced.load();
}

u32 MasterModule::getSlaveId() const
{
    return m_ourSlaveId;
}

u64 MasterModule::getAuthPassword() const
{
    return m_authPassword;
}

void MasterModule::tcpState(uint clientid, TcpClientState state, int error)
{
    MutexLock lock(m_masterMutex);

    if (state == TCSConnected) {
        m_master = m_context->tcp->client(clientid);

        MasterSlavePackets::Auth authPacket;

        authPacket.setMode(m_config.authMode);
        authPacket.setPlaintextPassword(m_config.plainTextPassword);

        authPacket.setCapacity(static_cast<u32>(m_context->user->capacity()));
        authPacket.setName(m_context->slaveName);
        authPacket.setSlaveAddress(m_context->slave->publicAddress());
        authPacket.setSlavePort(m_context->slave->publicPort());
        authPacket.setUserAddress(m_context->user->publicAddress());
        authPacket.setUserPort(m_context->user->publicPort());

        m_context->tcp->sendTo(m_master, &authPacket);
    } else if (state == TCSDisconnected) {
        m_context->log << Logger::Line::Start
                       << "Master server disconnected! Reason: " << MiscUtils::systemError(error)
                       << Logger::Line::End;

        m_master.reset();
        m_synced.store(false);
        m_authed.store(false);
        m_context->eventQueue->append(new EventSimple(EventSimple::EventId::MasterDisconnected));
    }
}

bool MasterModule::tcpNew(SharedPtr<Client> &client)
{
    UNUSED(client);

    return false;
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

bool MasterModule::authResponse(uint clientid, Packet *packet)
{
    UNUSED(clientid);

    MasterSlavePackets::AuthResponse *response = static_cast<MasterSlavePackets::AuthResponse*>(packet);

    if (response->status() != MasterSlavePackets::AuthResponse::Status::Ok) {
        m_context->log << Logger::Line::Start
                       << "Master server refused authentication request: " << static_cast<u32>(response->status())
                       << Logger::Line::End;

        return true;
    }

    m_ourSlaveId = response->id();
    m_authPassword = response->authPassword();
    m_authed.store(true);

    MasterSlavePackets::SyncStart sync;

    {
        MutexLock lock(m_masterMutex);

        if (m_master)
            m_context->tcp->sendTo(m_master, &sync);
    }

    m_context->log->message("Master server accepted authentication request");

    return true;
}

bool MasterModule::syncEnd(uint clientid, Packet *packet)
{
    UNUSED(clientid);
    UNUSED(packet);

    m_synced.store(true);

    m_context->log->message("Synchronization finished");

    return true;
}

END_NAMESPACE

