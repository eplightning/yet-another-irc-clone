#pragma once

#include <common/types.h>
#include <server/event.h>
#include <server/tcp_server.h>
#include <server/dispatcher.h>
#include <server/log.h>

YAIC_NAMESPACE

class MasterClient {
public:
    MasterClient(SharedPtr<Client> client);

    SharedPtr<Client> client() const;

protected:
    SharedPtr<Client> m_client;
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

struct Context {
    EventQueue eventQueue;
    TcpServer tcp;

    HashMap<uint, MasterClient> clients;
    PacketDispatcher clientDispatcher;

    HashMap<uint, SlaveServer> slaves;
    PacketDispatcher slaveDispatcher;

    String configPath;

    Log *log;
};

END_NAMESPACE
