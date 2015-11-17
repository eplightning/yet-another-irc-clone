#pragma once

#include <common/packet.h>
#include <common/types.h>

#define MUPACKETS_NAMESPACE namespace MasterUserPackets {

YAIC_NAMESPACE MUPACKETS_NAMESPACE

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

END_NAMESPACE END_NAMESPACE
