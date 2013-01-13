/*
 * Copyright (c) 1992 OMRON Corporation.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)locore.s	8.1 (Berkeley) 6/10/93
 */
/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)locore.s	8.1 (Berkeley) 6/10/93
 */

/* For _C_LABEL() and friends. */
#include <m68k/asm.h>

#define	T_BUSERR	0
#define	T_ADDRERR	1
#define	T_ILLINST	2
#define	T_ZERODIV	3
#define	T_CHKINST	4
#define	T_TRAPVINST	5
#define	T_PRIVINST	6
#define	T_MMUFLT	8
#define	T_FMTERR	10
#define	T_FPERR		11
#define	T_COPERR	12

#define	PSL_LOWIPL	8192
#define	PSL_HIGHIPL	9984

#define	SPL1		8448
#define	SPL2		8704
#define	SPL3		8960
#define	SPL4		9216
#define	SPL5		9472
#define	SPL6		9728

#define	CLOCK_REG	1660944384
#define	CLK_CLR		1

#define	ILLGINST	16
#define	NMIVEC		124
#define	EVTRAPF		188

	.text

ASENTRY_NOPROFILE(start)
ASGLOBAL(Reset)
	jmp _C_LABEL(start1)	/* 0: NOT USED (reset PC) */
	.word	0		/* 1: NOT USED (reset PC) */
	VECTOR(buserr)		/* 2: bus error */
	VECTOR(addrerr)		/* 3: address error */
	VECTOR(illinst)		/* 4: illegal instruction */
	VECTOR(zerodiv)		/* 5: zero divide */
	VECTOR(chkinst)		/* 6: CHK instruction */
	VECTOR(trapvinst)	/* 7: TRAPV instruction */
	VECTOR(privinst)	/* 8: privilege violation */
	VECTOR(badtrap)		/* 9: trace */
	VECTOR(illinst)		/* 10: line 1010 emulator */
	VECTOR(illinst)		/* 11: line 1111 emulator */
	VECTOR(badtrap)		/* 12: unassigned, reserved */
	VECTOR(coperr)		/* 13: coprocessor protocol violation */
	VECTOR(fmterr)		/* 14: format error */
	VECTOR(badtrap)		/* 15: uninitialized interrupt vector */
	VECTOR(badtrap)		/* 16: unassigned, reserved */
	VECTOR(badtrap)		/* 17: unassigned, reserved */
	VECTOR(badtrap)		/* 18: unassigned, reserved */
	VECTOR(badtrap)		/* 19: unassigned, reserved */
	VECTOR(badtrap)		/* 20: unassigned, reserved */
	VECTOR(badtrap)		/* 21: unassigned, reserved */
	VECTOR(badtrap)		/* 22: unassigned, reserved */
	VECTOR(badtrap)		/* 23: unassigned, reserved */
	VECTOR(badtrap)		/* 24: unassigned, reserved */
	VECTOR(badtrap)		/* 25: unassigned, reserved */
	VECTOR(lev2intr)	/* 26: level 2 interrupt autovector */
	VECTOR(lev3intr)	/* 27: level 3 interrupt autovector */
	VECTOR(badtrap)		/* 28: level 4 interrupt autovector */
	VECTOR(lev5intr)	/* 29: level 5 interrupt autovector */
	VECTOR(lev6intr)	/* 30: level 6 interrupt autovector */
	VECTOR(badtrap)		/* 31: level 7 interrupt autovector */
	VECTOR(illinst)		/* 32: syscalls */
	VECTOR(illinst)		/* 33: sigreturn syscall or breakpoint */
	VECTOR(illinst)		/* 34: breakpoint or sigreturn syscall */
	VECTOR(illinst)		/* 35: TRAP instruction vector */
	VECTOR(illinst)		/* 36: TRAP instruction vector */
	VECTOR(illinst)		/* 37: TRAP instruction vector */
	VECTOR(illinst)		/* 38: TRAP instruction vector */
	VECTOR(illinst)		/* 39: TRAP instruction vector */
	VECTOR(illinst)		/* 40: TRAP instruction vector */
	VECTOR(illinst)		/* 41: TRAP instruction vector */
	VECTOR(illinst)		/* 42: TRAP instruction vector */
	VECTOR(illinst)		/* 43: TRAP instruction vector */
	VECTOR(illinst)		/* 44: TRAP instruction vector */
	VECTOR(illinst)		/* 45: TRAP instruction vector */
	VECTOR(illinst)		/* 45: TRAP instruction vector */
	VECTOR(illinst)		/* 47: TRAP instruction vector */
 	VECTOR(fptrap)		/* 48: FPCP branch/set on unordered cond */
 	VECTOR(fptrap)		/* 49: FPCP inexact result */
 	VECTOR(fptrap)		/* 50: FPCP divide by zero */
 	VECTOR(fptrap)		/* 51: FPCP underflow */
 	VECTOR(fptrap)		/* 52: FPCP operand error */
 	VECTOR(fptrap)		/* 53: FPCP overflow */
 	VECTOR(fptrap)		/* 54: FPCP signalling NAN */

	VECTOR(badtrap)		/* 55: unassigned, reserved */
	VECTOR(badtrap)		/* 56: unassigned, reserved */
	VECTOR(badtrap)		/* 57: unassigned, reserved */
	VECTOR(badtrap)		/* 58: unassigned, reserved */
	VECTOR(badtrap)		/* 59: unassigned, reserved */
	VECTOR(badtrap)		/* 60: unassigned, reserved */
	VECTOR(badtrap)		/* 61: unassigned, reserved */
	VECTOR(badtrap)		/* 62: unassigned, reserved */
	VECTOR(badtrap)		/* 63: unassigned, reserved */
#define BADTRAP16       \
	VECTOR(badtrap) ; VECTOR(badtrap) ; \
	VECTOR(badtrap) ; VECTOR(badtrap) ; \
	VECTOR(badtrap) ; VECTOR(badtrap) ; \
	VECTOR(badtrap) ; VECTOR(badtrap) ; \
	VECTOR(badtrap) ; VECTOR(badtrap) ; \
	VECTOR(badtrap) ; VECTOR(badtrap) ; \
	VECTOR(badtrap) ; VECTOR(badtrap) ; \
	VECTOR(badtrap) ; VECTOR(badtrap)

	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */


	START = 0x700000
	STACK = 0x800000
	DIPSW = 0x49000000

ASENTRY_NOPROFILE(start1)
	movw	#PSL_HIGHIPL,%sr	| no interrupts
	movl	#STACK,%sp		| set SP

	movl	#_C_LABEL(prgcore), %a2	| save program address
	movl	#_ASM_LABEL(Reset), %a2@+	| save start of core
	movl	#_C_LABEL(end),  %a2@+	| save end of core
	movl	#STACK, %a2@		| save initial stack addr

/* clear BSS area */
	movl	#_C_LABEL(edata),%a2	| start of BSS
	movl	#_C_LABEL(end),%a3	| end
Lbssclr:
	clrb	%a2@+			| clear BSS
	cmpl	%a2,%a3			| done?
	bne	Lbssclr			| no, keep going

/* save address to goto ROM monitor */
	movec	%vbr,%a0		| ROM vbr to %a0
	movl	%a0@(NMIVEC),%d0	| restore NMIVEC
	movl	#_ASM_LABEL(gotoROM),%a0	| save to _gotoROM
	movl	%d0,%a0@		|
	movl	#_ASM_LABEL(Reset),%a0	| BP vbr to %a0
	movl	#_C_LABEL(exit),%a0@(NMIVEC)	| save address
	

/* switch vector tabel */
	movec	%vbr,%a0
	movl	%a0@(ILLGINST),%sp@-	| save ILLINST vector for BrkPtr
	movl	%a0@(EVTRAPF),%sp@-

	movl	#_ASM_LABEL(Reset),%a0
	movl	%sp@+,%a0@(EVTRAPF)
	movl	%sp@+,%a0@(ILLGINST)	| restore ILLINST vector
	movec	%a0,%vbr

	movl	#DIPSW,%a0
	movw	%a0@,%d0
	lsrl	#8,%d0
	andl	#0xFF,%d0
	movl	%d0,_C_LABEL(dipsw1)
	movw	%a0@,%d0
	andl	#0xFF,%d0
	movl	%d0,_C_LABEL(dipsw2)

/* determine our CPU */

	/* XXX should be generated via assym.h */
	CACHE_OFF = 0x0808
	DC_FREEZE = 0x0200
	CPU_68030 = 1
	CPU_68040 = 2

	movl	#CACHE_OFF,%d0
	movc	%d0,%cacr		| clear and disable on-chip cache(s)
	movl	#DC_FREEZE,%d0		| data freeze bit
	movc	%d0,%cacr		|   only exists on 68030
	movc	%cacr,%d0		| read it back
	tstl	%d0			| zero?
	jeq	Lnot68030		| yes, we have 68040
	movl	#CPU_68030,%d0
	jra	Lstart0
Lnot68030:
	movl	#CPU_68040,%d0
Lstart0:
	movl	%d0,_C_LABEL(cputype)

/* final setup for C code */
	movw	#PSL_LOWIPL,%sr		| no interrupts
	jsr	_C_LABEL(main)		| lets go
	jsr	start

/*
 * exit to ROM monitor
 */

	ROM_VBR = 0

ENTRY_NOPROFILE(exit)
GLOBAL(_rtt)
	movw	#PSL_HIGHIPL,%sr	| no interrupts
	movl	#ROM_VBR,%a0
	movec	%a0,%vbr
	movl	#_ASM_LABEL(gotoROM),%a0
	movl	%a0@,%a1
	jmp	%a1@

/*
 * Trap/interrupt vector routines
 */ 

ENTRY_NOPROFILE(buserr)
	tstl	_C_LABEL(nofault)	| device probe?
	jeq	_C_LABEL(addrerr)	| no, handle as usual
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
ENTRY_NOPROFILE(addrerr)
	clrw	%sp@-			| pad SR to longword
	moveml	#0xFFFF,%sp@-		| save user registers
	movl	%usp,%a0		| save the user SP
	movl	%a0,%sp@(60)		|   in the savearea
	lea	%sp@(64),%a1		| grab base of HW berr frame
	movw	%a1@(12),%d0		| grab SSW for fault processing
	btst	#12,%d0			| RB set?
	jeq	LbeX0			| no, test RC
	bset	#14,%d0			| yes, must set FB
	movw	%d0,%a1@(12)		| for hardware too
LbeX0:
	btst	#13,%d0			| RC set?
	jeq	LbeX1			| no, skip
	bset	#15,%d0			| yes, must set FC
	movw	%d0,%a1@(12)		| for hardware too
LbeX1:
	btst	#8,%d0			| data fault?
	jeq	Lbe0			| no, check for hard cases
	movl	%a1@(18),%d1		| fault address is as given in frame
	jra	Lbe10			| thats it
Lbe0:
	btst	#4,%a1@(8)		| long (type B) stack frame?
	jne	Lbe4			| yes, go handle
	movl	%a1@(4),%d1		| no, can use save PC
	btst	#14,%d0			| FB set?
	jeq	Lbe3			| no, try FC
	addql	#4,%d1			| yes, adjust address
	jra	Lbe10			| done
Lbe3:
	btst	#15,%d0			| FC set?
	jeq	Lbe10			| no, done
	addql	#2,%d1			| yes, adjust address
	jra	Lbe10			| done
Lbe4:
	movl	%a1@(38),%d1		| long format, use stage B address
	btst	#15,%d0			| FC set?
	jeq	Lbe10			| no, all done
	subql	#2,%d1			| yes, adjust address
Lbe10:
	movl	%d1,%sp@-		| push fault VA
	movw	%d0,%sp@-		| and SSW
	clrw	%sp@-			|   padded to longword
	movw	%a1@(8),%d0		| get frame format/vector offset
	andw	#0x0FFF,%d0		| clear out frame format
	cmpw	#12,%d0			| address error vector?
	jeq	Lisaerr			| yes, go to it
#if 0
	movl	%d1,%a0			| fault address
	.long	0xf0109e11		| ptestr #1,%a0@,#7
	.long	0xf0176200		| pmove %psr,%sp@
	btst	#7,%sp@			| bus error bit set?
	jeq	Lismerr			| no, must be MMU fault
	clrw	%sp@			| yes, re-clear pad word
#endif
	jra	Lisberr			| and process as normal bus error
Lismerr:
	movl	#T_MMUFLT,%sp@-		| show that we are an MMU fault
	jra	Lbexit			| and deal with it
Lisaerr:
	movl	#T_ADDRERR,%sp@-	| mark address error
	jra	Lbexit			| and deal with it
Lisberr:
	movl	#T_BUSERR,%sp@-		| mark bus error
Lbexit:
	jbsr	_C_LABEL(trap)		| handle the error
	lea	%sp@(12),%sp		| pop value args
	movl	%sp@(60),%a0		| restore user SP
	movl	%a0,%usp		|   from save area
	moveml	%sp@+,#0x7FFF		| restore most user regs
	addql	#4,%sp			| toss SSP
	tstw	%sp@+			| do we need to clean up stack?
	jeq	_ASM_LABEL(rei)		| no, just continue
	btst	#7,%sp@(6)		| type 9/10/11 frame?
	jeq	_ASM_LABEL(rei)		| no, nothing to do
	btst	#5,%sp@(6)		| type 9?
	jne	Lbex1			| no, skip
	movw	%sp@,%sp@(12)		| yes, push down SR
	movl	%sp@(2),%sp@(14)	| and PC
	clrw	%sp@(18)		| and mark as type 0 frame
	lea	%sp@(12),%sp		| clean the excess
	jra	_ASM_LABEL(rei)		| all done
Lbex1:
	btst	#4,%sp@(6)		| type 10?
	jne	Lbex2			| no, skip
	movw	%sp@,%sp@(24)		| yes, push down SR
	movl	%sp@(2),%sp@(26)	| and PC
	clrw	%sp@(30)		| and mark as type 0 frame
	lea	%sp@(24),%sp		| clean the excess
	jra	_ASM_LABEL(rei)		| all done
Lbex2:
	movw	%sp@,%sp@(84)		| type 11, push down SR
	movl	%sp@(2),%sp@(86)	| and PC
	clrw	%sp@(90)		| and mark as type 0 frame
	lea	%sp@(84),%sp		| clean the excess
	jra	_ASM_LABEL(rei)		| all done

ENTRY_NOPROFILE(illinst)
	clrw	%sp@-
	moveml	#0xFFFF,%sp@-
	moveq	#T_ILLINST,%d0
	jra	_C_LABEL(fault)

ENTRY_NOPROFILE(zerodiv)
	clrw	%sp@-
	moveml	#0xFFFF,%sp@-
	moveq	#T_ZERODIV,%d0
	jra	_C_LABEL(fault)

ENTRY_NOPROFILE(chkinst)
	clrw	%sp@-
	moveml	#0xFFFF,%sp@-
	moveq	#T_CHKINST,%d0
	jra	_C_LABEL(fault)

ENTRY_NOPROFILE(trapvinst)
	clrw	%sp@-
	moveml	#0xFFFF,%sp@-
	moveq	#T_TRAPVINST,%d0
	jra	_C_LABEL(fault)

ENTRY_NOPROFILE(privinst)
	clrw	%sp@-
	moveml	#0xFFFF,%sp@-
	moveq	#T_PRIVINST,%d0
	jra	_C_LABEL(fault)

ENTRY_NOPROFILE(coperr)
	clrw	%sp@-
	moveml	#0xFFFF,%sp@-
	moveq	#T_COPERR,%d0
	jra	_C_LABEL(fault)

ENTRY_NOPROFILE(fmterr)
	clrw	%sp@-
	moveml	#0xFFFF,%sp@-
	moveq	#T_FMTERR,%d0
	jra	_C_LABEL(fault)

ENTRY_NOPROFILE(fptrap)
#ifdef FPCOPROC
	clrw	%sp@-		| pad SR to longword
	moveml	#0xFFFF,%sp@-	| save user registers
	movl	%usp,%a0	| and save
	movl	%a0,%sp@(60)	|   the user stack pointer
	clrl	%sp@-		| no VA arg
#if 0
	lea	_u+PCB_FPCTX,%a0	| address of FP savearea
	.word	0xf310		| fsave %a0@
	tstb	%a0@		| null state frame?
	jeq	Lfptnull	| yes, safe
	clrw	%d0		| no, need to tweak BIU
	movb	a0@(1),d0	| get frame size
	bset	#3,%a0@(0,%d0:w)	| set exc_pend bit of BIU
Lfptnull:
	.word	0xf227,0xa800	| fmovem %fpsr,%sp@- (code arg)
	.word	0xf350		| frestore %a0@
#else
	clrl	%sp@-		| push dummy FPSR
#endif
	movl	#T_FPERR,%sp@-	| push type arg
	jbsr	_C_LABEL(trap)	| call trap
	lea	%sp@(12),%sp	| pop value args
	movl	%sp@(60),%a0	| restore
	movl	%a0,%usp	|   user SP
	moveml	%sp@+,#0x7FFF	| and remaining user registers
	addql	#6,%sp		| pop SSP and align word
	jra	_ASM_LABEL(rei)	| all done
#else
	jra	_C_LABEL(badtrap)	| treat as an unexpected trap
#endif

ENTRY_NOPROFILE(fault)
	movl	%usp,%a0	| get and save
	movl	%a0,%sp@(60)	|   the user stack pointer
	clrl	%sp@-		| no VA arg
	clrl	%sp@-		| or code arg
	movl	%d0,%sp@-	| push trap type
	jbsr	_C_LABEL(trap)	| handle trap
	lea	%sp@(12),%sp	| pop value args
	movl	%sp@(60),%a0	| restore
	movl	%a0,%usp	|   user SP
	moveml	%sp@+,#0x7FFF	| restore most user regs
	addql	#6,%sp		| pop SP and pad word
	jra	_ASM_LABEL(rei)	| all done

ENTRY_NOPROFILE(badtrap)
	clrw	%sp@-
	moveml	#0xC0C0,%sp@-
	movw	%sp@(24),%sp@-
	clrw	%sp@-
	jbsr	_C_LABEL(straytrap)
	addql	#4,%sp
	moveml	%sp@+,#0x0303
	addql	#2,%sp
	jra	_ASM_LABEL(rei)

/*
 * Interrupt handlers.
 * All device interrupts are auto-vectored.  Most can be configured
 * to interrupt in the range IPL2 to IPL6.  Here are our assignments:
 *
 *	Level 0:	
 *	Level 1:	
 *	Level 2:	SCSI SPC
 *	Level 3:	
 *	Level 4:	
 *	Level 5:	System Clock
 *	Level 6:	Internal SIO used uPD7201A
 *	Level 7:	Non-maskable: Abort Key (Dispatched vector to ROM monitor)
 */

ENTRY_NOPROFILE(lev2intr)
	clrw	%sp@-
	moveml	#0xC0C0,%sp@-
	jbsr	_C_LABEL(scintr)
	moveml	%sp@+,#0x0303
	addql	#2,%sp
	jra	_ASM_LABEL(rei)

ENTRY_NOPROFILE(lev3intr)
	clrw	%sp@-
	moveml	#0xC0C0,%sp@-
	jbsr	_C_LABEL(lance_intr)
	moveml	%sp@+,#0x0303
	addql	#2,%sp
	jra	_ASM_LABEL(rei)

ENTRY_NOPROFILE(lev5intr)
	clrw	%sp@-			| push pad word
	moveml	#0xC0C0,%sp@-		| save scratch regs
	movl	#CLOCK_REG,%a0		| get clock CR addr
	movb	#CLK_CLR,%a0@		| reset system clock
	lea	%sp@(16),%a1		| get pointer to PS
	movl	%a1@,%sp@-		| push padded PS
	movl	%a1@(4),%sp@-		| push PC
	jbsr	_C_LABEL(hardclock)	| call generic clock int routine
	addql	#8,%sp			| pop params
	moveml	%sp@+,#0x0303		| restore scratch regs
	addql	#2,%sp			| pop pad word
	jra	_ASM_LABEL(rei)		| all done
ENTRY_NOPROFILE(hardclock)
	rts

ENTRY_NOPROFILE(lev6intr)
	clrw	%sp@-
	moveml	#0xC0C0,%sp@-
	jbsr	_C_LABEL(_siointr)
	moveml	%sp@+,#0x0303
	addql	#2,%sp
	jra	_ASM_LABEL(rei)


/*
 * Emulation of VAX REI instruction.
 *
 * This code deals with checking for and servicing ASTs
 * (profiling, scheduling) and software interrupts (network, softclock).
 * We check for ASTs first, just like the VAX.  To avoid excess overhead
 * the T_ASTFLT handling code will also check for software interrupts so we
 * do not have to do it here.
 *
 * This code is complicated by the fact that sendsig may have been called
 * necessitating a stack cleanup.  A cleanup should only be needed at this
 * point for coprocessor mid-instruction frames (type 9), but we also test
 * for bus error frames (type 10 and 11).
 */
#if 0
	.comm	_ssir,1
ASENTRY_NOPROFILE(rei)
#ifdef DEBUG
	tstl	_C_LABEL(panicstr)		| have we paniced?
	jne	Ldorte			| yes, do not make matters worse
#endif
	btst	#PCB_ASTB,_u+PCB_FLAGS+1| AST pending?
	jeq	Lchksir			| no, go check for SIR
	btst	#5,%sp@			| yes, are we returning to user mode?
	jne	Lchksir			| no, go check for SIR
	clrw	%sp@-			| pad SR to longword
	moveml	#0xFFFF,%sp@-		| save all registers
	movl	%usp,%a1		| including
	movl	%a1,%sp@(60)		|    the users SP
	clrl	%sp@-			| VA == none
	clrl	%sp@-			| code == none
	movl	#T_ASTFLT,%sp@-		| type == async system trap
	jbsr	_C_LABEL(trap)		| go handle it
	lea	%sp@(12),%sp		| pop value args
	movl	%sp@(60),%a0		| restore
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,#0x7FFF		| and all remaining registers
	addql	#4,%sp			| toss SSP
	tstw	%sp@+			| do we need to clean up stack?
	jeq	Ldorte			| no, just continue
	btst	#7,%sp@(6)		| type 9/10/11 frame?
	jeq	Ldorte			| no, nothing to do
	btst	#5,%sp@(6)		| type 9?
	jne	Last1			| no, skip
	movw	%sp@,%sp@(12)		| yes, push down SR
	movl	%sp@(2),%sp@(14)	| and PC
	clrw	%sp@(18)		| and mark as type 0 frame
	lea	%sp@(12),%sp		| clean the excess
	jra	Ldorte			| all done
Last1:
	btst	#4,%sp@(6)		| type 10?
	jne	Last2			| no, skip
	movw	%sp@,%sp@(24)		| yes, push down SR
	movl	%sp@(2),%sp@(26)	| and PC
	clrw	%sp@(30)		| and mark as type 0 frame
	lea	%sp@(24),%sp		| clean the excess
	jra	Ldorte			| all done
Last2:
	movw	%sp@,%sp@(84)		| type 11, push down SR
	movl	%sp@(2),%sp@(86)	| and PC
	clrw	%sp@(90)		| and mark as type 0 frame
	lea	%sp@(84),%sp		| clean the excess
	jra	Ldorte			| all done
Lchksir:
	tstb	_ssir			| SIR pending?
	jeq	Ldorte			| no, all done
	movl	%d0,%sp@-		| need a scratch register
	movw	%sp@(4),%d0		| get SR
	andw	#PSL_IPL7,%d0		| mask all but IPL
	jne	Lnosir			| came from interrupt, no can do
	movl	%sp@+,%d0		| restore scratch register
Lgotsir:
	movw	#SPL1,%sr		| prevent others from servicing int
	tstb	_ssir			| too late?
	jeq	Ldorte			| yes, oh well...
	clrw	%sp@-			| pad SR to longword
	moveml	#0xFFFF,%sp@-		| save all registers
	movl	%usp,%a1		| including
	movl	%a1,%sp@(60)		|    the users SP
	clrl	%sp@-			| VA == none
	clrl	%sp@-			| code == none
	movl	#T_SSIR,%sp@-		| type == software interrupt
	jbsr	_trap			| go handle it
	lea	%sp@(12),%sp		| pop value args
	movl	%sp@(60),%a0		| restore
	movl	%a0,%usp		|   user SP
	moveml	%sp@+,#0x7FFF		| and all remaining registers
	addql	#6,%sp			| pop SSP and align word
	rte
Lnosir:
	movl	%sp@+,%d0		| restore scratch register
Ldorte:
#else
ASENTRY_NOPROFILE(rei)			| dummy Entry of rei
#endif
	rte				| real return

/*
 * non-local gotos
 */
ALTENTRY(savectx, _setjmp)
ENTRY(setjmp)
	movl	%sp@(4),%a0	| savearea pointer
	moveml	#0xFCFC,%a0@	| save d2-d7/a2-a7
	movl	%sp@,%a0@(48)	| and return address
	moveq	#0,%d0		| return 0
	rts

ENTRY(qsetjmp)
	movl	%sp@(4),%a0	| savearea pointer
	lea	%a0@(40),%a0	| skip regs we do not save
	movl	%a6,%a0@+		| save FP
	movl	%sp,%a0@+		| save SP
	movl	%sp@,%a0@		| and return address
	moveq	#0,%d0		| return 0
	rts

ENTRY(longjmp)
	movl	%sp@(4),%a0
	moveml	%a0@+,#0xFCFC
	movl	%a0@,%sp@
	moveq	#1,%d0
	rts

ENTRY_NOPROFILE(getsfc)
	movc	%sfc,%d0
	rts
ENTRY_NOPROFILE(getdfc)
	movc	%dfc,%d0
	rts

/*
 * Set processor priority level calls.  Most could (should) be replaced
 * by inline asm expansions.  However, SPL0 and SPLX require special
 * handling.  If we are returning to the base processor priority (SPL0)
 * we need to check for our emulated software interrupts.
 */

ENTRY(spl0)
	moveq	#0,%d0
	movw	%sr,%d0			| get old SR for return
	movw	#PSL_LOWIPL,%sr		| restore new SR
|	jra	Lsplsir
	rts

ENTRY(splx)
	moveq	#0,%d0
	movw	%sr,%d0			| get current SR for return
	movw	%sp@(6),%d1		| get new value
	movw	%d1,%sr			| restore new SR
|	andw	#PSL_IPL7,%d1		| mask all but PSL_IPL
|	jne	Lspldone		| non-zero, all done
|Lsplsir:
|	tstb	_ssir			| software interrupt pending?
|	jeq	Lspldone		| no, all done
|	subql	#4,%sp			| make room for RTE frame
|	movl	%sp@(4),%sp@(2)		| position return address
|	clrw	%sp@(6)			| set frame type 0
|	movw	#PSL_LOWIPL,%sp@	| and new SR
|	jra	Lgotsir			| go handle it
|Lspldone:
	rts

ENTRY(spl1)
	moveq	#0,%d0
	movw	%sr,%d0
	movw	#SPL1,%sr
	rts

ALTENTRY(splscsi, _spl2)
ENTRY(spl2)
	moveq	#0,%d0
	movw	%sr,%d0
	movw	#SPL2,%sr
	rts

ENTRY(spl3)
	moveq	#0,%d0
	movw	%sr,%d0
	movw	#SPL3,%sr
	rts

ENTRY(spl4)
	moveq	#0,%d0
	movw	%sr,%d0
	movw	#SPL4,%sr
	rts

ENTRY(spl5)
	moveq	#0,%d0
	movw	%sr,%d0
	movw	#SPL5,%sr
	rts

ENTRY(spl6)
	moveq	#0,%d0
	movw	%sr,%d0
	movw	#SPL6,%sr
	rts

ALTENTRY(splhigh, _spl7)
ENTRY(spl7)
	moveq	#0,%d0
	movw	%sr,%d0
	movw	#PSL_HIGHIPL,%sr
	rts


	.data

/*
 * Memory Infomation Field for secondary booter memory allocator
 */

GLOBAL(prgcore)
	.long	0
	.long	0
	.long	0

ASLOCAL(gotoROM)
	.long	0

GLOBAL(dipsw1)
	.long	0

GLOBAL(dipsw2)
	.long	0

GLOBAL(cputype)
	.long	CPU_68030
