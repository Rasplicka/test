#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "asm.h"

#include "graphics.h"
//#include "fontConsolas.h"

#ifdef PIC32MZ

#define     _LED_INV_REG    LATHINV
#define     _LED_INV_VAL    0b100

#endif

#ifdef PIC32MM0064

#define     _LED_INV_REG    LATBINV
#define     _LED_INV_VAL    0b0                 //nic

#endif

#ifdef PIC32MM0256

#define     _LED_INV_REG    LATCINV
#define     _LED_INV_VAL    0b1000              //RC3                

#endif

char txt[]="Tady je nejaky text";

//extern DISPLAY dispA;
//PORT_INFO pinfo;
//IMAGE_SRC img3;
//IMAGE_SRC font;

void m3_start();
static void blick3();
void drawLinex(short x1, short y1, short x2, short y2, short color);
void linex(short x, short y, short color);

void m3_start()
{
    blick3();
    /*
    pinfo.cs_portBase = PORTC_BASE;                 //CS
    pinfo.cs_pin = BIT2;
    pinfo.reset_portBase = PORTC_BASE;              //RESET
    pinfo.reset_pin = BIT1;
    pinfo.dc_portBase = PORTC_BASE;                 //DC
    pinfo.dc_pin = BIT0;
    pinfo.busMode = BUS_MODE._8bit;
    pinfo.directMode=1;
    
    portWriter_init(&pinfo, PERIPH_TYPE.spi, 1);        //pinfo obsahuje fce pro vysilani dat na pozadovany port SPI, PMP, ...
    */
    
    //disp9341_set(&dispB);                               //dispA obsahuje fce modulu disp9341
    //nastavi port pro dispA
    //pokud se pouziva pouze jeden display, staci nastavit jednou
    //pokud je vice displeju, vola se pred kazdym pouzitim displeje
    //dispB.selectPort(&pinfo);
    //dispA.initDisplay();
    //dispA.clear(COLOR.Black);
    
    while(1)
    {
         doEvents();
    }
    
    //setImageSrc(&image51, &img3); //image51
    //while(dispA.getInitialized()==0){ doEvents(); }
    
    //setFontSrc(&font_dlg18, &font);
    
    while(1)
    {
        short col;
        int c, r;
        /*
        //test box 
        for(r=0; r<240; r+=25)
        {
            for(c=0; c<320; c+=32)
            {
                col=(short)random(65535);
                dispA.drawBox(c, r, c+32, r+24, col);
                pauseEvent(200);
            }
        }
        */
        
        /*
        //test line
        dispA.clear(COLOR.Black);
        col=0xFFFF;
        for(r=1; r<15; r++)
        {
           dispA.drawLine(5, r*15, 320, r*15, r, col);
        }
        pauseEvent(4000); 
        */
        
        //test point
        //dispA.clear(COLOR.Black);
        
        col=RGB16(0 ,63, 0);
        
        /*
        drawLinex(0, 0, 10, 100, col);
        
        col=RGB16(31 ,0, 0);
        drawLinex(0, 200, 50, 10, col);
        
        col=RGB16(0 ,0, 31);
        drawLinex(80, 5, 10, 220, col);
        
        col=RGB16(31 ,63, 31);
        drawLinex(200, 230, 180, 20, col);
        */
        
        for(c=10; c<100; c++)
        {
            //drawLinex(c, 0, c+4, 239, col);
            //linex
            //dispA.drawLine(0, c-20, 319, c+15, 1, col);
        }
        
        //dispA.drawString(txt3, &img2, 100, a);
        //dispA.drawString(txt, &font, 20, 200);
        pauseEvent(10000);
        
        
        /*
        
        //for(r=0; r<240; r+=2)
        //{
        r=120;
        
            for(c=0; c<320; c++)
            {
                //col=(short)c;
                r+=(short)random(20)-10;
                if(r<10){r=10;}
                if(r>230){r=230;}
                dispA.drawPoint(c, r, col);
            }
        //}
        
        pauseEvent(200);
        */
    }
    
    while(1)
    {
        //dispA.selectPort(&pinfo);
        //dispA.clear(COLOR.Black);
        //dispA.drawImage(&img3, 40, 20);
        //pauseEvent(4000);   
    }
    
}

static void blick3()
{
    while(1)
    {
        //do LATxINV zapise 1 na prislusnou pozici
        _LED_INV_REG = _LED_INV_VAL;
        
        int a, b=0;
        for(a=0; a<150000; a++)
        {
            b++;
            if(a % 1000 == 0)
            {
                doEvents();
            }
        }
    }    
}

void drawLinex(short x1, short y1, short x2, short y2, short color)
{
    /*
    short dx=x2-x1, dy=y2-y1;
    if(dx<0) {dx*=-1;}
    if(dy<0) {dy*=-1;}

    if(dx>=dy)
    {    
        //x1 musi byt mensi, nez x2
        if(x1 > x2) 
        { 
            short x=x1; x1=x2, x2=x; 
            short y=y1; y1=y2, y2=y;
        }
    
        short rx=x2-x1;
        short ry=y2-y1;
        float r=(float)ry/(float)rx;
    
        short a; 
        short resy;

        for(a=x1; a<=x2; a++)
        {
            resy=((float)a)*r + y1;
            dispA.drawPoint(a, resy, color);
        }
    }
    else
    {
        //y1 musi byt mensi, nez y2
        if(y1 > y2) 
        { 
            short y=y1; y1=y2, y2=y;
            short x=x1; x1=x2, x2=x; 
        }        
        
        short rx=x2-x1;
        short ry=y2-y1;
        float r=(float)rx/(float)ry;
    
        short a; 
        short resx;        
        
        for(a=y1; a<=y2; a++)
        {
            resx=((float)a)*r + x1;
            dispA.drawPoint(resx, a, color);
        }        
        
    }
*/
    
    
}