#pragma once

#include <common/types.h>
#include <server/syseventloop.h>
#include <vector>

#include <mutex>

YAIC_NAMESPACE

/**
 * @brief Implementacja wydarzeń systemowych na Linux'ie
 */
class SysEventLoopApiLinux : public SysEventLoop {
public:
    explicit SysEventLoopApiLinux(EventQueue *evq);
    ~SysEventLoopApiLinux();

    int addTimer(uint seconds);
    void removeTimer(int id);
    bool runLoop();
    void stopLoop();

protected:
    int m_epollfd;
    Vector<int> m_timerfd;
    std::mutex m_timerfdMutex;
    int m_eventfd;
};

END_NAMESPACE
