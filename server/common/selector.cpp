#include <server/selector.h>

#include <common/types.h>

#if defined(__APPLE__) && defined(__MACH__)
    #define SELECTOR_API_KQUEUE
#elif defined(__linux__)
    #define SELECTOR_API_EPOLL
#else
    #error "Unknown OS"
#endif

#if defined(SELECTOR_API_KQUEUE)
    #include "selector/selector_kqueue.h"
#elif defined(SELECTOR_API_EPOLL)
    #include "selector/selector_epoll.h"
#endif

YAIC_NAMESPACE

const int SelectorInfo::ReadEvent = 1 << 0;
const int SelectorInfo::WriteEvent = 1 << 1;
const int SelectorInfo::CloseEvent = 1 << 2;

Selector *Selector::factory()
{
#if defined(SELECTOR_API_KQUEUE)
    return new SelectorApiKqueue(32);
#elif defined(SELECTOR_API_EPOLL)
    return new SelectorApiEpoll(32);
#endif
}

SelectorEvent::SelectorEvent(const SelectorInfo *info, int type) :
    m_info(info), m_type(type)
{

}

SelectorInfo::SelectorInfo(int fd, int type, void *data, int evtype) :
    m_fd(fd), m_type(type), m_data(data), m_eventType(evtype), m_closed(false)
{

}

END_NAMESPACE
