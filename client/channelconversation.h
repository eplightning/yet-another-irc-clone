#ifndef CHANNELCONVERSATION_H
#define CHANNELCONVERSATION_H

#include <QObject>
#include <QStandardItem>

#include "common/types.h"
#include "common/packet.h"
#include "common/packets/slave_user.h"

#include "conversation.h"

using namespace YAIC;

class ChannelConversation : public Conversation
{
public:
    ChannelConversation(u64 id, QString name, QStandardItemModel *channelListModel, QVector<SlaveUserPackets::ChanUser> user, s32 flags);
    void addUser(SlaveUserPackets::ChanUser user);
    void delUser(u64 userId);
    QString getUserName(u64 id);
    QVector <SlaveUserPackets::ChanUser> getUsers();
    SlaveUserPackets::ChanUser *getUser(int position);
    u64 getId();
    bool containsUser(u64 id);
    void setFlagsToUser(u64 userId, u32 flags);
    void updateUserName(u64 userId, String nick);
    s32 getFlags();

signals:

public slots:

private:
    u64 id;
    QVector<SlaveUserPackets::ChanUser> users;
    s32 flags;
    int findUserPosition(u64 userId);
};

#endif // CHANNELCONVERSATION_H
