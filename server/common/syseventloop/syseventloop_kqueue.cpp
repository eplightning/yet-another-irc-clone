#include <syseventloop/syseventloop_kqueue.h>

#include <common/types.h>
#include <server/syseventloop.h>
#include <server/misc_utils.h>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <unistd.h>

YAIC_NAMESPACE

SysEventLoopApiKqueue::SysEventLoopApiKqueue(EventQueue *evq) :
    SysEventLoop(evq), m_kqueuefd(kqueue()), m_timerid(0)
{

}

SysEventLoopApiKqueue::~SysEventLoopApiKqueue()
{
    close(m_kqueuefd);
}

int SysEventLoopApiKqueue::addTimer(uint seconds)
{
    // NOT THREAD SAFE
    // naprawić jak będzie potrzeba wywoływania tego spoza main loopa (nigdy?)
    m_timerid = m_timerid + 1;

    struct kevent event;

    EV_SET(&event, m_timerid, EVFILT_TIMER, EV_ADD | EV_ENABLE, NOTE_SECONDS, seconds, 0);

    if (kevent(m_kqueuefd, &event, 1, NULL, 0, NULL) != -1)
        return m_timerid;

    return -1;
}

void SysEventLoopApiKqueue::removeTimer(int id)
{
    struct kevent event;

    EV_SET(&event, id, EVFILT_TIMER, EV_DELETE, 0, 0, 0);

    kevent(m_kqueuefd, &event, 1, NULL, 0, NULL);
}

bool SysEventLoopApiKqueue::runLoop()
{
    // ignorujemy sygnały które nas zabijają
    signal(SIGTERM, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    // upewniamy się że mamy odblokowane sygnały
    MiscUtils::unblockSignals();

    // rejestrujemy wszystko z kqueue
    struct kevent events[6];
    EV_SET(&events[0], 1, EVFILT_USER, EV_ADD | EV_ENABLE | EV_CLEAR, NOTE_FFNOP, 0, 0);
    EV_SET(&events[1], SIGTERM, EVFILT_SIGNAL, EV_ADD, 0, 0, 0);
    EV_SET(&events[2], SIGINT, EVFILT_SIGNAL, EV_ADD, 0, 0, 0);
    EV_SET(&events[3], SIGQUIT, EVFILT_SIGNAL, EV_ADD, 0, 0, 0);
    EV_SET(&events[4], SIGHUP, EVFILT_SIGNAL, EV_ADD, 0, 0, 0);
    EV_SET(&events[5], SIGPIPE, EVFILT_SIGNAL, EV_ADD, 0, 0, 0);

    if (kevent(m_kqueuefd, events, 6, NULL, 0, NULL) == -1)
        return false;

    struct kevent incoming[16];

    int nevents;
    bool stopLoop = false;

    while (!stopLoop && (nevents = kevent(m_kqueuefd, NULL, 0, incoming, 16, NULL)) > -1) {
        for (int i = 0; i < nevents; i++) {
            if (incoming[i].filter == EVFILT_USER) {
                stopLoop = true;
                break;
            } else if (incoming[i].filter == EVFILT_SIGNAL) {
                EventSimple::EventId type;

                switch (incoming[i].ident) {
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
            } else if (incoming[i].filter == EVFILT_TIMER) {
                m_evq->append(new EventTimer(incoming[i].ident));
            }
        }
    }

    return true;
}

void SysEventLoopApiKqueue::stopLoop()
{
    struct kevent event;

    EV_SET(&event, 1, EVFILT_USER, 0, NOTE_FFNOP | NOTE_TRIGGER, 0, 0);

    kevent(m_kqueuefd, &event, 1, NULL, 0, NULL);
}

END_NAMESPACE
