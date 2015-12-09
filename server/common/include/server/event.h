#pragma once

#include <common/types.h>
#include <common/packet.h>

#include <mutex>
#include <condition_variable>
#include <queue>

YAIC_NAMESPACE

class Event {
public:
    enum class Type {
        Packet = 1,
        Simple,
        Timer
    };

    virtual ~Event();
    virtual Type type() const = 0;
};

class EventQueue {
public:
    EventQueue();
    ~EventQueue();

    void append(Event *event);
    Event *pop();

protected:
    std::condition_variable m_cond;
    std::mutex m_mutex;
    std::queue<Event*> m_events;
};

class EventPacket : public Event {
public:
    EventPacket(Packet *packet, uint clientid, int source = 0);

    Type type() const;

    Packet *packet() const;
    uint clientid() const;
    int source() const;

protected:
    Packet *m_packet;
    uint m_clientid;
    int m_source;
};

class EventSimple : public Event {
public:
    enum class EventId {
        SignalTerminate = 0,
        SignalInterrupt,
        SignalQuit,
        SignalHangUp,
        TcpLoopDied,
        SysLoopDied
    };

    EventSimple(EventId type);

    Type type() const;
    EventId id() const;

protected:
    EventId m_id;
};

class EventTimer : public Event {
public:
    EventTimer(int timer);

    Type type() const;
    int timer() const;

protected:
    int m_timer;
};

END_NAMESPACE
