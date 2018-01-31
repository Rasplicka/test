#include <xc.h>
#include <stdio.h>
//#include <stdlib.h>
#include "asm.h"
#include "def.h"
#include "fnc.h"
#include "graphics.h"
#include "timer.h"

#ifdef PIC32MZ

#define     _LED_INV_REG    LATHINV
#define     _LED_INV_VAL    0b001

#endif

#ifdef PIC32MM0064

#define     _LED_INV_REG    LATBINV
#define     _LED_INV_VAL    0b100000

#endif  

#ifdef PIC32MM0256

#define     _LED_INV_REG    LATBINV
#define     _LED_INV_VAL    0b100000          //RB5                

#endif


int power=1;

void m1_start();
void floatToString(float f, char* ret, int dec);
void floatToStringFormat(float f, char* ret, int len, int dec);
float rndFloat(float f, int dec);
float rnd4(float f);
void write_ad();

void draw();
void on_button(int, int, int);
void write_button(int, int, int);
void write_power();

//#define     STACK_SIZE       1024
//int m1_stackSize=STACK_SIZE;
//char  m1_stackBase[STACK_SIZE];

//char* m1_stack=stack+STACK_SIZE-4;

inline void setSP();

//extern int drawString(char font, char x, char y, char* string);

char result[17];
char h1_string[8];
char h2_string[8];
char h3_string[8];
//stringCreator sc={result, 17, NULL, NULL, NULL, NULL};
int time_s=0;
int time_m=0;
char interval_ad=0;
char interval_bar=0;
int prew_point=-1;

extern int usb_value;

static void blick1();
void drawGraph();

void m1_start()
{
    blick1();
    
    
    //usb_mm_Init();
    
    //drawText(0, "VELKA PISMENA", 0, 1, FONT1306_MID);
    //blick();
    
    //char num[8];
    //char volt[]="Voltage is \0";
    
    
    /*
    //char test1[]={"test 1\0"};
    printString(0, "test 1\0");
    
    //char test2[]={"test 1\0"};
    printString(0, "test 1\0");
    
    //char test3[]={"test 3\0"};
    printString(0, "test 3\0");
    
    //char test4[]={"dlouhy test 4.15\0"};
    printString(0, "dlouhy test 4.15\0");
    
    //char test5[]={"Display 1306\0"};
    printString(0, "Display 1306\0");
    
    
    //char test6[]={"A/D conv. v.1\0"};
    printString(0, "A/D conv. v.1\0");    
    
    //char test7[]={"OS-MM v.4\0"};
    printString(0, "OS-MM v.4\0");  
      
    displayClear(0);
    */
    
    float f=1.1111;
    floatToString(f, result, 8);
    
             
    int x=0xFF;
    byteToChar(x, result, 1);
    
    x=1;
    byteToChar(x, result, 3);
    
    x=158;
    byteToChar(x, result, 1);
    
    
    uintToChar(x, result, 1);
    
    ubtn_regEvent(&on_button);
    
    //char* img=displayGetImageData(0, 5);
    //drawImage(0, img, 0, 0);                        //BAT. icon
    
    //disp1306_drawHBar8(0, 20, 40, 0, 16);
    
    //drawText(0, "VELKA PISMENA", 0, 1, FONT1306_MID);
    
    //drawText(0, "Tady je nejaky", 0, 2, FONT1306_MID);
    
    //drawText(0, "Temp.", 0, 3, FONT1306_MID);
    //disp1306_drawHBar16(0, 20, 40, 48, 12);
    
    
    //drawText(0, "I  I  I  I  I  I", 0, 1, FONT1306_MID);
    //drawText(0, "0  1  2  3  4  5", 0, 2, FONT1306_MID);
    //drawGraph();
    //printString(0, "Jedna");
    //printString(0, "Dva");
    //printString(0, "Tri");
    //drawText(0, "11111", 0, 0, FONT1306_MID);
    //drawText(0, "22222", 0, 1, FONT1306_MID);
    //drawText(0, "33333", 0, 3, FONT1306_MID);
    
    timer1_regEventInterval(&interval_ad, 500);
    timer1_regEventInterval(&interval_bar, 100);
    
    //pwm_on(1, 1);
    //pwm_on(2, 1);
    
    drawGraph();
    
    int c=5, b=0;
    int s=0;
    while(1)
    {
        //do LATxINV zapise 1 na prislusnou pozici
        _LED_INV_REG = _LED_INV_VAL;
        
        int a;
        for(a=1; a<500000; a++)
        {
            if(a % 500 == 0) { doEvents(); } 
           
            if(interval_ad>0)
            {
                write_ad();
                interval_ad=0;
            }
            
            if(interval_bar>0)
            {
                //s++;
                //if(s>20){s=0;}
                //disp1306_drawHBar8(0, 20, 40, 0, s);
                //interval_bar=0;
                
                //int pp=pwm_getProc(1);
                //if(pp!=power)
                //{
                //    power=pp;
                //    write_power();
                //}
            }            
        }
        //b++;
        //s++;
        
        //if(s==6){s=0;}
        //disp1306_drawBattery(0, 46, 8, s);
        
        //if(s>20){s=0;}
        //disp1306_drawHBar8(0, 20, 40, 0, s);
        //disp1306_drawHBar16(0, 20, 40, 48, s);
        
        
        //time_s++;
        //if(time_s==60) { time_s=0; time_m++; }
        //if(time_m==60){time_m=0;}
        //draw();
        
        //if()
        //displaySetContrast(0, b);
        
        //write_ad();
        
        //sprintf(num, "%d", b);
        //createString(result, 17, "Vyltaje is ", num, NULL);
        //printString(0, result); 
        
        //createStringStruct(sc);
        //printString(0, sc.result); 
        
        /*
        //sprintf(num, "%d", c);
        floatToString((float)c*0.1, num, 1);
        drawString(FONT1306_MID, 0, 3, num);
        doEvents();
        

        sprintf(num, "%d", c);
        createString(res, 17, volt, num, unit);
        drawString(FONT1306_MID, 0, 2, res);
        */
        
        //c+=11;

    }
}

void drawGraph()
{
   
    /*
   //stupnice 0 ... 40 
   drawText(0, "0             40", 0, 0, FONT1306_MID);         //zadava row a col
   drawString(0, "10", 24, 0, FONT1306_MID);       //zadava se x a y
   drawString(0, "20", 54, 0, FONT1306_MID);       //zadava se x a y
   drawString(0, "30", 84, 0, FONT1306_MID);       //zadava se x a y
   disp1306_drawScale8(0, 0, 16); 
   
   */
   //stupnice min ... 0 ... max 
    /*
   drawText(0, "-      0       +", 0, 0, FONT1306_MID);         //zadava row a col
   char* img=displayGetImageData(0, 21);
   drawImage(0, img, 3, 16); 
   */
}

void write_ad()
{
    /*
    int v1=usb_value; //adcscan_getValue(6);
    int v2=adcscan_getValue(28);
    //int v3=adcscan_getValue(4);
    
    char result[12];
    char num[12];
    
    char* img;
    int y=24;
    //drawImage(0, img, 0, 0);                        
    
    int x=(int)((float)v1/4096.0 * (float)(128-7));
    
    if(x != prew_point)
    {
        if(prew_point > -1)
        {
            img=displayGetImageData(0, 20);
            drawImage(0, img, prew_point, y);   
        }
        
        //if(x>prew_point){x+=(x-prew_point)/4;}
        //else {x-=(prew_point-x)/4;}
        
        img=displayGetImageData(0, 19);
        drawImage(0, img, x, y);      
        prew_point=x;
    }
    
    

    
    
    intToChar(x, num, 1);
    //formatRight(result, 5, num);
    drawText(0, num, 0, 3, FONT1306_MID);
    
    intToChar(v1, num, 1);
    //formatRight(result, 5, num);
    drawText(0, num, 8, 3, FONT1306_MID);
    */
    
    
    
    //intToChar(v2, num, 1);
    //formatRight(result, 5, num);
    //drawText(0, result, 0, 2, FONT1306_MID);
    
    //double max_graph=30;
    //int val=(int) ((double)v1 / 4096.0 * max_graph);
    //disp1306_drawHBar8(0, 30, 0, 0, val);
    
    //intToChar(v3, num, 1);
    //formatRight(result, 5, num);
    //drawText(0, result, 0, 3, FONT1306_MID);    
}

void on_button(int event, int button_value, int cnt)
{
    //write_button(event, button_value, cnt);
    
    if(event==UBTN_DOWN | event==UBTN_REPEAT)
    {
        /*
        if(button_value==32)
        {
            //power = power << 1;//  (power*power) ;
            //power=100;
            //pwm_setPowerTarget(1, power, 95, 0);
            //pwm_setPowerTarget(1, 0, power, 5, 50);
            pwm_setPowerExp(1, 37, 3);
            pwm_setPowerLinearProc(2, 100, 1, 5);
            
        }
        else if(button_value==33)
        {
            //power = power >> 1; //(power*power) ;
            //power=0;
            //pwm_setPowerTarget(1, power, 95, 0);
            //pwm_setPowerTarget(1, 0, power, 8, 50);
            pwm_setPowerExp(1, 0, 1);
            pwm_setPowerLinearProc(2, 0, 1, 5);
        }
        */
    
        //pwm_setPower(1, power);
    
        //char c[12];
        //intToChar(power, c, 1);
        //formatRight(c, 5, c);
        //drawText(0, c, 10, 3, FONT1306_MID);
    }
}

void write_power()
{
        char c[12];
        intToChar(power, c, 1);
        formatRight(c, 5, c);
        concat(c,"%", 12);
        //drawText(0, c, 10, 3, FONT1306_MID);    
}

void write_button(int event, int button_value, int cnt)
{

    /*
    char* s;
    int l=0;
    if(event==UBTN_DOWN)          {s="DOWN            ";}
    else if(event==UBTN_UP)       {s="UP              ";}
    else if(event==UBTN_DOWN_LONG){s="DOWN_LONG       ";}
    else if(event==UBTN_UP_LONG)  {s="UP_LONG         ";}
    else if(event==UBTN_REPEAT)   {s="REPEAT          ";}    

    
    char c[12];

   
    intToChar(cnt, c, 1);

    //sprintf(c, "%d", cnt);
    
    if(button_value==32)
    {
        drawText(0, s, 0, 1, FONT1306_MID);
        drawText(0, c, 12, 1, FONT1306_MID);
    }
    
    if(button_value==33)
    {
        drawText(0, s, 0, 2, FONT1306_MID);
        drawText(0, c, 12, 2, FONT1306_MID);
    }
    */

}

void draw()
{
    //date, time
    /*
    int date_d=21;
    int date_m=7;
    int date_y=17;
    
    int time_h=15;
    //int time_m=32;
    
    sprintf(h1_string, "%02d", date_d);
    sprintf(h2_string, "%02d", date_m);
    sprintf(h3_string, "%4d", 2000 + date_y);
    
    createString(result, 17, h1_string, "/", h2_string, "/", h3_string, NULL);
    drawString(0, result, 0, 0, FONT1306_MID);
    
    
    sprintf(h1_string, "%02d", time_h);
    sprintf(h2_string, "%02d", time_m);
    sprintf(h3_string, "%02d", time_s);
    
    createString(result, 17, h1_string, ".", h2_string, ".", h3_string, NULL); // ":", h3_string, NULL);
    drawString(0, result, 0, 1, FONT1306_BIG);
    //drawString(0, h3_string, 14, 3, FONT1306_MID);
    
    */
}

static void blick1()
{
    while(1)
    {
        //do LATxINV zapise 1 na prislusnou pozici
        _LED_INV_REG = _LED_INV_VAL;
        
        int a, b=0;
        for(a=0; a<190000; a++)
        {
            b++;
            if(a % 1000 == 0)
            {
                doEvents();
            }
        }
    }    
}





