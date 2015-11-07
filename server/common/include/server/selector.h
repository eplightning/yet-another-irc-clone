#pragma once

#include <common/types.h>

YAIC_NAMESPACE

class SelectorInfo {
public:
    const static int ReadEvent;
    const static int WriteEvent;
    const static int CloseEvent;

    SelectorInfo(int fd, int type, void *data, int evtype);

    int fd() const { return m_fd; }
    int type() const { return m_type; }
    void *data() const { return m_data; }
    int eventType() const { return m_eventType; }
    bool closed() const { return m_closed; }

    void setEventType(int value) { m_eventType = value; }
    void setClosed(bool closed) { m_closed = closed; }

protected:
    int m_fd;
    int m_type;
    void *m_data;
    int m_eventType;
    bool m_closed;
};

class SelectorEvent {
public:
    SelectorEvent(const SelectorInfo *info, int type);

    const SelectorInfo *info() const { return m_info; }
    int type() const { return m_type; }

protected:
    const SelectorInfo *m_info;
    int m_type;
};

class Selector {
public:
    enum class WaitRetval {
        Success = 0,
        Error = 1
    };

    static Selector *factory();

    virtual ~Selector() {}

    virtual void add(int fd, int type, void *data, int eventType) = 0;
    virtual void close(int fd) {UNUSED(fd);}
    virtual void modify(int fd, int eventType) = 0;
    virtual void remove(int fd) = 0;
    virtual WaitRetval wait(Vector<SelectorEvent> &events) = 0;
};

END_NAMESPACE
