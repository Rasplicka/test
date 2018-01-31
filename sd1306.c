#include "sd1306.h"
#include "periph.h"

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

//fonts

// <editor-fold defaultstate="collapsed" desc="local definitions">

//pouzite piny (v initPort musi byt nastaveny jako OUT)
//SPI signaly a hodnoty
#define     SPI_CS_ACTIVE       0           //aktivni uroven L
#define     SPI_CS_DEACTIVE     1           //neaktivni uroven H
#define     SPI_DC_COMMAND      0           //DC signal L=command
#define     SPI_DC_DATA         1           //DC signal H=data

#define     _CSNONE                                                 //pokud neni pouzit CS signal (trvale LOW)
//#define     _CS0_pin        LATBbits.LATB3  //(SO28 pin 7)
//#define     _CS1_pin        0               //none
//#define     _CS2_pin        0               //none
//#define     _CS3_pin        0               //none
#define     _DC_pin         LATBbits.LATB6  //(SO28 pin 15)
#define     _RESET_pin      LATBbits.LATB7  //(SO28 pin 16)

//queue
#define     Q_ISIZE         32              //velikost polozky queue
#define     Q_CAPA          8               //pocet polozek queue

#define     FUNC_D0(fn, str, x, y, font)    (fn)((font),(x),(y),(str)) 

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="local vars">

char stack[512];
char* sd1306_stackTop=stack+512-4;

char queue[Q_ISIZE * Q_CAPA];                       //obsahuje texty, urcene pro vypis na display
char queue_count = 0;                               //pocet platnych polozek
char queue_process = 0;                             //index prave provadene polozky
char spi_status = 0;                                //stav SPI 0=volne, 1=zacatek vys. 2=D/C nastaveno na command, 3=D/C nastaveno na data

char disp_sequence_buffer[4]; //pouze pro command
char* disp_sequence_pos;
char disp_sequence_comm_count;
char disp_sequence_data_count;

char aktualDispIndex = 0;                           //index aktualniho displeje (CS) - nastavuje pouze fce loop() a initDisp())

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="local fn">
void loop();
void drawData(char font, char x, char y, char* data);

void drawMidChar(char x, char y, char code);
void drawBigChar(char x, char y, char code);
void send(char* buffer, int len, int dc);

void writeOneCommand(char len, char d0, char d1, char d2, char d3);
char* getMidAsciiData(char code);
char* getBigAsciiData(char code);

void writeSPISequenceInt();
void resetDisplay();
void initDisplay();

void setDCPin(int value);
void setCSPin(int value);
void setResetPin(int value);

void drawData1(char font, char x, char y, char* data);
void drawData2(char font, char x, char y, char* data);

// </editor-fold>

//definuje fce pro kazdy displayIndex [0-7]
//kazdy displej muze mit vlastni driver, tzn. ze mohou byt pouzity ruzne displeje soucasne 
void (*_drawData)(char, char, char, char*);//=&drawData;
const void* func[]={&drawData1, &drawData2};


//functionPtr func[]={&drawData, &drawData};

void sd1306_start()
{
    //char x[5]={0,1,2,3,4};

    //_drawData=func[0];
    //_drawData(0, 0, 0, x);
    
    //_drawData=func[1];
    //_drawData(0, 0, 0, x);
    
    
    //oznaci vsechny polozky queue jako empty (prvni byte = -1)
    int a;
    for(a=0; a<Q_CAPA; a++)
    {
        queue[a * Q_ISIZE]=0x0;
    }
    
        //test
    char d0[]="|||||...........\0";
    char d1[]="pyw_mnij! 85~C\0";
    
    for(a=0; a<20; a++)
    {
        sd1306_drawString(1, 0, 2, d0);
    }
    
    
    resetDisplay();
    initDisplay();
    sd1306_clearDisplay();
    
    //test
    //char d2[]="8910.44\0";        
    //drawString(FONT1306_BIG, 0, 0, d2);
    
    //drawMidChar(0, 0, 'A');
    

    //drawString(1, 0, 3, d1);
    
    loop();
}

void sd1306_drawString(char font, char x, char y, char* string)
{
    //vraci -1 pri chybe (full), nebo 0=ok
    //kopiruje data do queue, pokud je volne misto
    //string \0, max 16+1 znaku. Pokud neni na konci \0, fce doplni jako 17-ty znak
    
    //test zda neni queue full
    if(queue_count == Q_CAPA) 
    { 
        //full
        return; 
    }  

    //cekani na uvolneni queue
    //while(queue_count >= Q_CAPA)
    //{
    //    doEvents();
    //}
    
    char xpos=queue_process;
    while(1)
    {
        if(queue[xpos*Q_ISIZE] == 0)
        {
            //ok, nasel prazdnou, ukonci smycku
            break;
        }
        
        //dalsi polozka
        xpos++;
        if(xpos==Q_CAPA){xpos=0;}
        if(xpos== queue_process)
        {
            //prosel celou queue, nenasel prazdnou polozku (nemelo by nastat)
            return;
        }
    }
    
    
    /*
    if(queue[xpos*Q_ISIZE] == 0)
    {
        //aktualni polozka je empty, pouzije ji
        nasel=1;
    }
    else
    {
        //aktualni polozka neni empty, hleda dalsi
        xpos++;
        if(xpos==Q_CAPA){xpos=0;}
        
        while(xpos != queue_process)
        {
            if(queue[xpos*Q_ISIZE] == 0)
            {
                nasel=1;
                break;
            }
            
            xpos++;
            if(xpos==Q_CAPA){xpos=0;}
        }
    }
    
    if(nasel==0){ return -1; }      //nemelo by nastat (viz. queue_count)
    */
    
    //xpos je pozice volne polozky
    
    int index=xpos*Q_ISIZE;
    queue[index]=font;
    queue[index+1]=sd1306_displayIndex;
    queue[index+2]=x;                       //col
    queue[index+3]=y;                       //row
    
    //kopiruje text
    int a;
    char data;
    //int exist_end=0;
    
    if(font==COMMAND1306)
    {
        //command, kopiruje vzdy 4 bytes 
        queue[index+4]=string[0];
        queue[index+5]=string[1];
        queue[index+6]=string[2];
        queue[index+7]=string[3];
    }
    else if(font==ZERO1306)
    {
        //nuluje cely display
    }
    else
    {   
        //text, kopiruje dokud nenajde \0, nebo neni konec polozky queue
        for(a=4; a<Q_ISIZE; a++)
        {
            data=string[a-4];
            queue[index + a]=data;
            if(data==0x0) 
            {
                //nasel konec stringu (\0)
                //exist_end=1;
                break;
            }
        }
    }
    
    /*
    if(exist_end==0)
    {
        //string neobsahuje \0 na konci, doplni na posledni pozici
        queue[index + Q_ISIZE-1]=0x0;
    }
    */
    
    queue_count++;
    
    //return 0;
}

void sd1306_clearDisplay()
{
    //vypisuje mezery
    
    sd1306_drawString(ZERO1306, 0, 0, NULL);
    
    /*
    int c=0, p=0;
    
    for(c=0; c<16; c++)
    {
        for(p=0; p<4; p++)
        {
            drawMidChar(c, p, 32);      //32 = ascii kod mezera
        }
    }
    */
}

void sd1306_setContrast(char value)
{
    //contrast (0x81) + value  0(min)... 255(max)
    //vraci -1 pro chybe (queue full)
    char data[4];
    data[0]=0x81;
    data[1]=value;
    data[2]=0;
    data[3]=0;
    sd1306_drawString(COMMAND1306, 2, 0, data);
}

void sd1306_sleep()
{
    //sleep mode
    //vraci -1 pro chybe (queue full)
    char data[4];
    data[0]=0xAE;
    data[1]=0;
    data[2]=0;
    data[3]=0;
    sd1306_drawString(COMMAND1306, 1, 0, data);    
}

void sd1306_resume()
{
    //sleep mode
    //vraci -1 pro chybe (queue full)
    char data[4];
    data[0]=0xAF;
    data[1]=0;
    data[2]=0;
    data[3]=0;
    sd1306_drawString(COMMAND1306, 1, 0, data);    
}


void loop()
{
loop_start: ;
    if(queue_count == 0) { doEventsL((uint*)&&loop_start); }        //queue empty

    if(spi_status != 0) { doEventsL((uint*)&&loop_start); }         //probiha jine vysilani

    char* data = queue + (queue_process * Q_ISIZE);
    if(data[0] == 0) 
    {
        //nemelo by nikdy nastat.
        doEventsL((uint*)&&loop_start);
    }
    else
    {
        //nastav aktualDispIndex (displej 0-3)
        aktualDispIndex=data[1];
        
        //font Mid i Big
        //drawData(data[0], data[2], data[3], data+4);
        //const void* ptr=func[0];
        
        //FUNC_D0(func[0], data+4, data[2], data[3], data[0]);
        _drawData=func[0]; //&drawData;
        _drawData(data[0], data[2], data[3], data+4);
        
        data[0] = 0;                                                //oznaci polozku jako empty
        queue_count--;
        queue_process++;                                            //nastav dalsi polozku jako aktualni
        if(queue_process >= Q_CAPA) { queue_process=0; }
    }
    
    //doEvents();
    //goto loop_start ;
    doEventsL((uint*)&&loop_start);
}

void drawData1(char font, char x, char y, char data[])
{
    int a=(int)data[0];
    int b=2;
    int c=a+b;
}

void drawData2(char font, char x, char y, char* data)
{
    int a=1;
    int b=2;
    int c=a+b;
}

void drawData(char font, char x, char y, char* data)
{
    //FONT1306_MID 16x4
    //FONT1306_BIG  8x2 - pouze cisla, "\", "."

    //platny znak (neni \0 - konec stringu)
    if(font==FONT1306_MID)              //font=1
    {    
        while(*data != 0x0)
        {
            drawMidChar(x, y, *data);
            x++;
            if(x >= 16) 
            {
                //max 16 znaku
                break;
            }
            data++;
        }
    }
    else if(font==FONT1306_BIG)         //font=2
    {
        while(*data != 0x0)
        {
            drawBigChar(x, y, *data);
            x++;
            if(x >= 8) 
            {
                //max 8 znaku
                break;
            } 
            data++;
        }
    }
    else if(font==COMMAND1306)
    {
        //x=pocet znaku v prikazu, max 4
        //y=nepouzito
        //data[0,1,2,3]=data
        writeOneCommand(x, *data+0, *data+1, *data+2, *data+3);
    }
    else
    {
        //font=ZERO1306, nuluje display
        //vypisuje mezery
        int c=0, p=0;
    
        for(c=0; c<16; c++)
        {
            for(p=0; p<4; p++)
            {
                drawMidChar(c, p, 32);      //32 = ascii kod mezera
            }
        }        
    }
 
}


//fce zavisle na interface (SPI, I2C, ...)
void drawMidChar(char x, char y, char code)
{
    //x=0-15 sloupec
    //y=0-3  radek
    //code=ascii kod
    //fce zapisuje jeden znak, ceka na dokonceni (doEvents)
    //znak 8x16
    
    //adresy v RAM displeje, column a page   
    char start_col=x*8;                             //sirka znaku 8 bodu
    char start_page=y*2;                            //vyska 2x page (16 bodu)
    char end_col=start_col+7;
    char end_page=start_page+1;
    
    char buff[3];
    
    //command set col
    buff[0]=0x21;
    buff[1]=start_col;
    buff[2]=end_col;
    send(buff, 3, SPI_DC_COMMAND);
    
    //command set page
    buff[0]=0x22;
    buff[1]=start_page;
    buff[2]=end_page;
    send(buff, 3, SPI_DC_COMMAND);
    
    //data
    char* bitmap=getMidAsciiData(code);             //adresa dat znaku (ve flash)
    send(bitmap, 16, SPI_DC_DATA);
    
    /*
    //odesla command nastaveni column
    writeOneCommand(3, 0x21, start_col, end_col, 0);
    //odeslat command nastaveni page
    writeOneCommand(3, 0x22, start_page, end_page, 0);

    //data
    while (spi_status != 0) { doEvents(); }

    //nastav pozici prvniho znaku na zacatek
    disp_sequence_comm_count=0;                 //pouze data
    disp_sequence_data_count=16;                //znak ma 16 bytes 
    
    disp_sequence_pos=getMidAsciiData(code);    //adresa dat znaku (ve flash)     
    spi_status=1;   
    
    writeSPISequenceInt();
    */

}

void drawBigChar(char x, char y, char code)
{
    //x=0-7 sloupec
    //y=0-1  radek
    //code=ascii kod (. / 0-9)
    //fce zapisuje jeden znak, ceka na dokonceni (doEvents)
    //znak 16x32
    
    //adresy v RAM displeje, column a page    
    char start_col=x*16;                            //sirka znaku 16 bodu
    char start_page=y*4;                            //vyska 4x page (32 bodu)
    char end_col=start_col+15;
    char end_page=start_page+3;
    
    char buff[3];
    
    //command set col
    buff[0]=0x21;
    buff[1]=start_col;
    buff[2]=end_col;
    send(buff, 3, SPI_DC_COMMAND);
    
    //command set page
    buff[0]=0x22;
    buff[1]=start_page;
    buff[2]=end_page;
    send(buff, 3, SPI_DC_COMMAND);
    
    //data
    char* bitmap=getBigAsciiData(code);             //adresa dat znaku (ve flash)
    send(bitmap, 64, SPI_DC_DATA);
    
    
    /*
    //odesla command nastaveni column
    writeOneCommand(3, 0x21, start_col, end_col, 0);
    
    //odeslat command nastaveni page
    writeOneCommand(3, 0x22, start_page, end_page, 0);
    
    //data
    while (spi_status != 0) { doEvents(); }
    //nastav pozici prvniho znaku na zacatek
    disp_sequence_comm_count=0;                 //pouze data
    disp_sequence_data_count=64;                //znak ma 64 bytes 
    
    disp_sequence_pos=getBigAsciiData(code);    //adresa dat znaku (ve flash)     
    spi_status=1;           
    writeSPISequenceInt();
    */
}

void send(char* buffer, int len, int dc)
{

    while(SPI1Control.working != 0) { doEvents(); }     //ceka na uvolneni SPI
    
#ifndef _CSNONE     
    //pouze, pokud pouziva CS pin
    setCSPin(SPI_CS_ACTIVE);
#endif     
    
    setDCPin(dc);
     
     
    SPI1Control.working=1;                              //pouzit SPI 
    spi1Exchange(buffer, NULL, len);                    //zajisti start vysilani
     
     while(SPI1Control.inProcess != 0) { doEvents(); }  //ceka na dokonceni

#ifndef _CSNONE
     //pouze, pokud pouziva CS pin
     setCSPin(SPI_CS_DEACTIVE);
#endif      
     
    //uvolnit SPI 
    SPI1Control.working=0;
    
}



void writeOneCommand(char len, char d0, char d1, char d2, char d3)
{
    //ceka na dokonceni
    while (spi_status != 0) 
    { 
        doEvents(); 
    }

    //nastav pozici prvniho znaku na zacatek
    disp_sequence_pos=disp_sequence_buffer;
    
    disp_sequence_comm_count=len;
    disp_sequence_data_count=0;
    
    disp_sequence_buffer[0]=d0;
    disp_sequence_buffer[1]=d1;
    disp_sequence_buffer[2]=d2;
    disp_sequence_buffer[3]=d3;
    
    spi_status = 1;
    writeSPISequenceInt();
}

char* getMidAsciiData(char code)
{
    //prvni znak ma kod 32 - mezera
    int offset = (code-32) * 16;                      //velikost znaku 16 bytes
    return (char*)(font_1306_mid + offset);
}

char* getBigAsciiData(char code)
{
    //prvni znak ma kod 46 = "." (des. tecka)
    int offset = (code-46) * 64;                      //velikost znaku 64 bytes
    return (char*)(font_1306_big + offset);
}





void writeSPISequenceInt()
{
    #define SPI_HW_BUFFER_SIZE  16   //!!! 16
    #define SPI_DC_COMMAND      0
    #define SPI_DC_DATA         1
    #define SPI_CS_ACTIVE       0
    #define SPI_CS_DEACTIVE     1
    
    //odesila data z disp_sequence_buffer, vola se po kazdem vysilani aby
    //jedno volani odesila max SPI_HW_BUFFER_SIZE
    
    if(spi_status == 0) {return;}
    
    if(disp_sequence_comm_count > 0)
    {
        //vysila command
        if(spi_status != 2)
        {
            //musi nastavit D/C bit na command
            setDCPin(SPI_DC_COMMAND);
            spi_status = 2;
#ifndef _CSNONE            
            setCSPin(SPI_CS_ACTIVE);
#endif            
        }
        
        int c=0;
        while(disp_sequence_comm_count > 0 && c<SPI_HW_BUFFER_SIZE)
        {
            c++; 
            disp_sequence_comm_count--;                  //pocet --
            //odelila command, max SPI_HW_BUFFER_SIZE bytes
            SPI1BUF=*disp_sequence_pos;
        
            disp_sequence_pos++;                        //dalsi znak
        }
        
        //povolit TX buffer empty interrupt
        IEC0SET=0x00200000; 
        return;    
        
    }
    else if (disp_sequence_data_count > 0)
    {
        //vysila data
        if(spi_status != 3)
        {
            //musi nastavit D/C bit na data
            setDCPin(SPI_DC_DATA);
            spi_status = 3;
#ifndef _CSNONE             
            setCSPin(SPI_CS_ACTIVE);
#endif            
        }
        
        int c=0;
        while(disp_sequence_data_count > 0 && c<SPI_HW_BUFFER_SIZE)
        {
            c++; 
            disp_sequence_data_count--;                  //pocet --
            //odelila command, max SPI_HW_BUFFER_SIZE bytes
            SPI1BUF=*disp_sequence_pos;
        
            disp_sequence_pos++;                        //dalsi znak
        }
        
        //povolit TX buffer empty interrupt
        IEC0SET=0x00200000; 
        return; 
        
    }
    else
    {
        //po odvisilani posledni casti nevysila nic, pouze nastavi disp_sequence_status
        spi_status = 0;
        
        //zakazat TX buffer empty interrupt
        IEC0CLR=0x00200000; 
#ifndef _CSNONE         
        setCSPin(SPI_CS_DEACTIVE);
#endif        
    }

}

void resetDisplay()
{
    //CS up
#ifdef _CS0_pin    
    aktualDispIndex=0;
    setCSPin(SPI_CS_DEACTIVE);
#endif      
    
#ifdef _CS1_pin    
    aktualDispIndex=1;
    setCSPin(SPI_CS_DEACTIVE);
#endif    
    
#ifdef _CS2_pin    
    aktualDispIndex=2;
    setCSPin(SPI_CS_DEACTIVE);
#endif       
    
#ifdef _CS3_pin    
    aktualDispIndex=3;
    setCSPin(SPI_CS_DEACTIVE);
#endif       

    //provede UP, DOWN (reset), UP
    //je-li zapojeno vice displeju, maji spolecny RESET (i D/C)
    
    //UP
    setResetPin(1);
    doEvents();
    //DOWN - probiha reset displeje
    setResetPin(0);
    doEvents();
    doEvents();
    doEvents();
    //UP
    setResetPin(1);
    doEvents();
}

void initDisplay()
{
    //inicializacni sequence
    //AE, D5 - 80, A8 - 3F, D3-00, 00, 8D-14, 20-00, DA-12, 81-CF, D9-F1, DB-40, A4, A6, AF 
    //vynechano A1-C8
    
    //inicializuje za zapne (ON) vsechny definovane displeje (_CSNONE, _CS0_pin, _CS1_pin, ...)
    
#ifdef _CSNONE
    
    aktualDispIndex=0;
    //off
    writeOneCommand(1, 0xAE, 0, 0, 0);
    
    //clock, multiplex ratio
    writeOneCommand(4, 0xD5, 0x80, 0xA8, 0x3F);

    //offset 
    writeOneCommand(3, 0xD3, 0x00, 0x00, 0x00);
    
    //charge pump
    writeOneCommand(4, 0x8D, 0x14, 0x20, 0x00);
    
    //set remap, hw config
    writeOneCommand(4, 0xA1, 0xC8, 0xDA, 0x12);
    
    //contrast, pre-charge 
    writeOneCommand(4, 0x81, 0xCF, 0xD9, 0xF1);
    
    //deselect level 
    writeOneCommand(2, 0xDB, 0x40, 0, 0);
    
    //pixels off, not inverted, on
    writeOneCommand(3, 0xA4, 0xA6, 0xAF, 0);
    
#endif    
    
#ifdef _CS0_pin
    
    aktualDispIndex=0;
    //off
    writeOneCommand(1, 0xAE, 0, 0, 0);
    
    //clock, multiplex ratio
    writeOneCommand(4, 0xD5, 0x80, 0xA8, 0x3F);

    //offset 
    writeOneCommand(3, 0xD3, 0x00, 0x00, 0x00);
    
    //charge pump
    writeOneCommand(4, 0x8D, 0x14, 0x20, 0x00);
    
    //set remap, hw config
    writeOneCommand(4, 0xA1, 0xC8, 0xDA, 0x12);
    
    //contrast, pre-charge 
    writeOneCommand(4, 0x81, 0xCF, 0xD9, 0xF1);
    
    //deselect level 
    writeOneCommand(2, 0xDB, 0x40, 0, 0);
    
    //pixels off, not inverted, on
    writeOneCommand(3, 0xA4, 0xA6, 0xAF, 0);

#endif    
    
#ifdef _CS1_pin

    aktualDispIndex=1;
    //off
    writeOneCommand(1, 0xAE, 0, 0, 0);
    
    //clock, multiplex ratio
    writeOneCommand(4, 0xD5, 0x80, 0xA8, 0x3F);

    //offset 
    writeOneCommand(3, 0xD3, 0x00, 0x00, 0x00);
    
    //charge pump
    writeOneCommand(4, 0x8D, 0x14, 0x20, 0x00);
    
    //set remap, hw config
    writeOneCommand(4, 0xA1, 0xC8, 0xDA, 0x12);
    
    //contrast, pre-charge 
    writeOneCommand(4, 0x81, 0xCF, 0xD9, 0xF1);
    
    //deselect level 
    writeOneCommand(2, 0xDB, 0x40, 0, 0);
    
    //pixels off, not inverted, on
    writeOneCommand(3, 0xA4, 0xA6, 0xAF, 0);

#endif      
  
#ifdef _CS2_pin
    
    aktualDispIndex=2;
    //off
    writeOneCommand(1, 0xAE, 0, 0, 0);
    
    //clock, multiplex ratio
    writeOneCommand(4, 0xD5, 0x80, 0xA8, 0x3F);

    //offset 
    writeOneCommand(3, 0xD3, 0x00, 0x00, 0x00);
    
    //charge pump
    writeOneCommand(4, 0x8D, 0x14, 0x20, 0x00);
    
    //set remap, hw config
    writeOneCommand(4, 0xA1, 0xC8, 0xDA, 0x12);
    
    //contrast, pre-charge 
    writeOneCommand(4, 0x81, 0xCF, 0xD9, 0xF1);
    
    //deselect level 
    writeOneCommand(2, 0xDB, 0x40, 0, 0);
    
    //pixels off, not inverted, on
    writeOneCommand(3, 0xA4, 0xA6, 0xAF, 0);

#endif      
   
#ifdef _CS3_pin

    aktualDispIndex=3;
    //off
    writeOneCommand(1, 0xAE, 0, 0, 0);
    
    //clock, multiplex ratio
    writeOneCommand(4, 0xD5, 0x80, 0xA8, 0x3F);

    //offset 
    writeOneCommand(3, 0xD3, 0x00, 0x00, 0x00);
    
    //charge pump
    writeOneCommand(4, 0x8D, 0x14, 0x20, 0x00);
    
    //set remap, hw config
    writeOneCommand(4, 0xA1, 0xC8, 0xDA, 0x12);
    
    //contrast, pre-charge 
    writeOneCommand(4, 0x81, 0xCF, 0xD9, 0xF1);
    
    //deselect level 
    writeOneCommand(2, 0xDB, 0x40, 0, 0);
    
    //pixels off, not inverted, on
    writeOneCommand(3, 0xA4, 0xA6, 0xAF, 0);

#endif  
    
}

//ovladani pinu DC, CS, RESET
#ifdef PIC32MM

void setDCPin(int value)
{
    //RB6 (pin 15)
    //_DC_pin=value;
    LATBbits.LATB6=value;
}

void setCSPin(int value)
{
    //nastav CS pin na value
    
    if(aktualDispIndex==0)
    {
#ifdef _CS0_pin
        _CS0_pin=value;
#endif           
    }
    else if(aktualDispIndex==1)
    {
#ifdef _CS1_pin        
        _CS1_pin=value;
#endif         
    }
    else if(aktualDispIndex==2)
    {
#ifdef _CS2_pin        
        _CS2_pin=value;
#endif         
    }
    else if(aktualDispIndex==3)
    {
#ifdef _CS3_pin        
        _CS3_pin=value;
#endif         
    }    

    //LATBbits.LATB3=value;
}

void setResetPin(int value)
{
    //RB7 (pin 16)
    //_RESET_pin=value;
     LATBbits.LATB7=value;
}

#endif

#ifdef PIC32MZ
//nedela nic

void setDCPin(int value)
{

}

void setCSPin(int value)
{
    
}

void setResetPin(int value)
{
    
}

#endif

//interupt handler
#ifdef PIC32MM

void SPI1TxHandler()
{
    //vola se pri TxBuffer empty
    writeSPISequenceInt();
    
    // clear any existing event IFS0.20=SPI0_Error, IFS0.21=SPI1.Tx, IFS0.22=SPI1.Rx
    // clear event IFS0.21=SPI1.Tx
    IFS0CLR=0x00200000;
}

void SPI1RxHandler()
{
    //vola se pri RxBuffer full
    //cteni Rx bufferu (vysilani provadi zaroven cteni)
    int data;
    
    //enhanced mode=0
    //data=SPI1BUF;
    
    //enhanced mode=1
    while(SPI1STATbits.SPIRBE == 0)
    {
        //cteni, dokud neni buffer prazdny
        data=SPI1BUF;
    }
    
    // clear event IFS0.22=SPI1.Rx
    IFS0CLR=0x00400000;
}

#endif