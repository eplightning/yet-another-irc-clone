#include <common/types.h>
#include <common/packet.h>

#include <server/selector.h>
#include <server/tcp_server.h>

#include <iostream>

#include <functional>

#include <arpa/inet.h>
#include <fcntl.h>

using namespace std;
using namespace YAIC;

std::map<uint, SharedPtr<Client>> g_clients;
TcpServer serv;

void drop(uint clientid)
{
    g_clients.erase(clientid);
    std::cout << "Drop: " << clientid << endl;
}

void lost(uint clientid)
{
    std::cout << "Lost: " << clientid << endl;
}

bool incoming(SharedPtr<Client> client)
{
    g_clients[client->id()] = client;
    std::cout << "Connect: " << client->address() << " " << client->id() << endl;
    return true;
}

void receiveData(uint clientid, PacketHeader header, char *data)
{
    std::cout << "Receive: " << clientid << endl;
    serv.disconnect(g_clients[clientid]);
}

int main()
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
    sockaddr_in addr;
    addr.sin_family = PF_INET;
    addr.sin_port = htons(1234);
    inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr);
    if (::bind(sock, (sockaddr*) &addr, sizeof(sockaddr_in)))
        return 1;

    listen(sock, 10);

    ListenPool *pool = new ListenPool;
    pool->droppedConnection = std::bind(&drop, std::placeholders::_1);
    pool->lostConnection = std::bind(&lost, std::placeholders::_1);
    pool->newConnection = std::bind(&incoming, std::placeholders::_1);
    pool->receive = std::bind(&receiveData, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    pool->sockets.resize(1);
    pool->sockets[0].protocol = ConnectionProtoIpv4;
    pool->sockets[0].socket = sock;
    pool->sockets[0].pool = pool;

    serv.createPool("main", pool);

    serv.runLoop();

    return 0;
}
