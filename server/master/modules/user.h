#pragma once

#include <core/context.h>

#include <common/types.h>
#include <server/dispatcher.h>

#include <libconfig.h++>
#include <thread>

YAIC_NAMESPACE

struct UserModuleConfig {
    Vector<String> listen;
    uint timeout;
};

class UserModule {
public:
    UserModule(Context *context);
    ~UserModule();

    void loadFromLibconfig(const libconfig::Setting &section);
    void init();

protected:
    void initTcp();
    void initTimeout();

    TimerDispatcher::Result timeoutHandler(int timer);

    void tcpDropped(uint clientid);
    void tcpLost(uint clientid);
    bool tcpNew(SharedPtr<Client> client);
    void tcpReceive(uint clientid, PacketHeader header, const Vector<char> &data);

    PacketDispatcher::Result serversRequest(uint clientid, Packet *packet);

protected:
    Context *m_context;
    UserModuleConfig m_config;

    int m_timeoutTimer;
};

END_NAMESPACE
