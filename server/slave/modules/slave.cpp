#include <modules/slave.h>

#include <core/global.h>
#include <core/context.h>

#include <common/types.h>
#include <server/tcp.h>
#include <server/syseventloop.h>
#include <server/misc_utils.h>
#include <common/packets/master_slave.h>

#include <libconfig.h++>

YAIC_NAMESPACE

SlaveModule::SlaveModule(Context *context) :
    m_context(context)
{
    m_config.publicAddress = "127.0.0.1";
    m_config.publicPort = 31413;
}

SlaveModule::~SlaveModule()
{

}

void SlaveModule::loadConfig(const libconfig::Setting &section)
{
    if (section.exists("listen")) {
        const libconfig::Setting &listenSection = section.lookup("listen");

        if (listenSection.isArray()) {
            for (auto &listener : listenSection)
                m_config.listen.push_back(listener);
        } else if (listenSection.isScalar()) {
            m_config.listen.push_back(listenSection);
        }
    }

    if (section.exists("public-address")) {
        const libconfig::Setting &sub = section.lookup("public-address");

        if (sub.isScalar()) {
            String address;
            u16 port;

            if (SocketUtils::readAddress(sub.c_str(), port, address) != CPUnknown) {
                m_config.publicAddress = address;
                m_config.publicPort = port;
            }
        }
    }

    if (section.exists("timeout")) {
        const libconfig::Setting &sub = section.lookup("timeout");

        if (sub.isScalar())
            m_config.timeout = sub;
    }

    if (section.exists("heartbeat")) {
        const libconfig::Setting &sub = section.lookup("heartbeat");

        if (sub.isScalar())
            m_config.heartbeatInterval = sub;
    }
}

bool SlaveModule::init()
{
    /*if (!initTcp())
        return false;*/

    if (!initPackets())
        return false;

    /*if (!initTimeout())
        return false;*/

    return true;
}

void SlaveModule::dispatchPacket(EventPacket *ev)
{
    m_context->dispatcher->dispatch(ev->clientid(), ev->packet());
}

void SlaveModule::dispatchTimer(EventTimer *ev)
{
    m_timerDispatcher.dispatch(ev->timer());
}

void SlaveModule::dispatchSimple(EventSimple *ev)
{
    UNUSED(ev);
}

const String &SlaveModule::publicAddress() const
{
    return m_config.publicAddress;
}

u16 SlaveModule::publicPort() const
{
    return m_config.publicPort;
}

bool SlaveModule::initPackets()
{
    // from master
    m_context->dispatcher->append(Packet::Type::NewSlave,
                              BIND_DISPATCH(this, &SlaveModule::newSlave));

    m_context->dispatcher->append(Packet::Type::RemoveSlave,
                              BIND_DISPATCH(this, &SlaveModule::removeSlave));

    return true;
}

bool SlaveModule::newSlave(uint clientid, Packet *packet)
{
    UNUSED(clientid);

    SharedPtr<Client> master = m_context->master->getMaster();
    if (!master)
        return false;

    MasterSlavePackets::NewSlave *request = static_cast<MasterSlavePackets::NewSlave*>(packet);

    // TODO: add slave

    MasterSlavePackets::NewAck ack(request->id());
    m_context->tcp->sendTo(master, &ack);

    m_context->log << Logger::Line::Start
                   << "Slave acknowledged: " << request->id()
                   << Logger::Line::End;

    return true;
}

bool SlaveModule::removeSlave(uint clientid, Packet *packet)
{
    UNUSED(clientid);

    MasterSlavePackets::RemoveSlave *request = static_cast<MasterSlavePackets::RemoveSlave*>(packet);

    // TODO: remove slave

    m_context->log << Logger::Line::Start
                   << "Slave removed, as requested by the master server: " << request->id()
                   << Logger::Line::End;

    return true;
}

END_NAMESPACE
