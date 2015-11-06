#pragma once

#include <common/types.h>
#include <server/selector.h>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

YAIC_NAMESPACE

class SelectorApiKqueue : public Selector {
public:
    SelectorApiKqueue(int bufsize);
    virtual ~SelectorApiKqueue();

    virtual void add(int fd, int type, void *data, int eventType);
    virtual void modify(int fd, int eventType);
    virtual void remove(int fd);
    virtual WaitRetval wait(Vector<SelectorEvent> &events);

protected:
    void appendChange(int fd, int filter, int flags, int fflags, SelectorInfo *info);

    int m_bufsize;
    int m_kqueuefd;
    HashMap<int, SelectorInfo*> m_info;
    Vector<struct kevent> m_changes;
};

END_NAMESPACE
