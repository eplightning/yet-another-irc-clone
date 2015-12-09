#pragma once

#include <common/packet.h>
#include <common/types.h>

#define MUPACKETS_NAMESPACE namespace MasterUserPackets {

YAIC_NAMESPACE MUPACKETS_NAMESPACE

// -------------------------
// User -> Master
// -------------------------

class RequestServers : public Packet {
public:
    const static int FlagIpv4Only;
    const static int FlagIpv6Only;

    RequestServers();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    s32 flags() const;
    void setFlags(s32 flags);

    u8 max() const;
    void setMax(u8 max);

protected:
    s32 m_flags;
    u8 m_max;
};

// -------------------------
// Master -> User
// -------------------------

class ServerListServer {
public:
    ServerListServer();
    ServerListServer(const String &addr, u16 port);

    String address;
    u16 port;
};

class ServerList : public Packet {
public:
    ServerList();

    bool decodePayload(const Vector<char> &payload);
    void encodePayload(Vector<char> &payload) const;

    Vector<ServerListServer> &servers();
    const Vector<ServerListServer> &servers() const;

protected:
    Vector<ServerListServer> m_servers;
};

END_NAMESPACE END_NAMESPACE
