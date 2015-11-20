#pragma once

#include <common/types.h>
#include <server/event.h>
#include <server/tcp_server.h>
#include <server/dispatcher.h>
#include <server/log.h>
#include <server/syseventloop.h>

#include <chrono>
#include <mutex>

YAIC_NAMESPACE

class User {
public:
    User(SharedPtr<Client> client);

    SharedPtr<Client> client() const;

    const std::chrono::steady_clock::time_point &connectedAt() const;

protected:
    SharedPtr<Client> m_client;
    std::chrono::steady_clock::time_point m_connectedAt;
};

class SlaveServer {
public:
    SlaveServer(SharedPtr<Client> client);

    SharedPtr<Client> client() const;

    u32 id() const;
    void setId(u32 id);

    u16 port() const;
    void setPort(u16 port);

    uint connections() const;
    void setConnections(uint connections);

    uint capacity() const;
    void setCapacity(uint capacity);

    uint load() const;

protected:
    SharedPtr<Client> m_client;
    u32 m_id;
    u16 m_port;
    uint m_load;
    uint m_capacity;
};

class Context {
public:
    Context();
    ~Context();

    SharedPtr<User> user(uint clientid);

public:
    EventQueue eventQueue;
    TcpServer tcp;
    SysEventLoop *sysLoop;
    String configPath;
    Log *log;

    std::mutex usersMutex;
    HashMap<uint, SharedPtr<User>> users;
    PacketDispatcher userDispatcher;

    std::mutex slavesMutex;
    HashMap<uint, SharedPtr<SlaveServer>> slaves;
    PacketDispatcher slaveDispatcher;

    TimerDispatcher timerDispatcher;
};

END_NAMESPACE
