#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "asm.h"
#include "usb_mm.h"
#include "graphics.h"
#include "font_fixed1306.h"
//#include "disp9341a.h"
//#include "font1306.h"
//#include "fontConsolas.h"
//#include "font_twcen.h"
//#include "font_ygm.h"

#ifdef PIC32MZ

#define     _LED_INV_REG    LATHINV
#define     _LED_INV_VAL    0b010

#endif

#ifdef PIC32MM0064

#define     _LED_INV_REG    LATBINV
#define     _LED_INV_VAL    0b0                 //nic

#endif

#ifdef PIC32MM0256

#define     _LED_INV_REG    LATBINV
#define     _LED_INV_VAL    0b10000000          //RB7                

#endif

   

//char txt[]="0123";
char txt1[]="\x9Físlo \x7F\x43 áéíóúý";
char txt2[]="15:39  +-.,";
char txt3[]="delsi text cislo 3 nejdelsi";

//extern DISPLAY dispA;
//extern PORT_INFO pinfo;
extern GRAPHICS graphics;
extern DISPLAY dispSys;
/*
IMAGE_SRC f_dlg18;
IMAGE_SRC f_consolas16;
IMAGE_SRC f_consolas20;
IMAGE_SRC f_consolas28;
IMAGE_SRC f_consolas36;
IMAGE_SRC f_arial18;
IMAGE_SRC f_gothic78;
IMAGE_SRC f_ygm16;
IMAGE_SRC f_ygm20;
IMAGE_SRC f_ygm28;
IMAGE_SRC f_ygm36;
IMAGE_SRC f_ygm46;
IMAGE_SRC f_twcen18;
IMAGE_SRC f_twcen22;
IMAGE_SRC f_twcen28;
IMAGE_SRC f_twcen36;
IMAGE_SRC f_twcen46;
IMAGE_SRC f_twcen80;
*/


short   x=0;
short   y=0;
//char buffer[1024];

char hour=10;
char min=0;   

void m2_start();
//char stack2[8];
void dispText(DISPLAY* d, char* txt);


void blick2();
void usb_reaction(char command[], char data_out[]);
int usb_value=0;

void writePrint();

short cmap[16]; 
void loadColorMap(short cm[]);

void initGraph();
void testGraphText();
void testGraphLine();
void testTouch(short x, short y);
extern int touchXpt2046_regEvent(void* fn);
void testDisp1306();

void m2_start()
{
    testDisp1306();
    
    //blick2();
    //setFontSrc(&font_dlg18, &f_dlg18);
    
    //initGraph();
    //testGraphText();
    //testGraphLine();
    //void* tt=(void*)&testTouch;
    
    //IMAGE_SRC f;
    
    //setFontSrc(&font_fixed32x, &f);
    //f.foreColor=RGB16(31, 63, 31);
    //graphics.drawString("12345678", &f, 0, 0);
    //graphics.drawString("ABC def", &f, 0, 32);
    
    //IMAGE_SRC fs;
    //setFontSrc(&font_fixed48x, &fs);
    //graphics.drawString("12:55", &fs, 8, 8);
    
    
    //IMAGE_SRC im;
    //setImageSrc(&image1306x, &im);
    //graphics.drawImage(&im, 0, 0);
    
    //graphics.drawString("ABCDEFGHIJKLMNOP", &fs, 0, 40);
    //graphics.drawString("abcdefghijklmnop", &fs, 0, 48);
    
    //graphics.drawString("ABCdefghijklmnop", NULL, 0, 0);
    //graphics.drawString("1234567890-+*/WW", NULL, 0, 16);
    //graphics.drawString(txt1, NULL, 0, 32);
    
    
    //touchXpt2046_regEvent(&testTouch);
    
    //dispText(&dispA);
    
    /*
    //test image 
    setImageSrc(&image52, &img2); //image51
    //if(img2.format==0x4){ setImageColorMap(&img2, cmap); }
    //else if(img2.format==0x1){ img2.foreColor=0xFFFF; img2.bgColor=0x0; }
    //img2.start_x=-1;
    //int l=imageToBuffer2(&img2, buffer, 1024, 1);
    
    int a, b;
    
    while(1)
    {
        for(a=0; a<200; a+=20)
        {
            for(b=0; b<140; b+=10)
            {
                dispA.clear(COLOR.Black);
                dispA.drawImage(&img2, a, b);
                pauseEvent(3000);                
            }
        }
    }
    */
    
    



    
    /*
    setImageSrc(image03, &imgx);
    dispA.drawImage(&imgx, 10, 10);
    */


    //dispA.drawString(txt, &fo, 0, 90);
    //dispA.drawString(txt, &fo, 0, 160);    
    
    /*
    char th[2];
    char tm[2];
    char res[5];
    
    while(1)
    {
        pauseEvent(1000);
        min++;
        if(min==60){min=0;}
        
        byteToChar(hour, th, 2);
        byteToChar(min, tm, 2);
        createString(res, 6, th, ":", tm, NULL);
        
        dispA.drawString(res, &fo, 30, 50);
        doEvents();
    }
    */
    
    while(USB_isConnected()==0)
    {
        doEvents();  
    }
    
    //reakce na pozadavek z hosta
    char buffer_cmd[64];                   //prijem command z hosta (max. 16B)
    char data_out[64];                     //data pro hosta
    char ep=1;

    while(1)
    {
        USB_rxData(ep, buffer_cmd, 64);         //Pripravi se na data z hosta
        while(USB_isRxProgress(ep)==1) 
        { 
            //cekani na data
            doEvents(); 
        }
 
        //ok, data prijata
        //usb_value=buffer_cmd[0]*256 + buffer_cmd[1];
        //dekoduje command 
        usb_reaction(buffer_cmd, data_out);
    
        //odeslani dat do hosta
        //while(USB_isTxProgress(ep)==1)
        //{ 
        //    doEvents(); 
        //}
        //USB_txData(ep, data_out, 64);
    }
    
    
    while(1)
    {
        //do LATxINV zapise 1 na prislusnou pozici
        _LED_INV_REG = _LED_INV_VAL;
        
        int a, b=0;
        for(a=0; a<100000; a++)
        {
            b++;
            if(a % 1000 == 0)
            {
                doEvents();
            }
        }
    }
}

/*
void initGraph()
{
    //definuje jinou colorMap (system obsahuje std colorMap (B/W), ktera se automaticky nastavi v IMAGE_SRC fci setFontSrc/setImageSrc)
    loadColorMap(cmap);

    //<editor-fold defaultstate="collapsed" desc="PortWriter, pin set">
    //pin configuration
    //pinfo.portIndex = 1;                            //SPI[1]=SPI2
    pinfo.cs_portBase = PORTC_BASE; //CS
    pinfo.cs_pin = BIT2;
    pinfo.reset_portBase = PORTC_BASE; //RESET
    pinfo.reset_pin = BIT1;
    pinfo.dc_portBase = PORTC_BASE; //DC
    pinfo.dc_pin = BIT0;
    pinfo.busMode = BUS_MODE._8bit;
    pinfo.directMode = 1;

    portWriter_init(&pinfo, PERIPH_TYPE.spi, 1); //pinfo obsahuje fce pro vysilani dat na pozadovany port SPI, PMP, ...
    // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="inicializace displeje">
    disp9341_driver(&dispA);                    //dispA obsahuje fce modulu disp9341
    dispA.initDisplay(&pinfo);                  //init
    // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="aktivuje Graphics">
    //dispA je asociovan s Graphics, vystup Graphics jde na dispA
    setGraphics(&gr, &dispA, &pinfo); 
    // </editor-fold>

    //cls
    gr.clear(COLOR.Black);    
}
*/

void testGraphText()
{
    
    /*
    //font 4-bit, pouziva std color map
    setFontSrc(&font_twcen18, &f_twcen18);
    gr.drawString(txt1, &f_twcen18, 0, 0);  
    
    setFontSrc(&font_ygm16, &f_ygm16);
    gr.drawString(txt1, &f_ygm16, 0, 20); 
    
    setFontSrc(&font_twcen22, &f_twcen22);
    gr.drawString(txt1, &f_twcen22, 0, 40); 
    
    setFontSrc(&font_ygm20, &f_ygm20);
    gr.drawString(txt1, &f_ygm20, 0, 70);    

    //font 4-bit, nastavi jinou colorMap
    setFontSrc(&font_twcen28, &f_twcen28);
    setImageColorMap(&f_twcen28, cmap);
    gr.drawString(txt1, &f_twcen28, 0, 110); 
    
    setFontSrc(&font_ygm28, &f_ygm28);
    setImageColorMap(&f_ygm28, cmap);
    gr.drawString(txt1, &f_ygm28, 0, 150);   
    

    setFontSrc(&font_twcen80, &f_twcen80);
    gr.drawString(txt2, &f_twcen80, 0, 150); 
    
    */
    

    //setFontSrc(&font_twcen46, &f_twcen46);
    //gr.drawString(txt1, &f_twcen46, 0, 120);     
    
    
    //setFontSrc(&font_dlg18, &f_dlg18);
    //gr.drawString(txt1, &f_dlg18, 0, 0);

    //setFontSrc(&font_ygm28, &f_ygm28);
    //gr.drawString(txt1, &f_ygm28, 0, 0);    
    
    //setFontSrc(&font_ygm36, &f_ygm36);
    //gr.drawString(txt1, &f_ygm36, 0, 30);    
    
    //setFontSrc(&font_ygm46, &f_ygm46);
    //gr.drawString(txt1, &f_ygm46, 0, 70);      
    
    //setFontSrc(&font_consolas16, &f_consolas16);
    //f_consolas16.foreColor=COLOR.GreenYellow;
    //gr.drawString(txt1, &f_consolas16, 0, 30);
    
    //setFontSrc(&font_consolas20, &f_consolas20);
    //f_consolas20.foreColor=COLOR.GreenYellow;
    //gr.drawString(txt1, &f_consolas20, 0, 45);
    
    //setFontSrc(&font_consolas28, &f_consolas28);
    //f_consolas28.foreColor=COLOR.GreenYellow;
    //gr.drawString(txt1, &f_consolas28, 0, 70);
    
    //setFontSrc(&font_consolas36, &f_consolas36);
    //f_consolas36.foreColor=COLOR.Pink;
    //gr.drawString(txt1, &f_consolas36, 0, 100);    
    
    //setFontSrc(&font_arial18, &f_arial18);
    //f_arial18.foreColor=COLOR.Cyan;
    //gr.drawString(txt1, &f_arial18, 0, 150); 
    
    //setFontSrc(&font_ygm16, &f_ygm16);
    //f_ygm16.foreColor=COLOR.Cyan;
    //gr.drawString(txt1, &f_ygm16, 0, 150); 
    
    //setFontSrc(&font_ygm20, &f_ygm20);
    //f_ygm16.foreColor=COLOR.Cyan;
    //gr.drawString(txt1, &f_ygm20, 0, 180);
    
    //setFontSrc(&font_gothic78, &f_gothic78);
    //f_gothic78.foreColor=COLOR.Yellow;
    //gr.drawString(txt2, &f_gothic78, 0, 130);     
    
}

void testTouch(short x, short y)
{
    
    graphics.drawPoint(x, y, RGB16(0,63,31));
    
}

void testGraphLine()
{
    graphics.drawBox(0, 100, 50, 120, 1, COLOR.Blue);
    graphics.drawBox(300, 200, 319, 239, 1, COLOR.Magenta);
    
    graphics.drawLine(0, 100, 50, 120, 1, COLOR.Cyan);
    graphics.drawLine(0, 120, 50, 100, 1, COLOR.Cyan);
            
    //short color=colWHITE;
    graphics.drawLine(0, 0, 319, 0, 1, COLOR.Orange);
    graphics.drawLine(319, 0, 319, 239, 1, COLOR.Yellow);
    graphics.drawLine(319, 239, 0, 239, 1, COLOR.Orange);
    graphics.drawLine(0, 239, 0, 0, 1, COLOR.Yellow);    
}

void writePrint()
{
    int a;
    char res[32];
    char tm[4];
    for(a=0; a<100; a++)
    {
        byteToChar(a, tm, 1);
        createString(res, 32, "Výpis textu \x9Físlo : ", tm, NULL);
        graphics.print(res);
        pauseEvent(1000);
    }
}

void dispText(DISPLAY* d, char* txt)
{
    //char txt2[] = {"áéíóúý ÁÉÍÓÚÝ"};
    //char txt[] = {"Kratky text"};
    //char txt[] = {'J','i','\xFE','í',' ','R','a','\xE7','p','l','i','\x9F','k','a','ý','á','í','é','?','?',' '};
    //char txt[] = {'\xA6','l','u','\x9C','o','u','\x9F','k','\xEC',' ','k','\xDE','\xE5',',',' ','\x9F','e','p','i','c','e'};

    //int a;
    //short y=0, x=0;
    
    //FONT_SRC fo;
    //setFontSrc(fontConsolas12x20, &fo);
    //fo.foreColor=colWHITE;    //RGB16(31, 63, 31);
    //fo.bgColor=colBLACK;    //RGB16(0, 0, 0);
    
    //d->drawString(txt, &fo, x, y);
    //y+=fo.height;
    //x+=d->textWidth(txt, &fo);
    
    /*
    FONT_SRC foa;
    setFontSrc(fontArial18, &foa);
    foa.foreColor=colWHITE; //RGB16(31, 63, 31);
    foa.bgColor=colBLACK;   //RGB16(0, 0, 0);    
    
    d->drawString(txt, &foa, 0, y);
    y+=foa.height;
    */
    
    
    //short color=colWHITE;
    //d->drawLine(0, 0, 319, 0, color);
    //d->drawLine(319, 0, 319, 239, color);
    //d->drawLine(319, 239, 0, 239, color);
    //d->drawLine(0, 239, 0, 0, color);
    
}

void usb_reaction(char command[], char data_out[])
{
    /*
    if(command[0]==1)
    {
        dispText(&dispA, txt1);
    }
    else if(command[0]==2)
    {
        dispText(&dispA, txt2);
    }
    else if(command[0]==3)
    {
        dispText(&dispA, txt3);
    }
    else if(command[0]==4)
    {
        x=command[1];
        y=command[2];
    }
    else if(command[0]==10)
    {
        dispA.setOrientation(command[1]);
    }
    else if(command[0]==11)
    {
        
        dispA.setBrightness(command[1]);
        
        int a=command[1];
        a=a*10;
        //pwm_setPower(3, a);
    }
    else if(command[0]==20)
    {
        char t[2];
        t[0]=command[1];
        t[1]=0;
        dispText(&dispA, t);
    }
    else if(command[0]==96)
    {
        //COLOR color;
        short* c=(short*)&COLOR;
        short col=c[command[1]];
       
        dispA.fillBox(command[2], command[3], command[4], command[5], col);
    }    
    else if(command[0]==97)
    {
        short* c=(short*)&COLOR;
        short col=c[command[1]];
        dispA.clear(col);
    }
    else if(command[0]==98)
    {
        dispA.clear(COLOR.White);
    }
    else if(command[0]==99)
    {
        dispA.clear(COLOR.Black);
    }
    else if(command[0]==100)
    {
        dispA.controlDisplay(command[1], command[2], command[3], command[4]);
    }
    */

}
void blick2()
{
    //RB7 

    LATBINV=0b10000000;
    
    while(1)
    {
        int a, b;
        for(a=0; a<300000; a++)
        {
            b=a+1;
            if(a % 1000 == 0)
            {
                doEvents();
            }
        }
        
        LATBINV=0b10000000;
    }
    
}

void loadColorMap(short cm[])
{

    //green
    cm[0]=RGB16(0, 0, 0);
    cm[1]=RGB16(0, 4, 0);
    cm[2]=RGB16(0, 8, 0);
    cm[3]=RGB16(0, 12, 0);

    cm[4]=RGB16(0, 16, 0);
    cm[5]=RGB16(0, 20, 0);
    cm[6]=RGB16(0, 24, 0);
    cm[7]=RGB16(0, 28, 0);

    cm[8]=RGB16(0, 32, 0);
    cm[9]=RGB16(0, 36, 0);
    cm[10]=RGB16(0, 40, 0);
    cm[11]=RGB16(0, 44, 0);

    cm[12]=RGB16(0, 48, 0);
    cm[13]=RGB16(0, 52, 0);
    cm[14]=RGB16(0, 56, 0);
    cm[15]=RGB16(0, 60, 0); 
    
    /*
    //White 
    cm[0]=RGB16(0, 0, 0);
    cm[1]=RGB16(2, 4, 2);
    cm[2]=RGB16(4, 8, 4);
    cm[3]=RGB16(6, 12, 6);

    cm[4]=RGB16(8, 16, 8);
    cm[5]=RGB16(10, 20, 10);
    cm[6]=RGB16(12, 24, 12);
    cm[7]=RGB16(14, 28, 14);

    cm[8]=RGB16(16, 32, 16);
    cm[9]=RGB16(18, 36, 18);
    cm[10]=RGB16(20, 40, 20);
    cm[11]=RGB16(22, 44, 22);

    cm[12]=RGB16(24, 48, 24);
    cm[13]=RGB16(26, 52, 26);
    cm[14]=RGB16(28, 56, 28);
    cm[15]=RGB16(30, 60, 30);    
    */
}

void testDisp1306()
{
    //test ssd1306 SPI, def.h definuje USE_DISP1306, SYSDISPLAY_1306SPI, USE_SYSTEMFONT_FIXEDx
    //modul globals inicializuje driver, portWriter a fontSys
    //x v nazvu fontu znamena, ze se nejedna o std. font (fileID=0x3)
    
    
    //pouzije v globals definovany fontSys
    //char txt[]="\x9Físlo \x7F\x43 áéíóúý";
    //graphics.drawString("ABCdefghijklmnop", NULL, 0, 0);
    //graphics.drawString("1234567890-+*/WW", NULL, 0, 16);
    //graphics.drawString(txt, NULL, 0, 32);

    //velky font 32x16
    
    //IMAGE_SRC f;
    //setFontSrc(&font_rock32x, &f);
    //f.foreColor=RGB16(31, 63, 31);
    //graphics.drawString("12345678", &f, 0, 0);
    //graphics.drawString("ABC def", &f, 0, 32);
    
    //maximalni font 48x22 (max. 5 znaku za radek)
    //IMAGE_SRC fs;
    //setFontSrc(&font_rock48x, &fs);
    //fs.foreColor=RGB16(31, 63, 31);
    //graphics.drawString("12:55", &fs, 8, 8);
    
    //nejmensi font 8x8
    //IMAGE_SRC fs;
    //setFontSrc(&font_fixed8x, &fs);
    //fs.foreColor=RGB16(31, 63, 31);
    //graphics.drawString("ABCDEFGHIJKLMNOP", &fs, 0, 40);
    //graphics.drawString("abcdefghijklmnop", &fs, 0, 48);
    
    //image 128x64
    IMAGE_SRC im;
    setImageSrc(&image1306x, &im);
    graphics.drawImage(&im, 0, 0);
    
    //regulace neni temer poznat
    //dispSys.setBrightness(255);
    
    //inverze
    //dispSys.controlDisplay(0xff, 0xff, 0xff, 1);
    
    //fill box
    //graphics.fillBox(0, 0, 128, 64, 0b10101010);
    
    //print, musi byt nastaven systemovy font
    //graphics.print("Radek c.1");
    //graphics.print("Radek c.2");
    //graphics.print("Radek c.3");
    //graphics.print("Radek c.4");
    //graphics.print("Radek c.5");
    //graphics.print("Radek c.6");
    graphics.print(txt1);
}