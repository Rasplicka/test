#include "xc.h"
#include "def.h"    
#include "asm_macro.S"    
   
#ifdef RTC

    
//<editor-fold defaultstate="collapsed" desc="Popis">
//date a time se cte z RTC modulu
//presny cas (ms 32-bit) od zacatku dne je v promene day_ms, kterou incrementuje timer1
//day_ms se synchronizuje s RTC kazdou minutu (alarm interrupt)    
//doba spusteni je v timer_ms (32-bit)

//bcd format
//   des  jed  des  jed  des  jed
//0b RRRR.RRRR-MMMM.MMMM-DDDD.DDDD-xxxx xWWW - week day
//0b HHHH.HHHH-MMMM.MMMM-SSSS.SSSS-xxxx xxxx      
    
//</editor-fold>

.set micromips
.section .asm_rtc, code 
    
//spolecne macro pro MM i MZ    

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

    
    
#ifdef PIC32MM 
    
#define	    WRLOCK	    0x0800		//RTCCON1.b11  
#define     TIME_SYNC	    0x0004
#define	    ALARM_SYNC	    0x0002
#define	    RTCCON1_SET	    0xC3FF0000		//ALARM enable, ON, ... 
#define	    RTCCON1_ON	    0x00008000	    
#define	    RTCCON2_SET	    0x3E800001		//clock=Low Power RC oscilator (32 kHz)  
    
//<editor-fold defaultstate="collapsed" desc="_DISABLE_INT">
.macro _DISABLE_INT
//zakaza interrupt (DI)	      , zalohu bitu do t9
//meni t8, t9, v0    
    
    mfc0    t9, _CP0_STATUS
    ext     t9, t9, 0, 1
    di
  
.endm 
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="_RESTORE_INT">
.macro _RESTORE_INT    
//restore interrupt  , zaloha v t9   
   
    beqz    t9, 1001f			//skok pokud nema povolit interrupt (zaloha EI v t9)
    nop
    ei
    
1001:
    
.endm    
    
//</editor-fold>  
    
//<editor-fold defaultstate="collapsed" desc="_SYSKEY_WRLOCK">
.macro _SYSKEY_WRLOCK   
//nuluj RTCCON1.WRLock, pouze po syskey    
    
1001:
    //nastavi RTCCON.3, ceka dokud neni nastaven
    move    v0, zero
    sw      v0, (SYSKEY)
    li      v0, 0xaa996655
    sw      v0, (SYSKEY)
    li      v0, 0x556699aa
    sw      v0, (SYSKEY)
    
    //nuluj WRLOCK
    li      v0, WRLOCK			//WRLOCK=0
    sw      v0, (RTCCON1CLR)
    sync

    lw      v0, (RTCCON1)		//ceka na WRLOCK=0 (obcas se musi opakovat unlock 2x)
    li	    v1, WRLOCK
    and	    v0, v1
    bnez    v0, 1001b			//skok, WRLOCK neni 0
    nop
 
.endm    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="_DATETIME_SYNC">

.macro _DATETIME_SYNC
//ceka, dokud RTCSTAT.b2 != 0 (pri 1 probiha zmena RTC)
    
1001:    
    lw	    v0, (RTCSTAT)
    li	    v1, TIME_SYNC
    and	    v0, v1
    bnez    v0, 1001b
    nop
    
.endm  
    
//</editor-fold>
  
//<editor-fold defaultstate="collapsed" desc="_ALARM_SYNC">

.macro _ALARM_SYNC    
//ceka, dokud RTCSTAT.b1 != 0 (pri 1 probiha zmena RTC)
    
1001:    
    lw	    v0, (RTCSTAT)
    li	    v1, ALARM_SYNC
    and	    v0, v1
    bnez    v0, 1001b
    nop
    
.endm  
    
//</editor-fold>

   
//<editor-fold defaultstate="collapsed" desc="rtc_Init">
.align 2    
.global rtc_Init
.ent rtc_Init
rtc_Init:
//nastavi time_ms (long int) a date_century
//vraci v0=ok, muze pokracovat, 1=chyba, nelze spustiti OS 

    _PUSH   ra
    //POZOR, v prubehu fce je t8 a t9 pouzito jako zaloha stavu interrupt a dma suspend
   
    
    
    //test, zda je RTC modul zapnuty, pokud ano, nemeni nastaveni date/time
    //RTC zustava zapnuty pri RESETu i probuzeni ze sleep modu
    lw	    v0, (RTCCON1)
    ext	    v0, v0, 15, 1		//RTCCON.15=ON
    bnez    v0, 1f
    nop
    
#ifdef PIC32MM0064
    //prior=1, sub=0
    li	    v0, 0x1F0000
    sw	    v0, (IPC3CLR)
    li	    v0, 0x40000
    sw	    v0, (IPC3SET)
    
    //IE, flag
    li	    v0, (1 << 14)
    sw	    v0, (IFS0CLR)
    sw	    v0, (IEC0SET)
#endif
    
#ifdef PIC32MM0256
    //prior=1, sub=0
    li	    v0, 0b11111
    sw	    v0, (IPC3CLR)
    li	    v0, 0b10000
    sw	    v0, (IPC8SET)
    
    //IE, flag
    li	    v0, (1 << 0)
    sw	    v0, (IFS1CLR)
    sw	    v0, (IEC1SET)
#endif    
    
    
    
    //t9=stav CP0.STATUS.EI, disable interrupt
    //nastav WRLock=0
    _DISABLE_INT			//meni t8, t9, v0
    
    //nuluj RTCCON1.WRLOCK
    _SYSKEY_WRLOCK			//meni v0, v1
    _DATETIME_SYNC			//nemi v0, v1
    _ALARM_SYNC				//meni v0, v1

    //set time, date
    li      v0, 0x00000000              //00:00:00
    sw      v0, (RTCTIME)
    sw	    v0, (ALMTIME)
    sw	    v0, (ALMDATE)
    li      v0, 0x01010102              //2001, leden, 1, den v tydnu 7=SO (1=NE, 2=PO)
    sw      v0, (RTCDATE)

    //input clock LPRC
    li	    v0, RTCCON2_SET		//LPRC
    sw	    v0, (RTCCON2)
    
    //ON=1
    li      v0, RTCCON1_SET		//ON=1, WRLock=1
    sw      v0, (RTCCON1)
    
    li	    v0, RTCCON1_ON
    sw	    v0, (RTCCON1SET)

    //WRLOCK=1
    li	    v0, WRLOCK			//set RTCCON1.WRLOCK
    sw      v0, (RTCCON1SET)

     //restore interrupt
    _RESTORE_INT			//ze zalohy t9, meni v0
    
1:    
    //NASTAV var date_century, ktera definuje stoleti, RTC vraci pouze 2-mistny rok (napr. 16)
    //rok = date_century + RTC (2000 + 16)
    //li	    v0, 2000
    //sw	    v0, (date_century)
    //...
    
    bal	    rtc_setDayMs
    nop
    
    _POP    ra
    jrc     ra
    
.end rtc_Init
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="rtc_setDayMs">
.align 2     
.global rtc_setDayMs    
.ent rtc_setDayMs
rtc_setDayMs:
//meni a0, a1, v0, v1, (volane fce t0-t3, t8, t9)    
    
    //RTC sec * 1000
    //+ RTC min * 60
    //+ RTC hod * 60
    
    lw      a0, (RTCTIME)
    //src, dest
    _GET_SEC_FROM_BCD a0, t0		//meni t9
    _GET_MIN_FROM_BCD a0, t1		//meni t9
    _GET_HOUR_FROM_BCD a0, t2		//meni t9
    
    li	    t9, 1000
    li	    t8, 60
    mtlo    zero
    mthi    zero
    
    maddu   t0, t9	//   sec  * 1000
    maddu   t1, t8	// + min  * 60
    maddu   t2, t8	// + hour * 60
    
    mflo    v0
    sw	    v0, (day_ms)
    
    jrc	    ra
    
.end rtc_setDayMs 
//</editor-fold>
  
//<editor-fold defaultstate="collapsed" desc="rtc_getTime">

.align 2    
.global rtc_getTime
.ent rtc_getTime
rtc_getTime:
    
    _DATETIME_SYNC			//nemi v0, v1
    lw	    v0, (RTCTIME)
    
    jrc	    ra
    
.end rtc_getTime 
    
//</editor-fold>
  
//<editor-fold defaultstate="collapsed" desc="rtc_getDate">
    
.align 2    
.global rtc_getDate
.ent rtc_getDate
rtc_getDate:
    
    _DATETIME_SYNC			//nemi v0, v1
    lw	    v0, (RTCDATE)
    
    jrc	    ra
    
.end rtc_getDate  
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="rtc_getHalfSecond">

.align 2
.global rtc_getHalfSecond
.ent rtc_getHalfSecond
rtc_getHalfSecond:    
    
    //RTCSTAT.b0
    lw	    v0, (RTCSTAT)
    ext	    v0, v0, 0, 1
    
    jrc	    ra
    
.end rtc_getHalfSecond  
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="rtc_setTime">
    
.align 2
.global rtc_setTime    
.ent rtc_setTime
rtc_setTime:
//a0=BCD time
    
//bcd format
//   des  jed  des  jed  des  jed
//0b RRRR.RRRR-MMMM.MMMM-DDDD.DDDD-xxxx xWWW - week day
//0b HHHH.HHHH-MMMM.MMMM-SSSS.SSSS-xxxx xxxx    
    
    _PUSH   ra

    
    
    //POZOR, v dalsim prubehu fce je t8 a t9 pouzito jako zaloha stavu interrupt a dma s
    _DISABLE_INT			//meni t8, t9, v0
    
    //nuluj RTCCON1.WRLOCK
    _SYSKEY_WRLOCK			//meni v0, v1
    _DATETIME_SYNC			//nemi v0, v1
    _ALARM_SYNC				//meni v0, v1

    //set time, date
    sw      a0, (RTCTIME)
    
    //WRLOCK=1
    li	    v0, WRLOCK			//set RTCCON1.WRLOCK
    sw      v0, (RTCCON1SET)
    
    //restore interrupt
    _RESTORE_INT			//ze zalohy t9, meni v0    
    
    bal	    rtc_setDayMs
    nop
    
    _POP    ra
    jrc     ra
    
.end rtc_setTime 
    
//</editor-fold>
   
//<editor-fold defaultstate="collapsed" desc="rtc_setDate">
    
.align 2
.global rtc_setDate    
.ent rtc_setDate
rtc_setDate:
//a0=BCD time
    
//bcd format
//   des  jed  des  jed  des  jed
//0b RRRR.RRRR-MMMM.MMMM-DDDD.DDDD-xxxx xWWW - week day
//0b HHHH.HHHH-MMMM.MMMM-SSSS.SSSS-xxxx xxxx    
   
    
    //POZOR, v dalsim prubehu fce je t8 a t9 pouzito jako zaloha stavu interrupt a dma s
    _DISABLE_INT			//meni t8, t9, v0
    
    //nuluj RTCCON1.WRLOCK
    _SYSKEY_WRLOCK			//meni v0, v1
    _DATETIME_SYNC			//nemi v0, v1
    _ALARM_SYNC				//meni v0, v1

    //set time, date
    sw      a0, (RTCDATE)
    
    //WRLOCK=1
    li	    v0, WRLOCK			//set RTCCON1.WRLOCK
    sw      v0, (RTCCON1SET)
    
    //restore interrupt
    _RESTORE_INT			//ze zalohy t9, meni v0    
    
    jrc     ra
    
.end rtc_setDate 
    
//</editor-fold>    
    
   
#endif	    //MM    
    
#endif	    //RTC