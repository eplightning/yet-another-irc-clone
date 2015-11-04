#pragma once

#include <common/types.h>

#include <netinet/in.h>

YAIC_NAMESPACE

// W nagłówku są definicje żeby kompilator inline'ował to

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

    Packet(Type type) : packetType(type) {}
    virtual ~Packet() {}

    virtual bool fromPayload(u16 size, const char *payload) { UNUSED(size); UNUSED(payload); return true; }
    virtual Vector<char> toPayload() { return Vector<char>(); }

    bool fromRawPacket(const RawPacket &packet) { return fromPayload(packet.payloadSize, packet.payload); }

protected:
    // writing
    void beginWrite(Vector<char> &payload)
    {
        write(payload, static_cast<u16>(packetType));
        write(payload, static_cast<u16>(0));
    }

    void endWrite(Vector<char> &payload)
    {
        u16 *payloadSize = reinterpret_cast<u16*>(&payload[2]);
        *payloadSize = htons(static_cast<u16>(payload.size() - sizeof(u16) * 2));
    }

    void write(Vector<char> &payload, u8 data)
    {
        payload.push_back(data);
    }

    void write(Vector<char> &payload, s8 data)
    {
        payload.push_back(data);
    }

    void write(Vector<char> &payload, u16 data)
    {
        data = htons(data);
        char *raw = reinterpret_cast<char*>(&data);
        payload.insert(payload.end(), raw, raw + 2);
    }

    void write(Vector<char> &payload, s16 data)
    {
        data = htons(data);
        char *raw = reinterpret_cast<char*>(&data);
        payload.insert(payload.end(), raw, raw + 2);
    }

    void write(Vector<char> &payload, u32 data)
    {
        data = htonl(data);
        char *raw = reinterpret_cast<char*>(&data);
        payload.insert(payload.end(), raw, raw + 4);
    }

    void write(Vector<char> &payload, s32 data)
    {
        data = htonl(data);
        char *raw = reinterpret_cast<char*>(&data);
        payload.insert(payload.end(), raw, raw + 4);
    }

    void write(Vector<char> &payload, const String &data)
    {
        write(payload, static_cast<u32>(data.size()));
        payload.insert(payload.end(), data.cbegin(), data.cend());
    }

public:
    Type packetType;
};

static const Packet::Type LAST_PACKET_TYPE = Packet::Type::ServerList;

END_NAMESPACE
