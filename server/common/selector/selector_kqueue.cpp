#include "selector/selector_kqueue.h"

#include <common/types.h>
#include <server/selector.h>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>

YAIC_NAMESPACE

SelectorApiKqueue::SelectorApiKqueue(int bufsize) :
    m_bufsize(bufsize), m_kqueuefd(kqueue())
{
}

SelectorApiKqueue::~SelectorApiKqueue()
{
    for (auto &kv : m_info) {
        delete kv.second;
    }

    close(m_kqueuefd);
}

void SelectorApiKqueue::add(int fd, int type, void *data, int eventType)
{
    // prevents leak in case of invalid calls
    auto it = m_info.find(fd);
    if (it != m_info.end())
        delete (*it).second;

    SelectorInfo *info = new SelectorInfo(fd, type, data, eventType);
    m_info[fd] = info;

    if (eventType & SelectorInfo::ReadEvent) {
        appendChange(fd, EVFILT_READ, EV_ADD, 0, info);
    }

    if (eventType & SelectorInfo::WriteEvent) {
        appendChange(fd, EVFILT_WRITE, EV_ADD, 0, info);
    }
}

void SelectorApiKqueue::modify(int fd, int eventType)
{
    auto it = m_info.find(fd);
    if (it == m_info.end())
        return;

    SelectorInfo *info = (*it).second;

    if (info->eventType() & SelectorInfo::ReadEvent && !(eventType & SelectorInfo::ReadEvent)) {
        appendChange(fd, EVFILT_READ, EV_DELETE, 0, info);
    } else if (!(info->eventType() & SelectorInfo::ReadEvent) && eventType & SelectorInfo::ReadEvent) {
        appendChange(fd, EVFILT_READ, EV_ADD, 0, info);
    }

    if (info->eventType() & SelectorInfo::WriteEvent && !(eventType & SelectorInfo::WriteEvent)) {
        appendChange(fd, EVFILT_WRITE, EV_DELETE, 0, info);
    } else if (!(info->eventType() & SelectorInfo::WriteEvent) && eventType & SelectorInfo::WriteEvent) {
        appendChange(fd, EVFILT_WRITE, EV_ADD, 0, info);
    }
}

void SelectorApiKqueue::remove(int fd)
{
    auto it = m_info.find(fd);
    if (it == m_info.end())
        return;

    modify(fd, 0);

    delete (*it).second;
    m_info.erase(it);
}

Selector::WaitRetval SelectorApiKqueue::wait(Vector<SelectorEvent> &events)
{
    events.clear();

    struct kevent kevents[m_bufsize];
    struct kevent *changelist = 0;

    if (m_changes.size() > 0)
        changelist = &(m_changes[0]);

    int nevents = kevent(m_kqueuefd, changelist, m_changes.size(), kevents, m_bufsize, 0);
    m_changes.clear();

    if (nevents == -1)
        return WaitRetval::Error;

    for (int i = 0; i < nevents; i++) {
        if (kevents[i].filter == EVFILT_READ) {
            events.emplace_back(static_cast<SelectorInfo*>(kevents[i].udata), SelectorInfo::ReadEvent);
        } else if (kevents[i].filter == EVFILT_WRITE) {
            events.emplace_back(static_cast<SelectorInfo*>(kevents[i].udata), SelectorInfo::WriteEvent);
        }

        if (kevents[i].flags & EV_EOF) {
            SelectorInfo *info = static_cast<SelectorInfo*>(kevents[i].udata);

            if (!info->closed()) {
                info->setClosed(true);
                events.emplace_back(info, SelectorInfo::CloseEvent);
            }
        }
    }

    return WaitRetval::Success;
}

void SelectorApiKqueue::appendChange(int fd, int filter, int flags, int fflags, SelectorInfo *info)
{
    m_changes.emplace_back();
    struct kevent &kev = m_changes.back();
    EV_SET(&kev, fd, filter, flags, fflags, 0, info);
}

END_NAMESPACE
