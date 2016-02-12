#pragma once

#include <core/global.h>

#include <common/types.h>
#include <server/dispatcher.h>

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

    // NOTE: not thread-safe, use only while holding m_slaves lock
    u16 port() const;
    uint capacity() const;
    uint connections() const;
    uint load() const;
    SlaveState state() const;
    const String &name() const;

    long long lastPacketSeconds(const std::chrono::time_point<SteadyClock> &now);

    void authenticate(uint capacity, u16 port, const String &name);
    void setConnections(uint connections);
    void setState(SlaveState state);
    void updateLastPacket();

protected:
    SharedPtr<Client> m_client;
    u32 m_id;
    u16 m_port;
    uint m_load;
    uint m_capacity;
    SlaveState m_state;
    String m_name;
    std::chrono::time_point<SteadyClock> m_lastPacket;
};

enum SlaveAuthMode {
    SAMNone = 0,
    SAMPlainText = 1
};

struct SlaveModuleConfig {
    Vector<String> listen;
    SlaveAuthMode authMode;
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
    void dispatchGeneric(Event *ev);

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

    Context *m_context;
    SlaveModuleConfig m_config;
    TimerDispatcher m_timerDispatcher;

    int m_timeoutTimer;
    int m_heartbeatTimer;

    Mutex m_slavesMutex;
    SlaveServers m_slaves;
};

END_NAMESPACE
