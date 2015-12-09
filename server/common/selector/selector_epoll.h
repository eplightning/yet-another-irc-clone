#pragma once

#include <common/types.h>
#include <server/selector.h>

#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/time.h>

YAIC_NAMESPACE

class SelectorApiEpoll : public Selector {
public:
    explicit SelectorApiEpoll(int bufsize);
    ~SelectorApiEpoll();

    void add(int fd, int type, void *data, int eventType);
    void close(int fd);
    void modify(int fd, int eventType);
    void remove(int fd);
    bool wait(Vector<SelectorEvent> &events);

protected:
    int m_bufsize;
    int m_epollfd;
    HashMap<int, SelectorInfo*> m_info;
};

END_NAMESPACE
