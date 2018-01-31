//font + image
extern void setFontSrc(const void*, void*);
extern int fontCharParam(void*, char);

extern void setImageColorMap(void*, short*);
extern void setImageSrc(const void*, void*);
extern void fillRectDirect(short color, int pixels, int*);
extern int imageToBuffer(void*, char*, int, int);
extern short RGB16(char, char, char);
extern short correctColor16(short);
//extern void drawLineQuick(void*, void*, short, void*);
extern void drawLineQuick(void*, void*, void*);
//extern void drawPointQuick(short, short, short, void*);
extern void drawPointQuick(void*, int*, void*);
