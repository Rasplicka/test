#include "xc.h"
#include "def.h"  
#include "asm_macro.S"
   

//.set mips32    
#define MICROMIPS  
.set micromips     
    
.section os_alloc_stack_here
//tuto sekci linker vlozi jako posledni v data RAM
//definuje prvni volne misto v RAM    
//stack_area:	    .dword	    0x0	    //dword proto, aby byla zarovnana na 8 bytes
//v pripade, ze by linker umistoval tuto sekci jinam, bylo by mozne fci malloc alokovat
//libovolna data v heap, kterou linker vzdy umistuje hned za data. Adresa techto dat
//by definovala zacatek oblasti stacku    
    
.section .asm_fn, code     
    
//<editor-fold defaultstate="collapsed" desc="allocStack">
    
.align 2
.global allocStack
.ent allocStack
allocStack:
//a0=pozadovana velikost stacku
//a1=thread table, adresa akt. polozky    
//vraci v0=1 OK, v0=0 chyba, neni misto
    
    addiu   v0, zero, PROC_T_CAPA	//max. pocet stacku je stejny, jako pocet procesu
    la	    t8, stack_list
    la	    t9, stack_area
    
1:    
    lw	    v1, (t8)			//load stack item (= stack size, nebo 0)
    beqz    v1, 3f
    nop
    
    addiu   t8, 4
    addu    t9, v1			//RAM += stack size (volna adresa)
    addiu   v0, -1			//cnt--
    bnez    v0, 1b			//skok na dalsi polozku
    nop
    
    //nenasel stack
2:  //end_error
    //v0=0
    li16    v0, 0
    jrc	    ra
    
3:
    //t9=aktualni adresa, ktera se pouzije jako stack_base
    li	    t0, RAM_BASE
    li	    t1, RAM_SIZE
    addu    t0, t1			//adresa za RAM (base + size)
    
    addu    v0, t9, a0			//adresa za stack (base + pozad. size)
    subu    t0, v0			//RAM-stack
    bltz    t0, 2b			//skok, neni misto
    nop
    
    //ok, je misto
    sw	    a0, (t8)			//save polozku
    
    //t9=aktualni adresa (stack base)
    sw	    t9, TH_T_STACK_BASE*4(a1)	//th_table[stack_base]
    
    li	    v1, STACK_CHECK_VALUE
    sw	    v1, (t9)			//stack[base]=STACK_CHECK_VALUE
    
    addu    t9, a0			//stack top (base+size)
    addiu   t9, -4			//stack top -4
    sw	    t9, TH_T_SP*4(a1)		//th_table[sp]

    //OK, konec
    li16    v0, 1
    jrc	    ra
    
.end allocStack  
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="doEvents">     
.align 2	
.global doEvents    
.ent doEvents    
doEvents:
    //prepnuti procesu
    //ra obsahuje navratovou adresu
    
#ifdef SAFE_PROCESS    
    //nuluj count
    mtc0    zero, _CP0_COUNT 
#endif    
    
    la	    t9, proc_t_pos	    //zde je ulozena akt. pozice v proc_t
    lw	    t8, (t9)		    //process table
    
#ifdef ENABLE_CHECK_STACK_OVERFLOW
    //test, zda posledni (nejnizsi) word ve stacku na kontrolni hodnotu
    //pokud ne, nastalo stack overflow
    lw	    t6, TH_T_STACK_BASE*4(t8)	//adresa stack_base
    lw	    t6, (t6)			//load data
    li	    t7, STACK_CHECK_VALUE
    tne	    t6, t7		    //trap
    nop
    
#endif   
    
    //<editor-fold defaultstate="collapsed" desc="save regs">
    swp     v0, TH_T_V0*4(t8)	    //v0, v1
    swp     a0, TH_T_A0*4(t8)	    //a0, a1
    swp     a2, TH_T_A2*4(t8)	    //a2, a3

    swm32   s0-s7, TH_T_S0*4(t8)	    //s0-s7

    swp     gp, TH_T_GP*4(t8)	    //gp, sp
    swp     fp, TH_T_FP*4(t8)	    //fp, ra

    //HI/LO
    //mflo    t0
    //mfhi    t1
    //swp	    t0, TH_T_LO*4(t8)
    //</editor-fold>
    
    la	    v0, proc_t_max	    //proc_t_max
    lw	    v0, (v0)		    //v0=process_max
    bne	    v0, t8, 1f		    //skok, neni na posl. pozici proc_t
    nop
    
    //je na posl.
    nop
    la	    t8, proc_t		    //nastav prvni polozku proc_t
    nop
    b16	    2f
    nop
1:    
    //
    addu    t8, PROC_T_ISIZE	    //dalsi polozka proc_t
    
2:    
    //t8=adresa polozky v proc_t
    sw	    t8, (t9)		    //ulozit adresu akt. polozky proc_t
    
    //<editor-fold defaultstate="collapsed" desc="load regs"> 
    //HI/LO
    //lwp	    v0, TH_T_LO*4(t8)
    //mtlo    v0
    //mthi    v1 

    //lw      $1,  TH_T_REGS + 4 (t8)       //at
    lwp     v0, TH_T_V0*4(t8)	    //v0, v1
    lwp     a0, TH_T_A0*4(t8)	    //a0, a1
    lwp     a2, TH_T_A2*4(t8)	    //a2, a3

    //s0-s7 v memory model
    lwm32   s0-s7, TH_T_S0*4(t8)

    lwp     gp, TH_T_GP*4(t8)	    //gp, sp
    lwp     fp, TH_T_FP*4(t8)	    //fp, ra
    
    //</editor-fold>   

    jrc	    ra
    nop
.end doEvents    
//</editor-fold>    
    
//<editor-fold defaultstate="collapsed" desc="startEvents">    
.align 2    
.global startEvents
.ent startEvents    
startEvents:
    //start, spusti prvni proces v process table
    
#ifdef SAFE_PROCESS    
    //nuluj count
    mtc0    zero, _CP0_COUNT 
    ehb
    
    //nastav compare
    li	    v0, 0xFFFFF
    mtc0    v0, _CP0_COMPARE
    ehb
    
#endif      
    
    la	    k0, proc_t		//process table
    la	    k1, proc_t_pos	//zde je ulozena akt. pozice v proc_t
    sw	    k0, (k1)		//uloz prvni pozici
    
    lw	    ra, TH_T_RA*4(k0)
    lw	    sp, TH_T_SP*4(k0)
    
    jrc	    ra
    nop				//musi zde zustat
.end startEvents
//</editor-fold>    
    
//<editor-fold defaultstate="collapsed" desc="doEventsL">     
.align 2    
.global doEventsL    
.ent doEventsL 
doEventsL:
    //prepnuti procesu
    //a0 obsahuje navratovou adresu, kam se skoci v dalsim cyklu
    	
#ifdef SAFE_PROCESS    
    //nuluj count
    mtc0    zero, _CP0_COUNT 
#endif      
    
    //di
    move    ra, a0			//a0 pouzije jako ra
    
#ifdef MICROMIPS    
    ori	    ra, 0x1			//micromips
#endif    
    
    la	    t9, proc_t_pos		//zde je ulozena akt. pozice v proc_t
    lw	    t8, (t9)			//process table
    
#ifdef ENABLE_CHECK_STACK_OVERFLOW
    //test, zda posledni (nejnizsi) word ve stacku na kontrolni hodnotu
    //pokud ne, nastalo stack overflow
    lw	    t6, TH_T_STACK_BASE*4(t8)
    lw	    t6, (t6)			//load data
    li	    t7, STACK_CHECK_VALUE
    tne	    t6, t7			//trap
    nop
    
#endif      
    
    //<editor-fold defaultstate="collapsed" desc="save regs">
    swp     v0, TH_T_V0*4(t8)	    //v0, v1
    swp     a0, TH_T_A0*4(t8)	    //a0, a1
    swp     a2, TH_T_A2*4(t8)	    //a2, a3

    swm32   s0-s7, TH_T_S0*4(t8)	    //s0-s7

    //26,27 = t8,t9
    swp     gp, TH_T_GP*4(t8)	    //gp, sp
    swp     fp, TH_T_FP*4(t8)	    //fp, ra

    //HI/LO
    //mflo    t0
    //mfhi    t1
    //swp	    t0, TH_T_LO*4(t8)
    //</editor-fold>
    
    la	    v0, proc_t_max	    //proc_t_max
    lw	    v0, (v0)		    //v0=process_max
    bne	    v0, t8, 1f		    //skok, neni na posl. pozici proc_t
    nop
    
    //je na posl.
    nop
    la	    t8, proc_t		    //nastav prvni polozku proc_t
    nop
    b16	    2f
    nop
1:    
    //
    addu    t8, PROC_T_ISIZE	    //dalsi polozka proc_t
    
2:    
    //t8=adresa polozky v proc_t
    sw	    t8, (t9)		    //ulozit adresu akt. polozky proc_t
    
    //<editor-fold defaultstate="collapsed" desc="load regs"> 
    //HI/LO
    //lwp	    v0, TH_T_LO*4(t8)
    //mtlo    v0
    //mthi    v1 

    lwp     v0, TH_T_V0*4(t8)	    //v0, v1
    lwp     a0, TH_T_A0*4(t8)	    //a0, a1
    lwp     a2, TH_T_A2*4(t8)	    //a2, a3

    //s0-s7 v memory model
    lwm32   s0-s7, TH_T_S0*4(t8)

    lwp     gp, TH_T_GP*4(t8)	    //gp, sp
    lwp     fp, TH_T_FP*4(t8)	    //fp, ra
    
    //nop
    //nop
    //nop
    //nop
    //</editor-fold>   

    
    //ori	    ra, 0x1
    
    //ei
    jrc	    ra
    nop
.end doEventsL    
//</editor-fold>        
    
//<editor-fold defaultstate="collapsed" desc="getGP">         
.align 2    
.global getGP    
.ent getGP 
getGP:
    //vraci hodnotu GP, nastavi se pro kazdy process v proc_t
    move    v0, gp
    jrc	    ra
    
.end getGP    
//</editor-fold>   
   
    
//<editor-fold defaultstate="collapsed" desc="strLen">
.align 2    
.global strLen    
.ent strLen
strLen:
//a0=adresa stringu \0
    
    //vraci delku
    li16    v0, 0
    nop
    b16	    2f
    nop
1:
    addiu   v0, 1
    addiu   a0, 1
    
2:    
    lbu	    t9, (a0)
    bnez    t9, 1b
    nop
    
    jrc	    ra
    
.end strLen 
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="concat">
.global concat
.ent concat
concat:
//a0=dest	    //pole, ke kteremu prida src (obsahuje string \0)
//a1=src	    //obsahuje string \0
//a2=desl len	    //max. velikost pole dest, vc. \0   
//meni a0-a2, v0    
    
    //vraci v0=1 OK, v0=0 chyba
    //k prvnimu (a0) pripoji druhy string (a1)
    //prvni i druhy musi koncit \0
    //pokud prvni nekonci \0 (nebo je delsi nez len (a2) ) nic se neprovede
    //pokud se po slouceni nevejdou do len(a2) druhy bude zkraceny, \0 bude jako posledni znak
    
    nop
    b16	    2f
    nop
    
1:  
    addiu   a0, 1
    addiu   a2, -1;
    beqz    a2, 98f		//skok, je na konci, nenasel \0
    nop
2:    
    //hleda \0 v prvnim stringu
    lbu	    v0, (a0)
    bnez    v0, 1b		//skok, neni \0 
    nop
    
    //nasel /0 (a0)
3:    
    lbu	    v0, (a1)
    sb	    v0, (a0)
    
    beqz    v0, 99f		//skok, druhy text je na konci vc. \0
    nop
    
    addiu   a2, -1
    beqz    a2, 97f		//nenasel \0 na konci druheho textu
    nop
    
    addiu   a0, 1
    addiu   a1, 1
    nop
    b16	    3b			//dalsi znak
    nop
    
97:
    //vlozi \0 na konec
    sb	    zero, (a0)
    
98:
    move    v0, zero
    jrc	    ra
    
99:
    //konec
    li16    v0, 1
    jrc	    ra
    
.end concat 
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="lTrim">
.align 2    
.global lTrim    
.ent lTrim
lTrim:
//a0=adresa input  string, zakoncene \0
//a1=adresa output string, zakoncene \0   
    
    //meni v0, v1, t8, t9
    //kopiruje input string do output, bez mezer na zacatku

    move    v0, a0
    move    v1, a1
    addiu   t8, zero, _CHAR_SPACE
    
1:    
    lbu	    t9, (v0)
    bne	    t9, t8, 2f			    //skok, neni mezera
    nop
    //je mezera
    addiu   v0, 1
    nop
    b16	    1b
    nop
    
2:
    //jiz vynechal leve mezery
    sb	    t9, (v1)
    beqz    t9, 9f			    //skok, input je na konci
    nop
    
    addiu   v1, 1
    addiu   v0, 1
    lbu	    t9, (v0)
    nop
    b16	    2b
    nop
    
9:
    //za output vlozi 0x0
    jrc	    ra
    
.end lTrim 
    
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="rTrim">
.align 2     
.global rTrim    
.ent rTrim    
rTrim:
//a0=adresa input  string, zakoncene \0
//a1=adresa output string, zakoncene \0
    
    //meni t6-t9, v0, v1
    
    move    v0, a0
    move    t7, zero
    addiu   t6, zero, _CHAR_SPACE
    
1:    
    lbu	    t8, (v0)
    beqz    t8, 5f				//skok, je na konci input str
    nop
    
    beq	    t8, t6, 2f				//skok, je mezera
    nop
    
    //neni mezera
    move    t7, v0				//zalohuj posledni pocizi znaku
2:
    addiu   v0, 1
    nop
    b16	    1b
    nop
    
5:  
    move    v1, a1
    beqz    t7, 9f				//skok, zadny znak v output
    nop
    
    //t7=adresa posledniho znaku
    move    v0, a0
    
6:
    lbu	    t8, (v0)
    sb	    t8, (v1)
    addiu   v1, 1
    
    beq	    v0, t7, 9f				//skok, toto byl posledni znak
    nop    
    addiu   v0, 1
    nop
    b16	    6b
    nop
    
9:
    sb	    zero, (v1)
    jrc	    ra
    
.end rTrim   
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="trim">
.align 2     
.global trim
.ent trim
trim:
//a0=adresa input  string, zakoncene \0
//a1=adresa output string, zakoncene \0 
//meni v0, v1, t5-t9  
    
    _PUSH   ra
    
    move    t5, a0
    
    bal	    lTrim	    //meni v0, v1, t8, t9
    nop
    
    move    a0, a1
    bal	    rTrim
    nop
    
    move    a0, t5	    //meni t6-t9, v0, v1
    
    _POP    ra
    jrc	    ra
    
.end trim
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="addSpace">
.align 2    
.global addSpace
.ent addSpace
addSpace:    
//a0=string \0
//a1=pozadovany pocet platnych znaku (nepocita \0)
    
    //prida mezery na konec, aby dosahl pozadovaneho poctu znaku (nepocita \0 na konci, pouze platne znaky)
    
    li	    t0, _CHAR_SPACE
    move    t1, zero
    
1:    
    lbu	    v0, (a0)
    beqz    v0, 5f
    nop
    
    addiu   t1, 1			//pocet ++
    addiu   a0, 1
    nop
    b16	    1b
    nop
    
    
5:    
    //nasel konec \0
    beq	    a1, t1, 9f			//skok, je pozadovany pocet
    nop
    
    sb	    t0, (a0)
    addiu   t1, 1
    addiu   a0, 1
    nop
    b16	    5b
    nop
    
9:
    sb	    zero, (a0)
    jrc	    ra
    
.end addSpace  
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="formatLeft">
    
.align 2
.global formatLeft
.ent formatLeft
formatLeft:
//a0=char* result
//a1=int size   (musi byt mensi, nez velikost result, fce prida \0)
//a2=char* text (zakonceno \0)
    
//prida na konec mezery, aby v result byla celkova delka size (pocet platnych znaku, na konec jeste prida \0)    
//vysledek ulozi do result    
//je-li text delsi nez size, bude zkracen na size (oriznuty zprava)  
    
    tlti    a1, 1		    //trap, pri a1<1
    addiu   v1, zero, _CHAR_SPACE   //v1=SPACE
    
    addu    t8, a0, a1		    //t8=adresa v result, kam zapise \0 (result + size)
    
1:    
    beq	    a0, t8, 3f
    nop
    
    lbu	    v0, (a2)
    beqz    v0, 2f
    nop
    
    sb	    v0, (a0)
    addiu   a0, 1
    addiu   a2, 1
    nop
    b16	    1b
    nop
    
2:
    sb	    v1, (a0)
    addiu   a0, 1
    
    bne	    a0, t8, 2b	    //skok, result jeste nema pozadovanou velikost
    nop
    
    
    
3:
    //a0=adresa za textem, kam vlozi \0
    sb	    zero, (a0)
    
    jrc	    ra
    
.end formatLeft 
    
//</editor-fold>
   
//<editor-fold defaultstate="collapsed" desc="formatRight">
    
.align 2
.global formatRight
.ent formatRight
formatRight:
//a0=char* result
//a1=int size   (musi byt mensi, nez velikost result, fce prida \0)
//a2=char* text (zakonceno \0)
    
//prida na zacatek mezery, aby v result byla celkova delka size (pocet platnych znaku, bez \0)
//vysledek ulozi do result    
//je-li text delsi nez size, bude zkracen na size (oriznuty zleva)    
    
    tlti    a1, 1		    //trap, pri a1<1
    addiu   v1, zero, _CHAR_SPACE   //v1=SPACE
    
    //najdi konec textu
    move    t9, a2		    //adresa textu
    
    //najdi konec textu
1:    
    lbu	    t8, (t9)
    beqz    t8, 2f		    //skok, nasel konec textu
    nop
    addiu   t9, 1		    //++
    nop
    b16	    1b
    nop
    
    
2:
    //t9=adresa za poslednim platnym znakem v text (ukazuje na \0)
    
    addu    t8, a0, a1			//pozice za result, zde bude v result \0 (result + size)
    sb	    zero, (t8)			//\0
    
3:    
    beq	    t9, a2, 4f			//skok, text je na prvnim znaku
    nop
    
    beq	    t8, a0, 9f			//skok, result je na prvnim znaku (text je delsi nez size)
    nop
    
    addiu   t9, -1
    addiu   t8, -1
    
    lbu	    v0, (t9)
    sb	    v0, (t8)
    nop
    b16	    3b
    nop
    
4:
    //text vycerpal, bude doplnovat MEZERY
    beq	    t8, a0, 9f			//skok, result je na prvnim znaku
    nop
    
    addiu   t8, -1
    sb	    v1, (t8)
    nop
    b16	    4b
    nop
    
9:
    
    jrc	    ra
    
.end formatRight 
    
//</editor-fold>
    

//<editor-fold defaultstate="collapsed" desc="rndInt">
    
.align 2
.global rndInt
.ent rndInt
rndInt:
//a0=int
//a1=cipher zaokrouhli 1=desitky, 2=stovky, 3=tisice, ...
    //fce provede zaokrouhleni cisla 
    
    addiu   v0, zero, 5
    addiu   v1, zero, 10
    addiu   t9, zero, 10
    addiu   t8, zero, 1
    
    bgez    a0, 1f	    //skok, a0 je kladne
    nop
    
    //a0 je zaporne, prevod na kladne
    //addi    t8, zero, -1
    //mult    a0, t8
    //mflo    a0
    addiu   v0, zero, -5
    
    
1:    
    addiu   a1, -1
    
    beqz    a1, 2f	    //skok, v0 a v1 je nastaveno na spravne hodnoty
    nop
    
    multu   v1, t9	    //v1x10 (100,1000,...)
    mflo    v1
    mult    v0, t9	    //v0x10 (50,500,... / -50,-500,...)
    mflo    v0
    nop
    b16	    1b
    nop
    
2:    
    //v0=5,50,500,5000,... / -5,-50,-500,... 
    //v1=10,100,1000,10000,...
    add     a0, v0	//i+v0
    div	    a0, v1	// / v1
    mflo    a0
    multu   a0, v1	// x v1
    mflo    v0		//return v0
    
    //+/- vysledek
    //mult    v0, t8
    //mflo    v0
    
    jrc	    ra
    
9:
    //chyba a1=0
    move    v0, a0
    jrc	    ra
    
.end rndInt  
    
//</editor-fold>
  
//<editor-fold defaultstate="collapsed" desc="random">
    
.align 2
.global random
.ent random
random:
//a0=rozsah  (0 ... rozsah)
//fce vraci nahodne cislo - word 32-bit
    
    mfc0    v0, _CP0_COUNT
    
    addu    v0, t9
    addu    v0, t8
    addu    v0, a0
    addu    v0, a1
    addu    v0, s2
    addu    v0, s3    
    addu    v0, sp
    addu    v0, k1
    
    //nuluj b31, kladne cislo
    ins	    v0, zero, 31, 1
    nop
    b16	    2f
    nop
    
1:
    srl	    v0, 1
2:    
    subu    t9, v0, a0
    bgtz    t9, 1b
    nop
    
    jrc	    ra
    
.end random  
    
//</editor-fold>    
    
//<editor-fold defaultstate="collapsed" desc="compareTimerMs">
    
.align 2
.global compareTimerMs
.ent compareTimerMs    
compareTimerMs:
//a0=porovnavany cas (predpoklada se, ze je starsi, nez soucasny - timer_ms)
//vraci kolik ms uplynulo od zadaneho casu    
//meni t8,t9 (v0)    
    
    //je-li timer nastaven na 10ms, rozdil je po 10ms
    
    lw	    t9, (timer_ms)
    subu    v0, t9, a0
     
    bltz    v0, 1f
    nop
    jrc	    ra
    
1:
    //doslo k preteceni timer_ms
    li	    t8, 0xFFFFFFFF
    subu    v0, t8, a0 
    addu    v0, t9
    jrc	    ra
    
.end compareTimerMs 

.align 2
.global pauseEvent
.ent pauseEvent
pauseEvent:
//a0=pauza ms
    
#ifdef SIMULATOR
    //simulator nemeri cas
    jrc	    ra
#endif    
    
    move    a2, ra		    //zaloha ra
    move    v1, a0		    //pocet ms cekani
    lw	    a0, timer_ms	    //pocatecni time_ms
    
1:
    //test, zda je jiz Threading (muze volat doEvents)
    lbu	    t9, (SYSTEM_STATUS)	    //SYSTEM_STATUS.b0=Threading
    ext	    t9, t9, 0, 1	    //ext b0
    beqz    t9, 2f
    nop
    
    //doEvents
    bal	    doEvents		    //neuklada obsah t0-t9
    nop
    
2:    
    //porovnava aktualni time_ms s time_ms v a0 (pocatecni cas)
    bal	    compareTimerMs	    //meni t9, t8, v0 vraci kolik uplynulo ms (po 10 ms)
    nop
    
    subu    v0, v1
    bltz    v0, 1b		    //skok, neuplynul cas
    nop
    
    move    ra, a2		    //obnov ra
    jrc	    ra
    
.end pauseEvent    
    
//</editor-fold>
   
     
//<editor-fold defaultstate="collapsed" desc="macro _WRITE_CIPHER">
    
.macro _WRITE_CIPHER num, cip, rad, result
//num = zapisovane cislo 0-9
//cip = pozadovany pocet cifer (min.)    
//rad = aktualni rad (10=mld, 2=desitky, 1=jednotky)    
//result = vysledne pole 
//meni t9    
    
    beqz    \num, 1002f
    nop
    
    //num neni 0
    addiu   \cip, zero, 10	//nastavi cip=10, tzn. ze od ted bude zapisovat kazde cislo, i 0
    
1001:    
    addiu   \num, 0x30		//mun - ascii
    sb	    \num, (\result)
    addiu   \result, 1		//result++
    
    nop
    b16	    1003f
    nop
    
1002:
    //num je nula
    subu    t9, \rad, \cip
    blez    t9, 1001b		//skok, tuto "0" bude zapisovat (rad <= cip)
    nop
    
    //num je nula na zacatku, zatim nepronadi zapis
    
1003:    
    
.endm 
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="intToChar">
    
.align 2
.global intToChar
.ent intToChar
intToChar:
//a0=int
//a1=char* result (min. char[12] -xxxxxxxxxx\0)
//a2=pocet cifer (min. pocet cifer, je-li cislo mensi, bude zleva doplneno 0) 1-10    
//meni t4-t9, a0-a2    
    
    bgez    a0, uintToChar	    //skok, int je kladne
    nop
    
    //int je zaporne, zapise "-" a prevede na kladne
    //result[.]="-"
    addiu   t9, zero, _CHAR_MINUS
    sb	    t9, (a1)
    addiu   a1, 1		    //result++
    
    //int prevod na kladne cislo (NEG(int) + 1)
    not	    a0
    addiu   a0, 1
    
.global uintToChar   
uintToChar:    
    
    addiu   t4, zero, 10	    //rad, aktualni rad cisla (10=mld, 1=jednotky)
    li	    t5, 1000000000
    addiu   t7, zero, 10	    //konst    
    
    
1:
    //int je kladne cislo
    divu    t6, a0, t5
    mflo    t6			    //t6=num
    mfhi    a0			    //zbytek
    
    //num, cip, rad, result
    _WRITE_CIPHER t6, a2, t4, a1    //meni t9
    //result++, pokud provedl zapis
    
    addiu   t4, -1		    //rad--
    divu    t5, t7		    //c/10
    mflo    t5
    
    li16    v0, 1
    bne	    t5, v0, 1b		    //skok, dalsi kolo
    nop
    
    //konec a0=jednotky
    addiu   a0, 0x30		    //prevod na ascii znak
    sb	    a0, (a1)		    //result[]=jednotky
    sb	    zero, 1(a1)		    //result[]=\0
    
    jrc	    ra
    
.end intToChar
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="byteToChar">
    
.align 2
.global byteToChar
.ent byteToChar
byteToChar:  
//a0=byte
//a1=char* result (min char[4], xxx\0  )
//a2=pocet cifer (min. pocet cifer, je-li cislo mensi, bude zleva doplneno 0) 1-3
//meni t4-t9, a0-a2
    
//prevod byte bez znamenka
    
    addiu   t4, zero, 3		    //rad, aktualni rad cisla (3=stovky, 1=jednotky)
    li	    t5, 100		    //c
    addiu   t7, zero, 10	    //konst  
    
    andi    a0, 0xFF		    //nuluje b8-b31
    
    
1:
    //int je kladne cislo
    divu    t6, a0, t5
    mflo    t6			    //t6=num
    mfhi    a0			    //zbytek
    
    //num, cip, rad, result
    _WRITE_CIPHER t6, a2, t4, a1    //meni t9
    //result++, pokud provedl zapis
    
    addiu   t4, -1		    //rad--
    divu    t5, t7		    //c/10
    mflo    t5
    
    li16    v0, 1
    bne	    t5, v0, 1b		    //skok, dalsi kolo
    nop
    
    //konec a0=jednotky
    addiu   a0, 0x30		    //prevod na ascii znak
    sb	    a0, (a1)		    //result[]=jednotky
    sb	    zero, 1(a1)		    //result[]=\0
    
    jrc	    ra
    
.end byteToChar
    
//</editor-fold>
    
    
    
    
    
//<editor-fold defaultstate="collapsed" desc="setPortDigOut">
.align 2    
.global setPortDigOut    
.ent setPortDigOut
setPortDigOut:    
//a0 base adresa portu, PORTA_BASE, PORTB_BASE, ...
//a1 pozadivane bity BIT0, BIT2 | BIT3, ...  
    
    //nastavuje pouze TRIS (in/out) a ANSEL (analog/digital)
    
    //napr.: setPortDigOut(POTRA_BASE, BIT1 | BIT4 | BIT8);
    
    //TRISbit=0
    sh32    a1, (TRIS_OFFSET + CLR_OFFSET)(a0)
    //ANSELbit=0 
    sh32    a1, (ANSEL_OFFSET + CLR_OFFSET)(a0)
    
    jrc	    ra
    
.end setPortDigOut 

//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="setPortDigIn">
.align 2    
.global setPortDigIn     
.ent setPortDigIn
setPortDigIn:    
//a0 base adresa portu, PORTA_BASE, PORTB_BASE, ...
//a1 pozadivane bity BIT0, BIT2 | BIT3, ...    
    
    //nastavuje pouze TRIS (in/out) a ANSEL (analog/digital)
    
    //napr.: setPortDigIn(POTRA_BASE, BIT1 | BIT4 | BIT8);
    
    //TRISbit=1
    sh32    a1, (TRIS_OFFSET + SET_OFFSET)(a0)
    //ANSELbit=0 
    sh32    a1, (ANSEL_OFFSET + CLR_OFFSET)(a0)
    
    jrc	    ra
    
.end setPortDigIn 
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="setPortAnalogIn">
.align 2    
.global setPortAnalogIn     
.ent setPortAnalogIn
setPortAnalogIn:    
//a0 base adresa portu, PORTA_BASE, PORTB_BASE, ...
//a1 pozadivane bity BIT0, BIT2 | BIT3, ...    
    
    //nastavuje pouze TRIS (in/out) a ANSEL (analog/digital)
    
    //napr.: setPortAnalogIn(POTRA_BASE, BIT1 | BIT4 | BIT8);
    
    //TRISbit=1
    sh32    a1, (TRIS_OFFSET + SET_OFFSET)(a0)
    //ANSELbit=1 
    sh32    a1, (ANSEL_OFFSET + CLR_OFFSET)(a0)
    
    jrc	    ra
    
.end setPortAnalogIn 
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="setPortOpenDrain">
.align 2    
.global setPortOpenDrain     
.ent setPortOpenDrain
setPortOpenDrain:    
//a0 base adresa portu, PORTA_BASE, PORTB_BASE, ...
//a1 pozadivane bity BIT0, BIT2 | BIT3, ... 
//a2 1=nastav ODC, 0=nuluj ODC (vypnuto)    
    
    //nastavuje hodnotu ODCx 
    
    //napr.: setPortOpenDrain(POTRA_BASE, BIT1 | BIT4 | BIT8, 1);		//nastav OpenDrain B.1, B.4, B.8
    
    beqz    a2, 1f
    nop
    
    //ODCx=1
    sh32    a1, (ODC_OFFSET + SET_OFFSET)(a0)
    jrc	    ra
    
1:
    //ODCx=0
    sh32    a1, (ODC_OFFSET + CLR_OFFSET)(a0)
    jrc	    ra
    
.end setPortOpenDrain
    
//</editor-fold>
  
//<editor-fold defaultstate="collapsed" desc="setPortPullUp">
.align 2    
.global setPortPullUp     
.ent setPortPullUp
setPortPullUp:    
//a0 base adresa portu, PORTA_BASE, PORTB_BASE, ...
//a1 pozadivane bity BIT0, BIT2 | BIT3, ... 
//a2 1=nastav PullUp, 0=nuluj PullUp (vypnuto)    
    
    //nastavuje hodnotu CNPUx 
    
    //napr.: setPortPullUp(POTRA_BASE, BIT1 | BIT4 | BIT8, 1);			//nastav PullUp B.1, B.4, B.8
    
    beqz    a2, 1f
    nop
    
    //CNPUx=1
    sh32    a1, (CNPU_OFFSET + SET_OFFSET)(a0)
    jrc	    ra
    
1:
    //CNPUx=0
    sh32    a1, (CNPU_OFFSET + CLR_OFFSET)(a0)
    jrc	    ra
    
.end setPortPullUp 
    
//</editor-fold>
 
//<editor-fold defaultstate="collapsed" desc="setPortPullDown">
.align 2    
.global setPortPullDown     
.ent setPortPullDown
setPortPullDown:    
//a0 base adresa portu, PORTA_BASE, PORTB_BASE, ...
//a1 pozadivane bity BIT0, BIT2 | BIT3, ... 
//a2 1=nastav PullDown, 0=nuluj PullDown (vypnuto)    
    
    //nastavuje hodnotu CNPDx 
    
    //napr.: setPortPullDown(POTRA_BASE, BIT1 | BIT4 | BIT8, 1);			//nastav PullDown B.1, B.4, B.8
    
    beqz    a2, 1f
    nop
    
    //CNPDx=1
    sh32    a1, (CNPD_OFFSET + SET_OFFSET)(a0)
    jrc	    ra
    
1:
    //CNPDx=0
    sh32    a1, (CNPD_OFFSET + CLR_OFFSET)(a0)
    jrc	    ra
    
.end setPortPullDown
    
//</editor-fold>
    
    
//<editor-fold defaultstate="collapsed" desc="defragTableW">
    
.align 2
.global defragTableW
.ent defragTableW
defragTableW:
//a0=adresa tabulky
//a1=item size [bytes] (delitelne 4)
//a2=max. pocet polozek, item cnt
//meni t7-t9, v0, v1, a0-a2
    
    //tabulka musi obsahovat polozky word (velikost polozky min. 1 word, delitelna 4)
    //posouva polozky v tabulce tak, aby mezi nimi nebyly neplatne polozky
    //tzn. prvni nulova polozka je konec tabulky
    //polozka musi zacinat word, ktery urcuje jeji platnost. Hodnota 0x0 = polozka je neplatna, hodnota != 0x0, polozka je platna
    
    
    //a0=src
    //t7=dest
    move    t7, a0
    
1:
    lw	    t9, (a0)		    //load src data
    beqz    t9, 3f		    //skok, src data=0
    nop
    
    //src data neni 0
    bne	    t7, a0, 4f		    //skok src != dest, bude presun
    nop
    
    //src=dest, zadny presun
2:    
    addu    t7, a1		    //dest++
    
3:
    addu    a0, a1		    //src++
    addiu   a2, -1		    //item cnt--
    bne	    a2, zero, 1b	    //neni konec
    nop
    
    //je konec, prosel vsechny polozky
    jrc	    ra
    nop
    
4:
    //presun dat ze src do dest
    move    v0, a0		    //src
    move    v1, t7		    //dest
    move    t8, a1		    //size bytes
    
5:    
    lw	    t9, (v0)
    sw	    t9, (v1)
    addiu   v0, 4
    addiu   v1, 4
    addiu   t8, -4
    bnez    t8, 5b		    //skok, polozka pokracuje
    nop
    
    //src[0]=0x0
    sw	    zero, (a0)
    nop
    b16	    2b
    nop
    
    
.end defragTableW    
    
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="defragTableB">
    
.align 2
.global defragTableB
.ent defragTableB
defragTableB:
//a0=adresa tabulky
//a1=item size [bytes] (delitelne 1)
//a2=max. pocet polozek, item cnt
//meni t7-t9, v0, v1, a0-a2    
    
    //tabulka musi obsahovat polozky word (velikost polozky min. 1 word, delitelna 4)
    //posouva polozky v tabulce tak, aby mezi nimi nebyly neplatne polozky
    //tzn. prvni nulova polozka je konec tabulky
    //polozka musi zacinat word, ktery urcuje jeji platnost. Hodnota 0x0 = polozka je neplatna, hodnota != 0x0, polozka je platna
    
    
    //a0=src
    //t7=dest
    move    t7, a0
    
1:
    lbu	    t9, (a0)		    //load src data
    beqz    t9, 3f		    //skok, src data=0
    nop
    
    //src data neni 0
    bne	    t7, a0, 4f		    //skok src != dest, bude presun
    nop
    
    //src=dest, zadny presun
2:    
    addu    t7, a1		    //dest++
    
3:
    addu    a0, a1		    //src++
    addiu   a2, -1		    //item cnt--
    bne	    a2, zero, 1b	    //neni konec
    nop
    
    //je konec, prosel vsechny polozky
    jrc	    ra
    nop
    
4:
    //presun dat ze src do dest
    move    v0, a0		    //src
    move    v1, t7		    //dest
    move    t8, a1		    //size bytes
    
5:    
    lbu	    t9, (v0)
    sb	    t9, (v1)
    addiu   v0, 1
    addiu   v1, 1
    addiu   t8, -1
    bnez    t8, 5b		    //skok, polozka pokracuje
    nop
    
    //src[0]=0x0
    sb	    zero, (a0)
    nop
    b16	    2b
    nop
    
    
.end defragTableB    
    
//</editor-fold>    
    
//<editor-fold defaultstate="collapsed" desc="cpyStr">
/*    
.align 2
.global cpyStr
.ent cpyStr
cpyStr:
//t0=src
//t1=dest
//t2=max size
    
1:    
    lbu	    t9, (t0)
    sb	    t9, (t1)
    beqz    t9, 2f		//skok, konec - nasel \0
    nop
    
    addiu   t0, 1
    addiu   t1, 1
    addiu   t2, -1
    bnez    t2, 1b
    nop
   
2:    
    jrc	    ra
    
.end cpyStr 
*/ 
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="memcpy32">
.align 2
.global memcpy32
.ent memcpy32
memcpy32:
//a0=src
//a1=dest
//a2=len
    
    //len musi byt delitelne 4
    
    beqz    a2, 2f
    nop
    
1:    
    lw	    t9, (a0)
    sw	    t9, (a1)
    addiu   a0, 4
    addiu   a1, 4
    addiu   a2, -4
    bnez    a2, 1b
    nop
2:    
    jrc	    ra
    
.end memcpy32    
    
//</editor-fold>

  
    
//<editor-fold defaultstate="collapsed" desc="pwm_soft_timer">
#ifdef PWM_SOFT
    
.align 2
.global pwm_soft_timer    
.ent pwm_soft_timer
pwm_soft_timer:
//je volano z timer1 interruptu, je-li definovano PWM_SOFT
//nastavuje vykon zmenou CCPxRB    
//pracuje podle dat v pwm_table
//byte[0]     timebase time step
//byte[1]     aktualni time value
//byte[2-3]   target value
//byte[4-7]   step 
//byte[8-11]  adresa fce pwm_soft_linear_down, pwm_soft_linear_up, ...
//byte[12-15] adresa CCPxRB registru - obsahuje value   
//byte[16-19] adresa pole dat pro jiny, nez linearni prubeh    
    
    la	    a0, pwm_table
    move    t0, zero
    addiu   t1, zero, CCP_PWM_COUNT		//pocet pwm (polozek v pwm_table)
    
1://loop:    
    lbu	    t7, 0(a0)				//load timebase (0=neplatna polozka)
    beqz    t7, 4f				//skok, neplatna polozka
    nop
    
    //tato polozka je platna
    lbu	    t8, 1(a0)				//load time
    addiu   t8, -1				//time --
    bnez    t8, 2f//x0				//skok, jeste nenastal time
    nop
   
    //time=0, provede dalsi krok pwm
    sb	    t7, 1(a0)				//nastav novy time
    lw	    t9, 8(a0)				//load fn
    jr	    t9					//skok na fn linear_up, linear_down, ...

2://x0:
    //skok sem, pokud jeste nevyprsel time (t8=novy time)
    sb	    t8, 1(a0)				//save time
3://x:
    //skok sem, pokud pwm jeste nedosahl target
    addiu   t0, 1
    
4://next:    
    //skok sem, pokud pwm jiz dosahl target
    addiu   t1, -1				//max_items --
    addiu   a0, CCP_PWM_TABLE_ISIZE		//item ++
    bnez    t1, 1b //loop			//skok, na dalsi polozku
    nop
    
    //byla posledni polozka
    bnez    t0, 5f//noclr_fn
    nop
    
    //zadny pwm neni aktivni, zrusi volani fce z timer1
    sw	    zero, (pwm_soft_fn)
    
5://noclr_fn:
    jrc	    ra
    nop
    
    //<editor-fold defaultstate="collapsed" desc="pwm_soft_linear_down">
.global pwm_soft_linear_down   
pwm_soft_linear_down:
    //a0=adresa polozky v tabulce
    lw	    t8, 12(a0)		    //adresa value registru
    lw	    t9, (t8)		    //load obsah value reg
    lw	    t6, 4(a0)		    //load step
    subu    t9, t6		    //nova value
    
    lhu	    t6, 2(a0)		    //load target value
    subu    t5, t9, t6
    bgtz    t5, 6f//e2		    //skok, val jeste neni target
    nop
    
    //target je jiz dosazena
    sw	    t6, (t8)		    //save reg=target
    sw	    zero, 0(a0)		    //timebase=0, zneplatni polozku
    nop
    b16	    4b//next
    nop
    
6://e2:    
    //valu jeste neni target
    sw	    t9, (t8)		    //save value do CCP reg
    nop
    b16	    3b//x
    nop 
    //</editor-fold>
    
    //<editor-fold defaultstate="collapsed" desc="pwm_soft_linear_up">
.global pwm_soft_linear_up    
pwm_soft_linear_up:    
    //a0=adresa polozky v tabulce
    lw	    t8, 12(a0)		    //adresa value registru
    lw	    t9, (t8)		    //load obsah value reg
    lw	    t6, 4(a0)		    //load step
    addu    t9, t6		    //nova value
    
    lhu	    t6, 2(a0)		    //load target value
    subu    t5, t6, t9
    bgtz    t5, 7f//e1		    //skok, val jeste neni target
    nop
  
    //target je jiz dosazena
    sw	    t6, (t8)		    //save reg=target
    sw	    zero, 0(a0)		    //timebase=0, zneplatni polozku
    nop
    b16	    4b//next
    nop
    
7://e1:    
    //valu jeste neni target
    sw	    t9, (t8)		    //save value do reg
    nop
    b16	    3b//x
    nop
    //</editor-fold>

    //<editor-fold defaultstate="collapsed" desc="pwm_soft_exp">
.global pwm_soft_exp
pwm_soft_exp:
    //a0=adresa polozky v pwm_table
    lw	    t8, 12(a0)		    //adresa value registru
    lbu	    t9, 2(a0)		    //target index
    lbu	    t7, 4(a0)		    //akt. index
    lb	    t6, 5(a0)		    //step (byte se znamenkem)
    
    add	    t7, t6		    //index + step (+1,-1,0)
    sb	    t7, 4(a0)		    //save novy index
    
    lw	    t6, 16(a0)		    //adresa tabulky dat 
    sll	    t5, t7, 1		    //akt. index x 2 (16-bit data)
    addu    t6, t5		    //base + offset	
    lhu	    t5, (t6)		    //load value data
    sw	    t5, (t8)		    //save value do CCP reg
    
    bne	    t7, t9, 3b		    //skok, jeste neni target  
    nop
    
    //akt. index= target index
    sw	    zero, (a0)		    //zneplatni polozku
    nop
    b16	    4b//next
    nop    
    //</editor-fold>
  
.end pwm_soft_timer
    
#endif    
//</editor-fold>
    

//<editor-fold defaultstate="collapsed" desc="iVector11 timer1_interrupt">
.align 2    
.global iVector_timer1   
.ent iVector_timer1
iVector_timer1:
    
    //#define INTERVAL	10		//10ms
    
    lw	    v0, (timer_ms)	    
    addiu   v0, TIMER1_INTERVAL		//timer_ms += TIMER1_INTERVAL (10ms)
    sw	    v0, (timer_ms)
    
#ifdef RTC
    lw	    v0, (day_ms)
    addiu   v0, TIMER1_INTERVAL
    sw	    v0, (day_ms)
#endif    
   
//word addr, word ms, word ms value    
    la	    v0, timer1_events
    addiu   v1, zero, TIMER1_EVENT_CAPA
    li16    a0, 1
    move    t0, zero			//priznak, zda byla vyrazena udalost
    	
1:    
    lw	    t9, (v0)			//adresa promene, kterou nastavi na 1, pokud nastala udalost
    beqz    t9, 4f			//skok, neplatna polozka, konec dat
    nop
   
    //platna polozka
    lw	    t8, 8(v0)			//load aktualni value (citac)
    addiu   t8, -TIMER1_INTERVAL	//citac --
    sw	    t8, 8(v0)			//save citac
    bgtz    t8, 3f			//skok, neni 0, zadna udalost
    nop
    
    //nastala udalost
    sb	    a0, (t9)			//nastav status byte registrovane udalosti
    lw	    t8, 4(v0)			//load default value (pokud je 0, neni udalost repeat)
    bnez    t8, 2f			//skok, je repeat
    nop
    
    //t8=0, udalost neni repeat, zrusit 
    sw	    zero, (v0)
    addiu   t0, 1			//nastav priznak zruseni event

2:
    //je repeat, t8=default value (provede i kdyz neni repeat, nevadi to)
    sw	    t8, 8(v0)
    
3:
    //next
    addiu   v0, TIMER1_EVENT_ISIZE	//dalsi polozka
    addiu   v1, -1			//items--
    bnez    v1, 1b			//loop
    nop
    
4:    
    beqz    t1, 5f			//skok, nebyla zrusena zadna udalost
    nop
    //byla zrusena udalost (nebyla repeat), defrag. tabulku
    la	    a0, timer1_events
    addiu   a1, zero, TIMER1_EVENT_ISIZE
    addiu   a2, zero, TIMER1_EVENT_CAPA
    //a0=adresa tabulky, a1=velikost polozky, a2=max. pocet polozek (kapacita)
    //bal	    defragTableW
    nop

5:    
    
#ifdef PWM_SOFT
 
    //skok do fce pro soft rizeni pwm, pouze pokud je adresa fce nastavena
    
    lw	    t9, (pwm_soft_fn)
    beqz    t9, 6f
    nop
    jalr    t9
    nop
6:    
    
#endif    
    
    
    //konec, nuluj flag IFS0.4
#ifdef PIC32MZ    
    li16    v0, 0x10
    sw      v0, (IFS0CLR)
#endif    
   
#ifdef PIC32MM0064   
    li	    v0, (1 << 11)
    sw      v0, (IFS0CLR)
#endif     
    
#ifdef PIC32MM0256   
    li	    v0, (1 << 17)
    sw      v0, (IFS0CLR)
#endif     
    
    
    eret
    nop
  
.end iVector_timer1
  
//</editor-fold>        
  
//<editor-fold defaultstate="collapsed" desc="iVector_rtc RTC alarm">

.align 2    
.global iVector_rtc   
.ent iVector_rtc
iVector_rtc:
    //allarm jednou za minutu pri sec=00
    
    //nastav day_ms, podle aktualniho casu
    mflo    s0			//uchova Lo
    mfhi    s1			//uchova Hi
    
    bal	    rtc_setDayMs	//meni HiLo
    nop
    
    mthi    s1			//obnov Hi
    mtlo    s0			//obnov Lo
    
    
    lw	    v0, (minute_event);
    beqz    v0, 1f		//skok, neni zadna fce minute_event
    nop
    
    jalr    v0
    nop
    
1:
    
#ifdef PIC32MM0064    
    li	    v0, (1 << 14)	//nuluj flag
    sw	    v0, (IFS0CLR)
#endif
    
#ifdef PIC32MM0256    
    li	    v0, (1 << 0)	//nuluj flag
    sw	    v0, (IFS1CLR)
#endif    
    
    
    eret
    nop
    
.end iVector_rtc
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="iVector15 ADC">
    
.align 2    
.global iVector_adc  
.ent iVector_adc
iVector_adc:
//AD converter provadi pravidelnou kalibraci, nastavenim AD1CON2.OFFCAL bitu
//vstup AD (SHA) se tim pripoji na Vss, prevedena hodnota by mela byt 0,
//ale vetsinou je o neco vetsi. Tato hodnota je calibData, o kterou bude
//zmenseny vysledek AD prevodu, az do dalsi kalibrace 
//proto muze byt vysledek AD prevodu i zaporny    
    
    #define ADC_BUFFES_SPACE	0x10		    //offset jednotlivych buffer registru
    #define ADC_CALIB_COUNTER	0b11111111111	    //hodnota pro kalibraci, jsou-li stejne bity v adc_counter nulove

    /*
    //test, zda bude kalibrace
    lw	    v0, (adc_counter)
    andi    v0, ADC_CALIB_COUNTER	    //nuluje horni bity
    bnez    v0, 2f			    //skok, neni kalibrace
    nop
    

    
    //test, zda je prvni, nebo druhy krok kalibrace (v prvnim nastavi OFFCAL, v druhem cte data)
    lw	    v0, (AD1CON2)
    ext	    v1, v0, 12, 1
    bnez    v1, 1f			    //skok, OFFCAL je jiz nastaveno
    nop
    
    //dalsi AD converze bude kalibrace
    addiu   v0, zero, (1 << 12)
    sw	    v0, (AD1CON2SET)		    //nastav OFFCAL AD1CON2.b12
    nop
    b16	    5f				    //eret
    nop
    
1:
    //aktualni AD konverze byla kalibracni
    
    addiu   v0, zero, (1 << 12)
    //sw	    v0, (AD1CON2CLR)		    //nuluj OFFCAL  AD1CON2.b12
    
    lw	    v1, (ADC1BUF0)		    //kalibracni data
    sw	    v1, (calibData)		    //ulozit kalibracni data
    
    //adc_counter++
    lw	    v0, (adc_counter)
    addiu   v0, 1
    //sw	    v0, (adc_counter)
    
    nop
    b16	    5f				    //eret
    nop    
    
    

    
2:
    //adc_counter++, v0=adc_counter
    addiu   v0, 1
    sw	    v0, (adc_counter)
    */
    
    la	    v0, (ADC1BUF0)
    la	    v1, (adc_values)
    lw	    t8, (calibData)
    
#ifdef AN0   
    lw	    t9, (v0)
    sw	    t9, (v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif    
    
#ifdef AN1    
    lw	    t9, (v0)
    sw	    t9, (1*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif        
    
#ifdef AN2    
    lw	    t9, (v0)
    sw	    t9, (2*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif      
    
#ifdef AN3    
    lw	    t9, (v0)
    sw	    t9, (3*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif     
    
#ifdef AN4    
    lw	    t9, (v0)
    sw	    t9, (4*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif     
    
#ifdef AN5    
    lw	    t9, (v0)
    sw	    t9, (5*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif     
    
#ifdef AN6    
    lw	    t9, (v0)
    sw	    t9, (6*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif     
    
#ifdef AN7    
    lw	    t9, (v0)
    sw	    t9, (7*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif     
    
#ifdef AN8    
    lw	    t9, (v0)
    sw	    t9, (8*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif     
    
#ifdef AN9    
    lw	    t9, (v0)
    sw	    t9, (9*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif     
    
#ifdef AN10    
    lw	    t9, (v0)
    sw	    t9, (10*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif     
    
#ifdef AN11    
    lw	    t9, (v0)
    sw	    t9, (11*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif     
    
#ifdef AN12    
    lw	    t9, (v0)
    sw	    t9, (12*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif     
    
#ifdef AN13    
    lw	    t9, (v0)
    sw	    t9, (13*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif  
    
#ifdef AN14    
    lw	    t9, (v0)
    sw	    t9, (14*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif      
    
#ifdef AN15    
    lw	    t9, (v0)
    sw	    t9, (15*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif  
    
#ifdef AN16    
    lw	    t9, (v0)
    sw	    t9, (16*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif      
    
#ifdef AN17    
    lw	    t9, (v0)
    sw	    t9, (17*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif      
    
#ifdef AN18    
    lw	    t9, (v0)
    sw	    t9, (18*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif      
    
#ifdef AN19    
    lw	    t9, (v0)
    sw	    t9, (19*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif      
    
#ifdef ANVDDCORE
    lw	    t9, (v0)
    sw	    t9, (27*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif 
    
#ifdef ANVBG
    lw	    t9, (v0)
    sw	    t9, (28*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif    
 
#ifdef ANVSS
    lw	    t9, (v0)
    sw	    t9, (29*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif      
    
#ifdef ANVDD
    lw	    t9, (v0)
    sw	    t9, (30*4)(v1)
    addiu   v0, ADC_BUFFES_SPACE
#endif  
    
5:    
    
#ifdef PIC32MM0064    
    li	    v0, (1 << 15)	//nuluj flag
    sw	    v0, (IFS0CLR)
#endif
  
#ifdef PIC32MM0256    
    li	    v0, (1 << 1)	//nuluj flag
    sw	    v0, (IFS1CLR)
#endif    
    
    
    eret
    nop
    
.end iVector_adc 
    
//</editor-fold>
  
//<editor-fold defaultstate="collapsed" desc="iVector0 CPU Timer">
.align 2    
.global iVector0
.ent iVector0
iVector0:
    
    //aktualni proces trva prilis dlouho, neprovedl doEvents
    nop
    b	    general_exception
    nop
    
.end iVector0  
    
//</editor-fold>
  
    
//<editor-fold defaultstate="collapsed" desc="Interrupt MM">         
   
//<editor-fold defaultstate="collapsed" desc="setSrsValue">         
.align 2    
.global setSrsValue    
.ent setSrsValue 
setSrsValue:
    
    //nastavi SP a GP pro SRS1, ktery pouziva interrupt

    //nastav previous SRS=1
    li	    v0, 0x1   
    mfc0    v1, _CP0_SRSCTL
    ins	    v1, v0, 6, 4
    mtc0    v1, _CP0_SRSCTL
    ehb
    
    //zapis gp
    //la	    v0, (gp_value)
    lw	    v0, (v0)			    //k0=gp_value
    wrpgpr  gp, v0
    
    //zapis sp
    //la	    v0, (sp_srs1_top)
    lw	    v0, (v0)			    //k0=adresa stacku
    wrpgpr  sp, v0
    
    jrc	    ra
    
.end setSrsValue    
//</editor-fold>   
    
//<editor-fold defaultstate="collapsed" desc="setSrsValue2">         
    
.align 2    
.global setSrsValue2    
.ent setSrsValue2 
setSrsValue2:
//a0=SP pro SRS[1]    
    
    //nastavi SP a GP pro SRS1, ktery pouziva interrupt
    //GP kopiruje ze SRS[0]

    //nastav previous SRS=1
    li16    v0, 0x1   
    mfc0    v1, _CP0_SRSCTL
    ins	    v1, v0, 6, 4
    mtc0    v1, _CP0_SRSCTL
    ehb
    
    //zapis gp
    wrpgpr  gp, gp
    
    //zapis sp
    wrpgpr  sp, a0
    
    jrc	    ra
    
.end setSrsValue2
    
//</editor-fold>     
    
    
//<editor-fold defaultstate="collapsed" desc="setInterrupt, enableInterrupt">
.align 2    
.global setInterrupt
.ent setInterrupt
setInterrupt:
  
    //Vector spacing=8, MVEC=1, prox. timer disable
    li	    v0, 0x00011000
    sw	    v0, (INTCON)
    
    //b4-b31
    //level 1(CPU timer)SRS0
    //level 2-7 SRS1
    //b0-b3 use SRS
    li	    v0, 0x11111110
    sw	    v0, (PRISS)
    
    mtc0    v0, _CP0_SRSMAP
    ehb
    
    jrc	    ra
    
.end setInterrupt
    
.align 2    
.global enableInterrupt
.ent enableInterrupt
enableInterrupt:
    
    //enable interrupt
    ei
    jrc	    ra
    
.end enableInterrupt    
    
    
//</editor-fold>
   
//<editor-fold defaultstate="collapsed" desc="interrupt vectors">
    
//tabulka vektoru je pevne na adrese 0x9D00 0200, nezavisle na EBASE (na rozdil od dokumenatce)    
//compilator vlozi skok na fci obsluhy takto: extern void __attribute__((vector(21))) asm_fn_name();
//asm_fn muze volat c_fn, ale zpracovani musi koncit ERET (tzn. mimo telo c_fn)
//v prubehu zpracovani interruptu je STATUS.EXL=1, dalsi interrupty cekaji na dokonceni predchoziho   
//vsechny IPL pouzivaji SRS[1], kde je nastaven SP a GP (vsechny obsluhy pouzivaji stack[512] vyhrazeny pouze pro interrupt)
    
#ifdef PIC32MM    
    
//<editor-fold defaultstate="collapsed" desc="SPI1Tx">
.align 2    
.global iVector_spi1Tx
.ent iVector_spi1Tx 
iVector_spi1Tx:   

    //SPI Tx
#ifdef SPI1_USE    
    bal	    spi1_TxInterrupt
    nop
#endif
    
    eret
    nop
    
.end iVector_spi1Tx
    
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="SPI2Tx">
.align 2    
.global iVector_spi2Tx
.ent iVector_spi2Tx 
iVector_spi2Tx:   

    //SPI Tx
#ifdef SPI2_USE      
    bal	    spi2_TxInterrupt
    nop
#endif    
    
    eret
    nop
    
.end iVector_spi2Tx   
    
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="SPI3Tx">
.align 2    
.global iVector_spi3Tx
.ent iVector_spi3Tx 
iVector_spi3Tx:   

    //SPI Tx
#ifdef SPI3_USE      
    bal	    spi3_TxInterrupt
    nop
#endif    
    
    eret
    nop
    
.end iVector_spi3Tx   
    
//</editor-fold>    
    
#endif 
    
#ifdef PIC32MM0256		    //I2C ma pouze PIC32MM0256
    
//<editor-fold defaultstate="collapsed" desc="I2C1Slave">
.align 2
.global iVector_i2c1Slave    
.ent iVector_i2c1Slave
iVector_i2c1Slave:
    
    //I2C1 Slave
#ifdef I2C1_USE    
    bal	    i2c1_SlaveInterrupt
    nop
#endif    
    
    eret
    nop
    
.end iVector_i2c1Slave
    
//</editor-fold>    
//<editor-fold defaultstate="collapsed" desc="I2C1Master">
.align 2
.global iVector_i2c1Master    
.ent iVector_i2c1Master
iVector_i2c1Master:
    
    //I2C1 Master
#ifdef I2C1_USE    
    bal	    i2c1_MasterInterrupt
    nop
#endif    
    
    eret
    nop
    
.end iVector_i2c1Master  
    
//</editor-fold>
//<editor-fold defaultstate="collapsed" desc="I2C1Bus">
.align 2
.global iVector_i2c1Bus    
.ent iVector_i2c1Bus
iVector_i2c1Bus:
    
    //I2C1 Bus
#ifdef I2C1_USE    
    bal	    i2c1_BusInterrupt
    nop
#endif    
    
    eret
    nop
    
.end iVector_i2c1Bus 
    
//</editor-fold>    
    
//<editor-fold defaultstate="collapsed" desc="I2C2Slave">
.align 2
.global iVector_i2c2Slave    
.ent iVector_i2c2Slave
iVector_i2c2Slave:
    
    //I2C2 Slave
#ifdef I2C2_USE    
    bal	    i2c2_SlaveInterrupt
    nop
#endif    
    
    eret
    nop
    
.end iVector_i2c2Slave
    
//</editor-fold>
//<editor-fold defaultstate="collapsed" desc="I2C2Master">
.align 2
.global iVector_i2c2Master    
.ent iVector_i2c2Master
iVector_i2c2Master:
    
    //I2C2 Master
#ifdef I2C2_USE    
    bal	    i2c2_MasterInterrupt
    nop
#endif    
    
    eret
    nop
    
.end iVector_i2c2Master  //</editor-fold>
//<editor-fold defaultstate="collapsed" desc="I2C2Bus">
.align 2
.global iVector_i2c2Bus    
.ent iVector_i2c2Bus
iVector_i2c2Bus:
    
    //I2C2 Bus
#ifdef I2C2_USE    
    bal	    i2c2_BusInterrupt
    nop
#endif    
    
    eret
    nop
    
.end iVector_i2c2Bus 
    
//</editor-fold>
  
//<editor-fold defaultstate="collapsed" desc="I2C3Slave">
.align 2
.global iVector_i2c3Slave    
.ent iVector_i2c3Slave
iVector_i2c3Slave:
    
    //I2C3 Slave
#ifdef I2C3_USE    
    bal	    i2c3_SlaveInterrupt
    nop
#endif    
    
    eret
    nop
    
.end iVector_i2c3Slave
    
//</editor-fold>
//<editor-fold defaultstate="collapsed" desc="I2C3Master">
.align 2
.global iVector_i2c3Master    
.ent iVector_i2c3Master
iVector_i2c3Master:
    
    //I2C3 Master
#ifdef I2C3_USE    
    bal	    i2c3_MasterInterrupt
    nop
#endif    
    
    eret
    nop
    
.end iVector_i2c3Master  //</editor-fold>
//<editor-fold defaultstate="collapsed" desc="I2C3Bus">
.align 2
.global iVector_i2c3Bus    
.ent iVector_i2c3Bus
iVector_i2c3Bus:
    
    //I2C3 Bus
#ifdef I2C3_USE    
    bal	    i2c3_BusInterrupt
    nop
#endif    
    
    eret
    nop
    
.end iVector_i2c3Bus 
    
//</editor-fold>  
    
    
//<editor-fold defaultstate="collapsed" desc="USB">
.align 2
.global iVector_usb    
.ent iVector_usb
iVector_usb:
    
#ifdef USB_MM    
    bal	    usb_mm_interrupt
    nop
#endif    
    
    eret
    nop
    
.end iVector_usb  
//</editor-fold>
  
    
#endif     
    
//</editor-fold>
   
    
//<editor-fold defaultstate="collapsed" desc="cpuTimer_Init">
.align 2
.global cpuTimer_Init
.ent cpuTimer_Init
cpuTimer_Init:
    
    li	    v0, 0xFFFFFFFF
    mtc0    v0, _CP0_COMPARE
    ehb
    mtc0    zero, _CP0_COUNT
    ehb

    lw	    v0, (IPC0)
    ori	    v0, 0b00100			//IPC0.PRI=1, IPC0.SUB=0
    sw	    v0, (IPC0)
    
    li16    v0, 1
    sw	    v0, (IEC0SET)		//IEC0.b0 = 1, enable
    
    sw	    v0, (IFS0CLR)		//IFS0.b0 = 0, nuluj flag
    
    jrc	    ra
   
.end cpuTimer_Init
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="general_exception">
.align 2    
.global _general_exception_handler    
.ent _general_exception_handler    
_general_exception_handler:    
    
.global general_exception
general_exception:
 
    
#ifdef ENABLE_APP_RESTART_ON_ERROR    
    
    //provede nove spusteni procesu (v dalsim loop)
    la	    k1, proc_t_pos	    //zde je ulozena akt. pozice v proc_t
    lw	    k0, (k1)		    //process table
    
    lwp	    v0, TH_T_START_ADDR*4(k0)
    move    ra, v0
    move    sp, v1
    
    la	    v0, doEvents
    mtc0    v0, _CP0_EPC
    ehb
    
    eret
    nop
    
#else

1:
    //stop
    li	t9, 0x1
    li	t8, 0x2
    
    nop
    b16	    1b
    nop
    
#endif    
   
.end _general_exception_handler  
    
//</editor-fold>
 
   