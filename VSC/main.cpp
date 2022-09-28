#include "mbed.h"

void OnRxchar(); //cada vez que recibe un caracter salta a esta interrupcion 
int Decode(uint8_t index); //Me devuelve 1 si la cabecera va bien , 0 si hay algun error

void decodeData(uint8_t index);

typedef enum{
    ALIVE = 0xF0,
    FIRMWARE = 0xF1,
    LEDS = 0x10,
    PULSADORES = 0x12,
    SERVO = 0xA2,
} _eEstadoMEFcmd;

PwmOut servo(PA_8);
DigitalOut LED(PC_13);
DigitalOut TRIGGER(PB_13);
InterruptIn ECHO(PB_12);
RawSerial Pc(PA_9, PA_10);
BusIn Pulsadores(PA_4, PA_5, PA_6, PA_7);

Timer myTimer;

volatile uint8_t rxData[256],RindexW,RindexR; //tdo lo que se usa como interrupcion deberia ser volatile ,este es el buffer
volatile uint8_t txData[256],TindexW,TindexR;
uint8_t ID, length, cks;

int main(){
    uint32_t move=0,counter=0,intervalo=100,mask=21;
    bool ServoMove=false;
    myTimer.start();
    TindexR=0;
    TindexW=0;
    Pc.baud(115200); //protocolo para pasarle info al micro t=1/11520
    Pc.attach(&OnRxchar,SerialBase::RxIrq); //interrupcion para llamar al onrxchar
    servo.period_ms(20);

    servo.pulsewidth_us(2500);
    wait_ms(300);
    servo.pulsewidth_us(500);
    wait_ms(300);
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

        if(servoMove == true){
            if ((myTimer.read_ms()-ServoTime)>100){
                servoMove=false;
            }
        }
        
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
                header=1;
            break;
        case 1:
            if(rxData[RindexR]=='N')
            header=2;
            else{
                header=0;
                RindexR--;
            }
            break;
        case 2:
            if(rxData[RindexR]=='E')
            header=3;
            else{
                header=0;
                RindexR--;
            }
            break;
        case 3:
            if(rxData[RindexR]=='R')
            header=4;
            else{
                header=0;
                RindexR--;
            }
            break;
        case 4:
            length = rxData[RindexR];
            header=5;
            break;
        case 5:
            if(rxData[RindexR]==':'){
            ind=RindexR+1;// me quedo con la posición en donde está el ID
            header=6;
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
                if(cks ==rxData[RindexR])
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
        if(servoMove = false){
            servo.pulsewidth_us(bufAux[(indiceAux++ * 2500)/90]);
            ServoTime = myTimer.read_ms;
            servoMove = true;
        }
        bufAux[indiceAux++]=servo.read();
        bufAux[indiceAux++]=0x0D;
        bufAux[NBYTES]=0x03;
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