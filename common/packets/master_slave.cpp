#include <common/packets/master_slave.h>

#include <common/packet.h>
#include <common/types.h>

YAIC_NAMESPACE MSPACKETS_NAMESPACE

MasterHeartbeat::MasterHeartbeat() :
    Packet(Packet::Type::MasterHeartbeat)
{

}

bool MasterHeartbeat::decodePayload(const Vector<char> &payload)
{
    UNUSED(payload);

    return true;
}

void MasterHeartbeat::encodePayload(Vector<char> &payload) const
{
    UNUSED(payload);
}


SlaveHeartbeat::SlaveHeartbeat() :
    Packet(Packet::Type::SlaveHeartbeat)
{

}

SlaveHeartbeat::SlaveHeartbeat(uint connections) :
    Packet(Packet::Type::SlaveHeartbeat), m_connections(connections)
{

}

bool SlaveHeartbeat::decodePayload(const Vector<char> &payload)
{
    return read(payload, m_connections);
}

void SlaveHeartbeat::encodePayload(Vector<char> &payload) const
{
    write(payload, m_connections);
}

u32 SlaveHeartbeat::connections() const
{
    return m_connections;
}

Auth::Auth() :
    Packet(Packet::Type::SlaveAuth)
{

}

bool Auth::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_name);

    result &= read(payload, m_capacity);

    u32 modeid;
    result &= read(payload, modeid);
    m_mode = static_cast<Auth::Mode>(modeid);

    result &= read(payload, m_userAddress);
    result &= read(payload, m_userPort);
    result &= read(payload, m_slaveAddress);
    result &= read(payload, m_slavePort);
    result &= read(payload, m_plaintextPassword);

    return result;
}

void Auth::encodePayload(Vector<char> &payload) const
{
    write(payload, m_name);
    write(payload, m_capacity);
    write(payload, static_cast<u32>(m_mode));
    write(payload, m_userAddress);
    write(payload, m_userPort);
    write(payload, m_slaveAddress);
    write(payload, m_slavePort);
    write(payload, m_plaintextPassword);
}

const String &Auth::name() const
{
    return m_name;
}

void Auth::setName(const String &name)
{
    m_name = name;
}

Auth::Mode Auth::mode() const
{
    return m_mode;
}

void Auth::setMode(const Mode &mode)
{
    m_mode = mode;
}

const String &Auth::userAddress() const
{
    return m_userAddress;
}

void Auth::setUserAddress(const String &userAddress)
{
    m_userAddress = userAddress;
}

u16 Auth::userPort() const
{
    return m_userPort;
}

void Auth::setUserPort(const u16 &userPort)
{
    m_userPort = userPort;
}

const String &Auth::slaveAddress() const
{
    return m_slaveAddress;
}

void Auth::setSlaveAddress(const String &slaveAddress)
{
    m_slaveAddress = slaveAddress;
}

u16 Auth::slavePort() const
{
    return m_slavePort;
}

void Auth::setSlavePort(const u16 &slavePort)
{
    m_slavePort = slavePort;
}

const String &Auth::plaintextPassword() const
{
    return m_plaintextPassword;
}

void Auth::setPlaintextPassword(const String &plaintextPassword)
{
    m_plaintextPassword = plaintextPassword;
}

u32 Auth::capacity() const
{
    return m_capacity;
}

void Auth::setCapacity(const u32 &capacity)
{
    m_capacity = capacity;
}

NewAck::NewAck() :
    Packet(Packet::Type::SlaveNewAck)
{

}

NewAck::NewAck(u32 id) :
    Packet(Packet::Type::SlaveNewAck), m_id(id)
{

}

bool NewAck::decodePayload(const Vector<char> &payload)
{
    return read(payload, m_id);
}

void NewAck::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
}

u32 NewAck::id() const
{
    return m_id;
}

AuthResponse::AuthResponse() :
    Packet(Packet::Type::SlaveAuthResponse)
{

}

bool AuthResponse::decodePayload(const Vector<char> &payload)
{
    u32 statusid;
    if (!read(payload, statusid))
        return false;
    m_status = static_cast<AuthResponse::Status>(statusid);

    if (!read(payload, m_id))
        return false;

    if (!read(payload, m_authPassword))
        return false;

    return true;
}

void AuthResponse::encodePayload(Vector<char> &payload) const
{
    write(payload, static_cast<u32>(m_status));
    write(payload, m_id);
    write(payload, m_authPassword);
}

AuthResponse::Status AuthResponse::status() const
{
    return m_status;
}

u32 AuthResponse::id() const
{
    return m_id;
}

void AuthResponse::setStatus(AuthResponse::Status status)
{
    m_status = status;
}

void AuthResponse::setId(u32 id)
{
    m_id = id;
}

u64 AuthResponse::authPassword() const
{
    return m_authPassword;
}

void AuthResponse::setAuthPassword(const u64 &authPassword)
{
    m_authPassword = authPassword;
}

NewSlave::NewSlave() :
    Packet(Packet::Type::NewSlave)
{

}

bool NewSlave::decodePayload(const Vector<char> &payload)
{
    bool result = read(payload, m_id);
    result &= read(payload, m_address);
    result &= read(payload, m_port);

    return result;
}

void NewSlave::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
    write(payload, m_address);
    write(payload, m_port);
}

u32 NewSlave::id() const
{
    return m_id;
}

const String &NewSlave::address() const
{
    return m_address;
}

u16 NewSlave::port() const
{
    return m_port;
}

void NewSlave::setId(u32 id)
{
    m_id = id;
}

void NewSlave::setAddress(const String &address)
{
    m_address.assign(address);
}

void NewSlave::setPort(u16 port)
{
    m_port = port;
}

RemoveSlave::RemoveSlave() :
    Packet(Packet::Type::RemoveSlave)
{

}

RemoveSlave::RemoveSlave(u32 id) :
    Packet(Packet::Type::RemoveSlave), m_id(id)
{

}

bool RemoveSlave::decodePayload(const Vector<char> &payload)
{
    return read(payload, m_id);
}

void RemoveSlave::encodePayload(Vector<char> &payload) const
{
    write(payload, m_id);
}

u32 RemoveSlave::id() const
{
    return m_id;
}

SyncEnd::SyncEnd() :
    Packet(Packet::Type::SlaveSyncEnd)
{

}

bool SyncEnd::decodePayload(const Vector<char> &payload)
{
    UNUSED(payload);
    return true;
}

void SyncEnd::encodePayload(Vector<char> &payload) const
{
    UNUSED(payload);
}

SyncStart::SyncStart() :
    Packet(Packet::Type::SlaveSyncStart)
{

}

bool SyncStart::decodePayload(const Vector<char> &payload)
{
    UNUSED(payload);
    return true;
}

void SyncStart::encodePayload(Vector<char> &payload) const
{
    UNUSED(payload);
}

END_NAMESPACE END_NAMESPACE
