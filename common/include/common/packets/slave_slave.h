#pragma once

#include <common/packet.h>
#include <common/types.h>

#define SSPACKETS_NAMESPACE namespace SlaveSlavePackets {

YAIC_NAMESPACE SSPACKETS_NAMESPACE

class Hello : public Packet {
public:
    Hello();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u32 id() const;
    void setId(u32 id);

    const String &name() const;
    void setName(const String &name);

    u64 authPassword() const;
    void setAuthPassword(u64 authPassword);

protected:
    u32 m_id;
    String m_name;
    u64 m_authPassword;
};

class HelloResponse : public Packet {
public:
    HelloResponse();
    HelloResponse(u32 id);

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    u32 id() const;

    u64 authPassword() const;
    void setAuthPassword(u64 authPassword);

protected:
    u32 m_id;
    u64 m_authPassword;
};

class Heartbeat : public Packet {
public:
    Heartbeat();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;
};

END_NAMESPACE END_NAMESPACE
