
//char* disp1306a_stack;

void disp1306a_start();
void disp1306a_drawText(int index, char* string, int col, int row, int font);
void disp1306a_drawString(int index, char* string, int x, int y, int font);
void disp1306a_drawImage(int index, char* image, int x, int y);
void disp1306a_fillRect(int index, int x1, int y1, int x2, int y2, int color);
void disp1306a_print(int index, char* string);
void disp1306a_clear(int index);
void disp1306a_setContrast(int index, int value);
void disp1306a_sleep(int index);
void disp1306a_resume(int index);
int  disp1306a_getReady(int index);
char* disp1306a_getImageData(int image_id);
int  disp1306a_getWidth(int index);
int  disp1306a_getHeight(int index);