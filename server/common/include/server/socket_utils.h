#pragma once

#include <common/types.h>

YAIC_NAMESPACE

enum ConnectionProto {
    ConnectionProtoUnknown,
    ConnectionProtoIpv4,
    ConnectionProtoIpv6
};

class SocketUtils {
public:
    static int createListenSocket(const String &address, u16 port, ConnectionProto proto);
    static int createListenSocket(const String &full);
    static bool makeNonBlocking(int sock);
    static ConnectionProto readAddress(const String &full, u16 &port, String &address);
};

END_NAMESPACE
