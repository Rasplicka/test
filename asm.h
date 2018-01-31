//fn.S
//extern char* stack_area;

extern void allocStack(int, int*);

extern void doEvents();
extern void startEvents();
extern void doEventsL();
extern uint getGP();
extern void setStack(void*, int);

extern int strLen(char*);
extern int concat(char*, char*, int);
extern void lTrim(char*, char*);
extern void rTrim(char*, char*);
extern void trim(char*, char*);
extern void addSpace(char*, int);
extern void formatLeft(char*, int, char*);
extern void formatRight(char*, int, char*);

extern int rndInt(int, int);
extern int random(int); 
extern uint compareTimerMs(uint);
extern void pauseEvent(uint);
extern void uintToChar(uint, char*, int);
extern void intToChar(int, char*, int);
extern void byteToChar(int, char*, int);

extern void enableInterrupt();

extern void setPortDigOut(uint, uint);
extern void setPortDigIn(uint, uint);
extern void setPortAnalogIn(uint, uint);
extern void setPortOpenDrain(uint, uint, uint);
extern void setPortPullUp(uint, uint, uint);
extern void setPortPullDown(uint, uint, uint);

extern void defragTableW(int*, int, int);
extern void defragTableB(char*, int, int);
extern void memcpy32(char*, char*, int);

//font + image
//extern void setFontSrc(const void*, void*);
//extern int fontCharParam(void*, char);

//extern void setImageColorMap(void*, short*);
//extern void setImageSrc(const void*, void*);
//extern void fillRectDirect(short color, int pixels, int*);
//extern int imageToBuffer(void*, char*, int, int);
//extern short RGB16(char, char, char);
//extern short correctColor16(short);
//extern void drawLineQuick(void*, void*, short, void*);
//extern void drawPointQuick(short, short, short, void*);



extern void pwm_soft_timer();
extern void pwm_soft_linear_up();
extern void pwm_soft_linear_down();
extern void pwm_soft_exp();

extern void setSrsValue();
extern void setSrsValue2(char*);
extern void setInterrupt();
extern void enableInterrupt();

extern void iVector_timer1();
extern void iVector_usb();
extern void iVector_rtc();
extern void iVector_adc();
extern void iVector_spi1Tx();
extern void iVector_spi2Tx();

#ifdef PIC32MM0256      //I2C ma pouze PIC32MM0256
extern void iVector_i2c1Slave();
extern void iVector_i2c1Master();
extern void iVector_i2c1Bus();
extern void iVector_i2c2Slave();
extern void iVector_i2c2Master();
extern void iVector_i2c2Bus();
extern void iVector_i2c3Slave();
extern void iVector_i2c3Master();
extern void iVector_i2c3Bus();
#endif

extern void cpuTimer_Init();
extern void general_exception();
extern void watch(uint);


//timer1
extern uint timer_ms;
extern uint day_ms;
extern void timer1_Init();
extern int  timer1_regEventInterval(char*, uint);
extern int  timer1_regEventDelay(char*, uint);
extern void timer1_unregEvent(char*);


#ifdef RTC

//rtc.S
extern void rtc_Init();
extern void rtc_setDayMs();
extern uint rtc_getTime();
extern uint rtc_getDate();
extern int  rtc_getHalfSecond();

extern void rtc_setTime(uint);
extern void rtc_setDate(uint);

#endif

#ifdef UBTN

//ubtn.S
extern void ubtn_start();
extern int  ubtn_regEvent(void*);
extern void ubtn_unregEvent(void*);

#endif