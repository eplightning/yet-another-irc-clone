#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QTextBrowser>

#include "dialog.h"
#include "channeljoiningdialog.h"
#include "channel.h"
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
    void on_serverChanged();

private:
    Ui::MainWindow *ui;
    QStandardItemModel *channelListModel;
    QStandardItemModel *userListModel;
    Dialog dialog;
    QString userName;
    QString mainChatText;
    QString inChannel;
    Channel *channelList;
    Vector<MasterUserPackets::ServerListServer> severs;
    tcpSocket *master;
    tcpSocket *slave;
    QString masterIP;
    int masterPort;

    void setChannelList(QList<QString>  &str);
    void addItemToUserList(QString &str);
    void showDialog();
    void connectWithServer();
};

#endif // MAINWINDOW_H
