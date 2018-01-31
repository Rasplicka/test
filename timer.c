#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "timer.h"
#include "def.h"

//vars


void timer_Init()
{
    //
    //IEC0.11, IFS0.11
    
    //timer_ms (uint) cita pocet ms od zapnuti
    //pouziva se k mereni doby udalosti
    //day_ms (uint) pocita pocet ms od zacatku dne
    //pri zmene casu RTC se zmeni, po startu se nastavi podle RTC
    //pouziva se ke spusteni udalosti v urcitt cas
    //spolu s RTC DD/MM/YYYY urcuje den/cas
    
    timer_ms=0;
    
#ifdef RTC    
    //povoli RTC modul, nastavi 1/1/2001
    rtc_Init();
    //nastav day_ms, podle aktualniho casu
    //setDayMsFromRtc();
    
#endif    
    T1CON=0x0;                  //off
    TMR1=0x0;
    T1CON=0x0202;               //set (off)
    
    IFS0bits.T1IF=0;            //IFlag=0
    IEC0bits.T1IE=1;            //IEnable=1
    
    IPC2bits.T1IP=1;            //priority
    IPC2bits.T1IS=0;            //subpriority
    
    //1/10 sec, 10x za sekundu 
    PR1=3200;
    
    //ON, LPRC (32kHz 1:1), idle-run, 
    T1CONSET=0x8000;
}

