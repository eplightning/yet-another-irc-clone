#ifndef SERVERMESSAGESCONVERSATION_H
#define SERVERMESSAGESCONVERSATION_H

#include "conversation.h"

class ServerMessagesConversation : public Conversation
{
public:
    ServerMessagesConversation(QStandardItemModel *channelListModel);
    void addMessage(QString text);
    void clear();

signals:

public slots:
};

#endif // SERVERMESSAGESCONVERSATION_H
