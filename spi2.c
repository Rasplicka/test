#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "spi.h"
#include "def.h"

#ifdef SPI

//spi driver PIC32MM0064/0256, (PIC32MZ - neni dodelano)
//ver. 9/8/2017

//SPI - Master mode
//SPI muze vyuzivat vice modulu, je-li na jednom SPI kanalu vice zarizeni, musi pouzivat CS signal
//fce spi_Exchange provede zapis dat SPI (nebo zapis i cteni, je-li zadany Rx buffer)
//data jsou v tx_bufferu, delka len
//volajici musi pockat na used=0
//napr:
//while(spi_getUsed(0)==1){ doEvents(); }
//spi_setUsed(0, 1)
//setCSpin(active)
//setDCPin(DATA)
//spi_Exchange(0, ... , &sendFinish)
//return

//void sendFinish() {
//setCSPin(deactive)
//spi_setUsed(0, 0) }




#ifdef PIC32MM

#define     SPI_FIFO_SIZE               16     //fifo size [bytes]

#ifdef PIC32MM0064
#define     SPI1_DISABLE_TxINTERRUPT    IEC0CLR=0x00200000
#define     SPI1_ENABLE_TxINTERRUPT     IEC0SET=0x00200000
#define     SPI2_DISABLE_TxINTERRUPT    IEC1CLR=0x00000040
#define     SPI2_ENABLE_TxINTERRUPT     IEC1SET=0x00000040
#endif

#ifdef PIC32MM0256
#define     SPI1_DISABLE_TxINTERRUPT    IEC1CLR=0x00000400
#define     SPI1_ENABLE_TxINTERRUPT     IEC1SET=0x00000400
#define     SPI2_DISABLE_TxINTERRUPT    IEC1CLR=0x00002000
#define     SPI2_ENABLE_TxINTERRUPT     IEC1SET=0x00002000
#define     SPI3_DISABLE_TxINTERRUPT    IEC1CLR=0x00010000
#define     SPI3_ENABLE_TxINTERRUPT     IEC1SET=0x00010000
#endif

//local var
void (*_finish)(int);

SPIControl s1={ NULL, NULL, NULL, NULL, 0, 0, 0, 0, SPI_EMPTY, SPI_WAIT };
SPIControl s2={ NULL, NULL, NULL, NULL, 0, 0, 0, 0, SPI_EMPTY, SPI_WAIT };   
SPIControl s3={ NULL, NULL, NULL, NULL, 0, 0, 0, 0, SPI_EMPTY, SPI_WAIT }; 
SPIControl* controlStruct[]={&s1, &s2, &s3};

//local void
static void clearRxFifo(int index);

void spi_Init()
{
    
#ifdef SPI1_USE
    
    SPI1CON = 0;                    // Stops and resets the SPI1.
    clearRxFifo(0);
    
#ifdef PIC32MM0064  
    // disable all interrupts IEC0.20 - SPI1Error, 21 - SPI1 TRN, 22 - SPI1 REC 
    IEC0CLR=0x00700000;
    // clear any existing event
    IFS0CLR=0x00700000;
    
    //SPI1 Tx, Rx, Err, Prior:1, SubPrior:0
    IPC5bits.SPI1TXIP = 1;                  //Tx prior
    IPC5bits.SPI1TXIS = 0;                  //Tx subprior
    IPC5bits.SPI1EIP = 1;                   //Err prior
    IPC5bits.SPI1EIS = 0;                   //Err subprior
    IPC5bits.SPI1RXIP = 1;                  //Rx prior
    IPC5bits.SPI1RXIS = 0;                  //Rx subprior
#endif  
    
#ifdef PIC32MM0256   
    IEC1bits.SPI1EIE=0;
    IEC1bits.SPI1RXIE=0;
    IEC1bits.SPI1TXIE=0;
    
    IFS1bits.SPI1EIF=0;
    IFS1bits.SPI1RXIF=0;
    IFS1bits.SPI1TXIF=0;
    
    IPC10bits.SPI1TXIP = 1;
    IPC10bits.SPI1TXIS = 0;
    IPC10bits.SPI1EIP = 1;
    IPC10bits.SPI1EIS = 0;
    IPC10bits.SPI1RXIP = 1;
    IPC10bits.SPI1RXIS = 0;
#endif
    
    // FRMERR disabled; 
    SPI1STAT = 0x0;
    // SPI1BRG 10; 
    SPI1BRG = 0x0;          //0x1... pri 8MHz
    // AUDMONO disabled; AUDEN disabled; SPITUREN disabled; FRMERREN disabled; IGNROV disabled; SPISGNEXT disabled; SPIROVEN disabled; AUDMOD disabled; IGNTUR disabled; 
    SPI1CON2 = 0x0;
    
    //buffer Enhanced, Rx pin I/O port, FrameSync disable, 8-bit data
    // MCLKSEL PBCLK; DISSDO disabled; SRXISEL Last Word is Read; CKP Idle:Low, Active:High; FRMEN disabled; FRMSYPW One-Clock; SSEN disabled; FRMCNT 1; MSSEN disabled; MSTEN Master; MODE16 disabled; FRMPOL disabled; SMP Middle; SIDL disabled; FRMSYNC disabled; CKE Idle to Active; MODE32 disabled; SPIFE Frame Sync pulse precedes; STXISEL Complete; DISSDI disabled; ON enabled; ENHBUF enabled; 
    SPI1CON = 0x18130;      //0x18130
    
#endif
    
#ifdef SPI2_USE

    SPI2CON = 0;                    // Stops and resets the SPI2.
    clearRxFifo(1);
    
#ifdef PIC32MM0064  
    
    // disable all interrupts IEC0.20 - SPI1Error, 21 - SPI1 TRN, 22 - SPI1 REC 
    IEC1CLR=0x000000E0;
    // clear any existing event
    IFS1CLR=0x000000E0;
    
    //SPI1 Tx, Rx, Err, Prior:1, SubPrior:0
    IPC9bits.SPI2TXIP = 1;                  //Tx prior
    IPC9bits.SPI2TXIS = 0;                  //Tx subprior
    IPC9bits.SPI2EIP = 1;                   //Err prior
    IPC9bits.SPI2EIS = 0;                   //Err subprior
    IPC9bits.SPI2RXIP = 1;                  //Rx prior
    IPC9bits.SPI2RXIS = 0;                  //Rx subprior
#endif    
    
#ifdef PIC32MM0256  
    IEC1bits.SPI2EIE=0;
    IEC1bits.SPI2RXIE=0;
    IEC1bits.SPI2TXIE=0;
    
    IFS1bits.SPI2EIF=0;
    IFS1bits.SPI2RXIF=0;
    IFS1bits.SPI2TXIF=0;
    
    IPC11bits.SPI2TXIP = 1;
    IPC11bits.SPI2TXIS = 0;
    IPC11bits.SPI2EIP = 1;
    IPC11bits.SPI2EIS = 0;
    IPC11bits.SPI2RXIP = 1;
    IPC11bits.SPI2RXIS = 0;
#endif  
    
    // FRMERR disabled; 
    SPI2STAT = 0x0;
    // SPI1BRG 10; 
    SPI2BRG = 0x0;          //0xA, 0x1... pri 8MHz
    // AUDMONO disabled; AUDEN disabled; SPITUREN disabled; FRMERREN disabled; IGNROV disabled; SPISGNEXT disabled; SPIROVEN disabled; AUDMOD disabled; IGNTUR disabled; 
    SPI2CON2 = 0x0;
    
    //buffer Enhanced, Rx pin I/O port, FrameSync disable, 8-bit data
    // MCLKSEL PBCLK; DISSDO disabled; SRXISEL Last Word is Read; CKP Idle:Low, Active:High; FRMEN disabled; FRMSYPW One-Clock; SSEN disabled; FRMCNT 1; MSSEN disabled; MSTEN Master; MODE16 disabled; FRMPOL disabled; SMP Middle; SIDL disabled; FRMSYNC disabled; CKE Idle to Active; MODE32 disabled; SPIFE Frame Sync pulse precedes; STXISEL Complete; DISSDI disabled; ON enabled; ENHBUF enabled; 
    SPI2CON = 0x18130;      //0x18130
    
#endif    
    
#ifdef SPI3_USE

    SPI3CON = 0;                    // Stops and resets the SPI2.
    clearRxFifo(2);
    
#ifdef PIC32MM0064  

#endif    
    
#ifdef PIC32MM0256  
    IEC1bits.SPI3EIE=0;
    IEC1bits.SPI3RXIE=0;
    IEC1bits.SPI3TXIE=0;
    
    IFS1bits.SPI3EIF=0;
    IFS1bits.SPI3RXIF=0;
    IFS1bits.SPI3TXIF=0;
    
    IPC12bits.SPI3TXIP = 1;
    IPC12bits.SPI3TXIS = 0;
    IPC11bits.SPI3EIP = 1;
    IPC11bits.SPI3EIS = 0;
    IPC12bits.SPI3RXIP = 1;
    IPC12bits.SPI3RXIS = 0;
#endif  
    
    // FRMERR disabled; 
    SPI3STAT = 0x0;
    // SPI1BRG 10; 
    SPI3BRG = 0x0;          //0xA, 0x1... pri 8MHz
    // AUDMONO disabled; AUDEN disabled; SPITUREN disabled; FRMERREN disabled; IGNROV disabled; SPISGNEXT disabled; SPIROVEN disabled; AUDMOD disabled; IGNTUR disabled; 
    SPI3CON2 = 0x0;
    
    //buffer Enhanced, Rx pin I/O port, FrameSync disable, 8-bit data
    // MCLKSEL PBCLK; DISSDO disabled; SRXISEL Last Word is Read; CKP Idle:Low, Active:High; FRMEN disabled; FRMSYPW One-Clock; SSEN disabled; FRMCNT 1; MSSEN disabled; MSTEN Master; MODE16 disabled; FRMPOL disabled; SMP Middle; SIDL disabled; FRMSYNC disabled; CKE Idle to Active; MODE32 disabled; SPIFE Frame Sync pulse precedes; STXISEL Complete; DISSDI disabled; ON enabled; ENHBUF enabled; 
    SPI3CON = 0x18130;      //0x18130
    
#endif        
    
}

int spi_getUsed(int index)
{
    //used nastavuje volajici proces, muze nastavit jakoukoliv hodnotu > -1(SPI_EMPTY)
    //tato hodnota bude pouzita po skonceni prenosu pri volani _finish, volajici proces
    //podle ni muze zjistit, jaky prenos byl ukoncen
    return controlStruct[index]->used;
}

void spi_setUsed(int index, int val)
{
    //used nastavuje volajici proces, muze nastavit jakoukoliv hodnotu > -1(SPI_EMPTY)
    //tato hodnota bude pouzita po skonceni prenosu pri volani _finish, volajici proces
    //podle ni muze zjistit, jaky prenos byl ukoncen    
    controlStruct[index]->used=val;
}

char spi_process(int index)
{
    //vraci 1=probiha vysilani bufferu, 0=vysilani dokonceno
    //nastavuje SPI automaticky
    return controlStruct[index]->process;
}

void spi_Exchange(int index, char* txbuff, char* rxbuff, int l, void* f)
{
    //je-li rxbuff=NULL, probiha pouze vysilani, jinak vysilani i prijem
    //void f se vola po ukonceni trasnakce
    
    controlStruct[index]->tx_buffer=txbuff;
    controlStruct[index]->rx_buffer=rxbuff;
    controlStruct[index]->len=l;
    //controlStruct[index].inProcess=1;
    controlStruct[index]->tx_count=0;
    controlStruct[index]->finish=f;
    controlStruct[index]->process=SPI_WORK;
    
    if(controlStruct[index]->rx_buffer != NULL)
    {
        //vyjme vsechny data z spi_rx_bufferu
        clearRxFifo(index);
    }
    
    
    
    if(index==0)
    { 
#ifdef SPI1_USE        
        spi1_TxInterrupt(); 
#endif        
    }
    else if(index==1)
    { 
#ifdef SPI2_USE             
        spi2_TxInterrupt(); 
#endif        
    }
    else if (index==2)
    {
#ifdef SPI3_USE             
        spi3_TxInterrupt();
#endif        
    }
}

void spi_ExchangeMode(int index, char* txbuff, char* rxbuff, int l, void* f, void* event)
{
    //je-li rxbuff=NULL, probiha pouze vysilani, jinak vysilani i prijem
    //void f se vola po ukonceni trasnakce
    
    controlStruct[index]->tx_buffer=txbuff;
    controlStruct[index]->rx_buffer=rxbuff;
    controlStruct[index]->len=l;
    //controlStruct[index].inProcess=1;
    controlStruct[index]->tx_count=0;
    controlStruct[index]->finish=f;
    if(event==NULL)
    {
        controlStruct[index]->mode=0;
        controlStruct[index]->mode_event=NULL;        
    }
    else
    {
        controlStruct[index]->mode=1;
        controlStruct[index]->mode_count=0;
        controlStruct[index]->mode_event=event;
    }
    
    controlStruct[index]->process=SPI_WORK;
    
    if(controlStruct[index]->rx_buffer != NULL)
    {
        //vyjme vsechny data z spi_rx_bufferu
        clearRxFifo(index);
    }
    
    if(index==0)
    { 
#ifdef SPI1_USE        
        spi1_TxInterrupt(); 
#endif        
    }
    else if(index==1)
    { 
#ifdef SPI2_USE             
        spi2_TxInterrupt(); 
#endif        
    }
    else if (index==2)
    {
#ifdef SPI3_USE             
        spi3_TxInterrupt();
#endif        
    }
}

#ifdef SPI1_USE
void spi1_TxInterrupt()
{
    //vola se z spiExchange a z SPI interruptu
    //interrupt pri dokonceni vysilani (nikoliv tx_fifo empty)
    
    SPIControl* ctl = controlStruct[0];
    
    //load rx_fifo, pokud existuje
    while(ctl->tx_count > 0)
    {
        //predpoklada, ze v rx_fifo je stejny pocet bytes, jaky byl minule odvysilan
        *ctl->rx_buffer=SPI1BUF;
        ctl->rx_buffer++;
        ctl->tx_count--;
    }

    
    if(ctl->len == 0)
    {
        //konec
        //ctl.inProcess=0;
        //disableSpi1TxInterrupt();  
        SPI1_DISABLE_TxINTERRUPT;
        
        if(ctl->finish != NULL)
        {
            _finish=ctl->finish;
            _finish(ctl->used);
        }
    }
    else
    {
        //odeslat dalsi data, max SPI_HW_BUFFER_SIZE bytes
        int c=0;
        while(ctl->len > 0 && c<SPI_FIFO_SIZE)
        {
            c++;
            ctl->len--;                                                  //pocet --
            if(ctl->rx_buffer != NULL) { ctl->tx_count ++;}       //priste bude probihat read fifo
            //tx fifo
            SPI1BUF=*ctl->tx_buffer;
            ctl->tx_buffer++;                                            //dalsi znak
        }
        
        //povolit TX buffer empty interrupt
        //enableSpi1TxInterrupt();        
        SPI1_ENABLE_TxINTERRUPT;
    }
    
}
#endif


#ifdef SPI2_USE
void spi2_TxInterrupt()
{
    //vola se z spiExchange a z SPI interruptu
    //interrupt pri dokonceni vysilani (nikoliv tx_fifo empty)
    
    //ctl->mode=1
    //[0] 0b00 000001    //nasleduje 1 byte + vola EventFn(0b00000001)
    //[1] 0xXX           //data byte
    //[2] 0b01 000100    //nasleduji 4 byte + vola EventFn(0b01000100)
    //[3-6] 4xdata
    //len=7
    //pokud ctl->mode_count=0, jedna se o command byte 
    //command byte definuje pocet bytes dat, ktere nasleduji (ty se vysilaji). Zaroven se CB pouzije jako param pri volani Event fce
    //po odvysilani daneho poctu dat nasleduje dalsi CB, dokud (ctl->len > 0)
    
    SPIControl* ctl = controlStruct[1];
    
    //load rx_fifo, pokud existuje
    while(ctl->tx_count > 0)
    {
        //tx_count je pocet bytes posledniho vysilani
        //predpoklada, ze v rx_fifo je stejny pocet bytes, jaky byl minule odvysilan
        *ctl->rx_buffer=SPI2BUF;
        ctl->rx_buffer++;
        ctl->tx_count--;
    }

    
    if(ctl->len == 0)
    {
        //konec, vsechna data byla odeslana
        SPI2_DISABLE_TxINTERRUPT;
        
        if(ctl->finish != NULL)
        {
            _finish=ctl->finish;
            _finish(ctl->used);
        }
        
        ctl->process=SPI_WAIT;
    }
    else
    {
        if(ctl->mode==1 && ctl->mode_count==0)
        {
            char x=*ctl->tx_buffer;
            ctl->mode_count=(x & 0x3F);                   //nuluje b7,b6 (b0-b5 = pocet bytes)
            _finish=ctl->mode_event;
            _finish(x);
            
            if((x>>6)==0b11)
            {
                //pokud command byte b6-b7 = 11, nastavi mode=0 (pouze odesila zbytek bufferu v modu 0)
                ctl->mode=0;
            }
                
            ctl->len--;
            ctl->tx_buffer++; 
        }
        
        int c=0;
        if(ctl->mode==0)
        {
            //odeslat dalsi data, max SPI_HW_BUFFER_SIZE bytes
            while(ctl->len > 0 && c<SPI_FIFO_SIZE)
            {
                c++;
                ctl->len--;                                                     //pocet --
                if(ctl->rx_buffer != NULL) { ctl->tx_count ++;}                 //priste bude probihat read fifo
                //tx fifo
                SPI2BUF=*ctl->tx_buffer;
                ctl->tx_buffer++;                                               //dalsi znak
            }
        }
        else
        {
            while(ctl->mode_count > 0 && ctl->len > 0 && c<SPI_FIFO_SIZE)
            {
                c++;
                ctl->mode_count--;
                ctl->len--;                                                     //pocet --
                if(ctl->rx_buffer != NULL) { ctl->tx_count ++;}                 //priste bude probihat read fifo
                //tx fifo
                SPI2BUF=*ctl->tx_buffer;
                ctl->tx_buffer++;                                               //dalsi znak
            }                
        }
        
        //povolit TX buffer empty interrupt
        SPI2_ENABLE_TxINTERRUPT;
    }
        
}
#endif


#ifdef SPI3_USE
void spi3_TxInterrupt()
{
    //vola se z spiExchange a z SPI interruptu
    //interrupt pri dokonceni vysilani (nikoliv tx_fifo empty)
    
    SPIControl* ctl = controlStruct[2];
    
    //load rx_fifo, pokud existuje
    while(ctl->tx_count > 0)
    {
        //predpoklada, ze v rx_fifo je stejny pocet bytes, jaky byl minule odvysilan
        *ctl->rx_buffer=SPI3BUF;
        ctl->rx_buffer++;
        ctl->tx_count--;
    }

    
    if(ctl->len == 0)
    {
        //konec
        //ctl.inProcess=0;
        //disableSpi1TxInterrupt();  
        SPI3_DISABLE_TxINTERRUPT;
        
        if(ctl->finish != NULL)
        {
            _finish=ctl->finish;
            _finish(ctl->used);
        }
    }
    else
    {
        //odeslat dalsi data, max SPI_HW_BUFFER_SIZE bytes
        int c=0;
        while(ctl->len > 0 && c<SPI_FIFO_SIZE)
        {
            c++;
            ctl->len--;                                                 //pocet --
            if(ctl->rx_buffer != NULL) { ctl->tx_count ++;}             //priste bude probihat read fifo
            //tx fifo
            SPI3BUF=*ctl->tx_buffer;
            ctl->tx_buffer++;                                           //dalsi znak
        }
        
        //povolit TX buffer empty interrupt
        //enableSpi1TxInterrupt();        
        SPI3_ENABLE_TxINTERRUPT;
    }
        
}
#endif


static void clearRxFifo(int index)
{
#ifdef SIMULATOR
    return;
#endif    
    
    //vyprazdni rx_fifo
    int x;
    if(index==0)
    {
        while(SPI1STATbits.SPIRBE==0)
        {
            x=SPI1BUF;
        }
    }
    else 
    {
        while(SPI2STATbits.SPIRBE==0)
        {
            x=SPI2BUF;
        }
    }
}

#endif //(ifdef PIC32MM)

#ifdef PIC32MZ

#endif  //(ifdef PIC32MZ)


#endif