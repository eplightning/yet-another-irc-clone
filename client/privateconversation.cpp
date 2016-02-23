#include "privateconversation.h"

PrivateConversation::PrivateConversation(u64 userId, QString name, QStandardItemModel *channelListModel):
    Conversation("*", name, channelListModel)
{
    this->userId = userId;
    this->userName = name;
    this->userOnline = true;
}

u64 PrivateConversation::getUserId()
{
    return userId;
}

QString PrivateConversation::getUserName()
{
    return userName;
}

bool PrivateConversation::isUserOnline()
{
    return userOnline;
}

void PrivateConversation::setUserOffline()
{
    this->userOnline = false;
}

void PrivateConversation::setUserName(QString newUserName)
{
    this->userName = newUserName;
}
