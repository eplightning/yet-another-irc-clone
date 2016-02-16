#pragma once

#include <core/global.h>

#include <common/types.h>
#include <server/dispatcher.h>
#include <common/packets/master_slave.h>

#include <libconfig.h++>
#include <thread>
#include <atomic>

YAIC_NAMESPACE

struct Context;

struct MasterModuleConfig {
    String address;
    uint timeout;
    uint heartbeatInterval;
    MasterSlavePackets::Auth::Mode authMode;
    String plainTextPassword;
};

class MasterModule {
public:
    MasterModule(Context *context);
    ~MasterModule();

    // called by app
    void loadConfig(const libconfig::Setting &section);
    bool init();
    void dispatchPacket(EventPacket *ev);
    void dispatchTimer(EventTimer *ev);
    void dispatchSimple(EventSimple *ev);

    // api
    SharedPtr<Client> getMaster();
    bool isAuthed();
    bool isSynced();

    // thread-safe if authed
    u32 getSlaveId() const;
    u64 getAuthPassword() const;

protected:
    bool initPackets();
    bool initTcp();
    bool initTimeout();

    bool tcpNew(SharedPtr<Client> &client);
    void tcpState(uint clientid, TcpClientState state, int error);
    void tcpReceive(uint clientid, PacketHeader header, const Vector<char> &data);

    bool heartbeatHandler(int timer);
    bool timeoutHandler(int timer);

    bool authResponse(uint clientid, Packet *packet);
    bool syncEnd(uint clientid, Packet *packet);

    void masterDisconnected();

    TimerDispatcher m_timerDispatcher;
    int m_heartbeatTimer;
    int m_timeoutTimer;

    Context *m_context;
    MasterModuleConfig m_config;

    SharedPtr<Client> m_master;
    std::mutex m_masterMutex;

    std::chrono::time_point<SteadyClock> m_lastPacket;
    std::mutex m_lastPacketMutex;

    u32 m_ourSlaveId;
    u64 m_authPassword;
    std::atomic<bool> m_authed;
    std::atomic<bool> m_synced;
};

END_NAMESPACE
