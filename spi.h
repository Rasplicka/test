void spi_Init();

char spi_getUsed(int index);
//void spi_setUsed(int index, int val);
char spi_Process(char index, char wait);
void spi_ExchangeDE(int index, char* txbuff, char* rxbuff, int l);
void spi_Exchange(int index, char* txbuff, char* rxbuff, int l);
void spi_ExchangeModeDE(int index, char* txbuff, char* rxbuff, int l, char mode);
void spi_ExchangeMode(int index, char* txbuff, char* rxbuff, int l, char mode);
void spi_Free(int index);
char spi_Use(char index, char wait, void* finish, void* event);                 
volatile int* spi_getHwBuffer(char index);
void spi_setBusMode(char index, char mode);
void spi_setSpeed(int index, int speed);

void spi1_TxInterrupt();
void spi2_TxInterrupt();
void spi3_TxInterrupt();