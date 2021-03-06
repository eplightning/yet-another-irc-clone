#pragma once

#include <common/types.h>
#include <server/event.h>

YAIC_NAMESPACE

/**
 * @brief Abstrakcja na obsługe podstawowych wydarzeń systemowych (timery, sygnaly)
 */
class SysEventLoop {
public:
    static SysEventLoop *factory(EventQueue &evq);

    explicit SysEventLoop(EventQueue *evq);
    virtual ~SysEventLoop();

    virtual int addTimer(uint seconds) = 0;
    virtual void removeTimer(int id) = 0;
    virtual bool runLoop() = 0;
    virtual void stopLoop() = 0;

protected:
    EventQueue *m_evq;
};

END_NAMESPACE
