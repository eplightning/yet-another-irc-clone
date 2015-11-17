#include <server/socket_utils.h>

#include <common/types.h>

#include <cstring>

#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

YAIC_NAMESPACE

int SocketUtils::createListenSocket(const String &address, u16 port, ConnectionProto proto)
{
    int sock = ::socket((proto == ConnectionProtoIpv4) ? AF_INET : AF_INET6, SOCK_STREAM, 0);

    if (sock == -1)
        return -1;

    sockaddr_storage addr;
    socklen_t addrLen;
    void *inaddr;

    memset(&addr, 0, sizeof(sockaddr_storage));

    if (proto == ConnectionProtoIpv4) {
        sockaddr_in *addr4 = reinterpret_cast<sockaddr_in*>(&addr);
        addrLen = sizeof(sockaddr_in);

        addr4->sin_family = AF_INET;
        addr4->sin_port = htons(port);
        inaddr = &addr4->sin_addr;
    } else {
        sockaddr_in6 *addr6 = reinterpret_cast<sockaddr_in6*>(&addr);
        addrLen = sizeof(sockaddr_in6);

        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(port);
        inaddr = &addr6->sin6_addr;
    }

    if (inet_pton((proto == ConnectionProtoIpv4) ? AF_INET : AF_INET6, address.c_str(), inaddr) != 1)
        return -4;

    if (::bind(sock, reinterpret_cast<sockaddr*>(&addr), addrLen) == -1)
        return -2;

    if (::listen(sock, SOMAXCONN) == -1)
        return -3;

    return sock;
}

int SocketUtils::createListenSocket(const String &full, ConnectionProto &proto)
{
    String address;
    u16 port;
    proto = readAddress(full, port, address);

    if (proto == ConnectionProtoUnknown)
        return -5;

    return createListenSocket(address, port, proto);
}

bool SocketUtils::makeNonBlocking(int sock)
{
    int flags = fcntl(sock, F_GETFL, 0);

    if (flags == -1)
        return false;

    return fcntl(sock, F_SETFL, flags | O_NONBLOCK) != -1;
}

ConnectionProto SocketUtils::readAddress(const String &full, u16 &port, String &address)
{
    if (full.empty())
        return ConnectionProtoUnknown;

    size_t sep = full.find_last_of(':');

    if (sep == String::npos || sep == full.size() - 1 || sep == 0)
        return ConnectionProtoUnknown;

    try {
        port = static_cast<u16>(std::stoul(full.substr(sep + 1)));
    } catch (std::exception &e) {
        return ConnectionProtoUnknown;
    }

    // IPv6 ([ipv6address]:port)
    if (full.front() == '[') {
        if (sep <= 2 || full.at(sep - 1) != ']')
            return ConnectionProtoUnknown;

        address.assign(full, 1, sep - 2);

        return ConnectionProtoIpv6;
    // IPv4 (ipv4address:port)
    } else {
        address.assign(full, 0, sep);

        return ConnectionProtoIpv4;
    }
}

END_NAMESPACE
