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
//    if(Deg <= 180 && Deg >= 0)
        emit takeDeg(Deg);
}

