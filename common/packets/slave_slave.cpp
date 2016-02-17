#include <common/packets/slave_slave.h>

#include <common/packet.h>
#include <common/types.h>

YAIC_NAMESPACE SSPACKETS_NAMESPACE

bool Hello::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_id);
    result &= read(payload, m_name);
    result &= read(payload, m_authPassword);

    return result;
}

void Hello::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
    write(payload, m_name);
    write(payload, m_authPassword);
}

u32 Hello::id() const
{
    return m_id;
}

void Hello::setId(u32 id)
{
    m_id = id;
}

const String &Hello::name() const
{
    return m_name;
}

void Hello::setName(const String &name)
{
    m_name = name;
}

u64 Hello::authPassword() const
{
    return m_authPassword;
}

void Hello::setAuthPassword(u64 authPassword)
{
    m_authPassword = authPassword;
}

Hello::Hello() :
    Packet(Packet::Type::SlaveHello)
{

}

HelloResponse::HelloResponse() :
    Packet(Packet::Type::SlaveHelloResponse)
{

}

HelloResponse::HelloResponse(u32 id) :
    Packet(Packet::Type::SlaveHelloResponse), m_id(id)
{

}

bool HelloResponse::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_id);
    result &= read(payload, m_authPassword);

    return result;
}

void HelloResponse::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
    write(payload, m_authPassword);
}

u32 HelloResponse::id() const
{
    return m_id;
}

u64 HelloResponse::authPassword() const
{
    return m_authPassword;
}

void HelloResponse::setAuthPassword(u64 authPassword)
{
    m_authPassword = authPassword;
}

Heartbeat::Heartbeat() :
    Packet(Packet::Type::SlaveSlaveHeartbeat)
{

}

bool Heartbeat::decodePayload(const Vector<char> &payload)
{
    UNUSED(payload);

    return true;
}

void Heartbeat::encodePayload(Vector<char> &payload) const
{
    UNUSED(payload);
}

END_NAMESPACE END_NAMESPACE
