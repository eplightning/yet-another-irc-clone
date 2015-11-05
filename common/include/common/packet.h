#pragma once

#include <common/types.h>

#include <netinet/in.h>

YAIC_NAMESPACE

#pragma pack(push, 1)
struct RawPacket {
    u16 type;
    u16 payloadSize;
    char payload[1];
};
#pragma pack(pop)

class Packet {
public:
    enum class Type : u16 {
        Unknown = 0,
        RequestServers = 1,
        ServerList = 2
    };

    Packet(Type type) : packetType(type), packetReaderPos(0) {}
    virtual ~Packet() {}

    virtual bool decodePayload(Vector<char> &payload) = 0;
    virtual void encodePayload(Vector<char> &payload) const = 0;

    void encode(Vector<char> &packet) const;
    Vector<char> encode() const;

protected:
    void write(Vector<char> &payload, u8 data) const;
    void write(Vector<char> &payload, s8 data) const;
    void write(Vector<char> &payload, u16 data) const;
    void write(Vector<char> &payload, s16 data) const;
    void write(Vector<char> &payload, u32 data) const;
    void write(Vector<char> &payload, s32 data) const;
    void write(Vector<char> &payload, const String &data) const;

    void read(const Vector<char> &payload, u8 &data);
    void read(const Vector<char> &payload, s8 &data);
    void read(const Vector<char> &payload, s16 &data);
    void read(const Vector<char> &payload, u16 &data);
    void read(const Vector<char> &payload, s32 &data);
    void read(const Vector<char> &payload, u32 &data);
    void read(const Vector<char> &payload, String &data);

public:
    Type packetType;
protected:
    int packetReaderPos;
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

inline void Packet::read(const Vector<char> &payload, u8 &data)
{
    data = static_cast<u8>(payload[packetReaderPos]);
    packetReaderPos += 1;
}

inline void Packet::read(const Vector<char> &payload, s8 &data)
{
    data = static_cast<s8>(payload[packetReaderPos]);
    packetReaderPos += 1;
}

inline void Packet::read(const Vector<char> &payload, s16 &data)
{
    data = ntohs(*(reinterpret_cast<const s16*>(&payload[packetReaderPos])));
    packetReaderPos += 2;
}

inline void Packet::read(const Vector<char> &payload, u16 &data)
{
    data = ntohs(*(reinterpret_cast<const u16*>(&payload[packetReaderPos])));
    packetReaderPos += 2;
}

inline void Packet::read(const Vector<char> &payload, s32 &data)
{
    data = ntohl(*(reinterpret_cast<const s32*>(&payload[packetReaderPos])));
    packetReaderPos += 4;
}

inline void Packet::read(const Vector<char> &payload, u32 &data)
{
    data = ntohl(*(reinterpret_cast<const u32*>(&payload[packetReaderPos])));
    packetReaderPos += 4;
}

inline void Packet::read(const Vector<char> &payload, String &data)
{
    u32 len;
    read(payload, len);
    data.reserve(len);
    data.assign(payload.cbegin() + packetReaderPos, payload.cbegin() + packetReaderPos + len);
    packetReaderPos += len;
}

END_NAMESPACE
