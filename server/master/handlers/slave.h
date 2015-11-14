#pragma once

#include "core/context.h"

#include <common/types.h>

#include <libconfig.h++>

YAIC_NAMESPACE

enum SlaveAuthMode {
    None = 0,
    PlainText = 1
};

struct SlaveHandlerConfig {
    Vector<String> listen;
    SlaveAuthMode authMode;
    String plainTextPassword;
};

class SlaveHandler {
public:
    SlaveHandler(Context *context);

    ~SlaveHandler();

    void loadFromLibconfig(const libconfig::Setting &section);
    void init();

protected:
    Context *m_context;
    SlaveHandlerConfig m_config;
};

END_NAMESPACE
