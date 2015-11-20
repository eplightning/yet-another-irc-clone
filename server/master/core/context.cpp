#include "core/context.h"

#include <common/types.h>
#include <server/event.h>
#include <server/tcp_server.h>
#include <server/dispatcher.h>

#include <chrono>

YAIC_NAMESPACE

User::User(SharedPtr<Client> client) :
    m_client(client), m_connectedAt(std::chrono::steady_clock::now())
{

}

SharedPtr<Client> User::client() const
{
    return m_client;
}

const std::chrono::steady_clock::time_point &User::connectedAt() const
{
    return m_connectedAt;
}


SlaveServer::SlaveServer(SharedPtr<Client> client) :
    m_client(client), m_id(0), m_port(0), m_load(0), m_capacity(0)
{

}

SharedPtr<Client> SlaveServer::client() const
{
    return m_client;
}

void SlaveServer::setId(u32 id)
{
    m_id = id;
}

u16 SlaveServer::port() const
{
    return m_port;
}

void SlaveServer::setPort(u16 port)
{
    m_port = port;
}

uint SlaveServer::connections() const
{
    return m_load;
}

void SlaveServer::setConnections(uint load)
{
    m_load = load;
}

uint SlaveServer::capacity() const
{
    return m_capacity;
}

void SlaveServer::setCapacity(uint capacity)
{
    m_capacity = capacity;
}

uint SlaveServer::load() const
{
    if (m_capacity == 0)
        return 0;

    return (m_load * 100) / m_capacity;
}

u32 SlaveServer::id() const
{
    return m_id;
}

Context::Context()
    : sysLoop(nullptr), log(nullptr)
{

}

Context::~Context()
{
    if (log != nullptr)
        delete log;

    if (sysLoop != nullptr)
        delete sysLoop;
}

SharedPtr<User> Context::user(uint clientid)
{
    std::lock_guard<std::mutex> lock(usersMutex);

    auto it = users.find(clientid);

    if (it == users.end())
        return nullptr;

    return it->second;
}


END_NAMESPACE
