#pragma once

#include <core/global.h>

#include <common/types.h>
#include <server/dispatcher.h>

#include <libconfig.h++>
#include <thread>
#include <mutex>

YAIC_NAMESPACE

struct Context;

class User {
public:
    User(SharedPtr<Client> &client);

    SharedPtr<Client> &client();
    const SteadyClock::time_point &connectedAt() const;

protected:
    SharedPtr<Client> m_client;
    SteadyClock::time_point m_connectedAt;
};

struct UserModuleConfig {
    Vector<String> listen;
    uint timeout;
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

protected:
    bool initPackets();
    bool initTcp();
    bool initTimeout();

    bool tcpNew(SharedPtr<Client> &client);
    void tcpState(u32 clientid, TcpClientState state, int error);
    void tcpReceive(u32 clientid, PacketHeader header, const Vector<char> &data);

    bool timeoutHandler(int timer);

    bool serversRequest(u32 clientid, Packet *packet);

    SharedPtr<User> getUser(u32 clientid);

protected:
    Context *m_context;
    UserModuleConfig m_config;
    TimerDispatcher m_timerDispatcher;

    int m_timerTimeout;

    HashMap<u32, SharedPtr<User>> m_users;
    std::mutex m_usersMutex;
};

END_NAMESPACE
