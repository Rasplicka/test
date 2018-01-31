#include "def.h"

void main();




#ifdef PIC32MM0064
//interrupt vektory musi byt zde

extern void __attribute__((interrupt(), vector(0)))  iVector0();        //CPU timer
extern void __attribute__((interrupt(), vector(11))) iVector_timer1();  //Timer1
extern void __attribute__((interrupt(), vector(14))) iVector_rtc();     //RTC alarm
extern void __attribute__((interrupt(), vector(15))) iVector_adc();     //adc complete
extern void __attribute__((interrupt(), vector(21))) iVector21();       //SPI1 Tx
extern void __attribute__((interrupt(), vector(38))) iVector38();       //SPI2 Tx
#endif

#ifdef PIC32MM0256
extern void __attribute__((interrupt(), vector(0)))  iVector0();            //CPU timer
extern void __attribute__((interrupt(), vector(17))) iVector_timer1();      //Timer1
extern void __attribute__((interrupt(), vector(29))) iVector_usb();         //USB
extern void __attribute__((interrupt(), vector(32))) iVector_rtc();         //RTC alarm
extern void __attribute__((interrupt(), vector(33))) iVector_adc();         //adc complete
extern void __attribute__((interrupt(), vector(42))) iVector_spi1Tx();      //SPI1 Tx
extern void __attribute__((interrupt(), vector(45))) iVector_spi2Tx();      //SPI2 Tx
extern void __attribute__((interrupt(), vector(48))) iVector_spi3Tx();      //SPI3 Tx

extern void __attribute__((interrupt(), vector(65))) iVector_i2c1Slave();   //I2C1 Master Mode
extern void __attribute__((interrupt(), vector(66))) iVector_i2c1Master();  //I2C1 Master Mode
extern void __attribute__((interrupt(), vector(67))) iVector_i2c1Bus();     //I2C1 Master Mode
extern void __attribute__((interrupt(), vector(68))) iVector_i2c2Slave();   //I2C2 Master Mode
extern void __attribute__((interrupt(), vector(69))) iVector_i2c2Master();  //I2C2 Master Mode
extern void __attribute__((interrupt(), vector(70))) iVector_i2c2Bus();     //I2C2 Master Mode
extern void __attribute__((interrupt(), vector(71))) iVector_i2c3Slave();   //I2C3 Master Mode
extern void __attribute__((interrupt(), vector(72))) iVector_i2c3Master();  //I2C3 Master Mode
extern void __attribute__((interrupt(), vector(73))) iVector_i2c3Bus();     //I2C3 Master Mode

#endif