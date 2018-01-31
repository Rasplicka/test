#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "graphics.h"
#include "disp1306a.h"
#include "asm.h"
#include "font1306.h"
#include "spi.h"
#include "i2c.h"

#define USE_DISP1306
#ifdef USE_DISP1306

//Zmena send - send_i2c ve fn processItem, drawMidChar

// <editor-fold defaultstate="collapsed" desc="Popis">

//modul pro ovladani displeje SSD1306, zapojene na jednom SPI kanale (pouzivaji spolecnou queue)
//muze byt max. 8 displeju
//pri pouziti soucasne i displeju na jinem SPI kanale, musi byt vytvoren jiny modul (kopie s jinym nazvem, napr. disp1306b)
//s nastavenim SPI_PORT, oba moduly bezi jako dva ruzne procesy
//pokud by byl pouzit jiny iface displeje (I2C, paralelni) staci zmenit fci send(), ktera odesila data
//SW iface pro std. fce (drawText, drawString, print, ...) je v souboru display.c, kde jsou odkazy na prislusne
//fce v tomto modulu. Zde lze nastavit fce pro vice displeju, podle jejich indexu, tzn. ze kazdy displej muze
//pouzivat fce jineho modulu.

// </editor-fold>

/*
 I2C pouziva pouze SDA, CLK a RESET signal. Reset muze byt nahrazen RC obvodem na displeji
 Adresa modulu je 0b0111100 (pri DC signal=0) nebo 0b0111101 (pri DC signal=1)
 Na jedne I2C sbernici je tedy mozne pouzit dva displeje
 Nektere moduly nemaji ani DC ani Reset signal (vyvedeny)
 I2C (SDA i CLK) musi byt pulled up rezistory 2k2
 Nastaveni I2CBRG musi byt vyssi, nez min. hodnota (viz. i2c modul)
 */

/*nastaveni modulu*/
#define     USE_SPI_PORT        1                   //0=prvni (SPI1), 1=druhy (SPI2), ...
//#define     USE_I2C_PORT        1                   //0=prvni (I2C1), 2=druhy (I2C2), ...
//#define     I2C_ADDRESS         0b0111100

//#define     SPI_PORT            1                   
//#define     I2C_PORT            1                   

#define     CAN_PRINT                               //implement fci print (console), bez print setri RAM 4x16 bytes

#define     _CSNONE_INDEX       0                   //pokud neni pouzit CS signal (trvale LOW - pouze jeden SPI display)
//#define     _CS0_pin          LATBbits.LATB3      //(SO28 pin 7) CS disp Index 0
//#define     _CS1_pin          0                   //none CS disp Index 1
//#define     _CS2_pin          0                   //none CS disp Index 2
//#define     _CS3_pin          0                   //none CS disp Index 3
//#define     _CS4_pin          0                   //none CS disp Index 4
//#define     _CS5_pin          0                   //none CS disp Index 5
//#define     _CS6_pin          0                   //none CS disp Index 6
//#define     _CS7_pin          0                   //none CS disp Index 7

//PIC32MM0064, 28pin
//#define     _DC_pin             LATBbits.LATB6      //(SO28 pin 15), pokud se nepouziva D/C signal, neni _DC_pin definovano 
//#define     _RESET_pin          LATBbits.LATB7      //(SO28 pin 16), pokud je RESET zajisten HW, neni _RESET_pin definovano

//PIC32MM0256, 36pin
#define     _DC_pin             LATCbits.LATC1      //pin 4, RC1
#define     _RESET_pin          LATCbits.LATC0      //pin 3, RC0  


/*
vypis |VOLT=.....150 mV|                    //radek 16 znaku
 
char result[17];                            //max. pocet znaku + \0
char volt[]="VOLT=\0";                      //const - flash
char val[9];                                //8 znaku + \0
char unit[]=" mV\0";                        //const - flash
int x=150;
  
sprintf(val, "%8d", x);                     //delka bude vzdy 8 znaku, zleva doplneno mezerama
createString(result, 17, volt, val, unit);
sd1306_displayIndex=0;                         //displej[0], nastavovat pouze pokud je pouzito vice displeju
drawString(FONT1306_MID, 0, line(0-3), result);
   
 
//vypis |xxx....1306xxxxx|                  //vypis pouze cisla max. 8 cifer ....1306 (vypisuje i mezery)
                                            //pozice 3-10
char val[9];                                //8 znaku + \0
int x=1306;
sprintf(val, "%8d", x);                     //delka bude vzdy 8 znaku, zleva doplneno mezerama  
sd1306_displayIndex=0;                         //displej[0], nastavovat pouze pokud je pouzito vice displeju 
drawString(FONT1306_MID, 3, line(0-3), val); 
 

//vypis |xxxxxxxxxxxxx.25|                  //vypis pouze cisla max. 3 cifry .25 (vypisuje i mezery)
                                            //pozice 13-15
char val[4];                                //3 znaky + \0
int x=25;
sprintf(val, "%3d", x);                     //delka bude vzdy 8 znaku, zleva doplneno mezerama  
sd1306_displayIndex=0;                         //displej[0], nastavovat pouze pokud je pouzito vice displeju 
drawString(FONT1306_MID, 13, line(0-3), val); 
 
 
//vypis |abc=1256....neco|
char result[17];                            //max. pocet znaku + \0 
char abc[]="abc=\0";
char unit[]="neco\0";                       //const - flash 
char val[9]; 
int x=1256;
sprintf(val, "%8d", x);                     //delka bude vzdy 8 znaku, zleva doplneno mezerama 
alignLeft(val, 8);                          //zarovnani num vlevo
createString(result, 17, abc, val, unit); 
sd1306_displayIndex=0;                         //displej[0], nastavovat pouze pokud je pouzito vice displeju 
drawString(FONT1306_MID, 0, line(0-3), result); 
 
 */

// <editor-fold defaultstate="collapsed" desc="MACRO">

//macro kopiruje data z t0 do t1, t2=max. pocet bytes (skonci driv, pokud najde \0)
#define     CPY_STRING  asm("1:"); asm("lbu $25, ($8)"); asm("sb	$25, ($9)"); asm("beqz $25, 2f"); asm("nop"); asm("addiu $8, 1"); asm("addiu $9, 1"); asm("addiu $10, -1"); asm("bnez $10, 1b"); asm("2:");

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Definition">

//SPI signaly a hodnoty
#define     SPI_CS_ACTIVE       0           //aktivni uroven L
#define     SPI_CS_DEACTIVE     1           //neaktivni uroven H
#define     SPI_DC_COMMAND      0           //DC signal L=command
#define     SPI_DC_DATA         1           //DC signal H=data

//queue
#define     Q_ISIZE             32          //velikost polozky queue
#define     Q_CAPA              8           //pocet polozek queue

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="local vars">

//#define     STACK_SIZE      512
//char stack[STACK_SIZE];
//char* disp1306a_stack = stack + STACK_SIZE - 4; 

char queue[Q_ISIZE * Q_CAPA];       //obsahuje texty, urcene pro vypis na display
char queue_usedItems = 0;           //pocet platnych polozek
char queue_processItemIndex = 0;    //index prave provadene polozky

char ready[]={0,0,0,0,0,0,0,0};
//SPI
//char command_cp[3];
//I2C

#ifdef USE_SPI_PORT
char command_cp[3];
#endif

#ifdef USE_I2C_PORT
//POZOR, max. velikost obrazku je omezena velikost i2c_buffer
    /*    
    i2c_buffer[0]=0x80;             //command
    i2c_buffer[1]=0x21;             //set x    
    i2c_buffer[2]=0x80;             //command
    i2c_buffer[3]=x;                //x start
    i2c_buffer[4]=0x80;             //command
    i2c_buffer[5]=end_x;            //x end
    i2c_buffer[6]=0x80;             //command
    i2c_buffer[7]=0x22;             //set page
    i2c_buffer[8]=0x80;             //command
    i2c_buffer[9]=page;             //start page
    i2c_buffer[10]=0x80;            //commandge
    i2c_buffer[11]=end_page;        //end page
    i2c_buffer[12]=0x40;            //data
    */
char i2c_buffer[] = {0x80, 0x21, 0x80, 0, 0x80, 0, 0x80, 0x22, 0x80, 0, 0x80, 0, 0x40};
char i2c_buffer_cmd[]={0x0};
#endif

//buffer pro konzolovy vystup, fce print() (protoze SSD1306 neumi vertikalni scroll)
#ifdef CAN_PRINT
char line0[16];
char line1[16];
char line2[16];
char line3[16];
char* lines[]={line0, line1, line2, line3};
int empty_line=0;                               //radek pro vypis print()
#endif

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="local functions">

static void scroll(int index);
static void loop();
static void processItem();
//static void drawData(char* q_item);
static void drawMidChar(int index, char x, char page, char code);
static void drawBigChar(int index, char x, char page, char code);
static void drawImage(int index, char x, char page, char* img);
static void fillByte(int index, char byte, int x1, int page1, int x2, int page2);
static char* getQueueFreeItem();
static char fillInitData(char* buffer);
static void send(int index, char* buffer, int len, int dc);
//static void send_i2c(int index, char* buffer, int len, int dc);
static void send_i2c(int index, char* buffer, int len, char* alt_buffer, int alt_len);
static void sendFinish(int w);
static void send_i2cFinish(int w);
static char* getMidAsciiData(char code);
static char* getBigAsciiData(char code);
//static char* getImageData(int image_id);

static void resetDisplay();
static void pause(int t);
static void setDCPin(int value);
static void setCSPin(int index, int value);
static void setResetPin(int value); 

static void drawText(char* text, IMAGE_SRC* font, short x, short y);
// </editor-fold>


void setDisp1306(DISPLAY* d)
{
    //d->drawString=&drawText;
    
}

static void drawText(char* text, IMAGE_SRC* font, short x, short y)
{
    int a=x+y;
}

void disp1306a_start()
{
    setDCPin(0);
    
    //oznaci vsechny polozky queue jako empty (prvni byte = -1)
    int a;
    for(a=0; a<Q_CAPA; a++)
    {
        queue[a * Q_ISIZE]=0x0;
    }

    // <editor-fold defaultstate="collapsed" desc="CS pin">
    
#ifndef _CSNONE_INDEX
    setCSPin(0, SPI_CS_DEACTIVE);
    setCSPin(1, SPI_CS_DEACTIVE);
    setCSPin(2, SPI_CS_DEACTIVE);
    setCSPin(3, SPI_CS_DEACTIVE);
    setCSPin(4, SPI_CS_DEACTIVE);
    setCSPin(5, SPI_CS_DEACTIVE);
    setCSPin(6, SPI_CS_DEACTIVE);
    setCSPin(7, SPI_CS_DEACTIVE);
#endif
    
    // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="RESET">
  
    //resetuje vsechny displeje na SPI1, maji RESET signal spolecny
    resetDisplay();

    // </editor-fold>

    // <editor-fold defaultstate="collapsed" desc="init index 0-7">
    
#ifdef _CSNONE_INDEX
    char* item = getQueueFreeItem();
    if (item != NULL) {
        item[0] = COMMAND1306;
        item[1] = _CSNONE_INDEX;            //display index
        item[2] = fillInitData(item + 4);   //col / pocet znaku pro command
        item[3] = 0;                        //row
        queue_usedItems++;
    }
   
    disp1306a_clear(_CSNONE_INDEX);
    
    processItem();
    processItem();
    ready[_CSNONE_INDEX]=1;
#endif

#ifdef _CS0_pin    
    char* item = getQueueFreeItem();
    if (item != NULL) {
        item[0] = COMMAND1306;
        item[1] = 0; //display index
        item[2] = fillInitData(item + 4); //col / pocet znaku pro command
        item[3] = 0; //row
        queue_usedItems++;
    }
    disp1306a_clear(0);
    
    processItem();
    processItem(); 
    ready[0]=1;
#endif    

#ifdef _CS1_pin    
    char* item = getQueueFreeItem();
    if (item != NULL) {
        item[0] = COMMAND1306;
        item[1] = 1; //display index
        item[2] = fillInitData(item + 4); //col / pocet znaku pro command
        item[3] = 0; //row
        queue_usedItems++;
    }
    disp1306a_clear(1);
    
    processItem();
    processItem(); 
    ready[1]=1;
#endif    

#ifdef _CS2_pin    
    char* item = getQueueFreeItem();
    if (item != NULL) {
        item[0] = COMMAND1306;
        item[1] = 2; //display index
        item[2] = fillInitData(item + 4); //col / pocet znaku pro command
        item[3] = 0; //row
        queue_usedItems++;        
    }
    disp1306a_clear(2);  
    
    processItem();
    processItem();
    ready[2]=1;    
#endif 

#ifdef _CS3_pin    
    char* item = getQueueFreeItem();
    if (item != NULL) {
        item[0] = COMMAND1306;
        item[1] = 3; //display index
        item[2] = fillInitData(item + 4); //col / pocet znaku pro command
        item[3] = 0; //row
        queue_usedItems++;        
    }
    disp1306a_clear(3);

    processItem();
    processItem();
    ready[3]=1;
#endif     

#ifdef _CS4_pin    
    char* item = getQueueFreeItem();
    if (item != NULL) {
        item[0] = COMMAND1306;
        item[1] = 4; //display index
        item[2] = fillInitData(item + 4); //col / pocet znaku pro command
        item[3] = 0; //row
        queue_usedItems++;        
    }
    disp1306a_clear(4);

    processItem();
    processItem();  
    ready[4]=1;
#endif     

#ifdef _CS5_pin    
    char* item = getQueueFreeItem();
    if (item != NULL) {
        item[0] = COMMAND1306;
        item[1] = 5; //display index
        item[2] = fillInitData(item + 4); //col / pocet znaku pro command
        item[3] = 0; //row
        queue_usedItems++;        
    }
    disp1306a_clear(5); 
    
    processItem();
    processItem(); 
    ready[5]=1;
#endif     

#ifdef _CS6_pin    
    char* item = getQueueFreeItem();
    if (item != NULL) {
        item[0] = COMMAND1306;
        item[1] = 6; //display index
        item[2] = fillInitData(item + 4); //col / pocet znaku pro command
        item[3] = 0; //row
        queue_usedItems++;        
    }
    disp1306a_clear(6);

    processItem();
    processItem(); 
    ready[6]=1;
#endif     

#ifdef _CS7_pin    
    char* item = getQueueFreeItem();
    if (item != NULL) {
        item[0] = COMMAND1306;
        item[1] = 7; //display index
        item[2] = fillInitData(item + 4); //col / pocet znaku pro command
        item[3] = 0; //row
        queue_usedItems++;        
    }
    disp1306a_clear(7);  
    
    processItem();
    processItem(); 
    ready[7]=1;
#endif   
    
    // </editor-fold>
    
    loop();
}

void disp1306a_drawText(int index, char* string, int col, int row, int font)
{
    //vlozi do queue polozku, pokud je queue plna, vola doEvents
    //zvysi queue_usedItems
    //col a row jsou sloupec a radek pri pouziti font (ruzne fonty maji ruzny pocet radku a sloupcu)
    
    //spi1_1306_drawString se vola z jinych apps, proto doEvents umozni tomuto modulu uvolnit queue
    
    //q_item je ulozeno v reg t1 (pouzije ho macro CPY_STRING)
    register char* q_item __asm__("$9")=getQueueFreeItem(); //je-li queue plna, vola doEvents
    if(q_item==NULL) { return; }                            //nemelo by nikdy nastat
    
    int x, page;
    if(font==FONT1306_MID)
    {
        x=col*8;            //sirka 8px
        page=row*2;         //vyska 2xpage (2x8)
    }
    else
    {
        x=col*16;           //sirka 16px
        page=row*4;         //vyska 4xpage (4x8)
    }
    
    q_item[0]=font;
    q_item[1]=index;
    q_item[2]=x;                       
    q_item[3]=page; 
    
    asm("addiu $9, 4");                                     //t1 (q_item) + 4
    register char* st __asm__   ("$8")=string;              //t0=src
    //register char* q __asm__    ("$9")=q_item+4;           //t1=dest
    register uint size __asm__  ("$10")=Q_ISIZE-4;          //t2=max. size
    
    //macro, kopiruje string dokud nenajde 0, nebo max. size (max. pocet kopirovanych bytes)
    CPY_STRING
    
    /*
    asm("jal   cpyStr");
    asm("nop");
    */
            
    
    //text, kopiruje dokud nenajde \0, nebo neni konec polozky queue
    //c/c++ verze kopirovani dat
    /*
    int a;
    char data;
    for(a=4; a<Q_ISIZE; a++)
    {
        data=string[a-4];
        q_item[a]=data;
        if(data==0x0) 
        {
            //nasel konec stringu (\0)
            break;
        }
    }
    */
    
    queue_usedItems++;    
}

void disp1306a_drawString(int index, char* string, int x, int y, int font)
{
    //vlozi do queue polozku, pokud je queue plna, vola doEvents
    //zvysi queue_usedItems
    //y (px) urcuje radek (vybere jeden z exist. radku, nelze psat mezi radky)
    //x (px) pozice zacatku, 0-127
    
    //spi1_1306_drawString se vola z jinych apps, proto doEvents umozni tomuto modulu uvolnit queue
    char* q_item=getQueueFreeItem();            //je-li queue plna, vola doEvents
    if(q_item==NULL) { return; }                //nemelo by nikdy nastat
    
    //prevod y na page
    char page=y/8;
    
    q_item[0]=font;
    q_item[1]=index;
    q_item[2]=x;                       
    q_item[3]=page;  
    
    //text, kopiruje dokud nenajde \0, nebo neni konec polozky queue
    int a;
    char data;
    for(a=4; a<Q_ISIZE; a++)
    {
        data=string[a-4];
        q_item[a]=data;
        if(data==0x0) 
        {
            //nasel konec stringu (\0)
            break;
        }
    }
    
    queue_usedItems++;
}

void disp1306a_drawImage(int index, char* image, int x, int y)
{
    //image=adresa pole bytes
    //image[0]=id, image[1]=size(bytes), image[2]=width[px], image[3]=height(px), image[4...]=data
    
    //vlozi do queue polozku, pokud je queue plna, vola doEvents
    //zvysi queue_usedItems
    
    //spi1_1306_drawString se vola z jinych apps, proto doEvents umozni tomuto modulu uvolnit queue
    char* q_item=getQueueFreeItem();            //je-li queue plna, vola doEvents
    if(q_item==NULL) { return; }                //nemelo by nikdy nastat
    
    //prevod y na page
    char page=y/8;
    
    q_item[0]=IMAGE1306;
    q_item[1]=index;
    q_item[2]=x;                       
    q_item[3]=page; 
    
    //q_item[4-7]=image; 
    int* i=(int*)q_item;
    i[1]=(int)image;
    
    queue_usedItems++;
}

void disp1306a_fillRect(int index, int x1, int y1, int x2, int y2, int color)
{
    //vyplnuje obdelnik zadanym byte, x a y jsou body displeje (0-127, 0-63)
    //na vysku vyplnuje jednotlive pages, nikoliv body
    
    //vlozi do queue polozku, pokud je queue plna, vola doEvents
    //zvysi queue_usedItems
    
    //spi1_1306_drawString se vola z jinych apps, proto doEvents umozni tomuto modulu uvolnit queue
    char* q_item=getQueueFreeItem();            //je-li queue plna, vola doEvents
    if(q_item==NULL) { return; }                //nemelo by nikdy nastat
    
    q_item[0]=FILL1306;
    q_item[1]=index;
    q_item[2]=x1;                   //start x                    
    q_item[3]=y1/8;                 //start page
   
    q_item[4]=x2;                   //end x
    q_item[5]=y2/8;                 //end page
    q_item[6]=color; 
    
    queue_usedItems++;
}

void disp1306a_print(int index, char* string)
{

#ifdef CAN_PRINT  
    
    int must_repaint=0;
    if(empty_line > 3)
    {
        //posune polozky lines[], uvolni posledni
        char* l=lines[1];
        lines[0]=l;
        
        l=lines[2];
        lines[1]=l;
        
        l=lines[3];
        lines[2]=l;
        
        empty_line=3;
        must_repaint=1;
    }
    
    int a, exist=0;
    char* line=lines[empty_line];

    //kopiruje string do line[], vzdy 16 znaku, je-li string kratsi, doplni mezery aby prepsal stary radek
    for(a=0; a<16; a++)
    {
        if(exist==0)
        {
            char data=string[a];
            if(data==0)
            {
                exist=1; line[a]=32;
            }
            else
            {
                line[a]=string[a];
            }
        }
        else
        {
            line[a]=32;
        }
    }
    
    if(must_repaint==1)
    {
        //spi1_1306_clear(index);
        disp1306a_drawText(index, lines[0], 0, 0, FONT1306_MID);
        disp1306a_drawText(index, lines[1], 0, 1, FONT1306_MID);
        disp1306a_drawText(index, lines[2], 0, 2, FONT1306_MID);
        disp1306a_drawText(index, lines[3], 0, 3, FONT1306_MID);        
    }
    else
    {
       //char* arr=*lines[empty_line];
       disp1306a_drawText(index, lines[empty_line], 0, empty_line, FONT1306_MID); 
    }
    empty_line++;
    
#endif
    
}

void disp1306a_clear(int index)
{
    char* item = getQueueFreeItem();
    if (item != NULL) {
        item[0] = ZERO1306;
        item[1] = index;        //display index
        item[2] = 0;            //col / pocet znaku pro command = 2
        item[3] = 0;            //row - nepouzito
        
        queue_usedItems++;
    }     
}

void disp1306a_setContrast(int index, int value)
{
    char* item = getQueueFreeItem();
    if (item != NULL) {
        item[0] = COMMAND1306;
        item[1] = index;        //display index
        item[2] = 2;            //col / pocet znaku pro command = 2
        item[3] = 0;            //row - nepouzito
        
        item[4]=0x81;           //command
        item[5]=value;          //hodnota
        
        queue_usedItems++;
    }    
}

void disp1306a_sleep(int index)
{
    char* item = getQueueFreeItem();
    if (item != NULL) {
        item[0] = COMMAND1306;
        item[1] = index;        //display index
        item[2] = 1;            //col / pocet znaku pro command = 2
        item[3] = 0;            //row - nepouzito
        
        item[4]=0xAE;           //command
        
        queue_usedItems++;
    }      
}

void disp1306a_resume(int index)
{
    char* item = getQueueFreeItem();
    if (item != NULL) {
        item[0] = COMMAND1306;
        item[1] = index;        //display index
        item[2] = 1;            //col / pocet znaku pro command = 2
        item[3] = 0;            //row - nepouzito
        
        item[4]=0xAF;           //command
        
        queue_usedItems++;
    }      
}

int disp1306a_getReady(int index)
{
    //po inicializaci vraci 1, jinak 0
    return ready[index];
}

char* disp1306a_getImageData(int image_id)
{
    //vraci adresu image (head)
    //head[0]=id, head[1]=data size, head[2]=width, head[3]=height
    int pos=0;
    while(1)
    {
        char id=icon_1306[pos];
        
        if(id==0){ return NULL; }                               //id=0, konec, nenasel data
        if(id==image_id){ return (char*)(icon_1306 + pos);}     //nasel hledane id
        
        pos+=4 + icon_1306[pos+1];                              //pos += head + data_size
    }    
}

int disp1306a_getWidth(int index)
{
    return 128;
}

int disp1306a_getHeight(int index)
{
    return 64;
}

static void loop()
{
loop_label: ;

    if(queue_usedItems > 0) 
    { 
        processItem();
    }        
    
    doEventsL((uint*)&&loop_label);    
}

static void processItem()
{
    //q_item[0]=font
    //q_item[1]=index displeje
    //q_item[2]=col/len-pro-command
    //q_item[3]=row
    //q_item[4...Q_ISIZE-1]=data
    
    //FONT1306_MID 16 cols, 4 rows
    //FONT1306_BIG  8 cols, 2 rows - pouze cisla a "+-\."
    
    char* q_item = queue + (queue_processItemIndex * Q_ISIZE);
    char font=q_item[0];
    
    if(font != 0 )
    {
        int index=q_item[1];                //index displeje
    
        if(font==FONT1306_MID)              //font=1
        {    
            char* string=q_item+4;
            char x=q_item[2];
            char page=q_item[3];
            while(*string != 0x0)
            {
                //platny znak (neni \0 - konec stringu)
                drawMidChar(index, x, page, *string);
                x+=8;
                if(x >= 128) 
                {
                    //max 128 px (16 znaku)
                    break;
                }
                string++;
            }
        }
        else if(font==FONT1306_BIG)         //font=2
        {
            char* string=q_item+4;
            char x=q_item[2];
            char page=q_item[3];        
            while(*string != 0x0)
            {
                //platny znak (neni \0 - konec stringu)
                drawBigChar(index, x, page, *string);
                x+=16;
                if(x >= 128) 
                {
                    //max 128 px (8 znaku)
                    break;
                } 
                string++;
            }
        }
        else if(font==COMMAND1306)
        {
            //odeslani command    
            char* string=q_item+4;
            int len=q_item[2];
#ifdef USE_SPI_PORT            
            //SPI
            send(index, string, len, SPI_DC_COMMAND);
#endif      
#ifdef USE_I2C_PORT            
            //I2C
            //i2c_buffer_cmd[0]=0x0 - command prefix
            send_i2c(index, i2c_buffer_cmd, 1, string, len);
#endif   
        }
        
        else if(font==IMAGE1306)
        {
            //q_item[4-7]=adresa dat
            int* i=(int*)q_item;
            char* img=(char*)i[1];
        
            drawImage(index, q_item[2], q_item[3], img);
        }
        else if(font==FILL1306)
        {
            //vyplni obdelnik zadanum bytem
            //index, byte, x1, page1, x2, page2
            fillByte(index, q_item[6], q_item[2], q_item[3], q_item[4], q_item[5]);
        }
        else
        {
            //font=ZERO1306, nuluje display
            //vypisuje mezery
            int x=0, p=0;
    
            for(x=0; x<128; x+=8)
            {
                for(p=0; p<8; p+=2)
                {
                    drawMidChar(index, x, p, 32);      //32 = ascii kod mezera
                }
            }        
        }        
        
    }
    
    q_item[0]=0;                        //oznacit jako empty
    queue_usedItems--;
    queue_processItemIndex++;                                            //nastav dalsi polozku jako aktualni
    if(queue_processItemIndex >= Q_CAPA) { queue_processItemIndex=0; }
    
}

/*
static void drawData(char* q_item)
{
    //q_item[0]=font
    //q_item[1]=index displeje
    //q_item[2]=col/len-pro-command
    //q_item[3]=row
    //q_item[4...Q_ISIZE-1]=data
    
    //FONT1306_MID 16 cols, 4 rows
    //FONT1306_BIG  8 cols, 2 rows - pouze cisla a "+-\."
    
    char font=q_item[0];
    int index=q_item[1];
    
    if(font==FONT1306_MID)              //font=1
    {    
        char* string=q_item+4;
        char x=q_item[2];
        char page=q_item[3];
        while(*string != 0x0)
        {
            //platny znak (neni \0 - konec stringu)
            drawMidChar(index, x, page, *string);
            x+=8;
            if(x >= 128) 
            {
                //max 128 px (16 znaku)
                break;
            }
            string++;
        }
    }
    else if(font==FONT1306_BIG)         //font=2
    {
        char* string=q_item+4;
        char x=q_item[2];
        char page=q_item[3];        
        while(*string != 0x0)
        {
            //platny znak (neni \0 - konec stringu)
            drawBigChar(index, x, page, *string);
            x+=16;
            if(x >= 128) 
            {
                //max 128 px (8 znaku)
                break;
            } 
            string++;
        }
    }
    else if(font==COMMAND1306)
    {
        //odeslani command    
        char* string=q_item+4;
        int len=q_item[2];
        send(index, string, len, SPI_DC_COMMAND);
    }
    else if(font==IMAGE1306)
    {
        //q_item[4-7]=data
        int* i=(int*)q_item;
        char* img=(char*)i[1];
        
        drawImage(index, q_item[2], q_item[3], img);
    }
    else if(font==FILL1306)
    {
        //index, byte, x1, page1, x2, page2
        fillByte(index, q_item[6], q_item[2], q_item[3], q_item[4], q_item[5]);
    }
    else
    {
        //font=ZERO1306, nuluje display
        //vypisuje mezery
        int x=0, p=0;
    
        for(x=0; x<128; x+=8)
        {
            for(p=0; p<16; p+=2)
            {
                drawMidChar(index, x, p, 32);      //32 = ascii kod mezera
            }
        }        
    }
 
}
*/

static void drawMidChar(int index, char x, char page, char code)
{
    //x=0-15 sloupec
    //y=0-3  radek
    //code=ascii kod
    //fce zapisuje jeden znak, ceka na dokonceni (doEvents)
    //znak 8x16
    
    //adresy v RAM displeje, column a page   
    //char start_col=x*8;                             //sirka znaku 8 bodu
    //char start_page=y*2;                            //vyska 2x page (16 bodu)
    char end_x=x+7;
    char end_page=page+1;
    
#ifdef USE_I2C_PORT
    //I2C
    //i2c_buffer[0]=0x80;             //command, POZOR - prvni byte prepisuje send command (napr. RESET)
    //i2c_buffer[1]=0x21;             //set x    
    //i2c_buffer[2]=0x80;             //command
    i2c_buffer[3]=x;                //x start
    //i2c_buffer[4]=0x80;             //command
    i2c_buffer[5]=end_x;            //x end
    //i2c_buffer[6]=0x80;             //command
    //i2c_buffer[7]=0x22;             //set page
    //i2c_buffer[8]=0x80;             //command
    i2c_buffer[9]=page;             //start page
    //i2c_buffer[10]=0x80;            //commandge
    i2c_buffer[11]=end_page;        //end page
    //i2c_buffer[12]=0x40;            //data
    
    //data
    char* bitmap=getMidAsciiData(code);             //adresa dat znaku (ve flash)
    send_i2c(index, i2c_buffer, 13, bitmap, 16); 
    //send_i2c(index, bitmap, 16, 13); 
#endif

#if USE_SPI_PORT    

    //SPI
    command_cp[0]=0x21;
    command_cp[1]=x;
    command_cp[2]=end_x;
    send(index, command_cp, 3, SPI_DC_COMMAND);
      
    command_cp[0]=0x22;
    command_cp[1]=page;
    command_cp[2]=end_page;
    send(index, command_cp, 3, SPI_DC_COMMAND);

    //data
    char* bitmap=getMidAsciiData(code);             //adresa dat znaku (ve flash)
    send(index, bitmap, 16, SPI_DC_DATA);
#endif
    
}

static void drawBigChar(int index, char x, char page, char code)
{
    //x=0-7 sloupec
    //y=0-1  radek
    //code=ascii kod (. / 0-9)
    //fce zapisuje jeden znak, ceka na dokonceni (doEvents)
    //znak 16x32
    
    //adresy v RAM displeje, column a page    
    //char start_col=x*16;                            //sirka znaku 16 bodu
    //char start_page=y*4;                            //vyska 4x page (32 bodu)
    char end_x=x+15;
    char end_page=page+3;
    
#ifdef USE_I2C_PORT
    //I2C
    //i2c_buffer[0]=0x80;             //command, POZOR - prvni byte prepisuje send command (napr. RESET)
    //i2c_buffer[1]=0x21;             //set x    
    //i2c_buffer[2]=0x80;             //command
    i2c_buffer[3]=x;                //x start
    //i2c_buffer[4]=0x80;             //command
    i2c_buffer[5]=end_x;            //x end
    //i2c_buffer[6]=0x80;             //command
    //i2c_buffer[7]=0x22;             //set page
    //i2c_buffer[8]=0x80;             //command
    i2c_buffer[9]=page;             //start page
    //i2c_buffer[10]=0x80;            //commandge
    i2c_buffer[11]=end_page;        //end page
    //i2c_buffer[12]=0x40;            //data
    
    //data
    char* bitmap=getMidAsciiData(code);             //adresa dat znaku (ve flash)
    send_i2c(index, i2c_buffer, 13, bitmap, 64); 
#endif    
    
#ifdef USE_SPI_PORT    
    command_cp[0]=0x21;
    command_cp[1]=x;
    command_cp[2]=end_x;
    send(index, command_cp, 3, SPI_DC_COMMAND);
    
    command_cp[0]=0x22;
    command_cp[1]=page;
    command_cp[2]=end_page;
    send(index, command_cp, 3, SPI_DC_COMMAND);
    
    //data
    char* bitmap=getBigAsciiData(code);             //adresa dat znaku (ve flash)
    send(index, bitmap, 64, SPI_DC_DATA);
#endif    
}

static void drawImage(int index, char x, char page, char* img)
{
    //data[0]=id, data[1]=bytes, data[2]=width(px), data[3]=height(px)
    //POZOR, max. velikost obrazku je omezena velikost i2c_buffer
    char end_x = x+img[2]-1;
    char end_page= page+((img[3]/8)-1);
    
#ifdef USE_I2C_PORT  
    //I2C
    //i2c_buffer[0]=0x80;             //command, POZOR - prvni byte prepisuje send command (napr. RESET)
    //i2c_buffer[1]=0x21;             //set x    
    //i2c_buffer[2]=0x80;             //command
    i2c_buffer[3]=x;                //x start
    //i2c_buffer[4]=0x80;             //command
    i2c_buffer[5]=end_x;            //x end
    //i2c_buffer[6]=0x80;             //command
    //i2c_buffer[7]=0x22;             //set page
    //i2c_buffer[8]=0x80;             //command
    i2c_buffer[9]=page;             //start page
    //i2c_buffer[10]=0x80;            //commandge
    i2c_buffer[11]=end_page;        //end page
    //i2c_buffer[12]=0x40;            //data
    
    //data
    //send_i2c(index, img+4, img[1], 13);  
    send_i2c(index, i2c_buffer, 13, img+4, img[1]);
#endif   
    
#ifdef USE_SPI_PORT  
    //SPI
    command_cp[0]=0x21;
    command_cp[1]=x;
    command_cp[2]=end_x;
    send(index, command_cp, 3, SPI_DC_COMMAND);    
    
    command_cp[0]=0x22;
    command_cp[1]=page;
    command_cp[2]=end_page;
    send(index, command_cp, 3, SPI_DC_COMMAND);    

    //data
    send(index, img+4, img[1], SPI_DC_DATA);
#endif    
}

static void fillByte(int index, char byte, int x1, int page1, int x2, int page2)
{
    //vyplnuje zadany obdelnik pozadovanym byte
    //max. velikost bt[] je 1 page (128 bytes, kvuli velikosti zasobniku)
    //tzn. ze fce send odesila vzdy pouze jeden radek (page)
    
#ifdef USE_SPI_PORT  
    //I2C
    //not implemented
#endif    
    
#ifdef USE_SPI_PORT  
    //SPI
    int cols=x2 - x1 + 1;
    int pages=page2 - page1 + 1;

    char bt[cols];
    int a;
    for(a=0; a<cols; a++) { bt[a]=byte; }
    
    command_cp[0]=0x21;
    command_cp[1]=x1;
    command_cp[2]=x2;
    send(index, command_cp, 3, SPI_DC_COMMAND); 
    
    command_cp[0]=0x22;
    command_cp[1]=page1;
    command_cp[2]=page2;
    send(index, command_cp, 3, SPI_DC_COMMAND); 
    
    //data
    for(a=0; a<pages; a++)
    {
        send(index, bt, cols, SPI_DC_DATA);
    }
#endif    
}

static char* getQueueFreeItem()
{
    //vraci adresu polozky, neni-li zadna volna provede doEvents
    
    while(queue_usedItems == Q_CAPA)
    {
        //queue full
        doEvents();
    }

    //
    char xpos=queue_processItemIndex;
    while(1)
    {
        if(queue[xpos*Q_ISIZE] == 0)
        {
            //ok, nasel prazdnou, ukonci smycku
            break;
        }
        
        //dalsi polozka
        xpos++;
        if(xpos == Q_CAPA){xpos=0;}
        if(xpos == queue_processItemIndex)
        {
            //prosel celou queue, nenasel prazdnou polozku (nemelo by nastat)
            return NULL;
        }
    } 
    
    return queue+(xpos*Q_ISIZE);
}

static char fillInitData(char* buffer)
{
    //vraci pocet bytes, ktere vlozi
    buffer[0]=0xAE;
    buffer[1]=0xD5;
    buffer[2]=0x80;            
    buffer[3]=0xA8;            
    buffer[4]=0x3F;            
    buffer[5]=0xD3;      //offset
    buffer[6]=0x00;           
    buffer[7]=0x00;            
    buffer[8]=0x8D;      //charge pump
    buffer[9]=0x14;
    buffer[10]=0x20;            
    buffer[11]=0x00;            
    buffer[12]=0xA1;     //set remap, hw config
    buffer[13]=0xC8;
    buffer[14]=0xDA;            
    buffer[15]=0x12;            
    buffer[16]=0x81;     //contrast, pre-charge
    buffer[17]=0xCF;
    buffer[18]=0xD9;            
    buffer[19]=0xF1;            
    buffer[20]=0xDB;     //deselect level 
    buffer[21]=0x40;
    buffer[22]=0xA4;            
    buffer[23]=0xA6;            
    buffer[24]=0xAF; 
            
    return 25;
}

static void send(int index, char* buffer, int len, int dc)
{
#ifdef USE_SPI_PORT    
    while(spi_getUsed(USE_SPI_PORT) != SPI_STATE.EMPTY) { doEvents(); }
    //!!!spi_setUsed(USE_SPI_PORT, index); - fce zrusena, pouzit spi_Use
    
#ifndef _CSNONE_INDEX     
    //nastav CS signal, pouze pokud pouziva CS pin
    setCSPin(index, SPI_CS_ACTIVE);
#endif     
    
#ifdef _DC_pin    
    //nastav D/C pin
    setDCPin(dc);   
#endif    
    
    //spi_Exchange(USE_SPI_PORT, buffer, NULL, len, &sendFinish);
    
    //po skonceni transakce vola sendFinish(), ktera uvolni SPI
#endif    
}

/*
static void send_i2c(int index, char* buffer, int len, int pre_fill)
{
    //pre_fill je pocet bytes vyplnenych v i2c_bufferu, ktery
    //obsahuje vetsinou control byte, ten se musi vysilat pred samotnymi daty
    //v pripade drawMid/BigChar jsou v i2c_buffer jiz vyplneny command pro
    //nastaveni x a page (buffer obsahuje data znaku)
#ifdef USE_I2C_PORT     
    while(i2c_getWorking(USE_I2C_PORT) != I2C_EMPTY) { doEvents(); }
    i2c_setWorking(USE_I2C_PORT, index);  
    
    //int a;
    //for(a=0; a<len; a++)
    //{
    //    i2c_buffer[a+pre_fill]=buffer[a];
    //}
    
    //src, dest, len (delitelne 4)
    memcpy32(buffer, i2c_buffer+pre_fill, len);
    
    i2c_write(USE_I2C_PORT, i2c_buffer, len+pre_fill, I2C_ADDRESS, &send_i2cFinish);
#endif    
}
*/

static void send_i2c(int index, char* buffer, int len, char* alt_buffer, int alt_len)
{
    //buffer = i2c_buffer, ktery obsahuje control bytes (nastaveni row a col)
    //alt_buffer jsou data znaku ve flash
    //I2C modul odesila oba buffery bez stop-bitu mezi nimi
    
#ifdef USE_I2C_PORT     
    while(i2c_getWorking(USE_I2C_PORT) != I2C_EMPTY) { doEvents(); }
    i2c_setWorking(USE_I2C_PORT, index);  
    
    //odesila 2 buffery, buffer obsahuje command (row, col), alt_buffer obsahuje data znaku z flash
    i2c_writeAlt(USE_I2C_PORT, buffer, len, I2C_ADDRESS, &send_i2cFinish, alt_buffer, alt_len);
#endif    
}

static void sendFinish(int w)
{
    //vola SPI interrupt, po skonceni transakce
    //param w je hodnota Working (uklada do ni index)
    
#ifdef USE_SPI_PORT     
    
#ifndef _CSNONE_INDEX
    //nastav CS, pouze pokud pouziva CS pin
    setCSPin(w, SPI_CS_DEACTIVE);
#endif      
     
    //uvolnit SPI
    //spi_setWorking(USE_SPI_PORT, SPI_EMPTY);
    
#endif  
}

static void send_i2cFinish(int w)
{
#ifdef USE_I2C_PORT     
    i2c_setWorking(USE_I2C_PORT, I2C_EMPTY);
#endif    
}

static char* getMidAsciiData(char code)
{
    //prvni znak ma kod 32 - mezera
    int offset = (code-32) * 16;                      //velikost znaku 16 bytes
    return (char*)(font_1306_mid + offset);
}

static char* getBigAsciiData(char code)
{
    //prvni znak ma kod 46 = "." (des. tecka)
    int offset = (code-43) * 64;                      //velikost znaku 64 bytes
    return (char*)(font_1306_big + offset);
}

/*
static char* getImageData(int image_id)
{
    //vraci adresu image (head)
    //head[0]=id, head[1]=data size, head[2]=width, head[3]=height
    int pos=0;
    while(1)
    {
        char id=icon_1306[pos];
        
        if(id==0){ return NULL; }                               //id=0, konec, nenasel data
        if(id==image_id){ return (char*)(icon_1306 + pos);}     //nasel hledane id
        
        pos+=4 + icon_1306[pos+1];                              //pos += head + data_size
        
    }
}
*/

static void resetDisplay()
{
    //provede UP, DOWN (reset), UP
    //je-li zapojeno vice displeju, maji spolecny RESET (i D/C)
#ifdef _RESET_pin     
    
    //UP
    setResetPin(1);
    pause(100);
    //DOWN - probiha reset displeje
    setResetPin(0);
    pause(5000);
    //UP
    setResetPin(1);
    pause(100);
    
#endif    
    
}

static void pause(int t)
{
    int a;
    for(a=0; a<t; a++)
    {
        a++;
        a--;
    }
}

static void setDCPin(int value)
{
    //RB6 (pin 15)
    _DC_pin=value;
    //LATBbits.LATB6=value;
}

static void setCSPin(int index, int value)
{
    //nastav CS pin na value
    

#ifdef _CS0_pin
    if(index==0) { _CS0_pin=value; }
#endif           

#ifdef _CS1_pin
    if(index==1) { _CS1_pin=value; }
#endif 

#ifdef _CS2_pin
    if(index==2) { _CS2_pin=value; }
#endif 

#ifdef _CS3_pin
    if(index==3) { _CS3_pin=value; }
#endif 

#ifdef _CS4_pin
    if(index==4) { _CS4_pin=value; }
#endif 

#ifdef _CS5_pin
    if(index==5) { _CS5_pin=value; }
#endif 

#ifdef _CS6_pin
    if(index==6) { _CS6_pin=value; }
#endif 
    
#ifdef _CS7_pin
    if(index==7) { _CS7_pin=value; }
#endif     

    //LATBbits.LATB3=value;
}

static void setResetPin(int value)
{
    //RB7 (pin 16)
    _RESET_pin=value;
}

#endif //disp1306