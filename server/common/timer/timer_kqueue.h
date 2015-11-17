#pragma once

#include <common/types.h>
#include <server/timer.h>

YAIC_NAMESPACE

class TimerApiKqueue : public Timer {
public:
    TimerApiKqueue(uint seconds);
    ~TimerApiKqueue();

    void start();
    void stop();
    bool wait();

protected:
    int m_kqueuefd;
    bool m_started;
};

END_NAMESPACE
