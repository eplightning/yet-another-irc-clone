#pragma once

#include <core/global.h>

#include <common/types.h>
#include <server/dispatcher.h>
#include <common/packets/master_slave.h>

#include <libconfig.h++>

YAIC_NAMESPACE

struct Context;

enum SlaveState {
    SSUnauthed,
    SSAuthed,
    SSSyncing,
    SSActive,
    SSClosing
};

class SlaveServer {
public:
    SlaveServer(SharedPtr<Client> &client, u32 id);

    SharedPtr<Client> &client();
    u32 id() const;

    // thread-safe (because const) if state >= SSAuthed
    const String &name() const;
    uint capacity() const;
    u16 userPort() const;
    u16 slavePort() const;
    const String &userAddress() const;
    const String &slaveAddress() const;

    // not thread-safe, use only while holding m_slaves lock
    uint connections() const;
    uint load() const;
    SlaveState state() const;
    long long lastPacketSeconds(const std::chrono::time_point<SteadyClock> &now);

    void setConnections(uint connections);
    void setState(SlaveState state);
    void updateLastPacket();
    void setUserPort(u16 userPort);
    void setSlavePort(u16 slavePort);
    void setUserAddress(const String &userAddress);
    void setSlaveAddress(const String &slaveAddress);
    void setCapacity(uint capacity);
    void setName(const String &name);

protected:
    SharedPtr<Client> m_client;
    u32 m_id;
    u16 m_userPort;
    u16 m_slavePort;
    String m_userAddress;
    String m_slaveAddress;
    uint m_load;
    uint m_capacity;
    SlaveState m_state;
    String m_name;
    std::chrono::time_point<SteadyClock> m_lastPacket;
};

struct SlaveModuleConfig {
    Vector<String> listen;
    MasterSlavePackets::Auth::Mode authMode;
    String plainTextPassword;
    uint timeout;
    uint heartbeatInterval;
};

typedef HashMap<uint, SharedPtr<SlaveServer>> SlaveServers;

class SlaveModule {
public:
    SlaveModule(Context *context);
    ~SlaveModule();

    void loadConfig(const libconfig::Setting &section);
    bool init();
    void dispatchPacket(EventPacket *ev);
    void dispatchTimer(EventTimer *ev);
    void dispatchSimple(EventSimple *ev);

    SharedPtr<SlaveServer> getSlave(uint clientid);
    Vector<SharedPtr<SlaveServer>> getSlaves(bool ipv4 = true, bool ipv6 = true);

protected:
    bool initPackets();
    bool initTcp();
    bool initTimers();

    bool tcpNew(SharedPtr<Client> &client);
    void tcpState(uint clientid, TcpClientState state, int error);
    void tcpReceive(uint clientid, PacketHeader header, const Vector<char> &data);

    bool heartbeatHandler(int timer);
    bool timeoutHandler(int timer);

    bool updateLoad(uint clientid, Packet *packet);
    bool auth(uint clientid, Packet *packet);
    bool syncStart(uint clientid, Packet *packet);
    bool newAck(uint clientid, Packet *packet);

    Context *m_context;
    SlaveModuleConfig m_config;
    TimerDispatcher m_timerDispatcher;

    int m_timeoutTimer;
    int m_heartbeatTimer;

    Mutex m_slavesMutex;
    SlaveServers m_slaves;

    Map<u32, int> m_syncProgress;

    u64 m_authPassword;
};

END_NAMESPACE
