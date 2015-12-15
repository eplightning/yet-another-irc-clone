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
    int tries;
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

protected:
    bool initPackets();
    bool initTcp();

    bool tcpNew(SharedPtr<Client> &client);
    void tcpState(uint clientid, TcpClientState state, int error);
    void tcpReceive(uint clientid, PacketHeader header, const Vector<char> &data);

    PacketDispatcher m_packetDispatcher;
    TimerDispatcher m_timerDispatcher;

    Context *m_context;
    MasterModuleConfig m_config;

    SharedPtr<Client> m_master;
    std::mutex m_masterMutex;
};

END_NAMESPACE
