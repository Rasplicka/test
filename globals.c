#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"

#ifdef USE_SYSTEMFONT_ARIAL 
    #include "font_arial.h"
    IMAGE_SRC fontSys;
#endif
    
#ifdef USE_SYSTEMFONT_FIXEDx
    #include "font_fixed1306.h"
    IMAGE_SRC fontSys;
#endif    

#ifdef USE_GRAPHICS
//global vars
GRAPHICS graphics;
DISPLAY dispSys;
PORT_INFO pinfo_dispSys;

//local fce
static void initDispSys();

static void initDispSys()
{
    //Inicializuje systemovy display
    
    //definuje jinou colorMap (system obsahuje std colorMap (B/W), ktera se automaticky nastavi v IMAGE_SRC fci setFontSrc/setImageSrc)
    //loadColorMap(cmap);

    //nastaveni konkretniho displeje (ili9341/SPI)
    
#ifdef SYSDISPLAY_9341SPI    
    //<editor-fold defaultstate="collapsed" desc="PortWriter, pin set, driver">
    //pin configuration C2=CS, C1=RESET, C0=DC
    pinfo_dispSys.cs_portBase = PORTC_BASE;             //CS
    pinfo_dispSys.cs_pin = BIT2;
    pinfo_dispSys.reset_portBase = PORTC_BASE;          //RESET
    pinfo_dispSys.reset_pin = BIT1;
    pinfo_dispSys.dc_portBase = PORTC_BASE;             //DC
    pinfo_dispSys.dc_pin = BIT0;
    pinfo_dispSys.busMode = BUS_MODE._8bit;             //8-bit mode
    pinfo_dispSys.directMode = 1;                       //direct mode

    //v pinfo_dispSys nastavi fce pro vysilani dat na pozadovany port SPI, PMP, ...
    portWriter_init(&pinfo_dispSys, PERIPH_TYPE.spi, 1); //SPI[1]=SPI2
    
    //v dispSys nastavi fce driveru disp9341 
    disp9341_driver(&dispSys);                    
    // </editor-fold>
#endif
    
#ifdef SYSDISPLAY_1306SPI
    //<editor-fold defaultstate="collapsed" desc="PortWriter, pin set, driver">
    //pin configuration C2=CS, C1=RESET, C0=DC
    pinfo_dispSys.cs_portBase = PORTC_BASE;             //CS
    pinfo_dispSys.cs_pin = BIT2;
    pinfo_dispSys.reset_portBase = PORTC_BASE;          //RESET
    pinfo_dispSys.reset_pin = BIT1;
    pinfo_dispSys.dc_portBase = PORTC_BASE;             //DC
    pinfo_dispSys.dc_pin = BIT0;
    pinfo_dispSys.busMode = BUS_MODE._8bit;             //8-bit mode
    pinfo_dispSys.directMode = 1;                       //direct mode

    //v pinfo_dispSys nastavi fce pro vysilani dat na pozadovany port SPI, PMP, ...
    portWriter_init(&pinfo_dispSys, PERIPH_TYPE.spi, 1); //SPI[1]=SPI2
    
    //v dispSys nastavi fce driveru disp9341 
    disp1306_driver(&dispSys);                    
    // </editor-fold>
#endif    
    
    //pro vsechny typy displeju
    
    // <editor-fold defaultstate="collapsed" desc="init displeje, aktivuje Graphics, init font">
    //inicializuje display
    dispSys.initDisplay(&pinfo_dispSys);            
    
    //nastavi fce Graphics na strukturu dispSys, vystup Graphics jde na dispSys
    setGraphics(&graphics, &dispSys, &pinfo_dispSys); 
    
    //init system font (pouziva font_sys v souboru font_sys.h)
#ifdef USE_SYSTEMFONT_ARIAL     
    setFontSrc(&font_arial18, &fontSys);
    fontSys.foreColor=RGB16(0, 63, 0);
#endif    
    
#ifdef USE_SYSTEMFONT_FIXEDx
    setFontSrc(&font_fixed16x, &fontSys);
    fontSys.foreColor=RGB16(31, 63, 31);
#endif    
    // </editor-fold>

    //cls
    graphics.clear(COLOR.Black);    
}

#endif

#ifdef USE_TOUCHPAD
//global vars
PORT_INFO pInfo_touchSys;

//local fce
static void initTouchSys();

static void initTouchSys()
{
    //CS=RA3, pin 8
    pInfo_touchSys.cs_portBase = PORTA_BASE;                    //CS
    pInfo_touchSys.cs_pin = BIT3;
    pInfo_touchSys.busMode = BUS_MODE._8bit;                    //8-bit mode
    portWriter_init(&pInfo_touchSys, PERIPH_TYPE.spi, 1);       //pinfo obsahuje fce pro vysilani dat na pozadovany port SPI, PMP, ...
}

#endif

void globals_Start()
{
    //tato fce se vola jako inicializace systemu, tesne pred prvnim spustenim threads
    
#ifdef USE_GRAPHICS    
    initDispSys();
#endif    
    
#ifdef USE_TOUCHPAD     
    initTouchSys();
#endif
    
}