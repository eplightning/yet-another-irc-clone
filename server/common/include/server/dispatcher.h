#pragma once

#include <common/types.h>
#include <common/packet.h>

#include <functional>

YAIC_NAMESPACE

#define BIND_DISPATCH(O, F) std::bind(F, O, std::placeholders::_1, std::placeholders::_2)
#define BIND_TIMER(O, F) std::bind(F, O, std::placeholders::_1)

class PacketDispatcher {
public:
    typedef std::function<bool(uint clientid, Packet *packet)> DispatchFunction;

    PacketDispatcher();

    void dispatch(uint clientid, Packet *packet) const;
    void append(Packet::Type packet, const DispatchFunction &func);
    void append(u16 packet, const DispatchFunction &func);

protected:
    HashMap<u16, Vector<DispatchFunction>> m_routing;
};

class TimerDispatcher {
public:
    typedef std::function<bool(int timer)> DispatchFunction;

    TimerDispatcher();

    void dispatch(int timer) const;
    void append(int timer, const DispatchFunction &func);
    void remove(int timer);

protected:
    HashMap<int, Vector<DispatchFunction>> m_routing;
};

END_NAMESPACE
