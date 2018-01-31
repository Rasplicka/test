#include "xc.h"
#include "def.h"    
#include "asm_macro.S"    
   
#ifdef RTC

#ifdef PIC32MM

//MM ma RTCCON1, MZ ma RTCCON    
#define	    RTCCON	    RTCCON1
#define	    RTCCONSET	    RTCCON1SET
#define	    RTCCONCLR	    RTCCON1CLR    
    
#define	    WREN	    0x0800		//RTCCON1.b11  
    
#endif    
    
#ifdef PIC32MZ

#define	    WREN	    0x0008		//RTCCON.b3 
#define	    HAVE_DMA    
    
#endif    
    
//<editor-fold defaultstate="collapsed" desc="Popis">
//iso format
//v0=0x00HH MMSS  H:8 -bit, M:8-bit, S:8-bit
//v1=0xRRRR MMDD  R:16-bit, M:8-bit, D:8-bit  
    
//bcd format
//   des  jed  des  jed  des  jed 
//0b RRRR.RRRR-MMMM.MMMM-DDDD.DDDD-xxxx xWWW - week day
//0b HHHH.HHHH-MMMM.MMMM-SSSS.SSSS-xxxx xxxx   

//tick format 64-bit signed value
//v0 / a0 / 0+(time_ms) L val
//v1 / a1 / 4+(time_ms) H val  
    
//get_tick_from_bcd prevod BCD date-time >> tick (muze zpusibit trap)
//get_tick_from_iso prevod ISO date-time >> tick (muze zpusibit trap)
//get_iso_from_tick prevod tick date-time >> ISO (muze zpusobit trap, overflow)    
//get_bcd_from_tick prevod tick date-time >> BCD (muze zpusobit trap, overflow)    
    
//BCD format pouziva RTC modul. Po startu se nacte date-time z RTC modulu, dale
//se jiz pouziva time_ms cas v jednotkach tick (pocet ms od 1.1.0001)
//vypis casu pro uzivatele, nebo vstup od uzivatele je v ISO. Po nastaveni casu
//uzivatelem se nastavi nova hodnota i v RTC modulu
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="macro">

//<editor-fold defaultstate="collapsed" desc="_GET_HOUR_FROM_BCD">
.macro _GET_HOUR_FROM_BCD src, dest
    ext     \dest, \src, 28, 4		//desitky
    li      t9, 10	    
    mul     \dest, t9
    ext     t9, \src, 24, 4		//jednotky
    addu    \dest, t9        
.endm
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="_GET_MIN_FROM_BCD">
.macro _GET_MIN_FROM_BCD src, dest
    ext     \dest, \src, 20, 4		//desitky
    li      t9, 10	    
    mul     \dest, t9
    ext     t9, \src, 16, 4		//jednotky
    addu    \dest, t9    
.endm
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="_GET_SEC_FROM_BCD">
.macro _GET_SEC_FROM_BCD src, dest
    //sec 0-59
    ext     \dest, \src, 12, 4		//desitky
    li      t9, 10
    mul     \dest, t9
    ext     t9, \src, 8, 4		//jednotky
    addu    \dest, t9
.endm
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="_GET_Y_FROM_BCD">
.macro _GET_Y_FROM_BCD src, dest
    //do t0 year 2000-2099
    ext     \dest, \src, 28, 4		    //4 bits desitky
    li	    t9, 10
    mul     \dest, t9              
    ext     t9, \src, 24, 4		    //4 bits jednotky
    addu    \dest, t9			    //dest=rok 0-99
    
    //load stoleti z cfg
    //lw	    t9, (working_ffat)		    //adresa primarni ffat
    //lw	    t9, BOOT_CENTURY_OFFSET+4(t9)   //adresa polozky century
    //FFAT byla zmenena...
    //lw	    t9, (date_century)
    li	    t9, 2000			    //stoleti
    addu    \dest, t9			    //stoleti + RR 
.endm
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="_GET_M_FROM_BCD">
.macro _GET_M_FROM_BCD src, dest
    //month 1-12
    ext     \dest, \src, 20, 4	    //4 bits desitky
    li      t9, 10
    mul     \dest, t9
    ext     t9, \src, 16, 4         //4 bits jednotky
    addu    \dest, t9
.endm
//</editor-fold>
     
//<editor-fold defaultstate="collapsed" desc="_GET_D_FROM_BCD">
.macro _GET_D_FROM_BCD src, dest    
    //day 1-31
    ext     \dest, \src, 12, 4	    //4 bits desitky
    li      t9, 10
    mul     \dest, t9
    ext     t9, \src, 8, 4	    //4 bits jednotky
    addu    \dest, t9
.endm  
//</editor-fold>    

//<editor-fold defaultstate="collapsed" desc="_DISABLE_DMA_INT">
.macro _DISABLE_DMA_INT
//zakaze DMA (nastavi SUSPEND), zalohu DMACON do t8 
//zakaza interrupt (DI)	      , zalohu bitu do t9
//meni t8, t9, v0    
    
    mfc0    t9, _CP0_STATUS
    ext     t9, t9, 0, 1
    di

#ifdef HAVE_DMA    
    
    //zaloha DMACON
    lw      t8, (DMACON)
    //ext	    t8, t8, 12, 1 
    
    //DMA suspend 
    addiu   v0, zero, 0x1000			//set b12 (suspend)
    sw      v0, (DMACONSET)
    
1001:
    //cakat, dokud se DMA neuvolni
    lw      v0, (DMACON)
    ext     v0, v0, 11, 1			//DMA busy
    bnez    v0, 1001b
    nop    
    
#endif    
    
.endm 
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="_RESTORE_DMA_INT">
.macro _RESTORE_DMA_INT    
//restore DMA suspend, zaloha v t8 (cely DMACON)
//restore interrupt  , zaloha v t9   
   
#ifdef HAVE_DMA
    
    sw	    t8, (DMACON)
    //beqz    t8, 1001f			//skok, pokud DMA bylo SUSPEND, zaloha v t8
    //nop
    //nuluj DMA SUSPEND
    //addiu   v0, zero, 0x1000		//DMACON.12 = SUSPEND, nulovat
    //sw      v0, (DMACONCLR)
    
#endif    

//1001:    
    beqz    t9, 1002f			//skok pokud nema povolit interrupt (zaloha EI v t9)
    nop
    ei
    
1002:
.endm    
    
//</editor-fold>  
    
//<editor-fold defaultstate="collapsed" desc="_SYSKEY_WREN_SYNC">
.macro _SYSKEY_WREN_SYNC    
//nastavi RTCCON.WREN, pouze po syskey    
    
1001:
    //nastavi RTCCON.3, ceka dokud neni nastaven
    move    v0, zero
    sw      v0, (SYSKEY)
    li      v0, 0xaa996655
    sw      v0, (SYSKEY)
    li      v0, 0x556699aa
    sw      v0, (SYSKEY)
    
    //nastav RTCCON.3 (RTCWREN)
    li      v0, WREN			//SET RTCCON.WREN
    sw      v0, (RTCCONSET)
    sync

    lw      v0, (RTCCON)		//ceka na WREN=1 (obcas se musi opakovat unlock 2x)
    ext     v0, v0, 3, 1
    beqz    v0, 1001b
    nop   
    
1002:
    //cekani na sync, RTCCON.2
    lw      v0, (RTCCON)            //ceka na sync=0
    ext     v0, v0, 2, 1
    bnez    v0, 1002b
    nop
    
.endm    
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="_BIN_TO_BCD">
.macro _BIN_TO_BCD val   
//val=number 0-99, prevede na v cislice 0-9(b7-b4), 0-9(b3-b0)
    move    t9, zero
1001:
    addiu   t8, zero, 10
    slt	    t8, \val, t8
    bnez    t8, 1002f			//vysledek < 10
    nop
    
    //val>9
    addiu   t9, 1			//desitky ++
    addiu   \val, -10			//val -= 10
    nop
    b16	    1001b
    nop

1002:    
    ins	    \val, t9, 4, 4
    
.endm 
//</editor-fold>
    
    
//</editor-fold>

   
.set micromips
.section .asm_rtc, code 
   
   
//<editor-fold defaultstate="collapsed" desc="rtc_init">
.align 2    
.global rtc_init
.ent rtc_init
rtc_init:
//nastavi time_ms (long int) a date_century
//vraci v0=ok, muze pokracovat, 1=chyba, nelze spustiti OS 
    
    _PUSH   ra
    //POZOR, v prubehu fce je t8 a t9 pouzito jako zaloha stavu interrupt a dma suspend
   
    //test, zda je RTC modul zapnuty, pokud ano, nemeni nastaveni date/time
    //RTC zustava zapnuty pri RESETu i probuzeni ze sleep modu
    lw	    v0, (RTCCON)
    ext	    v0, v0, 15, 1		//RTCCON.15=ON
    bnez    v0, 1f
    nop
    
    //t9=stav CP0.STATUS.EI, disable interrupt
    //t8=stav DMA.SUSPEND
    _DISABLE_DMA_INT			//meni t8, t9, v0
    
    //nastavi RTCCON.WREN
    _SYSKEY_WREN_SYNC			//meni v0

    //set time, date
    li      v0, 0x00000000              //00:00:00
    sw      v0, (RTCTIME)
    li      v0, 0x01010102              //2001, leden, 1, den v tydnu 7=SO (1=NE, 2=PO)
    sw      v0, (RTCDATE)

    //pouze pri WREN=1
    li      v0, 0x8000			//SET ON RTCCON.15
    sw      v0, (RTCCONSET)

    li	    v0, WREN                  //CLR RTCCON.WREN
    sw      v0, (RTCCONCLR)

     //restore DMA, interrupt
    _RESTORE_DMA_INT			//ze zalohy t8, t9, meni v0
    
1:    
    //NASTAV var date_century, ktera definuje stoleti, RTC vraci pouze 2-mistny rok (napr. 16)
    //rok = date_century + RTC (2000 + 16)
    //li	    v0, 2000
    //sw	    v0, (date_century)
    //...
    
    bal	    set_tick_from_rtc
    nop

    move    v0, zero
    
    _POP    ra
    jrc     ra
    
.end rtc_init
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="set_tick_from_rtc">
.align 2     
.global set_tick_from_rtc    
.ent set_tick_from_rtc
set_tick_from_rtc:
//meni a0, a1, v0, v1, (volane fce t0-t3, t8, t9)    
    
    _PUSH   ra
    
1:
    lw      v0, (RTCCON)            //ceka na sync=0, aby neprobehlo cteni pri zmene hodnot
    ext     v1, v0, 2, 1
    bne     v1, zero, 1b
    nop
    
    lw      a1, (RTCDATE)
    lw      a0, (RTCTIME)
    
    //cte data z RTC registru a vraci hodnotu v jednotkach tick 64-bit (v0(L)-v1(H))
    bal	    get_tick_from_bcd	    //meni t0-t3, t8, t9, v0, v1
    nop
    
    //save tick v0,v1
    swp	    v0, (time_ms)
    
    _POP    ra
    jrc	    ra
    
.end set_tick_from_rtc 
//</editor-fold>
  
    
//<editor-fold defaultstate="collapsed" desc="get_time_from_rtc">
.align 2     
.global get_time_from_rtc
.ent get_time_from_rtc
get_time_from_rtc:
//vraci v0 = time
1:
    lw      v0, (RTCCON)            //ceka na sync=0, aby neprobehlo cteni pri zmene hodnot
    ext     v0, v0, 2, 1
    bnez    v0, 1b
    nop

    lw      v0, (RTCTIME)
    jrc     ra
    
.end get_time_from_rtc
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="get_date_from_rtc">
.align 2     
.global get_date_from_rtc
.ent get_date_from_rtc
get_date_from_rtc:
//vraci v0 = date
1:
    lw      v0, (RTCCON)            //ceka na sync=0
    ext     v0, v0, 2, 1
    bnez    v0, 1b
    nop

    lw      v0, (RTCDATE)
    jrc     ra
    
.end get_date_from_rtc
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="set_time_to_rtc">
.align 2     
.global set_time_to_rtc
.ent set_time_to_rtc
set_time_to_rtc:
//a0=value BCD format
//meni t8, t9, v0, v1
//neprovadi zadne testy    

//hr10  b28-b31
//hr01  b24-b27
//min10 b20-b23
//min01 b16-b19
//sec10 b12-b15
//sec01 b08-b11


    //t9=stav CP0.STATUS.EI, disable interrupt
    //t8=stav DMA.SUSPEND
    _DISABLE_DMA_INT			//meni t8, t9, v0
    
    _SYSKEY_WREN_SYNC			//meni v0 

    sw      a0, (RTCTIME)           //SAVE value
    nop

    li      v0, 0b1000              //CLR WREN
    sw      v0, (RTCCONCLR)

    //restore DMA, interrupt
    _RESTORE_DMA_INT			//ze zalohy t8, t9, meni v0


    move    v0, zero
    jrc     ra

99:
    li16    v0, 1
    jrc     ra
    
.end set_time_to_rtc
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="set_date_to_rtc">
.align 2     
.global set_date_to_rtc
.ent set_date_to_rtc
set_date_to_rtc:
//a0=value BCD format
//meni t8, t9, v0, v1
//neprovadi zadne testy    

//b28-b31 year10
//b24-b27 year01
//b20-b23 month10
//b16-b19 month01
//b12-b15 day10
//b8-b11  day01
//b0-2	  week day num      
    
    //t9=stav CP0.STATUS.EI, disable interrupt
    //t8=stav DMA.SUSPEND
    _DISABLE_DMA_INT			//meni t8, t9, v0
    
    _SYSKEY_WREN_SYNC			//meni v0 

    sw      a0, (RTCDATE)           //SAVE value
    nop

    li      v0, 0b1000              //CLR WREN
    sw      v0, (RTCCONCLR)


    //restore DMA, interrupt
    _RESTORE_DMA_INT			//ze zalohy t8, t9, meni v0
    
    jrc     ra

.end set_date_to_rtc
//</editor-fold>    
    
//<editor-fold defaultstate="collapsed" desc="set_time_date_to_rtc">
.align 2     
.global set_time_date_to_rtc
.ent set_time_date_to_rtc
set_time_date_to_rtc:
//a0=time BCD format
//a1=date BCD format    
//meni t8, t9, v0
//neprovadi zadne testy    

    //t9=stav CP0.STATUS.EI, disable interrupt
    //t8=stav DMA.SUSPEND
    _DISABLE_DMA_INT			//meni t8, t9, v0
    
    _SYSKEY_WREN_SYNC			//meni v0 

    sw	    a0, (RTCTIME)
    sw      a1, (RTCDATE)		//SAVE value
    nop

    li      v0, 0b1000			//CLR WREN
    sw      v0, (RTCCONCLR)


    //restore DMA, interrupt
    _RESTORE_DMA_INT			//ze zalohy t8, t9, meni v0
    
    jrc     ra

.end set_time_date_to_rtc
//</editor-fold> 
    
//<editor-fold defaultstate="collapsed" desc="set_bin_time">
.align 2     
.global set_bin_time   
.ent set_bin_time    
set_bin_time:
//a0=hour 0-23
//a1=min  0-59
//a2=sec  0-59
//meni t0-t9, a0-a3, v0, v1
    
    //nastavi novy cas v RTC modulu i v time_ms
    //pokud je cas chybny vyvola TRAP (exception)
    //nastaveni casu od uzivatele, napr. pres USB
    
    _PUSH   ra
    _PUSH   s0
    
    //do s0 vlozi BIN time  s0=0x00HHMMSS
    sll	    s0, a0, 16
    sll	    a1, 8
    or	    s0, a1
    or	    s0, a2
    
    //zjisti BIN date
    lwp	    a0, (time_ms)			//load tick a01
    //a01=time_ms
    bal	    get_iso_from_tick			//meni t0-t9,a0-a3,v0,v1
    nop						//vraci v1=date RRRRMMDD
    
    //a0=BIN time, a1=BIN date
    move    a0, s0
    move    a1, v1
    //a0=time, a1=date
    bal	    get_tick_from_iso			//meni t0-t9,a0-a3,v0,v1
    nop
    
    //v0, v1 = nove tick
    swp	    v0, (time_ms)
    
    //zjisti BCD time/date
    move    a0, v0
    move    a1, v1
    bal	    get_bcd_from_tick			//meni t0-t9,a0-a3,v0,v1
    nop						//vraci v0=time BCD, v1=date BCD 

    //save time/date do RTC modulu
    move    a0, v0
    move    a1, v1
    bal	    set_time_date_to_rtc		//meni t8, t9, v0
    nop
    
    _POP    s0
    _POP    ra
    jrc	    ra
    
    
.end set_bin_time    
    
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="set_bin_date">
.align 2     
.global set_bin_date   
.ent set_bin_date    
set_bin_date:
//a0=YYYY 0-65535
//a1=MM  1-12
//a2=DD  1-31
//meni t0-t9, a0-a3, v0, v1
    
    //nastavi novy cas v RTC modulu i v time_ms
    //pokud je datum chybne (napr. 29.2.2015 - neprestupny rok) vyvola TRAP (exception)
    //nastaveni data od uzivatele, napr. pres USB
    
    _PUSH   ra
    _PUSH   s0
    
    //do s0 vlozi BIN date  s0=0xRRRRMMDD
    sll	    s0, a0, 16
    sll	    a1, 8
    or	    s0, a1
    or	    s0, a2
    
    //zjisti BIN time
        
    lwp	    a0, (time_ms)			//load tick a01
    //a01=time_ms
    bal	    get_iso_from_tick			//meni t0-t9,a0-a3,v0,v1
    nop						//vraci v0=time 00HH MMSS
    
    //a0=BIN time, a1=BIN date
    move    a0, v0
    move    a1, s0
    //a0=time, a1=date
    bal	    get_tick_from_iso			//meni t0-t9,a0-a3,v0,v1
    nop
    
    //v0, v1 = nove tick
    swp	    v0, (time_ms)
    
    //zjisti BCD time/date
    move    a0, v0
    move    a1, v1
    bal	    get_bcd_from_tick			//meni t0-t9,a0-a3,v0,v1
    nop						//vraci v0=time BCD, v1=date BCD 

    //save time/date do RTC modulu
    move    a0, v0
    move    a1, v1
    bal	    set_time_date_to_rtc		//meni t8, t9, v0
    nop
    
    _POP    s0
    _POP    ra
    jrc	    ra
    
 .end set_bin_date    
    
//</editor-fold>    


//------------------------------------------------------------------------------
    
//<editor-fold defaultstate="collapsed" desc="get_tick_from_bcd">
.align 2  
.ent get_tick_from_bcd
get_tick_from_bcd:
//a0=time 0000 0000 HH10 HH01 MM10 MM01 SS10 SS01
//a1=date RR10 RR01 MM10 MM01 DD10 DD01 0000 DEN		
    
    //meni t0-t3, t8, t9, v0, v1
    
    //vraci tick v01 (time_ms)
    //testuje rozsah 0-23,0-59,1-9999,1-12,1-31
    //mimo rozsah zpusobi trap
    //netestuje platnost data (29.2, 31.4 apod...)
    
    //<editor-fold defaultstate="collapsed" desc="popis">
    //datum v roce   
    //RD2 = rozdil dnu od 1.1.2000
    //DvR = den v roce (0=1.1., 1=2.1., ...)   
    //MOD = RD2 MOD 366
    //PNR = pocet neprestupnych let    
    //DvR = MOD + PNR 
    //</editor-fold>

    _PUSH   ra
    
    _GET_Y_FROM_BCD a1, v1
    ins	    v0, v1, 16, 16
    _GET_M_FROM_BCD a1, v1
    ins	    v0, v1, 8, 8
    _GET_D_FROM_BCD a1, v1
    ins	    v0, v1, 0, 8
    move    a1, v0	
    
    //a1=iso date (0xRRRR MMDD), vraci pocet dnu od 1.1.0001
    //fce muze zpusobit trap, pokud je datum mimo rozsah (1-9999, 1-12, 1-31)
    bal	    get_days_from_iso_date 	    //meni t0-t3, t8, t9, v0, v1
    nop
    //v0=pocet dni
    
     _GET_SEC_FROM_BCD a0, t0   
     _GET_MIN_FROM_BCD a0, t1
     _GET_HOUR_FROM_BCD a0, t2
     
    //nuluj HO/LO
    mtlo    zero 
    mthi    zero 

    //sec
    addiu   v1, zero, 1000		    //SEC x 1000 - pocet jednotek v sec
    maddu   v1, t0
    //min   
    li	    v1, 60*1000			    //MIN x 60*1000 - pocet jednotek v min
    maddu   v1, t1
    //hod
    li	    v1, 60*60*1000		    //HOUR x 60*60*1000 - pocet jednotek v hodine
    maddu   v1, t2   
    //dny
    li	    v1, 60*60*24*1000		    //DAYS x 24*60*60*1000 - pocet jednotek v dni
    maddu   v1, v0
    
    mflo    v0
    mfhi    v1
   
    _POP    ra
    jrc	    ra
  
.end get_tick_from_bcd 
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="get_tick_from_iso">
.align 2     
.ent get_tick_from_iso
get_tick_from_iso:
//a0=0x00HH MMSS    
//a1=0xRRRR MMDD  
//vraci tick v0, v1    
//meni t0-t9,a0-a3,v0,v1
//muze zpusobit trap, pokud ISO datum neni spravne (napr. 30.2.1990)
//nastavi time_ms, pokud je datum platne, jinak trap   
//testuje platnost data (nejen rozsah 0-23,0-59,1-9999,1-12,1-31)   
    
    _PUSH_S03RA
     
    //a1=0xRRRR MMDD 
    bal	    get_days_from_iso_date	    //meni t0-t3, t8, t9,  v0, v1
    nop
    
    //nuluj HO/LO
    mtlo    zero 
    mthi    zero 
    
    //sekundy
    ext	    t0, a0, 0, 8
    TGEIU   t0, 60
    addiu   v1, zero, 1000		    //SEC x 1000 - pocet jednotek v sec
    maddu   v1, t0
    
    //minuty
    ext	    t0, a0, 8, 8
    TGEIU   t0, 60 
    li	    v1, 60*1000			    //MIN x 60*1000 - pocet jednotek v min
    maddu   v1, t0
    
    //hodiny
    ext	    t0, a0, 16, 8
    TGEIU   t0, 24 
    li	    v1, 60*60*1000		    //HOUR x 60*60*1000 - pocet jednotek v hodine
    maddu   v1, t0
    
    //dny
    li	    v1, 60*60*24*1000		    //DAYS x 24*60*60*1000 - pocet jednotek v dni
    maddu   v1, v0
    
    move    s0, a0
    move    s1, a1
    
    //load vysledne tick 
    mflo    s2
    mfhi    s3
    
    //test, zda zpetny prevod odpovida, pokud ne, datum je chybne
    move    a0, s2
    move    a1, s3
    //a01 = tick, zpetny prevod na date-time
    bal	    get_iso_from_tick		    //meni t0-t9,a0-a3,v0,v1
    nop
    
    //pokud neodpovida zadani, datum je neplatne (t2,t3 zaloha zadani)
    tne	    v0, s0
    tne	    v1, s1

    //vysledny tick do v0, v1
    move    v0, s2	
    move    v1, s3
    
    _POP_S03RA
    jrc	    ra

.end get_tick_from_iso   
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="get_iso_from_tick ">
.align 2     
.global get_iso_from_tick 
.ent get_iso_from_tick 
get_iso_from_tick:
//a01 = tick
//meni t0-t9,a0-a3,v0,v1
    
//vraci time v0: 0x00HH MMSS
//vraci date v1: 0xRRRR MMDD     

//pokud je tick mimo rozsah, nastane trap
  
    
    _PUSH   ra
    
    //li	    a1, 0x11EFA
    //li	    a0, 0xE44CB400
    
    //<editor-fold defaultstate="collapsed" desc="test tick range">
    //test max a1=0x11EFA a0=0xE44CB3FF	    31.12.9999 23:59:59:999
    //test min a1=0x0	  a0=0x0	    01.01.0001 00:00:00:000
    //fce pracuje v tomto rozsahu
    
    TLTI    a1, 0	    //zaporne cislo
    
    li	    a2, 0x11EFA
    slt	    a2, a1, a2
    bnez    a2, 1f	    //ok, a1 < max
    nop
    
    //a1 >= max
    li	    a2, 0x11EFB
    TGE     a1, a2	    //a1 > max, je over
    
    //a1 == max
    li	    a2, 0xE44CB400 
    TGEU    a0, a2	    //a1+a0 > max, je over
1:   
    //</editor-fold>
 
    
    //pocet tiku za den 86.400.000 = 0x5265C00
    li	    a2, 0x5265C00 
     
    //a0-1 64-bit number / a2 32-bit number (unsigned)
    //vysledek 32-bit unsigned v0, zbytek a3
    //vzhledem k omezeni tick-range je a01 vzdy kladne cislo a vysledek 32-bit kladne cislo
    
    bal	    math.div_64
    nop
    move    t0, a3			//zbytek ms v dalsim dni
   
    //<editor-fold defaultstate="collapsed" desc="popis">
    //v0=pocet dnu od 1.1.0001
    //v1=zbytek ms
    //prepocet dnu na roky
    //D=pocet dnu
    //sum = d/146097 * 400 (zbytek=Z4)
    //sum += Z4/36524 * 100 (zbytek=Z1)
    //sum += Z1/1461 * 4 (zbytek=ZP)
    //sum += ZP/365	 (zbytek=ZD)  
    //ZD=pocet dnu v poslednim roce
    //</editor-fold>
    
    //<editor-fold defaultstate="collapsed" desc="vypocet roku">
    //400-lete cykly
    li	    t9, 146097	    //pocet dnu za 400 let
    divu    t8, v0, t9	    //d/146097
    mfhi    v0		    //zbytek
    li	    t9, 400
    mul	    v1, t8, t9
    
    //t4=Z4-zbytek po 400-letych cyklech
    //100-lete cykly
    li	    t9, 36524	    //pocet dnu za 100 let	
    divu    t8, v0, t9
    mfhi    v0
    li	    t9, 100
    mul	    t8, t9
    addu    v1, t8
    
    //t4=Z1-zbytek po 100-letych cyklech
    //4-lete cykly
    li	    t9, 1461	    //pocet dnu za 4 roky
    divu    t8, v0, t9
    mfhi    v0
    li	    t9, 4
    mul	    t8, t9
    addu    v1, t8
    
    //t4=ZP-zbytek po 4-letych cyklech
    li	    t9, 365
    divu    t8, v0, t9
    mfhi    v0
    addu    v1, t8
    
    //+1 rok, zacatek kalendare je rok 1
    addiu   v1, 1
        
    //v1=letopocet(rok) 
    //v0=zbytek dnu v roce
    //t0=zbytek ms v akt. dni    
    
    //</editor-fold>

    //<editor-fold defaultstate="collapsed" desc="test leap">
    //v1 rok
    addiu   t8, zero, 4
    divu    t9, v1, t8
    mfhi    t8				    //v0 = zbytek po deleni 4
    bnez    t8, 2f  //not_leap		    //skok, neni delitelny 4
    nop
    
    //je delitelny 4 (pokud neni delitelny 100 je prestupny)
    addiu   t8, zero, 100
    divu    t9, v1, t8
    mfhi    t8				    //v0 zbytek po deleni 100
    bnez    t8, 1f  //leap			    //skok, je delitelny 4, ale ne 100, je prestupny
    nop
    
    //je deitelny 4 i 100 (pokud je delitelny 400 je prestupny)
    addiu   t8, zero, 400
    divu    t9, v1, t8
    mfhi    t8				    //zbytek po deleni 400
    bnez    t8, 2f  //not_leap		    //skok, pokud neni delitelny 400
    nop
    
    //je delityelny 400, je prestupny
   
1:  //leap  
    la	    t8, day_count_p
    nop
    b16	    3f
    nop
    
2:  //not_leap 
    //tento rok neni prestupny
    la	    t8, day_count_np

3:
    //</editor-fold>
    
    //<editor-fold defaultstate="collapsed" desc="mesic">
    //v0=pole s pocty dnu v mesicich
    //t4=zbytek dnu v roce (pocet uplynulych dni, 0 = 1.1.)
    
    addiu    t1, zero, 1		    //mesic
    
4:  //loop    
    lbu	    a2, (t8)
    slt	    t9, v0, a2
    bnez    t9, 5f
    nop
    
    //zbyva vice dnu, nez ma mesic
    subu    v0, a2			    //zbytek dnu - dnu v akt. mesici
    addiu   t1, 1			    //mesic ++
    addiu   t8, 1			    //adresa dalsiho mesice v poli
    nop
    b16	    4b
    nop
    
5:
    //t1=cislo mesice
    //v0=pocet uplynulych dnu v mesici (0=1., 1=2., ...) 
    
    sll	    v1, 16			    //rok b16-31
    ins	    v1, t1, 8, 8		    //mesic b8-15
    
    addiu   v0, v0, 1			    //cislo dne
    ins	    v1, v0, 0, 8		    //den b0-7
   
    //datum v1=0xRRRR MMDD
    //</editor-fold>
    
    //<editor-fold defaultstate="collapsed" desc="vypocet casu">
    //cas t0=pocet ms ve dni
    //do v0 vlozi 0x00HH MMSS
    
    //hodina
    li	    t1, 60*60*1000
    divu    v0, t0, t1			    //v0=hodin
    mfhi    t0				    //zbytek, minuty, sec.
    sll	    v0, 16			    //b16-23
    
    //minuta
    li	    t1, 60*1000
    divu    a2, t0, t1			    //t2=minut
    mfhi    t0				    //zbytek, sec.
    ins	    v0, a2, 8, 8		    //b8-15
    
    //sec
    addiu   t1, zero, 1000
    divu    a2, t0, t1			    //t2=sekund
    ins	    v0, a2, 0, 8		    //b0-8
    //</editor-fold>

    _POP    ra
    jrc	    ra
  
.end get_iso_from_tick 
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="get_bcd_from_tick">
.align 2     
.ent get_bcd_from_tick
get_bcd_from_tick:
//a01 = tick
//meni t0-t9,a0-a3,v0,v1    
    
    _PUSH   ra
    
    bal	    get_iso_from_tick		//meni t0-t9,a0-a3,v0,v1
    nop
    //vraci time v0: 0x00HH MMSS
    //vraci date v1: 0xRRRR MMDD 
    
    move    a0, v0
    move    a1, v1	

    //rok, vynecha stoleti
    ext	    t0, a1, 16, 16
    li	    t1, 1000
    div	    t0, t1
    mfhi    t0
    li	    t1, 100
    div	    t0, t1
    mfhi    t0				//desitky + jednotky
    
    _BIN_TO_BCD	t0	    
    ins	    v1, t0, 24, 8
    
    //mesic
    ext	    t0, a1, 8, 8
    _BIN_TO_BCD	t0
    ins	    v1, t0, 16, 8
    
    //den
    ext	    t0, a1, 0, 8
    _BIN_TO_BCD	t0
    ins	    v1, t0, 8, 8
    
    //week day=0
    ins	    v1, zero, 0, 8
    
    
    //hodina
    ext	    t0, v0, 16, 8
    _BIN_TO_BCD	t0
    ins	    v0, t0, 24, 8
    
    //minuta
    ext	    t0, v0, 8, 8
    _BIN_TO_BCD	t0
    ins	    v0, t0, 16, 8
    
    //sekunda
    ext	    t0, v0, 0, 8
    _BIN_TO_BCD	t0
    ins	    v0, t0, 8, 8
    
    ins	    v0, zero, 0, 8
    
    //v0=bcd time
    //v1=bcd date
    
    _POP    ra
    jrc	    ra
   
.end get_bcd_from_tick 
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="get_day_of_week">
.align 2     
.global	get_day_of_week    
.ent get_day_of_week
get_day_of_week:
//a0(L) a1(H) = tick
//v0 vraci cislo dne. 0=NE, 1=PO, 2=UT, 3=ST, 4=CT, 5=PA, 6=SO     

//meni v0, v1, t0, a2, a3, t8, t9    
    
    _PUSH   ra
    
    li	    a2, 0x5265C00
    
    bal	    math.div_64_sign
    nop
    
    //v0/v1 = pocet dni
    movep   a0, a1, v0, v1
    li16    a2, 7			//deleno 7
    
    bal	    math.div_64_sign
    nop
    
    //a3=zbytek dnu v tydnu
    addiu   v0, a3, 1			//+1
    
    _POP    ra
    jrc	    ra
   
.end get_day_of_week 
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="add_days">
.align 2     
.ent add_days    
add_days:
//a01=tick     signed int64
//a2=pocet dni signed int32
//v01 vraci novy tick signed int64    
//meni v0, v1
    
    mtlo    a0
    mthi    a1
    
    //pocet tiku za den 86.400.000 = 0x5265C00
    li	    v0, 0x5265C00
    move    v1, a2  
    madd    v0, v1
    
    mflo    v0
    mfhi    v1
    
    jrc	    ra
    
.end add_days 
//</editor-fold>
   
   
//<editor-fold defaultstate="collapsed" desc="get_days_from_iso_date">
.align 2     
.ent get_days_from_iso_date 
get_days_from_iso_date:
//a1=0x00RR RRMD
    
//vraci v0 pocet dnu od 1.1.0001
//vraci v1 = 0 - ok, v1 = 1 - mimo rozsah     
//meni t0-t3, t8, t9,  v0, v1  

//pokud rok, mesic, den je mimo rozsah, nastane trap exception    
    
//Rx = zadany rok
//RR = rozdil Rx-1600 //2000
//x = 0 (1600 nebyl prestupny x=0 - i kdyz by mel byt, 2000 byl prestupny x=1)    
//x += (RR/400) x (4 x 24 + 1)	//pocet prestupnych let za 400 roku
//Z4 = RR%400			//Z4=zbytek
//x += (Z4/100) x 24		//pocet prestupnych let za 100 roku
//Z1 = Z4%100			//Z1=zbytek
//x += (Z1/4)			//pocet prestupnych roku ve zbytku (0-99)    
    
//x=pocet prestupnych let
//celkem dnu Dc = RR * 365 + x
//tick = Tref + Tday * Dc + (tick tento den)
    
    
    //_GET_Y_FROM_BCD a0, t0		//do t0 rok Rx

    ext	    t0, a1, 16, 16
    //t0=rok (1-9999)
    TGEIU   t0, 10000
    TLTIU   t0, 1

    //v1=RR - rozdil Rx-1 (rok pocatku)   
    addiu   v1, t0, -1
       
    move    t2, zero			//x
    addiu   t3, zero, 400
    divu    t8, v1, t3			//t4=RR/400
    mfhi    t3				//Z4 - zbytek
    addiu   t9, zero, 97
    mul	    t9, t8			//(RR/400) x 97
    addu    t2, t9			//x+=(RR/400) x 97  - pocet prestupnych let v kazdych 400
    
    //t3=Z4, zbytek po deleni 400
    addiu   t9, zero, 100
    divu    t3, t3, t9			//Z4/100
    mfhi    t8				//Z1 - zbytek
    addiu   t9, zero, 24			
    mul	    t9, t3			//t5=(Z4/100) x 24
    addu    t2, t9			//x+=(Z4/100) x 24 - pocet prest. let za 100 let (ve zbytku)
    
    //t4=Z1 zbytek
    addiu   t9, zero, 4
    divu    t8, t9			//t4 = Z1/4 - pocet prestupnych let ve zbytku
    addu    t2, t8
    
    //t2=pocet prestupnych let celkem
    li	    t3, 365
    mul	    t3, v1			//pocet let x 365
    addu    t3, t2			// + prestupnych let (= pocet dnu navic)
    
    //t0=rok x
    //t3=pocet dnu celkem od 1.1.1600 k 1.1. roku x
    //nyni zjisti pocet dnu v roce x
    
    
    
    //_GET_M_FROM_BCD a0, t1
    ext	    t1, a1, 8, 8
    //t1=cislo mesice
    
    //test rozsahu 1-12
    TGEIU   t1, 13
    TLTIU   t1, 1
    
    //_GET_D_FROM_BCD a0, t2
    ext	    t2, a1, 0, 8
    //t2=cislo dne
    
    //test rozsahu 1-31
    TGEIU   t2, 32
    TLTIU   t2, 1
    
    //<editor-fold defaultstate="collapsed" desc="test leap, sum + mesice">
    addiu   t9, zero, 4
    divu    t9, t0, t9
    mfhi    t9				    //v0 = zbytek po deleni 4
    bnez    t9, 2f  //not_leap		    //skok, neni delitelny 4
    nop
    
    //je delitelny 4 (pokud neni delitelny 100 je prestupny)
    addiu   t9, zero, 100
    divu    t9, t0, t9
    mfhi    t9				    //v0 zbytek po deleni 100
    bnez    t9, 1f  //leap		    //skok, je delitelny 4, ale ne 100, je prestupny
    nop
    
    //je deitelny 4 i 100 (pokud je delitelny 400 je prestupny)
    addiu   t9, zero, 400
    divu    t9, t0, t9
    mfhi    t9				    //zbytek po deleni 400
    bnez    t9, 2f  //not_leap		    //skok, pokud neni delitelny 400
    nop
    
    //je delityelny 400, je prestupny
   
1:  //leap    
    //tento rok je prestupny
    la	    v0, days_by_month_p
    nop
    b16	    3f
    nop
    
2:  //not_leap 
    //tento rok neni prestupny
    //move    v0, zero
    la	    v0, days_by_month_np
    
3:
    addiu   v1, t1, -1				    //mesic -1 (dny v predchozich mesicich)
    lwxs    v0, v1(v0)				    //load pocet dnu z tabulky
    addu    t3, v0 
    
    //</editor-fold>
   
    //t3=pocet dnu v predchozich letech a mesicich
    
    //pripocitat dny v aktualnim mesici
    addiu   v0, t2, -1				    //den -1, predchozi dny
    addu    v0, t3
    
    jrc	    ra
   
.end get_days_from_iso_date  
//</editor-fold>    
    
//<editor-fold defaultstate="collapsed" desc="is_year_leap">
.align 2     
.ent is_year_leap    
is_year_leap:
//a0=year
//meni v0
//vraci v0=1 - je prestupny, v0=0 - neni prestupny
    
    li16    v0, 4
    divu    a0, v0
    mfhi    v0				    //v0 = zbytek po deleni 4
    bnez    v0, 2f	//is_not_leap	    //skok, neni delitelny 4
    nop
    
    //je delitelny 4 (pokud neni delitelny 100 je prestupny)
    addiu   v0, zero, 100
    divu    a0, v0
    mfhi    v0				    //v0 zbytek po deleni 100
    bnez    v0, 1f	//is_leap	    //skok, je delitelny 4, ale ne 100, je prestupny
    nop
    
    //je deitelny 4 i 100 (pokud je delitelny 400 je prestupny)
    addiu   v0, zero, 400
    divu    a0, v0
    mfhi    v0				    //zbytek po deleni 400
    bnez    v0, 2f	//is_not_leap	    //skok, pokud neni delitelny 400
    nop
    
    //je prestupny
   
1:  //is_leap    
    li16    v0, 1
    jrc	    ra
    
2:  //is_not_leap    
    move    v0, zero
    jrc	    ra
    
.end is_year_leap
//</editor-fold>


day_count_p:		.byte 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 
day_count_np:	.byte 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31    
    
days_by_month_np:  .word 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334   //pocet dnu v roce na konci mesice neprestupny rok
days_by_month_p:   .word 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335   //pocet dnu v roce na konci mesice prestupny rok
    
#endif	//RTC