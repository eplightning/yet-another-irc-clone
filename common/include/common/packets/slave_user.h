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

END_NAMESPACE END_NAMESPACE
