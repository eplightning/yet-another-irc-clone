#pragma once

#include <core/global.h>

#include <common/types.h>
#include <server/dispatcher.h>

#include <libconfig.h++>
#include <thread>

YAIC_NAMESPACE

struct Context;

struct MasterModuleConfig {
    String address;
    uint timeout;
    uint heartbeatInterval;
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

protected:
    bool initPackets();
    bool initTcp();
    bool initTimeout();

    bool tcpNew(SharedPtr<Client> &client);
    void tcpState(uint clientid, TcpClientState state, int error);
    void tcpReceive(uint clientid, PacketHeader header, const Vector<char> &data);

    bool heartbeatHandler(int timer);
    bool timeoutHandler(int timer);


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
};

END_NAMESPACE
