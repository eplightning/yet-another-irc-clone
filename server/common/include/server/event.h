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
        Packet = 1
    };

    virtual ~Event() {}
    virtual Type type() = 0;
};

class EventQueue {
public:
    EventQueue();

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

END_NAMESPACE
