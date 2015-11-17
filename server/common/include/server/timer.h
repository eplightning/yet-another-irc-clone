#pragma once

#include <common/types.h>

YAIC_NAMESPACE

class Timer {
public:
    static Timer *factory(uint seconds);

    Timer(uint seconds);
    virtual ~Timer();

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool wait() = 0;

protected:
    uint m_seconds;
};

END_NAMESPACE
