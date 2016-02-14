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
    return serverName;
}

void Dialog::on_connectButton_clicked()
{
    if((!ui->serverNameEdit->text().isEmpty()) && (!ui->userNameEdit->text().isEmpty()))
    {
        userName = ui->userNameEdit->text();
        close();
    }
    else
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Uwaga","Żadne z pól nie może być puste!");
        messageBox.setFixedSize(500,200);
    }
}
