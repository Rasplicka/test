#include <xc.h>
#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "main.h"
//#include "def.h"
#include "asm.h"
#include "fnc.h"
#include "graphics.h"                 //display modul
#include "disp1306a.h"
#include "spi.h"
#include "i2c.h"
#include "timer.h"
#include "pwm.h"
#include "usb_mm.h"
#include "disp9341.h"


// <editor-fold defaultstate="collapsed" desc="OS vars">

//RAM sekce pro system (pouziva sekci .jcr, kterou linker vklada na zacatek RAM)
uint* proc_t_pos __section(".jcr") = 0;
uint* proc_t_max __section(".jcr") = 0;
char proc_t_count __section(".jcr") = 0;
uint proc_t[(PROC_T_ISIZE / 4) * PROC_T_CAPA] __section(".jcr"); //8 x 88B
//uint gp_value __section(".jcr") = 0;
//char* sp_srs1_top __section(".jcr") = 0;

//stack_list pro pouziti pri allocStack, stack pro kazdy proces je alokovan ve volne RAM
int stack_list[PROC_T_CAPA] __section(".jcr");                  //kazda polozka (32-bit) definuje velikost jednoho stacku

//stack pro interrupt SRS1 je mimo ostatni stacky
#define     _SRS1_STACK_SIZE        512
char stack_interrupt_srs1[_SRS1_STACK_SIZE] __section(".jcr");

//stack je posledni oblast RAM, stack ma definovanou velikost STACK_SIZE
char stack_area[STACK_SIZE] __at(RAM_BASE + RAM_SIZE - STACK_SIZE) __section(".os_stack");

// </editor-fold>


//app1
extern void m1_start();
//app2
extern void m2_start(); 
//app3
extern void m3_start(); 

extern void touchXpt2046_start();

struct
{
    unsigned Threading : 1;
    unsigned dummy     : 7;
    unsigned Errors    : 8; 
    
}SYSTEM_STATUS;

//local fn
char  reg_process(int* start_addr, int size);
char getFreeProcessID();
void system_Init();
void setClock();


void main()
{
    //GND novy komit 5
    //+
    //CLK       17, RP12
    //DATA      18, RP13
    //RST       19, RP14 B9
    //DC        21, RP18 C9

    //MZ test LED
    //TRISH=0b1111111111111000;
    //PORTH=0x0;
    //LATHSET=0b000;
            
    //char* string="text";
    //disp1306a_drawText(0, string, 0, 0, 1);
    
    setClock();
    
#ifdef SAFE_PROCESS    
    cpuTimer_Init();
#endif
    system_Init();                                  //init interrupt (ale zustane DI), nastav SRS1, ...
    timer1_Init();
    initPort();
    
    spi_Init();
    //i2c_Init();
    //adcscan_Init();

   
#ifdef PWM    
    pwm_Init();
#endif  
    
#ifdef USB_MM
    usb_mm_Init();
#endif    
    
    enableInterrupt();                              //provede EI
    globals_Start();
    
    //disp9341_Init();
    
    //reg_process((int*)&disp9341a_start, 1024);
    
    //reg_process((int*)&disp1306a_start, 1024); 
    reg_process((int*)&m1_start, 1024); 
    reg_process((int*)&m2_start, 1024);
    reg_process((int*)&m3_start, 1024);
    //reg_process((int*)&ubtn_start, 512);          //dve tlacitka A2, A3
    reg_process((int*)touchXpt2046_start, 512);
    //reg_process((int*)&m2, 1024);
    //reg_process((int*)&m3, 1024);
    
    
    SYSTEM_STATUS.Threading=1;
    startEvents();
    
    while(1)
    {
        
    }
}

char reg_process(int* start_addr, int stack_size)
{
    //prvede registraci procesu v proc_t
    //vlozi do proc_t adresu start fce a vychozi hodnotu pro stack(top adresa)
    //vraci ID procesu, nebo -1 pri chybe
    
    //test, zda je volne misto v proc_t
    if(proc_t_count >= PROC_T_CAPA) { return -1; } 
    
    char id=getFreeProcessID();
    int* tab=proc_t + ((proc_t_count) * (PROC_T_ISIZE/4));     //adresa polozky proc_t
    proc_t_count++;
    
    allocStack(stack_size, tab);                //nastavuje SP a START_SP
    tab[TH_T_ID]=id;
    tab[TH_T_RA]=(int)start_addr;
    tab[TH_T_GP]=getGP();
    
    tab[TH_T_START_ADDR]=(int)start_addr;       //START_RA pro pripad restartu app
    
    proc_t_max=tab;
    
    return id;
}

char getFreeProcessID()
{
    //v proc_t musi byt alespon jedno volne misto
    //prvni nulova hodnota ID znamena konec platnych polozek (volne polozky pouze na konci tabulky)
    char* proc_t_bytes=(char*)proc_t;
    
    char id=1;
    while(1)
    {
        char exist_id=proc_t_bytes[0];
        if(id==exist_id)
        {
            //nasel stejne ID, cele znova
            id++;                                   //nove id
            proc_t_bytes=(char*)proc_t;             //nastav zacatek proc_table
        }
        else if(exist_id==0)
        {
            //nasel konec tabulky, pouzije ID
            break;
        }
        else
        {
            //jdi na dalsi polozku
            proc_t_bytes += PROC_T_ISIZE;
        }
    }
    
    return id;
}

void system_Init()
{
    char* sp_srs1 = stack_interrupt_srs1 + _SRS1_STACK_SIZE - 4;
    
    //nastavi vychozi hodnoty GP a SP pro SRS[1]
    //nastav GP SRS[1] na stejnou hodnotu, jako SRS[0]
    //nastav SP SRS[1]
    setSrsValue2(sp_srs1);

    //Multivector, spacing 8 bytes, IPL 1-7 pouziva SRS[1]
    //Neobsahuje EI, STATUS.EI zustava 0 (interrupt disable)
    setInterrupt();
    
}

void blick()
{
    //RB5, RB7, RC3
    
    LATA = 0xFFFF;
    LATB = 0xFFFF;
    LATC = 0xFFFF;
    
    ANSELA=0;
    ANSELB=0;
    ANSELC=0;

    TRISBbits.TRISB5=0;     //LED   pin 14
    TRISBbits.TRISB7=0;
    TRISCbits.TRISC3=0;
    
    LATBINV=0b10100000;
    LATCINV=0b1000;
    
    while(1)
    {
        int a, b;
        for(a=0; a<1500000; a++)
        {
            b=a+1;
        }
        
        LATBINV=0b10100000;
        LATCINV=0b1000;
    }
    
}

void setClock()
{
    //PIC32MM0256
    //Nastavi PLLMULT (f pro USB musi byt 96 MHz)
    //PLLODIV = /4    (f pro CPU max. 24 MHz)
    
    //SYSKEY = 0x00000000; // force lock
    SYSKEY = 0xAA996655; // unlock
    SYSKEY = 0x556699AA;
    
    //nastaveni pro POSC Q=12MHz: SPLLCONbits.PLLMULT = 0x4 (externi krystal 12 MHz)
    //natsaveni pro FRC     8MHz: SPLLCONbits.PLLMULT = 0x6 (podle doc. by melo byt 0x5)
    
    SPLLCONbits.PLLMULT=0x6;        // x 12 (8x12=96 MHz) USB
    SPLLCONbits.PLLODIV=0x2;        // / 4  (96/4=24 MHz) CPU
    
    SYSKEY = 0x00000000;            //force lock
    
    while(REFO1CONbits.ACTIVE==1)
    { }
    
    //96MHz
    REFO1CONbits.ROSEL=7;
    //na vstupu DIV je 48MHz
    REFO1CONbits.RODIV=0;                   //max. freq.
    //REFO1CONbits.RODIV=0x8;               // /8
    
    REFO1TRIMbits.ROTRIM=0;
    REFO1CONbits.ON=1;
}