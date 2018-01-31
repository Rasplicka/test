

// <editor-fold defaultstate="collapsed" desc="DEVCFG0 PIC32MZ">

#ifdef PIC32MZ
/*** DEVCFG0 ***/

#pragma config DEBUG =      OFF             //x
#pragma config JTAGEN =     OFF             //x
#pragma config ICESEL =     ICS_PGx2        //
#pragma config TRCEN =      OFF             //x
#pragma config BOOTISA =    MIPS32 //MICROMIPS       //MIPS32
#pragma config FECCCON =    OFF_UNLOCKED    //
#pragma config FSLEEP =     OFF             //
#pragma config DBGPER =     PG_ALL          //x
#pragma config EJTAGBEN =   NORMAL          //
#pragma config CP =         OFF             //

/*** DEVCFG1 ***/

#pragma config FNOSC =      SPLL            //SPLL
#pragma config DMTINTV =    WIN_127_128     //
#pragma config FSOSCEN =    OFF             //
#pragma config IESO =       OFF             //OFF
#pragma config POSCMOD =    EC              //x
#pragma config OSCIOFNC =   OFF             //
#pragma config FCKSM =      CSECME          //x CSDCMD
#pragma config WDTPS =      PS1024          //1048576 
#pragma config WDTSPGM =    STOP            //
#pragma config FWDTEN =     OFF              //OFF
#pragma config WINDIS =     NORMAL          //
#pragma config FWDTWINSZ =  WINSZ_25        //
#pragma config DMTCNT =     DMT31           //x
#pragma config FDMTEN =     OFF             //

/*** DEVCFG2 ***/

#pragma config FPLLIDIV =   DIV_3           //primary osc 24MHz / 3 = 8MHz
#pragma config FPLLRNG =    RANGE_5_10_MHZ
#pragma config FPLLICLK =   PLL_POSC
#pragma config FPLLMULT =   MUL_50          //8 x 50 = 400MHz
#pragma config FPLLODIV =   DIV_2           //    /2 = 200MHz (SPLL = 200MHz)
#pragma config UPLLFSEL =   FREQ_24MHZ
#pragma config UPLLEN =     ON

/*** DEVCFG3 ***/

#pragma config USERID =     0xffff
#pragma config FMIIEN =     OFF
#pragma config FETHIO =     OFF
#pragma config PGL1WAY =    0 //OFF
#pragma config PMDL1WAY =   0 //OFF
#pragma config IOL1WAY =    0 //ON
#pragma config FUSBIDIO =   OFF

/*** BF1SEQ0 ***/
#pragma config TSEQ =       0xffff
#pragma config CSEQ =       0xffff

#endif

// </editor-fold>


// <editor-fold defaultstate="collapsed" desc="DEVCFG0 PIC32MM">

#ifdef PIC32MM

// PIC32MM0064GPL028 Configuration Bit Settings
// 'C' source line config statements
// FDEVOPT
#pragma config SOSCHP = OFF             // Secondary Oscillator High Power Enable bit (SOSC oprerates in normal power mode.)
#pragma config FUSBIDIO = OFF
#pragma config FVBUSIO = OFF
#pragma config USERID = 0xFFFF          // User ID bits (User ID bits)

// FICD
//POZOR, pri JTAGEN=ON nefunguje SPI1, pri nastaveni OFF debug ICD3 funguje normalne
#pragma config JTAGEN = OFF             // !!!JTAG disable 
#pragma config ICS = PGx1               // ICE/ICD Communication Channel Selection bits (Communicate on PGEC1/PGED1)

// FPOR
#pragma config BOREN = BOR3             // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware; SBOREN bit disabled)
#pragma config RETVR = OFF              // Retention Voltage Regulator Enable bit (Retention regulator is disabled)
#pragma config LPBOREN = ON             // Low Power Brown-out Enable bit (Low power BOR is enabled, when main BOR is disabled)

// FWDT
#pragma config SWDTPS = PS1048576       // Sleep Mode Watchdog Timer Postscale Selection bits (1:1048576)
#pragma config FWDTWINSZ = PS25_0       // Watchdog Timer Window Size bits (Watchdog timer window size is 25%)
#pragma config WINDIS = OFF             // Windowed Watchdog Timer Disable bit (Watchdog timer is in non-window mode)
#pragma config RWDTPS = PS1048576       // Run Mode Watchdog Timer Postscale Selection bits (1:1048576)
#pragma config RCLKSEL = LPRC           // Run Mode Watchdog Timer Clock Source Selection bits (Clock source is LPRC (same as for sleep mode))
#pragma config FWDTEN = OFF             // Watchdog Timer Enable bit (WDT is disabled)

// FOSCSEL
//FNOSC=FRCDIV (8MHz FRC), nebo PLL (24MHz = FRCx3)
#pragma config FNOSC = PLL              // Oscillator Selection bits (Fast RC oscillator (FRC) with divide-by-N) //PRI
#pragma config PLLSRC = FRC             // System PLL Input Clock Selection bit (FRC oscillator is selected as PLL reference input on device reset) //PRI
#pragma config SOSCEN = OFF             // Secondary Oscillator Enable bit (Secondary oscillator (SOSC) is disabled)
#pragma config IESO = OFF               // Two Speed Startup Enable bit (Two speed startup is disabled)
#pragma config POSCMOD = OFF            // Primary Oscillator Selection bit (Primary oscillator is disabled) //HS
#pragma config OSCIOFNC = OFF           // System Clock on CLKO Pin Enable bit (OSCO pin operates as a normal I/O)
#pragma config SOSCSEL = ON             // Secondary Oscillator External Clock Enable bit (RA4 and RB4 are controlled by I/O PORT)
#pragma config FCKSM = CSDCMD//SCECME           // Clock Switching and Fail-Safe Clock Monitor Enable bits (Clock switching is enabled; Fail-safe clock monitor is enabled)

// FSEC
#pragma config CP = OFF                 // Code Protection Enable bit (Code protection is disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#endif

// </editor-fold>