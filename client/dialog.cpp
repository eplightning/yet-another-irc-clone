#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    userName = "";
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}

QString Dialog::getUserName()
{
    return userName;
}

QString Dialog::getServerName()
{
    return serverIP;
}

int Dialog::getPortNumber()
{
    return port;
}

void Dialog::on_connectButton_clicked()
{
    if ((!ui->serverIPEdit->text().isEmpty()) && (!ui->userNameEdit->text().isEmpty()) && (!ui->serverPortEdit->text().isEmpty()))
    {
        userName = ui->userNameEdit->text();
        serverIP = ui->serverIPEdit->text();

        bool isPortInt;
        port = ui->serverPortEdit->text().toInt(&isPortInt, 10);
        if (!isPortInt)
        {
            QMessageBox messageBox;
            messageBox.critical(0,"Uwaga","Port musi być wartością typu int.");
            messageBox.setFixedSize(500,200);
            return;
        }
        close();
    }
    else
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Uwaga","Żadne z pól nie może być puste!");
        messageBox.setFixedSize(500,200);
    }
}
