//obsahuje globalni definice pro C/C++
#include "def.h"

const struct
{
    char    _8bit;
    char    _16bit;
    char    _32bit;
}BUS_MODE={0, 1, 2};

const struct
{
    char    spi;
    char    pmp;
    char    i2c;
}PERIPH_TYPE={0, 1, 2};

const struct
{
    char    EMPTY;              //SPI je volne k pouziti
    char    USED;               //nejaka fce pouziva SPI, nikdo jiny nemuze
    char    FINISHED;           //neprobiha odesilani/prijem dat
    char    SENDING;            //probiha odesilani/prijem dat
}SPI_STATE={0, 1, 0, 1};

typedef struct 
{
    //void* PORT_INFO, char* buffer, short len
    void    (*writeBuffer)(void*, char* buffer, short len);                     
    
    //void* PORT_INFO, char* buffer, short len, char mode<br>
    //mode:0 data only<br>
    //mode:1 command, data (command call event fn. It can be use to change output signals, for example D/C)<br>
    void    (*writeBufferMode)(void*, char* buffer, short len, char mode);      
    
    //void* PORT_INFO, char* txBuffer, char* rxBuffer, int len<br>
    //used for SPI, transmit (txBuffer) and receive (rxBuffer) data<br>
    //SPI can WRITE only, or WRITE and READ, using this fn
    void    (*exchangeBuffer)(void*, char* txbuf, char* rxbuf, int len);
    
    //Gets port to use. Other process cannot use it<br>
    //When the port is in use, wait by using doEvents fn
    void    (*getPort)(void*);
    
    //The process free port 
    void    (*freePort)(void*);
    
    //Specify fn, that is called SPI, when mode is 1
    void    (*eventFn)(char);
    
    //Specify fn, that is called after data finished
    void    (*finishFn)(char);
    
    //Sets bus mode 8/16/32 bit
    void    (*setBusMode)(void*, char);
    
    //Adrress of buffer register in hardware module
    int*    directModeHwBuffer; 
    
    int     cs_portBase;            //Port base address
    int     cs_pin;                 //Define port bit
    int     reset_portBase;         //Port base address
    int     reset_pin;              //Define port bit
    int     dc_portBase;            //Port base address
    int     dc_pin;                 //Define port bit
    
    //Port index, 0=SPI1, 1=SPI2, ...
    char    portIndex;             
    
    //When directMode=1, driver write data directly to HW buffer register
    char    directMode;
    
    //Bus mode 8/16/32-bit, viz. struct BUS_MODE
    char    busMode;                //0=8-bit, 1=16-bit
    
    //I2C device address
    char    i2cAddress;
    
    //viz. struct PERIPH_TYPE, 0-SPI, 1-PMP, 2-I2C, ...
    char    periphType;
}PORT_INFO;


//#define     RGB565              5
//#define     RGB666              6
//#define     DISPLAY_BUS_8 

const struct
{
    short   White;
    short   Black;
    short   Red;
    short   Blue;
    short   Green;
    short   Orange;
    short   Yellow;
    short   Magenta;
    short   Cyan;
    short   GreenYellow;
    short   Pink;
    short   Purple;
    short   Maroon;
    short   DarkGreen;
    short   Olive;
    short   Grey;
    short   LightGrey;
    short   DarkGrey;
    
}COLOR = { 0xFFFF, 0x0000, 0xF800, 0x001F, 0x07E0, 0xFD20, 0xFFE0, 0xF81F, 0x07FF, 0xAFE5, 0xF81F, 0x780F, 0x7800, 0x03E0, 0x7BE0, 0x79EF, 0xC318, 0x38E7 };

const short stdColorMap[]={0x0000, 0x1082, 0x2104, 0x3186, 0x4208, 0x528A, 0x630C, 0x738E,
                           0x8410, 0x9492, 0xA514, 0xB596, 0xC618, 0xD69A, 0xE71C, 0xF79E};

typedef struct
{
    short   x1;
    short   y1;
    short   x2;
    short   y2;
    short   color;
    short   displayWidth;
    short   displayHeight;
}LINE_SRC;

typedef struct
{
    short   x;
    short   y;
    short   color;
}POINT;

typedef struct
{
    char    file_id;        //0 0x1-font FS, 0x2-font VS, 0x9 image BMP
    char    format;         //1 0x1, 0x4, 0x55
    char    blockSize;      //2 0, 16, 32 
    char    compression;    //3 0-neni, 1-RLE    
    short   width;          //4
    short   height;         //6
    
    int     rleCnt;         //8       
    int     rleData;        //12
    char*   srcPosition;
    int     srcWord;
    
    void*   getNextData;
    void*   getPoint;
    void*   pointToBuffer;
    void*   onScreen;
    
    char*   srcStartPosition;
    char*   srcAfter;
    short   foreColor;
    short   bgColor;    
    short   x;
    short   y;
    short   start_x;
    short   start_y;
    short   end_x;
    short   end_y;
    //char    extSize;
    char*   fontDataAddr;
    short   fontItemSize;     
    char    firstAscii;
    char    firstVar;
    char    bitCnt;
    char    eof;

    
}IMAGE_SRC;

typedef struct 
{
    char            fileId;
    char            format;
    char            blockSize;
    char            compression;
    unsigned short  width;
    unsigned short  height;
    unsigned short  colorMapOffset;
    unsigned short  colorMapSize;
    unsigned short  dummy;
    unsigned short  dataOffset;
    unsigned int    dataSize;
    char            name[12]; 
}IMAGE_HEAD;

typedef struct 
{
    char            fileId;
    char            format;
    char            blockSize;
    char            compression;
    unsigned short  width;
    unsigned short  height;
    unsigned short  itemSize;
    char            firstAscii;
    char            firstVarAscii;
    char            style;
    char            lineOffset;
    unsigned short  dataOffset;
    int             dummy2;
    char            name[12]; 
}FONT_HEAD;

typedef struct
{
    //privatni struktura, spojena s jednim konkretnim displejem
    void (*selectPort)(PORT_INFO* pi, void* d);
    void (*initDisplay)(PORT_INFO* pi);
    void (*drawString)(char* text, IMAGE_SRC* font, short x, short y);
    void (*fillBox)(short x1, short y1, short x2, short y2, short color);
    void (*drawLine)(short x1, short y1, short x2, short y2, short w, short color);
    void (*drawImage)(IMAGE_SRC* da, short x, short y);
    void (*drawPoint)(short x, short y, short color);
    void (*print)(char* text);
    void (*clear)(short color);
    void (*setOrientation)(char x);
    void (*setBrightness)(char val);
    void (*controlDisplay)(char on, char sleep, char bl, char inv);
    short (*textWidth)(char* text, IMAGE_SRC* font);
    char (*getInitialized)();
    char (*getOrientation)();
    short (*getWidth)();
    short (*getHeight)();
    
    //privatni promene struktury DISLAY, spojene s konkretnim displejem (kazdy dislej na svoji struct DISPLAY)
    short print_y;
}DISPLAY;

typedef struct
{
    //globalni struktura pro graficky vystup. Jedna v celem projektu. 
    //Pri vice displejich, musi kazdy pred pouzitim graphics volat setGraphics
    //Tvori interface pro grafiku
    void (*drawCircle)(short x, short y, short r, short color);
    void (*drawBox)(short x1, short y1, short x2, short y2, short w, short color);
    
    void (*drawString)(char* text, IMAGE_SRC* font, short x, short y);
    void (*fillBox)(short x1, short y1, short x2, short y2, short color);
    void (*drawLine)(short x1, short y1, short x2, short y2, short w, short color);
    void (*drawImage)(IMAGE_SRC* da, short x, short y);
    void (*drawPoint)(short x, short y, short color);
    void (*print)(char* text);
    void (*clear)(short color);
    short (*textWidth)(char* text, IMAGE_SRC* font);
    
}GRAPHICS;

typedef struct
{
    char*           tx_buffer;
    char*           rx_buffer;
    void*           finishFn;
    void*           eventFn;
    volatile int*   hw_buffer;
    
    short           len;            //celkova delka dat v bufferu
    short           tx_count;       //pocet prijatych bytes

    char            mode_count;     //citac dat v mode1
    char            mode;           //b0 0=pouze data, 1=control byte,dat
    char            used;           //indikuje, ze nektera fce pouziva SPI, kanal neni volny
    char            process;        //0=buffer volny, 1=buffer ceka na osedlani, 2=buffer je odesilan
    
}SPIControl;

typedef struct 
{
    char*   tx_buffer;
    char*   rx_buffer;
    void*   finishFn;

    short   len; 
    char    address;                //slave device
    char    used;                   //volny/obsazeny
    char    process;                //sending/finished
    char    error;   
    char    internalState;          //state ridi start/data/end signal
   
}I2CControl;


void globals_Start();