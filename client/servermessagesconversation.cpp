#include "servermessagesconversation.h"

ServerMessagesConversation::ServerMessagesConversation(QStandardItemModel *channelListModel):
    Conversation("@", "Komunikaty Serwera", channelListModel)
{

}

void ServerMessagesConversation::addMessage(QString text)
{
    this->text += text + "<br>";
}

void ServerMessagesConversation::clear()
{
    text = "";
}
