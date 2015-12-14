#include <modules/slave.h>

#include <modules/user.h>

#include <common/types.h>
#include <server/tcp.h>
#include <server/syseventloop.h>
#include <server/misc_utils.h>

#include <libconfig.h++>

YAIC_NAMESPACE

SlaveServer::SlaveServer(SharedPtr<Client> &client, u32 id, u16 port, uint capacity)
    : m_client(client), m_id(id), m_port(port), m_capacity(capacity)
{

}

SharedPtr<Client> &SlaveServer::client()
{
    return m_client;
}

u32 SlaveServer::id() const
{
    return m_id;
}

u16 SlaveServer::port() const
{
    return m_port;
}

uint SlaveServer::capacity() const
{
    return m_capacity;
}

uint SlaveServer::connections() const
{
    return m_load;
}

uint SlaveServer::load() const
{
    if (m_capacity == 0)
        return 0;

    return (m_load * 100) / m_capacity;
}

bool SlaveServer::active() const
{
    return m_active;
}

void SlaveServer::setConnections(uint connections)
{
    m_load = connections;
}

void SlaveServer::setActive(bool active)
{
    m_active = active;
}

SlaveModule::SlaveModule(Context *context)
    : m_context(context)
{

}

SlaveModule::~SlaveModule()
{

}

void SlaveModule::loadConfig(const libconfig::Setting &section)
{
    UNUSED(section);
}

bool SlaveModule::init()
{
    return true;
}

void SlaveModule::dispatchPacket(EventPacket *ev)
{
    UNUSED(ev);
}

void SlaveModule::dispatchTimer(EventTimer *ev)
{
    UNUSED(ev);
}

void SlaveModule::dispatchSimple(EventSimple *ev)
{
    UNUSED(ev);
}

HashMap<uint, SharedPtr<SlaveServer> > &SlaveModule::slaves()
{
    return m_slaves;
}

std::mutex &SlaveModule::slavesMutex()
{
    return m_slavesMutex;
}



END_NAMESPACE
