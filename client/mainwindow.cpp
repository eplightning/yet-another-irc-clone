#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    channelListModel(nullptr),
    userListModel(nullptr),
    master(nullptr),
    slave(nullptr),
    serverConversation(nullptr),
    selectedConversation(nullptr)
{   
    ui->setupUi(this);
    QObject::connect(ui->chatEditBox, SIGNAL(enterPressed()), this, SLOT(on_sendingButton_clicked()));

    channelListModel = new QStandardItemModel();
    ui->channelList->setModel(channelListModel);

    userListModel = new QStandardItemModel();

    ui->userList->setModel(userListModel);
    dialog.setModal(true);
    showDialog();
    connectWithServer();
}

MainWindow::~MainWindow()
{
    delete ui;

    if (channelListModel)
        delete channelListModel;

    if (userListModel)
        delete userListModel;

    if (slave)
        delete slave;

    if (master)
        delete master;

    for (int i = 0; i < channelList.size(); i++)
    {
        delete channelList[i];
    }

    for (int i = 0; i < privateMessagesList.size(); i++)
    {
        delete privateMessagesList[i];
    }

    if (serverConversation)
        delete serverConversation;
}

void MainWindow::on_sendingButton_clicked()
{
    QString text = ui->chatEditBox->toPlainText();
    selectedConversation->addMessage(userName, text);
    refreshChatBox();
    ui->chatEditBox->clear();

    //Jeśli znajdujemy się w kanale wysyłamy wiadomość na kanał.
    if (selectedConversation->getPrefix() == "#")
    {
        ChannelConversation *conversation = static_cast<ChannelConversation*>(selectedConversation);
        SlaveUserPackets::SendChannelMessage packet(conversation->getId());
        packet.setMessage(text.toStdString());
        slave->write(&packet);
    }
    //W przciwnym wypadku wysyłamy prywatną wiadomość.
    else
    {
        PrivateConversation *conversation = static_cast<PrivateConversation*>(selectedConversation);
        SlaveUserPackets::SendPrivateMessage packet;
        packet.setMessage(text.toStdString());
        packet.setUser(conversation->getUserId());
        slave->write(&packet);
    }
    selectedConversation->setRead();
}

void MainWindow::on_channelList_activated(const QModelIndex &index)
{
    //Jeśli wybraliśmy Komunikaty serwera uniemożliwiamy pisanie na kanał.
    if (channelListModel->itemFromIndex(index)->text() == "Komunikaty Serwera")
    {
        ui->chatEditBox->setEnabled(false);
        ui->sendingButton->setEnabled(false);
        selectedConversation = serverConversation;
    }
    //W przeciwnym razie szukamy konwersacji (kanału lub prywatnej konwersacji) do wyświetlenia.
    else
    {
        int row = index.row();
        QVector<int> places;
        int count = 0;
        //Na liście może się znaleźć kilka prywatnych konwersacji o tej samej nazwie. Dlatego sprawdzamy, które miejsce wśród nazw o tej samej nazwie zajmuje ta przez
        //nas szukana.
        for (int z = 0; z < channelListModel->rowCount(); z++)
        {
            if(channelListModel->itemFromIndex(index)->text() == channelListModel->itemFromIndex(channelListModel->index(z,0))->text())
            {
                places.push_back(z);
            }
            count = places.indexOf(row);
        }
        ui->chatEditBox->setEnabled(true);
        ui->sendingButton->setEnabled(true);

        //Znajdujemy kanał o szukanej nazwie - nazwy kanałów są unikatowe.
        for (int i = 0; i < channelList.size(); i++)
        {
            if (channelList[i]->getFullName() == channelListModel->itemFromIndex(index)->text())
            {
                selectedConversation = channelList[i];
            }
        }
        //Znajdujemy prywatną konwersację o szukanej nazwie - biorąc pod uwagę pozycję konwersacji do otwarcia wśród konwersacji nazwanych tak samo.
        for (int j = 0; j < privateMessagesList.size(); j++)
        {
            if (privateMessagesList[j]->getFullName() == channelListModel->itemFromIndex(index)->text())
            {
                if (count == 0)
                {
                    selectedConversation = privateMessagesList[j];
                }
                count --;
            }
        }
    }
    //Zmianiamy stan kanału na przeczytany i odświeżamy wyświetlaną konwersację oraz listę użytkowników.
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
            //Jeśli udało się połączyć z serwerem (slave) to łączymy sygnały ze slotami.
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
            QObject::connect(slave, SIGNAL(serverDisconnected()),
                                  this, SLOT(on_serverDisconnected()));

            SlaveUserPackets::Handshake a;
            a.setNick(userName.toStdString());
            slave->write(&a);
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
            slave->disconnect();
            break;
        case SlaveUserPackets::HandshakeAck::Status::InvalidNick:
            serverConversation->addMessage("Nie dołączono do serwera z powodu niedozwolonej nazwy użytkownika.");
            slave->disconnect();
            break;
        case SlaveUserPackets::HandshakeAck::Status::Full:
            serverConversation->addMessage("Serwer pełny, dołączenie nie jest możliwe. Spróbuj później.");
            slave->disconnect();
            break;
        default:
            break;
    }
    refreshChatBox();
}

void MainWindow::on_channelJoiningButton_clicked()
{
    SlaveUserPackets::ListChannels channels;
    slave->write(&channels);
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

    //Przenosimy listę kanałów do qlisty.
    for (unsigned i = 0; i < channels.size(); i++)
    {
        qChannels.push_back(QString::fromStdString(channels[i]));
    }
    //Tworzymy okno wyboru kanału.
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
            serverConversation->addMessage("Nie udało się dołączyć do kanału: " + QString::fromStdString(p->name()) + ".");
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
    //Czyścimy po ew. wcześniejszej sesji.
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

    //Łączymy się z masterem.
    if (!master->connectWith(masterIP, masterPort, Packet::Direction::MasterToUser))
    {
        serverConversation->addMessage("Nie udało się połączyć z serwerem głównym.");
        refreshChatBox();
    }
    else if (userName != nullptr)
    {
        QObject::connect(master, SIGNAL(serversRead(MasterUserPackets::ServerList*)),
                              this, SLOT(on_serverListRead(MasterUserPackets::ServerList*)));
        MasterUserPackets::RequestServers a;
        a.setMax(1);
        master->write(&a);
    }

    //Zaznaczamy pierwszy kanał.
    QModelIndex first = channelListModel->item(0)->index();
    ui->channelList->selectionModel()->select(first,
                                              QItemSelectionModel::SelectionFlag::Select);
    on_channelList_activated(first);
}

void MainWindow::on_channelLeavingButton_clicked()
{
    QString channelName;
    int row = 0;
    int count = 0;
    QVector<int> places;
    foreach (const QModelIndex &index, ui->channelList->selectionModel()->selectedIndexes())
    {        
        channelName = channelListModel->itemFromIndex(index)->text();
        row = index.row();
    }

    //Jeśli wybrano jakiś element.
    if (channelName != nullptr)
    {
         // Na liście mogą być konwersacje prywatne o tej samej nazwie (sytuacja gdy pierwszy rozmówca sie wyloguje, a nowy o tej samej nazwi sie zaloguje przed
         // zamknięciem poprzedniej konwersacji). Szukamy położenia nazwy wsród duplikatów.
        for (int z = 0; z < channelListModel->rowCount(); z++)
        {
            if(channelName == channelListModel->itemFromIndex(channelListModel->index(z,0))->text())
            {
                places.push_back(z);
            }
            count = places.indexOf(row);
        }
        //Szukamy kanału do zamknięcia - brak duplikatów. W razie znalezienia - prośba o opuszczenie kanału do serwera.
        for (int i = 0; i < channelList.size(); i++)        {
            if (channelName == channelList[i]->getFullName())
            {                
                SlaveUserPackets::PartChannel packet(channelList[i]->getId());
                slave->write(&packet);
            }
        }
        //Szukamy prywatnej konwersacji do zamknięcia - z użyciem duplikatów.
        for (int j = 0; j < privateMessagesList.size(); j++)
        {
            if (channelName == privateMessagesList[j]->getFullName())
            {
                if (count == 0)
                {
                    //Jeśli znajdziemy konwersację to usuwamy ją z listy widocznych konwersacji.
                    if (selectedConversation != nullptr && selectedConversation->getFullName() == channelName)
                    {
                        selectedConversation = nullptr;
                    }
                    serverConversation->addMessage("Zamknięto konwersację z użytkownikiem " + privateMessagesList[j]->getName() + ".");
                    privateMessagesList[j]->removeFromList();

                    //Jeśli dodatkowo rozmówca już jest offline usuwamy konwersację - i tak nie da się już do niej wrócić.
                    if (!privateMessagesList[j]->isUserOnline())
                    {
                        delete privateMessagesList[j];
                        privateMessagesList.remove(j);
                    }
                }
                count--;
            }
        }
    }
    else
    {
        serverConversation->addMessage("Aby opuścić kanał należy najpierw go zaznaczyć.");
    }
    //Odświeżamy okno wiadomości i listę użytkowników.
    refreshChatBox();
    refreshUserList();
}

void MainWindow::on_channelParted(SlaveUserPackets::ChannelParted *p)
{
    //Szukamy kanału, którego dotyczy sygnał.
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
                    serverConversation->addMessage("Pomyślnie opuszczono kanał " + channelList[index]->getName() + ".");
                    refreshChatBox();
                    break;
                case SlaveUserPackets::ChannelParted::Reason::Kicked:
                    serverConversation->addMessage("Zostałeś wyrzucony z kanału " + channelList[index]->getName() + ".");
                    refreshChatBox();
                    break;
                case SlaveUserPackets::ChannelParted::Reason::Unknown:
                    serverConversation->addMessage("Z nieznanego powodu opuszczono kanał " + channelList[index]->getName() + ".");
                    refreshChatBox();
                    break;
            }

            //Jeśli opuściliśmy aktualnie otwarty kanał odświeżamy widok.
            if (selectedConversation != nullptr && selectedConversation->getFullName() == channelList[index]->getFullName())
            {
                selectedConversation = nullptr;
                refreshChatBox();
                refreshUserList();
            }
            //Usuwamy kanał z listy.
            channelList[index]->removeFromList();
            delete channelList[index];
            channelList.remove(index);
            break;
        case SlaveUserPackets::ChannelParted::Status::UnknownError:
            serverConversation->addMessage("Błąd przy wychodzeniu z kanału " + channelList[index]->getName() + ".");
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
            SlaveUserPackets::JoinChannel packet(name.toStdString());
            slave->write(&packet);
        }
}

void MainWindow::refreshChatBox()
{
    if (selectedConversation!=nullptr)
    {
        ui->chatBox->setHtml(selectedConversation->getText());
        //Jeśli otwarta jest konwersacja prywatna, której drugi uczestnik już jest online - blokujemy dodwanie wiadomości.
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
            //Dodajemy naszą nazwę użytkownika na listę użytkowników (wraz z uwzględnieniem flag).
            ChannelConversation *a = static_cast<ChannelConversation*>(selectedConversation);
            if (a->getFlags() == 0)
            {
                addItemToUserList(userName);
            }
            else
            {
                addItemToUserList("@" + userName);
            }
            //Dodajemy do listy użytkowników wraz z uwzględnieniem ich flag dla danego kanału.
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
        //Brak flag w prywatnej konwersacji - dodajemy tylko nazwę użytkownika i rozmówcy (jeśli jest online).
        else if (selectedConversation->getPrefix() == "*")
        {
            addItemToUserList(userName);
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
    //Usuwamy użytkownika z każdego kanału na którym jest.
    for (int i = 0; i < channelList.size(); i++)
    {
        if (channelList[i]->containsUser(p->id()))
        {
            channelList[i]->addServerMessage("Użytkownik " + channelList[i]->getUserName(p->id()) + " opuścił konwersację.");
            channelList[i]->delUser(p->id());
        }
    }
    //Uwuwamy użytkownika z ew. konwersacji prywatnej.
    for (int j = 0; j < privateMessagesList.size(); j++)
    {
        if (privateMessagesList[j]->getUserId() == p->id())
        {
            privateMessagesList[j]->setUserOffline();
            //Jeśli konwersacja została wcześniej zamknięta - ponownie pojawia się na liście.
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
    for (int i = 0; i < channelList.size(); i++)
    {
        if (channelList[i]->getId() == p->channel())
        {
            for (int j = 0; j < channelList[i]->getUsers().size(); j++)
            {
                channelList[i]->setFlagsToUser(p->user(), p->flags());
            }
        }
    }
    refreshUserList();
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
    //Zmiana nazwy użytkownika pociąga za sobą zmianę nazwy konwersacji prywatnej.
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

//Po wybraniu użytkownika z listy otwieramy prywatną konwersację z nim.
void MainWindow::on_userList_doubleClicked(const QModelIndex &index)
{
    //Na pierwszym miejscu na liście jest nasza użytkownika klikającego - ignorujemy wybranie tej opcji.
    if (selectedConversation != nullptr && index.row()>0)
    {
        if (selectedConversation->getPrefix() == "#")
        {
            ChannelConversation *a = static_cast<ChannelConversation*>(selectedConversation);
            //Jeśli nie mamy jeszcze konwersacji z wybranym użytkownikiem.
            if (!privateMessagesListContains(a->getUser(index.row()-1)->id))
            {
                PrivateConversation *conversation = new PrivateConversation(a->getUser(index.row()-1)->id,
                                                                                    QString::fromStdString(a->getUser(index.row()-1)->nick), channelListModel);
                privateMessagesList.push_back(conversation);
                selectedConversation = conversation;
            }
            else
            {
                for (int j = 0; j < privateMessagesList.size(); j++)
                {
                    if (privateMessagesList[j]->getUserId() == a->getUser(index.row()-1)->id)
                    {
                        selectedConversation = privateMessagesList[j];
                        //Jeśli konwersacja została zamknięta.
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
        //Jeśli kliknęliśmy w prywatnej rozmowie - wybraliśmy użytkownika z jakim właśnie rozmawiamy. Oznaczamy konwerację jako przeczytaną.
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
    //Szukamy prywatnej konwersacji na liście konwersacji.
    for (int i = 0; i < privateMessagesList.size(); i++)
    {
        if (privateMessagesList[i]->getUserId() == p->user())
        {
            messageAdded = true;
            //Jeśli konwersacja nie jest widoczna - dodajemy ją znowu do listView.
            if (!privateMessagesList[i]->isOnTheList())
            {
                privateMessagesList[i]->reAddToList();
            }
            privateMessagesList[i]->addMessage(QString::fromStdString(p->nick()), QString::fromStdString(p->message()));
            privateMessagesList[i]->setUnread();
        }
    }

    //Jak nie znaleźliśmy konwersacji - należy ją utworzyć.
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
    serverConversation->addMessage("Utracono połączenie z serwerem.");
    refreshChatBox();
}
