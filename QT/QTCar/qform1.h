#ifndef QFORM1_H
#define QFORM1_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QTimer>
#include <dialog.h>

typedef union{
    uint8_t     u8[4];
    int8_t      i8[4];
    uint16_t    u16[2];
    int16_t     i16[2];
    uint32_t    u32;
    int32_t     i32;
    float       f;
}_work;

QT_BEGIN_NAMESPACE
namespace Ui { class QForm1; }
QT_END_NAMESPACE

class QForm1 : public QMainWindow
{
    Q_OBJECT

public:
    QForm1(QWidget *parent = nullptr);
    ~QForm1();

private slots:
    void OnQTimer1();

    void OnRxChar();

    bool eventFilter(QObject *watched, QEvent *event) override;

    void on_pushButton_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::QForm1 *ui;

    QTimer *QTimer1;
    QSerialPort *QSerialPort1;

    uint8_t rxBuf[256], header, nBytes, cks, index, tmoRX;

    void SendCMD(uint8_t *buf, uint8_t length);

    void DecodeCmd(uint8_t *rxBuf);

    void servoDeg(uint8_t servDeg);

    Dialog *dialog;

    typedef enum{
        ALIVE=0xF0,
        FIRMWARE=0xF1,
        LEDS=0x10,
        PULSADORES=0x12,
        SERVO=0xA2,
        ACKNOWLEDGE=0x0D,
        UNKNOWNCOMANND=0xFF
    }_eIDCommand;
};
#endif // QFORM1_H
