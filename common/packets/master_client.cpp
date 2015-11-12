#include <common/packets/master_client.h>

#include <common/packet.h>
#include <common/types.h>

YAIC_NAMESPACE MCPACKETS_NAMESPACE

const int RequestServers::FlagIpv4Only = 1 << 0;
const int RequestServers::FlagIpv6Only = 1 << 1;

RequestServers::RequestServers() : Packet(Packet::Type::RequestServers), m_flags(0), m_max(0)
{

}

bool RequestServers::decodePayload(const Vector<char> &payload)
{
    bool result = true;

    result &= read(payload, m_flags);
    result &= read(payload, m_max);

    return result;
}

void RequestServers::encodePayload(Vector<char> &payload) const
{
    write(payload, m_flags);
    write(payload, m_max);
}

s32 RequestServers::flags() const
{
    return m_flags;
}

void RequestServers::setFlags(s32 flags)
{
    m_flags = flags;
}

u8 RequestServers::max() const
{
    return m_max;
}

void RequestServers::setMax(u8 value)
{
    m_max = value;
}

END_NAMESPACE END_NAMESPACE
