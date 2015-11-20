#include <common/types.h>
#include <common/packet.h>

#include <common/packets/master_user.h>

#include <iostream>

#include <functional>

#include <arpa/inet.h>
#include <fcntl.h>
#include <cstdio>
#include <unistd.h>

using namespace std;
using namespace YAIC;


using namespace std;

int main()
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    addr.sin_family = PF_INET;
    addr.sin_port = htons(1234);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    socklen_t len = sizeof(sockaddr_in);
    connect(sock, (sockaddr*) &addr, len);

    MasterUserPackets::RequestServers req;
    req.setFlags(MasterUserPackets::RequestServers::FlagIpv4Only);
    req.setMax(5);

    Vector<char> packet;
    req.encode(packet);

    int written = write(sock, &(packet[0]), packet.size());

    MasterUserPackets::ServerList srvlist;

    PacketHeader header;
    read(sock, (char*) &header, PACKET_HEADER_SIZE);
    header.payloadSize = ntohs(header.payloadSize);
    packet.clear();
    packet.resize(header.payloadSize);

    cout << "Payload: " << header.payloadSize << endl;

    read(sock, &(packet[0]), header.payloadSize);

    srvlist.decodePayload(packet);

    cout << "Servers: " << srvlist.servers().size() << endl;

    for (auto &x : srvlist.servers()) {
        std::cout << x.address  << " : " << x.port << std::endl;
    }

    getchar();

    return 0;
}

