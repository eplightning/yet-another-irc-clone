#ifndef CONVERSATION_H
#define CONVERSATION_H

#include <QObject>
#include <QStandardItem>

#include "common/types.h"
#include "common/packet.h"
#include "common/packets/slave_user.h"

using namespace YAIC;

class Conversation
{
public:
    Conversation(QString prefix, QString name, QStandardItemModel *channelListModel);
    QString getName();
    QString getFullName();
    QString getText();
    QString getPrefix();
    void setRead();
    void setUnread();
    void addMessage(QString author, QString messageText);
    void userDisconnectedMessage(QString userName);
    void removeFromList();
    void addServerMessage(QString messageText);

signals:

public slots:

protected:
    QString name;
    QString prefix;
    QString text;
    QStandardItem *item;
    QStandardItemModel *channelListModel;


};

#endif // CONVERSATION_H
