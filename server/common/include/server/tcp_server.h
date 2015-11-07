#pragma once

#include <common/types.h>
#include <common/packet.h>
#include <server/selector.h>

#include <functional>
#include <queue>
#include <mutex>

#include <arpa/inet.h>

YAIC_NAMESPACE

enum ConnectionProto {
    ConnectionProtoIpv4,
    ConnectionProtoIpv6
};

struct ClientReceiveBuffer {
    RawPacket header;
    uint received;
    char data[PACKET_MAX_SIZE];
};

struct ClientSendBuffer {
    uint sent;
    Vector<char> data;
};

struct ListenPool;

class Client {
public:
    enum class State {
        Connected = 1,
        Disconnecting = 2
    };

    Client(uint id, int fd, const sockaddr *addr, ListenPool *pool);
    ~Client();

    uint id() const { return m_id; }
    int socket() const { return m_socket; }
    ConnectionProto proto() const { return m_proto; }
    u16 port() const { return m_port; }
    ListenPool *pool() const { return m_pool; }
    const String &address() const { return m_address; }
    ClientReceiveBuffer &receiveBuffer() { return m_receiveBuffer; }

    ClientSendBuffer *sendBuffer();
    void attachSendBuffer(ClientSendBuffer *buffer);

    State state() const { return m_state; }
    void setState(State state) { m_state = state; }
    int writeError() const { return m_writeError; }
    void setWriteError(int error) { m_writeError = error; }
    int readError() const { return m_readError; }
    void setReadError(int error) { m_readError = error; }

protected:
    uint m_id;
    int m_socket;
    ConnectionProto m_proto;
    u16 m_port;
    String m_address;
    ListenPool *m_pool;
    State m_state;
    int m_writeError;
    int m_readError;

    ClientSendBuffer *m_sendBuffer;
    std::queue<ClientSendBuffer*> m_sendBufferQueue;
    std::mutex m_sendBufferMutex;
    ClientReceiveBuffer m_receiveBuffer;
};

struct ListenPoolSocket {
    ConnectionProto protocol;
    int socket;
    ListenPool *pool;
};

enum NewConnectionResponse {
    NewConnectionResponseReject,
    NewConnectionResponseAccept
};

// połączenie zostanie zamknięte po tym callbacku
typedef std::function<void(uint clientid)> DroppedConnectionDelegate;

// oznacza że już więcej pakietów nie zostanie odczytane (ale zostaną wysłane pozostałe w kolejce wysyłania)
typedef std::function<void(uint clientid)> LostConnectionDelegate;

// nowe połączenie (można odrzucić)
typedef std::function<NewConnectionResponse(SharedPtr<Client> client)> NewConnectionDelegate;

// nowy pakiet
typedef std::function<void(uint clientid, RawPacket header, char *data)> ReceiveDataDelegate;

struct ListenPool {
    Vector<ListenPoolSocket> sockets;
    DroppedConnectionDelegate droppedConnection;
    LostConnectionDelegate lostConnection;
    NewConnectionDelegate newConnection;
    ReceiveDataDelegate receive;
};

class TcpServer {
public:
    TcpServer(int rounds = 4);
    ~TcpServer();

    void createPool(String name, ListenPool *pool);
    void disconnect(SharedPtr<Client> client, bool force = false);
    void sendTo(SharedPtr<Client> client, ClientSendBuffer *buffer);
    void sendTo(SharedPtr<Client> client, const Packet *packet);
    void sendTo(const Vector<SharedPtr<Client>> &clients, const Packet *packet);

    void runLoop();
    void stopLoop();

protected:
    void newConnection(ListenPoolSocket *listen, Selector *select);
    bool pipeNotification(Selector *select, Vector<Client*> &eraseList);
    void readEvent(Client *client, SelectorEvent &event, Selector *select, Vector<Client*> &eraseList, int rounds = 0);
    void writeEvent(Client *client, Selector *select, Vector<Client*> &eraseList, int rounds = 0);

    void stopReading(Client *client, Selector *select, Vector<Client*> &eraseList, int error);
    void stopWriting(Client *client, Selector *select, Vector<Client*> &eraseList, int error);

    int m_pipe[2];
    Map<String, ListenPool*> m_pools;
    HashMap<uint, SharedPtr<Client>> m_clients;
    uint m_nextClientId;
    int m_rrRounds;
};

END_NAMESPACE
