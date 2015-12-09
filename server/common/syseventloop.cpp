#include <server/syseventloop.h>

#include <common/types.h>
#include <server/event.h>

#if defined(__APPLE__) && defined(__MACH__)
    #define SYSEVENT_API_KQUEUE
#elif defined(__linux__)
    #define SYSEVENT_API_LINUX
#else
    #error "Unknown OS"
#endif

#if defined(SYSEVENT_API_KQUEUE)
    #include <syseventloop/syseventloop_kqueue.h>
#elif defined(SYSEVENT_API_LINUX)
    #include <syseventloop/syseventloop_linux.h>
#endif

YAIC_NAMESPACE

SysEventLoop *SysEventLoop::factory(EventQueue &evq)
{
#if defined(SYSEVENT_API_KQUEUE)
    return new SysEventLoopApiKqueue(&evq);
#elif defined(SYSEVENT_API_LINUX)
    return new SysEventLoopApiLinux(&evq);
#endif
}

SysEventLoop::SysEventLoop(EventQueue *evq) :
    m_evq(evq)
{

}

SysEventLoop::~SysEventLoop()
{

}

END_NAMESPACE
