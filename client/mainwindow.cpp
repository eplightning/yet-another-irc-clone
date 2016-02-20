#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{   
    ui->setupUi(this);

    //Initialing a serverList model
    channelListModel = new QStandardItemModel();
    ui->channelList->setModel(channelListModel);

    //Initialing a userList model
    userListModel = new QStandardItemModel();

    ui->userList->setModel(userListModel);
    dialog.setModal(true);
    showDialog();
    connectWithServer();

    //We need to get here names of channels on the server
    QList<QString>  a;
    a.push_back("Pierwszy");
    a.push_back("Drugi");

    //Initializing channel list
    setChannelList(a);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_sendingButton_clicked()
{
    if(!inChannel.isNull())
    {

        //Changing textEdit_2 box
        mainChatText += "<b>" + userName + "</b><br>" + ui->chatEditBox->toPlainText() + "<br>";
        ui->chatBox->setHtml("<html>"+mainChatText+"</html>");
        ui->chatEditBox->clear();
    }

}

void MainWindow::on_channelList_doubleClicked(const QModelIndex &index)
{
    inChannel = channelListModel->itemFromIndex(index)->text();

    //Test - after adding connection with the server this part will bewe changed
    userListModel->clear();
    QString a1 = (inChannel+"1");
    QString a2 = (inChannel+"2");
    addItemToUserList(a1);
    addItemToUserList(a2);

    mainChatText = "";
    ui->chatBox->setPlainText("");
}

void MainWindow::on_serverListRead(MasterUserPackets::ServerList *p)
{
    ui->chatEditBox->setPlainText("asd");
    severs  = p->servers();
    if (!severs.empty())
    {
        //slave = new tcpSocket();
        if (!slave->connectWith(QString::fromStdString(severs[0].address), severs[0].port, Packet::Direction::SlaveToUser))
        {
            QMessageBox messageBox;
            messageBox.critical(0,"Uwaga","Nie udało się połączyć z serwerem!");
            messageBox.setFixedSize(500,200);
        }
        else
        {
            QObject::connect(slave, SIGNAL(handshakeAck(SlaveUserPackets::HandshakeAck*)),
                                  this, SLOT(on_handshakeAckCome(SlaveUserPackets::HandshakeAck*)));
            QObject::connect(slave, SIGNAL(channels(SlaveUserPackets::Channels*)),
                                  this, SLOT(on_channelsReceived(SlaveUserPackets::Channels*)));
            SlaveUserPackets::Handshake *a = new SlaveUserPackets::Handshake();
            a->setNick(userName.toStdString());
            slave->write(a);
        }
    }
    else
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Uwaga","Brak serwerów.");
        messageBox.setFixedSize(500,200);
    }
}

void MainWindow::on_handshakeAckCome(SlaveUserPackets::HandshakeAck *p)
{

    switch (p->status())
    {
        case SlaveUserPackets::HandshakeAck::Status::Ok:
            ui->chatEditBox->setEnabled(true);
            ui->channelJoiningButton->setEnabled(true);
            ui->channelLeavingButton->setEnabled(true);
            ui->sendingButton->setEnabled(true);    //TODO - only when in channel
            break;
        case SlaveUserPackets::HandshakeAck::Status::UnknownError:
            qDebug() << "Unnown";
            break;
        case SlaveUserPackets::HandshakeAck::Status::InvalidNick:
            qDebug() << "Invalid nick";
            break;
        case SlaveUserPackets::HandshakeAck::Status::Full:
            qDebug() << "Full";
            break;
        default:
            //doSth
            break;
    }
}

void MainWindow::on_channelJoiningButton_clicked()
{
    SlaveUserPackets::ListChannels *channels = new SlaveUserPackets::ListChannels();
    slave->write(channels);
}

void MainWindow::on_serverChanged()
{
    showDialog();
    delete master;
    if (slave->isConnected())
    {
        slave->disconnect();
    }
    delete slave;
    connectWithServer();
}

void MainWindow::on_channelsReceived(SlaveUserPackets::Channels *p)
{
    Vector<String> channels = p->channels();
    QList<QString> qChannels;

    for (int i=0; i<channels.size(); i++)
    {
        qChannels.push_back(QString::fromStdString(channels[i]));
    }
    ChannelJoiningDialog channelJoin;
    channelJoin.setItems(qChannels);
    channelJoin.setModal(true);
    if(!channelJoin.exec())
    {
        QString channel = channelJoin.getChoosenChannel();
        if(channel != nullptr)
        {
            SlaveUserPackets::JoinChannel *packet = new SlaveUserPackets::JoinChannel(channel.toStdString());
            slave->write(packet);
        }
    }
}

//Set all channels in channelList
void MainWindow::setChannelList(QList<QString> &list)
{
    channelListModel->clear();
    for (int i = 0; i < list.size(); i++)
    {
        QStandardItem *item;
        item = new QStandardItem();

        item->setData(list[i], Qt::DisplayRole);
        item->setEditable(false);

        //Test pogrubiania danych na liście
/*        if(list[i]=="Pierwszy")
        {
            QFont serifFont("Sans", 10, QFont::Bold);
            item->setFont(serifFont);
        }
*/
        channelListModel->appendRow(item);
    }
/*
    QFont boldFont("Sans", 10, QFont::Bold);
    channelListModel->item(0, 0)->setFont(boldFont);
*/
}

//Add user to userList (listo of users on the channel)
void MainWindow::addItemToUserList(QString &str)
{
    QStandardItem *item;
    item = new QStandardItem();

    item->setData(str, Qt::DisplayRole);
    item->setEditable(false);

    userListModel->appendRow(item);
}

void MainWindow::showDialog()
{
    userName = nullptr;
    if (!dialog.exec())
    {
        userName = dialog.getUserName();
        masterIP = dialog.getServerName();
        masterPort = dialog.getPortNumber();
    }
}

void MainWindow::connectWithServer()
{
    //TODO - clear everything
    ui->chatEditBox->setEnabled(false);
    ui->channelJoiningButton->setEnabled(false);
    ui->channelLeavingButton->setEnabled(false);
    ui->sendingButton->setEnabled(false);

    master = new tcpSocket(this);
    slave = new tcpSocket(this);
    if (!master->connectWith(masterIP, masterPort, Packet::Direction::MasterToUser))
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Uwaga","Nie udało się połączyć z serwerem!");
        messageBox.setFixedSize(500,200);
    }
    else if (userName != nullptr)
    {
        QObject::connect(master, SIGNAL(serversRead(MasterUserPackets::ServerList*)),
                              this, SLOT(on_serverListRead(MasterUserPackets::ServerList*)));
        MasterUserPackets::RequestServers *a = new MasterUserPackets::RequestServers();
        a->setMax(1);
        master->write(a);
    }
}

