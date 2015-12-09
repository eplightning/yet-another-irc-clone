#pragma once

#include <common/types.h>
#include <server/selector.h>

#include <set>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

YAIC_NAMESPACE

class SelectorApiKqueue : public Selector {
public:
    explicit SelectorApiKqueue(int bufsize);
    ~SelectorApiKqueue();

    void add(int fd, int type, void *data, int eventType);
    void close(int fd);
    void modify(int fd, int eventType);
    void remove(int fd);
    bool wait(Vector<SelectorEvent> &events);

protected:
    void appendChange(int fd, int filter, int flags, int fflags, SelectorInfo *info);

    int m_bufsize;
    int m_kqueuefd;
    HashMap<int, SelectorInfo*> m_info;
    Vector<struct kevent> m_changes;
    std::set<int> m_closedFds;
};

END_NAMESPACE
