#include "tcpsocket.h"

using namespace YAIC;

tcpSocket::tcpSocket(QObject *parent) :
    QObject(parent)
{
    isReadingPayload = false;
    readHeaderLength = 0;
    readLength = 0;
    connected = false;
}

bool tcpSocket::connectWith(QString address, int port, Packet::Direction direction)
{
    this->dir = direction;
    this->port = port;
    this->address = address;
    socket = new QTcpSocket(this);

    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));

    socket->connectToHost(address, static_cast<quint16>(port));

    if (!socket->waitForConnected(5 * 1000))
    {
        return false;
    }

    connected = true;
    if (direction==Packet::Direction::SlaveToUser){
        timerUserHeartbeat = new QTimer(this);
        connect(timerUserHeartbeat, SIGNAL(timeout()), this, SLOT(sendHeartbeat()));
        timerUserHeartbeat->start(1000);

        timerSlaveHeartbeat = new QTimer(this);
        connect(timerSlaveHeartbeat, SIGNAL(timeout()), this, SLOT(heartbeatTimeExpired()));
        timerSlaveHeartbeat->start(10 * 1000);

        lastReceivedPacketTime = QDateTime::currentDateTime();
    }
    return true;
}

void tcpSocket::disconnect()
{
    socket->close();
    timerUserHeartbeat->stop();
    timerSlaveHeartbeat->stop();
    delete socket;
    delete timerUserHeartbeat;
    delete timerSlaveHeartbeat;
}

void tcpSocket::write(Packet *p)
{
    std::vector<char> bytes;
    p->encode(bytes);

    socket->write(&bytes[0], static_cast<qint64>(bytes.size()));
}

void tcpSocket::readyRead()
{
    while (socket->bytesAvailable())
    {
        if (!isReadingPayload)
        {
           QByteArray data = socket->read(Packet::HeaderSize - readHeaderLength);
           qint64 read = data.size();

           const char* dataPacked = reinterpret_cast<const char*>(data.constData());
           bufferedData.insert(bufferedData.end(), dataPacked, dataPacked + read);

           readHeaderLength += read;

           if (readHeaderLength == Packet::HeaderSize)
           {
               const u16 *ptrType = reinterpret_cast<const u16*>(&bufferedData[0]);
               const u32 *ptrLength = reinterpret_cast<const u32*>(&bufferedData[0] + sizeof(u16));

               u16 packetType = ntohs(*ptrType);
               u32 packetLength = ntohl(*ptrLength);

               packetHeader.payloadSize = packetLength;
               packetHeader.type = packetType;

               readHeaderLength = 0;
               isReadingPayload = true;
           }
        }

        if (isReadingPayload)
        {
            if (packetHeader.payloadSize > 0)
            {
                QByteArray data = socket->read(packetHeader.payloadSize - readLength);
                qint64 read = data.size();
                readLength += read;

                const char* dataPacked = reinterpret_cast<const char*>(data.constData());
                bufferedData.insert(bufferedData.end(), dataPacked, dataPacked + read);
            }

            if (packetHeader.payloadSize == readLength)
            {
                isReadingPayload = false;
                readLength = 0;

                if (Packet::checkDirection(packetHeader.type, dir))
                {
                    Packet *a = Packet::factory(packetHeader, bufferedData);

                    if (a != nullptr)
                    {
                        lastReceivedPacketTime = QDateTime::currentDateTime();
                        switch (a->packetType())
                        {
                            case Packet::Type::ServerList:
                                emit serversRead(static_cast<MasterUserPackets::ServerList*>(a));
                                break;
                            case Packet::Type::SlaveUserHeartbeat:
                                break;
                            case Packet::Type::HandshakeAck:
                                emit handshakeAck(static_cast<SlaveUserPackets::HandshakeAck*>(a));
                                break;
                            case Packet::Type::Channels:
                                emit channels(static_cast<SlaveUserPackets::Channels*>(a));
                                break;
                            case Packet::Type::ChannelJoined:
                                emit channelJoined(static_cast<SlaveUserPackets::ChannelJoined*>(a));
                                break;
                            default:
                                //doSth
                                break;
                        }
                        delete a;
                    }
                }
                bufferedData.clear();
            }
        }
    }
}

void tcpSocket::sendHeartbeat()
{
    SlaveUserPackets::UserHeartbeat *a = new SlaveUserPackets::UserHeartbeat();
    write(a);
    delete a;
}

void tcpSocket::heartbeatTimeExpired()
{
    if ((lastReceivedPacketTime.toTime_t() - QDateTime::currentDateTime().toTime_t()) > (10 * 1000))
    {
        disconnect();
        qDebug() << "Rozłączyło mnie";
    }
}

bool tcpSocket::isConnected()
{
    return connected;
}
