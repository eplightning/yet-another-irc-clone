#include <server/event.h>

#include <common/types.h>
#include <common/packet.h>

YAIC_NAMESPACE

Event::~Event()
{

}

EventQueue::EventQueue()
    : m_cond(), m_mutex(), m_events(), m_stopped(false)
{

}

EventQueue::~EventQueue()
{
    while (!m_events.empty()) {
        Event *ev = m_events.front();
        delete ev;
        m_events.pop();
    }
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

    while (!m_stopped && m_events.empty())
        m_cond.wait(lock);

    if (m_stopped)
        return nullptr;

    Event *ev = m_events.front();
    m_events.pop();

    return ev;
}

void EventQueue::stop()
{
    m_mutex.lock();
    m_stopped = true;
    m_mutex.unlock();

    m_cond.notify_all();
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

EventSimple::EventSimple(EventSimple::EventId type, EventSimple::SimpleType param) :
    m_id(type), m_param(param)
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

EventSimple::SimpleType EventSimple::param() const
{
    return m_param;
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

EventLoop::EventLoop(int workers, EventQueue *evq, EventLoopDelegate func)
    : m_workers(workers), m_evq(evq), m_handler(func)
{

}

EventLoop::~EventLoop()
{
    waitForThreads();
}

void EventLoop::run()
{
    Event *ev;

    do {
        ev = m_evq->pop();

        if (ev == nullptr)
            break;
    } while (m_handler(ev));
}

void EventLoop::startThreads()
{
    if (!m_threads.empty())
        waitForThreads();

    for (int i = 0; i < m_workers; i++)
        m_threads.emplace_back(&EventLoop::run, this);
}

void EventLoop::waitForThreads()
{
    for (auto &x : m_threads) {
        if (x.joinable())
            x.join();
    }

    m_threads.clear();
}

EventSimple::SimpleType::SimpleType(u64 id)
{
    this->id = id;
}

EventSimple::SimpleType::SimpleType(u32 clientid)
{
    this->clientid = clientid;
}

EventSimple::SimpleType::SimpleType(void *data)
{
    this->data = data;
}

EventSimple::SimpleType::SimpleType(int num)
{
    this->num = num;
}

END_NAMESPACE
