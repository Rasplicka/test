

void usb_mm_Init();
int USB_isConnected();
void USB_txData(char ep, char* buffer, short int len);
int USB_isTxProgress(char ep);
void USB_rxData(char ep, char* buffer, short int len);
int USB_isRxProgress(char ep);


