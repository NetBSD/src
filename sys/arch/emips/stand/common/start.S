/*	$NetBSD: start.S,v 1.2.16.1 2014/08/20 00:02:52 tls Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

	/* Trivial support for printing stuff on the serial line from C pgms.
     */
#include <mips/asm.h>
#include <mips/cpuregs.h>
#define __ASSEMBLER__ 1
#include <machine/emipsreg.h>

/* Offsets in the CXTINFO structure
 */
#define TS_AT (1 * 4)
#define TS_V0 (2 * 4)
#define TS_V1 (3 * 4)
#define TS_A0 (4 * 4)
#define TS_A1 (5 * 4)
#define TS_A2 (6 * 4)
#define TS_A3 (7 * 4)
#define TS_T0 (8 * 4)
#define TS_T1 (9 * 4)
#define TS_T2 (10 * 4)
#define TS_T3 (11 * 4)
#define TS_T4 (12 * 4)
#define TS_T5 (13 * 4)
#define TS_T6 (14 * 4)
#define TS_T7 (15 * 4)
#define TS_S0 (16 * 4)
#define TS_S1 (17 * 4)
#define TS_S2 (18 * 4)
#define TS_S3 (19 * 4)
#define TS_S4 (20 * 4)
#define TS_S5 (21 * 4)
#define TS_S6 (22 * 4)
#define TS_S7 (23 * 4)
#define TS_T8 (24 * 4)
#define TS_T9 (25 * 4)
#define TS_K0 (26 * 4)
#define TS_K1 (27 * 4)
#define TS_GP (28 * 4)
#define TS_SP (29 * 4)
#define TS_FP (30 * 4)
#define fp s8
#define TS_RA (31 * 4)

#define TS_PC (32 * 4)
#define TS_SR (33 * 4)
#define TS_HI (34 * 4)
#define TS_LO (35 * 4)
#define TS_EC (36 * 4)
#define SIZEOF_CXTINFO (37*4)

/* PROM_MODE means the user plans to keep this code around while running an OS.
 * So we act kind of like PROM code (BIOS?), but we live in RAM.
 * So we need to safeguard ourselves against corruptions, some unavoidable.
 * Like the overriding of the exception vectors, rigth where our "start" code is.
 */

        IMPORT(main,4)
        IMPORT(_end,4)

        .set noreorder

EXPORT(start)
    bgezal zero,_C_LABEL(real_start)
    nop


/* Does not handle the exception, really.
 * But to test interrupts should be enough
 */
     .org 0x00000080
NESTED_NOPROFILE(ExceptionHandler,SIZEOF_CXTINFO,$31)
    la     k1, UserInterruptHandler
    lw     k1,0(k1)
    bne    k1,zero,Dispatch
    mfc0   k0, MIPS_COP_0_EXC_PC
    j      k0
    nop /* do not! pop status */

EXPORT(UserInterruptHandler)
    .word 0

EXPORT(Dispatch)
	/* Save state on stack */
	addiu   sp, sp, -SIZEOF_CXTINFO
	/* save registers */
    .set noat
	sw      AT, TS_AT(sp)
    .set at
	sw      v0, TS_V0(sp)
	sw      v1, TS_V1(sp)
	sw      a0, TS_A0(sp)
	sw      a1, TS_A1(sp)
	sw      a2, TS_A2(sp)
	sw      a3, TS_A3(sp)
	sw      t0, TS_T0(sp)
	sw      t1, TS_T1(sp)
	sw      t2, TS_T2(sp)
	sw      t3, TS_T3(sp)
	sw      t4, TS_T4(sp)
	sw      t5, TS_T5(sp)
	sw      t6, TS_T6(sp)
	sw      t7, TS_T7(sp)
	sw      s0, TS_S0(sp)
	sw      s1, TS_S1(sp)
	sw      s2, TS_S2(sp)
	sw      s3, TS_S3(sp)
	sw      s4, TS_S4(sp)
	sw      s5, TS_S5(sp)
	sw      s6, TS_S6(sp)
	sw      s7, TS_S7(sp)
	sw      t8, TS_T8(sp)
	sw      t9, TS_T9(sp)
	sw      k0, TS_K0(sp)
	sw      k1, TS_K1(sp)
	sw      gp, TS_GP(sp)
	/* sp: later */
	sw      fp, TS_FP(sp)
	sw      ra, TS_RA(sp)

    mfc0    a0, MIPS_COP_0_STATUS
    mflo    t0
    mfhi    t1
	sw      a0, TS_SR(sp)
	sw      t0, TS_LO(sp)
	sw      t1, TS_HI(sp)
	sw      k0, TS_PC(sp)

	/* Save original stack */
    move    a0,sp
	addiu   t0, sp, SIZEOF_CXTINFO
    jalr    k1
	sw      t0, TS_SP(sp)
    
    /* Returned value is new PCXINFO */
    move    a0,v0

	/* First load most registers */
    .set noat
	lw       AT, TS_AT(a0)
	lw       v0, TS_V0(a0)
	lw       v1, TS_V1(a0)
	/* a0 later */
	lw       a1, TS_A1(a0)
	lw       a2, TS_A2(a0)
	lw       a3, TS_A3(a0)
	lw       t0, TS_T0(a0)
	lw       t1, TS_T1(a0)
	lw       t2, TS_T2(a0)
	lw       t3, TS_T3(a0)
	lw       t4, TS_T4(a0)
	lw       t5, TS_T5(a0)
	lw       t6, TS_T6(a0)
	lw       t7, TS_T7(a0)
	lw       s0, TS_S0(a0)
	lw       s1, TS_S1(a0)
	lw       s2, TS_S2(a0)
	lw       s3, TS_S3(a0)
	lw       s4, TS_S4(a0)
	lw       s5, TS_S5(a0)
	lw       s6, TS_S6(a0)
	lw       s7, TS_S7(a0)
	lw       t8, TS_T8(a0)
	lw       t9, TS_T9(a0)
    /* k0,k1 not restored */
	lw       gp, TS_GP(a0)
	/* sp later */
	lw       fp, TS_FP(a0)
	lw       ra, TS_RA(a0)

    lw       k1, TS_HI(a0)
    lw       k0, TS_LO(a0)
    mthi     k1
    mtlo     k0
    lw       k1, TS_SR(a0)
    mtc0     k1, MIPS_COP_0_STATUS
     /* NB: After this instruction we cannot take any interrupts or traps
      */
	lw	sp, TS_SP(a0)

	/* Put pc into k0 */
	lw	k0, TS_PC(a0)
	lw	a0, TS_A0(a0)
	j	k0
    rfe
    .set at

END(ExceptionHandler)

     .org 0x00000200
EXPORT(real_start)
	.ent _C_LABEL(real_start)

#ifdef SECONDARY_BOOTBLOCK
    /*
     * If this is the program that goes into FLASH we must copy ourselves down to RAM.
     * FLASH default on the MLx is at 0xf0000000, DRAM at 0. 
     */        
    addi    a0,ra,-8         /* Compensate for the first two instructions */

    /* Get the address(relative) of TextStart
     */
    bgezal  zero, _C_LABEL(MipsStart2) /* Always jumps */
    nop
        
    /* All of the static data, since we are at it.
     */
TextStart:                                /* + 0 */
    /* Text start at final link address */
    .int    start

DataEnd:                                  /* + 4 */
    /* Data end == bss start */
    .int    _edata

BssEnd:                                   /* + 8 */
    /* Bss end */
    .int    _end

RelocToRAM:                               /* *+12 */
    .int    InRAM

MipsStart2:       

    /* Source = a0, Dst = t2 */
    lw      t2, 0(ra)     /* _C_LABEL(TextStart) */

    /* EndPtr = t3 */
     /* in bdelay slot */

    /* If a0 != t2 then we are running in Flash but should run in RAM
     * In that case copy .text. Otherwise skip to .bss.
     */
    beq     a0,t2,ZroLoop-4
    lw      t3, 4(ra)    /* _C_LABEL(DataEnd)   */

CpyLoop:
    /* loop copying 2 words at a time */
    lw      t4,0(a0)
    lw      t5,4(a0)
    addiu   a0,a0,8
    sw      t4,0(t2)
    addiu   t2,t2,8
    sltu    t1,t2,t3
    bne     t1,zero,CpyLoop
    sw      t5,-4(t2)

    /* zero the bss
     */
    lw      t4, 8(ra)   /* _C_LABEL(BssEnd)  */
ZroLoop:
    sltu    t1,t3,t4
    sw      zero,0(t3)
    bne     t1,zero,ZroLoop
    addiu   t3,t3,4

    /* Jump to RAM copy (below)
     */
    lw      t1, 12(ra)   /* _C_LABEL(RelocToRAM) */
    jr      t1
    nop

    /*
     * Execute from here after copying out of FLASH into RAM
     */
InRAM:

#endif /*  SECONDARY_BOOTBLOCK */
	
    /* Get a stack
     */
#ifdef __GP_SUPPORT__
    la      gp, _C_LABEL (_gp)
#endif
    la    sp,_end
	addiu sp,sp,(8*1024)          /* BUGBUG arbitrary */

    /* Jump to main
     */
    jal   main
    add   a0,sp,zero

    /* Load failed, reset the processor and jump back to the origins.
     */
EXPORT(_rtt)    /* ahem */
    li     t0,0x1260ff80  /* NB: On new builds this is a SYS-RESET as well */
    mtc0   t0,MIPS_COP_0_STATUS

    lui    t0,(BRAM_DEFAULT_ADDRESS>>16) /* nb: knows about 16bit chop */
	jr     t0
    nop

EXPORT(Stop)
	b     Stop
    nop

END(real_start)

        .set noreorder
        .set noat
        .set nomacro

/* void Delay(UINT32 count)
 */
LEAF(Delay)
    bne    a0,zero,_C_LABEL(Delay)
    subu   a0,1
    j      ra
    nop
END(Delay)

/* UINT32 GetPsr(void)
 * Returns the PSR (coprocessor 0 status)
 */
LEAF(GetPsr)
    mfc0   v0, MIPS_COP_0_STATUS
    j      ra
    nop
END(GetPsr)

/* void SetPsr(UINT32 Psr)
 * Sets the PSR (coprocessor 0 status)
 */
LEAF(SetPsr)
    mtc0   a0,MIPS_COP_0_STATUS
    j      ra
    nop
END(SetPsr)

/* UINT32 GetCause(void)
 * Returns the Cause register (coprocessor 0)
 */
LEAF(GetCause)
    mfc0   v0,MIPS_COP_0_CAUSE
    j      ra
    nop
END(GetCause)

/* UINT32 GetEpc(void)
 * Returns the Epc register (coprocessor 0)
 */
LEAF(GetEpc)
    mfc0   v0,MIPS_COP_0_EXC_PC
    j      ra
    nop
END(GetEpc)


/* int PutWord(UINT32 Word);
 * Returns: 0 if ok, -1 otherwise
 */
NESTED(PutWord,12,$31)
    subu   sp,sp,12
    sw     s0,8(sp)
    sw     s1,4(sp)
    sw     ra,0(sp)

    or     s1,a0,zero
    /* Spit all nibbles
     */
    li     s0,8
PutWordLoop:
    srl    a0,s1,32-4
    li     t0,10
    sltu   t1,a0,t0
    bnez   t1,$Digit
    li     a1,'0'
    subu   a0,a0,t0
    li     a1,'a'
$Digit:
    sll    s1,s1,4
    jal    PutChar
    add    a0,a0,a1

    subu   s0,s0,1
    bne    v0,zero,PutWordDone /* printed ok? */
    li     v0,-1

    /* done yet? */
    bne    s0,zero,PutWordLoop
    nop

    /* done
     */
    li     v0,0
PutWordDone:
    lw     ra,0(sp)
    lw     s1,4(sp)
    lw     s0,8(sp)
    jr     ra
    addiu  sp,sp,12

END(PutWord)

/* int Puts(char *String);
 * Returns: 0 if ok, -1 otherwise
 */
NESTED(Puts,8,$31)
    subu   sp,sp,8
    sw     s0,4(sp)
    sw     ra,0(sp)

    or     s0,a0,zero
    /* Spit all chars until zero
     */
PutsLoop:
    lbu    a0,0(s0)
    addiu  s0,s0,1
    beq    a0,zero,PutsDoneOk
    nop
    jal    PutChar
    nop
    beq    v0,zero,PutsLoop
    nop

    /* Timed out
     */
    b      PutsDone
    li     v0,-1

    /* done
     */
PutsDoneOk:
    li     v0,0
PutsDone:
    lw     ra,0(sp)
    lw     s0,4(sp)
    jr     ra
    addiu  sp,sp,8

END(Puts)


/* int GetChar(void);
 * Returns: a non-negative value if ok, -1 otherwise
 */
LEAF(GetChar)
    lui    t0,(USART_DEFAULT_ADDRESS>>16) /* nb: knows about 16bit chop */
    lui    t1,1000          /* n*65k spins max */
RxNotReady:
    lw     t4,USARTST(t0)       /* ChannelStatus */
    andi   t4,t4,USI_RXRDY
    bgtz   t4,$GotByte
    subu   t1,t1,1
    /* still ok to spin? */
    bgtz   t1,RxNotReady
    nop
    /* Timed out
     */
    jr     ra
    li     v0,-1

    /* Gottabyte
     */
$GotByte:        
    lw     v0,USARTRX(t0)        /* RxData */
    jr     ra
    andi   v0,0xff
END(GetChar)

/* int PutChar(UINT8 v);
 * Returns: 0 if ok, -1 otherwise
 */
LEAF(PutChar)
    lui    t0,(USART_DEFAULT_ADDRESS>>16) /* nb: knows about 16bit chop */
    lui    t1,1000          /* n*65k spins max */
    li     v0,0
TxNotReady:
    lw     t4,USARTST(t0)       /* ChannelStatus */
    andi   t4,t4,USI_TXRDY
    bgtz   t4,TxReady
    subu   t1,t1,1
    /* still ok to spin? */
    bgtz   t1,TxNotReady
    nop
    /* Timed out
     */
    jr     ra
    li     v0,-1

    /* Send it
     */
TxReady:
    jr     ra
    sw     a0,USARTTX(t0)

END(PutChar)

/* Second arg is a function to call with the first arg:
 * void switch_stack_and_call(void *arg, void (*function)(void *));
 */
LEAF(switch_stack_and_call)
    /* Get a stack and jump. It would be a very bad idea to return but..
     */
    lui   sp,%hi(_end)
    addiu sp,%lo(_end)
    jr    a1
	addiu sp,sp,(2*1024)          /* BUGBUG arbitrary */

END(switch_stack_and_call)

