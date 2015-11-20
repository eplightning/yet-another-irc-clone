#pragma once

#include <common/types.h>
#include <server/syseventloop.h>

YAIC_NAMESPACE

class SysEventLoopApiKqueue : public SysEventLoop {
public:
    SysEventLoopApiKqueue(EventQueue *evq);
    ~SysEventLoopApiKqueue();

    int addTimer(uint seconds);
    void removeTimer(int id);
    bool runLoop();
    void stopLoop();

protected:
    int m_kqueuefd;
    int m_timerid;
};

END_NAMESPACE
