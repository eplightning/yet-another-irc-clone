#include <common/packets/slave_user.h>

#include <common/packet.h>
#include <common/types.h>

YAIC_NAMESPACE SUPACKETS_NAMESPACE

UserHeartbeat::UserHeartbeat() :
    Packet(Packet::Type::UserHeartbeat)
{

}

bool UserHeartbeat::decodePayload(const Vector<char> &payload)
{
    UNUSED(payload);

    return true;
}

void UserHeartbeat::encodePayload(Vector<char> &payload) const
{
    UNUSED(payload);
}

SlaveHeartbeat::SlaveHeartbeat() :
    Packet(Packet::Type::SlaveUserHeartbeat)
{

}

bool SlaveHeartbeat::decodePayload(const Vector<char> &payload)
{
    UNUSED(payload);

    return true;
}

void SlaveHeartbeat::encodePayload(Vector<char> &payload) const
{
    UNUSED(payload);
}

Handshake::Handshake() :
    Packet(Packet::Type::Handshake)
{

}

bool Handshake::decodePayload(const Vector<char> &payload)
{
    return read(payload, m_nick);
}

void Handshake::encodePayload(Vector<char> &payload) const
{
    write(payload, m_nick);
}

const String &Handshake::nick() const
{
    return m_nick;
}

void Handshake::setNick(const String &nick)
{
    m_nick.assign(nick);
}

HandshakeAck::HandshakeAck() :
    Packet(Packet::Type::HandshakeAck)
{

}

bool HandshakeAck::decodePayload(const Vector<char> &payload)
{
    u32 status;
    bool result = read(payload, status);
    m_status = static_cast<HandshakeAck::Status>(status);

    result &= read(payload, m_userid);

    return result;
}

void HandshakeAck::encodePayload(Vector<char> &payload) const
{
    write(payload, static_cast<u32>(m_status));
    write(payload, m_userid);
}

u64 HandshakeAck::userId() const
{
    return m_userid;
}

HandshakeAck::Status HandshakeAck::status() const
{
    return m_status;
}

void HandshakeAck::setUserId(u64 userid)
{
    m_userid = userid;
}

void HandshakeAck::setStatus(HandshakeAck::Status status)
{
    m_status = status;
}


END_NAMESPACE END_NAMESPACE
