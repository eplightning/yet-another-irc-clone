#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QTextBrowser>

#include "dialog.h"
#include "channeljoiningdialog.h"
#include "channelconversation.h"
#include "servermessagesconversation.h"
#include "tcpsocket.h"
#include "common/packet.h"
#include "common/types.h"
#include "common/packets/master_user.h"
#include "common/packets/slave_user.h"

using namespace YAIC;

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_sendingButton_clicked();
    void on_channelJoiningButton_clicked();
    void on_channelList_doubleClicked(const QModelIndex &index);
    void on_serverListRead(MasterUserPackets::ServerList *p);
    void on_handshakeAckCome(SlaveUserPackets::HandshakeAck *p);
    void on_channelsReceived(SlaveUserPackets::Channels *p);
    void on_channelJoined(SlaveUserPackets::ChannelJoined *p);
    void on_serverChanged();
    void on_channelLeavingButton_clicked();
    void on_channelParted(SlaveUserPackets::ChannelParted *p);
    void on_channelChosen(QString name);
    void on_channelMessage(SlaveUserPackets::ChannelMessage *p);
    void on_channelUserJoined(SlaveUserPackets::ChannelUserJoined *p);
    void on_channelUserParted(SlaveUserPackets::ChannelUserParted *p);
    void on_userDisconnected(SlaveUserPackets::UserDisconnected *p);
    void on_channelUserUpdated(SlaveUserPackets::ChannelUserUpdated *p);
    void on_userUpdated(SlaveUserPackets::UserUpdated *p);

private:
    Ui::MainWindow *ui;
    QStandardItemModel *channelListModel;
    QStandardItemModel *userListModel;
    Dialog dialog;
    QString userName;
    QString masterIP;
    int masterPort;
    QVector<ChannelConversation*> channelList;
    Vector<MasterUserPackets::ServerListServer> severs;
    tcpSocket *master;
    tcpSocket *slave;
    ServerMessagesConversation *serverConversation;
    Conversation *selectedConversation;

    void addItemToUserList(QString str);
    void showDialog();
    void connectWithServer();
    void refreshChatBox();
    void refreshUserList();
};

#endif // MAINWINDOW_H
