#include "conversation.h"

Conversation::Conversation(QString prefix, QString name, QStandardItemModel *channelListModel)
{
    this->name = name;
    this->prefix = prefix;
    this->channelListModel = channelListModel;

    this->text = "";

    this->item = new QStandardItem();
    item->setData(getFullName(), Qt::DisplayRole);
    item->setEditable(false);
    channelListModel->appendRow(item);
    this->onTheList = true;
}

void Conversation::setRead()
{
    QFont serifFont("Sans", 9, QFont::Normal);
    item->setFont(serifFont);
}

void Conversation::setUnread()
{
    QFont serifFont("Sans", 9, QFont::Bold);
    item->setFont(serifFont);
}

void Conversation::addMessage(QString author, QString messageText)
{
    QTime time;
    text += "<b>" + author + "</b>: " + messageText + "<br>" + time.currentTime().toString() + "<br>";
    setUnread();
}

QString Conversation::getFullName()
{
    return prefix + name;
}

QString Conversation::getName()
{
    return name;
}

void Conversation::removeFromList()
{
    channelListModel->removeRow(channelListModel->indexFromItem(item).row());
    this->onTheList = false;
}

QString Conversation::getText()
{
    return text;
}

QString Conversation::getPrefix()
{
    return prefix;
}

void Conversation::addServerMessage(QString messageText)
{
    text += "<i>" + messageText + "</i><br>";
    setUnread();
}

int Conversation::getRow()
{
    return channelListModel->indexFromItem(item).row();
}

void Conversation::renameConversation(QString newName)
{
    this->name = newName;
    item->setData(getFullName(), Qt::DisplayRole);
}

void Conversation::reAddToList()
{
    this->item = new QStandardItem();
    item->setData(getFullName(), Qt::DisplayRole);
    item->setEditable(false);
    channelListModel->appendRow(item);
    this->onTheList = true;
}

bool Conversation::isOnTheList()
{
    return onTheList;
}
