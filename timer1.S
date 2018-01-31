#include "xc.h"
#include "asm_macro.S"
#include "def.h"  
    
    //timer_ms (uint) cita pocet ms od zapnuti
    //pouziva se k mereni doby udalosti
    //day_ms (uint) pocita pocet ms od zacatku dne
    //pri zmene casu RTC se zmeni, po startu se nastavi podle RTC
    //pouziva se ke spusteni udalosti v urcity cas
    //spolu s RTC DD/MM/YYYY urcuje den/cas    

#ifdef PIC32MM0064
    
    
#endif    
    
.section .data    
.global timer_ms
timer_ms:	    .word	0x0			    //pocet ms od zapnuti
.global day_ms   
day_ms:		    .word	0x0			    //pocet ms v tomto dni, od 0:00:00 hod    
    
.global timer1_events    
timer1_events:	    .space	TIMER1_EVENT_CAPA * TIMER1_EVENT_ISIZE
    
    
.set micromips
.section .asm_timer1, code 
    
//<editor-fold defaultstate="collapsed" desc="timer_Init">
    
.align 2
.global timer1_Init
.ent timer1_Init
timer1_Init:
    //timer1 je pripojen k LPRC oscilatoru, ten ma vzdy 32kHz, nezavisla na PBCLK 
    //stejne jako RTCC
    
    _PUSH   ra
    sw	    zero, (timer_ms)
    
#ifdef RTC    
    //povoli RTC modul, nastavi 1/1/2001
    //nastav day_ms, podle aktualniho casu

    jal	    rtc_Init
    nop

#endif     

    //OFF, setting
    sw	    zero, (T1CON)	    //off (ON=0)
    sw	    zero, (TMR1)
    li	    t9, 0x0202
    sw	    t9, (T1CON)		    //set (ON=0), LPRC (32kHz 1:1), idle-run, 
    
#ifdef PIC32MM0064    //IPC/IEC/IFS
    
    //prior 1, subprior 0
    li	    t9, (1<<26)
    sw	    t9, (IPC2SET) 
    
    //IFS=0, IEC=1
    li	    t9, (1<<11)
    sw	    t9, (IFS0CLR)
    sw	    t9, (IEC0SET)
#endif
    
#ifdef PIC32MM0256    //IPC/IEC/IFS
    
    //prior 1, subprior 0
    li	    t9, (1<<12)
    sw	    t9, (IPC4SET) 
    
    //IFS=0, IEC=1
    li	    t9, (1<<17)
    sw	    t9, (IFS0CLR)
    sw	    t9, (IEC0SET)
#endif    
    
    //1/100 sec, 100x za sekundu 
    li	    t9, 320
    sw	    t9, (PR1)
    
    li	    t9, (1<<15)
    sw	    t9, (T1CONSET)
    
    _POP    ra
    jrc	    ra
    
    //
    
    //IEC0.11, IFS0.11
    
    //timer_ms (uint) cita pocet ms od zapnuti
    //pouziva se k mereni doby udalosti
    //day_ms (uint) pocita pocet ms od zacatku dne
    //pri zmene casu RTC se zmeni, po startu se nastavi podle RTC
    //pouziva se ke spusteni udalosti v urcitt cas
    //spolu s RTC DD/MM/YYYY urcuje den/cas
    
    /*
    timer_ms=0;
    
#ifdef RTC    
    //povoli RTC modul, nastavi 1/1/2001
    rtc_Init();
    //nastav day_ms, podle aktualniho casu
    //setDayMsFromRtc();
    
#endif    
    T1CON=0x0;                  //off
    TMR1=0x0;
    T1CON=0x0202;               
    
    IFS0bits.T1IF=0;            //IFlag=0
    IEC0bits.T1IE=1;            //IEnable=1
    
    IPC2bits.T1IP=1;            //priority
    IPC2bits.T1IS=0;            //subpriority
    
    //1/10 sec, 10x za sekundu 
    PR1=3200;
    
    //ON, LPRC (32kHz 1:1), idle-run, 
    T1CONSET=0x8000;
    */
    
.end timer1_Init   
    
//</editor-fold>
 
//<editor-fold defaultstate="collapsed" desc="timer1_regEventInterval">
    
.align 2
.global timer1_regEventInterval
.ent timer1_regEventInterval
timer1_regEventInterval:
//a0=adresa status byte
//a1=interval ms 
//vraci v0=1 OK, v0=0 chyba, plna tabulka  
    
//po uplynuti casu nastavi status byte=1, nulovat ho pak musi app 
//udalost nastava periodicky, pro zruseni ji musi app unreg
    
    la	    v1, (timer1_events)
    addiu   v0, zero, TIMER1_EVENT_CAPA

1:    
    lw	    t9, (v1)
    bnez    t9, 2f
    nop
    
    //volna polozka
    sw	    a0, (v1)				//adresa status byte
    sw	    a1, 4(v1)				//default value
    sw	    a1, 8(v1)				//value
   
    li16    v0, 1				//vraci OK
    jrc	    ra
    
2:
    addiu   v1, TIMER1_EVENT_ISIZE
    addiu   v0, -1
    bnez    v0, 1b
    nop
    
    //v0=0, vraci chybu
    jrc	    ra
    
.end timer1_regEventInterval    
    
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="timer1_regEventDelay">
    
.align 2
.global timer1_regEventDelay
.ent timer1_regEventDelay
timer1_regEventDelay:
//a0=adresa status byte
//a1=delay ms 
//vraci v0=1 OK, v0=0 chyba, plna tabulka 
    
//po uplynuti casu nastavi status byte=1, nulovat ho pak musi app  
//udalost nastane pouze jednou, app ji pak nemusi unreg (provede se automaticky)    
    
    
    la	    v1, (timer1_events)
    addiu   v0, zero, TIMER1_EVENT_CAPA

1:    
    lw	    t9, (v1)
    bnez    t9, 2f
    nop
    
    //volna polozka
    sw	    a0, (v1)				//adresa status byte
    sw	    zero, 4(v1)				//default value = 0, neni repeat
    sw	    a1, 8(v1)				//value
   
    li16    v0, 1				//vraci OK
    jrc	    ra
    
2:
    addiu   v1, TIMER1_EVENT_ISIZE
    addiu   v0, -1
    bnez    v0, 1b
    nop
    
    //v0=0, vraci chybu
    jrc	    ra     
    
    
.end timer1_regEventDelay
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="timer1_unregEvent">
    
.align 2
.global timer1_unregEvent
.ent timer1_unregEvent
timer1_unregEvent:
//a0=adresa status byte    
    
    _PUSH ra
    
    addiu   v0, zero, TIMER1_EVENT_CAPA
    la	    v1, timer1_events
1:    
    lw	    t9, (v1)
    bne	    t9, a0, 3f
    nop
    
    //nasel polozku
    sw	    zero, (v1)	    //vymazat
    
    la	    a0, timer1_events
    li16    a1, TIMER1_EVENT_ISIZE
    addiu   a2, zero, TIMER1_EVENT_CAPA
    //a0=adresa tabulky, a1=velikost polozky, a2=max. pocet polozek
    la	    t9, (defragTableW)
    jalr    t9
    //bal	    defragTableW
    nop
    
2:
    _POP ra
    jrc	    ra
    nop
    
3:
    addiu   v0, -1	    //count--
    addiu   v1, 4	    //item++ (event table)
    bnez    v0, 1b	    //skok na dalsi item
    nop
    
    //nenasel polozku
    _POP    ra
    jrc	    ra
    nop   
    
.end timer1_unregEvent  
    
//</editor-fold>
   