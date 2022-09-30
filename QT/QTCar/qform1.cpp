#include "qform1.h"
#include "ui_qform1.h"

QForm1::QForm1(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::QForm1)
{
    ui->setupUi(this);
    QSerialPort1=new QSerialPort(this);
    dialog = new Dialog(this);
    ui->comboBox->installEventFilter(this);

    ui->comboBox_2->addItem("ALIVE", 0xF0);
    ui->comboBox_2->addItem("FIRMWARE", 0xF1);
    ui->comboBox_2->addItem("LEDS", 0x10);
    ui->comboBox_2->addItem("PULSADORES", 0x12);
    ui->comboBox_2->addItem("SERVO", 0xA2);
    ui->comboBox_2->addItem("HC-SR04", 0xA3);

    header=0;
    connect(QSerialPort1,&QSerialPort::readyRead, this,&QForm1::OnRxChar);
    connect(dialog, &Dialog::takeDeg, this, &QForm1::servoDeg);
}

QForm1::~QForm1(){
    delete ui;
    delete dialog;
}

bool QForm1::eventFilter(QObject *watched, QEvent *event){
    if(watched == ui->comboBox) {
        if (event->type() == QEvent::MouseButtonPress) {
            ui->comboBox->clear();
            QSerialPortInfo SerialPortInfo1;

            for(int i=0;i<SerialPortInfo1.availablePorts().count();i++)
                ui->comboBox->addItem(SerialPortInfo1.availablePorts().at(i).portName());

            return QMainWindow::eventFilter(watched, event);
        }
        else {
            return false;
        }
    }
    else{
         return QMainWindow::eventFilter(watched, event);
    }
}

void QForm1::OnRxChar(){
    int count;
    uint8_t *buf;
    QString strHex;

    count = QSerialPort1->bytesAvailable();
    if(count <= 0)
        return;

    buf = new uint8_t[count];
    QSerialPort1->read((char *)buf, count);

    strHex = "<-- 0x";
    for (int a=0; a<count; a++) {
        strHex = strHex + QString("%1").arg(buf[a], 2, 16, QChar('0')).toUpper();
    }
     ui->plainTextEdit->appendPlainText(strHex);
    for (int i=0; i<count; i++) {
        strHex = strHex + QString("%1").arg(buf[i], 2, 16, QChar('0')).toUpper();

        switch(header){
        case 0:
            if(buf[i] == 'U'){
                header = 1;
                tmoRX = 5;
            }
            break;
        case 1:
            if(buf[i] == 'N')
                header = 2;
            else{
                header = 0;
                i--;
            }
            break;
        case 2:
            if(buf[i] == 'E')
                header = 3;
            else{
                header = 0;
                i--;
            }
            break;
        case 3:
            if(buf[i] == 'R')
                header = 4;
            else{
                header = 0;
                i--;
            }
            break;
        case 4:
            nBytes = buf[i];
            header = 5;
            break;
        case 5:
            if(buf[i] == ':'){
                header = 6;
                index = 0;
                cks = 'U' ^ 'N' ^ 'E' ^ 'R' ^ ':' ^ nBytes;
            }
            else{
                header = 0;
                i--;
            }
            break;
        case 6:
            nBytes--;
            if(nBytes > 0){
                rxBuf[index++] = buf[i];
                cks ^= buf[i];
            }
            else{
                header = 0;
                if(cks == buf[i])
                    DecodeCmd(rxBuf);
                else
                    ui->plainTextEdit->appendPlainText("ERROR CHECKSUM");
            }
            break;
        }
    }
    delete [] buf;
}

void QForm1::OnQTimer1(){

}

void QForm1::SendCMD(uint8_t *buf, uint8_t length){
    uint8_t tx[24];
    uint8_t cks, i;
    QString strHex;
    _work w;

    if(!QSerialPort1->isOpen())
        return;

    w.i32 = -1000;

    tx[7] = w.u8[0];
    tx[8] = w.u8[1];
    tx[9] = w.u8[2];
    tx[10] = w.u8[3];


    tx[0] = 'U';
    tx[1] = 'N';
    tx[2] = 'E';
    tx[3] = 'R';
    tx[4] = length + 1;
    tx[5] = ':';

    memcpy(&tx[6], buf, length);

    cks = 0;
    for (i=0; i<(length+6); i++) {
        cks ^= tx[i];
    }

    tx[i] = cks;

    strHex = "--> 0x";
    for (int i=0; i<length+7; i++) {
        strHex = strHex + QString("%1").arg(tx[i], 2, 16, QChar('0')).toUpper();
    }

    ui->plainTextEdit->appendPlainText(strHex);

    QSerialPort1->write((char *)tx, length+7);
}

void QForm1::on_pushButton_clicked(){
    if(QSerialPort1->isOpen()){
        QSerialPort1->close();
        ui->pushButton->setText("OPEN");
    } else {
        if(ui->comboBox->currentText() == "")
            return;

        QSerialPort1->setPortName(ui->comboBox->currentText());
        QSerialPort1->setBaudRate(115200);
        QSerialPort1->setParity(QSerialPort::NoParity);
        QSerialPort1->setDataBits(QSerialPort::Data8);
        QSerialPort1->setStopBits(QSerialPort::OneStop);
        QSerialPort1->setFlowControl(QSerialPort::NoFlowControl);

        if(QSerialPort1->open(QSerialPort::ReadWrite)){
            ui->pushButton->setText("CLOSE");
        }
        else
            QMessageBox::information(this, "Serial PORT", "ERROR. Opening PORT");
    }
}

void QForm1::on_pushButton_3_clicked(){
    ui->plainTextEdit->clear();
}

void QForm1::on_pushButton_2_clicked(){
    uint8_t cmd, buf[24];
//    _work w;
    int n;
    bool ok = false;
    QString strHex;


    if(ui->comboBox_2->currentText() == "")
        return;

    cmd = ui->comboBox_2->currentData().toInt();
    ui->plainTextEdit->appendPlainText("0x" + (QString("%1").arg(cmd, 2, 16, QChar('0'))).toUpper());

    n = 0;
    switch (cmd) {
        case ALIVE://ALIVE   PC=>MBED 0xF0 ;  MBED=>PC 0xF0 0x0D
            n = 1;
            ok = true;
            break;
        case FIRMWARE://FIRMWARE   PC=>MBED 0xF1 ;  MBED=>PC 0xF1 FIRMWARE
            n = 1;
            ok = true;
            break;
        case LEDS://LEDS   PC=>MBED 0x10 LEDS_A_MODIFICAR VALOR_LEDS ;  MBED=>PC 0x10 ESTADO_LEDS
            n = 3;
            ok = true;
            break;
        case PULSADORES://PULSADORES   PC=>MBED 0x12 ;  MBED=>PC 0x12 VALOR PULSADORES
            n = 1;
            ok = true;
            break;
        case SERVO://SERVO    PC=>MBED POSICIONAR_SERVO; MBED=>PC 0xA2 FIN_MOVIMIENTO_SERVO
            if(dialog->isHidden())
                dialog->show();
            break;
        case DISTANCIA://HC-SR04   PC=>MBED LAST_DISTANCE; MBED=>PC ACKWNOWLEDGE;  MBED=>PC uInt32 DISTANCE_US
            n = 1;
            ok = true;
            break;
        default:
        ;
    }
    if(ok == true){
        buf[0] = cmd;
        SendCMD(buf, n);
        ok = false;
    }
}

void QForm1::DecodeCmd(uint8_t *rxBuf){
    QString str;
    _work w;

    switch (rxBuf[0]){
    case LEDS:
        break;
     case PULSADORES:
            str = "SW3: ";
            if(rxBuf[1] & 0x08)
                str = str + "HIGH";
            else
                str = str + "LOW";
            str = str + " - SW2: ";
            if(rxBuf[1] & 0x04)
                str = str + "HIGH";
            else
                str = str + "LOW";
            str = str + " - SW1: ";
            if(rxBuf[1] & 0x02)
                str = str + "HIGH";
            else
                str = str + "LOW";
            str = str + " - SW0: ";
            if(rxBuf[1] & 0x01)
                str = str + "HIGH";
            else
                str = str + "LOW";
            ui->plainTextEdit->appendPlainText(str);
        break;
    case ALIVE:
        if(rxBuf[1] == ACKNOWLEDGE)
            ui->plainTextEdit->appendPlainText("I'M ALIVE");
        break;
    case UNKNOWNCOMANND:
        ui->plainTextEdit->appendPlainText("NO CMD");
        break;
    case SERVO:
        if(rxBuf[1] == ACKNOWLEDGE)
            ui->plainTextEdit->appendPlainText("SERVO MOVED");
        break;
    case DISTANCIA:
        if(rxBuf[5] == ACKNOWLEDGE){
            w.i32 = 0;
            for(uint8_t i=1; i<5; i++){
                w.u8[i-1] = rxBuf[i];
            }
//            ui->plainTextEdit->appendPlainText("DISTANCE: " + (QString("%1").arg(w.i32, 32, 10, QChar('0'))));
//            ui->plainTextEdit->appendPlainText("DISTANCE: %1").arg(w.i32, 32, 10, QChar('0'));
        }
        break;
    }
}

void QForm1::servoDeg(uint8_t servDeg){
    uint8_t buf[2];
    buf[0] = SERVO;
    buf[1] = servDeg;
    SendCMD(buf, 2);
}
//w.i8 = QInputDialog::getInt(this, "SERVO", "Angulo:", 0, -90, 90, 1, &ok);
//if(!ok)
//  break;
