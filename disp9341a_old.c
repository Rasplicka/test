#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "disp9341a.h"
#include "def.h"
#include "asm.h"
#include "fontConsolas.h"
#include "spi.h"

#define     USE_SPI_PORT        1                   //0=prvni (SPI1), 1=druhy (SPI2), ...

#define     _DC_pin             LATCbits.LATC0      //pin 4, RC1
#define     _RESET_pin          LATCbits.LATC1      //pin 3, RC0  
#define     _CS_pin             LATCbits.LATC2


#define     colRED              0b0000011111111111
#define     colGREEN            0b1111100000011111
#define     colBLUE             0b1111111111100000


char pixels[1024];
char fontW=8;
char fontH=16;

static void dinit();
static void writeChar(char data, short x, short y, short color);

static void command(char cmd, char* data, int len);
static int sendSync(int index, char* buffer, int len, int dc);
static void sendAsync(int index, char* buffer, int len, int dc);
static void sendFinish(int w);
static void setColorDef();
static short exshort(short data);

static void resetDisplay();
static void pause(int t);
static void setDCPin(int value);
static void setCSPin(int index, int value);
static void setResetPin(int value);


//RESET
//1. RDDSDR - read diag
//2. SLEEP OUT
//3. SET PARAM

//https://gist.github.com/postmodern/ed6e670999f456ad9f13

void disp9341a_start()
{
    _CS_pin=1;                      //CS=0
    
    int a, b;
    char txt[] = {"Tady je nejaky text misto cisel"};
    
    
    dinit();
    
    //setColorDef();
    
    //11000 110000 00111
    //short color=0xC607;
    
    //writeChar(txt[a], 0, 0, colRED);
    //writeChar(txt[a], 32, 32, colGREEN);
    //writeChar(txt[a], 64, 64, colBLUE);
    
    for (b=0; b<18; b++)
    {
        for(a=0; a<25; a++)
        {
            writeChar(txt[a], a*8, b*16, colGREEN);
        }
    }
    
    
    while(1)
    {
        a=1;
    }
}

static void dinit()
{
    char data[4];
    
    //ili9341_spi_init();
	resetDisplay();
    
	//ili9341_writecommand8(0x01);//soft reset
    command(0x01, NULL, 0);                      //0x01 = soft reset
            
	pause(500000);       //1000 ms

    
    //power control 1 0xC0, 0x21
    data[0]=0x21;
    command(0xC0, data, 1);                      //0xC0 = power control 1
    
    //power control 2 0xC1, 0x10
    data[0]=0x10;
    command(0xC1, data, 1);                      //0xC1 = power control 2
   
    //normal mode (not partial) 0x13
    command(0x13, NULL, 0);                      //0x13 = normal mode
        
    //inversion OFF 0x20
    command(0x20, NULL, 0);                      //0x20 = inversion OFF
    
	//memory access control
    data[0]=0x48;
    command(0x36, data, 1);
    
    /*
    //display function control
    data[0]=0x08;
    data[1]=0x82;
    data[2]=0x27;
    command(0xB6, data, 3);
    */
       
    //pixel format 666-18bit / 565-16bit
    data[0]=0x55;
    command(0x3A, data, 1);                      //0x3A = pixel format, 0x55=16-bit

             
    //sleep out
    command(0x11, NULL, 0);                      //0x11 = sleep out
    
    pause(200000);            //120 ms
    
    //display ON
    command(0x29, NULL, 0);                      //0x29 = display ON 
   
}

static void writeChar(char data, short x, short y, short color)
{
    //font 8x16
    
    //Column Set 0x2A
    //param 4xByte, B[0]=start H, B[1]=start L, B[2]=end H, B[3]=end L
    short* pix16=(short*)pixels;
    pix16[0]=exshort(x);             //start col
    pix16[1]=exshort(x + 7);        //end col
    command(0x2A, pixels, 4);
    
    //Row Set 0x2B
    //param 4xByte, B[0]=start H, B[1]=start L, B[2]=end H, B[3]=end L
    pix16[0]=exshort(y);             //start row
    pix16[1]=exshort(y + 15);       //end row
    command(0x2B, pixels, 4);    
    
    //int a;
    //for(a=0; a<512; a+=2)
    //{
        //pixels[a]=(char)color;
        //pixels[a+1]=(char)(color>>8);
        //pix16[a]=colRED;
        //pix16[a+1]=colGREEN;
        //pix16[a+2]=colBLUE;
    //}
    
    int len= fontToBuffer(pixels, color, fontConsolas, data);
    
    //Write memory data 0x2C
    command(0x2C, pixels, len);
    //NOP, terminate write
    command(0x0, NULL, 0);
}

static short exshort(short data)
{
    char l=(char)data;
    char h=(char)data>>8;
    
    return ((short)l)<<8 | (short)h; 
}

static void writeLine(short x1, short y1, short x2, short y2, short color)
{
    short* pix16=(short*)pixels;
    if(y1==y2)
    {
        //horizontal line
        if(x1>x2) { short x=x1; x1=x2, x2=x; }
        
        pix16[0]=x1;                    //start col
        pix16[1]=x2;                    //end col
        command(0x2A, pixels, 4);       //ColSet
        
        pix16[0]=y1;
        pix16[1]=y1;
        command(0x2B, pixels, 4);       //RowSet
        
        short a, step=0;
        for(a=0; a<x2-x1 + 1; a++)
        {
            pix16[a]=color;
        }
        
        //Write memory data 0x2C
        command(0x2C, pixels, x2-x1 + 1);
        //NOP, terminate write
        command(0x0, NULL, 0);
    }
    
}

static void setColorDef()
{
    //odasila tabuklu konverze barev 16/18 bit
    int a;
    //RED 32 polozek
    for(a=0; a<32; a++){pixels[a]=a;}
    //GREEN 64 polozek
    for(a=0; a<64; a++){pixels[a+32]=a;}
    //BLUE
    for(a=0; a<32; a++){pixels[a+96]=a;}
    
    //Write ColorSet + 128 bytes data
    command(0x2D, pixels, 128);
    
}



static void command(char cmd, char* data, int len)
{
    //index, data, len, DC
    
    while(sendSync(0, &cmd, 1, 0) == 0) {}
    
    if(len>0)
    {
        while(sendSync(0, data, len, 1) == 0) {}
    }
}

static void sendAsync(int index, char* buffer, int len, int dc)
{
   
    while(spi_getWorking(USE_SPI_PORT) != SPI_EMPTY) { doEvents(); }
    spi_setWorking(USE_SPI_PORT, index);
    
#ifdef _DC_pin    
    //nastav D/C pin
    setDCPin(dc);   
#endif    
    
    spi_Exchange(USE_SPI_PORT, buffer, NULL, len, &sendFinish);
    
    //po skonceni transakce vola sendFinish(), ktera uvolni SPI
   
}

static int sendSync(int index, char* buffer, int len, int dc)
{
    //vraci 0x0, pokud je SPI obsazeno
    if(spi_getWorking(USE_SPI_PORT) != SPI_EMPTY) { return 0; }
    
    spi_setWorking(USE_SPI_PORT, index);
    
#ifdef _DC_pin    
    //nastav D/C pin
    setDCPin(dc);   
#endif    
    _CS_pin=0; 
    pause(1000);
    
    spi_Exchange(USE_SPI_PORT, buffer, NULL, len, &sendFinish);
    
    //po skonceni transakce vola sendFinish(), ktera uvolni SPI
    //ceka na konec
    while(spi_getWorking(USE_SPI_PORT) != SPI_EMPTY)
    {
        
    }
    _CS_pin=1; 
    return 1;
}

static void sendFinish(int w)
{
    //vola SPI interrupt, po skonceni transakce
    //param w je hodnota Working (uklada do ni index)
    
#ifdef USE_SPI_PORT     
    //uvolnit SPI
    spi_setWorking(USE_SPI_PORT, SPI_EMPTY);
    
#endif  
}

static void resetDisplay()
{
    //provede UP, DOWN (reset), UP
    //je-li zapojeno vice displeju, maji spolecny RESET (i D/C)
#ifdef _RESET_pin     
    
    //UP
    setResetPin(1);
    pause(10000);
    //DOWN - probiha reset displeje
    setResetPin(0);
    pause(50000);
    //UP
    setResetPin(1);
    pause(10000);
    
#endif    
    
}

static void pause(int t)
{
    int a;
    for(a=0; a<t; a++)
    {
        a++;
        a--;
    }
}

static void setDCPin(int value)
{
    //RB6 (pin 15)
    _DC_pin=value;
    //LATBbits.LATB6=value;
}

static void setCSPin(int index, int value)
{
    //nastav CS pin na value
    

#ifdef _CS0_pin
    if(index==0) { _CS0_pin=value; }
#endif           

#ifdef _CS1_pin
    if(index==1) { _CS1_pin=value; }
#endif 

#ifdef _CS2_pin
    if(index==2) { _CS2_pin=value; }
#endif 

#ifdef _CS3_pin
    if(index==3) { _CS3_pin=value; }
#endif 

#ifdef _CS4_pin
    if(index==4) { _CS4_pin=value; }
#endif 

#ifdef _CS5_pin
    if(index==5) { _CS5_pin=value; }
#endif 

#ifdef _CS6_pin
    if(index==6) { _CS6_pin=value; }
#endif 
    
#ifdef _CS7_pin
    if(index==7) { _CS7_pin=value; }
#endif     

    //LATBbits.LATB3=value;
}

static void setResetPin(int value)
{
    //RB7 (pin 16)
    _RESET_pin=value;
}