#include <server/dispatcher.h>

#include <common/types.h>
#include <common/packet.h>

YAIC_NAMESPACE

PacketDispatcher::PacketDispatcher()
{

}

void PacketDispatcher::dispatch(uint clientid, Packet *packet) const
{
    auto it = m_routing.find(static_cast<u16>(packet->packetType()));

    if (it == m_routing.cend())
        return;

    for (auto &x : it->second) {
        Result res = x(clientid, packet);

        if (res == Result::Stop)
            break;
    }
}

void PacketDispatcher::append(Packet::Type packet, PacketDispatcher::DispatchFunction func)
{
    append(static_cast<u16>(packet), func);
}

void PacketDispatcher::append(u16 packet, PacketDispatcher::DispatchFunction func)
{
    auto it = m_routing.emplace(std::piecewise_construct, std::forward_as_tuple(packet), std::forward_as_tuple());

    (*(it.first)).second.push_back(func);
}

void PacketDispatcher::prepend(Packet::Type packet, PacketDispatcher::DispatchFunction func)
{
    prepend(static_cast<u16>(packet), func);
}

void PacketDispatcher::prepend(u16 packet, PacketDispatcher::DispatchFunction func)
{
    auto it = m_routing.emplace(std::piecewise_construct, std::forward_as_tuple(packet), std::forward_as_tuple());

    (*(it.first)).second.push_front(func);
}

TimerDispatcher::TimerDispatcher()
{

}

void TimerDispatcher::dispatch(int timer) const
{
    auto it = m_routing.find(timer);

    if (it == m_routing.cend())
        return;

    for (auto &x : it->second) {
        Result res = x(timer);

        if (res == Result::Stop)
            break;
    }
}

void TimerDispatcher::append(int timer, TimerDispatcher::DispatchFunction func)
{
    auto it = m_routing.emplace(std::piecewise_construct, std::forward_as_tuple(timer),
                                std::forward_as_tuple());

    (*(it.first)).second.push_back(func);
}

void TimerDispatcher::prepend(int timer, TimerDispatcher::DispatchFunction func)
{
    auto it = m_routing.emplace(std::piecewise_construct, std::forward_as_tuple(timer),
                                std::forward_as_tuple());

    (*(it.first)).second.push_front(func);
}

void TimerDispatcher::remove(int timer)
{
    m_routing.erase(timer);
}



END_NAMESPACE
