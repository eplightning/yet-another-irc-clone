#pragma once

#include <common/types.h>
#include <common/packet.h>

#include <arpa/inet.h>

YAIC_NAMESPACE

enum ConnectionProtocol {
    CPUnknown,
    CPIpv4,
    CPIpv6
};

enum TcpClientState {
    TCSConnected = 0,
    TCSDisconnecting,
    TCSWritingClosed,
    TCSDisconnected
};

/**
 * @brief Pakiet gotowy do wysłania
 */
struct TcpSendBuffer {
    uint sent;
    Vector<char> data;
};

/**
 * @brief Bufor gdzie trzymamy obecnie pobierany pakiet od klienta
 */
class TcpReceiveBuffer {
public:
    TcpReceiveBuffer();

    PacketHeader header;
    uint received;
    Vector<char> data;
};

/**
 * @brief Pare przydatnych funkcji
 */
class SocketUtils {
public:
    static int createListenSocket(const String &address, u16 port, ConnectionProtocol proto);
    static int createListenSocket(const String &full, ConnectionProtocol &proto);
    static bool fillSockaddr(sockaddr_storage *sockaddr, const String &address, u16 port, ConnectionProtocol proto);
    static bool makeNonBlocking(int sock);
    static ConnectionProtocol readAddress(const String &full, u16 &port, String &address);
    static ConnectionProtocol getProtoFromIp(const String &ip);
};

END_NAMESPACE
