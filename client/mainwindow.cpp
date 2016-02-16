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

    if(!master.connectWith(masterIP, masterPort))
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Uwaga","Nie udało się połączyć z serwerem!");
        messageBox.setFixedSize(500,200);
    }
    else
    {
        MasterUserPackets::RequestServers *a = new MasterUserPackets::RequestServers ();
        a->setMax(1);
        master.write(a);
        QObject::connect(&master, SIGNAL(serversRead(MasterUserPackets::ServerList*)),
                         this, SLOT(on_serverListRead(MasterUserPackets::ServerList*)));
    }

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

void MainWindow::on_serverChangingButton_clicked()
{
    showDialog();
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
    if(!severs.empty())
    {
        ui->chatBox->setPlainText(QString::fromStdString(severs[0].address));
    }
    else
    {
        ui->chatBox->setPlainText("Pusto tu");
    }
}



void MainWindow::showDialog()
{
    if(!dialog.exec())
    {
        userName = dialog.getUserName();
        masterIP = dialog.getServerName();
        masterPort = dialog.getPortNumber();
    }

    //Test dialog - here we need to check if there were no errors while connectiong
    if (userName == "xxx")
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Uwaga","Nazwa już wykorzystywana na tym serverze");
        messageBox.setFixedSize(500,200);
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
