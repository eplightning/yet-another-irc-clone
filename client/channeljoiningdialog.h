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

signals:
    void setChosenChannel(QString name);

private slots:
    void on_joiningButton_clicked();

    void on_ChannelJoiningDialog_finished(int result);

private:
    Ui::ChannelJoiningDialog *ui;
    QString chosenChannel;
};

#endif // CHANNELJOININGDIALOG_H
