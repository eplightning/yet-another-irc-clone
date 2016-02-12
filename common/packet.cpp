#include <common/packet.h>

#include <common/types.h>
#include <common/packets/master_user.h>
#include <common/packets/master_slave.h>

YAIC_NAMESPACE

const uint Packet::MaxSize = 96 * 1024;
const uint Packet::HeaderSize = sizeof(PacketHeader);
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
    *payloadSize = htons(static_cast<u32>(packet.size() - sizeof(PacketHeader)));
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
    Packet *out;

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
    //case Type::Unknown:
    default:
        return nullptr;
    }

    // pomijamy nagłówek
    out->m_packetReaderPos = sizeof(PacketHeader);

    if (out->decodePayload(data))
        return out;

    delete out;
    return nullptr;
}

END_NAMESPACE
