#pragma once

#include <common/types.h>
#include <common/packet.h>

#include <functional>
#include <deque>

YAIC_NAMESPACE

#define BIND_DISPATCH(O, F) std::bind(F, O, std::placeholders::_1, std::placeholders::_2)
#define BIND_TIMER(O, F) std::bind(F, O, std::placeholders::_1)

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

class TimerDispatcher {
public:
    enum class Result {
        Continue,
        Stop
    };

    typedef std::function<Result(int timer)> DispatchFunction;

    TimerDispatcher();

    void dispatch(int timer) const;
    void append(int timer, DispatchFunction func);
    void prepend(int timer, DispatchFunction func);
    void remove(int timer);

protected:
    HashMap<int, std::deque<DispatchFunction>> m_routing;
};

END_NAMESPACE
