#pragma once

#include <common/types.h>

#include <arpa/inet.h>

#define PACKETFACTORY_CASE(X, Y) case X: out = new Y; break;

YAIC_NAMESPACE

#pragma pack(push, 1)
struct PacketHeader {
    u16 type;
    u16 payloadSize;
};
#pragma pack(pop)

const static uint PACKET_MAX_SIZE = 32 * 1024;
const static uint PACKET_HEADER_SIZE = sizeof(PacketHeader);
const static uint PACKET_MAX_PAYLOAD_SIZE = PACKET_MAX_SIZE - PACKET_HEADER_SIZE;

class Packet {
public:
    enum class Direction : u8 {
        UserToSlave = 0, // <1;
        SlaveToUser = 1, // <8192;
        UserToMaster = 2, // <16384;
        MasterToUser = 3, // <24576;
        SlaveToMaster = 4, // <32768;
        MasterToSlave = 5, // <40960;
        SlaveToSlave = 6, // <49152;
        Unknown = 7
    };

    enum class Type : u16 {
        Unknown = 0,
        RequestServers = 16384,
        ServerList = 24576
    };

    static bool checkDirection(u16 rawType, Direction dir);
    static Packet *factory(PacketHeader header, const Vector<char> &data);

public:
    Packet(Type type);
    virtual ~Packet();

    virtual bool decodePayload(const Vector<char> &payload) = 0;
    virtual void encodePayload(Vector<char> &payload) const = 0;

    void encode(Vector<char> &packet) const;
    Vector<char> encode() const;
    Type packetType() const;

protected:
    void write(Vector<char> &payload, u8 data) const;
    void write(Vector<char> &payload, s8 data) const;
    void write(Vector<char> &payload, u16 data) const;
    void write(Vector<char> &payload, s16 data) const;
    void write(Vector<char> &payload, u32 data) const;
    void write(Vector<char> &payload, s32 data) const;
    void write(Vector<char> &payload, const String &data) const;

    bool read(const Vector<char> &payload, u8 &data);
    bool read(const Vector<char> &payload, s8 &data);
    bool read(const Vector<char> &payload, s16 &data);
    bool read(const Vector<char> &payload, u16 &data);
    bool read(const Vector<char> &payload, s32 &data);
    bool read(const Vector<char> &payload, u32 &data);
    bool read(const Vector<char> &payload, String &data);

protected:
    Type m_packetType;
    uint m_packetReaderPos;
};

inline void Packet::write(Vector<char> &payload, u8 data) const
{
    payload.push_back(data);
}

inline void Packet::write(Vector<char> &payload, s8 data) const
{
    payload.push_back(data);
}

inline void Packet::write(Vector<char> &payload, u16 data) const
{
    data = htons(data);
    char *raw = reinterpret_cast<char*>(&data);
    payload.insert(payload.end(), raw, raw + 2);
}

inline void Packet::write(Vector<char> &payload, s16 data) const
{
    data = htons(data);
    char *raw = reinterpret_cast<char*>(&data);
    payload.insert(payload.end(), raw, raw + 2);
}

inline void Packet::write(Vector<char> &payload, u32 data) const
{
    data = htonl(data);
    char *raw = reinterpret_cast<char*>(&data);
    payload.insert(payload.end(), raw, raw + 4);
}

inline void Packet::write(Vector<char> &payload, s32 data) const
{
    data = htonl(data);
    char *raw = reinterpret_cast<char*>(&data);
    payload.insert(payload.end(), raw, raw + 4);
}

inline void Packet::write(Vector<char> &payload, const String &data) const
{
    write(payload, static_cast<u32>(data.size()));
    payload.insert(payload.end(), data.cbegin(), data.cend());
}

inline bool Packet::read(const Vector<char> &payload, u8 &data)
{
    if (payload.size() < 1 + m_packetReaderPos)
        return false;

    data = static_cast<u8>(payload[m_packetReaderPos]);
    m_packetReaderPos += 1;
    return true;
}

inline bool Packet::read(const Vector<char> &payload, s8 &data)
{
    if (payload.size() < 1 + m_packetReaderPos)
        return false;

    data = static_cast<s8>(payload[m_packetReaderPos]);
    m_packetReaderPos += 1;
    return true;
}

inline bool Packet::read(const Vector<char> &payload, s16 &data)
{
    if (payload.size() < 2 + m_packetReaderPos)
        return false;

    data = ntohs(*(reinterpret_cast<const s16*>(&payload[m_packetReaderPos])));
    m_packetReaderPos += 2;
    return true;
}

inline bool Packet::read(const Vector<char> &payload, u16 &data)
{
    if (payload.size() < 2 + m_packetReaderPos)
        return false;

    data = ntohs(*(reinterpret_cast<const u16*>(&payload[m_packetReaderPos])));
    m_packetReaderPos += 2;
    return true;
}

inline bool Packet::read(const Vector<char> &payload, s32 &data)
{
    if (payload.size() < 4 + m_packetReaderPos)
        return false;

    data = ntohl(*(reinterpret_cast<const s32*>(&payload[m_packetReaderPos])));
    m_packetReaderPos += 4;
    return true;
}

inline bool Packet::read(const Vector<char> &payload, u32 &data)
{
    if (payload.size() < 4 + m_packetReaderPos)
        return false;

    data = ntohl(*(reinterpret_cast<const u32*>(&payload[m_packetReaderPos])));
    m_packetReaderPos += 4;
    return true;
}

inline bool Packet::read(const Vector<char> &payload, String &data)
{
    u32 len;
    if (!read(payload, len) || payload.size() < len + m_packetReaderPos)
        return false;

    data.reserve(len);
    data.assign(payload.cbegin() + m_packetReaderPos, payload.cbegin() + m_packetReaderPos + len);
    m_packetReaderPos += len;
    return true;
}

END_NAMESPACE
