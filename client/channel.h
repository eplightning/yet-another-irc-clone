#ifndef CHANNEL_H
#define CHANNEL_H

#include <QString>
#include <QStandardItem>
#include <algorithm>

#include "common/types.h"

using namespace YAIC;

//Temporary
enum class UserFlags {
    Operator = 1 << 0
};

struct User {
    u64 id;
    UserFlags flags;
    String nick;
};

class Channel
{
public:
    Channel(u64 id, QString name, QStandardItemModel *channelListModel);
    QString getName();
    QString getText();
    void setRead();
    void setUnread();
    void addMessage(QString author, QString messageText);
    void setUsers(Vector<User> users);
    void addUser(User user);
    void delUser(u64 userId);

signals:

public slots:

private:
    u64 id;
    QString name;
    QString text;
    QStandardItem *item;
    QStandardItemModel *channelListModel;
    Vector<User> users;

    int findUserPosition(u64 userId);
};

#endif // CHANNEL_H
