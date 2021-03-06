#include "channeljoiningdialog.h"
#include "ui_channeljoiningdialog.h"

ChannelJoiningDialog::ChannelJoiningDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChannelJoiningDialog)
{
    ui->setupUi(this);
    chosenChannel = nullptr;
}

ChannelJoiningDialog::~ChannelJoiningDialog()
{
    delete ui;
}

void ChannelJoiningDialog::setItems(QList<QString> list)
{
        ui->comboBox->addItems(list);
}

void ChannelJoiningDialog::on_joiningButton_clicked()
{
    chosenChannel = ui->comboBox->currentText();
    close();
}

void ChannelJoiningDialog::on_ChannelJoiningDialog_finished(int result)
{
    Q_UNUSED(result);
    emit setChosenChannel(chosenChannel);
}
