#pragma once

#include <common/types.h>
#include <common/packet.h>

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>

YAIC_NAMESPACE

/**
 * @brief Reprezentuje wydarzenie w kolejce
 */
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

/**
 * @brief Kolejka wydarzeń
 */
class EventQueue {
public:
    EventQueue();
    ~EventQueue();

    void append(Event *event);
    Event *pop();
    void stop();

protected:
    std::condition_variable m_cond;
    std::mutex m_mutex;
    std::queue<Event*> m_events;
    bool m_stopped;
};

#define BIND_EVENT_LOOP_HANDLER(O, F) std::bind(F, O, std::placeholders::_1)

typedef std::function<bool(Event*)> EventLoopDelegate;

/**
 * @brief Główna pętla aplikacji obsługująca wydarzenia pojawiające się w kolejce
 */
class EventLoop {
public:
    EventLoop(int workers, EventQueue *evq, EventLoopDelegate func);
    ~EventLoop();

    void run();
    void startThreads();
    void waitForThreads();

protected:
    int m_workers;
    EventQueue *m_evq;
    EventLoopDelegate m_handler;
    Vector<std::thread> m_threads;
};

class EventPacket : public Event {
public:
    EventPacket(Packet *packet, u32 clientid, int source = 0);

    Type type() const;

    Packet *packet() const;
    u32 clientid() const;
    int source() const;

protected:
    Packet *m_packet;
    u32 m_clientid;
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
        SysLoopDied,
        MasterDisconnected,
        UserDisconnected,
        UserForeignDisconnected
    };

    union SimpleType {
        u64 id;
        u32 clientid;
        void *data;
        int num;

        SimpleType(u64 id);
        SimpleType(u32 clientid);
        SimpleType(void *data);
        SimpleType(int num);
    };

    EventSimple(EventId type, SimpleType param = 0);

    Type type() const;
    EventId id() const;
    SimpleType param() const;

protected:
    EventId m_id;
    SimpleType m_param;
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
