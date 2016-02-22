#ifndef PRIVATECONVERSATION_H
#define PRIVATECONVERSATION_H

#include "conversation.h"

class PrivateConversation : public Conversation
{
public:
    PrivateConversation(u64 userid, QString name, QStandardItemModel *channelListModel);
    u64 getUserId();
    QString getUserName();
    bool isUserOnline();
    void setUserOffline();
    void setUserName(QString newUserName);

signals:

public slots:

private:
    QString userName;
    u64 userId;
    bool userOnline;
};

#endif // PRIVATECONVERSATION_H
