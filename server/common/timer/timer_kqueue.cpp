#include <timer/timer_kqueue.h>

#include <common/types.h>
#include <server/timer.h>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>

YAIC_NAMESPACE

TimerApiKqueue::TimerApiKqueue(uint seconds)
    : Timer(seconds), m_kqueuefd(kqueue()), m_started(false)
{

}

TimerApiKqueue::~TimerApiKqueue()
{
    close(m_kqueuefd);
}

void TimerApiKqueue::start()
{
    struct kevent events[2];

    EV_SET(&events[0], 1, EVFILT_TIMER, EV_ADD | EV_ENABLE, NOTE_SECONDS, m_seconds, 0);
    EV_SET(&events[1], 2, EVFILT_USER, EV_ADD | EV_ENABLE | EV_CLEAR, NOTE_FFNOP, 0, 0);

    kevent(m_kqueuefd, events, 2, NULL, 0, 0);

    m_started = true;
}

void TimerApiKqueue::stop()
{
    struct kevent events;

    EV_SET(&events, 2, EVFILT_USER, 0, NOTE_FFNOP | NOTE_TRIGGER, 0, 0);

    kevent(m_kqueuefd, &events, 1, NULL, 0, 0);
}

bool TimerApiKqueue::wait()
{
    if (!m_started)
        return false;

    struct kevent events[2];

    int ev = kevent(m_kqueuefd, NULL, 0, events, 2, 0);

    if (ev == -1) {
        m_started = false;
        return false;
    }

    bool result = true;

    for (int i = 0; i < ev; i++) {
        if (events[i].ident == 2) {
            struct kevent events2[2];

            EV_SET(&events2[0], 1, EVFILT_TIMER, EV_DELETE, 0, 0, 0);
            EV_SET(&events2[1], 2, EVFILT_USER, EV_DELETE, 0, 0, 0);

            kevent(m_kqueuefd, events2, 2, NULL, 0, 0);

            result = false;
            m_started = false;
        }
    }

    return result;
}

END_NAMESPACE
