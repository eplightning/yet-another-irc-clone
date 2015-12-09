#include <common/packets/master_user.h>

#include <common/packet.h>
#include <common/types.h>

YAIC_NAMESPACE MUPACKETS_NAMESPACE

const int RequestServers::FlagIpv4Only = 1 << 0;
const int RequestServers::FlagIpv6Only = 1 << 1;

RequestServers::RequestServers()
    : Packet(Packet::Type::RequestServers), m_flags(0), m_max(0)
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

ServerListServer::ServerListServer()
{

}

ServerListServer::ServerListServer(const String &addr, u16 port)
    : address(addr), port(port)
{

}

ServerList::ServerList()
    : Packet(Packet::Type::ServerList)
{

}

bool ServerList::decodePayload(const Vector<char> &payload)
{
    u32 size;
    if (!readVectorSize(payload, size))
        return false;

    m_servers.resize(size);

    for (uint i = 0; i < size; i++) {
        bool result = read(payload, m_servers[i].address);
        result &= read(payload, m_servers[i].port);

        if (!result)
            return false;
    }

    return true;
}

void ServerList::encodePayload(Vector<char> &payload) const
{
    writeVectorSize(payload, m_servers.size());

    for (auto &x : m_servers) {
        write(payload, x.address);
        write(payload, x.port);
    }
}

Vector<ServerListServer> &ServerList::servers()
{
    return m_servers;
}

const Vector<ServerListServer> &ServerList::servers() const
{
    return m_servers;
}


END_NAMESPACE END_NAMESPACE
