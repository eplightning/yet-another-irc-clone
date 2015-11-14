#pragma once

#include "core/context.h"

#include <common/types.h>

#include <libconfig.h++>

YAIC_NAMESPACE

struct ClientHandlerConfig {
    Vector<String> listen;
};

class ClientHandler {
public:
    ClientHandler(Context *context);

    ~ClientHandler();

    void loadFromLibconfig(const libconfig::Setting &section);
    void init();

protected:
    Context *m_context;
    ClientHandlerConfig m_config;
};

END_NAMESPACE
