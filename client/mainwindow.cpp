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
    QString text = ui->chatEditBox->toPlainText();
    selectedConversation->addMessage(userName, text);
    refreshChatBox();
    ui->chatEditBox->clear();

    if (selectedConversation->getPrefix() == "#")
    {
        QString name = selectedConversation->getFullName();
        u64 id;
        for (int i = 0; i < channelList.size(); i++)
        {
            if (channelList[i]->getFullName() == name)
            {
                id = channelList[i]->getId();
            }
        }
        SlaveUserPackets::SendChannelMessage *packet = new SlaveUserPackets::SendChannelMessage(id);
        packet->setMessage(text.toStdString());
        slave->write(packet);
    }
    else
    {
        PrivateConversation *conversation = static_cast<PrivateConversation*>(selectedConversation);
        SlaveUserPackets::SendPrivateMessage *packet = new SlaveUserPackets::SendPrivateMessage();
        packet->setMessage(text.toStdString());
        packet->setUser(conversation->getUserId());
        slave->write(packet);
    }
    selectedConversation->setRead();
}

void MainWindow::on_channelList_doubleClicked(const QModelIndex &index)
{
    if (channelListModel->itemFromIndex(index)->text() == "Komunikaty Serwera")
    {
        ui->chatEditBox->setEnabled(false);
        ui->sendingButton->setEnabled(false);
        selectedConversation = serverConversation;
    }
    else
    {
        ui->chatEditBox->setEnabled(true);
        ui->sendingButton->setEnabled(true);
        for (int i = 0; i < channelList.size(); i++)
        {
            if (channelList[i]->getFullName() == channelListModel->itemFromIndex(index)->text())
            {
                selectedConversation = channelList[i];
            }
        }
        for (int j = 0; j < privateMessagesList.size(); j++)
        {
            if (privateMessagesList[j]->getFullName() == channelListModel->itemFromIndex(index)->text())
            {
                selectedConversation = privateMessagesList[j];
            }
        }
    }
    selectedConversation->setRead();
    refreshUserList();
    refreshChatBox();
}

void MainWindow::on_serverListRead(MasterUserPackets::ServerList *p)
{
    master->disconnect();
    severs  = p->servers();
    if (!severs.empty())
    {
        if (!slave->connectWith(QString::fromStdString(severs[0].address), severs[0].port, Packet::Direction::SlaveToUser))
        {
            serverConversation->addMessage("Nie udało się połączyć z serwerem.");
            refreshChatBox();
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
            QObject::connect(slave, SIGNAL(channelMessage(SlaveUserPackets::ChannelMessage*)),
                                  this, SLOT(on_channelMessage(SlaveUserPackets::ChannelMessage*)));
            QObject::connect(slave, SIGNAL(channelUserJoined(SlaveUserPackets::ChannelUserJoined*)),
                                  this, SLOT(on_channelUserJoined(SlaveUserPackets::ChannelUserJoined*)));
            QObject::connect(slave, SIGNAL(channelUserPatred(SlaveUserPackets::ChannelUserParted*)),
                                  this, SLOT(on_channelUserParted(SlaveUserPackets::ChannelUserParted*)));
            QObject::connect(slave, SIGNAL(userDisconnected(SlaveUserPackets::UserDisconnected*)),
                                  this, SLOT(on_userDisconnected(SlaveUserPackets::UserDisconnected*)));
            QObject::connect(slave, SIGNAL(channelUserUpdated(SlaveUserPackets::ChannelUserUpdated*)),
                                  this, SLOT(on_channelUserUpdated(SlaveUserPackets::ChannelUserUpdated*)));
            QObject::connect(slave, SIGNAL(userUpdated(SlaveUserPackets::UserUpdated*)),
                                  this, SLOT(on_userUpdated(SlaveUserPackets::UserUpdated*)));
            QObject::connect(slave, SIGNAL(privateMessageReceived(SlaveUserPackets::PrivateMessageReceived*)),
                                  this, SLOT(on_privateMessageReceived(SlaveUserPackets::PrivateMessageReceived*)));

            SlaveUserPackets::Handshake *a = new SlaveUserPackets::Handshake();
            a->setNick(userName.toStdString());
            slave->write(a);
        }
    }
    else
    {
         serverConversation->addMessage("Brak serwerów.");
         refreshChatBox();
    }
}

void MainWindow::on_handshakeAckCome(SlaveUserPackets::HandshakeAck *p)
{
    switch (p->status())
    {
        case SlaveUserPackets::HandshakeAck::Status::Ok:
            ui->channelJoiningButton->setEnabled(true);
            ui->channelLeavingButton->setEnabled(true);
            serverConversation->addMessage("Dołączono do serwera.");
            break;
        case SlaveUserPackets::HandshakeAck::Status::UnknownError:
            serverConversation->addMessage("Podczas dołączania do serwera wystąpił nieznany błąd.");
            break;
        case SlaveUserPackets::HandshakeAck::Status::InvalidNick:
            serverConversation->addMessage("Nie dołączono do serwera z powodu niedozwolonej nazwy użytkownika.");
            break;
        case SlaveUserPackets::HandshakeAck::Status::Full:
            serverConversation->addMessage("Serwer pełny, dołączenie nie jest możliwe. Spróbuj później.");
            break;
        default:
            break;
    }
    refreshChatBox();
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
    ChannelJoiningDialog *channelJoin = new ChannelJoiningDialog(this);
    channelJoin->setItems(qChannels);
    channelJoin->setModal(true);

    connect(channelJoin, SIGNAL(setChosenChannel(QString)), this, SLOT(on_channelChosen(QString)));
    channelJoin->show();
}

void MainWindow::on_channelJoined(SlaveUserPackets::ChannelJoined *p)
{
    switch(p->status())
    {
        case SlaveUserPackets::ChannelJoined::Status::Ok:
        {
            ChannelConversation *conversation = new ChannelConversation(p->id(), QString::fromStdString(p->name()),
                                                                        channelListModel, QVector<SlaveUserPackets::ChanUser>::fromStdVector(p->users()), p->userFlags());
            channelList.push_back(conversation);
            break;
        }
        case SlaveUserPackets::ChannelJoined::Status::UnknownError:
        {
            serverConversation->addMessage("Nie udało się dołączyć do kanału: " + QString::fromStdString(p->name()));
            refreshChatBox();
            break;
        }
        default:
            break;
    }
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
    channelListModel->clear();
    userListModel->clear();

    serverConversation = new ServerMessagesConversation(channelListModel);

    selectedConversation = nullptr;

    for (int i = 0; i < channelList.size(); i++)
    {
        delete channelList[i];
    }
    channelList.clear();

    for (int i = 0; i < privateMessagesList.size(); i++)
    {
        delete privateMessagesList[i];
    }
    privateMessagesList.clear();

    ui->chatEditBox->setEnabled(false);
    ui->channelJoiningButton->setEnabled(false);
    ui->channelLeavingButton->setEnabled(false);
    ui->sendingButton->setEnabled(false);

    master = new tcpSocket(this);
    slave = new tcpSocket(this);
    if (!master->connectWith(masterIP, masterPort, Packet::Direction::MasterToUser))
    {
        serverConversation->addMessage("Nie udało się połączyć z serwerem głównym.");
        refreshChatBox();
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
        for (int j = 0; j < privateMessagesList.size(); j++)
        {
            if (channelName == privateMessagesList[j]->getFullName())
            {
                if (selectedConversation->getFullName() == channelName)
                {
                    selectedConversation = nullptr;
                }
                serverConversation->addMessage("Zamknięto konwersację z użytkownikiem " + privateMessagesList[j]->getName());
                privateMessagesList[j]->removeFromList();
                if (!privateMessagesList[j]->isUserOnline())
                {
                    delete privateMessagesList[j];
                    privateMessagesList.remove(j);
                }
            }
        }
    }
    else
    {
        serverConversation->addMessage("Aby opuścić kanał należy najpierw go zaznaczyć.");
    }
    refreshChatBox();
    refreshUserList();
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
                    refreshChatBox();
                    break;
                case SlaveUserPackets::ChannelParted::Reason::Kicked:
                    serverConversation->addMessage("Zostałeś wyrzucony z kanału " + channelList[index]->getName());
                    refreshChatBox();
                    break;
                case SlaveUserPackets::ChannelParted::Reason::Unknown:
                    serverConversation->addMessage("Z nieznanego powodu opuszczono kanał " + channelList[index]->getName());
                    refreshChatBox();
                    break;
            }

            if (selectedConversation->getFullName() == channelList[index]->getFullName())
            {
                selectedConversation = nullptr;
                refreshChatBox();
                refreshUserList();
            }
            //Opuść faktycznie kanał
            channelList[index]->removeFromList();
            delete channelList[index];
            channelList.remove(index);

            break;
        case SlaveUserPackets::ChannelParted::Status::UnknownError:
            serverConversation->addMessage("Błąd przy wychodzeniu z kanału " + channelList[index]->getName());
            refreshChatBox();
            break;
        default:
            break;
    }
}

void MainWindow::on_channelChosen(QString name)
{
        if (name != "")
        {
            SlaveUserPackets::JoinChannel *packet = new SlaveUserPackets::JoinChannel(name.toStdString());
            slave->write(packet);
        }
}

void MainWindow::refreshChatBox()
{
    if (selectedConversation!=nullptr)
    {
        ui->chatBox->setHtml(selectedConversation->getText());
        if (selectedConversation->getPrefix() == "*")
        {
            PrivateConversation *a = static_cast<PrivateConversation*>(selectedConversation);
            if(!a->isUserOnline())
            {
                ui->chatEditBox->setEnabled(false);
                ui->sendingButton->setEnabled(false);
            }
        }
    }
    else
    {
        ui->chatBox->setPlainText("");
        ui->chatEditBox->setEnabled(false);
        ui->sendingButton->setEnabled(false);
    }
}

void MainWindow::on_channelMessage(SlaveUserPackets::ChannelMessage *p)
{
    for (int i = 0; i < channelList.size(); i++)
    {
        if (channelList[i]->getId() == p->channel())
        {
            channelList[i]->addMessage(channelList[i]->getUserName(p->user()), QString::fromStdString(p->message()));
        }
    }
    refreshChatBox();
}

void MainWindow::on_channelUserJoined(SlaveUserPackets::ChannelUserJoined *p)
{
    for (int i = 0; i < channelList.size(); i++)
    {
        if (channelList[i]->getId() == p->channel())
        {
            channelList[i]->addUser(p->user());
            channelList[i]->addServerMessage("Użytkownik " + QString::fromStdString(p->user().nick) + " dołączył do konwersacji.");
        }
    }
    refreshUserList();
    refreshChatBox();
}

//Add user to userList (listo of users on the channel)
void MainWindow::addItemToUserList(QString str)
{
    QStandardItem *item;
    item = new QStandardItem();

    item->setData(str, Qt::DisplayRole);
    item->setEditable(false);

    userListModel->appendRow(item);
}

void MainWindow::refreshUserList()
{
    userListModel->clear();
    if (selectedConversation != nullptr)
    {
        if (selectedConversation->getPrefix() == "#")
        {
            /*
            for (int i = 0; i < channelList.size(); i++)
            {
                if (channelList[i]->getFullName() == selectedConversation->getFullName())
                {
                    for (int j = 0; j < channelList[i]->getUsers().size(); j++)
                    {
                        if (channelList[i]->getUser(j)->flags == 0)
                        {
                            addItemToUserList(QString::fromStdString(channelList[i]->getUser(j)->nick));
                        }
                        else
                        {
                            addItemToUserList("@" + QString::fromStdString(channelList[i]->getUser(j)->nick));
                        }
                    }
                }
            }
            */
            ChannelConversation *a = static_cast<ChannelConversation*>(selectedConversation);
            for (int i = 0; i < a->getUsers().size(); i++)
            {
                if (a->getUser(i)->flags == 0)
                {
                    addItemToUserList(QString::fromStdString(a->getUser(i)->nick));
                }
                else
                {
                    addItemToUserList("@" + QString::fromStdString(a->getUser(i)->nick));
                }
            }
        }
        else if (selectedConversation->getPrefix() == "*")
        {
            PrivateConversation *a = static_cast<PrivateConversation*>(selectedConversation);
            if (a->isUserOnline())
            {
                addItemToUserList(a->getUserName());
            }
        }
    }
}

void MainWindow::on_channelUserParted(SlaveUserPackets::ChannelUserParted *p)
{
    for (int i = 0; i < channelList.size(); i++)
    {
        if (channelList[i]->getId() == p->channel())
        {
            channelList[i]->addServerMessage("Użytkownik " + channelList[i]->getUserName(p->user()) + " opuścił konwersację.");
            channelList[i]->delUser(p->user());
        }
    }
    refreshUserList();
    refreshChatBox();
}

void MainWindow::on_userDisconnected(SlaveUserPackets::UserDisconnected *p)
{
    for (int i = 0; i < channelList.size(); i++)
    {
        if (channelList[i]->containsUser(p->id()))
        {
            channelList[i]->addServerMessage("Użytkownik " + channelList[i]->getUserName(p->id()) + " opuścił konwersację.");
            channelList[i]->delUser(p->id());
        }
    }
    for (int j = 0; j < privateMessagesList.size(); j++)
    {
        if (privateMessagesList[j]->getUserId() == p->id())
        {
            privateMessagesList[j]->setUserOffline();
            if (!privateMessagesList[j]->isOnTheList())
            {
                privateMessagesList[j]->reAddToList();
            }
            privateMessagesList[j]->addServerMessage("Użytkownik " + privateMessagesList[j]->getUserName() + " opuścił konwersację.");
        }
    }
    refreshUserList();
    refreshChatBox();
}

void MainWindow::on_channelUserUpdated(SlaveUserPackets::ChannelUserUpdated *p)
{
    //TODO - add message when needed
    for (int i = 0; i < channelList.size(); i++)
    {
        if (channelList[i]->getId() == p->channel())
        {
            for (int j = 0; j < channelList[i]->getUsers().size(); j++)
            {
                channelList[i]->setFlagsToUser(p->user(), p->flags());
            }
            //channelList[i]->addServerMessage("Użytkownik " + QString::fromStdString(p->user().nick) + " zmienił flagi.");
        }
    }
    refreshUserList();
    //refreshChatBox();
}

void MainWindow::on_userUpdated(SlaveUserPackets::UserUpdated *p)
{
    for (int i = 0; i < channelList.size(); i++)
    {
        if (channelList[i]->containsUser(p->id()))
        {
            channelList[i]->addServerMessage("Użytkownik " + channelList[i]->getUserName(p->id()) + " zmienił nick na: " + QString::fromStdString(p->nick()));
            channelList[i]->updateUserName(p->id(), p->nick());
        }
    }
    for (int j = 0; j < privateMessagesList.size(); j++)
    {
        if (privateMessagesList[j]->getUserId() == p->id())
        {
            privateMessagesList[j]->addServerMessage("Użytkownik " + privateMessagesList[j]->getUserName() +  "zmienił nick na: " + QString::fromStdString(p->nick()));
            privateMessagesList[j]->renameConversation(QString::fromStdString(p->nick()));
            privateMessagesList[j]->setUserName(QString::fromStdString(p->nick()));
        }
    }
    refreshUserList();
    refreshChatBox();
}

void MainWindow::on_userList_doubleClicked(const QModelIndex &index)
{
    if (selectedConversation != nullptr)
    {
        if (selectedConversation->getPrefix() == "#")
        {
            ChannelConversation *a = static_cast<ChannelConversation*>(selectedConversation);
                    if (!privateMessagesListContains(a->getUser(index.row())->id))
                    {
                        PrivateConversation *conversation = new PrivateConversation(a->getUser(index.row())->id,
                                                                                    QString::fromStdString(a->getUser(index.row())->nick), channelListModel);
                        privateMessagesList.push_back(conversation);
                        selectedConversation = conversation;
                    }
                    else
                    {
                        for (int j = 0; j < privateMessagesList.size(); j++)
                        {
                            if (privateMessagesList[j]->getUserId() == a->getUser(index.row())->id)
                            {
                                selectedConversation = privateMessagesList[j];
                                if (!privateMessagesList[j]->isOnTheList())
                                {
                                    privateMessagesList[j]->reAddToList();
                                }
                            }
                      }
                    }

                    ui->channelList->setCurrentIndex(channelListModel->index(selectedConversation->getRow(),0));

                    refreshChatBox();
                    refreshUserList();
        }
        else if (selectedConversation->getPrefix() == "*")
        {
            selectedConversation->setRead();
        }
    }
}

bool MainWindow::privateMessagesListContains(u64 userId)
{
    for (int i = 0; i < privateMessagesList.size(); i++)
    {
        if (privateMessagesList[i]->getUserId() == userId)
        {
            return true;
        }
    }
    return false;
}

void MainWindow::on_privateMessageReceived(SlaveUserPackets::PrivateMessageReceived *p)
{
    bool messageAdded = false;
    for (int i = 0; i < privateMessagesList.size(); i++)
    {
        if (privateMessagesList[i]->getUserId() == p->user())
        {
            messageAdded = true;
            if (!privateMessagesList[i]->isOnTheList())
            {
                privateMessagesList[i]->reAddToList();
            }
            privateMessagesList[i]->addMessage(QString::fromStdString(p->nick()), QString::fromStdString(p->message()));
            privateMessagesList[i]->setUnread();
        }
    }

    if (!messageAdded)
    {
        PrivateConversation *conversation = new PrivateConversation(p->user(), QString::fromStdString(p->nick()), channelListModel);
        conversation->addMessage(QString::fromStdString(p->nick()), QString::fromStdString(p->message()));
        conversation->setUnread();
        privateMessagesList.push_back(conversation);
    }
    refreshChatBox();
}

void MainWindow::on_serverDisconnected()
{
    serverConversation->addMessage("Utracono połączenie z serwerem");
    refreshChatBox();
}
