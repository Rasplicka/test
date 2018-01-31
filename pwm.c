
#include <xc.h>
#include <stdio.h>
//#include <stdlib.h>
#include "def.h"
#include "asm.h"
#include "pwm.h"

#ifdef PWM                  

#define     PERIOD              4096                            //regulace 0 - 4096

#ifdef PWM_SOFT
//pro PWM_SOFT plati:
//fce v tomto modulu nastavi data v tabulce pwm_table a asm fce pwm_soft_timer
//zajisti soft zmenu vykonu. pwm_soft_timer je volana z timer1 interruptu

//byte[0]     timebase, cas kroku
//byte[1]     aktualni time value
//byte[2-3]   (linear) target value / (exp) target index
//byte[4-7]   (linear) step / (exp) step(+1,-1,0), aktual index
//byte[8-11]  adresa fce pwm_soft_linear_down, pwm_soft_linear_up, pwm_soft_exp
//byte[12-15] adresa CCPxRB registru - obsahuje value   
//byte[16-19] (exp) adresa pole dat pro jiny, nez linearni prubeh, definuje hodnoty pro jednotlive indexy  

int   pwm_table[(CCP_PWM_TABLE_ISIZE/4) * CCP_PWM_COUNT];       //CCP_PWM_TABLE_ISIZE velikost bytes
void* pwm_soft_fn=NULL;                                         //adresa pwm_soft_timer, nebo null


#define     EXP_MAX             37                             //max. index pwm_exp
//const short pwm_exp[]={0,1,2,3,4,6,8,10,12,14,16,20,24,28,32,40,48,56,64,80,96,112,128,170,
//                       212,256,341,426,512,682,852,1024,1536,2048,3072,4096}; //36 polozek
//const short pwm_exp[]={0,1,2,3,4,6,8,10,12,14,16,20,24,28,32,40,48,56,64,80,96,112,128,160,
//                       200,256,300,390,512,640,820,1024,1250,1550,2048,2450,2950,3500,4096}; //39 polozek
//const short pwm_exp[]={0,2,4,8,13,19,25,32,40,50,64,80,100,128,160,
//                       200,256,300,390,512,640,820,1024,1250,1550,2048,2450,2950,3500,4096}; //30 polozek

const short pwm_exp[]={0,2,5,10,15,22,27,35,43,52,62,80,110,140,170,
                       200,240,300,350,400,500,600,700,800,900,1000,1200,1400,1600,1800,
                        2000, 2200, 2400, 2800, 3000, 3200, 3500, 4096}; //38 polozek

//stejne jako power_exp lze vytvorit jine prubehy



void pwm_setPowerLinearProc(char index, int target_proc, int step_ms, int step_size)
{
    //plynula zmena vykonu na vysledny, zadany v procentech
    //speed -rychlost zmeny 1=10ms, 2=20ms (nastaveni timer1_event=10ms, tj. nejmensi interval)
    //step_size - velikost zmeny 1 = 1/4096
    
    int target_val=target_proc * PERIOD / 100;
    pwm_setPowerLinear(index, target_val, step_ms, step_size);
}

void pwm_setPowerLinear(char index, int target_val, int step_ms, int step_size)
{
    //target = cilovy power
    //speed -rychlost zmeny 1=10ms, 2=20ms (nastaveni timer1_event=10ms, tj. nejmensi interval)
    //step_size - velikost zmeny 1 = 1/4096
    
    //pozastavi volani pwm fce (volane z timer1 interruptu) na dobu prenastavovani dat
    pwm_soft_fn=NULL;
    
    //prepocet vykonu z procent na target value
    //int target_val=target_proc * PERIOD / 100;
    if(target_val>PERIOD) { target_val = PERIOD; }
    if(target_val<0) { target_val = 0; }
    
    int pwm_table_index=0;
    
#ifdef CCP1_PWM    
    if(index==0)
    {
        int val=CCP1RB;
        pwm_table[pwm_table_index+0]=target_val<<16 | step_ms<<8 | step_ms;
        
        pwm_table[pwm_table_index+3]=(int)&CCP1RB;
        
        pwm_table[pwm_table_index+1]= step_size;          
        if(val>target_val)
        {
            //down
            pwm_table[pwm_table_index+2]=(int)&pwm_soft_linear_down;
        }
        else
        {
            //up
            pwm_table[pwm_table_index+2]=(int)&pwm_soft_linear_up;  
        }
    }
    pwm_table_index+=(CCP_PWM_TABLE_ISIZE/4);
#endif    
    
#ifdef CCP2_PWM    
    if(index==1)
    {
        int val=CCP2RB;
        pwm_table[pwm_table_index+0]=target_val<<16 | step_ms<<8 | step_ms;
        
        pwm_table[pwm_table_index+3]=(int)&CCP2RB;
        
        pwm_table[pwm_table_index+1]= step_size;          
        if(val>target_val)
        {
            //down
            pwm_table[pwm_table_index+2]=(int)&pwm_soft_linear_down;
        }
        else
        {
            //up
            pwm_table[pwm_table_index+2]=(int)&pwm_soft_linear_up;  
        }
    } 
    pwm_table_index+=(CCP_PWM_TABLE_ISIZE/4);
#endif    
    
#ifdef CCP3_PWM    
    if(index==2)
    {
        int val=CCP3RB;
        pwm_table[pwm_table_index+0]=target_val<<16 | step_ms<<8 | step_ms;
        
        pwm_table[pwm_table_index+3]=(int)&CCP3RB;
        
        pwm_table[pwm_table_index+1]= step_size;          
        if(val>target_val)
        {
            //down
            pwm_table[pwm_table_index+2]=(int)&pwm_soft_linear_down;
        }
        else
        {
            //up
            pwm_table[pwm_table_index+2]=(int)&pwm_soft_linear_up;  
        }
    }      
#endif    
    
    //timer1 interrupt vola fci pwm_soft_timer, po skonceni pwm_soft tento ukazatel nuluje, timer1 ji prestane volat 
    //pwm_soft_timer pracuje v interruptu (timeru1)
    pwm_soft_fn = &pwm_soft_timer;
}

void pwm_setPowerExp(char index, int target_index, int step_ms)
{
    //target_index je cilovy target, value bude zjistena z tabulky dat (akt. index je urcen automaticky)
    //rizeni vykonu podle tabulky dat, zmena vykonu znemou indexu dat
    
    //pozastavi volani pwm fce (volane z timer1 interruptu) na dobu prenastavovani dat
    pwm_soft_fn=NULL;
    
    int pwm_table_index=0;
    
    if(target_index<0) { target_index = 0; }
    if(target_index>EXP_MAX) { target_index = EXP_MAX; }
    
#ifdef CCP1_PWM    
    if(index==0)
    {
        int val=CCP1RB;
        
        //najdi aktualni index, prvni polozka data, ktera je vetsi nebo rovna
        int vali=-1, a;
        for(a=0; a<=EXP_MAX; a++)
        {
            if(val<=pwm_exp[a]) { vali=a; break; }
        }
        
        pwm_table[pwm_table_index+0]=target_index<<16 | step_ms<<8 | step_ms;  //target_index, time, timebase
        pwm_table[pwm_table_index+2]=(int)&pwm_soft_exp;                       //adresa fce
        pwm_table[pwm_table_index+3]=(int)&CCP1RB;                             //adresa registru
        pwm_table[pwm_table_index+4]=(int)pwm_exp;                             //adresa pole data
        
        if(vali<target_index)
        {
            //step=+1, aktualni index
            pwm_table[pwm_table_index+1]= 1<<8 | vali;
        }
        else if(vali>target_index)
        {
            //step=-1, aktualni index
            pwm_table[pwm_table_index+1]= -1<<8 | vali;
        }
        else
        {
            //step=0, vali=target (pouze nastavi target hodnotu)
            pwm_table[pwm_table_index+1] = target_index;
        }
    }  
    pwm_table_index+=(CCP_PWM_TABLE_ISIZE/4);
#endif     
    
#ifdef CCP2_PWM    
    if(index==1)
    {
        int val=CCP2RB;
        
        //najdi aktualni index, prvni polozka data, ktera je vetsi nebo rovna
        int vali=-1, a;
        for(a=0; a<=EXP_MAX; a++)
        {
            if(val<=pwm_exp[a]) { vali=a; break; }
        }
        
        pwm_table[pwm_table_index+0]=target_index<<16 | step_ms<<8 | step_ms;  //target_index, time, timebase
        pwm_table[pwm_table_index+2]=(int)&pwm_soft_exp;                       //adresa fce
        pwm_table[pwm_table_index+3]=(int)&CCP2RB;                             //adresa registru
        pwm_table[pwm_table_index+4]=(int)pwm_exp;                             //adresa pole data
        
        if(vali<target_index)
        {
            //step=+1, aktualni index
            pwm_table[pwm_table_index+1]= 1<<8 | vali;
        }
        else if(vali>target_index)
        {
            //step=-1, aktualni index
            pwm_table[pwm_table_index+1]= -1<<8 | vali;
        }
        else
        {
            //step=0, vali=target (pouze nastavi target hodnotu)
            pwm_table[pwm_table_index+1] = target_index;
        }
    }  
    pwm_table_index+=(CCP_PWM_TABLE_ISIZE/4);
#endif 
    
#ifdef CCP3_PWM    
    if(index==2)
    {
        int val=CCP3RB;
        
        //najdi aktualni index, prvni polozka data, ktera je vetsi nebo rovna
        int vali=-1, a;
        for(a=0; a<=EXP_MAX; a++)
        {
            if(val<=pwm_exp[a]) { vali=a; break; }
        }
        
        pwm_table[pwm_table_index+0]=target_index<<16 | step_ms<<8 | step_ms;  //target_index, time, timebase
        pwm_table[pwm_table_index+2]=(int)&pwm_soft_exp;                       //adresa fce
        pwm_table[pwm_table_index+3]=(int)&CCP3RB;                             //adresa registru
        pwm_table[pwm_table_index+4]=(int)pwm_exp;                             //adresa pole data
        
        if(vali<target_index)
        {
            //step=+1, aktualni index
            pwm_table[pwm_table_index+1]= 1<<8 | vali;
        }
        else if(vali>target_index)
        {
            //step=-1, aktualni index
            pwm_table[pwm_table_index+1]= -1<<8 | vali;
        }
        else
        {
            //step=0, vali=target (pouze nastavi target hodnotu)
            pwm_table[pwm_table_index+1] = target_index;
        }
    }  
#endif     
    
    //timer1 interrupt vola fci pwm_soft_timer, po skonceni pwm_soft tento ukazatel nuluje, timer1 ji prestane volat 
    //pwm_soft_timer pracuje v interruptu (timeru1)    
    pwm_soft_fn = &pwm_soft_timer;
}

#endif

void pwm_on(char index, char val)
{
    //provede zapnuti nebo vypnuti CPPx (val=1 ON, val=0 OFF)
    if(index==0)
    {
#ifdef CCP1_PWM        
        CCP1CON1bits.ON = val;                // Turn on MCCP module       
#endif    
    }
    else if (index==1)
    {
#ifdef CCP2_PWM       
        CCP2CON1bits.ON = val;                // Turn on MCCP module       
#endif        
    }
    else if (index==2)
    {
#ifdef CCP3_PWM       
        CCP3CON1bits.ON = val;                // Turn on MCCP module       
#endif        
    } 
    else if (index==3)
    {
#ifdef CCP4_PWM       
        CCP4CON1bits.ON = val;                // Turn on MCCP module       
#endif        
    }     
}

void pwm_setPower(char index, int val)
{
    //nastavi power (CCPxRB) na zadanou hodnotu
    //val - 0=nulovy vykon, PERIOD (4096)=max. vykon
    //na zacatku cyklu je vystup nastaven na 1, val definuje okamzik vypnuti
    //je-li CCPxRB=0 k sepnuti nedojde vubec   
    
    if(val > PERIOD) { val=PERIOD; }
    if(val < 0) { val=0; }
    
    if(index==0)
    {
        CCP1RB = val;
    }
    else if(index==1)
    {
        CCP2RB = val;
    }
    else if (index==2)
    {
        CCP3RB = val;
    }
    else if (index==3)
    {
        CCP4RB = val;
    }    
}

int pwm_getProc(char index)
{
    //vraci aktualni vykon v proc
    
    int val=0;
    if(index==0) { val=CCP1RB; }
    else if(index==1) { val=CCP2RB; }
    else if(index==2) { val=CCP3RB; }
    
    //prida 0,5%
    return (val+PERIOD/200)*100/PERIOD;
}

int pwm_getValue(char index)
{
    int val=0;
    if(index==0) { val=CCP1RB; }
    else if(index==1) { val=CCP2RB; }
    else if(index==2) { val=CCP3RB; }    
    return val;
}

void pwm_Init()
{
    //nastavi modul, ale zustava OFF, zapnout volanim fce pwm_on(index, 1)
    
#ifdef CCP1_PWM  

        // Set MCCP operating mode
        CCP1CON1bits.CCSEL = 0;             // Set MCCP operating mode (OC mode)
        CCP1CON1bits.MOD = 0b0101;          // Set mode (Buffered Dual-Compare/PWM mode)

        //Configure MCCP Timebase
        CCP1CON1bits.T32 = 0;               // Set timebase width (16-bit)
        CCP1CON1bits.TMRSYNC = 0;           // Set timebase synchronization (Synchronized)
        CCP1CON1bits.CLKSEL = 0b000;        // Set the clock source (Tcy)
        CCP1CON1bits.TMRPS = 0b00;          // Set the clock prescaler (1:1)
        CCP1CON1bits.TRIGEN = 0;            // Set Sync/Triggered mode (Synchronous)
        CCP1CON1bits.SYNC = 0b00000;        // Select Sync/Trigger source (Self-sync)

        //Configure MCCP output for PWM signal
        CCP1CON2bits.OCAEN = 1;             // Enable desired output signals (OC1A)
        CCP1CON3bits.OUTM = 0b000;          // Set advanced output modes (Standard output)
        CCP1CON3bits.POLACE = 0;            // Configure output polarity (Active High)
        CCP1TMRbits.TMRL = 0x0000;          // Initialize timer prior to enable module.

        CCP1PRbits.PRL = PERIOD;             // Configure timebase period
        CCP1RA = 0x0;                        // Set the rising edge compare value
        CCP1RB = 0;                          // Set the falling edge compare value
        //CCP1CON1bits.ON = 1;                 // Turn on MCCP module  
    
      
#endif

#ifdef CCP2_PWM
    //RP5=SCCP2, nastaveni vystupu SCCP2 na RP5 (SOIC28 = RB4, pin 11)
    //RPOR2bits.RP11R=6;
    RPOR1bits.RP5R=6;
    
    // Set MCCP operating mode
    CCP2CON1bits.CCSEL = 0;             // Set MCCP operating mode (OC mode)
    CCP2CON1bits.MOD = 0b0101;          // Set mode (Buffered Dual-Compare/PWM mode)
    
    //Configure MCCP Timebase
    CCP2CON1bits.T32 = 0;               // Set timebase width (16-bit)
    CCP2CON1bits.TMRSYNC = 0;           // Set timebase synchronization (Synchronized)
    CCP2CON1bits.CLKSEL = 0b000;        // Set the clock source (Tcy)
    CCP2CON1bits.TMRPS = 0b00;          // Set the clock prescaler (1:1)
    CCP2CON1bits.TRIGEN = 0;            // Set Sync/Triggered mode (Synchronous)
    CCP2CON1bits.SYNC = 0b00000;        // Select Sync/Trigger source (Self-sync)
    
    //Configure MCCP output for PWM signal
    CCP2CON2bits.OCAEN = 1;             // Enable desired output signals (OC1A)
    //CCP2CON3bits.OUTM = 0b000;          // Set advanced output modes (Standard output)
    CCP2CON3bits.POLACE = 0;            // Configure output polarity (Active High)
    CCP2TMRbits.TMRL = 0x0000;          // Initialize timer prior to enable module.
    
    CCP2PRbits.PRL = PERIOD;            // Configure timebase period
    CCP2RA = 0x0;                       // Set the rising edge compare value
    CCP2RB = 0;                         // Set the falling edge compare value
    //CCP1CON1bits.ON = 1;              // Turn on MCCP module    
#endif    

#ifdef CCP3_PWM    
    //RP6=SCCP3, nastaveni vystupu SCCP3 na RP6 (SOIC28 = RA4, pin 12)
    //RPOR1bits.RP6R=7;  
    
    
    // Set MCCP operating mode
    CCP3CON1bits.CCSEL = 0;             // Set MCCP operating mode (OC mode)
    CCP3CON1bits.MOD = 0b0101;          // Set mode (Buffered Dual-Compare/PWM mode)
    
    //Configure MCCP Timebase
    CCP3CON1bits.T32 = 0;               // Set timebase width (16-bit)
    CCP3CON1bits.TMRSYNC = 0;           // Set timebase synchronization (Synchronized)
    CCP3CON1bits.CLKSEL = 0b000;        // Set the clock source (Tcy)
    CCP3CON1bits.TMRPS = 0b00;          // Set the clock prescaler (1:1)
    CCP3CON1bits.TRIGEN = 0;            // Set Sync/Triggered mode (Synchronous)
    CCP3CON1bits.SYNC = 0b00000;        // Select Sync/Trigger source (Self-sync)
    
    //Configure MCCP output for PWM signal
    CCP3CON2bits.OCAEN = 1;             // Enable desired output signals (OC1A)
    //CCP3CON3bits.OUTM = 0b000;          // Set advanced output modes (Standard output)
    CCP3CON3bits.POLACE = 0;            // Configure output polarity (Active High)
    CCP3TMRbits.TMRL = 0x0000;          // Initialize timer prior to enable module.
    
    CCP3PRbits.PRL = PERIOD;            // Configure timebase period
    CCP3RA = 0x0;                       // Set the rising edge compare value
    CCP3RB = 0;                         // Set the falling edge compare value
    //CCP1CON1bits.ON = 1;              // Turn on MCCP module    
#endif      

#ifdef CCP4_PWM    
    //RP6=SCCP3, nastaveni vystupu SCCP3 na RP6 (SOIC28 = RA4, pin 12)
    //RPOR1bits.RP6R=7;
    
    //36-pin, PIN9, RB4, RP10=11, CCP4OUT
    RPOR2bits.RP10R=11;
    
    
    // Set MCCP operating mode
    CCP4CON1bits.CCSEL = 0;             // Set MCCP operating mode (OC mode)
    CCP4CON1bits.MOD = 0b0101;          // Set mode (Buffered Dual-Compare/PWM mode)
    
    //Configure MCCP Timebase
    CCP4CON1bits.T32 = 0;               // Set timebase width (16-bit)
    CCP4CON1bits.TMRSYNC = 0;           // Set timebase synchronization (Synchronized)
    CCP4CON1bits.CLKSEL = 0b000;        // Set the clock source (Tcy)
    CCP4CON1bits.TMRPS = 0b00;          // Set the clock prescaler (1:1)
    CCP4CON1bits.TRIGEN = 0;            // Set Sync/Triggered mode (Synchronous)
    CCP4CON1bits.SYNC = 0b00000;        // Select Sync/Trigger source (Self-sync)
    
    //Configure MCCP output for PWM signal
    CCP4CON2bits.OCAEN = 1;             // Enable desired output signals (OC1A)
    //CCP3CON3bits.OUTM = 0b000;          // Set advanced output modes (Standard output)
    CCP4CON3bits.POLACE = 0;            // Configure output polarity (Active High)
    CCP4TMRbits.TMRL = 0x0000;          // Initialize timer prior to enable module.
    
    CCP4PRbits.PRL = PERIOD;            // Configure timebase period
    CCP4RA = 0x0;                       // Set the rising edge compare value
    CCP4RB = 0;                         // Set the falling edge compare value
    //CCP1CON1bits.ON = 1;              // Turn on MCCP module    
#endif    
    
}

#endif