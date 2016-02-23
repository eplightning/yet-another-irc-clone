#pragma once

#include <core/global.h>
#include <modules/user.h>

#include <common/types.h>
#include <server/event.h>
#include <server/tcp_manager.h>

#include <libconfig.h++>
#include <thread>

YAIC_NAMESPACE

struct Context;

/**
 * @brief Główna klasa aplikacji mastera serwera, inicjalizuje i odpala pętle
 */
class MasterServerApplication {
public:
    MasterServerApplication();
    ~MasterServerApplication();

    int run(const char *configPath);

protected:
    bool loadConfig();
    bool initModules();
    bool initTcpServer();
    bool initSysEvent();

protected:
    Context *m_context;
    libconfig::Config m_config;
    std::thread m_tcpThread;
    std::thread m_sysThread;
    int m_confWorkers;
};

END_NAMESPACE
