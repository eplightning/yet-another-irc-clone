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
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_sendingButton_clicked()
{
    //TODO - send text to object
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
    //master->disconnect();
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
            QObject::connect(slave, SIGNAL(channelJoined(SlaveUserPackets::ChannelJoined*)),
                                  this, SLOT(on_channelJoined(SlaveUserPackets::ChannelJoined*)));
            QObject::connect(slave, SIGNAL(channelParted(SlaveUserPackets::ChannelParted*)),
                                  this, SLOT(on_channelParted(SlaveUserPackets::ChannelParted*)));

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

    for (unsigned i = 0; i < channels.size(); i++)
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

void MainWindow::on_channelJoined(SlaveUserPackets::ChannelJoined *p)
{
    //TODO add flags
    switch(p->status())
    {
        case SlaveUserPackets::ChannelJoined::Status::Ok:
        {
            ChannelConversation *conversation = new ChannelConversation(p->id(), QString::fromStdString(p->name()),
                                                                        channelListModel, QVector<SlaveUserPackets::ChanUser>::fromStdVector(p->users()));
            channelList.push_back(conversation);
            break;
        }
        case SlaveUserPackets::ChannelJoined::Status::UnknownError:
        {
            QMessageBox messageBox;
            messageBox.critical(0,"Uwaga","Nie udało się dołączyć do kanału: "+QString::fromStdString(p->name()));
            messageBox.setFixedSize(500,200);
            break;
        }
        default:
            break;
    }
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
    channelListModel->clear();
    serverConversation = new ServerMessagesConversation(channelListModel);

    for (int i = 0; i < channelList.size(); i++)
    {
        delete channelList[i];
    }
    channelList.clear();

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


void MainWindow::on_channelLeavingButton_clicked()
{
    QString channelName;
    foreach (const QModelIndex &index, ui->channelList->selectionModel()->selectedIndexes())
    {
        channelName = channelListModel->itemFromIndex(index)->text();
    }

    if (channelName != nullptr)
    {
        for (int i = 0; i < channelList.size(); i++)
        {
            if (channelName == channelList[i]->getFullName())
            {
                SlaveUserPackets::PartChannel *packet = new SlaveUserPackets::PartChannel(channelList[i]->getId());
                slave->write(packet);
            }
        }
    }
    else
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Uwaga","Aby opuścić kanał najpierw go zaznacz.");
        messageBox.setFixedSize(500,200);
    }
}

void MainWindow::on_channelParted(SlaveUserPackets::ChannelParted *p)
{
    int index = -1;
    for (int i = 0; i < channelList.size(); i++)
    {
        if (channelList[i]->getId() == p->id())
        {
            index = i;
        }
    }

    switch(p->status())
    {
        case SlaveUserPackets::ChannelParted::Status::Ok:
            switch(p->reason())
            {
                case SlaveUserPackets::ChannelParted::Reason::Requested:
                    serverConversation->addMessage("Pomyślnie opuszczono kanał " + channelList[index]->getName());
                    break;
                case SlaveUserPackets::ChannelParted::Reason::Kicked:
                    serverConversation->addMessage("Zostałeś wyrzucony z kanału " + channelList[index]->getName());
                    break;
                case SlaveUserPackets::ChannelParted::Reason::Unknown:
                    serverConversation->addMessage("Z nieznanego powodu opuszczono kanał " + channelList[index]->getName());
                    break;
            }
            //Opuść faktycznie kanał
            channelList[index]->removeFromList();
            delete channelList[index];
            channelList.remove(index);
            break;
        case SlaveUserPackets::ChannelParted::Status::UnknownError:
            serverConversation->addMessage("Błąd przy wychodzeniu z kanału " + channelList[index]->getName());
            break;
        default:
            break;
    }
}


