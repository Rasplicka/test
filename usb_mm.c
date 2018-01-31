#include <xc.h>
#include "def.h"
#include "usb_mm.h"


//http://www.keil.com/forum/12679/

/*
Cteni dat z PIC
1.PC odesle OUT TOKEN s informaci, jaka data pozaduje
2.PIC pripravi data v IN bufferu a vola PrepareTransmitData(len)
  ktera pripravi USB modul na IN TOKEN (preda rizeni SIO)
3.PC odesle IN TOKEN a SIO automaticky odesila obsah buferu
 *
 * SIO stridave odesila EVEN a ODD buffer (adresa v BD EVEN a BD ODD)
 * ale EVEN i ODD pouzivaji jeden buffer
 * takze data sw uklada vzdy do jednoho bufferu
 * PrepareTransmitData(len) pripravi k odeslani EVEN i ODD
 *
 * priklad: (odeslat 3 bytes)
 * FillInBuffer(0, x);
 * FillInBuffer(1, y);
 * FillInBuffer(2, z);
 * PrepareTransmitData(3);
 * //nasledujici IN TOKEN odesle tyto 3 bytes
 *
 *  priklad 2 (odeslat 4 bytes)
 * PrepareTransmitData4(a, b, c, d)
 * //nasledujici IN TOKEN odesle tyto 4 bytes
 *
 * Max. delka (velikost bufferu) 64 bytes
*/

typedef struct
{
    char data;
    char :8;
    short size;
    int ptr;
}BD_ITEM;

typedef struct
{
    struct
    {
        char Recipient:5;
        char Type:2;
        char Direction:1;
    }bmRequestType;  
    
    char bRequest;
    char bValueL;
    char bValueH;
    short wIndex;
    short wLenght;
    
}SETUP_TOKEN;


//max. pocet EP je 15, resp. max. velikost conf. descriptoru je 128B
//conf descriptor obsahuje conf. desc. (9B) + iface (9B) + EP descriptory (7B), celkem max. 128B
//EP jsou defaultne nastaveny v modu interrupt/100ms


#define     EP_COUNT        4       //pocet EP, bez EP0_OUT a EP0_IN (ty jsou povinne)
#define     EP1_OUT         64      //definuje pouzity EndPoint a velikost bufferu
#define     EP1_IN          64      //definuje pouzity EndPoint a velikost bufferu
#define     EP2_OUT         64      //definuje pouzity EndPoint a velikost bufferu
#define     EP2_IN          64      //definuje pouzity EndPoint a velikost bufferu
//#define     EP3_OUT         64      //definuje pouzity EndPoint a velikost bufferu
//#define     EP3_IN          64      //definuje pouzity EndPoint a velikost bufferu
//#define     EP4_OUT         64      //definuje pouzity EndPoint a velikost bufferu
//#define     EP4_IN          64      //definuje pouzity EndPoint a velikost bufferu

#define     USBBUS_CURRENT  20      //spotreba na VBus +5V [mA]

// <editor-fold defaultstate="collapsed" desc="EP0 a BD Table">
#define     DESC_SIZE       8       //velikost descriptoru (kazdy EP ma 4 descriptory)

//musi byt na adrese delitelne 512 (b0-b8 = 0)
char bd_table[DESC_SIZE * 4 * (EP_COUNT + 2)] __attribute__((aligned(512)));
char bufferOut0[8];                 //setup paket ma 8 bytes
//IN data (descriptory) jsou ve flash

SETUP_TOKEN* setup_token=(SETUP_TOKEN*)bufferOut0;      //prekryva OUT buffer EP0, pro snazsi pristup k datum
BD_ITEM* bd_ep0_in_even;                                //prekryva BD, pro snazsi pristup k datum
//BD_ITEM* bd_ep0_in_odd;             //prekryva BD, pro snazsi pristup k datum

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="USB consts">
#define     EP_MODE_ISO             1
#define     EP_MODE_BULK            2
#define     EP_MODE_INT             3

#define     DIRECTION_OUT           0
#define     DIRECTION_IN            1
#define     PPBI_EVEN               0
#define     PPBI_ODD                1
#define     OUT_TOKEN               1
#define     IN_TOKEN                9
#define     SETUP_TOKEN             13

//Standard Request
#define     GET_STATUS              0x00
#define     CLEAR_FEATURE           0x01
#define     SET_FEATURE             0x03
#define     SET_ADDRESS             0x05
#define     GET_DESCRIPTOR          0x06
#define     SET_DESCRIPTOR          0x07
#define     GET_CONFIGURATION       0x08
#define     SET_CONFIGURATION       0x09
#define     GET_INTERFACE           0x0A
#define     SET_INTERFACE           0x0B
#define     SYNCH_FRAME             0x0C
#define     SET_IDLE                0x0A
#define     DTYPE_DEVICE            0x01
#define     DTYPE_CONFIGURATION     0x02
#define     DTYPE_STRING            0x03
#define     DTYPE_QUALIFIER         0x06
#define     DTYPE_OTHER             0x07
#define     DTYPE_REPORT            0x22
//Recipient
#define     DEVICE                  0x0
#define     INTERFACE               0x1
#define     ENDPOINT                0x2
#define     OTHER                   0x3

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="vars">
char address    = 0;
char isize      = 0;
int ibuff       = 0;
char is_connected= 0;
char datain=0;
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="local fce">
static void USB_reset();
static char* USB_getBDAddress(char ep, char dir, char ppbi);
static char* USB_getBuffer(char ep, char dir, char ppbi);
static void USB_setBD(char* bd, char b0, int size, char* buffer_addr);
static void USB_prepareBDForOut(char ep, char ppbi, char toggle, short int len);
static void USB_sendDeviceDescriptor(short int len);
static void USB_sendDeviceConfiguration(unsigned int len);
static void USB_sendDeviceStatus();
static void USB_sendSetupNull();
static void USB_sendString(const unsigned char index);

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="MACRO">
#define     VA_TO_PA(a)         (int)a & 0x7FFFFFFF     //nuluje b31, plati pro RAM i FLASH
//#define     FLASHVA_TO_PA(a)    (int)a & 0x00FFFFFF | 0x1D000000   

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Descriptor, String">
const char cd[] ={
    //configuration desc. + iface + ep descriptor
    9,                      //delka conf.
    0x02,                   //Conf. Descriptor
    18 + (EP_COUNT * 7),    //total size L (+ iface, ep...)
    0,                      //total size H
    0x01,                   //iface count
    0x01,                   //number conf.
    0x00,                   //string index
    0b10000000,             //attrib. self power, no wake-up 1110 0000
    USBBUS_CURRENT / 2,     //spending mA (1 unit = 2mA) 200 units = 100mA
                            
    //iface
    0x09,                   //iface lenght
    0x04,                   //iface descriptor
    0x00,                   //index iface
    0x00,                   //index alt.
    EP_COUNT,
    0x00,                   //class code HID
    0x00,                   //subclass code
    0x00,                   //protocol 02
    0x00,                   //string index

    //EP1 IN
    0x07,                   //lenght
    0x05,                   //EP descriptor
    0b10000001,             //EP1 - in
    EP_MODE_BULK,            //interrupt ...11, izo ...01
    EP1_IN,                 //velikost bufferu L
    0x00,                   //velikost bufferu H
    100,                    //interrupt interval ms, izo = 1

    //EP1 OUT
    0x07,                   //lenght
    0x05,                   //EP descriptor
    0b00000001,             //EP1 - out
    EP_MODE_BULK,            //interrupt ...11, izo ...01
    EP1_OUT,                //velikost bufferu L
    0x00,                   //velikost bufferu H
    100,                    //interrupt interval ms, izo = 1

#ifdef EP2_IN    
    //EP2 IN
    0x07,                   //lenght
    0x05,                   //EP descriptor
    0b10000010,             //EP2 - in
    EP_MODE_INT,            //interrupt ...11, izo ...01
    EP2_IN,                 //velikost bufferu L
    0x00,                   //velikost bufferu H
    100,                    //interrupt interval ms, izo = 1
#endif    

#ifdef EP2_OUT     
    //EP2 OUT
    0x07,                   //lenght
    0x05,                   //EP descriptor
    0b00000010,             //EP2 - out
    EP_MODE_INT,            //interrupt ...11, izo ...01
    EP2_OUT,                //velikost bufferu L
    0x00,                   //velikost bufferu H
    100,                    //interrupt interval ms, izo = 1
#endif     

#ifdef EP3_IN     
    //EP3 IN
    0x07,                   //lenght
    0x05,                   //EP descriptor
    0b10000011,             //EP3 - in
    EP_MODE_INT,            //interrupt ...11, izo ...01
    EP3_IN,                 //velikost bufferu L
    0x00,                   //velikost bufferu H
    100,                    //interrupt interval ms, izo = 1
#endif     

#ifdef EP3_OUT     
    //EP3 OUT
    0x07,                   //lenght
    0x05,                   //EP descriptor
    0b00000011,             //EP3 - out
    EP_MODE_INT,            //interrupt ...11, izo ...01
    EP3_OUT,                //velikost bufferu L
    0x00,                   //velikost bufferu H
    100,                    //interrupt interval ms, izo = 1
#endif     
    
#ifdef EP4_IN     
    //EP4 IN
    0x07,                   //lenght
    0x05,                   //EP descriptor
    0b10000100,             //EP4 - in
    EP_MODE_INT,            //interrupt ...11, izo ...01
    EP4_IN,                 //velikost bufferu L
    0x00,                   //velikost bufferu H
    100,                    //interrupt interval ms, izo = 1
#endif     

#ifdef EP4_OUT     
    //EP4 OUT
    0x07,                   //lenght
    0x05,                   //EP descriptor
    0b00000100,             //EP4 - out
    EP_MODE_INT,            //interrupt ...11, izo ...01
    EP4_OUT,                //velikost bufferu L
    0x00,                   //velikost bufferu H
    100                     //interrupt interval ms, izo = 1    
#endif 
            
};

const char dd[] ={
    //device descriptor
    18,             //delka
    0x01,           //Device Descriptor
    0x00,           //verze 2.0 - full speed
    0x02,           //verze 2.0 - full speed
    0x00,           //class
    0x00,           //sub class
    0x00,           //protocol
    64,             //buffer size

    0x05,           //vendor id
    0x00,           //vendor id
    0x07,           //product id 05
    0x00,           //product id
    0x40,           //device release 04
    0x07,           //device release 03
    0x01,           //Manufact. string index
    0x02,           //Product string index
    0x00,           //serial Num. string index
    0x01            //Number of configurations
};

const short sd = 0x0000;            //b0=1-self powered, b1=1-remote wakeup                            
                                    //rozhodujici je hodnota Maximum Power Consumption v conf. descriptoru

const char strManufacturer[] = {
    16, DTYPE_STRING, //celkova delka, string
    'J', 0,
    'R', 0,
    ' ', 0,
    'S', 0,
    'o', 0,
    'f', 0,
    't', 0
};

const char strProduct[] = {
    16, DTYPE_STRING, //celkova delka, string
    'U', 0,
    'S', 0,
    'B', 0,
    ' ', 0,
    'P', 0,
    'I', 0,
    'C', 0
};

const char strZero[] = {
    5, DTYPE_STRING,
    0x9, 0x4, 0x0
}; 

// </editor-fold>

/*
//odeslani bufferu 
char buffer[64];
char ep=1;
while(USB_isTxProgress(ep)){ doEvents(); }
USB_txData(ep, buffer, 64);
 * 
  
 //reakce na pozadavek z hosta
 char buffer_cmd[4];                    //prijem command z hosta (command size = 4B)
 char data_out[64]                      //data pro hosta
 char ep=1;
 USB_rxData(ep, buffer_cmd, 4)          //ceka na data z hosta
 while(USB_isRxProgress(ep)) { doEvents(); }
 
 //dekoduje command 
 if(buffer_cmd[0]==1) 
 {
    fillBufferA(data_out);              //command==1, napln buffer daty A
 }
 else
 {
    fillBufferB(data_out);              //command!=1, napln buffer daty B
 } 
 transmitBuffer(data_out);              //osedlani dat
  
*/

//global fce
int USB_isConnected()
{
    if(U1ADDR==0 || U1OTGSTATbits.VBUSVD==0 || is_connected==0)
    {
        //nebyla pridelena adresa, nebo VBUS=0, neni pripojeno USB
        return 0;
    }
    else
    {
        //je pripojeno USB (probehla enumerace)
        return 1;
    }
}

void USB_txData(char ep, char* buffer, short int len)
{
    //odeslani dat (IN token, host cte data)
    //data jsou v bufferu, ktery alokuje volajici fce (muze mit vice bufferu)
    //ppbi vypnuto, pouzije EVEN BD

    //char* bd=USB_getBDAddress(ep, DIRECTION_IN, PPBI_EVEN);
    //U1CONbits.PPBRST=1;
    BD_ITEM* p=(BD_ITEM*)USB_getBDAddress(ep, DIRECTION_IN, PPBI_EVEN);
    p->size=len;
    p->ptr=VA_TO_PA(buffer);  //(int)buffer & 0x0FFFFFFF;
    p->data=0x80 | datain << 6;
    datain++;
    
    /*
    short int* bd16=(short int*)bd;
    bd16[1]=len;
    
    //adresa bufferu (physical addr)
    int* bd32=(int*)bd;
    bd32[1]=(int)buffer & 0x0FFFFFFF;    
    
    //b7=1 (UOWN=1 - SIE vlastni buffer), b6=1 DATA0/1 
    bd[0]=0x90;
    */
}

int USB_isTxProgress(char ep)
{
    //vraci 1, pokud probiha vysilani (jeste nebyl odvysilany paket)
    //bd[0].b7=1 SIE pracuje, 0=dokonceno CPU vlastni buffer
    
    BD_ITEM* p=(BD_ITEM*)USB_getBDAddress(ep, DIRECTION_IN, PPBI_EVEN);
    return p->data>>7;
    
    //char* bd=USB_getBDAddress(ep, DIRECTION_IN, PPBI_EVEN);
    //return bd[0]>>7;
}

void USB_rxData(char ep, char* buffer, short int len)
{
    //prijem dat (OUT token, host posila data)
    //data jsou v bufferu, ktery alokuje volajici fce (muze mit vice bufferu)
    //ppbi vypnuto, pouzije EVEN BD
    
    //U1CONbits.PPBRST=1;
    BD_ITEM* p=(BD_ITEM*)USB_getBDAddress(ep, DIRECTION_OUT, PPBI_EVEN);
    p->size=len;
    p->ptr=VA_TO_PA(buffer);  //(int)buffer & 0x0FFFFFFF;
    p->data=0x80;
    
    //p=(BD_ITEM*)USB_getBDAddress(ep, DIRECTION_OUT, PPBI_ODD);
    //p->size=len;
    //p->ptr=VA_TO_PA(buffer);  //(int)buffer & 0x0FFFFFFF;
    //p->data=0x80;
    
    /*
    char* bd=USB_getBDAddress(ep, DIRECTION_OUT, PPBI_EVEN);    
    
    short int* bd16=(short int*)bd;
    bd16[1]=len;

    //adresa bufferu (physical addr)
    int* bd32=(int*)bd;
    bd32[1]=(int)buffer & 0x0FFFFFFF;  
    
    //b7=1 (UOWN=1 - SIE vlastni buffer), b6=0 DATA0/1 
    bd[0]=0x80;   
    */
}

int USB_isRxProgress(char ep)
{
    //vraci 1, pokud probiha cteni (jeste nebyl prijat paket)
    //bd[0].b7=1 SIE pracuje, 0=dokonceno CPU vlastni buffer
    
    BD_ITEM* p=(BD_ITEM*)USB_getBDAddress(ep, DIRECTION_OUT, PPBI_EVEN);
    return p->data>>7;
    
    //char* bd=USB_getBDAddress(ep, DIRECTION_OUT, PPBI_EVEN);
    //return bd[0]>>7;
}

void usb_mm_Init()
{
    U1PWRCbits.USBPWR=1;            //module ON
    
    //adresa DB Table
    //U1BDTP1=0x10;                  //BDT.8  - BDT.15
    //U1BDTP2=0x00;                 //BDT.16 - BDT.23
    //U1BDTP3=0x00;                 //BDT.24 - BDT.31
    int phy_adr=((int)&bd_table) & 0x0FFFFFFF;
    U1BDTP1 = phy_adr >> 8;
    U1BDTP2 = phy_adr >> 16;
    U1BDTP3 = phy_adr >> 24;    //Phys. adresa, nikoliv Virt.

    // <editor-fold defaultstate="collapsed" desc="U1EPx">
    U1EP0 = 0b00001101;             //povolit Tx, Rx, SETUP 

#ifdef EP1_OUT    
    U1EP1 = 0b00001101; //povolit Tx, Tx, zakazat SETUP
#endif
#ifdef EP1_IN
    U1EP1 = 0b00001101; //povolit Tx, Tx, zakazat SETUP
#endif    

#ifdef EP2_OUT    
    U1EP2 = 0b00011101; //povolit Tx, Tx, zakazat SETUP
#endif
#ifdef EP2_IN
    U1EP2 = 0b00011101; //povolit Tx, Tx, zakazat SETUP
#endif   

#ifdef EP3_OUT    
    U1EP3 = 0b00011101; //povolit Tx, Tx, zakazat SETUP
#endif
#ifdef EP3_IN
    U1EP3 = 0b00011101; //povolit Tx, Tx, zakazat SETUP
#endif  
    
    // </editor-fold>
     
    U1CNFG1=0x0;
    //U1SOF=0b01001010;
    
    U1IRbits.URSTIF=1;
    U1IRbits.TRNIF=1;
    U1IEbits.URSTIE=1;          //povolit preruseni RESET
    U1IEbits.TRNIE=1;           //povolit preruseni trn completed

    U1CONbits.PKTDIS=0;         //povolit prijem paketu
    
    U1CONbits.USBEN=1;
    while(U1CONbits.USBEN==0){U1CONbits.USBEN = 1;}

    U1CONbits.PPBRST=1;
    USB_reset();

    // <editor-fold defaultstate="collapsed" desc="interrupt">
    IPC7bits.USBIP = 2;
    IPC7bits.USBIS = 3;
    IFS0bits.USBIF = 0;
    IEC0bits.USBIE = 1; 
    // </editor-fold>
}



//local fce
void USB_reset()
{
    U1ADDR = 0;
    address=0;
    
    //IN BD EVEN, ODD
    bd_ep0_in_even=(BD_ITEM*)USB_getBDAddress(0, DIRECTION_IN, PPBI_EVEN);
    //bd_ep0_in_odd=(BD_ITEM*)USB_getBDAddress(0, DIRECTION_IN, PPBI_ODD);
    
    //OUT BD EVEN
    BD_ITEM* x =(BD_ITEM*)USB_getBDAddress(0, DIRECTION_OUT, PPBI_EVEN);
    x->ptr=VA_TO_PA(bufferOut0);
    
    //OUT BD ODD (pouziva pouze EVEN buffer)
    //x =(BD_ITEM*)USB_getBDAddress(0, DIRECTION_OUT, PPBI_ODD);
    //x->ptr=VA_TO_PA(bufferOut0);  
   
    //pripravit na data EP0
    //U1CONbits.PPBRST=1;
    USB_prepareBDForOut(0, 0, 0, 8);
}

char* USB_getBDAddress(char ep, char dir, char ppbi)
{
    //dir  1=OUT/SETUP, 0=IN
    //ppbi 0=data0/EVEN, 1=data1/ODD
    
    int x=(int)bd_table;
    return (char*)(x | ep<<5 | dir<<4 | ppbi<<3);
}

char* USB_getBuffer(char ep, char dir, char ppbi)
{
    //vraci fyzickou adresu bufferu z BufferDescriptoru
    //BD word[1]=adresa bufferu
    int* bd_item=(int*)USB_getBDAddress(ep, dir, ppbi);
    return (char*)bd_item[1];                  //prevod phys. na virt.
    
}

void USB_setBD(char* bd, char b0, int size, char* buffer_addr)
{
    //BD 8 bytes
    //B0=b0
    //B2-3 = size
    //B4-7 = buffer adresa
    
    bd[0]=b0;
    
    short int* bd16=(short int*)bd;
    bd16[1]=size;
    
    int* bd32=(int*)bd;
    bd32[1]=(int)buffer_addr & 0x0FFFFFFF;
}

void USB_prepareBDForOut(char ep, char ppbi, char data01, short int len)
{
    //pripravi BD item pro prijem dat
    //BDT item =
    //8 bitu data,
    //8 bitu nic,
    //8 bitu len,
    //8 bitu len(2bity),
    //32 bitu adresa bufferu

    //ppbi=0;
    
    /*
    char* bd=USB_getBDAddress(ep, DIRECTION_OUT, ppbi);  //DIRECTION_IN
    short int* bd16=(short int*)bd;
    bd16[1]=len;

    //b7=1 (UOWN=SIE), b6=toggle
    bd[0]=0x80 | (data01 << 6);
    */
    
    BD_ITEM* x =(BD_ITEM*)USB_getBDAddress(ep, DIRECTION_OUT, ppbi);
    x->size=len;
    x->data=0x80 | (data01 << 6);
    
}

void USB_sendDeviceDescriptor(short int len)
{
    if(len>18){len=18;}
    //U1CONbits.PPBRST=1;
    
    bd_ep0_in_even->ptr=VA_TO_PA(dd);
    bd_ep0_in_even->size=len;
    bd_ep0_in_even->data=0b11000000;
    
    /*
    char* bd0=USB_getBDAddress(0, DIRECTION_IN, 0);  //DIRECTION_IN
    short int* bd016=(short int*)bd0;
    int* bd032=(int*)bd0;
    
    int b0=(int)dd & 0x00FFFFFF | 0x1D000000;
    bd032[1]=b0; 
    
    U1CONbits.PPBRST=1;
    if(len>18){len=18;}
    
    bd016[1]= len;
    bd0[0]=0b11000000;
    */
    
    /*
    bufferIn0[0] = 18;      //delka
    bufferIn0[1] = 0x01;	//Device Descriptor
    bufferIn0[2] = 0x00;    //verze 2.0
    bufferIn0[3] = 0x02;	//verze 2.0
    bufferIn0[4] = 0x00;	//class
    bufferIn0[5] = 0x00;	//sub class
    bufferIn0[6] = 0x00;	//protocol
    bufferIn0[7] = 64;      //buffer size
    
    if(len > 8)    
    {
        bufferIn0[8] = 0x05;	//vendor id
        bufferIn0[9] = 0x00;	//vendor id
        bufferIn0[10] = 0x07;	//product id 05
        bufferIn0[11] = 0x00;   //product id
        bufferIn0[12] = 0x40;	//device release 04
        bufferIn0[13] = 0x07;	//device release 03
        bufferIn0[14] = 0x01;	//Manufact. string index
        bufferIn0[15] = 0x02;	//Product string index
        bufferIn0[16] = 0x00;	//serial Num. string index
        bufferIn0[17] = 0x01;	//Number of configurations
        len=18;
    }

    //oba BD ukazuji na bufferIn0
    U1CONbits.PPBRST=1;
    //USB_prepareBDForIn(0, 0, 1, len);   //EVEN
    USB_prepareBDForIn(0, 1, 1, len);   //ODD
    */

}

void USB_sendDeviceConfiguration(unsigned int len)
{
    //len = pozadovana velikost (odesila tento pocet, nebo mensi)
    //char slen=0;        //skutecna velikost
    
    if(len==0xFF){len=9;}
    
    //U1CONbits.PPBRST=1;                         //nastav EVEN BD
    bd_ep0_in_even->ptr=VA_TO_PA(cd);
    
    if(len>64)
    {
        bd_ep0_in_even->size=64;
        bd_ep0_in_even->data =  0b11000000;     //SIE, DATA1

        //priprava pro dalsi paket
        isize=len-64;
        ibuff=bd_ep0_in_even->ptr+64;
    }
    else
    {
        bd_ep0_in_even->size = len;
        bd_ep0_in_even->data =  0b11000000;     //SIE, DATA1
        isize=0;                                //neni dalsi paket
    }
}

void USB_sendDeviceStatus()
{
    bd_ep0_in_even->ptr=VA_TO_PA(sd);
    bd_ep0_in_even->size=2;
    bd_ep0_in_even->data=0b11000000;    
}

void USB_sendSetupNull()
{
    //odeslani NULL na EP0, EVEN, data1
    //U1CONbits.PPBRST=1;
    bd_ep0_in_even->size=0;
    bd_ep0_in_even->data=0b11000000;
}

void USB_sendString(const unsigned char index)
{
    //U1CONbits.PPBRST=1;
    
	if(index==0)
	{
		//USB_fillString((char*)&strZero);
        bd_ep0_in_even->ptr=VA_TO_PA(strZero);
        bd_ep0_in_even->size=strZero[0];
        bd_ep0_in_even->data=0b11000000;
	}
	else if (index==1)
	{
		//USB_fillString((char*)&strManufacturer);
        bd_ep0_in_even->ptr=VA_TO_PA(strManufacturer);
        bd_ep0_in_even->size=strManufacturer[0];
        bd_ep0_in_even->data=0b11000000;
        
	}
	else if(index==2)
	{
		//USB_fillString((char*)&strProduct);
        bd_ep0_in_even->ptr=VA_TO_PA(strProduct);
        bd_ep0_in_even->size=strProduct[0];
        bd_ep0_in_even->data=0b11000000;        
	}
}

void usb_mm_interrupt()
{
    //Po prijeti SETUP paketu nastavi SIE PKTDIS=1 - zakaz prijimani dalsich paketu
    U1CONbits.PKTDIS=0;
    
    if(U1IRbits.URSTIF==1)
    {
        //reset
        USB_reset();
        U1IRbits.URSTIF=1;
    }
    
    if(U1IRbits.TRNIF==1)
    {
        //transaction
        
        //EP
        char ep=U1STATbits.ENDPT;
        //PID IN, OUT, SETUP, BDitem.b2-b5
        BD_ITEM* x =(BD_ITEM*)USB_getBDAddress(ep, U1STATbits.DIR, U1STATbits.PPBI); 
        char pid = x->data >> 2 & 0b1111;
        
        U1IRbits.TRNIF=1;
        
        if(ep==0 && pid==SETUP_TOKEN)
        {
            
            //prijata data SETUP na EP0
            if(setup_token->bRequest == GET_DESCRIPTOR) //bRequest
            {
                if(setup_token->bmRequestType.Recipient == DEVICE && setup_token->bValueH == DTYPE_DEVICE) //bmRequestType.Recipient
                {
                    //pozaduje device descriptor
                    USB_sendDeviceDescriptor(setup_token->wLenght);
                }

                else if(setup_token->bmRequestType.Recipient == DEVICE && setup_token->bValueH == DTYPE_CONFIGURATION)
                {
                    //pozaduje device configuration
                    USB_sendDeviceConfiguration(setup_token->wLenght);
                }

                else if(setup_token->bmRequestType.Recipient == DEVICE && setup_token->bValueH == DTYPE_QUALIFIER)
                {
                    //pozaduje device qualifier (pro jinou rychlost)
                    USB_sendSetupNull();
                }

                else if(setup_token->bmRequestType.Recipient == INTERFACE && setup_token->bValueH == DTYPE_REPORT)
                {
                    //report
                    USB_sendSetupNull();
                }

                else if(setup_token->bmRequestType.Recipient == DEVICE && setup_token->bValueH == DTYPE_STRING)
                {
                    //string
                    USB_sendString(setup_token->bValueL);
                }
            }

            else if(setup_token->bRequest == SET_ADDRESS)
            {
                //set address, odeslat potvrzeni, null paket
                address=setup_token->bValueL; // bufferOut0[2];
                USB_sendSetupNull();
            }

            else if(setup_token->bRequest == SET_CONFIGURATION)
            {
                USB_sendSetupNull();
            }

            else if(setup_token->bRequest == GET_CONFIGURATION)
            {
                USB_sendSetupNull();
            }

            else if(setup_token->bRequest == SET_IDLE)
            {
                USB_sendSetupNull();
            }

            else if(setup_token->bRequest == GET_STATUS)
            {
                //GET_STATUS
                USB_sendDeviceStatus();
                is_connected=1;
            }
            
            else 
            {

            }

            //pripravit na dalsi data na EP0
            //U1CONbits.PPBRST=1;
            USB_prepareBDForOut(0, 0, 0, 8);

        } //END SETUP  
        
        //OUT TOKEN na EP0
        else if(ep==0 && pid==OUT_TOKEN)
        {
            //out data na EP0, napr. potvrzeni prijeti dat - (NULL) paket, pripravit na dalsi data na EP0
            //U1CONbits.PPBRST=1;
            USB_prepareBDForOut(0, 0, 0, 8);
        }

        //IN TOKEN na EP0
        else if(ep==0 && pid==IN_TOKEN)
        {
            if(isize>0)
            {
                //druhy paket predchoziho vysilani
                //U1CONbits.PPBRST=1;                             //nastav EVEN BD
                bd_ep0_in_even->ptr=ibuff;
                bd_ep0_in_even->size=isize;
                bd_ep0_in_even->data =  0b10000000;             //SIE, DATA0
                
                isize=0;                                        //neni dalsi paket
            }
            else
            {
                if(address!=0)
                {
                    //bylo odeslano potvrzeni (NULL) - prijeti adresy
                    //nyni zacne probihat kom. na pridelene adrese
                    U1ADDR = address;
                    address = 0;
                }
            }
        }
        
        else if(ep != 0)
        {
            int a=0;
        }
    }
    
    IFS0bits.USBIF = 0;         //USBIF
}

