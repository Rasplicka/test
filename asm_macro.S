

//<editor-fold defaultstate="collapsed" desc="STACK POP/PUSH">

.macro _PUSH reg
    addiu   sp, sp, -4
    sw      \reg, 0(sp)
.endm

.macro _POP reg
    lw      \reg, 0(sp)
    addiu   sp, sp, 4
.endm
	
.macro _PUSH_S03RA
    addiu   sp, -20
    swm32   s0 - s3, ra, (sp)
.endm
	
.macro _POP_S03RA
    lwm32   s0 - s3, ra, (sp)
    addiu   sp, 20
.endm
  
.macro _PUSH_S04RA
    addiu   sp, -24
    swm32   s0 - s4, ra, (sp)
.endm
	
.macro _POP_S04RA
    lwm32   s0 - s4, ra, (sp)
    addiu   sp, 24
.endm    
    
.macro _PUSH_S07RA
    addiu   sp, -36
    swm32   s0 - s7, ra, (sp)
.endm
	
.macro _POP_S07RA
    lwm32   s0 - s7, ra, (sp)
    addiu   sp, 36
.endm      
    
    
//</editor-fold>

	
//<editor-fold defaultstate="collapsed" desc="podminene skoky BAL_xxx">
.macro _BAL_FBT first_reg, val_reg, addr
//skok, first > val
//meni t9
    
    subu    t9, \first_reg, \val_reg
    bgtz    t9, \addr
    nop
    
.endm    
    
.macro _BAL_FBTC first_reg, val_const, addr
//skok, first > val_const
//meni t9
    
    li	    t9, \val_const
    subu    t9, \first_reg, t9
    bgtz    t9, \addr
    nop
    
.endm  
    
.macro _BAL_FBE first_reg, val_reg, addr
//skok, first >= val_reg
//meni t9

    subu    t9, \first_reg, \val_reg
    bgez    t9, \addr
    nop
    
.endm     
    
.macro _BAL_FBEC first_reg, val_const, addr
//skok, first >= val_const
//meni t9
    
    li	    t9, \val_const
    subu    t9, \first_reg, t9
    bgez    t9, \addr
    nop
    
.endm      
    
.macro _BAL_FLT first_reg, val_reg, addr
//skok, first < val
//meni t9
    
    subu    t9, \first_reg, \val_reg
    bltz    t9, \addr
    nop
    
.endm  
    
.macro _BAL_FLTC first_reg, val_const, addr
//skok, first < val_const
//meni t9
    
    li	    t9, \val_const
    subu    t9, \first_reg, t9
    bltz    t9, \addr
    nop
    
.endm     
    
.macro _BAL_FLE first_reg, val_reg, addr
//skok, first <= val_reg
//meni t9

    subu    t9, \first_reg, \val_reg
    blez    t9, \addr
    nop
    
.endm 
    
.macro _BAL_FLEC reg, const, addr
    
    li	    t9, \const
    subu    t9, \reg, t9 
    blez    t9, \addr
    nop
    
.endm    
    
.macro _BAL_EQC	reg, const, addr    
//skok, reg = const    
//meni t9
    
    li	    t9, \const
    beq	    \reg, t9, \addr
    nop
    
.endm
    
.macro _BAL_NEQC reg, const, addr    
//skok, reg != const    
//meni t9
    
    li	    t9, \const
    bne	    \reg, t9, \addr
    nop
    
.endm    
    
   
.macro _SET_IF_EQUI reg, val, set_reg, set_result
//pokud reg obsahuje hodnotu val, nastavi do set_reg hodnotu set_result    
//_SET_IF_EQUI v1, 8, v0, 0
    
    li	    t9, \val
    bne	    \reg, \val, 1001f
    nop
    
    //reg=val, nastav set_reg
    li	    \set_reg, \set_result
    
1001:    
    
.endm    
   
.macro _SET_B_IF_EQUI reg, val, set_reg, set_result, branch
//pokud reg obsahuje hodnotu val, nastavi do set_reg hodnotu set_result    
//_SET_IF_EQUI v1, 8, v0, 0
    
    li	    t9, \val
    bne	    \reg, \val, 1001f
    nop
    
    //reg=val, nastav set_reg
    li	    \set_reg, \set_result
    nop
    b16	    \branch
    nop
    
1001:    
    
.endm      
    
    
//</editor-fold>    	
    
    
    