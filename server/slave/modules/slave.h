#pragma once

#include <core/global.h>

#include <common/types.h>
#include <server/dispatcher.h>
#include <common/packets/master_slave.h>

#include <libconfig.h++>
#include <thread>

YAIC_NAMESPACE

struct Context;

struct SlaveModuleConfig {
    Vector<String> listen;
    String publicAddress;
    u16 publicPort;
    uint timeout;
    uint heartbeatInterval;
};

class SlaveModule {
public:
    SlaveModule(Context *context);
    ~SlaveModule();

    // called by app
    void loadConfig(const libconfig::Setting &section);
    bool init();
    void dispatchPacket(EventPacket *ev);
    void dispatchTimer(EventTimer *ev);
    void dispatchSimple(EventSimple *ev);

    // api
    const String &publicAddress() const;
    u16 publicPort() const;

protected:
    bool initPackets();
    /*
    bool initTcp();
    bool initTimeout();

    bool tcpNew(SharedPtr<Client> &client);
    void tcpState(uint clientid, TcpClientState state, int error);
    void tcpReceive(uint clientid, PacketHeader header, const Vector<char> &data);

    bool heartbeatHandler(int timer);
    bool timeoutHandler(int timer);

    bool authResponse(uint clientid, Packet *packet);
    bool syncEnd(uint clientid, Packet *packet);

    void masterDisconnected();*/

    bool newSlave(uint clientid, Packet *packet);
    bool removeSlave(uint clientid, Packet *packet);

    TimerDispatcher m_timerDispatcher;
    /*int m_heartbeatTimer;
    int m_timeoutTimer;*/

    Context *m_context;
    SlaveModuleConfig m_config;

    /*SharedPtr<Client> m_master;
    std::mutex m_masterMutex;

    std::chrono::time_point<SteadyClock> m_lastPacket;
    std::mutex m_lastPacketMutex;

    u32 m_ourSlaveId;
    u64 m_authPassword;
    std::atomic<bool> m_authed;
    std::atomic<bool> m_synced;*/
};

END_NAMESPACE
