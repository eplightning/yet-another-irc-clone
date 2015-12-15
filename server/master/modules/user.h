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
    uint timeoutGranularity;
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

    // getters for other modules
    HashMap<uint, SharedPtr<User>> &users();
    std::mutex &usersMutex();

    // api for other modules
    SharedPtr<User> getUser(uint clientid);

protected:
    bool initPackets();
    bool initTcp();
    bool initTimeout();

    bool tcpNew(SharedPtr<Client> &client);
    void tcpState(uint clientid, TcpClientState state, int error);
    void tcpReceive(uint clientid, PacketHeader header, const Vector<char> &data);

    bool timeoutHandler(int timer);

    bool serversRequest(uint clientid, Packet *packet);

protected:
    Context *m_context;
    UserModuleConfig m_config;
    PacketDispatcher m_packetDispatcher;
    TimerDispatcher m_timerDispatcher;

    int m_timerTimeout;

    HashMap<uint, SharedPtr<User>> m_users;
    std::mutex m_usersMutex;
};

END_NAMESPACE
