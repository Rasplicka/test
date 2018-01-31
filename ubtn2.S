#include "xc.h"
#include "asm_macro.S"
#include "def.h"       

    
//<editor-fold defaultstate="collapsed" desc="popis">
    
//pracuje v modu 0, 1, nebo 2
//mod 0
//je-li btn stisknuto delsi dobu, odesle jeden event UBTN_DOWN_LONG
//a po uvolneni UBTN_UP_LONG    
//app reaguje na tyto udalosti, tzn. ze reaguje az na UP (nebo UBTN_DOWN_LONG, nikoliv na DOWN)    
//app rozlisuje mezi kratkym a dlouhym stisknutim    
//mod 1
//je-li btn stisknuto delsi dobu, odesilaji se periodicky UBTN_REPEAT (prvni pauza je delsi)    
//app reaguje na UBTN_DOWN a UBTN_REPEAT    
//app nerozlisuje kratke a dlouhe stisknuti  
    
//mod 0+1 pri kratkem stisknuti odesila UBTN_DOWN a UBTN_UP v obou modech    
    
//mod 2
//mod pro vstupni pin. Odesila pouze UBTN_DOWN a UBTN_UP pri zmene stavu    
    
//param0 je UBTN_UP/UBTN_DOWN/UBTN_LONG_DOWN/UBTN_LONG_UP/UBTN_REPEAT
//param1 je value (= kod tlacitka, viz. buttons_table)    
    
//stav je potvrzeny, pokud je prectena 10x stejna hodnota    
    
//max. pocet buttons je omezen na BUTTONS_TABLE_CAPA (mozno zmenit)   
//kazde tlacitko se definuje v buttons_table
    
//app, ktera chce prijimat events tohoto modulu, musi volat ubtn_regEvent(&fn)
//pak je fn volana pri kazde event
//void fn(int event, int button_value, char cnt)    
//cnt je poradi REPEAT, nebo DOWN_LONG (0-0xFF)
    
//fce ubtn_start nastavuje TRIS(input), ANLSEL(digital), CNPU(pull-up povoli), CNPD(pull-down zakaze)    
//tlacitko se pripojuje mezi pin a zem    
    
    
    
//POZOR po dlouhem stisku (odesila DOWN_LONG) a uvolneni neodesila UP, ale pouze UP_LONG    
//stisk                                                                                                  uvolneni    
//DOWN...................................................................................................
//..............REPEAT(0)....REPEAT(1)....REPEAT(2)....REPEAT(3)....REPEAT(4)....REPEAT(5)....REPEAT(6)..
//...........................DOWN_LONG(0)...........................DOWN_LONG(1).........................UP_LONG    

//stisk                    uvolneni    
//DOWN..........REPEAT(0)..UP
    
//stisk   uvolneni    
//DOWN....UP       
    
//</editor-fold>
    
#ifdef UBTN
    
#define		LONG_DOWN_MS	    3000			//po 3000 ms detekuje dlouhe stisknuti (mod 0)
#define		REPEAT_FIRST_MS	    2000			//po 2000 ms poprve odesila REPEAT event (mod 0)
#define		REPEAT_NEXT_MS	    200				//po 200 ms druhe a dalsi REPEAT event (mod 0)
    
  
//RAM vars    
.section .data
    
#define		ITEM_SIZE	    12				//polozka v items: rep_time[4B], long_time[4B], rep_count[1B], long_count[1B], up_state[1B], down_state[1B]
#define		BUTTONS_TABLE_ISIZE 8				//velikost polozky v buttons_table    
#define		BUTTONS_TABLE_CAPA  8  
    
#define		REP_TIME_OFF	    0
#define		LONG_TIME_OFF	    4
#define		REP_COUNT_OFF	    8
#define		LONG_COUNT_OFF	    9
#define		UP_STATE_OFF	    10
#define		DOWN_STATE_OFF	    11    
    
items:		.space		    BUTTONS_TABLE_CAPA * ITEM_SIZE   
    
//events table    
#define		EVENTS_CAPA	    8				//kapacita events tabulky
events:	.space	EVENTS_CAPA * 4					//8 polozek (word) pro adresy fci volanych pri event
    
    
.set micromips   
.section .ubtn, code
.align 2
//STATE 0xF0=UP, 0x0F=DOWN (mode 0,2), 0x0E=DOWN (mode 1)
    
//<editor-fold defaultstate="collapsed" desc="buttons_table">
    
buttons_table:
//tabulka obsahuje popis vsech tlacitek, je zakoncena 0x0
//!!!POZOR v tabulce nesmi byt vice polozek, nez BUTTONS_TABLE_CAPA
    
#ifdef PIC32MM
    
//button 0 (A.2-pin 9)   
.word	PORTA_BASE	//base addr portu 
.byte	2		//bit_index (A0)
.byte	0		//mode 0=down, repeat, long down, up, long up, 1=down, up
.hword	32		//value, ktera bude odeslana v event, param1
    
//button 1 (A.3-pin 10)   
.word	PORTA_BASE	//base addr portu     
.byte	3		//bit_index (A1)
.byte	0		//mode 0=long up/down, 1=repeat down
.hword	33		//value, ktera bude odeslana v event, param1    
    
#endif    
    
//konec tabulky    
.word	0x0		//konec tabulky
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="ubtn_start">
.align 2    
.global ubtn_start
.ent ubtn_start    
ubtn_start:
//nastavi bit v portu odpovidajicim tlacitku: TRIS=1(input), ANSEL=0(digital), CNPU=1(pull-up rezistor)    
//tlacitko se zapoji mezi pin a GND, stisknuti = LOG 0
//STATE nastavi na UP   
//definice tlacitek je v buttons_table    

    //nastav TRIS bit=1 (input), ANSEL bit=0 digital
    la	    t0, buttons_table
    la	    s0, items		    //RAM tabulka items
    addiu   s1, zero, 0xF	    //button UP state (nastavi u vsech tlacitek)
    
    li16    v1, 1
    
1:  //cyklus pro vsechny tlacitka
    //nastav default hodnoty, nastav port
    lw	    t1, (t0)		//t1=base addr registru portu
    beqz    t1, 9f		//skok, konec buttons_table
    nop
    
    lbu	    t9, 4(t0)		//load bit_index
    sllv    v0, v1, t9		//v0 ma nastaveny odpovidajici bit
    
    //TRISbit=1
    sw32    v0, (TRIS_OFFSET + SET_OFFSET)(t1)
    //ANSELbit=0 
    sw32    v0, (ANSEL_OFFSET + CLR_OFFSET)(t1)
    //LAT
    //sw	    v0, (LAT_OFFSET + SET_OFFSET)(t1)
    //PORT
    //sw	    v0, (PORT_OFFSET + SET_OFFSET)(t1)
    //CNPU=1 (pull-up)
    sw32    v0, (CNPU_OFFSET + SET_OFFSET)(t1)
    //CNPD=0 (pull-down)
    sw32    v0, (CNPD_OFFSET + CLR_OFFSET)(t1)    
    
    sb      s1, UP_STATE_OFF(s0)	    //UP_state=0xF  (button=UP)
    sb	    zero, DOWN_STATE_OFF(s0)	    //DOWN_state=0x0
    
    
    //next button
    addiu   t0, BUTTONS_TABLE_ISIZE
    addiu   s0, ITEM_SIZE	
	
    nop
    b16	    1b
    nop
9:    
    
    //doEvents
    la	    a0, ubtn_go
    bal	    doEventsL
    nop
    
.end ubtn_start  
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="ubtn_go">
.align 2    
.ent ubtn_go
ubtn_go:
//odesila event
//1. btn_down,      param0=1, param1=buttons_table[6]    - okamzik stisknuti
//2. btn_up,        param0=2, param1=buttons_table[6]	 - uvolneni po beznem stisknuti   
//3. btn_down_long, param0=3, param1=buttons_table[6]    - trva down delsi dobu (pouze jeden event, nikoliv opakovane) mod 0
//4. btn_up_long,   param0=4, param1=buttons_table[6]    - uvolneni po dlouhem stisknuti mod 0
//5. btn_repeat     param0=5, param1=buttons_table[6]    - opakovane down mod 1
    
    la	    s0, items
    addiu   s1, zero, 0xF	    //hodnota potvrzeny stav
    la	    s2, buttons_table

next:
    lw	    t1, (s2)		    //t1=base addr registru portu
    beqz    t1, end		    //skok konec
    nop    
    
    //zjisti akt. stav pinu do v0
    lw32    v0, PORT_OFFSET(t1)		    //load port
    lbu	    t9, 4(s2)			    //load bit_index
    srlv    v0, v0, t9			    //cteny bit do b0
    ext	    v0, v0, 0, 1		    //v0=stav pinu
    
    beqz    v0, pin_down		    //skok, pin=0-down
    nop    
    
    //pin je up
    lbu	    t9, UP_STATE_OFF(s0)
    bne	    t9, s1, up_test_change	    //skok, potencialni zmena stavu
    nop
    
    //trva stav UP
    sb	    zero, DOWN_STATE_OFF(s0)	    //down_state=0
    nop
    b16	    loop
    nop
    
up_test_change:
    //mozna zmena stavu na UP
    //t9=up_state
    addiu   t9, 1
    sb	    t9, UP_STATE_OFF(s0)	    //save novy up_state
    bne	    t9, s1, loop		    //skok, zmena stavu zatim nenastala (up_state != 0xF)
    nop
    
    //zmena stavu na UP
    sb	    zero, DOWN_STATE_OFF(s0)	    //down_state=0
    lbu	    t9, LONG_COUNT_OFF(s0)
    beqz    t9, send_up			    //skok, pri down nenastalo long_down, bude odesilat up
    nop
    
    //send long_up
    li16    a0, UBTN_UP_LONG		    //btn up long
    lhu	    a1, 6(s2)			    //value z buttons_table
    move    a2, zero
    bal	    runEvent
    nop
    
    nop
    b16	    loop
    nop
    
send_up:
    //send up
    li16    a0, UBTN_UP			    //btn up long
    lhu	    a1, 6(s2)			    //value z buttons_table
    move    a2, zero
    bal	    runEvent
    nop
    
    nop
    b16	    loop
    nop    

    
pin_down:
    lbu	    t9, DOWN_STATE_OFF(s0)
    bne	    t9, s1, down_test_change	    //skok, potencialni zmena stavu
    nop    
    
    //trva stav down
    sb	    zero, UP_STATE_OFF(s0)	    //up_state=0
    nop    
    
    lbu	    t9, 5(s2)			    //load mode
    bnez    t9, loop			    //skok, mode=1
    nop
    
    lw32    a0, REP_TIME_OFF(s0)	    //load repeatTime
    
    //a0=time (vraci rozdil od now [ms])
    bal	    compareTimerMs		    //meni t8, t9
    nop
    
    lbu	    s3, REP_COUNT_OFF(s0)	    //load repeatCount
    beqz    s3, rep_0			    //skok, repeatCount=0 	
    nop
    
    //repeatCount > 0
    addiu   v0, (-REPEAT_NEXT_MS)
    bltz    v0, test_long		    //skok, nevyprsel cas
    nop
    
send_rep:    
    //cas vyprsel, odeslat repeat
    li16    a0, UBTN_REPEAT		    //btn repeat
    lhu	    a1, 6(s2)			    //value z buttons_table    
    move    a2, s3			    //repeat count
    bal	    runEvent
    nop
    
    lw      t9, (timer_ms)
    sw32    t9, REP_TIME_OFF(s0)	    //nastav now repeatTime
    addiu   s3, 1			    //repeatC++
    andi    s3, 0xFF			    //nuluj b8-b31	
    beqz    s3, test_long		    //skok, pokud doslo k preteceni repeatC
    nop
    sb	    s3, REP_COUNT_OFF(s0)	    //uloz repeatCount
    
    nop
    b16	    test_long
    nop
    
    
rep_0:
    //repeat_count=0, v0=pocet ms
    addiu   v0, (-REPEAT_FIRST_MS)
    bgez    v0, send_rep		    //skok, cas vyprsel
    nop    
    
test_long:    
    //test cas long
    lw32    a0, LONG_TIME_OFF(s0)	    //load longTime
    
    //a0=time (vraci rozdil od now [ms])
    bal	    compareTimerMs		    //meni t8, t9
    nop
    
    addiu   v0, (-LONG_DOWN_MS)
    bltz    v0, loop			    //skok, cas jeste nevyprsel
    nop
    
    //cas vyprsel
    lbu	    s3, LONG_COUNT_OFF(s0)	    //load longCount
    li16    a0, UBTN_DOWN_LONG		    //btn long down
    lhu	    a1, 6(s2)			    //value z buttons_table    
    move    a2, s3			    //longCount
    bal	    runEvent
    nop    
    
    lw      t9, (timer_ms)
    sw32    t9, LONG_TIME_OFF(s0)	    //nastav now longTime
    addiu   s3, 1			    //longC++
    andi    s3, 0xFF			    //nuluj b8-b31
    beqz    s3, loop			    //skok, pokud doslo k preteceni longC
    nop
    sb	    s3, LONG_COUNT_OFF(s0)	    //uloz repeatCount
    
    nop
    b16	    loop
    nop    
    
down_test_change:
    //mozna zmena stavu na down
    //t9=down_state
    addiu   t9, 1
    sb	    t9, DOWN_STATE_OFF(s0)	    //save novy down_state
    bne	    t9, s1, loop		    //skok, zmena stavu zatim nenastala (down_state != 0xF)
    nop    

    //nastala zmena na down
    sb	    zero, UP_STATE_OFF(s0)	    //up_state=0
    lw      t9, (timer_ms)		    //now	
    sw32    t9, REP_TIME_OFF(s0)	    //reteatTime=now
    sw32    t9, LONG_TIME_OFF(s0)	    //longTime=now
    sb	    zero, REP_COUNT_OFF(s0)	    //nuluj repeatCount
    sb	    zero, LONG_COUNT_OFF(s0)	    //nuluj longCount
    
    //send down
    li16    a0, UBTN_DOWN		    //btn down
    lhu	    a1, 6(s2)			    //value z buttons_table
    move    a2, zero
    bal	    runEvent
    nop
    
    //pokracuje loop
   
loop:    
    //next button
    addiu   s0, ITEM_SIZE		    //items ++
    addiu   s2, BUTTONS_TABLE_ISIZE	    //button_table ++
    
    nop
    b16	    next
    nop
    
end:    
    //doEvents
    la	    a0, ubtn_go
    bal	    doEventsL
    nop    

.end ubtn_go  
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="ubtn_regEvent">
    
.align 2
.global	ubtn_regEvent
.ent ubtn_regEvent
ubtn_regEvent:
//a0=adresa fce
//vraci v0=1 OK, tabulka plna vraci v0=0    
    
    
    addiu   t0, zero, EVENTS_CAPA
    la	    t1, events
    
1:    
    lw	    t9, (t1)
    bnez    t9, 2f	    //skok, polozka neni volna 
    nop
    
    //je vola, pouzit
    sw	    a0, (t1)
    li16    v0, 1
    jrc	    ra

2:    
    addiu   t0, -1	    //count--
    addiu   t1, 4	    //item++ (event table)
    bnez    t0, 1b	    //skok na dalsi item
    nop
    
    //chyba, tabulka je plna
    move    v0, zero
    jrc	    ra    
    
.end ubtn_regEvent 
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="ubtn_unregEvent">
    
.align 2
.global	ubtn_unregEvent    
.ent ubtn_unregEvent
ubtn_unregEvent:
//a0=adresa fce, ktera bude unreg
    
    _PUSH   ra
    
    addiu   v0, zero, EVENTS_CAPA
    la	    v1, events
1:    
    lw	    t9, (v1)
    bne	    t9, a0, 3f
    nop
    
    //nasel polozku
    sw	    zero, (v1)	    //vymazat
    
    la	    a0, events
    li16    a1, 4
    addiu   a2, zero, EVENTS_CAPA
    //a0=adresa tabulky, a1=velikost polozky, a2=max. pocet polozek
    bal	    defragTableW
    nop
    
2:
    _POP    ra
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
    
.end ubtn_unregEvent    
    
//</editor-fold>
   
//<editor-fold defaultstate="collapsed" desc="runEvent">
    
.align 2    
.ent runEvent
runEvent:
//a0=udalost UBTN_UP/DOWN/LONG_DOWN/LONG_UP/REPEAT
//a1=button value z buttons_table 
//a2=cnt - index opakovani REPEAT a LONG_DOWN (0-0xFF)    
    
    _PUSH_S04RA
   
    move    s0, a0			    //zaloha a0 (volana fce muze zmenit)
    move    s1, a1			    //zaloha a1 (volana fce muze zmenit)
    move    s2, a2
    
    la	    s3, events
    addiu   s4, zero, EVENTS_CAPA
    
1:    
    lw	    t9, (s3)
    beqz    t9, 2f			    //nulova polozka, konec dat
    nop
    
    move    a0, s0			    //nastav param0
    move    a1, s1			    //nastav param1
    move    a2, s2
   
    //volani c/c++ fce, argumenty:
    //pokud se vejdou do a0-a3, c fce predpoklada, ze jsou v a0-a3 (pouzije hodnoty v reg. a0-a3)
    //ale zaroven je ulozi do zasobniku, nad akt. pozici SP, kam by je mela ulozit volajici fce
    //protoze v prubehu fce se na ne muze odvolavat nikoliv v a0-a3, ale cist je ze zasobniku
    //v ASM kodu to znamena, ze musi posunout zasobnik o prislusnou velikost (nebo tyto argumenty 
    //do zasobniku ulozit - to neni nutne c/c++ to dela automaticky)
    addiu   sp, -12			    //3 argumenty int
    jalr    t9				    //volani fce (muze menit vse, krome s0-s7)
    nop
    addiu   sp, 12
    
    //next
    addiu   s4, -1	    //count--
    addiu   s3, 4	    //item++ (event table)
    bnez    s4, 1b
    nop

    
2:    
    _POP_S04RA
    jrc	    ra
    
.end runEvent   
    
//</editor-fold>
 
#endif    