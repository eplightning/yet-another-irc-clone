#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QMessageBox>
#include <QRegExp>

namespace Ui
{
    class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();
    QString getUserName();
    QString getServerName();
    int getPortNumber();

private slots:
    void on_connectButton_clicked();

private:
    Ui::Dialog *ui;
    QString userName;
    QString serverIP;
    int port;

};

#endif // DIALOG_H
