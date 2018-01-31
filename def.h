//obsahuje globalni definice pro C/C++ i ASM

// <editor-fold defaultstate="collapsed" desc="CPU">
//#define PIC32MZ
//#define PIC32MM0064
//#define PIC32MM0064_28pin
#define PIC32MM0256
#define PIC32MM0256_36pin
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="System">
//#define     SIMULATOR


#define     STACK_CHECK_VALUE       0xF010E020

#define		RAM_BASE	    0x80000000
#ifdef PIC32MM0256
#define		RAM_SIZE	    32*1024 
#define     STACK_SIZE      8*1024
#endif
#ifdef PIC32MM0064
#define		RAM_SIZE	    8*1024
#define     STACK_SIZE      3*1024
#endif

//#define	  STACK_COUNT	    16                    //max. pocet zasobniku (pocet polozek v stack_table)
//#define     SAFE_PROCESS                            //povoluje ochranu prepinani procesu, kdy CPU timer spusti interrupt, bezi-li proces prilis dlouho
#define     ENABLE_APP_RESTART_ON_ERROR             //povoluje restart procesu, pokud nastal general_exception
#define     ENABLE_CHECK_STACK_OVERFLOW             //povoluje kontrolovat stack overflow

#define     TIMER1_EVENT_CAPA       16
#define     TIMER1_EVENT_ISIZE      12
#define     TIMER1_INTERVAL 10                      //interval ms

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="DISPLAY, GRAPHICS, TOUCHPAD">
#define             USE_GRAPHICS                    //pouzije graphics

//ili9341
//#define             USE_SYSTEMFONT_ARIAL            //pouzije system font arial18 - std font
//#define             SYSDISPLAY_9341SPI              //definuje typ systemoveho displeje (ili9341)
//#define             USE_DISP9341                    //pouzije driver displeje
//ssd1306
#define             USE_SYSTEMFONT_FIXEDx           //pouzije system font fixed16 - nestandardni font (pro ssd1306)
#define             SYSDISPLAY_1306SPI              //definuje typ systemoveho displeje (ssd1306)
#define             USE_DISP1306                    //pouzije driver displeje

#define             USE_TOUCHPAD                    //pouzije touchpad
// </editor-fold>


// <editor-fold defaultstate="collapsed" desc="Periph driver (SPI, I2C, ...)">
//SPI ---------------------------------------------------
//driver je pouzit, pokud pouziva alespon jeden SPI kanal
//#define     SPI1_USE                              //pouziva SPI1 (fce driveru pro SPI1)
#define     SPI2_USE                                //pouziva SPI2 (fce driveru pro SPI2)
//#define     SPI3_USE                              //pouziva SPI3 (fce driveru pro SPI3)


//I2C -----------------------------------------------------
#define     I2C                     //aktivuje I2C driver
//#define I2C1_USE
//#define I2C2_USE
//#define I2C3_USE
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Proc table">
#define     PROC_T_ISIZE    88          //velikost polozky v proc_t
#define     PROC_T_CAPA     8           //kapacita proc_t

//cislovani pro pouziti v c/c++ (index word), v asm se musi vynasobit 4
//
#define     TH_T_ID         0    
#define     TH_T_START_ADDR 1       //pouzije se pro restart procesu
#define     TH_T_START_SP   2       //pouzije se pro restart procesu
#define     TH_T_STACK_BASE 3

#define     TH_T_A0			4      //offset a0
#define     TH_T_A1			5
#define     TH_T_A2			6
#define     TH_T_A3			7      //offset a3

#define	    TH_T_V0			8
#define	    TH_T_V1			9

#define	    TH_T_S0			10	
#define	    TH_T_S2			12    
#define	    TH_T_S4			14
#define	    TH_T_S6			16

#define	    TH_T_GP			18
#define	    TH_T_SP			19
#define	    TH_T_FP			20
#define	    TH_T_RA			21

//#define	    TH_T_LO			30
//#define	    TH_T_HI			31
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Font (SSD1306)">
#define     FONT1306_MID    1
#define     FONT1306_BIG    2
#define     FILL1306        96
#define     IMAGE1306       97
#define     ZERO1306        98
#define     COMMAND1306     99
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="BIT definition">
//*******************************
//BIT definition
//*******************************
#define     BIT0           0b0000000000000001
#define     BIT1           0b0000000000000010
#define     BIT2           0b0000000000000100
#define     BIT3           0b0000000000001000
#define     BIT4           0b0000000000010000
#define     BIT5           0b0000000000100000
#define     BIT6           0b0000000001000000
#define     BIT7           0b0000000010000000
#define     BIT8           0b0000000100000000
#define     BIT9           0b0000001000000000
#define     BIT10          0b0000010000000000
#define     BIT11          0b0000100000000000
#define     BIT12          0b0001000000000000
#define     BIT13          0b0010000000000000
#define     BIT14          0b0100000000000000
#define     BIT15          0b1000000000000000
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="IO port">
#define	    ANSEL_OFFSET    0x0                         //offset ANSEL reg
#define	    TRIS_OFFSET	    0x10
#define	    PORT_OFFSET	    0x20 
#define     LAT_OFFSET      0x30                        //latch
#define     ODC_OFFSET      0x40                        //open drain
#define	    CNPU_OFFSET	    0x50                        //pull-up
#define	    CNPD_OFFSET	    0x60                        //pull-down

#define	    CLR_OFFSET	    0x4                         //offset CLR registru
#define	    SET_OFFSET	    0x8                         //offset SET registru
#define	    INV_OFFSET	    0xC                         //offset INV registru

#define     CNPU_SET        CNPU_OFFSET + SET_OFFSET    //offset reg. pro SET pull-up
#define     CNPU_CLR        CNPU_OFFSET + CLR_OFFSET    //offset reg. pro CLR pull-up
#define     CNPD_SET        CNPD_OFFSET + SET_OFFSET    //offset reg. pro SET pull-down
#define     CNPD_CLR        CNPD_OFFSET + CLR_OFFSET    //offset reg. pro CLR pull-down
#define     ODC_SET         ODC_OFFSET + SET_OFFSET     //offset reg. pro SET open drain
#define     ODC_CLR         ODC_OFFSET + CLR_OFFSET     //offset reg. pro CLR open drain

#ifdef PIC32MM0064

#define     PORTA_BASE      0xBF802600
#define     PORTB_BASE      0xBF802700
#define     PORTC_BASE      0xBF802800

#endif

#ifdef PIC32MM0256

#define     PORTA_BASE      0xBF802BB0
#define     PORTB_BASE      0xBF802CB0
#define     PORTC_BASE      0xBF802DB0

#endif

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="string/chars">
#define     _CHAR_SPACE     32
#define     _CHAR_MINUS     0x2D
// </editor-fold>


//*******************************************
//MODULES
//*******************************************

// <editor-fold defaultstate="collapsed" desc="ADC">
#define     ADC

//vstupy, ktere jsou skenovany
#ifdef PIC32MM0064
#define     AN8
#define     AN9
#define     AN10
#endif
#ifdef PIC32MM0256
#define     AN4
#define     AN5
#define     AN6
#define     AN7

//#define     ANVDDCORE         //Vcc CORE (AN27)
#define     ANVBG               //internal ref. 1.2V (AN28)
//#define     ANVSS             //GND (AN29)
//#define     ANVDD             //Vcc (AN30)

#endif
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="RTC">
#define     RTC
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="ubtn">
#define     UBTN

#define     UBTN_DOWN       1
#define     UBTN_UP         2
#define     UBTN_DOWN_LONG  3
#define     UBTN_UP_LONG    4
#define     UBTN_REPEAT     5
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="disp1306">
#define     DISP1306
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="PWM">
//#define     PWM                                     //rizeni vykonu, pouziva CCP moduly
//#define     CCP_PWM_COUNT       2                   //pocet pouzitych CCP modulu pro rizeni vykonu PWM
//#define     CCP1_PWM                              //CCP1 pouzita pro pwm
//#define     CCP2_PWM                                //CCP2 pouzita pro pwm
//#define     CCP3_PWM                                //CCP3 pouzita pro pwm
//#define     CCP4_PWM
//#define     PWM_SOFT                                //aktivuje fci soft zmeny vykonu

#define     CCP_PWM_TABLE_ISIZE 20                  //velikost polozky
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="USB">
#define     USB_MM

// </editor-fold>
