#pragma once

#include <core/global.h>

#include <common/types.h>
#include <server/dispatcher.h>

#include <libconfig.h++>

YAIC_NAMESPACE

class SlaveServer {
public:
    SlaveServer(SharedPtr<Client> &client, u32 id, u16 port, uint capacity);

    SharedPtr<Client> &client();
    u32 id() const;
    u16 port() const;
    uint capacity() const;

    // NOTE: not thread-safe, use only while holding m_slaves lock
    uint connections() const;
    uint load() const;
    bool active() const;

    void setConnections(uint connections);
    void setActive(bool active);

protected:
    SharedPtr<Client> m_client;
    u32 m_id;
    u16 m_port;
    uint m_load;
    uint m_capacity;
    bool m_active;
};

enum SlaveAuthMode {
    SAMNone = 0,
    SAMPlainText = 1
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

    void loadConfig(const libconfig::Setting &section);
    bool init();
    void dispatchPacket(EventPacket *ev);
    void dispatchTimer(EventTimer *ev);
    void dispatchSimple(EventSimple *ev);

    HashMap<uint, SharedPtr<SlaveServer>> &slaves();
    std::mutex &slavesMutex();

protected:
    Context *m_context;
    SlaveModuleConfig m_config;
    PacketDispatcher m_packetDispatcher;
    TimerDispatcher m_timerDispatcher;

    std::mutex m_slavesMutex;
    HashMap<uint, SharedPtr<SlaveServer>> m_slaves;
};

END_NAMESPACE
