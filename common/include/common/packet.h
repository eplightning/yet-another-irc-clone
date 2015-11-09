#pragma once

#include <common/types.h>

#include <netinet/in.h>

YAIC_NAMESPACE

#pragma pack(push, 1)
struct PacketHeader {
    u16 type;
    u16 payloadSize;
};
#pragma pack(pop)

class Packet {
public:
    enum class Type : u16 {
        Unknown = 0,
        RequestServers = 1,
        ServerList = 2
    };

    Packet(Type type) : m_packetType(type), m_packetReaderPos(0) {}
    virtual ~Packet() {}

    virtual bool decodePayload(Vector<char> &payload) = 0;
    virtual void encodePayload(Vector<char> &payload) const = 0;

    void encode(Vector<char> &packet) const;
    Vector<char> encode() const;
    Type packetType() const { return m_packetType; }

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

const static int PACKET_MAX_SIZE = 32 * 1024;
const static int PACKET_HEADER_SIZE = 2 * sizeof(u16);
const static int PACKET_MAX_PAYLOAD_SIZE = PACKET_MAX_SIZE - PACKET_HEADER_SIZE;

END_NAMESPACE
