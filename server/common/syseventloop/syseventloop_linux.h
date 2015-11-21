#pragma once

#include <common/types.h>
#include <server/syseventloop.h>
#include <vector>

YAIC_NAMESPACE

class SysEventLoopApiLinux : public SysEventLoop {
public:
    SysEventLoopApiLinux(EventQueue *evq);
    ~SysEventLoopApiLinux();

    int addTimer(uint seconds);
    void removeTimer(int id);
    bool runLoop();
    void stopLoop();

protected:
    int m_epollfd;
    Vector <int> m_timerfd;
    int m_eventfd;
};

END_NAMESPACE
