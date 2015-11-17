#pragma once

#include <core/context.h>

#include <common/types.h>

#include <libconfig.h++>

YAIC_NAMESPACE

enum SlaveAuthMode {
    None = 0,
    PlainText = 1
};

struct SlaveModuleConfig {
    Vector<String> listen;
    SlaveAuthMode authMode;
    String plainTextPassword;
};

class SlaveModule {
public:
    SlaveModule(Context *context);

    ~SlaveModule();

    void loadFromLibconfig(const libconfig::Setting &section);
    void init();

protected:
    Context *m_context;
    SlaveModuleConfig m_config;
};

END_NAMESPACE
