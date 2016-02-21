#include "privateconversation.h"

PrivateConversation::PrivateConversation(u64 userid, QString name, QStandardItemModel *channelListModel):
    Conversation("*", name, channelListModel)
{

}

