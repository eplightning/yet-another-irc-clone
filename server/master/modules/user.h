#pragma once

#include <core/context.h>

#include <common/types.h>

#include <libconfig.h++>

YAIC_NAMESPACE

struct UserModuleConfig {
    Vector<String> listen;
};

class UserModule {
public:
    UserModule(Context *context);
    ~UserModule();

    void loadFromLibconfig(const libconfig::Setting &section);
    void init();

protected:
    void initTcp();

    void tcpDropped(uint clientid);
    void tcpLost(uint clientid);
    bool tcpNew(SharedPtr<Client> client);
    void tcpReceive(uint clientid, PacketHeader header, const Vector<char> &data);

    PacketDispatcher::Result serversRequest(uint clientid, Packet *packet);

protected:
    Context *m_context;
    UserModuleConfig m_config;
};

END_NAMESPACE
