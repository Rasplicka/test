#include "xc.h"
#include "def.h"    
#include "asm_macro.S"    


//<editor-fold defaultstate="collapsed" desc="macro">

    //<editor-fold defaultstate="collapsed" desc="_TEST_MULT">
.macro	_TEST_MULT mult_bigger, mult_lower, mult_equal 
    mfhi    t9
    bltz    t9, \mult_bigger		//skok t9<0 - zaporny vysledek
    //ext	    t8, t9, 31, 1
    //bnez    t8, \mult_bigger
    nop
    
    //je kladne nebo 0
    bnez    t9, \mult_lower
    nop
    
    mflo    t8
    bnez    t8, \mult_lower
    nop   
    
    nop
    b16	    \mult_equal
    nop
.endm 
    
    

    
    //</editor-fold>
    
    //<editor-fold defaultstate="collapsed" desc="_REKFN">
.macro _REKFN rek_this, rek_next, cont, before_cont

\rek_this:
//a2=c, t8=v1, t9=z1, a3 zbytek (nastavi posledni rekurzivni fce)
//a1=a(stack), a0=b 
//A=a1 b=a1, C=a2	

	//<editor-fold defaultstate="collapsed" desc="ALGORITMUS">
	/*
        void del2(uint a, uint b, uint c, ref ulong v, ref ulong z, ref int deep)
        {
            deep++;
            uint f = 0xFFFFFFFF;

            uint v1 = f / c;
            uint z1 = (f % c) + 1;

            ulong v1a = (ulong)v1 * (ulong)a;
            ulong z1a = (ulong)z1 * (ulong)a;

            ulong v2 = 0;
            uint z1a_l = 0;
            uint z1a_h = 0;

            z1a_l = (uint)z1a;
            z1a_h = (uint)(z1a >> 32);
            sum64(ref z1a_l, ref z1a_h, b);

            if (z1a_h > 0)
            {
                //z1a+b 64-bit
                del2(z1a_h, z1a_l, c, ref v2, ref z, ref deep);
            }
            else
            {
                //z1a+b 32-bit
                v2 = (z1a_l) / c;
                z = (z1a_l) % c;
            }

            v = v1a + v2;

        }
	*/
	//</editor-fold>

    
    //B do acc
    mthi    zero
    mtlo    a0
    
    //Z1 x A + B
    maddu   a1, t9			//acc(B) += A x Z1
    //acc=vysledek
    
    mfhi    v1				//load HI
    beqz    v1, 1001f			//skok, pokud HI=0, vysledek Z1A je 32-bit (z1a_l)
    nop
    
    //64-bit, volani subfn
    _PUSH   a1
    
    mfhi    a1
    mflo    a0
    nop
    b16	    \rek_next			//skok do dalsi rek. fce, ktera dela u64(a1,a0)/u32(a2)
    nop

\cont:
    //vysledek v2 je v acc(HI/LO)
    _POP    a1
    maddu   t8, a1			//acc += A(a1) x V1(t8)
    
    //vysledek (V) je v acc
    nop
    b16	    \before_cont		//navrat do predchozi rek. fce
    nop
    
1001:     
    //32-bit, muze delit primo bez volani rek. fce
    mflo    v1			    //load z1a_l (32-bit), muze provest deleni (/C)
    divu    v1, a2		    //z1a_l / C
    mfhi    a3			    //C, zbytek do a3 (jiz se nemeni, vysledny zbytek)   
    
    //do acc.HI zero, v acc.LO je V2
    mthi    zero
   
    //V = V1a + V2
    maddu   t8, a1		    //V1A = V1 x A + V2(je v acc)
    
    //vysledek (V) je v acc
    nop
    b16	    \before_cont	    //navrat do predchozi rek. fce
    nop
    
.endm
    //</editor-fold>    
    
//</editor-fold>

.section .asm_math, code 
    
//<editor-fold defaultstate="collapsed" desc="math.add_64">
.align 2    
.global	math.add_64    
.ent math.add_64    
math.add_64:
//a0(L)-a1(H)
//a2(L)-a3(H)    
    
//soucet dvou 64-bit cisel, se znamenkem (overflow zpusobi exception)    
//vysledek vraci v0(L) v1(H)    
    
    addu    v0, a0, a2
    sltu    t9, v0, a0
    //sltu    t8, v0, a2
    //or	    t8, t9
    
    beqz    t9, 1f
    nop
    
    addi    a1, 1
    
1:
    add	    v1, a1, a3
    
    jrc	    ra
    
.end math.add_64  
//</editor-fold>
  
//<editor-fold defaultstate="collapsed" desc="math.sub_64">
.align 2    
.global math.sub_64    
.ent math.sub_64    
math.sub_64:    
//a0(L)-a1(H)
//a2(L)-a3(H)
//odecteni 64-bit cisel a01 - a23, cisla se znamenkem (overflow zpusobi exception)     
    
    subu    v0, a0, a2
    sltu    t9, a0, v0
    
    beqz    t9, 1f
    nop
    
    //addi    a3, 1
    addi    a1, -1	
1:
    sub	    v1, a1, a3
    
    jrc	    ra
    
.end math.sub_64 
//</editor-fold>

//<editor-fold defaultstate="collapsed" desc="math.div_64_sign">
.align 2    
.global math.div_64_sign    
.ent math.div_64_sign
math.div_64_sign:
//a0(L)1(H)	64-bit signed
//a2		32-bit signed
//a3		32-bit signed zbytek po deleni
//v0(L)1(H)	64-bit signed vysledek
    
//meni a0, a1, a2, a3, t1, v0, v1 (volana fce t8, t9)    
//vyuziti stacku viz. div_64     
    
    _PUSH   ra
    
    //t0 vysledne znamenko
    move    t0, zero

	//<editor-fold defaultstate="collapsed" desc="zmena polarity a01">
    //test polarity a01
    ext	    v0, a1, 31, 1
    beqz    v0, 1f
    nop
    
    //a01 je zaporne
    addiu   t0, 1
    
    not	    a1
    not	    a0
    
    addiu   a0, 1
    bnez    a0, 1f			//skok, pokud nedoslo k preteceni a0
    nop
    
    //preteceni a0, a1++
    addiu   a1, 1
    
1:  //ok, a01=abs(a01)    
    
	//</editor-fold>

	//<editor-fold defaultstate="collapsed" desc="zmena polarity a2">
    //test polarity a2
    ext	    v0, a2, 31,1
    beqz    v0, 2f
    nop
  
    //a2 je zaporne
    addiu   t0, 1
    
    not	    a2
    addiu   a2, 1
    
2:   
	//</editor-fold>
 
    //ok, a01 i a2 jsou kladna uint64 a uint32
    //t0.b0 = znamenko vysledku (0=kladny, 1=zaporny)    
    andi    t0, 0x01			//ponechat pouze b0
    
    bal	    math.div_64
    nop
    
    beqz    t0, 9f			//skok, pokud nemeni znamenko vysledku
    nop
    
	//<editor-fold defaultstate="collapsed" desc="zmena znamenka">
    //znema znamenka vysledku
    beqz    v0, 3f			//v0=0, musi odecist 1 od v1
    nop
    addiu   v0, -1
    not	    v0
    
    nop
    b16	    4f
    nop
    
3:
    //v0=0
    beqz    v1, 9f			//v0 i v1 = 0, vraci 0 (zadna zmena v0, v1)
    nop

    //v1!=0
    not	    v0
    addiu   v1, -1
    
4:    
    not	    v1
	//</editor-fold>

9:
    _POP    ra
    jrc	    ra
    
.end math.div_64_sign 
//</editor-fold>
    
//<editor-fold defaultstate="collapsed" desc="math.div_64">
.align 2    
.global	math.div_64
.ent math.div_64    
math.div_64:
//a0=lo (B), a1=hi (A)	    (uint64)
//a2=C			    (uint32)
//a3=vraci zbytek	    (uint32)
//v1/v0 = vysledek 64-bit   (uint64)
    
//meni a0, a1, a3, t8, t9, v0, v1
//na zasobnik uklada mezivysledky, pouzije max. 80 bytes zasobniku (20x4)
    
//a1/a0 i a2 = unsigned v plnem rozsahu, vysledek unsigned
    
//pokud ma pracovat se signed, musi predchazet fce, ktera prevede zaporna cisla
//na kladna a po vypoctu upravi znamenko
    
//pokud pocita v plnem rozsahu (0xFFFFFFFF FFFFFFFF), je max. pocet rek. volani 32 
//spotrebuje 32x4bytes na zasobniku
    
//pokud pocita v rozsahu po prevodu signed (0x7FFFFFFF FFFFFFFF) je max. pocet rek. volani 20
//spotrebuje 20x4bytes na zasobniku   
    
//32x volani rek. fce pri 0xFFFFFFFF FFFFFFFF / 0x80000000
//20x volani rek. fce pri 0x7FFFFFFF FFFFFFFF / 0x5555574D    

//v0,v1 64-bit vysledek  - a01(64-bit uint) / a2 (32-bit uint)   
    
    //<editor-fold defaultstate="collapsed" desc="algoritmus">
    /*
	F=0xFFFFFFFF
	V1=F/C
	Z1=F%C + 1
	
	V1A=V1 x A
	Z1A=Z1 x A
	
	V2=(Z1A + B) / C	//POKUD Z1A+B je 64-bit, vola se fce rekurzivne, dokud Z1A+B neni 32-bit
	Z= (Z1A + B) % C
	
	V=V1A + V2
	*/
	//</editor-fold>

    move    v0, zero
    not	    v0			    //v0=0xFFFFFFFF	
    divu    t8, v0, a2		    //t8=V1 (0xFFFFFFFF/C)
    mfhi    t9			    //t9=Z1 (0xFFFFFFFF MOD C)
    addiu   t9, 1		    //Z1+1
    
    //B do acc
    mthi    zero
    mtlo    a0			    //B(a0)
    
    //Z1 x A + B
    maddu   a1, t9		    //acc(B) += A x Z1
    //acc=vysledek
    
    mfhi    v1			    //load HI
    beqz    v1, 1f		    //skok, pokud HI=0, vysledek Z1A je 32-bit (Z1A_L)
    nop    
    
    //64-bit, volani subfn
    _PUSH   a1
    
    mfhi    a1
    mflo    a0
    nop
    b16	    rek1		    //skok do rekurzivni fce, ktera dela u64(a1,a0)/u32(a2)
    nop
    
rek0_continue:
    //vysledek v2 je v acc(HI/LO)
    _POP    a1
    maddu   t8, a1		    //acc += A(a1) x V1(t8)
    
    //vysledek (V) je v acc, zbytek Z v a3
    mfhi    v1
    mflo    v0

    jrc	    ra  

1:  //32-bit v acc (muze delit primo DIVU)
    mflo    v1
    divu    v1, a2		    //Z1A_L / C
    mfhi    a3			    //Z, zbytek   
    
    //do acc.HI zero, v acc.LO je V2
    mthi    zero
   
    //V=V1A+V2
    maddu   t8, a1		    //V = V1 x A + V2(je v acc)
    
    //vysledek (V) je v acc, zbytek Z v a3
    mfhi    v1
    mflo    v0

    jrc	    ra

    //32x rekurzivni fce
    _REKFN rek1, rek2, rek1_continue, rek0_continue    
    _REKFN rek2, rek3, rek2_continue, rek1_continue     
    _REKFN rek3, rek4, rek3_continue, rek2_continue     
    _REKFN rek4, rek5, rek4_continue, rek3_continue    
    _REKFN rek5, rek6, rek5_continue, rek4_continue     
    _REKFN rek6, rek7, rek6_continue, rek5_continue    
    _REKFN rek7, rek8, rek7_continue, rek6_continue    
    _REKFN rek8, rek9, rek8_continue, rek7_continue     
    _REKFN rek9, rek10, rek9_continue, rek8_continue
    _REKFN rek10, rek11, rek10_continue, rek9_continue    
    _REKFN rek11, rek12, rek11_continue, rek10_continue     
    _REKFN rek12, rek13, rek12_continue, rek11_continue
    _REKFN rek13, rek14, rek13_continue, rek12_continue    
    _REKFN rek14, rek15, rek14_continue, rek13_continue     
    _REKFN rek15, rek16, rek15_continue, rek14_continue    
    _REKFN rek16, rek17, rek16_continue, rek15_continue    
    _REKFN rek17, rek18, rek17_continue, rek16_continue     
    _REKFN rek18, rek19, rek18_continue, rek17_continue
    _REKFN rek19, rek20, rek19_continue, rek18_continue    
    _REKFN rek20, rek21, rek20_continue, rek19_continue     
    _REKFN rek21, rek22, rek21_continue, rek20_continue
    _REKFN rek22, rek23, rek22_continue, rek21_continue    
    _REKFN rek23, rek24, rek23_continue, rek22_continue     
    _REKFN rek24, rek25, rek24_continue, rek23_continue 
    _REKFN rek25, rek26, rek25_continue, rek24_continue     
    _REKFN rek26, rek27, rek26_continue, rek25_continue 
    _REKFN rek27, rek28, rek27_continue, rek26_continue 
    _REKFN rek28, rek29, rek28_continue, rek27_continue 
    _REKFN rek29, rek30, rek29_continue, rek28_continue 
    _REKFN rek30, rek31, rek30_continue, rek29_continue 
    _REKFN rek31, rek32, rek31_continue, rek30_continue 
    _REKFN rek32, rek33, rek32_continue, rek31_continue 
    
rek33: 
    //overflow (pocet vnoreni vic nez 24)
    li	    v0, 0x7FFFFFFF
    addi    v0, 0xF
    nop
    nop
    
.end math.div_64
//</editor-fold>  

    
     