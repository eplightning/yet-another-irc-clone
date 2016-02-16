#include "channel.h"

Channel::Channel(u64 id, QString name, QStandardItemModel *channelListModel)
{
    this->id = id;
    this->name = name;
    this->text = "";

    this->channelListModel = channelListModel;

    this->item = new QStandardItem;
    item->setData(name, Qt::DisplayRole);
    item->setEditable(false);
    channelListModel->appendRow(item);

}

void Channel::setRead()
{
    QFont serifFont("Sans", 10, QFont::Normal);
    item->setFont(serifFont);
}

void Channel::setUnread()
{
    QFont serifFont("Sans", 10, QFont::Bold);
    item->setFont(serifFont);
}

void Channel::addMessage(QString author, QString messageText)
{
    text += "<b>" + author + "</b><br>" + messageText + "<br>";
}

void Channel::setUsers(Vector<User> users)
{
    this->users = users;
}

void Channel::addUser(User user)
{
    if (findUserPosition(user.id) == -1)
    {
        users.push_back(user);
    }

}

void Channel::delUser(u64 userId)
{
    int index = findUserPosition(userId);
    if (index > -1)
    {
        users.erase(users.begin() + index);
    }
}

int Channel::findUserPosition(u64 userId)
{
    int index = -1;
    for (unsigned i = 0; i < users.size(); i++)
    {
        if (users[i].id == userId)
        {
            index = i;
        }
    }
    return index;
}
