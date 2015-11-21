#include <syseventloop/syseventloop_linux.h>

#include <common/types.h>
#include <server/syseventloop.h>

#include <sys/types.h>
#include <sys/timerfd.h>
#include <sys/eventfd.h>
#include <sys/signalfd.h>
#include <sys/epoll.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>

YAIC_NAMESPACE

SysEventLoopApiLinux::SysEventLoopApiLinux(EventQueue *evq) :
    SysEventLoop(evq), m_epollfd(epoll_create1(0)), m_eventfd(eventfd(0, EFD_NONBLOCK))
{

}

SysEventLoopApiLinux::~SysEventLoopApiLinux()
{
    for (auto x : m_timerfd)
        close(x);

    close(m_epollfd);
    close(m_eventfd);
}

int SysEventLoopApiLinux::addTimer(uint seconds)
{
    int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    m_timerfd.push_back(timerfd);

    struct itimerspec itime {};
    itime.it_value.tv_sec = seconds;
    itime.it_interval.tv_sec = seconds;

    timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &itime, NULL);

    struct epoll_event event {};
    event.data.fd = timerfd;
    event.events = EPOLLIN;

    if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, timerfd, &event) != -1)
        return timerfd;

    return -1;
}

void SysEventLoopApiLinux::removeTimer(int fd)
{
    auto it = std::find(m_timerfd.begin(), m_timerfd.end(), fd);

    if (it != m_timerfd.end())
    {
        close(fd);
        m_timerfd.erase(it);
    }
}

bool SysEventLoopApiLinux::runLoop()
{
    // eventfd
    struct epoll_event event1 {};
    event1.data.fd = m_eventfd;
    event1.events = EPOLLIN;
    if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_eventfd, &event1) == -1)
        return false;

    // signalfd
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGHUP);

    int sfd = signalfd(-1, &mask, 0);
    struct epoll_event event2 {};
    event2.data.fd = sfd;
    event2.events = EPOLLIN;
    if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, sfd, &event2) == -1)
        return false;

    struct epoll_event incoming[16];

    int nevents;
    bool stopLoop = false;
    while (!stopLoop && (nevents = epoll_wait(m_epollfd, incoming, 16, -1)) > -1) {
        for (int i = 0; i < nevents; i++) {
            if (incoming[i].data.fd == m_eventfd) {
                stopLoop = true;
                break;
            } else if (incoming[i].data.fd == sfd) {
                EventSimple::EventId type;

                struct signalfd_siginfo fdsi;
                if (read(sfd, &fdsi, sizeof(struct signalfd_siginfo)) == -1)
                    continue;

                switch (fdsi.ssi_signo) {
                case SIGTERM:
                    type = EventSimple::EventId::SignalTerminate;
                    break;

                case SIGQUIT:
                    type = EventSimple::EventId::SignalQuit;
                    break;

                case SIGINT:
                    type = EventSimple::EventId::SignalInterrupt;
                    break;

                default:
                    type = EventSimple::EventId::SignalHangUp;
                }

                m_evq->append(new EventSimple(type));
            } else {
                uint64_t expirations;
                read(incoming[i].data.fd, &expirations, sizeof(expirations));

                m_evq->append(new EventTimer(incoming[i].data.fd));
            }
        }
    }

    return true;
}

void SysEventLoopApiLinux::stopLoop()
{
    eventfd_write(m_eventfd, 1);
}

END_NAMESPACE
