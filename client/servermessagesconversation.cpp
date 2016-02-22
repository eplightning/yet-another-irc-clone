#include "servermessagesconversation.h"

ServerMessagesConversation::ServerMessagesConversation(QStandardItemModel *channelListModel):
    Conversation("", "Komunikaty Serwera", channelListModel)
{

}

void ServerMessagesConversation::addMessage(QString text)
{
    Conversation::addMessage("Serwer", text);
}

void ServerMessagesConversation::clear()
{
    text = "";
}
