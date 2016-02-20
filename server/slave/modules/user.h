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
    void slaveIdReceived(u32 id);
    SharedPtr<Client> getConnection(u32 clientid);
    void cleanupSlave(u32 slave);
    void syncSlave(SharedPtr<Client> &client);

protected:
    bool initPackets();
    bool initTcp();
    bool initTimeout();

    bool tcpNew(SharedPtr<Client> &client);
    void tcpState(uint clientid, TcpClientState state, int error);
    void tcpReceive(uint clientid, PacketHeader header, const Vector<char> &data);

    bool heartbeatHandler(int timer);
    bool timeoutHandler(int timer);

    bool handshake(uint clientid, Packet *packet);
    bool channelList(uint clientid, Packet *packet);
    bool joinChannel(uint clientid, Packet *packet);
    bool partChannel(uint clientid, Packet *packet);
    bool messageChannel(uint clientid, Packet *packet);
    bool privateMessage(uint clientid, Packet *packet);

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
