#pragma once

#include <core/context.h>
#include <modules/user.h>

#include <common/types.h>
#include <server/event.h>
#include <server/tcp_server.h>

#include <libconfig.h++>
#include <thread>

YAIC_NAMESPACE

class MasterServerApplication {
public:
    MasterServerApplication();
    ~MasterServerApplication();

    int run(const char *configPath);

private:
    static void signalHandler(int signum);
    static EventQueue *signalEvq;

protected:
    bool loadConfig();
    bool initModules();
    bool initTcpServer();
    bool installSignalHandler();

protected:
    Context *m_context;
    UserModule *m_userModule;
    libconfig::Config m_config;
    std::thread m_tcpThread;
};

END_NAMESPACE
