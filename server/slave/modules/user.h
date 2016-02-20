#pragma once

#include <core/global.h>

#include <common/types.h>
#include <server/dispatcher.h>
#include <common/packets/slave_user.h>
#include <components/users.h>
#include <components/channels.h>

#include <libconfig.h++>
#include <thread>
#include <atomic>

YAIC_NAMESPACE

struct Context;

struct UserModuleConfig {
    Vector<String> listen;
    String publicAddress;
    u16 publicPort;
    uint timeout;
    uint heartbeatInterval;
    uint capacity;
};

class UserModule {
public:
    UserModule(Context *context);
    ~UserModule();

    // called by app
    void loadConfig(const libconfig::Setting &section);
    bool init();
    void dispatchPacket(EventPacket *ev);
    void dispatchTimer(EventTimer *ev);
    void dispatchSimple(EventSimple *ev);

    // api
    const String &publicAddress() const;
    u16 publicPort() const;
    uint capacity() const;
    uint load() const;
    void slaveIdReceived(u32 id);
    void cleanupSlave(u32 slave);
    void syncSlave(SharedPtr<Client> &client);

protected:
    SharedPtr<Client> connection(u32 clientid);

    bool initPackets();
    bool initTcp();
    bool initTimeout();

    bool tcpNew(SharedPtr<Client> &client);
    void tcpState(u32 clientid, TcpClientState state, int error);
    void tcpReceive(u32 clientid, PacketHeader header, const Vector<char> &data);

    void heartbeatHandler(int timer);
    void timeoutHandler(int timer);

    void handshake(u32 clientid, Packet *packet);
    void channelList(u32 clientid, Packet *packet);
    void joinChannel(u32 clientid, Packet *packet);
    void partChannel(u32 clientid, Packet *packet);
    void messageChannel(u32 clientid, Packet *packet);
    void privateMessage(u32 clientid, Packet *packet);

    TimerDispatcher m_timerDispatcher;
    int m_heartbeatTimer;
    int m_timeoutTimer;

    Context *m_context;
    UserModuleConfig m_config;

    HashMap<u32, SharedPtr<Client>> m_connections;
    std::mutex m_connectionsMutex;

    std::map<u32, std::chrono::time_point<SteadyClock>> m_lastPackets;
    std::mutex m_lastPacketMutex;

    Users m_users;
    Channels m_channels;
};

END_NAMESPACE
