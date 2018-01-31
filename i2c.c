#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "i2c.h"

#ifdef I2C

#define I2C1_DISABLE_MASTER_INTERRUPT   IEC2CLR=0x002
#define I2C1_ENABLE_MASTER_INTERRUPT    IEC2SET=0x002
#define I2C1_DISABLE_SLAVE_INTERRUPT    IEC2CLR=0x001
#define I2C1_ENABLE_SLAVE_INTERRUPT     IEC2SET=0x001
#define I2C1_DISABLE_BUS_INTERRUPT      IEC2CLR=0x004
#define I2C1_ENABLE_BUS_INTERRUPT       IEC2SET=0x004

#define I2C2_DISABLE_MASTER_INTERRUPT   IEC2CLR=0x020
#define I2C2_ENABLE_MASTER_INTERRUPT    IEC2SET=0x020
#define I2C2_DISABLE_SLAVE_INTERRUPT    IEC2CLR=0x010
#define I2C2_ENABLE_SLAVE_INTERRUPT     IEC2SET=0x010
#define I2C2_DISABLE_BUS_INTERRUPT      IEC2CLR=0x040
#define I2C2_ENABLE_BUS_INTERRUPT       IEC2SET=0x040

#define I2C3_DISABLE_MASTER_INTERRUPT   IEC2CLR=0x100
#define I2C3_ENABLE_MASTER_INTERRUPT    IEC2SET=0x100
#define I2C3_DISABLE_SLAVE_INTERRUPT    IEC2CLR=0x080
#define I2C3_ENABLE_SLAVE_INTERRUPT     IEC2SET=0x080
#define I2C3_DISABLE_BUS_INTERRUPT      IEC2CLR=0x200
#define I2C3_ENABLE_BUS_INTERRUPT       IEC2SET=0x200


//local var
void (*_finish)(int);

I2CControl s1={ NULL, NULL, NULL, 0, 0, 0, 0, 0 };
I2CControl s2={ NULL, NULL, NULL, 0, 0, 0, 0, 0 };   
I2CControl s3={ NULL, NULL, NULL, 0, 0, 0, 0, 0 }; 
I2CControl* i2cStruct[]={&s1, &s2, &s3};

#define I2C1_USE
#define I2C2_USE
#define I2C3_USE

void i2c_Init()
{
    
#ifdef I2C1_USE    
    
    I2C1CON=0;                  //off, reset
    
    IPC16bits.I2C1MIP=1;
    IPC16bits.I2C1MIS=0;
    IPC16bits.I2C1SIP=1;
    IPC16bits.I2C1SIS=0;
    IPC16bits.I2C1BCIP=1;
    IPC16bits.I2C1BCIS=0;
    
    I2C1BRG=0xC;                //CPU 20MHz, bus 400kHz
    I2C1CONbits.ON=1;
    
#endif    
    
#ifdef I2C2_USE    
    
    I2C2CON=0;                  //off, reset
    
    IPC17bits.I2C2MIP=1;        //master interrupt
    IPC17bits.I2C2MIS=0;
    IPC17bits.I2C2SIP=1;        //slave interrupt
    IPC17bits.I2C2SIS=0;
    IPC17bits.I2C2BCIP=1;       //bus interrupt
    IPC17bits.I2C2BCIS=0;
    
    I2C2BRG=0xC;                //CPU-24MHz ... min. 0xB, CPU-8MHz ... min 0x5
    I2C2CONbits.ON=1;
    
#endif     
    
#ifdef I2C3_USE    
    
    I2C3CON=0;                  //off, reset
    
    IPC18bits.I2C3MIP=1;
    IPC18bits.I2C3MIS=0;
    IPC17bits.I2C3SIP=1;
    IPC17bits.I2C3SIS=0;
    IPC18bits.I2C3BCIP=1;
    IPC18bits.I2C3BCIS=0;
    
    I2C3BRG=0xC;              //CPU 20MHz, bus 400kHz
    I2C3CONbits.ON=1;
    
#endif        
    
}

char i2c_getUsed(int index)
{
    //used definuje, zda nejaky proces pouziva I2C, nebo zda je volne
    return i2cStruct[index]->used;
}

char i2c_Process(char index, char wait)
{
    //vraci SPI_STATE.SENDING=probiha vysilani bufferu, SPI_STATE.FINISHED=vysilani dokonceno
    //nastavuje SPI automaticky
    
    while(1)
    {
        if(i2cStruct[index]->process == SPI_STATE.FINISHED)
        {
            return SPI_STATE.FINISHED;
        }
        
        //vysilani neni dokonceno
        if(wait==0) { return SPI_STATE.SENDING; }
        doEvents();
    }
    
    return i2cStruct[index]->process;
}

void i2c_Free(int index)
{
    //pokud jeste probiha vysilani dat
    //ceka na dokonceni, protoze po spi_free se vetsinou vola setCS>1
    //takze by data nebyla prijata 
    
    while(i2cStruct[index]->process == SPI_STATE.SENDING)
    {
        //jeste probiha vysilani
        doEvents();
    }
    i2cStruct[index]->used=SPI_STATE.EMPTY;
}

char i2c_Use(char index, char wait, void* finish)
{
    //je-li i2c volne, nastavi SPI_STATE.USED a vraci 1 (true)
    //neni-li volne 
    //a) pri wait==1 ceka (doEvents) na uvolneni
    //b) pri wait==0 vraci 0 (obsazeno jinym procesem)
    
    while(1)
    {
        if((i2cStruct[index]->used == SPI_STATE.EMPTY) && (i2cStruct[index]->process == SPI_STATE.FINISHED))
        {
            i2cStruct[index]->used=SPI_STATE.USED;
            i2cStruct[index]->finishFn=finish;
            //i2cStruct[index]->eventFn=event;
            return 1;
        }
        
        if(wait==0) { return 0; }
        
        doEvents();
    }
}

void i2c_WriteDE(int index, char* buffer, int len, int addr)
{
    //pokud probiha vysilani, vola doEvents, ceka na dokonceni
    while(i2cStruct[index]->process != SPI_STATE.FINISHED)
    {
        doEvents();
    }
    
    i2c_Write(index, buffer, len, addr);
}

void i2c_Write(int index, char* buffer, int len, int addr)
{
    //addr je adresa slave (bez posunuti b0, pred odeslanim bude posunuta vlevo a do b0 se doplni 0=W, nebo 1=R)
    i2cStruct[index]->tx_buffer=buffer;
    i2cStruct[index]->len=len;
    i2cStruct[index]->address=addr;
    i2cStruct[index]->error=0;
    i2cStruct[index]->process=SPI_STATE.SENDING;
    i2cStruct[index]->internalState=0;
    
    if(index==0)
    { 
#ifdef I2C1_USE        
        i2c1_MasterInterrupt(); 
#endif        
    }
    else if (index==1) 
    { 
#ifdef I2C2_USE        
        i2c2_MasterInterrupt(); 
#endif        
    }
    else if (index==2) 
    { 
#ifdef I2C3_USE        
        i2c3_MasterInterrupt(); 
#endif        
    }
}


#ifdef I2C1_USE

void i2c1_MasterInterrupt()
{
   I2CControl* ctl = i2cStruct[0];
    
    //Nevim proc, ale interrupt nastava drive, nez je TFB nulovan (novy zapis do I2C2TRN je mozny az po nulovani TFB)
    //to same plati pro TRSTAT
    while(I2C1STATbits.TRSTAT==1 || I2C1STATbits.TBF==1)
    {
        
    }
    
    if(ctl->internalState==0)
    {
        //SEN, start signal
        I2C1_ENABLE_MASTER_INTERRUPT;
        
        if(I2C1STATbits.S==1)
        {
            //nyni by melo byt vzdy S=0
            //nevim proc, ale nekdy po odeslani END nastane na sbernici START stav
            //proto musi byt odeslano END
            I2C1CONbits.PEN=1;
            return;
        }
        else
        {
            I2C1CONbits.SEN=1;
            ctl->internalState=1;
            return;
        }
    }
    else if(ctl->internalState==1)
    {
        //Tx address byte (7-bit)
        //char ad=ctl->address << 1;
        I2C1TRN=(ctl->address << 1);           //b0=0, write
        
        ctl->internalState=2;
        return;
    }
    else if(ctl->internalState==2)
    {
        if(I2C1STATbits.ACKSTAT==1)
        {
            //chyba, slave odpovedel NACK
            ctl->error=1;
            I2C1CONbits.PEN=1;                  //PEN
            ctl->internalState=3;
            return;
        }
          
        //dalsi byte
        I2C1TRN=*ctl->tx_buffer;
        ctl->tx_buffer++;  
        return;
    }
    else if (ctl->internalState==3)
    {
        //after PEN, ukonceni prenosu
        I2C1_DISABLE_MASTER_INTERRUPT;
        
        if(ctl->finishFn != NULL)
        {
            _finish=ctl->finishFn;
            _finish(ctl->used);
        }
        return;
    }    
}

void i2c1_SlaveInterrupt()
{
    return;
}

void i2c1_BusInterrupt()
{
    return;
}

#endif

#ifdef I2C2_USE

void i2c2_MasterInterrupt()
{
    I2CControl* ctl = i2cStruct[1];
    
    //Nevim proc, ale interrupt nastava drive, nez je TFB nulovan (novy zapis do I2C2TRN je mozny az po nulovani TFB)
    //to same plati pro TRSTAT
    while(I2C2STATbits.TRSTAT==1 || I2C2STATbits.TBF==1)
    {
        
    }
    
    if(ctl->internalState == 0)
    {
        //SEN, start signal
        I2C2_ENABLE_MASTER_INTERRUPT;
        
        if(I2C2STATbits.S==1)
        {
            //nyni by melo byt vzdy S=0
            //nevim proc, ale nekdy po odeslani END nastane na sbernici START stav
            //proto musi byt odeslano END
            I2C2CONbits.PEN=1;
            return;
        }
        else
        {
            I2C2CONbits.SEN=1;
            ctl->internalState=1;
            return;
        }
    }
    else if(ctl->internalState == 1)
    {
        //Tx address byte (7-bit)
        //char ad=ctl->address << 1;
        I2C2TRN=(ctl->address << 1);           //b0=0, write
        
        ctl->internalState = 2;
        return;
    }
    else if(ctl->internalState == 2)
    {
        //vysilani dat
        if(I2C2STATbits.ACKSTAT==1)
        {
            //chyba, slave odpovedel NACK
            ctl->error=1;
            I2C2CONbits.PEN=1;                  //PEN
            ctl->internalState=3;
            return;
        }
        
        if(ctl->len > 0)
        {
            //dalsi byte z bufferu
            I2C2TRN=*ctl->tx_buffer;
            ctl->tx_buffer++;  
            ctl->len--;
            return;            
        }
        else
        {
            //konec, vse odeslano
            I2C2CONbits.PEN=1;                  //PEN
            ctl->internalState=3;
            return;
        }
    }
    else if (ctl->internalState == 3)
    {
        //after PEN, ukonceni prenosu
        I2C2_DISABLE_MASTER_INTERRUPT;
        
        if(ctl->finishFn != NULL)
        {
            _finish=ctl->finishFn;
            _finish(ctl->used);
        }
        
        ctl->process=SPI_STATE.FINISHED;
        return;
    }    
}

void i2c2_SlaveInterrupt()
{
    return;
}

void i2c2_BusInterrupt()
{
    return;
}

#endif

#ifdef I2C3_USE

void i2c3_MasterInterrupt()
{
    I2CControl* ctl = i2cStruct[2];
    
    //Nevim proc, ale interrupt nastava drive, nez je TFB nulovan (novy zapis do I2C2TRN je mozny az po nulovani TFB)
    //to same plati pro TRSTAT
    while(I2C3STATbits.TRSTAT==1 || I2C3STATbits.TBF==1)
    {
        
    }
    
    if(ctl->internalState == 0)
    {
        //SEN, start signal
        I2C3_ENABLE_MASTER_INTERRUPT;
        
        if(I2C3STATbits.S==1)
        {
            //nyni by melo byt vzdy S=0
            //nevim proc, ale nekdy po odeslani END nastane na sbernici START stav
            //proto musi byt odeslano END
            I2C3CONbits.PEN=1;
            return;
        }
        else
        {
            I2C3CONbits.SEN=1;
            ctl->internalState=1;
            return;
        }
    }
    else if(ctl->internalState == 1)
    {
        //Tx address byte (7-bit)
        //char ad=ctl->address << 1;
        I2C3TRN=(ctl->address << 1);           //b0=0, write
        
        ctl->internalState=2;
        return;
    }
    else if(ctl->internalState == 2)
    {
        if(I2C3STATbits.ACKSTAT==1)
        {
            //chyba, slave odpovedel NACK
            ctl->error=1;
            I2C3CONbits.PEN=1;                  //PEN
            ctl->internalState=3;
            return;
        }
        if(ctl->internalState == ctl->len)
        {
            //konec, vse odeslano
            I2C3CONbits.PEN=1;                  //PEN
            ctl->internalState=3;
            return;
        }
        
        //dalsi byte
        I2C3TRN=*ctl->tx_buffer;
        ctl->tx_buffer++;  
        return;
    }
    else if (ctl->internalState==3)
    {
        //after PEN, ukonceni prenosu
        I2C3_DISABLE_MASTER_INTERRUPT;
        
        if(ctl->finishFn != NULL)
        {
            _finish=ctl->finishFn;
            _finish(ctl->used);
        }
        return;
    }    
}

void i2c3_SlaveInterrupt()
{
    return;
}

void i2c3_BusInterrupt()
{
    return;
}

#endif

#endif //(I2C)