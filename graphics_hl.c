//Graphics High Level - obsahuje graficke fce vyssi urovne
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "graphics_hl.h"
#include "def.h"
#include "graphics.h"



#ifdef DISP1306
//fce pro displej SSD1306

void disp1306_drawHBar8(int index, int nop, int x, int y, int val)
{
    //nop = pocet pozic ukazatele (std=20), nepocita prvni a posledni image [ a ]
    //val = pocet aktivnich pozic
    //width=(nop+2) x 4 (std=88px)
    
#define     BAR_ITEM_WIDTH      4
    /*
    char* left=displayGetImageData(index, 1);
    char* mid_empty=displayGetImageData(index, 2);
    char* mid_full=displayGetImageData(index, 3);
    char* right=displayGetImageData(index, 4);
    
    //char page=y/8;
    
    //vynaze prostor uvnitr
    //fillRect(index, x, y, x+7+(nop*BAR_ITEM_WIDTH), y, 0x0);
    
    int a;
    char* img;
    
    for(a=0; a<(nop+2); a++)
    {
        
        if(a==0){img=left;}                 //prvni
        else if(a==nop+1){img=right;}       //posledni
        else
        {
            if(val>=a){img=mid_full;}       //obsah plny
            else {img=mid_empty;}           //obsah prazdny
        }
        
        drawImage(index, img, x, y);
        x+=BAR_ITEM_WIDTH;
    }
    */
}

void disp1306_drawHBar16(int index, int nop, int x, int y, int val)
{
    //nop = pocet pozic ukazatele (std=20), nepocita prvni a posledni image
    //val = pocet aktivnich pozic
    //width=(nop+2) x 4 (std=88px)
    
#define     BAR_ITEM_WIDTH      4
    
    /*
      
    char* left=displayGetImageData(index, 6);
    char* mid_empty=displayGetImageData(index, 7);
    char* mid_full=displayGetImageData(index, 8);
    char* right=displayGetImageData(index, 9);
    
    //vynaze prostor uvnitr
    //fillRect(index, x, y, x+7+(nop*BAR_ITEM_WIDTH), y+15, 0x0);
    
    
    int a;
    char* img;
    
    for(a=0; a<(nop+2); a++)
    {
        
        if(a==0){img=left;}                 //prvni (levy kraj)
        else if(a==nop+1){img=right;}       //posledni (pravy kraj)
        else
        {
            if(val>=a){img=mid_full;}       //obsah, plny
            else {img=mid_empty;}           //obsah, prazdny
        }
        
        drawImage(index, img, x, y);
        x+=BAR_ITEM_WIDTH;
    }
    
    */
}

void disp1306_drawBattery(int index, int x, int y, int state)
{
    //state 0-5 = charge (obsah 0-5 obdelniku)
    //state  -1 = discharge (sipka)
    
    /*
    
    char* img=displayGetImageData(index, 10);       //battery
    drawImage(index, img, x, y);
    
    //vynaze prostor uvnitr
    fillRect(index, x+2, y+8, x+38, y+23, 0x0);
    
    
    if(state==-1)
    {
        img=displayGetImageData(index, 12);         //sipka
        drawImage(index, img, x+15, y+8);
    }
    if(state==0)
    {
        //prazdna 
    }
    else
    {
        if(state>5) { state=5; }
        int akx=x+4;
        img=displayGetImageData(index, 11);         //obdelnik
        
        while(state>0)
        {
            drawImage(index, img, akx, y+8);
            state--;
            akx+=7;
        }
    }
    
    */
    
    
}

void disp1306_drawScale8(int index, int x, int y)
{
    /*
    //x=0 (stupnice pres celou sirku displeje)
    char* left=displayGetImageData(index, 15);              //levy kraj
    char* mids=displayGetImageData(index, 16);              //stred maly
    char* midb=displayGetImageData(index, 17);              //stred velky
    char* right=displayGetImageData(index, 18);             //konec (prazdny obdelnik)
    
    //char* img;
    drawImage(index, left, 0, y);
    
    drawImage(index, mids, 4, y);
    drawImage(index, mids, 10, y);
    drawImage(index, mids, 16, y);
    drawImage(index, mids, 22, y);
    drawImage(index, midb, 28, y);
    
    drawImage(index, mids, 34, y);
    drawImage(index, mids, 40, y);
    drawImage(index, mids, 46, y);
    drawImage(index, mids, 52, y);
    drawImage(index, midb, 58, y);

    drawImage(index, mids, 64, y);
    drawImage(index, mids, 70, y);
    drawImage(index, mids, 76, y);
    drawImage(index, mids, 82, y);
    drawImage(index, midb, 88, y);
    
    drawImage(index, mids, 94, y);
    drawImage(index, mids, 100, y);
    drawImage(index, mids, 106, y);
    drawImage(index, mids, 112, y);
    drawImage(index, midb, 118, y);    
    
    drawImage(index, right, 124, y);
    
    */
  
}

#endif