#pragma once

#include "core/context.h"
#include "handlers/client.h"

#include <common/types.h>
#include <server/event.h>
#include <server/tcp_server.h>

#include <thread>

YAIC_NAMESPACE

class MasterServerApplication {
public:
    MasterServerApplication();

    ~MasterServerApplication();

    int run(const char *configPath);

protected:
    bool loadConfig();
    bool initHandlers();
    bool initTcpServer();

protected:
    Context *m_context;
    SharedPtr<ClientHandler> m_clientHandler;
    libconfig::Config m_config;
    std::thread m_tcpThread;
};

END_NAMESPACE
