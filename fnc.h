

typedef struct {
    char* result;
    int len;
    char* s1;
    char* s2;
    char* s3;
    char* s4;
    
}stringCreator;

void pauseEvent2(int ms);
int createString(char* buffer, int buffer_len, char* a, ...);                   //volitelny pocet argumentu char*
void createStringStruct(stringCreator sc);
int appendString(char* buffer, int buffer_len, char* a, ...);                   //volitelny pocet argumentu char*
void alignLeft(char* str, int strlen);

float roundFloat(float f, int dec);
void floatToString(float f, char* ret, int dec);
void floatToStringFormat(float f, char* ret, int w, int dec);

void drawBattery1306(int index, int x, int y, int state);

