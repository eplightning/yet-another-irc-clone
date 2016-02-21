#ifndef PRIVATECONVERSATION_H
#define PRIVATECONVERSATION_H

#include "conversation.h"

class PrivateConversation : public Conversation
{
public:
    PrivateConversation(u64 userid, QString name, QStandardItemModel *channelListModel);

signals:

public slots:
};

#endif // PRIVATECONVERSATION_H
