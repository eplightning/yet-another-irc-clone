#include "selector/selector_epoll.h"

#include <common/types.h>
#include <server/selector.h>

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <sys/epoll.h>

YAIC_NAMESPACE

SelectorApiEpoll::SelectorApiEpoll(int bufsize) :
    m_bufsize(bufsize), m_epollfd(epoll_create1(0))
{
}

SelectorApiEpoll::~SelectorApiEpoll()
{
    for (auto &kv : m_info) {
        delete kv.second;
    }

    close(m_epollfd);
}

void SelectorApiEpoll::add(int fd, int type, void *data, int eventType)
{
    // prevents leak in case of invalid calls
    auto it = m_info.find(fd);
    if (it != m_info.end())
        delete (*it).second;

    SelectorInfo *info = new SelectorInfo(fd, type, data, eventType);

    struct epoll_event event;
    event.data.fd = fd;

    if (eventType & SelectorInfo::ReadEvent)
        event.events |= EPOLLIN;
    if (eventType & SelectorInfo::WriteEvent)
        event.events |= EPOLLOUT;

    epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &event);

    m_info[fd] = info;
}

void SelectorApiEpoll::close(int fd)
{
    auto it = m_info.find(fd);
    if (it == m_info.end())
        return;

    delete (*it).second;
    m_info.erase(it);
}

void SelectorApiEpoll::modify(int fd, int eventType)
{
    auto it = m_info.find(fd);
    if (it == m_info.end())
        return;

    SelectorInfo *info = (*it).second;
    info->setEventType(eventType);

    struct epoll_event event;
    event.data.fd = fd;

    if (eventType & SelectorInfo::ReadEvent)
        event.events |= EPOLLIN;
    if (eventType & SelectorInfo::WriteEvent)
        event.events |= EPOLLOUT;

    epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &event);
}

void SelectorApiEpoll::remove(int fd)
{
    auto it = m_info.find(fd);
    if (it == m_info.end())
        return;

    struct epoll_event event;
    event.data.fd = fd;
    event.events = 0;

    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, &event);

    delete (*it).second;
    m_info.erase(it);
}

Selector::WaitRetval SelectorApiEpoll::wait(Vector<SelectorEvent> &events)
{
     events.clear();
    // zmiany oraz eventy
     struct epoll_event returnedEvents[m_bufsize];
    int nevents = epoll_wait(m_epollfd, returnedEvents, m_bufsize, -1);
    if (nevents == -1)
        return WaitRetval::Error;

    for (int i = 0; i < nevents; i++)
    {
        int fd = returnedEvents[i].data.fd;
        auto it = m_info.find(fd);
        if (it == m_info.end())
            continue;

        SelectorInfo *info = (*it).second;

        if (returnedEvents[i].events & EPOLLOUT)
        {
            events.emplace_back(info, SelectorInfo::WriteEvent);
        }
        if ((returnedEvents[i].events & EPOLLIN) || (returnedEvents[i].events & EPOLLERR) || (returnedEvents[i].events & EPOLLHUP))
        {
            events.emplace_back(info, SelectorInfo::ReadEvent);
        }

        if ((returnedEvents[i].events & EPOLLERR) || (returnedEvents[i].events & EPOLLHUP))
        {
            info->setClosed(true);
        }
    }

    return WaitRetval::Success;
}

END_NAMESPACE

