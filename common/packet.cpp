#include <common/packet.h>

#include <common/types.h>

YAIC_NAMESPACE

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

bool Packet::checkDirection(u16 rawType, Packet::Direction dir)
{
    rawType >>= 13;

    return static_cast<u8>(dir) == rawType;
}

END_NAMESPACE
