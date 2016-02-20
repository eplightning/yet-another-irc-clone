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

class SlaveServer {
public:
    SlaveServer(u32 id, const String &name, bool elder, const String &address, u16 port);

    u32 id() const;
    const String &name() const;
    bool isElder() const;
    const String &address() const;
    u16 port() const;

    SharedPtr<Client> client();
    u32 clientid() const;
    bool isConnected();
    void replaceClient(SharedPtr<Client> &client);
    void replaceClient();

protected:
    u32 m_id;
    String m_name;
    SharedPtr<Client> m_client;
    std::atomic<u32> m_clientid;
    bool m_elder;
    String m_address;
    u16 m_port;

    std::mutex m_mutex;
};

struct SlaveModuleConfig {
    Vector<String> listen;
    String publicAddress;
    u16 publicPort;
    uint timeout;
    uint heartbeatInterval;
    uint reconnectInterval;
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
    SharedPtr<SlaveServer> getByClientId(u32 clientid);
    SharedPtr<SlaveServer> get(u32 id);

protected:
    SharedPtr<Client> connection(u32 clientid);

    bool initPackets();
    bool initTcp();
    bool initTimeout();

    bool tcpNew(SharedPtr<Client> &client);
    void tcpState(uint clientid, TcpClientState state, int error);
    void tcpReceive(uint clientid, PacketHeader header, const Vector<char> &data);

    void establishConnection(SharedPtr<SlaveServer> &slave);
    void synchronize(SharedPtr<Client> &client);

    bool heartbeatHandler(int timer);
    bool timeoutHandler(int timer);
    bool reconnectHandler(int timer);

    bool newSlave(uint clientid, Packet *packet);
    bool removeSlave(uint clientid, Packet *packet);
    bool hello(uint clientid, Packet *packet);
    bool helloResponse(uint clientid, Packet *packet);

    TimerDispatcher m_timerDispatcher;
    int m_heartbeatTimer;
    int m_timeoutTimer;
    int m_reconnectTimer;

    Context *m_context;
    SlaveModuleConfig m_config;

    HashMap<u32, SharedPtr<Client>> m_connections;
    std::mutex m_connectionsMutex;

    HashMap<u32, SharedPtr<SlaveServer>> m_slaves;
    std::mutex m_slavesMutex;

    std::queue<u32> m_reconnectQueue;
    std::mutex m_reconnectMutex;

    std::map<u32, std::chrono::time_point<SteadyClock>> m_lastPackets;
    std::mutex m_lastPacketMutex;
};

END_NAMESPACE
