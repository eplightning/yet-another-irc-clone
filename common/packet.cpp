#include <common/packet.h>

#include <common/types.h>
#include <common/packets/master_user.h>
#include <common/packets/master_slave.h>
#include <common/packets/slave_slave.h>
#include <common/packets/slave_user.h>

YAIC_NAMESPACE

const uint Packet::MaxSize = 96 * 1024;
const uint Packet::HeaderSize = sizeof(u16) + sizeof(u32);
const uint Packet::MaxPayloadSize = Packet::MaxSize - Packet::HeaderSize;
const uint Packet::MaxVectorSize = 16 * 1024;

Packet::Packet(Packet::Type type)
    : m_packetType(type), m_packetReaderPos(0)
{

}

Packet::~Packet()
{

}

void Packet::encode(Vector<char> &packet) const
{
    packet.reserve(4);
    write(packet, static_cast<u16>(m_packetType));
    write(packet, static_cast<u32>(0));

    encodePayload(packet);

    u32 *payloadSize = reinterpret_cast<u32*>(&packet[sizeof(u16)]);
    *payloadSize = htonl(static_cast<u32>(packet.size() - HeaderSize));
}

Vector<char> Packet::encode() const
{
    Vector<char> result;
    encode(result);
    return result;
}

Packet::Type Packet::packetType() const
{
    return m_packetType;
}

bool Packet::checkDirection(u16 rawType, Packet::Direction dir)
{
    return static_cast<u8>(dir) == (rawType >> 13);
}

Packet *Packet::factory(PacketHeader header, const Vector<char> &data)
{
    Packet *out = nullptr;

    switch (static_cast<Type>(header.type)) {
    PACKETFACTORY_CASE(Type::RequestServers, MasterUserPackets::RequestServers)
    PACKETFACTORY_CASE(Type::ServerList, MasterUserPackets::ServerList)
    PACKETFACTORY_CASE(Type::SlaveHeartbeat, MasterSlavePackets::SlaveHeartbeat)
    PACKETFACTORY_CASE(Type::SlaveAuth, MasterSlavePackets::Auth)
    PACKETFACTORY_CASE(Type::SlaveSyncStart, MasterSlavePackets::SyncStart)
    PACKETFACTORY_CASE(Type::SlaveNewAck, MasterSlavePackets::NewAck)
    PACKETFACTORY_CASE(Type::MasterHeartbeat, MasterSlavePackets::MasterHeartbeat)
    PACKETFACTORY_CASE(Type::SlaveAuthResponse, MasterSlavePackets::AuthResponse)
    PACKETFACTORY_CASE(Type::SlaveSyncEnd, MasterSlavePackets::SyncEnd)
    PACKETFACTORY_CASE(Type::NewSlave, MasterSlavePackets::NewSlave)
    PACKETFACTORY_CASE(Type::RemoveSlave, MasterSlavePackets::RemoveSlave)
    PACKETFACTORY_CASE(Type::SlaveHello, SlaveSlavePackets::Hello)
    PACKETFACTORY_CASE(Type::SlaveHelloResponse, SlaveSlavePackets::HelloResponse)
    PACKETFACTORY_CASE(Type::SlaveSlaveHeartbeat, SlaveSlavePackets::Heartbeat)
    PACKETFACTORY_CASE(Type::SlaveSyncUsers, SlaveSlavePackets::SyncUsers)
    PACKETFACTORY_CASE(Type::SlaveSyncChannels, SlaveSlavePackets::SyncChannels)
    PACKETFACTORY_CASE(Type::SlaveUserConnect, SlaveSlavePackets::UserConnect)
    PACKETFACTORY_CASE(Type::SlaveUserDisconnect, SlaveSlavePackets::UserDisconnect)
    PACKETFACTORY_CASE(Type::SlaveChannelNew, SlaveSlavePackets::ChannelNew)
    PACKETFACTORY_CASE(Type::SlaveChannelRemove, SlaveSlavePackets::ChannelRemove)
    PACKETFACTORY_CASE(Type::SlaveChannelUser, SlaveSlavePackets::ChannelUser)
    PACKETFACTORY_CASE(Type::SlaveChannelUserPart, SlaveSlavePackets::ChannelUserPart)
    PACKETFACTORY_CASE(Type::SlaveChannelMessage, SlaveSlavePackets::ChannelMessage)
    PACKETFACTORY_CASE(Type::SlavePrivateMessage, SlaveSlavePackets::PrivateMessage)
    PACKETFACTORY_CASE(Type::UserHeartbeat, SlaveUserPackets::UserHeartbeat)
    PACKETFACTORY_CASE(Type::Handshake, SlaveUserPackets::Handshake)
    PACKETFACTORY_CASE(Type::ListChannels, SlaveUserPackets::ListChannels)
    PACKETFACTORY_CASE(Type::JoinChannel, SlaveUserPackets::JoinChannel)
    PACKETFACTORY_CASE(Type::PartChannel, SlaveUserPackets::PartChannel)
    PACKETFACTORY_CASE(Type::SendChannelMessage, SlaveUserPackets::SendChannelMessage)
    PACKETFACTORY_CASE(Type::SendPrivateMessage, SlaveUserPackets::SendPrivateMessage)
    PACKETFACTORY_CASE(Type::SlaveUserHeartbeat, SlaveUserPackets::SlaveHeartbeat)
    PACKETFACTORY_CASE(Type::HandshakeAck, SlaveUserPackets::HandshakeAck)
    PACKETFACTORY_CASE(Type::Channels, SlaveUserPackets::Channels)
    PACKETFACTORY_CASE(Type::ChannelJoined, SlaveUserPackets::ChannelJoined)
    PACKETFACTORY_CASE(Type::ChannelParted, SlaveUserPackets::ChannelParted)
    PACKETFACTORY_CASE(Type::ChannelMessage, SlaveUserPackets::ChannelMessage)
    PACKETFACTORY_CASE(Type::ChannelUserJoined, SlaveUserPackets::ChannelUserJoined)
    PACKETFACTORY_CASE(Type::ChannelUserParted, SlaveUserPackets::ChannelUserParted)
    PACKETFACTORY_CASE(Type::ChannelUserUpdated, SlaveUserPackets::ChannelUserUpdated)
    PACKETFACTORY_CASE(Type::UserDisconnected, SlaveUserPackets::UserDisconnected)
    PACKETFACTORY_CASE(Type::UserUpdated, SlaveUserPackets::UserUpdated)
    PACKETFACTORY_CASE(Type::PrivateMessageReceived, SlaveUserPackets::PrivateMessageReceived)
    case Type::Unknown:
        return nullptr;
    }

    if (out == nullptr)
        return nullptr;

    // pomijamy nagłówek
    out->m_packetReaderPos = HeaderSize;

    if (out->decodePayload(data))
        return out;

    delete out;
    return nullptr;
}

END_NAMESPACE
