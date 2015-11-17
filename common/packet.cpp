#include <common/packet.h>

#include <common/types.h>
#include <common/packets/master_user.h>

YAIC_NAMESPACE

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
    write(packet, static_cast<u16>(0));

    encodePayload(packet);

    u16 *payloadSize = reinterpret_cast<u16*>(&packet[sizeof(u16)]);
    *payloadSize = htons(static_cast<u16>(packet.size() - sizeof(u16) * 2));
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
    rawType >>= 13;

    return static_cast<u8>(dir) == rawType;
}

Packet *Packet::factory(PacketHeader header, const Vector<char> &data)
{
    Packet *out;

    switch (static_cast<Type>(header.type)) {
    PACKETFACTORY_CASE(Type::RequestServers, MasterUserPackets::RequestServers)
    default: return nullptr;
    }

    // pomijamy nagłówek
    out->m_packetReaderPos = sizeof(PacketHeader);

    if (out->decodePayload(data))
        return out;

    delete out;
    return nullptr;
}

END_NAMESPACE
