#include "channelconversation.h"

ChannelConversation::ChannelConversation(u64 id, QString name, QStandardItemModel *channelListModel, QVector<SlaveUserPackets::ChanUser> users, s32 flags):
    Conversation("#", name, channelListModel)
{
    this->flags = flags;
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

QString ChannelConversation::getUserName(u64 id)
{
    for (int i = 0; i < users.size(); i++)
    {
        if (users[i].id == id)
        {
            return QString::fromStdString(users[i].nick);
        }
    }
    return "";
}

QVector<SlaveUserPackets::ChanUser> ChannelConversation::getUsers()
{
    return users;
}

SlaveUserPackets::ChanUser* ChannelConversation::getUser(int position)
{
    if (position < users.size())
    {
        return &users[position];
    }
    return nullptr;
}

bool ChannelConversation::containsUser(u64 id)
{
    if (findUserPosition(id) > -1)
    {
        return true;
    }
    return false;
}

void ChannelConversation::setFlagsToUser(u64 userId, u32 flags)
{
    for (int i = 0; i < users.size(); i++)
    {
        if (users[i].id == userId)
        {
            users[i].flags = flags;
        }
    }
}

void ChannelConversation::updateUserName(u64 userId, String nick)
{
    for (int i = 0; i < users.size(); i++)
    {
        if (users[i].id == userId)
        {
            users[i].nick = nick;
        }
    }
}

s32 ChannelConversation::getFlags()
{
    return flags;
}
