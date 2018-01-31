#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "graphics.h"

//privatni struct pro dany displej, pri pouziti vice displeju se nastavi pri kazdem volani setGraphics
//static PORT_INFO portInfo; 

//definuje hodnoty krajnich bodu (plati pro 8-bit, Vcc=3.3V)
#define     MINX            10
#define     MAXX            110
#define     MINY            5
#define     MAXY            120

#define     PENIRQ_PIN      BIT4
#define     PENIRQ_PORT     PORTA           //peniqr=RA4
#define     MODE8BIT                        //8-bit, jinak 12-bit

#define     EVENT_CAPA      8
static int events[EVENT_CAPA];

//b7=1 start, b6,5,4 = 001Y, 101X, b3 1=8bit, 0=12bit, b2=0 interni ref, b1,0 = 00
#ifdef MODE8BIT
static char mx_out=0b11011000; 
static char my_out=0b10011000;
#else
static char mx_out=0b11010000; 
static char my_out=0b10010000;
#endif

static void onEvent();
static void setResult(short x, short y);
static void getPort();
static void freePort();
static void setCSPin(char value);
static char getPenIrq();

static short x_data, y_data, prew_x_data=-1, prew_y_data=-1;
//static PORT_INFO* portInfo=NULL;
//static PORT_INFO portInfo;

static char buffer[2];
void (*_event)(short, short);
static short screenX=0, screenY=0;

extern DISPLAY dispSys;
extern PORT_INFO pInfo_touchSys;

static char res[32];
static char val[16];
extern GRAPHICS graphics;
extern IMAGE_SRC f_dlg18;
static char Orientation=3;

void touchXpt2046_start()
{
    //CS=RA3, pin 8
    //portInfo.cs_portBase = PORTA_BASE;                 //CS
    //portInfo.cs_pin = BIT3;
    //portInfo.busMode = BUS_MODE._8bit;
    //portWriter_init(&portInfo, PERIPH_TYPE.spi, 1);        //pinfo obsahuje fce pro vysilani dat na pozadovany port SPI, PMP, ...
    
    pauseEvent(2000);               //ceka na init displeje
    
    //char x1=1;
    while(1)
    {
        //x1=PORTAbits.RA4;
        //vstup PENIRQ je aktivni v 0
        if(getPenIrq()==0) 
        {
            //nastal dotek (penirq=0)
            getPort();
            spi_setSpeed(pInfo_touchSys.portIndex, 0x8);
        
            //write command: read x
            buffer[0]=mx_out;
            pInfo_touchSys.writeBuffer(&pInfo_touchSys, buffer, 1);
    
            pauseEvent(10);
        
            //read data (x)
            buffer[0]=0; buffer[1]=0;
            pInfo_touchSys.exchangeBuffer(&pInfo_touchSys, buffer, buffer, 2);
#ifdef MODE8BIT   
            x_data=buffer[0];
            
#else
            x_data = ((short)buffer[0]) << 4;
            x_data |= (((short)buffer[1]) >> 4);
#endif            
    
            pauseEvent(10);
        
            //write command: read y
            buffer[0]=my_out;
            pInfo_touchSys.writeBuffer(&pInfo_touchSys, buffer, 1);
        
            pauseEvent(10);
            
            //read data (y)
            buffer[0]=0; buffer[1]=0;
            pInfo_touchSys.exchangeBuffer(&pInfo_touchSys, buffer, buffer, 2);
#ifdef MODE8BIT   
            y_data=buffer[0];
            
#else
            y_data = ((short)buffer[0]) << 4;
            y_data |= (((short)buffer[1]) >> 4);
#endif               
       
            freePort();
            spi_setSpeed(pInfo_touchSys.portIndex, 0x0);
        
            if( (x_data >= prew_x_data-5) && (x_data <= prew_x_data+5) )
            {
                if( (y_data >= prew_y_data-5) && (y_data <= prew_y_data+5) )
                {
                    setResult(x_data, y_data);
                    onEvent();
                }
            }
            prew_x_data=x_data;
            prew_y_data=y_data;
            
            
            //setResult(x_data, y_data);
            //pauseEvent(200);
        }
        else
        {
            prew_x_data=-1;
            prew_y_data=-1;
        }
        
        doEvents();
    }
    
}
int touchXpt2046_regEvent(void* fn)
{
    int a;
    for(a=0; a<EVENT_CAPA; a++)
    {
        if(events[a]==0x0)
        {
            events[a]=(int)fn;
            return 1;
        }
    }
    
    return 0;
}
void touchXpt2046_unregEvent(void* fn)
{
    int a;
    for(a=0; a<EVENT_CAPA; a++)
    {
        if(events[a]==(int)fn)
        {
            events[a]=0x0;
            //a0=adresa tabulky, a1=velikost polozky, a2=max. pocet polozek
            defragTableW(events, 4, EVENT_CAPA);
            return;
        }
    }
}
static void onEvent()
{
    int a;
    for(a=0; a<EVENT_CAPA; a++)
    {
        if(events[a]!=0x0)
        {
            _event = (void*)events[a];
            _event(screenX, screenY);
        }
        else
        {
            //konec
            return;
        }
    }    
}


static void setResult(short x, short y)
{
    
    float w=(float)dispSys.getWidth();     //  320;
    float h=(float)dispSys.getHeight();    // 240;
    char orient=dispSys.getOrientation();
    
    float f;
    
    if(orient==0)
    {
        //na vysku, normalne
        f=((float)MAXY - (float) y) / ((float)MAXY - (float)MINY) * h;
        screenY=(short)f;
        if(screenY < 0){ screenY = 0; }
        if(screenY >= h){ screenY = h-1; }
        
        f=((float)MAXX - (float) x) / ((float)MAXX - (float)MINX) * w;
        screenX= w-(short)f;
        if(screenX < 0){ screenX = 0; }
        if(screenX >= w){ screenX = w-1; }          
    }
    else if(orient==1)
    {
        //na sirku, normalne
        f=((float)MAXY - (float) y) / ((float)MAXY - (float)MINY) * w;
        screenX=(short)f;
        if(screenX < 0){ screenX = 0; }
        if(screenX >= w){ screenX = w-1; }
        
        f=((float)MAXX - (float) x) / ((float)MAXX - (float)MINX) * h;
        screenY=(short)f;
        if(screenY < 0){ screenY = 0; }
        if(screenY >= h){ screenY = h-1; }        
    }
    else if(orient==2)
    {
        //na vysku, obracene
        f=((float)MAXY - (float) y) / ((float)MAXY - (float)MINY) * h;
        screenY=h-(short)f;
        if(screenY < 0){ screenY = 0; }
        if(screenY >= h){ screenY = h-1; }
        
        f=((float)MAXX - (float) x) / ((float)MAXX - (float)MINX) * w;
        screenX=(short)f;
        if(screenX < 0){ screenX = 0; }
        if(screenX >= w){ screenX = w-1; }          
    }
    else if(orient==3)
    {
        //na sirku, obracene
        f=((float)MAXY - (float) y) / ((float)MAXY - (float)MINY) * w;
        screenX=w-(short)f;
        if(screenX < 0){ screenX = 0; }
        if(screenX >= w){ screenX = w-1; }
        
        f=((float)MAXX - (float) x) / ((float)MAXX - (float)MINX) * h;
        screenY=h-(short)f;
        if(screenY < 0){ screenY = 0; }
        if(screenY >= h){ screenY = h-1; }        
    }    
    //screenX=x;
    //screenY=y;
    
    intToChar(screenX, val, 4);
    createString(res, 32, "x= ", val, NULL);
    graphics.drawString(res, NULL, 100, 80);

    intToChar(screenY, val, 4);
    createString(res, 32, "y= ", val, NULL);
    graphics.drawString(res, NULL, 100, 110);    
}


static void getPort()
{
    //spi_Use(display->portIndex, 1, NULL, &eventDC);
    pInfo_touchSys.getPort(&pInfo_touchSys);
    
    setCSPin(0);
}

static void freePort()
{
    //spi_Free(display->portIndex, 0);
    pInfo_touchSys.freePort(&pInfo_touchSys);               //pri cekani na odvysilani muze volat doEvents()
    
    setCSPin(1);
}

static void setCSPin(char value)
{
    //nastav CS pin na value
    int* p;
    if(value==0)
    {
        p=(int*)(pInfo_touchSys.cs_portBase + LAT_OFFSET + CLR_OFFSET);
    }
    else
    {
        p=(int*)(pInfo_touchSys.cs_portBase + LAT_OFFSET + SET_OFFSET);
    }
    
    *p = pInfo_touchSys.cs_pin;  
}

static char getPenIrq()
{
    int v=PENIRQ_PORT & PENIRQ_PIN;
    if(v==0){ return 0; }
    else { return 1; }
}