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
    text += "<b>" + author + "</b><br>" + messageText + "<br>";
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
