#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"

//I2C_ADDRESS displeje je  0b0111100 a 0b0111101 (tzn. ze na jednu bus lze pripojit dva displeje. Adresu b.0 urcuje pin displeje D/C - GND nebo Vcc)

#define USE_DISP1306
#define SW_RESET                                //je definovano, pokud je RESET signal pripojen k IO pinu. Neni-li definovano, ma HW RESET (RC obvod)

#ifdef USE_DISP1306
//font nebo image se generuje programem lcd-image-converter
//pro ssd1306 plati toto nastaveni
//1. load image (nebo font)
//2. okno Options: Prepare: Monochrome, MainScanDirection=Left To Right, LineScanDirection=Forward, Bands=8px
//3. okno Reordering: prehodit b0>b7, b1>b6, b2>b5, b3>b4, ... 
//4. okno Options: Image: BlockSize=8bit, ByteOrder=Little Endian
//Kazdy bajt je sloupec 8 pixelu, b0=horni px, b7=dolni px. Nejprve se plni prvni radek (8 px na vysku), pak druhy radek atd...
//Tzn. ze napr. 10 bytes na jednom radku vytvori box 8(h) x 10(w) px
//https://gist.github.com/postmodern/ed6e670999f456ad9f13

//globalni promene modulu (tzn. pri pouziti vice displeju ssd1306 jsou spolecne pro vsechny struct DISPLAY)
extern IMAGE_SRC fontSys;                       //pouziva fce print

#define     BUFFER_SIZE      256               //min. velikost by mela byt: Width x 2 + 20 (viz. fillBox, clear)           
static char pixelsEven[BUFFER_SIZE] __attribute__((aligned(4)));
static char pixelsOdd [BUFFER_SIZE] __attribute__((aligned(4)));
static char isInitialized=0;
static char pixAB=0;
//static LINE_SRC lineSrc;
//static POINT point;

//privatni struct pro dany displej, pri pouziti vice displeju se nastavi pri kazdem volani setGraphics
static PORT_INFO* portInfo=NULL;            
static DISPLAY* displayInfo=NULL;

//static char Orientation=0;
static short Width=128;                         //default na vysku, fce dinit nastavi displej na sirku a upravi W a H              
static short Height=64;


//local void
static void selectPort(PORT_INFO* pi, void* d);
static void drawString(char* text, IMAGE_SRC* font, short x, short y);
static void fillBox(short x1, short y1, short x2, short y2, short color);
static void drawLine(short x1, short y1, short x2, short y2, short w, short color);
static void drawImage(IMAGE_SRC* da, short x, short y);
static void drawPoint(short x, short y, short color);
static short textWidth(char* text, IMAGE_SRC* font);
static void clear(short color);
static void initDisplay();
static void setOrientation(char x);
static char getOrientation();
static void setBrightness(char val);
static void controlDisplay(char on, char sleep, char bl, char inv);
static char getInitialized();
static void print(char* t);
static short getWidth();
static short getHeight();

static void dinit();
static void writeChar(IMAGE_SRC* font, char code, short x, short y);
static int prefixSpi(char buffer[], short start_x, short end_x, short start_page, short end_page);
static int prefixI2c(char buffer[], short start_x, short end_x, short start_page, short end_page);
static void eventDC(char x);

static char* getBuffer();
static void getPort();
static void freePort();
//static void writeBuffer(char* buff, int len, char mode);

static void resetDisplay();
void setDCPin(char value);
static void setCSPin(char value);
static void setResetPin(char value);


//global
void disp1306_driver(DISPLAY* d)
{
    d->selectPort=&selectPort;
    d->drawString=&drawString;
    d->fillBox=&fillBox;
    d->drawLine=&drawLine;
    d->drawImage=&drawImage;
    d->drawPoint=&drawPoint;
    d->textWidth=&textWidth;
    d->print=&print;
    d->clear=&clear;
    d->initDisplay=&initDisplay;
    d->setOrientation=&setOrientation;
    d->setBrightness=&setBrightness;
    d->controlDisplay=&controlDisplay;
    d->getInitialized=&getInitialized;
    d->getOrientation=&getOrientation;
    d->getWidth=&getWidth;
    d->getHeight=&getHeight;
}


//local
static void selectPort(PORT_INFO* pi, void* d)
{
    portInfo=pi;
    portInfo->eventFn=&eventDC;
    
    displayInfo=(DISPLAY*)d;
}

static void drawString(char* text, IMAGE_SRC* font, short x, short y)
{
    int a;
    int l=strLen(text);
    
    if(font==NULL) { font=&fontSys; }
    
    getPort();
    
    for(a=0; a<l; a++)
    {
        writeChar(font, text[a], x, y);
        x+=font->width;
    }
    
    freePort();
    
}

static void fillBox(short x1, short y1, short x2, short y2, short color)
{
    //plni box vzorem, dolni byte parametru color
    //y musi byt delitelne 8
    if((y1 % 8) != 0) { return; }
     
    //x1 musi byt mensi, nez x2, to same y1 a y2
    if(x1 > x2) { short x=x1; x1=x2, x2=x; }
    if(y1 > y2) { short y=y1; y1=y2, y2=y; }  
    
    if(x1 >= Width) { return; }             //mimo, vlevo
    if(x2 < 0) { return; }                  //mimo, vpravo
    if(y1 >= Height) { return; }            //mimo nahore
    if(y2 < 0) { return; }                  //mimo dole
    
    if(x1 < 0){ x1=0; }
    if(x2 > (Width-1)) { x2=Width-1; }
    if(y1 < 0) { y1=0; }
    if(y2 > (Height-1)) { y2=Height-1; }
    
    getPort();
    
    int len=0;
    char* buffer=getBuffer();
    if(portInfo->periphType==PERIPH_TYPE.i2c)
    {
        len = prefixI2c(buffer, (char)x1, (char)x2, (char)(y1/8), (char)(y2/8));
        portInfo->writeBuffer(portInfo, buffer, len); 
    }
    else if(portInfo->periphType==PERIPH_TYPE.spi)
    {
        len = prefixSpi(buffer, (char)x1, (char)x2, (char)(y1/8), (char)(y2/8));
        portInfo->writeBufferMode(portInfo, buffer, len, 1); 
    }
    
    
    int count=(x2-x1 + 1) * ((y2-y1)/8 + 1);        //pocet bytes
    char patern=(char)color;                        //pouzije dolni byte parametru color
    
    if(portInfo->directMode==1)
    {
        //pouze SPI
        //direct write data >> SPIBUF
        spi_Process(portInfo->portIndex, 1);         //ceka na dokonceni      

        //odesila patern primo do hw_bufferu (SPIxBUF) (SPI 8-bit mode)
        int* spi_buf=portInfo->directModeHwBuffer;
        while(count>0)
        {
            *spi_buf=patern;
            count--;
        }
    }
    else
    {
        //SPI, I2C
        int a;
        while(count>0)
        {
            len=0;
            buffer=getBuffer(); 
            
            for(a=0; a<BUFFER_SIZE; a++)
            {
                buffer[a]=patern;
                len++;
                count--;
                if(count==0) { break; }
            }
            
            portInfo->writeBuffer(portInfo, buffer, len);
        }
    }
    
    freePort();
}

static void drawLine(short x1, short y1, short x2, short y2, short w, short color)
{
    //neumoznuje zobrazit cary
}

static void drawImage(IMAGE_SRC* da, short x, short y)
{
    //predpoklada, ze obrazek ma vysku 8/16/24/32/40/48/64
    //y musi byt delitelne 8
    if((y % 8 != 0)) { return; }
    
    //cely obrazek musi byt na displeji, nepodporuje zobrazeni casti obrazku
    if(x < 0) { return; }
    if((x + da->width) > Width) { return; }
    if(y < 0) { return; }
    if((y + da->height) > Height) { return; }
   
    getPort();
    
    int len=0;
    short start_x=x;
    short end_x=x + da->width - 1;
    short start_page=y/8;
    short end_page=y/8 + da->height/8 - 1;
    
    char* buffer=getBuffer();
    if(portInfo->periphType==PERIPH_TYPE.i2c)
    {
        len = prefixI2c(buffer, start_x, end_x, start_page, end_page);
        portInfo->writeBuffer(portInfo, buffer, len); 
    }
    else if(portInfo->periphType==PERIPH_TYPE.spi)
    {
        len = prefixSpi(buffer, start_x, end_x, start_page, end_page);
        portInfo->writeBufferMode(portInfo, buffer, len, 1); 
    }
    
    if(portInfo->directMode==1)
    {
        //direct write data >> SPIBUF
        spi_Process(portInfo->portIndex, 1);         //ceka na dokonceni      
        
        //odesila data znaku primo do hw_bufferu (SPIxBUF) (SPI 8-bit mode)
        char* a;
        int* spi_buf=portInfo->directModeHwBuffer;
        for(a=da->srcStartPosition; a < da->srcAfter; a++)
        {
            *spi_buf=*a;
        }
    }
    else
    {
        //vlozi data znaku do bufferu a ten odesila
        da->srcPosition=da->srcStartPosition;
        da->eof=0;
        
        do
        {
            len=0;
            buffer=getBuffer();
            
            while(len < BUFFER_SIZE)
            {
                buffer[len]=*da->srcPosition;
                da->srcPosition++;
                len++;
            
                if(da->srcPosition == da->srcAfter)
                {
                    da->eof=1; 
                    break;
                }
            }
            
            portInfo->writeBuffer(portInfo, buffer, len);
            
        } while (da->eof == 0);
    }

    freePort();
}

static void drawPoint(short x, short y, short color)
{
    //neumoznuje zobrazit jednotlive body

}

static short textWidth(char* text, IMAGE_SRC* font)
{
    int a, w=0;
    int l=strLen(text);
    for(a=0; a<l; a++)
    {
        fontCharParam(font, text[a]);
        w+=font->width;
    }     
    
    return w;
}

static void clear(short color)
{
    fillBox(0, 0, Width-1, Height-1, color);
}

static void initDisplay(PORT_INFO* pi)
{
    portInfo=pi;
    portInfo->eventFn=&eventDC;
    
    isInitialized=0;
    
#ifndef SIMULATOR    
    dinit();
#endif    
   
    isInitialized=1;
}

static void setOrientation(char x)
{
    //nelze menit orientaci
}

static char getOrientation()
{
    return 0;
}

static void setBrightness(char val)
{
    getPort();
    char* buffer=getBuffer();  

    if(portInfo->periphType==PERIPH_TYPE.spi)
    {
        buffer[0]=0b00000001;                       //control byte, nastavi DC=0
        buffer[1]=0x81;                             //0x81 = Contrast
        buffer[2]=val;                              //param 0 ... 0xFF
        portInfo->writeBufferMode(portInfo, buffer, 3, 1);
    }
    else if(portInfo->periphType==PERIPH_TYPE.i2c)
    {
        buffer[0]=0x0;                              //control byte, nastavi DC=0
        buffer[1]=0x81;                             //0x81 = Contrast
        buffer[2]=val;                              //param 0 ... 0xFF
        portInfo->writeBuffer(portInfo, buffer, 3);        
    }

    freePort();
}

static void controlDisplay(char on, char sleep, char bl, char inv)
{
    //hodnota 0xFF = bez zmeny, 0=OFF(vypni), 1=ON(zapni)
    //on (display) nastavuje sleep mode (on=1 - SLEEP OFF, on=0 - SLEEP ON)
    //sleep - nastavi/ukonci sleep mode (sleep=1 - SLEEP ON, sleep=0 - SLEEP OFF)
    //ON i SLEEP naji stejnou fci, ale pracuji opacne
    //bl (backLight) nema zadnou fci
    
    getPort();
    char* buffer;
    
    if(on != 0xFF)
    {
        buffer=getBuffer();
        if(portInfo->periphType==PERIPH_TYPE.spi)
        {
            //SPI
            if(on==0)
            {
                //0 = display off (sleep ON)
                buffer[0]=0b00000001;                       //control byte
                buffer[1]=0xAE;                             //0xAE = sleep ON
            }
            else
            {
                //1 = display on (sleep OFF)
                buffer[0]=0b00000001;                       //control byte
                buffer[1]=0xAF;                             //0xAF = sleep OFF
            }   
            portInfo->writeBufferMode(portInfo, buffer, 2, 1);
        }
        else if(portInfo->periphType==PERIPH_TYPE.i2c)
        {
            //I2C
            if(on==0)
            {
                //0 = display off (sleep ON)
                buffer[0]=0x0;                              //control byte
                buffer[1]=0xAE;                             //0xAE = sleep ON
            }
            else
            {
                //1 = display on (sleep OFF)
                buffer[0]=0x0;                              //control byte
                buffer[1]=0xAF;                             //0xAF = sleep OFF
            }   
            portInfo->writeBuffer(portInfo, buffer, 2);            
        }
    }
    
    if(sleep != 0xFF)
    {
        buffer=getBuffer();
        if(portInfo->periphType==PERIPH_TYPE.spi)
        {      
            //SPI
            if(on==0)
            {
                //0 = sleep off
                buffer[0]=0b00000001;                       //control byte
                buffer[1]=0xAF;                             //0xAF = sleep OFF
            }
            else
            {
                //1 = sleep on
                buffer[0]=0b00000001;                       //control byte
                buffer[1]=0xAE;                             //0xAE = sleep ON
            }
            portInfo->writeBufferMode(portInfo, buffer, 2, 1);
        }
        else if(portInfo->periphType==PERIPH_TYPE.i2c)
        {
            //I2C
            if(on==0)
            {
                //0 = sleep off
                buffer[0]=0x0;                              //control byte
                buffer[1]=0xAF;                             //0xAF = sleep OFF
            }
            else
            {
                //1 = sleep on
                buffer[0]=0x0;                              //control byte
                buffer[1]=0xAE;                             //0xAE = sleep ON
            }
            portInfo->writeBuffer(portInfo, buffer, 2);            
        }        
    }
    
    if(bl != 0xFF)
    {
        //backlight nema
     }
    
    if(inv != 0xFF)
    {
        buffer=getBuffer();
        if(portInfo->periphType==PERIPH_TYPE.spi)
        {      
            //SPI        
            if(inv==0)
            {
                //0=normal, not inv
                buffer[0]=0b00000001;                       //control byte
                buffer[1]=0xA6;                             //0xA6 = inversion OFF (normal mode)
            }
            else if (inv==1)
            {
                //1=inv 
                buffer[0]=0b00000001;                       //control byte
                buffer[1]=0xA7;                             //0x21 = inversion ON (inv mode)
            }
            portInfo->writeBufferMode(portInfo, buffer, 2, 1);
        }
        else if(portInfo->periphType==PERIPH_TYPE.i2c)
        {
            //I2C
            if(inv==0)
            {
                //0=normal, not inv
                buffer[0]=0x0;                              //control byte
                buffer[1]=0xA6;                             //0xA6 = inversion OFF (normal mode)
            }
            else if (inv==1)
            {
                //1=inv 
                buffer[0]=0x0;                              //control byte
                buffer[1]=0xA7;                             //0x21 = inversion ON (inv mode)
            }
            portInfo->writeBuffer(portInfo, buffer, 2);            
        }
    }
    
    freePort();
}

static char getInitialized()
{
    return isInitialized;
}

static void print(char* t)
{
    if((displayInfo->print_y + fontSys.height) > Height)
    {
        //plna obrazovka
        clear(COLOR.Black);
        displayInfo->print_y = 0;
    }
    
    drawString(t, &fontSys, 0, displayInfo->print_y);
    displayInfo->print_y += fontSys.height;
}

static short getWidth()
{
    return Width;
}

static short getHeight()
{
    return Height;
}


static void dinit()
{
    char* buffer;
	resetDisplay();

    getPort();
    buffer=getBuffer();
    
    if(portInfo->periphType==PERIPH_TYPE.spi)
    {
        //SPI
        buffer[0]=0xAE;
        buffer[1]=0xD5;
        buffer[2]=0x80;            
        buffer[3]=0xA8;            
        buffer[4]=0x3F;            
        buffer[5]=0xD3;      //offset
        buffer[6]=0x00;           
        buffer[7]=0x00;            
        buffer[8]=0x8D;      //charge pump
        buffer[9]=0x14;
        buffer[10]=0x20;            
        buffer[11]=0x00;            
        buffer[12]=0xA1;     //set remap, 
        buffer[13]=0xC8;
        buffer[14]=0xDA;     //pin hw config
        buffer[15]=0x12;            
        buffer[16]=0x81;     //contrast,
        buffer[17]=0x7F;     //CF
        buffer[18]=0xD9;     //pre-charge
        buffer[19]=0x02;     //F1      
        buffer[20]=0xDB;     //deselect level 
        buffer[21]=0x40;
        buffer[22]=0xA4;            
        buffer[23]=0xA6;            
        buffer[24]=0xAF; 
    
        setDCPin(0);            //0=command
        portInfo->writeBuffer(portInfo, buffer, 25);
    }
    else if (portInfo->periphType==PERIPH_TYPE.i2c)
    {
        //I2C
        buffer[0]=0x00;     //vsechny nasledujici bytes jsou command
        buffer[1]=0xAE;
        buffer[2]=0xD5;
        buffer[3]=0x80;            
        buffer[4]=0xA8;            
        buffer[5]=0x3F;            
        buffer[6]=0xD3;      //offset
        buffer[7]=0x00;           
        buffer[8]=0x00;            
        buffer[9]=0x8D;      //charge pump
        buffer[10]=0x14;
        buffer[11]=0x20;            
        buffer[12]=0x00;            
        buffer[13]=0xA1;     //set remap, hw config
        buffer[14]=0xC8;
        buffer[15]=0xDA;            
        buffer[16]=0x12;            
        buffer[17]=0x81;     //contrast, pre-charge
        buffer[18]=0x7F;
        buffer[19]=0xD9;            
        buffer[20]=0xF1;            
        buffer[21]=0xDB;     //deselect level 
        buffer[22]=0x40;
        buffer[23]=0xA4;            
        buffer[24]=0xA6;            
        buffer[25]=0xAF;         
        
        portInfo->writeBuffer(portInfo, buffer, 26);
    }

    freePort();
}

static void writeChar(IMAGE_SRC* fi, char code, short x, short y)
{
    //fi=font-image struct IMAGE_SRC
    if(fontCharParam(fi, code)==0)
    {
        //chyba, neni nastaven font (fi->fileID=0)
        return;
    }
    
    //cely znak musi byt na displeji, jinak ho nezobrazuje (neumoznuje zobrazit pouze cast znaku)
    
    if(y % 8 != 0)
    {
        //y musi byt delitelne 8
        return;
    }
    if(x < 0) { return; }
    if((x + fi->width) > Width) { return; }
    if(y < 0) { return; }
    if((y + fi->height) > Height) { return; }

    /*
    //x
    char* buffer=getBuffer();
    buffer[0]=0b00000011;                       //control byte (DC=0)
    buffer[1]=0x21;
    buffer[2]=(char)x;
    buffer[3]=(char)(x + fi->width - 1);
    portInfo->writeBufferMode(portInfo, buffer, 4, 1);
    
    //y
    buffer=getBuffer();
    buffer[0]=0b00000011;                       //control byte (DC=0)
    buffer[1]=0x22;
    buffer[2]=(char)(y/8);
    buffer[3]=(char)(y/8 + fi->height/8 - 1);
    buffer[4]=0b11000000;                      //control byte (ukonci SPI mode 1, nastavi DC=1)                      //control byte
    portInfo->writeBufferMode(portInfo, buffer, 5, 1);    
    */
    
    //x
    int len=0;
    short start_x=x;
    short end_x=x + fi->width - 1;
    short start_page=y/8;
    short end_page=y/8 + fi->height/8 - 1;
    
    char* buffer=getBuffer();
    if(portInfo->periphType==PERIPH_TYPE.i2c)
    {
        len = prefixI2c(buffer, start_x, end_x, start_page, end_page);
        portInfo->writeBuffer(portInfo, buffer, len); 
    }
    else if(portInfo->periphType==PERIPH_TYPE.spi)
    {
        len = prefixSpi(buffer, start_x, end_x, start_page, end_page);
        portInfo->writeBufferMode(portInfo, buffer, len, 1); 
    }
    
    /*
    buffer[0]=0b00000011;                       //control byte (DC=0)
    buffer[1]=0x21;
    buffer[2]=(char)x;
    buffer[3]=(char)(x + fi->width - 1);
    //portInfo->writeBufferMode(portInfo, buffer, 4, 1);
    
    //y
    //buffer=getBuffer();
    buffer[4]=0b00000011;                       //control byte (DC=0)
    buffer[5]=0x22;
    buffer[6]=(char)(y/8);
    buffer[7]=(char)(y/8 + fi->height/8 - 1);
    buffer[8]=0b11000000;                      //control byte (ukonci SPI mode 1, nastavi DC=1)                      //control byte
    portInfo->writeBufferMode(portInfo, buffer, 9, 1);
    */
    
    if(portInfo->directMode==1)
    {
        //plati pouze pro SPI
        //direct write data >> SPIBUF
        spi_Process(portInfo->portIndex, 1);         //ceka na dokonceni          
        
        //odesila data znaku primo do hw_bufferu (SPIxBUF) (SPI 8-bit mode)
        char* a;
        int* spi_buf=portInfo->directModeHwBuffer;
        for(a=fi->srcStartPosition; a < fi->srcAfter; a++)
        {
            *spi_buf=*a;
        }
    }
    else
    {
        //vlozi data znaku do bufferu a ten odesila
        fi->srcPosition=fi->srcStartPosition;
        fi->eof=0;
        
        do
        {
            //int len=0;
            len=0;
            buffer=getBuffer();
            
            while(len < BUFFER_SIZE)
            {
                buffer[len]=*fi->srcPosition;
                fi->srcPosition++;
                len++;
            
                if(fi->srcPosition == fi->srcAfter)
                {
                    fi->eof=1; 
                    break;
                }
            }
            
            portInfo->writeBuffer(portInfo, buffer, len);
            //portInfo->writeBufferMode(portInfo, buffer, len, 0);
            
        } while (fi->eof == 0);
    }
    
}

static int prefixSpi(char buffer[], short start_x, short end_x, short start_page, short end_page)
{
    //SPI
    buffer[0]=0b00000011;                       //control byte (DC=0)
    buffer[1]=0x21;
    buffer[2]=(char)start_x;
    buffer[3]=(char)end_x;
    //portInfo->writeBufferMode(portInfo, buffer, 4, 1);
    
    //y
    //buffer=getBuffer();
    buffer[4]=0b00000011;                       //control byte (DC=0)
    buffer[5]=0x22;
    buffer[6]=(char)start_page;
    buffer[7]=(char)end_page;
    buffer[8]=0b11000000;                      //control byte (ukonci SPI mode 1, nastavi DC=1)                      //control byte
    //portInfo->writeBufferMode(portInfo, buffer, 9, 1);
    return 9;
}

static int prefixI2c(char buffer[], short start_x, short end_x, short start_page, short end_page)
{
    //I2C
    buffer[0]=0x80;             //command, POZOR - prvni byte prepisuje send command (napr. RESET)
    buffer[1]=0x21;             //set x    
    buffer[2]=0x80;             //command
    buffer[3]=(char)start_x;    //x start
    buffer[4]=0x80;             //command
    buffer[5]=(char)end_x;      //x end
    buffer[6]=0x80;             //command
    buffer[7]=0x22;             //set page
    buffer[8]=0x80;             //command
    buffer[9]=(char)start_page; //start page
    buffer[10]=0x80;            //commandge
    buffer[11]=(char)end_page;  //end page
    buffer[12]=0x40;            //data
    
    return 13;                  //buffer len
}

static char* getBuffer()
{
    if(pixAB==0)
    {
        pixAB=1;
        return pixelsEven;
    }
    else
    {
        pixAB=0;
        return pixelsOdd;
    }
}

static void getPort()
{
    portInfo->getPort(portInfo);                            //zajisti pouziti periph pro tento driver (je-li obsazena, vola doEvents)
    
    if(portInfo->periphType != PERIPH_TYPE.i2c)
    {
        //I2C nema CS signal
        setCSPin(0);
    }
}

static void freePort()
{
    portInfo->freePort(portInfo);                           //uvolni periph az po odvisilani dat, proto muze volat doEvents()
    
    if(portInfo->periphType != PERIPH_TYPE.i2c)
    {
        //I2C nema CS signal    
        setCSPin(1);
    }
}

static void eventDC(char x)
{
    //plati pouze pro SPI
    //b6=0/1 obsahuje hodnotu, ktera bude nastavena na DC pin 
    //b7=0 - command v b0-b5 obsahuje pocet bytes dat, ktere nasleduji (nez bude dalsi command)
    //b7=1 - command ukonci MODE 1, port pokracuje v MODE 0 (vse dalsi jsou data)
    x=x & 0b01000000; // x>>6;
    if(x==0){setDCPin(0);}
    else{setDCPin(1);}
}


static void resetDisplay()
{
    //provede UP, DOWN (reset), UP
    //je-li zapojeno vice displeju, maji spolecny RESET (pripadne i D/C signal)
    
#ifdef SW_RESET
    //SW reset
    //UP
    setResetPin(1);
    pauseEvent(50);                     //50ms
    //DOWN - probiha reset displeje
    setResetPin(0);
    pauseEvent(100);                    //100ms
    //UP
    setResetPin(1);
    pauseEvent(50);                     //50ms
#else    
    //HW RESET
    //pauseEvent(50);                     //50ms (je-li treba cekat na dokonceni hw resetu) 
#endif    
    
}

void setDCPin(char value)
{
    //nastav DC pin na value
    int* p;
    if(value==0)
    {
        p=(int*)(portInfo->dc_portBase + LAT_OFFSET + CLR_OFFSET);
    }
    else
    {
        p=(int*)(portInfo->dc_portBase + LAT_OFFSET + SET_OFFSET);
    }
    
    *p = portInfo->dc_pin;  
    
    //RB6 (pin 15)
    //_DC_pin=value;
    //LATBbits.LATB6=value;
}

static void setCSPin(char value)
{
    //nastav CS pin na value
    int* p;
    if(value==0)
    {
        p=(int*)(portInfo->cs_portBase + LAT_OFFSET + CLR_OFFSET);
    }
    else
    {
        p=(int*)(portInfo->cs_portBase + LAT_OFFSET + SET_OFFSET);
    }
    
    *p = portInfo->cs_pin;  
}

static void setResetPin(char value)
{
    int* p;
    if(value==0)
    {
        p=(int*)(portInfo->reset_portBase + LAT_OFFSET + CLR_OFFSET);
    }
    else
    {
        p=(int*)(portInfo->reset_portBase + LAT_OFFSET + SET_OFFSET);
    }
    
    *p = portInfo->reset_pin;  
}

#endif