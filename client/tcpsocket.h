#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <QObject>
#include <QTcpSocket>
#include <QDateTime>

#include "common/packet.h"
#include "common/packets/master_user.h"
#include "common/packets/slave_user.h"
#include "common/types.h"
#include <netinet/in.h>

#include <QtCore>
#include <QDebug>

using namespace YAIC;

class tcpSocket: public QObject
{
    Q_OBJECT
public:
    tcpSocket(QObject *parent = 0);
    void getSlavesPort();
    bool connectWith(QString address, int port, Packet::Direction direction);
    void write(Packet *p);
    void disconnect();
    bool isConnected();

signals:
    void serversRead(MasterUserPackets::ServerList *p);
    void handshakeAck(SlaveUserPackets::HandshakeAck *p);
    void channels(SlaveUserPackets::Channels *p);
    void channelJoined(SlaveUserPackets::ChannelJoined *p);
    void channelParted(SlaveUserPackets::ChannelParted *p);
    void channelMessage(SlaveUserPackets::ChannelMessage *p);
    void channelUserJoined(SlaveUserPackets::ChannelUserJoined *p);
    void channelUserPatred(SlaveUserPackets::ChannelUserParted *p);
    void userDisconnected(SlaveUserPackets::UserDisconnected *p);
    void channelUserUpdated(SlaveUserPackets::ChannelUserUpdated *p);
    void userUpdated(SlaveUserPackets::UserUpdated *p);

public slots:
    void readyRead();
    void sendHeartbeat();
    void heartbeatTimeExpired();

private:
    int port;
    QString address;
    QTcpSocket *socket;
    PacketHeader packetHeader;
    qint64 readLength;
    qint64 readHeaderLength;
    Vector<char> bufferedData;
    bool isReadingPayload;
    QTimer *timerUserHeartbeat;
    QTimer *timerSlaveHeartbeat;
    Packet::Direction dir;
    QDateTime lastReceivedPacketTime;
    bool connected;
};

#endif // TCPSOCKET_H
