#include <server/timer.h>

#include <common/types.h>

#if defined(__APPLE__) && defined(__MACH__)
    #define TIMER_API_KQUEUE
#elif defined(__linux__)
    #define TIMER_API_LINUX
#else
    #error "Unknown OS"
#endif

#if defined(TIMER_API_KQUEUE)
    #include <timer/timer_kqueue.h>
#elif defined(TIMER_API_LINUX)
    #include <timer/timer_linux.h>
#endif

YAIC_NAMESPACE

Timer *Timer::factory(uint seconds)
{
#if defined(TIMER_API_KQUEUE)
    return new TimerApiKqueue(seconds);
#elif defined(TIMER_API_LINUX)
    return new TimerApiLinux(seconds);
#endif
}

Timer::Timer(uint seconds)
    : m_seconds(seconds)
{

}

Timer::~Timer()
{

}

END_NAMESPACE
