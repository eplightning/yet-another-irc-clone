#pragma once

#include <common/types.h>
#include <common/packet.h>

#include <functional>
#include <deque>

YAIC_NAMESPACE

#define BIND_DISPATCH(O, F) std::bind(F, O, std::placeholders::_1, std::placeholders::_2)

class PacketDispatcher {
public:
    enum class Result {
        Continue,
        Stop
    };

    typedef std::function<Result(uint clientid, Packet *packet)> DispatchFunction;

    PacketDispatcher();

    void dispatch(uint clientid, Packet *packet) const;
    void append(Packet::Type packet, DispatchFunction func);
    void append(u16 packet, DispatchFunction func);
    void prepend(Packet::Type packet, DispatchFunction func);
    void prepend(u16 packet, DispatchFunction func);

protected:
    HashMap<u16, std::deque<DispatchFunction>> m_routing;
};

END_NAMESPACE
