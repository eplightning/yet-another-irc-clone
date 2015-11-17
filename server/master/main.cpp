#include "core/app.h"

int main(int argc, char *argv[])
{
    YAIC::MasterServerApplication app;

    const char *configPath = "/etc/yaic";
    if (argc >= 2)
        configPath = argv[1];

    int ret = app.run(configPath);

    return ret;
    /*ConnectionProto proto;
    int sock = SocketUtils::createListenSocket("0.0.0.0:1234", proto);
    SocketUtils::makeNonBlocking(sock);

    if (sock < 0)
        return 1;

    ListenPool *pool = new ListenPool;
    pool->droppedConnection = std::bind(&drop, std::placeholders::_1);
    pool->lostConnection = std::bind(&lost, std::placeholders::_1);
    pool->newConnection = std::bind(&incoming, std::placeholders::_1);
    pool->receive = std::bind(&receiveData, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    pool->sockets.resize(1);
    pool->sockets[0].protocol = proto;
    pool->sockets[0].socket = sock;
    pool->sockets[0].pool = pool;

    serv.createPool("main", pool);

    serv.runLoop();*/
}
