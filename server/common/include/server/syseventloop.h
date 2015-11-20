#pragma once

#include <common/types.h>
#include <server/event.h>

YAIC_NAMESPACE

class SysEventLoop {
public:
    static SysEventLoop *factory(EventQueue *evq);

    SysEventLoop(EventQueue *evq);
    virtual ~SysEventLoop();

    virtual int addTimer(uint seconds) = 0;
    virtual void removeTimer(int id) = 0;
    virtual bool runLoop() = 0;
    virtual void stopLoop() = 0;

protected:
    EventQueue *m_evq;
};

END_NAMESPACE
