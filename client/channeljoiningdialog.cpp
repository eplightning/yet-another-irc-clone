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

QString ChannelJoiningDialog::getChoosenChannel()
{
    return chosenChannel;
}
