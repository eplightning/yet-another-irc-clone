#pragma once

#include <modules/slave.h>
#include <modules/user.h>

#include <common/types.h>
#include <server/event.h>
#include <server/tcp_manager.h>
#include <server/logger.h>
#include <server/syseventloop.h>
#include <server/dispatcher.h>

YAIC_NAMESPACE

/**
 * @brief Kontekst aplikacji
 */
struct Context {
    // main objects
    UniquePtr<EventQueue> eventQueue;
    UniquePtr<TcpManager> tcp;
    UniquePtr<SysEventLoop> sysLoop;
    UniquePtr<Logger> log;
    UniquePtr<PacketDispatcher> dispatcher;

    // modules
    UniquePtr<UserModule> user;
    UniquePtr<SlaveModule> slave;

    // configuration
    String configPath;
};

END_NAMESPACE
