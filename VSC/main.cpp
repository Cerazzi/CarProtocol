#include "mbed.h"

#define SERVO_MOVE      flags.bit.bit0
#define SOUND_MEDIDA    flags.bit.bit1
#define WAIT_ECHO       flags.bit.bit2

void OnRxchar(); //cada vez que recibe un caracter salta a esta interrupcion 
int Decode(uint8_t index); //Me devuelve 1 si la cabecera va bien , 0 si hay algun error
void decodeData(uint8_t index);
void takeDistance();

typedef enum{
    ALIVE = 0xF0,
    FIRMWARE = 0xF1,
    LEDS = 0x10,
    PULSADORES = 0x12,
    SERVO = 0xA2,
    HCSR04 = 0xA3
} _eEstadoMEFcmd;

typedef enum{
    START = 0,
    RISE = 1,
    FALL = 2
} _eTakeDistance;

typedef union{
    struct{
        uint8_t bit0 : 1;
        uint8_t bit1 : 1;
        uint8_t bit2 : 1;
        uint8_t bit3 : 1;
        uint8_t bit4 : 1;
        uint8_t bit5 : 1;
        uint8_t bit6 : 1;
        uint8_t bit7 : 1;
    } bit;
    uint8_t byte;
} _uflags;

typedef union{
    uint8_t int8b[4];
    uint32_t int32b;
}_uIntBytes;

PwmOut servo(PA_8);
DigitalOut LED(PC_13);
DigitalOut TRIGGER(PB_13);
DigitalIn ECHO(PB_12);
RawSerial Pc(PA_9, PA_10);
BusIn Pulsadores(PA_4, PA_5, PA_6, PA_7);

volatile uint8_t rxData[256],RindexW,RindexR; //tdo lo que se usa como interrupcion deberia ser volatile ,este es el buffer
volatile uint8_t txData[256],TindexW,TindexR;

uint8_t ID, length, cks;

uint32_t servoTime=0, echoTime=0, HCRTime=0, timeHRC1, timeHCR2;

Timer myTimer;
_uflags flags;
_uIntBytes distanceHC;
_eTakeDistance takeD = START;

int main(){
    uint32_t move=0,counter=0,intervalo=100,mask=21;
    myTimer.start();
    TindexR=0;
    TindexW=0;
    SERVO_MOVE = false;

    Pc.baud(115200); //protocolo para pasarle info al micro t=1/11520
    Pc.attach(&OnRxchar,SerialBase::RxIrq); //interrupcion para llamar al onrxchar
    servo.period_ms(20);

    servo.pulsewidth_us(2500);
    wait_ms(350);
    servo.pulsewidth_us(500);
    wait_ms(350);
    servo.pulsewidth_us(1500);

    while(1){
        if((myTimer.read_ms()-counter)>intervalo){
            counter=myTimer.read_ms();
            LED.write( (~mask) & (1<<move));
            move++;
            move&=31;
        }
        //Si los indices son diferentes, Decodifico lo que llego por el puerto Serie
        if(RindexR!=RindexW){
            Decode(RindexW);
        }
        //Si los indices son diferentes, envío lo que hay por el puerto Serie si está disponible(writeable)
        if(TindexR!=TindexW){
            if(Pc.writeable())
                Pc.putc(txData[TindexR++]);
        }

        if(SERVO_MOVE == true){
            if ((myTimer.read_ms()-servoTime)>250){
                servoTime = myTimer.read_ms();
                SERVO_MOVE = false;
            }
        }

        takeDistance();
    }
}

void OnRxchar(){
    while(Pc.readable()){ //recibe los datos y los guarda en el buffer si tiene espacio en el mbed
        rxData[RindexW++]=Pc.getc();
    }
}

int Decode(uint8_t index){
    static uint8_t header = 0, ind=0;

    while (RindexR != index){
        switch (header){
        case 0:
            if(rxData[RindexR]=='U')
                header = 1;
            break;
        case 1:
            if(rxData[RindexR]=='N')
            header = 2;
            else{
                header = 0;
                RindexR--;
            }
            break;
        case 2:
            if(rxData[RindexR]=='E')
            header = 3;
            else{
                header=0;
                RindexR--;
            }
            break;
        case 3:
            if(rxData[RindexR]=='R')
            header = 4;
            else{
                header=0;
                RindexR--;
            }
            break;
        case 4:
            length = rxData[RindexR];
            header = 5;
            break;
        case 5:
            if(rxData[RindexR]==':'){
            ind = RindexR+1;// me quedo con la posición en donde está el ID
            header = 6;
            cks = 'U' ^ 'N' ^ 'E' ^ 'R' ^ length ^ ':';
            }else{
                header=0;
                RindexR--;
            }
            break;
        case 6:
            length--;
            if(length!=0)
                cks ^= rxData[RindexR];
            else{
                if(cks == rxData[RindexR])
                    decodeData(ind);
                header=0;
                RindexR--;
            }
            break;
            default:
                header=0;
                break;
        }
        RindexR++;
    }
    return 0;
}

void decodeData(uint8_t index){
    uint8_t bufAux[20], indiceAux=0;
    #define NBYTES  4

    bufAux[indiceAux++]='U';
    bufAux[indiceAux++]='N';
    bufAux[indiceAux++]='E';
    bufAux[indiceAux++]='R';
    bufAux[indiceAux++]=0;
    bufAux[indiceAux++]=':';

    switch (rxData[index]){
    case ALIVE:
        bufAux[indiceAux++]=ALIVE;
        bufAux[indiceAux++]=0x0D;
        bufAux[NBYTES]=0x03;    
        break;
    case FIRMWARE:
    
        break;
    case LEDS:
    
        break;
    case PULSADORES:
        bufAux[indiceAux++]=PULSADORES;
        bufAux[indiceAux++]=Pulsadores.read();
        bufAux[NBYTES]=0x03; 
        break;
    case SERVO:
        bufAux[indiceAux++]=SERVO;
        if(SERVO_MOVE == false){
            servo.pulsewidth_us(((((int8_t)rxData[index+1]) * 100) / 9) + 1500);
            servoTime = myTimer.read_ms();
            SERVO_MOVE = true;
        }
        bufAux[indiceAux++]=0x0D;
        bufAux[indiceAux++]=0x0A;
        bufAux[NBYTES]=0x04;
        break;
    case HCSR04:
        bufAux[indiceAux++]=HCSR04;
        bufAux[indiceAux++]=distanceHC.int8b[0];
        bufAux[indiceAux++]=distanceHC.int8b[1];
        bufAux[indiceAux++]=distanceHC.int8b[2];
        bufAux[indiceAux++]=distanceHC.int8b[3];
        bufAux[NBYTES]=0x06;
        break;
    default:
        bufAux[indiceAux++]=0xFF;
        bufAux[NBYTES]=0x02;    
        break;
    }
    cks=0;
    for(uint8_t i=0 ;i<indiceAux;i++){
        cks^= bufAux[i];
        txData[TindexW++]=bufAux[i];
    }
    txData[TindexW++]=cks;
}

void takeDistance(){
    if((myTimer.read_ms() - HCRTime) > 100){
        HCRTime = myTimer.read_ms();
        takeD = START;
    }
    switch (takeD){
    case START:
        if((myTimer.read_us()-echoTime)>10){
            echoTime = myTimer.read_us();
            if(TRIGGER.read() == true){
                TRIGGER.write(false);
                takeD = RISE;
            } else {
                TRIGGER.write(true);
            }
        }
    break;
    case RISE:
        if(ECHO.read() == 1){
            timeHRC1 = myTimer.read_us();
            takeD = FALL;
        }
    break;
    case FALL:
        if(ECHO.read() == 0){
            timeHCR2 = myTimer.read_us();
            if(timeHCR2 > timeHRC1)
                distanceHC.int32b = (timeHCR2 - timeHRC1)/58;
            else 
                distanceHC.int32b = (timeHCR2 - timeHRC1 + 0xFFFFFFFF)/58;
        }
    break;
    }
}