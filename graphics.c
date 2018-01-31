//Graphics obsahuje zakladni graficke fce, vola primo fce driveru
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "graphics.h"


#ifdef USE_GRAPHICS

static DISPLAY* disp;
static PORT_INFO* pinfo;

static void drawCircle(short x, short y, short r, short color);
static void drawBox(short x1, short y1, short x2, short y2, short w, short color);
//static void print(char* text);

//void (*_drv)(DISPLAY*);
    //_drv=drv;
    //_drv(&g->display);
    //g->display.selectPort(pi);
    //g->initDisplay=g->display.initDisplay;

void setGraphics(GRAPHICS* g,  DISPLAY* d, PORT_INFO* pi)
{
    
    disp=d;
    pinfo=pi;
    
    d->selectPort(pinfo, disp);
    
    //fce graphics
    g->drawCircle=&drawCircle;
    g->drawBox=&drawBox;
    //g->print=&print;
    
    //fce driveru
    g->drawString=d->drawString;
    g->fillBox=d->fillBox;
    g->drawLine=d->drawLine;
    g->drawImage=d->drawImage;
    g->drawPoint=d->drawPoint;
    g->print=d->print;
    
    g->clear=d->clear;
    g->textWidth=d->textWidth;
}

static void drawCircle(short x, short y, short r, short color)
{
    //disp->draw...
    //disp->fillBox(x, y, x+200, y+100, RGB16(31,63,31));
    //disp->drawLine(x, y, x+200, y+100, 1, RGB16(0,63,0));
    //disp->drawLine(x, y+100, x+200, y, 1, RGB16(0,63,0));
    
    
}

static void drawBox(short x1, short y1, short x2, short y2, short w, short color)
{
    disp->drawLine(x1, y1, x2, y1, w, color);
    disp->drawLine(x2, y1, x2, y2, w, color);
    disp->drawLine(x2, y2, x1, y2, w, color);
    disp->drawLine(x1, y2, x1, y1, w, color);    
    
}

#endif


/*
//iface, zajisti volani spravne fce
//vola modul, ktery ovlada displej podle indexu displeje

//definuje fce pro kazdy displayIndex [0-7]
//kazdy displej muze mit vlastni driver, tzn. ze mohou byt pouzity ruzne displeje soucasne 
void (*_drawText)(int, char*, int, int, int);
const void* func_drawText[]={     &disp1306a_drawText,      NULL, NULL, NULL, NULL, NULL, NULL, NULL };

void (*_drawString)(int, char*, int, int, int);
const void* func_drawString[]={   &disp1306a_drawString,    NULL, NULL, NULL, NULL, NULL, NULL, NULL };

void (*_print)(int, char*);
const void* func_print[]={        &disp1306a_print,         NULL, NULL, NULL, NULL, NULL, NULL, NULL};

void (*_drawImage)(int, char*, int, int);
const void* func_drawImage[]={    &disp1306a_drawImage,     NULL, NULL, NULL, NULL, NULL, NULL, NULL };

void (*_fillRect)(int, int, int, int, int, int);
const void* func_fillRect[]={     &disp1306a_fillRect,     NULL, NULL, NULL, NULL, NULL, NULL, NULL };

void (*_clear)(int);
const void* func_clear[]={        &disp1306a_clear,         NULL, NULL, NULL, NULL, NULL, NULL, NULL};

void (*_setContrast)(int, int);
const void* func_setContrast[]={  &disp1306a_setContrast,   NULL, NULL, NULL, NULL, NULL, NULL, NULL};

void (*_sleep)(int);
const void* func_sleep[]={        &disp1306a_sleep,         NULL, NULL, NULL, NULL, NULL, NULL, NULL};

void (*_resume)(int);
const void* func_resume[]={       &disp1306a_resume,        NULL, NULL, NULL, NULL, NULL, NULL, NULL};

int (*_getReady)(int);
const void* func_getReady[]={     &disp1306a_getReady,      NULL, NULL, NULL, NULL, NULL, NULL, NULL};

char* (*_getImageData)(int);
const void* func_getImageData[]={ &disp1306a_getImageData,  NULL, NULL, NULL, NULL, NULL, NULL, NULL};

int (*_getWidth)(int);
const void* func_getWidth[]={     &disp1306a_getWidth,      NULL, NULL, NULL, NULL, NULL, NULL, NULL};

int (*_getHeight)(int);
const void* func_getHeight[]={    &disp1306a_getHeight,      NULL, NULL, NULL, NULL, NULL, NULL, NULL};


void drawText(int index, char* string, int col, int row, int font)
{
    //vypis v txt rezimu (sloupec, radek)
    _drawText=func_drawText[index];
    _drawText(index, string, col, row, font);
}

void drawString(int index, char* string, int x, int y, int font)
{
    //vypis na zadanou pozici x, y
    _drawString=func_drawString[index];
    _drawString(index, string, x, y, font);    
}

void printString(int index, char* string)
{
    //vypis na dalsi radek (console)
    _print=func_print[index];
    _print(index, string);
}

void drawImage(int index, char* img, int x, int y)
{
    //kresli image na pozici x, y
    _drawImage=func_drawImage[index];
    _drawImage(index, img, x, y);    
}

void fillRect(int index, int x1, int y1, int x2, int y2, int color)
{
    //vyplni zadany obdelnik barvou color
    _fillRect=func_fillRect[index];
    _fillRect(index, x1, y1, x2, y2, color);
}

void displayClear(int index)
{
    //vymaze cely displej
    _clear=func_clear[index];
    _clear(index);
}

void displaySetContrast(int index, int val)
{
    _setContrast=func_setContrast[index];
    _setContrast(index, val);
}

void displaySleep(int index)
{
    _sleep=func_sleep[index];
    _sleep(index);
}

void displayResume(int index)
{
    _resume=func_resume[index];
    _resume(index);
}

int displayGetReady(int index)
{
    _getReady=func_getReady[index];
    _getReady(index);
}

char* displayGetImageData(int index, int id)
{
    _getImageData=func_getImageData[index];
    _getImageData(id);
}

int displayGetWidth(int index)
{
    _getWidth=func_getWidth[index];
    _getWidth(index);
}

int displayGetHeight(int index)
{
    _getHeight=func_getHeight[index];
    _getHeight(index);
}

*/