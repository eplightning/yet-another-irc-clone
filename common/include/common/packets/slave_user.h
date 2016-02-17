#pragma once

#include <common/packet.h>
#include <common/types.h>

#define SUPACKETS_NAMESPACE namespace SlaveUserPackets {

YAIC_NAMESPACE SUPACKETS_NAMESPACE

class UserHeartbeat : public Packet {
public:
    UserHeartbeat();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;
};

class SlaveHeartbeat : public Packet {
public:
    SlaveHeartbeat();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;
};

class Handshake : public Packet {
public:
    Handshake();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    const String &nick() const;

    void setNick(const String &nick);

protected:
    String m_nick;
};

class HandshakeAck : public Packet {
public:
    enum class Status : u32 {
        Ok = 0,
        UnknownError = 1,
        InvalidNick = 2,
        Full = 3
    };

    HandshakeAck();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u64 userId() const;
    Status status() const;

    void setUserId(u64 userid);
    void setStatus(Status status);

protected:
    Status m_status;
    u64 m_userid;
};

END_NAMESPACE END_NAMESPACE
