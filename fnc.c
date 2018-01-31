#include <xc.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "fnc.h"
#include "asm.h"
#include "def.h"
#include "graphics.h"

void pauseEvent2(int ms)
{
    int a;
    ms/=10;
    
    for(a=0; a<ms; a++)
    {
        doEvents();
    }
}

int createString(char* buffer, int buffer_len, char* a, ...)
{
    //predpoklada, ze buffer je prazdny (presto na prvni pozici vlozi 0x0)
    //do buferu vlozi (spoji) zadane stringy a[0] - a[x]
    //kazdy string a[0]-a[x] musi byt zakonceny /0
    //pokud celkova delka presahne buffer_len, bude zkraceno (posledni znak bude \0)
    
    int ret=0x1;
    buffer[0]=0x0;                              //prazdny buffer
    va_list argList;                            //seznam argumentu
    va_start ( argList, a );                    //a
    
    /*
    for (;;) 
    {
        char* t = va_arg( argList, char* );
        ret &= concat(buffer, t, buffer_len);
        ret=1;
        
     //Do something with t 
    }
    */
    
    
    while(a != NULL)
    {
        ret &= concat(buffer, a, buffer_len);
        a=va_arg ( argList, char* );            //a=dalsi argument (...)
    }
    
    va_end(argList);
    
    return ret;
}

void createStringStruct(stringCreator sc)
{
    sc.result[0]=0x0;
    concat(sc.result, sc.s1, sc.len);
    if(sc.s2!=NULL){ concat(sc.result, sc.s2, sc.len); }
    if(sc.s3!=NULL){ concat(sc.result, sc.s3, sc.len); }
    if(sc.s4!=NULL){ concat(sc.result, sc.s4, sc.len); }    
}

int appendString(char* buffer, int buffer_len, char* a, ...)
{
    //buffer obsahuje data (nebo 0x0)
    //k bufferu pripoji stringy a[0]-a[x]
    //kazdy string a[0]-a[x] musi byt zakonceny /0
    //pokud celkova delka presahne buffer_len, bude zkraceno (posledni znak bude \0)    
    
    int ret=0x1;
    va_list argList;                            //seznam argumentu
    va_start ( argList, a );                    //a
    while(a)
    {
        ret &= concat(buffer, a, buffer_len);
        a=va_arg ( argList, char* );            //a=dalsi argument (...)
    }
    va_end(argList);
    
    return ret;
}

void alignLeft(char* str, int strlen)
{
    //vyradi leve mezery, prida prave mezery, aby pocet platnych znaku byl strlen
    //tzn. ze velikost char[] str musi byt o 1 vetsi
    //napr.: alignLeft(num, 8)
    //pred: num[9]="    1256\0"
    //po:   num[9]="1256    \0"
    lTrim(str, str);
    addSpace(str, strlen);
}

float roundFloat(float f, int dec)
{
    int a=dec;
    int n=1;
    while(a>0) { n*=10; a--; }
    
    if(f>=0) { f=f*n + 0.555555; }
    else { f=f*n - 0.555555; }
    int r=(int)f;
    f=(float)(r)/n;

    return f;
}

void floatToString(float f, char* ret, int dec)
{
    //prevede float na string (dec je max 1...9)
    ///ret musi byt pole dimenzovane min. na 16 znaku (15 + \0)
    
    f=roundFloat(f, dec);
    int n=1, a=dec;
    while(a>0) { n*=10; a--; }
    
    int whole=f;
    int decimal=(f-whole)*n; 
    if(decimal<0){decimal*=(-1);}
    
    char p1[12];
    char p2[12];
    intToChar(whole, p1, 1);
    intToChar(decimal, p2, dec);
    createString(ret, 16, p1, ".", p2, NULL);
    
    //char format[]="%1d.%01d";
    //char d=48+dec;                      //zmenit cislo na pozici 6
    //format[6]=d;
    
    //sprintf(ret, format, whole, decimal);
}

void floatToStringFormat(float f, char* ret, int w, int dec)
{
    //prevede float na string (w = 1...9, dec = 1...9)
    //w urcuje pocet mist pred des. teckou, je-li kratsi, bude pred cislem doplneno mezerami, je-li delsi, bude vysledek delsi (vypise vsechny cislice)
    //des je pocet des. mist, vzdy dodrzi pocet cislic, pridava 0 na konec 
    //vysledek ma vzdy stejny pocet znaku (pokud cislo neni vetsi)
    
    f=roundFloat(f, dec);
    int n=1, a=dec;
    while(a>0) { n*=10; a--; }
    
    int whole=f;
    int decimal=(f-whole)*n; 
    if(decimal<0){decimal*=(-1);}
    

    
    
    char format[]="%1d.%01d";
    char x=48+w;
    char y=48+dec;
    format[1]=x;
    format[6]=y;

    
    
    //sprintf(ret, format, whole, decimal);
}

void intToStringFormat(int i, char* ret, int w)
{
    //prevod int na string (w=1...9)
    //vraci pocet znaku = w, pred kratsi cislo vlozi mezery, je-li delsi, vysledek je delsi nez w
    
    char format[]="%1d";
    char x=48+w;
    format[1]=x;

    //sprintf(ret, format, i);    
}
