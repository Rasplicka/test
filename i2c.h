void i2c_Init();

char i2c_getUsed(int index);
char i2c_Process(char index, char wait);
void i2c_Free(int index);
char i2c_Use(char index, char wait, void* finish);
void i2c_WriteDE(int index, char* buffer, int len, int addr);
void i2c_Write(int index, char* buffer, int len, int addr);

void i2c1_MasterInterrupt();
void i2c2_MasterInterrupt();
void i2c3_MasterInterrupt();

void i2c1_SlaveInterrupt();
void i2c1_BusInterrupt();
void i2c2_SlaveInterrupt();
void i2c2_BusInterrupt();
void i2c3_SlaveInterrupt();
void i2c3_BusInterrupt();