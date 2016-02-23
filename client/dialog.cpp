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
        //Sprawdzamy, czy nazwa użytkownika składa się z dozwolonych znaków.
        QRegExp userNameValidation("\\b[a-zA-Z0-9_]+\\b");
        userNameValidation.setCaseSensitivity(Qt::CaseInsensitive);
        userNameValidation.setPatternSyntax(QRegExp::RegExp);
        if (userNameValidation.exactMatch(ui->userNameEdit->text()))
        {
                    userName = ui->userNameEdit->text();
        }
        else
        {
            QMessageBox messageBox;
            messageBox.critical(0,"Uwaga","Nazwa użytkownika może zawierać tylko litery a-z dowolnej wielkości, cyfry oraz znak '_'.");
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
