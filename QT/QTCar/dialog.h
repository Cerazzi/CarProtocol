#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

signals:
    void takeDeg(int8_t servDeg);

private slots:
    void on_pushButton_clicked();

private:
    Ui::Dialog *ui;
    int8_t Deg=0;
};

#endif // DIALOG_H
