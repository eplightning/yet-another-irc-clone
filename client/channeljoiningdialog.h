#ifndef CHANNELJOININGDIALOG_H
#define CHANNELJOININGDIALOG_H

#include <QDialog>

namespace Ui {
class ChannelJoiningDialog;
}

class ChannelJoiningDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChannelJoiningDialog(QWidget *parent = 0);
    ~ChannelJoiningDialog();
    void setItems(QList<QString> list);
    QString getChoosenChannel();

private slots:
    void on_joiningButton_clicked();

private:
    Ui::ChannelJoiningDialog *ui;
    QString chosenChannel;
};

#endif // CHANNELJOININGDIALOG_H
