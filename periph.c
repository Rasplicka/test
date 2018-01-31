#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "periph.h"
#include "def.h"
#include "asm.h"



void initPort()
{
    
#ifdef PIC32MZ
    
    TRISH=0b1111111111111000;
    PORTH=0x0;
    LATHSET=0b000;
    
#endif    
    
#ifdef PIC32MM0064_28pin
    
    
    /****************************************************************************
     * Setting the Output Latch SFR(s)
     ***************************************************************************/
    LATA = 0xFFFF;
    LATB = 0xFFFF;
    LATC = 0xFFFF;
    
    ANSELA=0;
    ANSELB=0;
    ANSELC=0;
    //direction out (0) 9,8,6,5
    //TRISB = 0xFC9F; //BC9F; 
    //PIC32MM0064

    TRISBbits.TRISB5=0;     //LED   pin 14
    TRISBbits.TRISB6=0;     //D/C   pin 15
    TRISBbits.TRISB8=0;     //CLK   pin 17
    TRISBbits.TRISB9=0;     //DO    pin 18    
    TRISBbits.TRISB7=0;     //RESET pin 16
    TRISBbits.TRISB3=0;     //CS    pin 11
    //ANSELBbits.ANSB3=0;     //CS    pin 7

    //TRISA=0;
    //TRISB=0;
    //TRISC=0;
    
    
    setPortDigOut(PORTB_BASE, BIT3 | BIT5 | BIT6 | BIT7 | BIT8 | BIT9);

    //SPI2
    //RPOR2bits.RP12R=3;
    //RPOR4bits.RP18R=4;
    
#endif
    
#ifdef PIC32MM0256_36pin
    
    //nastaveni IO portu v zavislosti a typu IC a aplikaci
    //PIC32MM0256
    //tris=1 - input, tris=0 - output, ansel=1 analog input
    LATA = 0xFFFF;
    LATB = 0xFFFF;
    LATC = 0xFFFF;
    
    ANSELA=0;
    ANSELB=0;
    ANSELC=0;
    
    //B2,3, C0,1
    
    TRISCbits.TRISC3=0;     //LED1   pin 14
    TRISBbits.TRISB5=0;     //LED2   pin 15
    TRISBbits.TRISB7=0;     //LED3   pin 17 SPI2 CLK
    TRISBbits.TRISB8=0;     //       pin 18 SPI2 DATA
    
    TRISBbits.TRISB9=0;     //       pin 19 SPI2 RESET
    TRISCbits.TRISC9=0;     //       pin 21 SPI2 DC
    
    TRISBbits.TRISB2=0;     //RP8-pin1 (CLK SPI2),       (DATA I2C2)
    TRISBbits.TRISB3=0;     //RP9-pin2 (DATA OUT SPI2),  (CLK  I2C2)
    
    //pozor, je pripojeno na GND
    //TRISAbits.TRISA2=0;
    //LATAbits.LATA2=1;
    

    
    //Vadny spoj
    TRISAbits.TRISA9=0;
    LATAbits.LATA9=1;

    //RB4, RP10, pouzito pro pwm displeje
    //TRISBbits.TRISB4=0;
    //LATBbits.LATB4=0;
    //RPOR2bits.RP10R=11;

    // <editor-fold defaultstate="collapsed" desc="test">
    //test

    /*
    LATBbits.LATB7 = 1;
    */


    
    // </editor-fold>

    //A.2 je pripojeno na GND (ili9341) - nemenit
    
    // <editor-fold defaultstate="collapsed" desc="SPI2, MM 36pin, RP8, RP9, C0, C1, C2 ">
    
    //pin
    //1  RP12 - SPI2CLK
    //2  RP13 - SPI2DATA out
    //3  C0 - DC     display
    //4  C1 - RESET  display
    //5  C2 - CS     display
    //7  A2-pripojeno na GND
    //8  A3 - CS     touchpad
    //9  RP10 - SPI2DATA in
    //10 A4 - penirq touchpad
    
    //DISPLAY: RESET, DC, CS
    TRISCbits.TRISC0=0;         //pin 3 DC (ili9341), C0
    TRISCbits.TRISC1=0;         //pin 4 RESET (ili9341), C1
    TRISCbits.TRISC2=0;         //pin 5 CS (ili9341), C2
    
    //TOUCH: CS signal TouchPad
    TRISAbits.TRISA3=0;         //pin 8 CS (XPT2046), RA3
    //LATAbits.LATA3=1;
    
    //CLK, SDO
    RPOR1bits.RP8R=9;           //pin 1 SPI2CLK  RP12,
    RPOR2bits.RP9R=8;           //pin 2 SPI2DO   RP13,
    
    //SDI
    TRISBbits.TRISB4=1;
    LATBbits.LATB4=1;
    RPINR11bits.SDI2R=0b01010;  //pin 9 SPI2DI  RP10, (RB4)
    
    //RA4, TouchPad penirq
    TRISAbits.TRISA4=1;
    LATAbits.LATA4=1;
    
    // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="I2C2, MM 36pin">
    //pin
    //1  SDA        display (pin se nastavi automaticky, pri zapnuti I2C2 modulu)
    //2  CLK        display (pin se nastavi automaticky, pri zapnuti I2C2 modulu)
    //4  C1 - RESET display
    
    TRISCbits.TRISC1=0;         //pin 4 RESET (ili9341), C1
    
    // </editor-fold>

    
    //setPortDigOut(PORTC_BASE, BIT0);        //nefunguje???
    
#endif    
    
}

void initInterrupt()
{
#ifdef PIC32MZ

#endif

#ifdef PIC32MM
    
    //nepouziva c/c++ interrupt vector
    //fn.S obsahuje tabulku se skoky do interrupt fci, po skonceni provede ERET 
    //tabulka vektoru je nezavisla na EBASE (na rozdil od dokumentace), vzdy zacina na x09D00 0200
    //v tabulce je pouze skok do fn.S funkce (vlozi compilator prikazem napr.: extern void __attribute__((vector(21))) iVector21();)
    //vector spacing je 8 bytes, multivector
    //vsechny ILP pouzivaji SRS[1]
    //setSrsValue() zajisti, ze SRS[1] gp a sp budou nastaveny na pouziti v c/c++ kodu
    //SRS[1] ma vlastni zasobnik (512 Bytes), gp je nastaveno jako v SRS[0]
    
    
    //nastavi vychozi hodnoty GP a SP pro SRS[1]
    //var gp_value obsahuje hodnotu pro GP
    //vat sp_srs1_top obsahuje hodnotu pro SP
    setSrsValue();
    
    //Multivector, spacing 8 bytes, IPL 1-7 pouziva SRS[1]
    //Neobsahuje EI, STATUS.EI zustava 0 (interrupt disable)
    setInterrupt();
     
#endif        
        
}
