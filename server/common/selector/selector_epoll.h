#pragma once

#include <common/types.h>
#include <server/selector.h>

#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/time.h>

YAIC_NAMESPACE

class SelectorApiEpoll : public Selector {
public:
    SelectorApiEpoll(int bufsize);
    virtual ~SelectorApiEpoll();

    virtual void add(int fd, int type, void *data, int eventType);
    virtual void close(int fd);
    virtual void modify(int fd, int eventType);
    virtual void remove(int fd);
    virtual WaitRetval wait(Vector<SelectorEvent> &events);

protected:
    int m_bufsize;
    int m_epollfd;
    HashMap<int, SelectorInfo*> m_info;
};

END_NAMESPACE
