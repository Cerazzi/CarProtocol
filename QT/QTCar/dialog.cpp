#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_pushButton_clicked()
{
    Deg = ui->lineEdit->text().toInt();
    if(Deg <= 90 && Deg >= -90)
        emit takeDeg(Deg);
}

