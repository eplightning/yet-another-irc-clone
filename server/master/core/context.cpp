#include "core/context.h"

#include <common/types.h>
#include <server/event.h>
#include <server/tcp_server.h>
#include <server/dispatcher.h>

YAIC_NAMESPACE

MasterClient::MasterClient(SharedPtr<Client> client) :
    m_client(client)
{

}

SharedPtr<Client> MasterClient::client() const
{
    return m_client;
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

END_NAMESPACE
