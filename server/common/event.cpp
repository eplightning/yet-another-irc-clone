#include <server/event.h>

#include <common/types.h>
#include <common/packet.h>

YAIC_NAMESPACE

EventQueue::EventQueue() : m_cond(), m_mutex(), m_events()
{

}

void EventQueue::append(Event *event)
{
    m_mutex.lock();
    m_events.push(event);
    m_mutex.unlock();

    m_cond.notify_one();
}

Event *EventQueue::pop()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    while (m_events.empty())
        m_cond.wait(lock);

    Event *ev = m_events.front();
    m_events.pop();

    return ev;
}

EventPacket::EventPacket(Packet *packet, uint clientid, int source) :
    m_packet(packet), m_clientid(clientid), m_source(source)
{

}

Event::Type EventPacket::type() const
{
    return Event::Type::Packet;
}

Packet *EventPacket::packet() const
{
    return m_packet;
}

uint EventPacket::clientid() const
{
    return m_clientid;
}

int EventPacket::source() const
{
    return m_source;
}

EventSimple::EventSimple(EventSimple::EventId type) :
    m_id(type)
{

}

Event::Type EventSimple::type() const
{
    return Event::Type::Simple;
}

EventSimple::EventId EventSimple::id() const
{
    return m_id;
}

EventTimer::EventTimer(int timer) :
    m_timer(timer)
{

}

Event::Type EventTimer::type() const
{
    return Event::Type::Timer;
}

int EventTimer::timer() const
{
    return m_timer;
}



END_NAMESPACE
