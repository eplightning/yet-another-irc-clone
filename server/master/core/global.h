#pragma once

#include <common/types.h>
#include <server/event.h>
#include <server/tcp_manager.h>
#include <server/logger.h>
#include <server/syseventloop.h>

YAIC_NAMESPACE

// nie chce robić .cpp więc daje #define zamiast const static int
#define MASTER_APP_SOURCE_USER  1
#define MASTER_APP_SOURCE_SLAVE 2

class UserModule;
class SlaveModule;

struct Context {
    // main objects
    UniquePtr<EventQueue> eventQueue;
    UniquePtr<TcpManager> tcp;
    UniquePtr<SysEventLoop> sysLoop;
    UniquePtr<Logger> log;

    // modules
    UniquePtr<UserModule> user;
    UniquePtr<SlaveModule> slave;

    // configuration
    String configPath;
};

END_NAMESPACE
