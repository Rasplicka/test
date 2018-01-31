
typedef struct {
    char* tx_buffer;
    char* rx_buffer;
    int tx_count;
    int len; 
    int inProcess;    
    int working;
    void* finish;
} spiControl;