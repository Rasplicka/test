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
//void fn(int event, int button_value)    
    
//fce ubtn_start nastavuje TRIS(input), ANLSEL(digital), CNPU(pull-up povoli), CNPD(pull-down zakaze)    
//tlacitko se pripojuje mezi pin a zem   
  
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
#define		REPEAT_FIRST_MS	    2000			//po 2000 ms poprve opakuje DOWN event (mod 1)
#define		REPEAT_NEXT_MS	    200				//po 200 ms druhe a dalsi opakovani DOWN event (mod 1)
    
  
//RAM vars    
.section .data
    
#define		BUTTONS_TABLE_ISIZE 8				//velikost polozky v buttons_table    
#define		BUTTONS_TABLE_CAPA  8    
start_time:	.space		    BUTTONS_TABLE_CAPA * 4
state:		.space		    BUTTONS_TABLE_CAPA    
    
#define		EVENTS_CAPA	    8				//kapacita events tabulky
events:	.space		    EVENTS_CAPA * 4		//8 polozek pro adresy fci volanych pri event
    
#define		STACK_SIZE	    256				//tento stack vyuzivaji fce volane z events tabulky!!!
ubtn_stack:	.space		    STACK_SIZE    
    
    
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
.byte	1		//mode 0=long up/down, 1=repeat down
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
//STATE nastavi na 0xF0 (= neni stisknuto)    
//definice tlacitek je v buttons_table    

    //nastav stack
    la	    a0, ubtn_stack
    li	    a1, STACK_SIZE
    bal	    setStack			//a0=base, a1=size
    nop
    
    //addiu   t0, (STACK_SIZE - 4)
    //move    sp, t0
    
    //nastav TRIS bit=1 (input), ANSEL bit=0 digital
    la	    t0, buttons_table
    la	    s0, start_time	//RAM tabulka start_time 12 x word
    la	    s1, state		//RAM tabulka STATE 12 x byte

    li16    v1, 1
    addiu   s2, zero, 0xF0	//button UP
    
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
    
    sw      zero, (s0)		//start_time=0x0
    sb	    s2, (s1)		//save STATE=0xF0 - button UP
    
    //next button
    addiu   t0, BUTTONS_TABLE_ISIZE
    addiu   s0, 4		//start_time[index++]
    addiu   s1, 1		//state[index++]
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
    
    la	    s2, buttons_table
    la	    s0, start_time
    la	    s1, state    
    addiu   s3, zero, 0xF	
    
1:
    lw	    t1, (s2)		    //t1=base addr registru portu
    beqz    t1, 10f		    //skok konec tabulky
    nop
    
    //zjisti akt. stav pinu
    lw32    v0, PORT_OFFSET(t1)	    //load port
    lbu	    t9, 4(s2)		    //load bit_index
    srlv    v0, v0, t9		    //cteny bit do b.0
    ext	    v0, v0, 0, 1	    //v0=stav pinu, 0 nebo 1
    
    //test, zda je ve stavu UP/DOWN
    lbu	    s4, (s1)		    //load STATE
    ext	    t9, s4, 4, 4	    //ext b3-b7
    beq	    t9, s3, 5f		    //skok, je UP, state=0xF? 
    nop
    
    //button je ve stavu DOWN
    bnez    v0, 3f		    //skok, pin=1 - znema
    nop
    
    //pin je stale 0, trva down
    //test mode 0/1
    lbu	    v0, 5(s2)		    //load mode
    bnez    v0, 45f		    //skok, mode=1 - repeat mode
    nop
    
    //mode 0 long down/up, test time
    //addiu   v0, zero, 0x0F
    sb	    s3, (s1)		    //save STATE vzdy 0x0F
    
    lw	    a0, (s0)		    //load start_time
    beqz    a0, 9f		    //skok, PRESS_LONG jiz bylo odeslano
    nop
    bal	    compareTimerMs	    //meni t8, t9
    nop
    
    _BAL_FLTC v0, LONG_DOWN_MS, 9f  //skok, nevyprsel cas
    
    //vyprsel cas dlouheho stisknuti, odesila DOWN_LONG
    li16    a0, UBTN_DOWN_LONG	    //btn down long
    lhu	    a1, 6(s2)		    //value z buttons_table
    //a0=param0 - event (UP/DOWN,...), a1=param1 - button value
    bal	    runEvent
    nop
    
    sw	    zero, (s0)		    //nuluj start_time, dalsi PRESS_LONG nebude posilat
    nop
    b16	    9f
    nop

3:    
    //pin=1, ale STATE=DOWN, s4=STATE
    ext	    t9, s4, 4, 4	    //ext b4-b7
    addiu   t9, 1
    li16    v0, 11
    beq	    t9, v0, 4f		    //skok, prave nastala zmena
    nop    
    
    //neni zmena (b4-b7 neni 10)
    sll	    t9, 4
    ori	    t9, 0x0F
    sb	    t9, (s1)		    //uloz novy STATE, b4-b7++
    nop
    b16	    9f
    nop
    
4:  
    //nastala znema na up
    addiu   t9, zero, 0xF0	    //STATE=0xF0
    sb	    t9, (s1)
    
    //test, zda je down long (start_time=0)
    lw	    t9, (s0)
    bnez    t9, 41f
    nop
    
    //je long up
    li	    a0, UBTN_UP_LONG
    nop
    b16	    42f
    nop
    
41: //neni long up   
    li16    a0, UBTN_UP		    //btn down (0=UP, 1=DOWN, 2=PRESS_LONG)

42:    
    lhu	    a1, 6(s2)		    //value z buttons_table
    
    //a0=param0 - event (UP/DOWN,...), a1=param1 - button value
    bal	    runEvent
    nop  
    
    nop
    b16	    9f
    nop

45:
    //test, zda je v modu 1 nebo 2, v0=mode
    li16    v1, 2
    beq	    v0, v1, 47f		    //skok, je v modu 2 
    nop
    
    //repeat mode, s4=STATE 0x0F = prvni pauza, 0x0E dalsi pauza
    lw	    a0, (s0)		    //load start_time
    nop
    bal	    compareTimerMs	    //meni t8, t9
    nop

    //test prvni/dalsi repeat
    beq	    s4, s3, 48f		    //skok, prvni repeat
    nop
    //dalsi repeat
    _BAL_FLTC v0, REPEAT_NEXT_MS, 47f  //skok, nevyprsel cas

46:     
    //vyprsel cas, odesila repeat
    lw	    a1, (timer_ms)
    sw	    a1, (s0)		    //nastav novy start_time
    //event
    li16    a0, UBTN_REPEAT	    //btn repeat
    lhu	    a1, 6(s2)		    //value z buttons_table
    
    //a0=param0 - event (UP/DOWN,...), a1=param1 - button value
    bal	    runEvent
    nop      
  
47:    
    //STATE nyni 0x0E
    li16    v0, 0x0E
    sb	    v0, (s1)		    //save STATE nyni 0x0E
    nop
    b16	    9f
    nop
    
48:
    _BAL_FBTC v0, REPEAT_FIRST_MS, 46b //skok, vyprsel cas pro prvni opakovani
    //nevyprsel cas pro prvni opakovani, pouze nastav STATE=0xF
    sb	    s3, (s1)		    //save STATE 0x0F
    nop
    b16	    9f
    nop
    
    ////////////////////////////////////////////////////////////////////////////
5:    
    //je UP b3-b7 = 0xF
    beqz    v0, 6f		    //skok, pin=down, nastala zmena z UP na DOWN
    nop
    
    //pin neni down, zadna zmena, button je stale UP
    addiu   v0, zero, 0xF0
    sb	    v0, (s1)
    nop
    b16	    9f
    nop
    
6:
    //zjisteno pin=0 (ale STATE=UP), s4=STATE
    ext	    t9, s4, 0, 4	    //ext b0-b3
    addiu   t9, 1
    li16    v0, 11
    beq	    t9, v0, 7f		    //skok, prave nastala zmena
    nop
    
    //neni zmena (b0-b3 neni 10)
    ori	    t9, 0xF0
    sb	    t9, (s1)		    //uloz novy STATE, b0-b3++
    nop
    b16	    9f
    nop
    
7:
    //nastala znema na down
    addiu   t9, zero, 0x0F	    //STATE=0x0F
    sb	    t9, (s1)
    
    li16    a0, UBTN_DOWN	    //btn down (0=UP, 1=DOWN, 2=PRESS_LONG)
    lhu	    a1, 6(s2)		    //value z buttons_table
    
    //a0=param0 - event (UP/DOWN,...), a1=param1 - button value
    bal	    runEvent
    nop
    
    //nastav start_time (nesmi byt 0x0)
    lw	    t9, (timer_ms)
    bnez    t9, 8f
    nop
    //time_ms=0 (0 nikdy nesmi byt, protoze indikuje stav, ze PRESS_LONG jiz byl odeslan)
    addiu   t9, 1
8:    
    sw	    t9, (s0)
    nop
    b16	    9f
    nop
    
9:    
    //next button
    addiu   s2, BUTTONS_TABLE_ISIZE
    addiu   s0, 4		//start_time++
    addiu   s1, 1		//state++
    nop
    b16	    1b
    nop
    
10:    
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
    
    //je vola, pouzir
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
   
//<editor-fold defaultstate="collapsed" desc="runEvent">
    
.align 2    
.ent runEvent
runEvent:
//a0=udalost UBTN_UP/DOWN/LONG_DOWN/LONG_UP/REPEAT
//a1=button value z buttons_table    
    
    _PUSH_S03RA
   
    move    s0, a0			    //zaloha a0 (volana fce muze zmenit)
    move    s1, a1			    //zaloha a1 (volana fce muze zmenit)
    
    addiu   s2, zero, EVENTS_CAPA
    la	    s3, events
    
1:    
    lw	    t9, (s3)
    beqz    t9, 2f
    nop
    
    move    a0, s0			    //nastav param0
    move    a1, s1			    //nastav param1
   
    //volani c/c++ fce, argumenty:
    //pokud se vejdou do a0-a3, c fce predpoklada, ze jsou v a0-a3 (pouzije hodnoty v reg. a0-a3)
    //ale zaroven je ulozi do zasobniku, nad akt. pozici SP, kam by je mela ulozit volajici fce
    //protoze v prubehu fce se na ne muze odvolavat nikoliv v a0-a3, ale cist je ze zasobniku
    //v ASM kodu to znamena, ze musi posunout zasobnik o prislusnou velikost (nebo tyto argumenty 
    //do zasobniku ulozit - to neni nutne c/c++ to dela automaticky)
    addiu   sp, -8			    //2 argumenty int
    jalr    t9				    //volani fce (muze menit vse, krome s0-s7)
    nop
    addiu   sp, 8
    
2:    
    addiu   s2, -1	    //count--
    addiu   s3, 4	    //item++ (event table)
    bnez    s2, 1b
    nop
    
    nop
    
    _POP_S03RA
    jrc	    ra
    
.end runEvent   
    
//</editor-fold>
 

#endif    