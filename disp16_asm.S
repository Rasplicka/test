#include "xc.h"
#include "def.h"  
#include "asm_macro.S"
   

//.set mips32    
#define MICROMIPS  
.set micromips     
.section .asm_fn, code    

//pausa pri directWrite do SPI hw bufferu (zavisi na rychlosti CPU, zapis bytes musi byt pomalejsi, nez vysilani SPI)
#define SPI_DIRECT_WAIT_STATE	2	    //plati pro PIC32MM, 24MHz, rychlejsi CPU by mely mit delsi pauzu
    
//<editor-fold defaultstate="collapsed" desc="IMAGE_SRC definition">
    //datova struktura pro IMAGE a FONT
#define	    _FILEID_OFF		    0
#define	    _FORMAT_OFF		    1
#define	    _BLOCKSIZE_OFF	    2
#define	    _COMPRESSION_OFF	    3
#define	    _WIDTH_OFF		    4
#define	    _HEIGHT_OFF		    6
    
    //save s0-s3
#define	    _RLECNT_OFF		    8
#define	    _RLEDATA_OFF	    12
#define	    _SRCPOSITION_OFF	    16    
#define	    _SRCWORD_OFF	    20    
    //load s0-s7
#define	    _GETNEXTDATA_OFF	    24
#define	    _GETPOINT_OFF	    28
#define	    _POINTTOBUFFER_OFF	    32
#define	    _ONSCREEN_OFF	    36
    
#define	    _SRCSTARTPOSITION_OFF   40    
    //load 2xWord (colorMap, nebo foreColor+bgColor)
#define	    _SRCAFTER_OFF	    44     
#define	    _FORECOLOR_OFF	    48
#define	    _BGCOLOR_OFF	    50

#define	    _X_OFF		    52
#define	    _Y_OFF		    54
    //load 2xWord
   
#define	    _STARTX_OFF		    56
#define	    _STARTY_OFF		    58
#define	    _ENDX_OFF		    60
#define	    _ENDY_OFF		    62
    
    //load 2xWord
#define	    _FONTDATAADDR_OFF	    64
#define	    _FONTITEMSIZE_OFF	    68   
#define	    _FIRSTASCII_OFF	    70
#define	    _FIRSTVAR_OFF	    71  
    
#define	    _BITCNT_OFF		    72   
#define	    _EOF_OFF		    73
    
//</editor-fold>
   
//<editor-fold defaultstate="collapsed" desc="IMAGE_HEAD, FONT_HEAD definition">
	//IMAGE_HEAD offset polozek (hlavicka souboru Image)
#define	    _HEAD_COLORMAPOFFSET_OFF	8
#define	    _HEAD_COLORMAPSIZE_OFF	10
#define	    _HEAD_DATASIZE_OFF		16
    
	//FONT_HEAD offset polozek (hlavicka souboru Font)
#define	    _HEAD_FONTITEMSIZE_OFF	8  
#define	    _HEAD_FIRSTASCII_OFF	10
#define	    _HEAD_FIRSTVAR_OFF		11  
	
	//IMAGE_HEAD + FONT_HEAD offset
#define	    _HEAD_DATAOFFSET_OFF	14

//</editor-fold>
	    
tableOfgetNextData:
    .word   get_next_data_8
    .word   get_next_data_16
    .word   get_next_data_32
    .word   get_next_data_8rle
    .word   get_next_data_16rle
    .word   get_next_data_32rle  
    
//<editor-fold defaultstate="collapsed" desc="setImageColorMap">
.align 2
.global setImageColorMap
.ent setImageColorMap
setImageColorMap:
//a0=IMAGE_SRC
//a1=colorMap
    
    //nastavuje jinou adresu colorMap (format 0x4)
    //colorMap=short[16] - 16x definice 16-bit barvy
    
    sw32    a1, _FORECOLOR_OFF(a0)	//foreColor + bgColor tvori adresu colorMap
    jrc	    ra
    
.end setImageColorMap  
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="setImageSrc">
.align 2
.global setImageSrc    
.ent setImageSrc
setImageSrc:    
//a0=adresa head
//a1=adresa IMAGE_SRC   
    
    //nastavuje data z head: fileId, format, blockSize, compression, width, height
    //pokud je format=0x4, nastavi adresu colorMap, pokud existuje (offset i size > 0)
    //nastavuje adr. fci: getNextData, getPoint, pointToBuffer
    //nastavuje srcStartPosition, srcAfter, expSize, eof=1
 
    
    lwp	    t8, (a0)		//id, format, blockSize, compression, width, height
    swp	    t8, (a1)
    
    //srcStartPosition
    lhu	    t9, _HEAD_DATAOFFSET_OFF(a0)    //offset dat za head
    addu    t9, a0
    sw32    t9, _SRCSTARTPOSITION_OFF(a1)   //startPosition
    
    //srcAfter
    lw32    t8, _HEAD_DATASIZE_OFF(a0)	    //velikost dat
    addu    t9, t8
    sw32    t9, _SRCAFTER_OFF(a1)	    //dataAfter
    
    //<editor-fold defaultstate="collapsed" desc="format 0x1">
    //test format 0x1
    lbu32   v0, _FORMAT_OFF(a1)
    li16    v1, 0x1
    bne	    v0, v1, 2f
    nop
    
    //format 0x1
    la	    v1, point_to_buffer_01
    sw32    v1, _POINTTOBUFFER_OFF(a1)		//pointToBuffer
    la	    v1, get_next_point_01		    
    sw32    v1, _GETPOINT_OFF(a1)		//getPoint
    
    //addiu   v1, zero, 16
    //sb	    v1, _EXPSIZE_OFF(a1)		//exp size
    
    nop
    b16	    10f					//after_format
    nop
    
    //</editor-fold>
    
    //<editor-fold defaultstate="collapsed" desc="format 0x4">
2:
    //test format 0x4, v0=format
    li16    v1, 0x4
    bne	    v0, v1, 7f
    nop
    
    //format 0x4
    la	    v1, point_to_buffer_04
    sw32    v1, _POINTTOBUFFER_OFF(a1)		//pointToBuffer  
    la	    v1, get_next_point_04		    
    sw32    v1, _GETPOINT_OFF(a1)		//getPoint
    
    //addiu   v1, zero, 0x4
    //sb	    v1, _EXPSIZE_OFF(a1)		//exp size
    
    //nastav colorMap
    lhu	    t9, _HEAD_COLORMAPSIZE_OFF(a0)
    beqz    t9, 5f				//skok, colorMap neexistuje (size=0)
    nop    
    lhu	    t9, _HEAD_COLORMAPOFFSET_OFF(a0)		//load offset
    beqz    t9, 5f				//skok, colorMap neexistuje (offset=0)
    nop
    
    //colorMap existuje, t9=offset
    addu    t9, a0
    sw32    t9, _FORECOLOR_OFF(a1)		//foreColor+bgColor tvori adresu colorMap
    
    nop
    b16	    10f					//after_format
    nop
    
5: 
    //colorMap neexistuje, nastav std
    la	    t9, stdColorMap
    sw32    t9, _FORECOLOR_OFF(a1)
    nop
    b16	    10f					//after_format
    nop
    
    //</editor-fold>
    
    //<editor-fold defaultstate="collapsed" desc="format 0x55">
7:  
   //test format 0x55, v0=format 
    addiu   v1, zero, 0x55
    bne	    v0, v1, 99f				//skok, jiny format, chyba
    nop
   
    //format 0x55
    la	    v1, point_to_buffer_55
    sw32    v1, _POINTTOBUFFER_OFF(a1)		//pointToBuffer  
    //addiu   v1, zero, 0x2
    //sb32    v1, _EXPSIZE_OFF(a1)		//exp size
    
    //getNexpPoint podle blockSize
    lbu32   v0, _BLOCKSIZE_OFF(a1)
    li16    v1, 0x8
    bne	    v0, v1, 8f				//skok, blockSize=16/32
    nop
    
    //blockSize 8
    la	    v1, get_next_point_55b		    
    sw32    v1, _GETPOINT_OFF(a1)		//getPoint
    
    nop
    b16	    10f					//after_format
    nop
    
8:
    //blockSize 16/32
    la	    v1, get_next_point_55a		    
    sw32    v1, _GETPOINT_OFF(a1)		//getPoint
    
    //</editor-fold>
    
    //<editor-fold defaultstate="collapsed" desc="getNextData">
   
10: //after_format: 
    
    //nastav getNextPoint, podle blockSize a RLE
    lbu32   v1, _BLOCKSIZE_OFF(a1)			    //8/16/32
    lbu32   t9, _COMPRESSION_OFF(a1)			    //0/1
    bnez    t9, 11f
    nop
    
    //nema kopresi, v1=blockSize
    //reg, val, set_reg, set_result, branch (set and branch if equal - reg=val)
    _SET_B_IF_EQUI v1, 8, v0, 0, 12f			    //meni t9
    _SET_B_IF_EQUI v1, 16, v0, 1, 12f			    //meni t9
    _SET_B_IF_EQUI v1, 32, v0, 2, 12f			    //meni t9		
    
    nop
    b16	    99f						    //jiny format
    nop
    
11:
    //ma kopresi, v1=blockSize  
    //reg, val, set_reg, set_result, branch (set and branch if equal - reg=val)
    _SET_B_IF_EQUI v1, 8, v0, 3, 12f			    //meni t9
    _SET_B_IF_EQUI v1, 16, v0, 4, 12f			    //meni t9
    _SET_B_IF_EQUI v1, 32, v0, 5, 12f			    //meni t9
    
    nop
    b16	    99f						    //jiny format
    nop
    
12:    
    //v0=index fce 0-5
    la	    t9, tableOfgetNextData			    //tabulka fci
    lwxs    t8, v0(t9)					    //load adresu fce
    sw32    t8, _GETNEXTDATA_OFF(a1)			    //save adresu fce
    
    //</editor-fold>

    //eof=1
    li16    v1, 1
    sb32    v1, _EOF_OFF(a1)
    
    jrc	    ra
    
99:
    //jiny format, nastavi adresu fce GETPOINT na eof, tzn. ze imageToBuffer nic neprovede
    la	    v1, imageToBuffer_eof		    
    sw32    v1, _GETPOINT_OFF(a1)		//getPoint = eof   
    jrc	    ra
    
.end setImageSrc    
    
//</editor-fold>
  
//<editor-fold defaultstate="collapsed" desc="fillRectDirect">
.align 2
.global fillRectDirect
.ent fillRectDirect
fillRectDirect:
//a0=color
//a1=pocet pixelu
//a2=adresa hwBufferu (SPIxBUF)    

    
    //zapisuje barvu a0 na display, zadany pocet pixelu, zapisem primo do SPIBUF
    //do kazdeho bodu zapise a0, direct write data >> SPIBUF

    li	    v0, SPI_DIRECT_WAIT_STATE
    addiu   v1, v0, 1			//wait state +1
1:    
    //li16    v1, 4
    //lw	    t9, (SPI2STAT)
    //ext	    t9, t9, 16, 5
    //subu    t9, v1
    //bgtz    t9, 1b
    //nop
    
    //wait state
2:
    addiu   v1, -1
    bnez    v1, 2b
    nop
    
    
    sh	    a0, (a2) //(SPI2BUF)	//zapis do hw bufferu
    addiu   a1, -1			//count--
    addiu   v1, v0, 1			//wait state +1
    bnez    a1, 1b
    nop
    
    jrc	    ra
    
.end fillRectDirect 
    
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="imageToBuffer">    
.align 2
.global	imageToBuffer
.ent imageToBuffer
imageToBuffer:
//a0=IMAGE_SRC struct
//a1=buffer
//a2=buffer len
//a3 0=prohozeni H a L bajtu barvy, pri 8-bit komunikaci, 1=neprovadi prohozeni barvy, 2=directWrite >> a1 (adresa SPIxBUF)
    
    //fce provadi presun dat definovanych IMAGE_SRC do bufferu
    //pokud dosahne konce dat, nastavi eof=1 (dalsi volani provede znovu inicializaci)
    //pokud zaplni buffer, ulozi aktualni pozici a v pristim volani pokracuje (eof=0)
    //vysledny pixel je 16-bit barva.
    //Pokud je v display.h definovano DISPLAY_BUS_8 prohodi H a L byte barvy (wsbh)
    //Pri 16-bit komunikaci (SPI, PMP) je barva ok.
    //format barvy b15-b0 RRRRRGGG-GGGBBBBB
    
    _PUSH_S07RA
    
    lbu32   v0, _EOF_OFF(a0)
    beqz    v0, 2f  //start			//skok, pokracovani
    nop
    
    //<editor-fold defaultstate="collapsed" desc="init values">
    //eof=1, provede init values
    
    lw32    v0, _SRCSTARTPOSITION_OFF(a0)	//srcPosition=startPosition
    sw32    v0, _SRCPOSITION_OFF(a0)
    
    sw32    zero, _RLECNT_OFF(a0)		//nuluj rleCnt
    sb32    zero, _BITCNT_OFF(a0)		//nuluj bitCnt
    sw32    zero, _X_OFF(a0)			//nuluj x+y
    
    lh32    v0, _STARTX_OFF(a0)			//load signed hword
    li16    v1, -1
    beq     v0, v1, 1f				//skok, zadne pixely mimo obraz
    nop
    
    //nastav fci onScreen na test (cast pixelu mimo obraz)
    la	    v0, test_on_screen
    sw32    v0, _ONSCREEN_OFF(a0)
    nop
    b16	    2f					//start
    nop
    
1:
    //nastav fci onScreen na pointToBuffer (zadne pixely mimo obraz), neprovadi test
    lw32    v0, _POINTTOBUFFER_OFF(a0)
    sw32    v0, _ONSCREEN_OFF(a0)
    
    //</editor-fold>

    
2:  //start   
    //vyber vystupni fci (s wsbh, bez swbh, directWrite)
    tgeiu   a3, 0x3			    //trap, pokud a3>=3 (mozne volby 0,1,2)
    la	    t9, 4f
    lwxs    a3, a3(t9)			    //a3=adresa vystupni fce
    
3:    
    //load 8xWord
    lwm32   s0-s7, _RLECNT_OFF(a0)
    //load data
    lwp32   t0, _SRCAFTER_OFF(a0)	    //srcAfter+color fore=b0-15, bg=b16-31
    lwp32   t4, _STARTX_OFF(a0)		    //startX t4 L, startY t4 H, endX t5 L, endY t5, H
    lhu32   t2, _X_OFF(a0)		    //x
    lhu32   t3, _Y_OFF(a0)		    //y
    lbu32   t6, _BITCNT_OFF(a0)
    lhu32   v1, _WIDTH_OFF(a0)
    li16    v0, 0			    //return size v bufferu
    
    //call getPoint
    jrc	    s5
    
4:  //vystupni fce
    .word   11f //x_cont_wsbh
    .word   12f //x_cont
    .word   5f //direct
    
5: //direct:  
    //directWrite (a1 obsahuje adresu SPIxBUF, kam zapisuje data)
    sh	    t8, (a1)
    //pokracuje next_loop_move_point
    
6://next_loop_move_point:
    beq	    s6, s7, 10f			    //skok, neprovadi move_point
    nop
    
    //move point (x++, y++)
7://move_point:
    addiu   t2, 1			//x++
    bne	    t2, v1, 10f			//skok, neni za x
    nop
    
    //konec radku, x=0, zvysit y
    move    t2, zero			//x=0
    ext	    t9, t5, 16, 16		//endY je v t5 H
    beq	    t9, t3, imageToBuffer_eof   //skok, je na poslednim radku
    nop
    
    //neni na poslednim, posun Y
    addiu   t3, 1			//y++

10:
    //start dalsi pixel, call getPoint
    jrc	    s5				    //getPoint
    nop
    
    
    //<editor-fold defaultstate="collapsed" desc="pointToBuffer">
    
point_to_buffer_55:
    //t7 obsahuje primo barvu pixelu
    move    t8, t7
    
    jrc	    a3				//skok na vystupni fci

point_to_buffer_04:
    //t7.b0-3=pixdata
    sll	    t7, 1			    //x 2
    addu    t7, t1			    //t1=colorMap (+ color)
    lhu	    t8, (t7)
    
    jrc	    a3				    //skok na vystupni fci
    
point_to_buffer_01:
    //t7.b0=pixdata
    ext	    t8, t1, 0, 16		    //foreColor
    ext	    t9, t1, 16, 16		    //bgColor
    movz    t8, t9, t7			    //do t8 bgColor, pokud t7=0        
    
    jrc	    a3				    //skok na vystupni fci
    
11: //x_cont_wsbh:  
    //t8=vysledna barva, ale bude upravena na 8-bit tx
    wsbh    t8
    
12: //x_cont:
    //t8=vysledna barva, write to buffer
    sh	    t8, (a1)			    //color > buffer
  
    addiu   a1, 2			    //buffer++
    addiu   v0, 2			    //return len ++
    bne	    v0, a2, 6b //next_loop_move_point    //skok, buffer neni plny
    nop
   
    //buffer je plny
  
    
    //</editor-fold>    
    
    //<editor-fold defaultstate="collapsed" desc="buffer full, EOF">
13: //buffer_full:
    //buffer je plny, ale neni konec dat, save data, (eof=0), v0=pocet bytes v bufferu
    
    //save
    swm32   s0-s3, _RLECNT_OFF(a0)
    sh32    t2, _X_OFF(a0)
    sh32    t3, _Y_OFF(a0)
    sb32    t6, _BITCNT_OFF(a0)
    
    sb32    zero, _EOF_OFF(a0)			//eof=0
    
    _POP_S07RA
    jrc	    ra
    
imageToBuffer_eof: //is_eof:   
    //je konec dat, nastav eof=1, v0=pocet bytes v bufferu
    li16    v1, 1
    sb32    v1, _EOF_OFF(a0)
    
    _POP_S07RA
    jrc	    ra 
    
    //</editor-fold>    
    
    //<editor-fold defaultstate="collapsed" desc="test on screen">
test_on_screen:
    //startX
    ext	    t9, t4, 0, 16		    //startX do t9
    subu    t9, t2, t9
    bltz    t9, 7b //move_point		    //skok, bod je mimo obraz
    nop
    
    //startY
    ext	    t9, t4, 16, 16
    subu    t9, t3, t9
    bltz    t9, 7b //move_point		    //skok, bod je mimo obraz
    nop
    
    //exdX
    ext	    t9, t5, 0, 16
    subu    t9, t2, t9
    bgtz    t9, 7b //move_point		    //skok, bod je mimo obraz
    nop
    
    //endY
    ext	    t9, t5, 16, 16
    subu    t9, t3, t9
    bgtz    t9, 7b //move_point		    //skok, bod je mimo obraz
    nop
    
    //neni mimo obrazovku
    jrc	    s6				    //pointToBuffer01/04/55
    
    //</editor-fold>
 
    //<editor-fold defaultstate="collapsed" desc="getNextPoint">
get_next_point_01:
    //t6=bitCnt, s3=srcWord
    bnez    t6, 1f			    //skok, srcWord obsahuje data
    nop
  
    //load data
    jalr    s4				    //load pixdata (srcWord s3)
    nop
    lbu32    t6, _BLOCKSIZE_OFF(a0)	    //bitCnt = blockSize
    
1:    
    addiu   t6, -1			    //bitCnt--
    ext	    t7, s3, 31, 1		    //t7(pixdata)=srcWord.b31
    sll	    s3, 1			    //srcWord<<1
    
    jrc	    s7				    //testOnScreen, nebo pointToBuffer01
    nop
    
get_next_point_04:
    //t6=bitCnt, s3=srcWord
    bnez    t6, 1f			    //skok, srcWord obsahuje data
    nop    
    
    //load data
    jalr    s4
    nop
    lbu32   t6, _BLOCKSIZE_OFF(a0)	    //bitCnt = blockSize
    srl	    t6, 2			    // /4
    
1:
    addiu   t6, -1
    ext	    t7, s3, 28, 4		    //t7(pixdata)=srcWord.b28-31
    sll	    s3, 4			    //srcWord<<4    
    
    jrc	    s7				    //testOnScreen, nebo pointToBuffer04
    nop
    
get_next_point_55a: //format 0x55, blockSize 16/32
    //t6=bitCnt, s3=srcWord
    bnez    t6, 1f			    //skok, srcWord obsahuje data
    nop   
    
    //load data
    jalr    s4
    nop
    //wsbh    s3, s3			    //prohodit B0 s B1, a B3 s B2
    lbu32   t6, _BLOCKSIZE_OFF(a0)	    //bitCnt = blockSize
    srl	    t6, 4			    // /16    
    
1:
    addiu   t6, -1
    ext	    t7, s3, 16, 16		    //t7(pixdata)=srcWord.b16-31
    sll	    s3, 16			    //srcWord<<16    
    
    jrc	    s7				    //testOnScreen, nebo pointToBuffer55
    nop    
    
get_next_point_55b: //format 0x55, blockSize 8
    
    //load data 8-bit
    jalr    s4
    nop
    srl	    t7, s3, 16			    //s3(b24-31) > t7(8-15) 
    //ext	    t7, s3, 24, 8		    //t7(pixdata) b0-b7
    
    jalr    s4
    nop
    srl	    s3, 24			    //s3(24-31) > s3(0-7)
    or	    t7, s3
    //ext	    t9, s3, 24, 8		    //vyjmout b24-31
    //ins	    t7, t9, 8, 8		    //vlozit do pixdata.b8-15
    
    jrc	    s7
    nop
    
    //</editor-fold>

    //<editor-fold defaultstate="collapsed" desc="get_next_data_8, RLE">
    
get_next_data_8:
    //s2=position, t0=after
    beq	    s2, t0, imageToBuffer_eof	    //skok, je za daty
    nop
    
    lbu	    s3, (s2)			    //srcWord=data
    addiu   s2, 1			    //position++    
    sll	    s3, 24			    //posun do b24-31
    
    jrc	    ra
    
get_next_data_8rle:
    //rleCnt=s0, rleData=s1
    bgtz    s0, 3f			    //skok, rleCnt>0
    nop
    
    bltz    s0, 1f			    //skok, rleCnt<0
    nop
    
    //rleCnt=0, load dalsi
    //s2=position, t0=after    
    beq	    s2, t0, imageToBuffer_eof	    //skok, je za daty
    nop
    
    //dalsi rleCnt
    lb	    s0, (s2)			    //load rleCnt
    addiu   s2, 1			    //position++
    bgtz    s0, 2f			    //skok, rleCnt > 0
    nop
    
1:    
    lbu	    s3, (s2)			    //load data
    addiu   s2, 1			    //position ++
    sll	    s3, 24			    //posun do b24-31
    addiu   s0, 1			    //rleCnt++
    jrc	    ra
    
2:
    //load nove rleData
    lbu	    s1, (s2)			    //load rleCnt
    addiu   s2, 1			    //position ++
    sll	    s1, 24			    //posun do b24-31
    
3:
    //rleCnt>0, dalsi rleData
    move    s3, s1			    //srcWord < rleData
    addiu   s0, -1			    //rleCnt--
    jrc	    ra
    
    //</editor-fold>
    
    //<editor-fold defaultstate="collapsed" desc="get_next_data_16, RLE">
    
get_next_data_16:
    //s2=position, t0=after
    beq	    s2, t0, imageToBuffer_eof	    //skok, je za daty
    nop
    
    lhu	    s3, (s2)			    //srcWord=data
    addiu   s2, 2			    //position++
    sll	    s3, 16			    //posun do b16-31

    jrc	    ra   
    
get_next_data_16rle:
    //rleCnt=s0, rleData=s1
    bgtz    s0, 3f			    //skok, rleCnt>0
    nop
    
    bltz    s0, 1f			    //skok, rleCnt<0
    nop
    
    //rleCnt=0, load dalsi
    //s2=position, t0=after    
    beq	    s2, t0, imageToBuffer_eof	    //skok, je za daty
    nop
    
    //dalsi rleCnt
    lh	    s0, (s2)			    //load rleCnt
    addiu   s2, 2			    //position++
    bgtz    s0, 2f			    //skok, rleCnt > 0
    nop
    
1:    
    lhu	    s3, (s2)			    //load data
    addiu   s2, 2			    //position ++
    sll	    s3, 16			    //posun do b16-31
    addiu   s0, 1			    //rleCnt++
    jrc	    ra
    
2:
    //load nove rleData
    lhu	    s1, (s2)			    //load rleCnt
    addiu   s2, 2			    //position ++
    sll	    s1, 16			    //posun do b16-31
    
3:
    //rleCnt>0, dalsi rleData
    move    s3, s1			    //srcWord < rleData
    addiu   s0, -1			    //rleCnt--
    jrc	    ra
    
    //</editor-fold>
    
    //<editor-fold defaultstate="collapsed" desc="get_next_data_32, RLE">
    
get_next_data_32:
    //s2=position, t0=after
    beq	    s2, t0, imageToBuffer_eof	    //skok, je za daty
    nop
    
    lw32    s3, (s2)			    //srcWord=data
    addiu   s2, 4			    //position++

    jrc	    ra       
    
get_next_data_32rle:    
    //rleCnt=s0, rleData=s1
    bgtz    s0, 3f			    //skok, rleCnt>0
    nop
    
    bltz    s0, 1f			    //skok, rleCnt<0
    nop
    
    //rleCnt=0, load dalsi
    //s2=position, t0=after    
    beq	    s2, t0, imageToBuffer_eof	    //skok, je za daty
    nop
    
    //dalsi rleCnt
    lw	    s0, (s2)			    //load rleCnt
    addiu   s2, 4
    bgtz    s0, 2f			    //skok, rleCnt > 0
    nop
    
1:    
    lw	    s3, (s2)			    //load data
    addiu   s2, 4			    //position ++
    addiu   s0, 1			    //rleCnt++
    jrc	    ra
    
2:
    //load nove rleData
    lw	    s1, (s2)			    //load rleCnt
    addiu   s2, 4			    //position ++
    
3:
    //rleCnt>0, dalsi rleData
    move    s3, s1			    //srcWord < rleData
    addiu   s0, -1			    //rleCnt--
    jrc	    ra
    
    //</editor-fold>
    
.end imageToBuffer    
    
//</editor-fold>    
    
//<editor-fold defaultstate="collapsed" desc="setFontSrc">
.align 2
.global setFontSrc
.ent setFontSrc
setFontSrc:
//a0=src data (adresa fontu)    
//a1=IMAGE_SRC2 struct  
   
   //nastavuje data z head: fileId, format, blockSize, compression, width, height
    //pokud je format=0x4, nastavi adresu colorMap, pokud existuje (offset i size > 0)
    //nastavuje adr. fci: getNextData, getPoint, pointToBuffer
    //nastavuje srcStartPosition, srcAfter, expSize, eof=1
 
    
    lwp32    t8, (a0)				//id, format, blockSize, compression, width, height
    swp32    t8, (a1)				//save
    
    lhu32   t8, _HEAD_DATAOFFSET_OFF(a0)	//dataOffset
    addu    t8, a0				//base+offset = zacatek dat fontu
    
    lw32    t9, _HEAD_FONTITEMSIZE_OFF(a0)	//load itemSize, firstAscii, firstVar
    	    
    //save t8+t9
    swp32   t8, _FONTDATAADDR_OFF(a1)		//save dataOffset, itemSize, firstAscii, firstVar

    
    //<editor-fold defaultstate="collapsed" desc="format 0x1">
    //test format 0x1
    lbu32   v0, _FORMAT_OFF(a1)
    li16    v1, 0x1
    bne	    v0, v1, 2f
    nop
    
    //format 0x1
    la	    v1, point_to_buffer_01
    sw32    v1, _POINTTOBUFFER_OFF(a1)		//pointToBuffer
    la	    v1, get_next_point_01		    
    sw32    v1, _GETPOINT_OFF(a1)		//getPoint
    
    nop
    b16	    10f					//after_format
    nop
    
    //</editor-fold>
    
    //<editor-fold defaultstate="collapsed" desc="format 0x4">
2:
    //test format 0x4, v0=format
    li16    v1, 0x4
    bne	    v0, v1, 7f
    nop
    
    //format 0x4
    la	    v1, point_to_buffer_04
    sw32    v1, _POINTTOBUFFER_OFF(a1)		//pointToBuffer  
    la	    v1, get_next_point_04		    
    sw32    v1, _GETPOINT_OFF(a1)		//getPoint
    
    
    
    //colorMap u fontu neexistuje, nastav std
    la	    t9, stdColorMap
    sw32    t9, _FORECOLOR_OFF(a1)
    nop
    b16	    10f					//after_format
    nop
    
    //</editor-fold>
    
    //<editor-fold defaultstate="collapsed" desc="format 0x55">
7:  
   //test format 0x55, v0=format 
    addiu   v1, zero, 0x55
    bne	    v0, v1, 99f				//skok, jiny format, chyba
    nop
   
    //format 0x55
    la	    v1, point_to_buffer_55
    sw32    v1, _POINTTOBUFFER_OFF(a1)		//pointToBuffer  
    
    //getNexpPoint podle blockSize
    lbu32   v0, _BLOCKSIZE_OFF(a1)
    li16    v1, 0x8
    bne	    v0, v1, 8f				//skok, blockSize=16/32
    nop
    
    //blockSize 8
    la	    v1, get_next_point_55b		    
    sw32    v1, _GETPOINT_OFF(a1)		//getPoint
    
    nop
    b16	    10f					//after_format
    nop
    
8:
    //blockSize 16/32
    la	    v1, get_next_point_55a		    
    sw32    v1, _GETPOINT_OFF(a1)		//getPoint
    
    //</editor-fold>
    
    //<editor-fold defaultstate="collapsed" desc="getNextData">
   
10: //after_format: 
    
    //nastav getNextPoint, podle blockSize a RLE
    lbu32   v1, _BLOCKSIZE_OFF(a1)			    //8/16/32
    lbu32   t9, _COMPRESSION_OFF(a1)			    //0/1
    bnez    t9, 11f
    nop
    
    //nema kopresi, v1=blockSize
    //reg, val, set_reg, set_result, branch (set and branch if equal - reg=val)
    _SET_B_IF_EQUI v1, 8, v0, 0, 12f			    //meni t9
    _SET_B_IF_EQUI v1, 16, v0, 1, 12f			    //meni t9
    _SET_B_IF_EQUI v1, 32, v0, 2, 12f			    //meni t9		
    
    nop
    b16	    99f						    //jiny format
    nop
    
11:
    //ma kopresi, v1=blockSize  
    //reg, val, set_reg, set_result, branch (set and branch if equal - reg=val)
    _SET_B_IF_EQUI v1, 8, v0, 3, 12f			    //meni t9
    _SET_B_IF_EQUI v1, 16, v0, 4, 12f			    //meni t9
    _SET_B_IF_EQUI v1, 32, v0, 5, 12f			    //meni t9
    
    nop
    b16	    99f						    //jiny format
    nop
    
12:    
    //v0=index fce 0-5
    la	    t9, tableOfgetNextData			    //tabulka fci
    lwxs    t8, v0(t9)					    //load adresu fce
    sw32    t8, _GETNEXTDATA_OFF(a1)			    //save adresu fce
    
    //</editor-fold>

    //eof=1
    li16    v1, 1
    sb32    v1, _EOF_OFF(a1)
    
    jrc	    ra
    
99:
    //jiny format, nastavi adresu fce GETPOINT na eof, tzn. ze imageToBuffer nic neprovede
    la	    v1, imageToBuffer_eof		    
    sw32    v1, _GETPOINT_OFF(a1)		//getPoint = eof   
    jrc	    ra
   
.end setFontSrc    
   
//</editor-fold>     

//<editor-fold defaultstate="collapsed" desc="fontCharParam">
.align 2    
.global fontCharParam    
.ent fontCharParam    
fontCharParam:    
//a0=IMAGE_SRC struct
//a1=ascii code  
    
    //fce nastavi adresu dat znaku (a1) v FONT_SRC dataPosition, dataAfter
    //1. u fixed size fontu, je-li v povinne oblasti, vypocita adresu
    //2. u var size fontu, nebo je-li za povinnou oblasti, prochazi tabulku a hleda znak
    //nastavi FONT_SRC->width
    //volani fontToBuffer pouzije tuto pozici

    //test, zda je nastaven font - IMAGE_SRC v a0
    lbu32   v0, _FILEID_OFF(a0)
    beqz    v0, 12f
    nop
    
    //adresa zacatku tabulky znaku
    lw	    v1, _FONTDATAADDR_OFF(a0)	    //start tabulky znaku
    
    //test, zda ascii >= firstAscii
    lbu32   t7, _FIRSTASCII_OFF(a0)	
    _BAL_FLT	a1, t7, 14f		    //skok, ascii<first //pouziva t9
    
    //test fileid=0x1 - fixed size
    lbu32   t9, _FILEID_OFF(a0)		    //load file_id	//0x1=fixed font, 0x2=var font
    li16    v0, 1
    bne	    t9, v0, 9f			    //skok, neni fixed-size font
    nop    
    
    //je fixed size, test zda je ascii ve fixed oblasti
    lhu32   v0, _FONTITEMSIZE_OFF(a0)	    //load itemSize
    lbu32   t6, _FIRSTVAR_OFF(a0)
    _BAL_FBE	a1, t6, 8f //fs_free_area    //skok, je free /meni t9	    
    
    //adresu vypocita, t7=firstAscii
    subu    a1, t7			    //index ascii
    mult    v0, a1			    //v0=fixed itemSize
    mflo    t9
    addu    v1, t9			    //startData + offset = zacatek dat znaku  
    
    nop
    b16	    11f				    //nasel_adresu_znaku
    nop
    
8:  //fs_free_area:
    //znak je ve var oblasti t6=firtsVar, t7=firstAscii, v0=fixed itemSize
    //firstVar-firstAscii x fixedSize = offset prvniho Var
    subu    t6, t7
    mult    t6, v0
    mflo    t9
    addu    v1, t9			    //v1=adresa prvniho free znaku
    
9:    
    //vyhleda znak, je v free oblasti, v1=adresa oblasti
    li16    v0, 0			    //v0-velikost predchoziho znaku
    
10:    
    //dummy byte - pokud predchozi velikost byla licha, pripocita dummy byte
    addiu   v0, 1
    ins	    v0, zero, 0, 1
    
    addu    v1, v0			    //adresa + size predchoziho
    //move    t8, t9			    //zaloha ascii code predchoziho znaku, pro pripad, ze nenajde hledany
    lbu	    t9, (v1)			    //load ascii code    
    
    //test konce
    beqz    t9, 13f			    //skok, je na konci
    nop
    
    lhu	    v0, 2(v1)			    //aktualni item size
    bne	    t9, a1, 10b			    //skok, toto je jiny znak
    nop    
    
11: //nasel_adresu_znaku:   
    //ok, nasel znak, v1=adresa znaku, v0=size
    lbu	    t9, 1(v1)			    //load width
    sh32    t9, _WIDTH_OFF(a0)		    //save width
    
    addu    v0, v1			    //adresa + size = after
    sw32    v0, _SRCAFTER_OFF(a0)	    //save dataAfter
    
    addiu   v1, 4			    //+4 item head
    sw32    v1, _SRCSTARTPOSITION_OFF(a0)   //save dataPosition
    
    li16    v0, 0x1
    sb32    v0, _EOF_OFF(a0)		    //eof=1

12:    
    //vraci v0=0-chyba, neni nastaven font (IMAGE_SRC v a0), 1=ok (i kdyz nenajde ascii code a vraci prvni/posledni znak)
    jrc	    ra
    
    
13:    
    //nenasel ascii code znaku, vraci adresu posledniho znaku
    subu    v1, v0
    nop
    b16	    11b
    nop   
    
14:
    //ascii < first, neexistuje, vraci prvni (v1)
    lhu	    v0, 2(v1)			    //aktualni item size
    nop
    b16	    11b
    nop

.end fontCharParam
    
//</editor-fold>     
    
//<editor-fold defaultstate="collapsed" desc="RGB16">
.align 2
.global RGB16
.ent RGB16
RGB16:
//a0=R
//a1=G
//a2=B
    
    //b15      8 - 7      0  
    //  RRRRRGGG - GGGBBBBB

    
    //blue
    move    v0, a2		
    
    //green
    ext	    t9, a1, 0, 6
    ins	    v0, t9, 5, 6
    
    //red
    ext	    t9, a0, 0, 5
    ins	    v0, t9, 11, 5	    //R = b11...b15
    
    jrc	    ra
    
    
.end RGB16   

//</editor-fold>    
    
//<editor-fold defaultstate="collapsed" desc="correctColor16">
.align 2
.global correctColor16
.ent correctColor16
correctColor16:
//a0=vstup, barva v normalnim formatu b15 RRRRRGGG - GGGBBBBB b0   
//v0 vraci upravenou barvu, pokud odesila barvu na port v 8-bit modu (napr. SPI, PMP) musi prohodit H a L byte (wsbh)
    
#ifdef DISPLAY_BUS_8    
    //pokud je komunikace 8-bit, musi prohodit H a L byte, 16-bit je ok 
    wsbh    v0, a0
#else
    //vraci original
    move    v0, a0
#endif     
    
    jrc	    ra

.end correctColor16   

//</editor-fold>     
    

//<editor-fold defaultstate="collapsed" desc="drawLineQuick">
    /*
.align 2
.global	drawLineQuick
.ent drawLineQuick
drawLineQuick:
//a0=LINE_SRC struct
//a1=
//a2=color
//a3=fn
    
    //a0=LINE_SRC
    //a1=POINT_SRC struct
    //a2=hw buffer
    //a3=dc fn

    _PUSH_S07RA
    
    //a0 struct x1, y1, x2, y2, dispw, disph
    lh	    s0, 0(a0)
    lh	    s1, 2(a0)
    lh	    s2, 4(a0)
    lh	    s3, 6(a0)
    lh	    s4, 8(a0)
    lh	    s5, 10(a0)
    move    s7, zero			//poradi bodu x (nebo y)	    
    
    //res, r1, r2
    _ABS_DIFF t0, s0, s2		//meni t9, abs. hornota rozdilu r1-r2
    _ABS_DIFF t1, s1, s3		//meni t9
    
    _BAL_FBE t1, t0, calcx		//skok deltaY > deltaX

calcy:    
    //deltaX > deltaY, pocita Y
    _BAL_FLE s0, s2, 1f			//skok, x1 <= x2
    
    //prohodi x1/x2 a y1/y2
    move    t9, s0
    move    s0, s2
    move    s2, t9
    
    move    t9, s1
    move    s1, s3
    move    s3, t9    
   
1:    
    //vypocet R y2-y1 / x2-x1 (pomer stran)
    sub	    t8, s2, s0			//x2-x1
    sub	    t9, s3, s1			//y2-y1
    sll	    t9, 12			//nasobek * 4096
    
    div	    t9, t8
    mflo    s6				//s6=R
    nop
    b16	    3f
    nop
    
2:  //loop s0 ... s2
    addiu   s0, 1			//X++
    addiu   s7, 1			//point ++
3:    
    //vypocet Y pro akt. X (s0)
    mult    s6, s7
    mflo    a1				// r * point
    
    //vypocet Y se zaokrouhlenim
    srl	    a1, 11
    ext	    t9, a1, 0, 1
    srl	    a1, 1
    seh	    a1				//rozsireni znamenka   
    beqz    t9, 5f			
    nop
    addi    a1, 1
5:    
    
    add     a1, s1			//Y += Y1
    move    a0, s0			//x
    //test, zda neni mimo display
    bltz    a0, 4f			//x<0
    nop
    bltz    a1, 4f			//y<0
    nop
    _BAL_FBE a0, s4, 4f			//x>=w
    _BAL_FBE a1, s5, 4f			//y>=h
    
    bal	    drawPointQuick		//nemeni a2/a3
    nop
    
4:    
    bne	    s0, s2, 2b			//skok, akt. x neni posledni
    nop
    
    //konec 
    _POP_S07RA
    jrc	    ra
   
calcx:    
    //deltaY > deltaX, pocita X
    //deltaX > deltaY, pocita Y
    _BAL_FLE s1, s3, 1f			//skok, y1 <= y2
    
    //prohodi y1/y2 a x1/x2
    move    t9, s1
    move    s1, s3
    move    s3, t9   
    
    move    t9, s0
    move    s0, s2
    move    s2, t9

1:        
    //vypocet R x2-x1 / y2-y1
    sub	    t8, s3, s1			//y2-y1
    sub	    t9, s2, s0			//x2-x1
    sll	    t9, 12			//nasobek
    
    div	    t9, t8
    mflo    s6				//s6=R
    nop
    b16	    3f
    nop 
    
2:  //loop s1 ... s3
    addiu   s1, 1			//Y++
    addiu   s7, 1			//point ++
3:    
    //vypocet X pro akt. Y (s1)
    mult    s6, s7
    mflo    a0				// r * y
    
    //vypocet X se zaokrouhlenim
    srl	    a0, 11
    ext	    t9, a0, 0, 1
    srl	    a0, 1
    seh	    a0				//rozsireni znamenka   
    beqz    t9, 5f 
    nop
    addi    a0, 1
5:    
    
    add     a0, s0			//X += X1
    move    a1, s1			//Y
    //test, zda neni mimo display
    bltz    a0, 4f			//x<0
    nop
    bltz    a1, 4f			//y<0
    nop
    _BAL_FBE a0, s4, 4f			//x>=w
    _BAL_FBE a1, s5, 4f			//y>=h
    
    bal	    drawPointQuick		//nemeni a2/a3
    nop
    
4:    
    bne	    s1, s3, 2b
    nop
    
    //konec 
    _POP_S07RA
    jrc	    ra    

.end drawLineQuick  
    */
    
//</editor-fold>
 
//<editor-fold defaultstate="collapsed" desc="MACRO">
    /*
.macro _WRITE_COMMAND com, dc_fn
//com = command
//dc_fn = adresa fce setDcPin	
   
	
    //DC=0
    li	a0, 0x0			    
    jal	\dc_fn
    nop
	
    //odeslat hword command >> hw buffer
    sh	\com, (SPI2BUF)
    //pauza na odeslani
    nop
    nop
    nop
    nop
	
    //DC=1
    li	a0, 0x1			    
    jal	\dc_fn
    nop	
.endm  
    */
//</editor-fold>
//<editor-fold defaultstate="collapsed" desc="drawPointQuick">
    /*
.align 2    
.global drawPointQuick    
.ent drawPointQuick
drawPointQuick:
//a0 = x
//a1 = y
//a2 = color
//a3 = dc fn
    
//a0 = point struct
//a2 = buffer
//a3 = dc_fn    
    
    //nesmi menit a2 a a3
    
    _PUSH   ra
    addiu   sp, -4
    
    move    t0, a0
    move    t1, a1
    
    //column (x)
    li	    t2, 0x2A		    //write column
    _WRITE_COMMAND t2, a3
    sh	    t0, (SPI2BUF)	    //start_x
    sh	    t0, (SPI2BUF)	    //end_x
    
    //page (y)
    li	    t2, 0x2B		    //write page
    _WRITE_COMMAND t2, a3    
    sh	    t1, (SPI2BUF)	    //start_y
    sh	    t1, (SPI2BUF)	    //end_y

    li	    t2, 0x2C		    //write data
    _WRITE_COMMAND t2, a3    
    sh	    a2, (SPI2BUF)	    //color
    
    addiu   sp, 4
    _POP    ra
    jrc	    ra
    
.end drawPointQuick 
    */
    
//</editor-fold>
 
//<editor-fold defaultstate="collapsed" desc="MACRO">
.macro _ABS_DIFF result, reg1, reg2 
//meni t9
//v result vraci abs. hodnotu rozdilu reg1-reg2    
    
    sub	    \result, \reg1, \reg2
    
    bgez    \result, 1001f
    nop
    
    addiu   t9, zero, -1
    mult    \result, t9
    mflo    \result
    
1001:    
    
.endm
//</editor-fold>    
//<editor-fold defaultstate="collapsed" desc="drawLineQuick">
.align 2
.global	drawLineQuick
.ent drawLineQuick
drawLineQuick:
//a0=LINE_SRC
//a1=hw buffer
//a2=dc fn
//a3=
    
    //v sp vytvori misto pro POINT struct (short x, short y, short color)
    //sp[0] ra (PUSH s0-s7 + ra)
    //sp -= 8
    //sp[0] x
    //sp[2] y
    //sp[4] color

    _PUSH_S07RA
    addiu   sp, -8	    //misto pro POINT struct
    lhu	    t9, 8(a0)	    //load LINE_SRC.color
    sh	    t9, 4(sp)	    //save POINT.color (0(sp)=x, 2(sp)=y, 4[sp]=color)
    
    //a0 struct x1, y1, x2, y2, dispw, disph
    lh	    s0, 0(a0)		//x1
    lh	    s1, 2(a0)		//y1
    lh	    s2, 4(a0)		//x2
    lh	    s3, 6(a0)		//y2
    lh	    s4, 10(a0)		//w    
    lh	    s5, 12(a0)		//h
    move    s7, zero			//poradi bodu x (nebo y)	    
    
    //res, r1, r2
    _ABS_DIFF t8, s0, s2		//meni t9, abs. hornota rozdilu r1-r2
    _ABS_DIFF t7, s1, s3		//meni t9
    
    _BAL_FBE t7, t8, calcx2		//skok deltaY > deltaX

calcy2:    
    //deltaX > deltaY, pocita Y
    _BAL_FLE s0, s2, 1f			//skok, x1 <= x2
    
    //prohodi x1/x2 a y1/y2
    move    t9, s0
    move    s0, s2
    move    s2, t9
    
    move    t9, s1
    move    s1, s3
    move    s3, t9    
   
1:    
    //vypocet R y2-y1 / x2-x1 (pomer stran)
    sub	    t8, s2, s0			//x2-x1
    sub	    t9, s3, s1			//y2-y1
    sll	    t9, 12			//nasobek * 4096
    
    div	    t9, t8
    mflo    s6				//s6=R
    nop
    b16	    3f
    nop
    
2:  //loop s0 ... s2
    addiu   s0, 1			//X++
    addiu   s7, 1			//point ++
3:    
    //vypocet Y pro akt. X (s0)
    mult    s6, s7
    mflo    t1				// r * point
    
    //vypocet Y se zaokrouhlenim
    srl	    t1, 11
    ext	    t9, t1, 0, 1
    srl	    t1, 1
    seh	    t1				//rozsireni znamenka   
    beqz    t9, 5f			
    nop
    addi    t1, 1
5:    
    
    add     t1, s1			//Y += Y1
    //move    a0, s0			//x
    //test, zda neni mimo display
    bltz    s0, 4f			//x<0
    nop
    bltz    t1, 4f			//y<0
    nop
    _BAL_FBE s0, s4, 4f			//x>=w
    _BAL_FBE t1, s5, 4f			//y>=h
    
    //param
    sh	    s0, 0(sp)			//POINT.x
    sh	    t1, 2(sp)			//POINT.y
    move    a0, sp			//a0=&POINT struct
    _PUSH   a1				//hw buffer
    _PUSH   a2				//dc fn
    //a0=&POINT, a1=hw buffer, a2=dc fn
    bal	    drawPointQuick		//muze menit vse, mimo s0-s7
    nop
    _POP    a2
    _POP    a1
    
4:    
    bne	    s0, s2, 2b			//skok, akt. x neni posledni
    nop
    
    //konec 
    addiu   sp, 8	    //misto pro POINT struct
    _POP_S07RA
    jrc	    ra
   
calcx2:    
    //deltaY > deltaX, pocita X
    //deltaX > deltaY, pocita Y
    _BAL_FLE s1, s3, 1f			//skok, y1 <= y2
    
    //prohodi y1/y2 a x1/x2
    move    t9, s1
    move    s1, s3
    move    s3, t9   
    
    move    t9, s0
    move    s0, s2
    move    s2, t9

1:        
    //vypocet R x2-x1 / y2-y1
    sub	    t8, s3, s1			//y2-y1
    sub	    t9, s2, s0			//x2-x1
    sll	    t9, 12			//nasobek
    
    div	    t9, t8
    mflo    s6				//s6=R
    nop
    b16	    3f
    nop 
    
2:  //loop s1 ... s3
    addiu   s1, 1			//Y++
    addiu   s7, 1			//point ++
3:    
    //vypocet X pro akt. Y (s1)
    mult    s6, s7
    mflo    t0				// r * y
    
    //vypocet X se zaokrouhlenim
    srl	    t0, 11
    ext	    t9, t0, 0, 1
    srl	    t0, 1
    seh	    t0				//rozsireni znamenka   
    beqz    t9, 5f 
    nop
    addi    t0, 1
5:    
    
    add     t0, s0			//X += X1
    //move    a1, s1			//Y
    //test, zda neni mimo display
    bltz    t0, 4f			//x<0
    nop
    bltz    s1, 4f			//y<0
    nop
    _BAL_FBE t0, s4, 4f			//x>=w
    _BAL_FBE s1, s5, 4f			//y>=h
    
    //param
    sh	    t0, 0(sp)			//POINT.x
    sh	    s1, 2(sp)			//POINT.y
    move    a0, sp			//a0=&POINT struct
    _PUSH   a1				//hw buffer
    _PUSH   a2				//dc fn
    //a0=&POINT, a1=hw buffer, a2=dc fn
    bal	    drawPointQuick		//muze menit vse, mimo s0-s7
    nop
    _POP    a2
    _POP    a1
    
4:    
    bne	    s1, s3, 2b
    nop
    
    //konec 
    addiu   sp, 8	    //misto pro POINT struct
    _POP_S07RA
    jrc	    ra    

.end drawLineQuick   
    
//</editor-fold>    
    
//<editor-fold defaultstate="collapsed" desc="MACRO">
.macro _WRITE_COMMAND2 com, dc_fn, hw
//com = command
//dc_fn = adresa fce setDcPin	
//hw = hw buffer    
	
    //DC=0
    li16    a0, 0x0			    
    jal	    \dc_fn
    nop
	
    //odeslat hword command >> hw buffer
    sh	    \com, (\hw)
    //pauza na odeslani
    nop
    nop
    nop
    nop
	
    //DC=1
    li16    a0, 0x1			    
    jal	    \dc_fn
    nop	
.endm  
//</editor-fold>    
//<editor-fold defaultstate="collapsed" desc="drawPointQuick">
.align 2    
.global drawPointQuick    
.ent drawPointQuick
drawPointQuick:
//a0 = point struct (short x, short y, short color)
//a1 = hw buffer (SPIxBUF)
//a2 = dc_fn (adresa fce, ktera nastavi DC pin)   
    
    //volajici fce musi nastavit SPI 16-bit mode
    //protoze vola fci c/c++, zachova pouze s0-s7
    
    _PUSH_S03RA
    addiu   sp, -4		    //protoze vola c/c++ fci s jednim arg.

    //zaloha a0-a2, protoze vola c/c++ fci, ktera by je mohla zmenit
    move    s0, a0
    move    s1, a1
    move    s2, a2
    
    //column (x)
    li	    s3, 0x2A		    //write column
    _WRITE_COMMAND2 s3, s2, s1	    //command, dc fn, hw buffer, meni a0
    lhu	    t9, 0(s0)		    //load x
    sh	    t9, (s1)		    //start_x >> hw buffer
    sh	    t9, (s1)		    //end_x >> hw buffer
    
    //page (y)
    li	    s3, 0x2B		    //write page
    _WRITE_COMMAND2 s3, s2, s1	    //command, dc fn, hw buffer, meni a0
    lhu	    t9, 2(s0)		    //load y
    sh	    t9, (s1)		    //start_y >> hw buffer
    sh	    t9, (s1)		    //end_y >> hw buffer

    li	    s3, 0x2C		    //write data
    _WRITE_COMMAND2 s3, s2, s1	    //command, dc fn, hw buffer, meni a0
    lhu	    t9, 4(s0)		    //load color
    sh	    t9, (s1)		    //color >> hw buffer
    
    addiu   sp, 4		    //protoze vola c/c++ fci s jednim arg.
    _POP_S03RA    
    jrc	    ra
    
.end drawPointQuick 
    
//</editor-fold>    