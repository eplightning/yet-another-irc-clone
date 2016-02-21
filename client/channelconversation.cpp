#include "channelconversation.h"

ChannelConversation::ChannelConversation(u64 id, QString name, QStandardItemModel *channelListModel, QVector<SlaveUserPackets::ChanUser> users):
    Conversation("#", name, channelListModel)
{
    this->id = id;
    this->users = users;
}

void ChannelConversation::addUser(SlaveUserPackets::ChanUser user)
{
    if (findUserPosition(user.id) == -1)
    {
        users.push_back(user);
    }

}

void ChannelConversation::delUser(u64 userId)
{
    int index = findUserPosition(userId);
    if (index > -1)
    {
        users.erase(users.begin() + index);
    }
}

int ChannelConversation::findUserPosition(u64 userId)
{
    int index = -1;
    for (int i = 0; i < users.size(); i++)
    {
        if (users[i].id == userId)
        {
            index = i;
        }
    }
    return index;
}

u64 ChannelConversation::getId()
{
    return this->id;
}
