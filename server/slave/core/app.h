#pragma once

#include <common/types.h>
#include <server/event.h>
#include <server/tcp_manager.h>

#include <libconfig.h++>
#include <thread>

YAIC_NAMESPACE

struct Context;

/**
 * @brief Glówna klasa slave serwera - inicjalizuje i odpala pętle
 */
class SlaveServerApplication {
public:
    SlaveServerApplication();
    ~SlaveServerApplication();

    int run(const char *configPath, const char *configName);

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
