#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QTextBrowser>

#include "dialog.h"
#include <vector>

namespace Ui {
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

    void on_serwerChangingButton_clicked();

    void on_channelList_doubleClicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;
    QStandardItemModel *channelListModel;
    QStandardItemModel *userListModel;
    Dialog dialog;
    QString userName;
    QString mainChatText;
    QString inChannel;

    void setChannelList(std::vector<QString> str);
    void addItemToUserList(QString str);
    void showDialog();


};

#endif // MAINWINDOW_H
