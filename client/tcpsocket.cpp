#include "tcpsocket.h"

using namespace YAIC;

tcpSocket::tcpSocket(QObject *parent) :
    QObject(parent)
{
    isReadingPayload = false;
    readHeaderLength = 0;
    readLength = 0;
}

bool tcpSocket::connectWith(QString address, int port)
{
    this->port = port;
    this->address = address;
    socket = new QTcpSocket(this);

    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));

    socket->connectToHost(address, static_cast<quint16>(port));

    if (!socket->waitForConnected(5 * 1000))
    {
        return false;
    }

    return true;
}

void tcpSocket::disconnect()
{
    socket->close();
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

                if (Packet::checkDirection(packetHeader.type, Packet::Direction::MasterToUser))
                {
                    Packet *a = Packet::factory(packetHeader, bufferedData);

                    if (a != nullptr)
                    {
                        switch (a->packetType())
                        {
                            case Packet::Type::ServerList:
                                emit serversRead(static_cast<MasterUserPackets::ServerList*>(a));
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
