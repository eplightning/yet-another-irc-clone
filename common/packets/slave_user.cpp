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


END_NAMESPACE END_NAMESPACE
