#include <xc.h>
#include <stdio.h>
#include <stdlib.h>

#include "def.h"
#include "asm.h"
#include "font1306.h"
#include "spi.h"



// <editor-fold defaultstate="collapsed" desc="global var">

char sd1306_displayIndex = 0; //uzivatelem nastaveny index, na ktery display posila data
char* sd1306_stackTop; 

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="global fn">

void sd1306_start();
void sd1306_drawString(char font, char x, char y, char* string);
void sd1306_clearDisplay(); 
void sd1306_setContrast(char value);
void sd1306_sleep();
void sd1306_resume();

// </editor-fold>



