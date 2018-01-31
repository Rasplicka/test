#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "def.h"

/*
 * 28 pin SOIC, SSOP
 * ANx   pin
 * AN0 - 2  RA0  
 * AN1 - 3  RA1
 * AN2 - 4  RB0
 * AN3 - 5  RB1
 * AN4 - 6  RB2
 * AN5 - 9  RA2
 * AN6 - 10 RA3
 * AN7 - 23 RB12
 * AN8 - 24 RB13
 * AN9 - 25 RB14
 * AN10- 26 RB15 
 * AN11- 7  RB3
 * AN12- 
 * AN13-
 * AVdd-28 (+)
 * AVss-27 (gnd)
*/         

//Pouze nastaveni ADC prevodniku - scan mode
//nove spusteni se provede Timer1 (vynulovani), tzn 1/10s
//dokonceni prevodu vsech inputs vyvola interrupt, ktery
//- nuluje IF
//- ulozi hodnoty do pole ADResult, kde vyplni platne polozky, ostatni maji hodnotu -1
//sw cte hodnotu AD prevodu volanim fce adcscan_getValue(input_numbe=ANx)

#ifdef ADC

int  adc_counter=0;          //zvyseni o 1 pri kazdem dokonceni ADC prevodu
int  adc_values[]={-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
uint calibData=0;

int adcscan_getValue(int index)
{
    return  adc_values[index];
}

void adcscan_Init()
{
    char int_after=-1;   
    unsigned int css=0;
    
    //NASTAVENI DEFINOVANYCH ANx VSTUPU
    // <editor-fold defaultstate="collapsed" desc="setting AN0 - AN19">
#ifdef AN0   
    // Set RA0/AN0 as an analog input (pin 2)
    ANSELAbits.ANSA0 = 1;
    TRISAbits.TRISA0 = 1;
    css |= 1;
    int_after++;
#endif     
#ifdef AN1   
    // Set RA1/AN1 as an analog input (pin 3)
    ANSELAbits.ANSA1 = 1;
    TRISAbits.TRISA1 = 1;
    css |= (1 << 1);
    int_after++;
#endif     
#ifdef AN2    
    // Set RB0/AN2 as an analog input (pin 4)
    ANSELBbits.ANSB0 = 1;
    TRISBbits.TRISB0 = 1;
    css |= (1 << 2);
    int_after++;
#endif     
#ifdef AN3    
    // Set RB1/AN3 as an analog input (pin 5)
    ANSELBbits.ANSB1 = 1;
    TRISBbits.TRISB1 = 1;
    css |= (1 << 3);
    int_after++;
#endif     
#ifdef AN4    
    // Set RB2/AN4 as an analog input (pin 6)
    ANSELBbits.ANSB2 = 1;
    TRISBbits.TRISB2 = 1;
    css |= (1 << 4);
    int_after++;
#endif    

#ifdef AN5    
    // Set RA2/AN5 as an analog input (pin 9)
    ANSELAbits.ANSA2 = 1;
    TRISAbits.TRISA2 = 1;
    //CNPDAbits.CNPDA2 = 1;               //pull down rezistor
    CNPUAbits.CNPUA2 = 1;               //pull up rezistor    
    css |= (1 << 5);
    int_after++;
#endif

#ifdef AN6     
    // Set RA3/AN6 as an analog input (pin 10)
    ANSELAbits.ANSA3 = 1;
    TRISAbits.TRISA3 = 1;
    //CNPDBbits.CNPDB13=1;
    CNPUAbits.CNPUA3=1;
    css |= (1 << 6);
    int_after++;
#endif    
#ifdef AN7     
    // Set RB12/AN7 as an analog input (pin 23)
    ANSELBbits.ANSB4 = 1;
    TRISBbits.TRISB4 = 1;
    css |= (1 << 7);
    int_after++;
#endif    
#ifdef AN8     
    // Set RB13/AN8 as an analog input (pin 24)
    ANSELBbits.ANSB13 = 1;
    TRISBbits.TRISB13 = 1;
    //CNPDBbits.CNPDB13=1;
    //CNPUBbits.CNPUB13=0;
    //ODCBbits.ODCB13=0;    
    css |= (1 << 8);
    int_after++;
#endif      
#ifdef AN9     
    // Set RB14/AN9 as an analog input (pin 25)
    ANSELBbits.ANSB14 = 1;
    TRISBbits.TRISB14 = 1;
    //CNPDBbits.CNPDB14=1;
    //CNPUBbits.CNPUB14=0;
    //ODCBbits.ODCB14=0;
    css |= (1 << 9);
    int_after++;
#endif 
#ifdef AN10     
    // Set RB15/AN10 as an analog input (pin 26)
    ANSELBbits.ANSB15 = 1;
    TRISBbits.TRISB15 = 1;
    //CNPDBbits.CNPDB15=1;
    //CNPUBbits.CNPUB15=0;
    //ODCBbits.ODCB15=0;
    css |= (1 << 10);
    int_after++;
#endif
#ifdef AN11     
    // Set RB3/AN11 as an analog input (pin 7)
    ANSELBbits.ANSB3 = 1;
    TRISBbits.TRISB3 = 1;
    css |= (1 << 11);
    int_after++;
#endif   


#ifdef AN12     
    // Set RC0/AN12 as an analog input
    ANSELCbits.ANSC0 = 1;
    TRISCbits.TRISC0 = 1;
    css |= (1 << 12);
    int_after++;
#endif       

#ifdef AN13     
    // Set RC1/AN13 as an analog input
    ANSELCbits.ANSC1 = 1;
    TRISCbits.TRISC1 = 1;
    css |= (1 << 13);
    int_after++;
#endif     

#ifdef AN14     
    // Set RC8/AN14 as an analog input
    ANSELCbits.ANSC8 = 1;
    TRISCbits.TRISC8 = 1;
    css |= (1 << 14);
    int_after++;
#endif     

#ifdef AN15     
    // Set RC5/AN15 as an analog input
    ANSELCbits.ANSC5 = 1;
    TRISCbits.TRISC5 = 1;
    css |= (1 << 15);
    int_after++;
#endif     

#ifdef AN16     
    // Set RA13/AN16 as an analog input
    ANSELAbits.ANSA13 = 1;
    TRISAbits.TRISA13 = 1;
    css |= (1 << 16);
    int_after++;
#endif     

#ifdef AN17     
    // Set RA12/AN17 as an analog input
    ANSELAbits.ANSA12 = 1;
    TRISAbits.TRISA12 = 1;
    css |= (1 << 17);
    int_after++;
#endif     

#ifdef AN18     
    // Set RA11/AN18 as an analog input
    ANSELAbits.ANSA11 = 1;
    TRISAbits.TRISA11 = 1;
    css |= (1 << 18);
    int_after++;
#endif       

#ifdef AN19     
    // Set RA6/AN19 as an analog input
    ANSELAbits.ANSA6 = 1;
    TRISAbits.TRISA6 = 1;
    css |= (1 << 19);
    int_after++;
#endif      
    // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="ANVDDCORE, ANVBG, ANVSS, ANVDD">
#ifdef ANVDDCORE
    //VDD (core Vcc) AN27
    css |= (1 << 27);
    int_after++;
#endif 

#ifdef ANVBG
    //AV Band Gap - interni ref 1.2V AN28
    css |= (1 << 28);
    int_after++;
#endif    

#ifdef ANVSS
    //AVSS (GND) AN29
    css |= (1 << 29);
    int_after++;
#endif    

#ifdef ANVDD
    //AVDD (Vcc) AN30
    css |= (1 << 30);
    int_after++;
#endif        
    // </editor-fold>
    
    //zapnuti interni reference 1.2V VBGADC=1, vypnuti VBGADC=0
    ANCFGbits.VBGADC=1;
    
    //AD1CHSbits.CH0SA=0b11101;
    //scan inputs
    AD1CON2bits.CSCNA=1;            //scan inputs
    //AutoScan disable
    AD1CON5bits.ASEN=0;             //auto scan (skenovani vstupu pro threshold detect)
    AD1CSS=css;
    AD1CON2bits.SMPI=int_after;     //interrupt po X prevodech (0=po 1, 1=po 2, 2=po 3, ...)
   
    //test jednoho vstupu, musi byt nastaveno AD1CON2bits.CSCNA=0
    AD1CHSbits.CH0NA=0;             //vref- = Vss
    //AD1CHSbits.CH0SA=0b11101;       //input = Vss (test zero) val=0
    //AD1CHSbits.CH0SA=0b11011;       //input = Vdd (core) val=3370
    //AD1CHSbits.CH0SA=0b11110;       //input = AVdd (+) val=4090
    //AD1CHSbits.CH0SA=0b11100;       //input = Vbg (internal ref. 1.2V) val=1750 (musi byt zapnuto Vbg - viz. ANCFG)
    
    //timing
    AD1CON3bits.ADRC = 1;           //Clock from peripheral clock FRC 8MHz
    AD1CON3bits.ADCS = 8;//8;       //ADC clock TAD = 8 peripheral clock, 1us/1MHz
    AD1CON3bits.SAMC = 10;//10;     //sample time 10 TAD, 10us/0.1MHz
    
    //AVdd/ss
    AD1CON2bits.VCFG=0;             //AVdd, AVss
    AD1CON2bits.OFFCAL=1;           //kalibrace (nefunguje)
    
    //AN1CON1
    AD1CON1bits.MODE12=1;           //1=12 bit, 0=10 bit 
    AD1CON1bits.ASAM = 1;           //auto sample po skonceni predchoziho
    AD1CON1bits.SSRC = 7;           //sample time - auto, podle AD1CON3.SAMC
    AD1CON1bits.ON = 1;             //Turn on the ADC
    
#ifdef PIC32MM0256
    IPC8bits.AD1IP=1;
    IPC8bits.AD1IS=0;
    IFS1bits.AD1IF=0;
    IEC1bits.AD1IE=1;
#endif

#ifdef PIC32MM0064    
    //interrupt
    IPC3bits.AD1IP=2;               //interrupt prior
    IPC3bits.AD1IS=0;               //interrupt subprior    
    IFS0bits.AD1IF=0;               //int flag=0
    IEC0bits.AD1IE=1;               //int enable=1
#endif
    
    //start
    AD1CON1bits.SAMP = 1;           //Start the first sampling cycle
}

#endif