#include "servermessagesconversation.h"

ServerMessagesConversation::ServerMessagesConversation(QStandardItemModel *channelListModel):
    Conversation("", "Komunikaty Serwera", channelListModel)
{

}

void ServerMessagesConversation::addMessage(QString text)
{
    Conversation::addMessage("Serwer", text);
    //this->text += text + "<br>";
}

void ServerMessagesConversation::clear()
{
    text = "";
}
