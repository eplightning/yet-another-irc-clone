#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <QObject>
#include <QTcpSocket>


#include "common/packet.h"
#include "common/packets/master_user.h"
#include "common/types.h"
#include <netinet/in.h>

#include <QDebug>

using namespace YAIC;

class tcpSocket: public QObject
{
    Q_OBJECT
public:
    tcpSocket(QObject *parent = 0);
    void getSlavesPort();
    bool connectWith(QString address, int port);
    void write(Packet *p);
    void disconnect();

signals:
    void serversRead(MasterUserPackets::ServerList *p);

public slots:
    //void disconnected();
    void readyRead();

private:
    int port;
    QString address;
    QTcpSocket *socket;
    PacketHeader packetHeader;
    qint64 readLength;
    qint64 readHeaderLength;
    Vector<char> bufferedData;
    Vector<char> bufferedHeading;
    bool isReadingPayload;
};

#endif // TCPSOCKET_H
