/*	$NetBSD: intvec.S,v 1.10.16.1 2007/03/12 05:51:16 rmind Exp $   */

/*
 * Copyright (c) 1994, 1997 Ludd, University of Lule}, Sweden.
 * All rights reserved.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "assym.h"
#include <sys/cdefs.h>
#include <net/netisr.h>

#define	__HAVE_GENERIC_SOFT_INTERRUPTS	/* XXX - cannot include types.h! */

#include "opt_ddb.h"
#include "opt_cputype.h"
#include "opt_emulate.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "leds.h"

#define SCBENTRY(name) \
	.text			; \
	.align 2		; \
	.globl __CONCAT(X,name)	; \
__CONCAT(X,name):

#define TRAPCALL(namn, typ) \
SCBENTRY(namn)			; \
	pushl $0		; \
	pushl $typ		; \
	jbr Xtrap

#define TRAPARGC(namn, typ) \
SCBENTRY(namn)			; \
	pushl $typ		; \
	jbr Xtrap

#define FASTINTR(namn, rutin) \
SCBENTRY(namn)			; \
	pushr $0x3f		; \
	calls $0,_C_LABEL(rutin)	; \
	popr $0x3f		; \
	rei

#define	PUSHR	pushr	$0x3f
#define	POPR	popr	$0x3f

#define KSTACK 0
#define ISTACK 1
#define	NOVEC	.long 0
#define INTVEC(label,stack)	\
	.long	__CONCAT(X,label)+stack;

	.text

	.globl	_C_LABEL(kernbase), _C_LABEL(rpb), _C_LABEL(kernel_text)
	.set	_C_LABEL(kernel_text),KERNBASE
_C_LABEL(kernbase):
_C_LABEL(rpb):	
/*
 * First page in memory we have rpb; so that we know where
 * (must be on a 64k page boundary, easiest here). We use it
 * to store SCB vectors generated when compiling the kernel,
 * and move the SCB later to somewhere else.
 */

	NOVEC;				# Unused, 0
	INTVEC(mcheck, ISTACK)		# Machine Check., 4
	INTVEC(invkstk, ISTACK) 	# Kernel Stack Invalid., 8
	NOVEC;			 	# Power Failed., C
	INTVEC(privinflt, KSTACK)	# Privileged/Reserved Instruction.
	INTVEC(xfcflt, KSTACK)		# Customer Reserved Instruction, 14
	INTVEC(resopflt, KSTACK)	# Reserved Operand/Boot Vector(?), 18
	INTVEC(resadflt, KSTACK)	# Reserved Address Mode., 1C
	INTVEC(access_v, KSTACK)	# Access Control Violation, 20
	INTVEC(transl_v, KSTACK)	# Translation Invalid, 24
	INTVEC(tracep, KSTACK)		# Trace Pending, 28
	INTVEC(breakp, KSTACK)		# Breakpoint Instruction, 2C
	NOVEC;			 	# Compatibility Exception, 30
	INTVEC(arithflt, KSTACK)	# Arithmetic Fault, 34
	NOVEC;			 	# Unused, 38
	NOVEC;			 	# Unused, 3C
	INTVEC(syscall, KSTACK)		# main syscall trap, chmk, 40
	INTVEC(resopflt, KSTACK)	# chme, 44
	INTVEC(resopflt, KSTACK)	# chms, 48
	INTVEC(resopflt, KSTACK)	# chmu, 4C
	NOVEC;				# System Backplane Exception/BIerror, 50
	INTVEC(cmrerr, ISTACK)		# Corrected Memory Read, 54
	NOVEC;				# System Backplane Alert/RXCD, 58
	INTVEC(sbiflt, ISTACK)		# System Backplane Fault, 5C
	NOVEC;				# Memory Write Timeout, 60
	NOVEC;				# Unused, 64
	NOVEC;				# Unused, 68
	NOVEC;				# Unused, 6C
	NOVEC;				# Unused, 70
	NOVEC;				# Unused, 74
	NOVEC;				# Unused, 78
	NOVEC;				# Unused, 7C
	NOVEC;				# Unused, 80
	NOVEC;				# Unused, 84
	INTVEC(astintr,	KSTACK)		# Asynchronous Sustem Trap, AST (IPL 02)
	NOVEC;				# Unused, 8C
	NOVEC;				# Unused, 90
	NOVEC;				# Unused, 94
	NOVEC;				# Unused, 98
	NOVEC;				# Unused, 9C
	INTVEC(softclock,ISTACK)	# Software clock interrupt (IPL 08)
	NOVEC;				# Unused, A4 (IPL 09)
	NOVEC;				# Unused, A8 (IPL 10)
	NOVEC;				# Unused, AC (IPL 11)
	INTVEC(softnet, ISTACK)		# Software network interrupt (IPL 12)
	INTVEC(softserial, ISTACK)	# Software serial interrupt (IPL 13)
	NOVEC;				# Unused, B8 (IPL 14)
	INTVEC(ddbtrap, ISTACK) 	# Kernel debugger trap, BC (IPL 15)
	INTVEC(hardclock,ISTACK)	# Interval Timer
	NOVEC;				# Unused, C4
	INTVEC(emulate, KSTACK)		# Subset instruction emulation, C8
	NOVEC;				# Unused, CC
	NOVEC;				# Unused, D0
	NOVEC;				# Unused, D4
	NOVEC;				# Unused, D8
	NOVEC;				# Unused, DC
	NOVEC;				# Unused, E0
	NOVEC;				# Unused, E4
	NOVEC;				# Unused, E8
	NOVEC;				# Unused, EC
	NOVEC;		
	NOVEC;		
	NOVEC;		
	NOVEC;		

	/* space for adapter vectors */
	.space 0x100

		.align 2
#
# mcheck is the badaddress trap, also called when referencing
# a invalid address (busserror)
# _memtest (memtest in C) holds the address to continue execution
# at when returning from a intentional test.
#
SCBENTRY(mcheck)
	tstl	_C_LABEL(cold)		# Ar we still in coldstart?
	bneq	L4		# Yes.

	pushr	$0x7f
	pushab	24(%sp)
	movl	_C_LABEL(dep_call),%r6	# CPU dependent mchk handling
	calls	$1,*MCHK(%r6)
	tstl	%r0		# If not machine check, try memory error
	beql	1f
	calls	$0,*MEMERR(%r6)
	pushab	2f
	calls	$1,_C_LABEL(panic)
2:	.asciz	"mchk"
1:	popr	$0x7f
	addl2	(%sp)+,%sp

	rei

L4:	addl2	(%sp)+,%sp	# remove info pushed on stack
	pushr	$0x3f		# save regs for clobbering
	movl	_C_LABEL(dep_call),%r0	# get CPU-specific mchk handler
	tstl	BADADDR(%r0)	# any handler available?
	bneq	4f		# yep, call it
	popr	$0x3f		# nope, restore regs
	brb	0f		# continue
4:	calls	$0,*BADADDR(%r0)	# call machine-specific handler
	popr	$0x3f		# restore regs
	brb	2f

0:	cmpl	_C_LABEL(vax_cputype),$1 # Is it a 11/780?
	bneq	1f		# No...

	mtpr	$0, $PR_SBIFS	# Clear SBI fault register
	brb	2f

1:	cmpl	_C_LABEL(vax_cputype),$4 # Is it a 8600?
	bneq	3f

	mtpr	$0, $PR_EHSR	# Clear Error status register
	brb	2f

3:	mtpr	$0xF,$PR_MCESR	# clear the bus error bit
2:	movl	_C_LABEL(memtest),(%sp)	# REI to new address
	rei

	TRAPCALL(invkstk, T_KSPNOTVAL)

SCBENTRY(privinflt)	# Privileged/unimplemented instruction
#ifndef NO_INSN_EMULATE
	jsb	_C_LABEL(unimemu)	# do not return if insn emulated
#endif
	pushl	$0
	pushl	$T_PRIVINFLT
	jbr	Xtrap

	TRAPCALL(xfcflt, T_XFCFLT);
	TRAPCALL(resopflt, T_RESOPFLT)
	TRAPCALL(resadflt, T_RESADFLT)

/*
 * Translation fault, used only when simulating page reference bit.
 * Therefore it is done a fast revalidation of the page if it is
 * referenced. Trouble here is the hardware bug on KA650 CPUs that
 * put in a need for an extra check when the fault is gotten during
 * PTE reference. Handled in pmap.c.
 */
SCBENTRY(transl_v)		# 20: Translation violation
	pushr	$0x3f
	pushl	28(%sp)
	pushl	28(%sp)
	calls	$2,_C_LABEL(pmap_simulref)
	tstl	%r0
	bneq	1f
	popr	$0x3f
	addl2	$8,%sp
	rei
1:	popr	$0x3f
	brw	Xaccess_v

SCBENTRY(access_v)			# 24: Access cntrl viol fault
	blbs	(%sp), ptelen
	pushl	$T_ACCFLT
	bbc	$1,4(%sp),1f
	bisl2	$T_PTEFETCH,(%sp)
1:	bbc	$2,4(%sp),2f
	bisl2	$T_WRITE,(%sp)
2:	movl	(%sp), 4(%sp)
	addl2	$4, %sp
	jbr	Xtrap

ptelen: movl	$T_PTELEN, (%sp)		# PTE must expand (or send segv)
	jbr	Xtrap;

TRAPCALL(tracep, T_TRCTRAP)
TRAPCALL(breakp, T_BPTFLT)

TRAPARGC(arithflt, T_ARITHFLT)

SCBENTRY(syscall)			# Main system call
	pushl	$T_SYSCALL
	pushr	$0xfff
	mfpr	$PR_USP, -(%sp)
	mfpr    $PR_SSP, %r0
	movl	CI_CURLWP(%r0), %r0
	movl	L_PROC(%r0), %r0
	pushl	%ap
	pushl	%fp
	pushl	%sp		# pointer to syscall frame; defined in trap.h
	calls	$1, *P_MD_SYSCALL(%r0)
	movl	(%sp)+, %fp
	movl	(%sp)+, %ap
	mtpr	(%sp)+, $PR_USP
	popr	$0xfff
	addl2	$8, %sp
	mtpr	$IPL_HIGH, $PR_IPL	# Be sure we can REI
	rei


SCBENTRY(cmrerr)
	PUSHR
	movl	_C_LABEL(dep_call),%r0
	calls	$0,*MEMERR(%r0)
	POPR
	rei

SCBENTRY(sbiflt);
	pushab	sbifltmsg
	calls	$1, _C_LABEL(panic)

TRAPCALL(astintr, T_ASTFLT)

SCBENTRY(softclock)
	PUSHR
	movab	_C_LABEL(softclock_head),%r0
	jsb	softintr_dispatch
	incl	_C_LABEL(softclock_intrcnt)+EV_COUNT
	adwc	$0,_C_LABEL(softclock_intrcnt)+EV_COUNT+4
	incl	_C_LABEL(uvmexp)+UVME_SOFTS
	POPR
	rei

SCBENTRY(softnet)
	PUSHR

#	tstl	_C_LABEL(netisr)			# any netisr's set
#	beql	2f			# no, skip looking at them one by one
#define DONETISR(bit, fn) \
	bbcci	$bit,_C_LABEL(netisr),1f; \
	calls	$0,_C_LABEL(fn); \
	1:

#include <net/netisr_dispatch.h>

#undef DONETISR

2:	movab	_C_LABEL(softnet_head),%r0
	jsb	softintr_dispatch
	incl	_C_LABEL(softnet_intrcnt)+EV_COUNT
	adwc	$0,_C_LABEL(softnet_intrcnt)+EV_COUNT+4
	incl	_C_LABEL(uvmexp)+UVME_SOFTS
	POPR
	rei

SCBENTRY(softserial)
	PUSHR
	movab	_C_LABEL(softserial_head),%r0
	jsb	softintr_dispatch
	incl	_C_LABEL(softserial_intrcnt)+EV_COUNT
	adwc	$0,_C_LABEL(softserial_intrcnt)+EV_COUNT+4
	incl	_C_LABEL(uvmexp)+UVME_SOFTS
	POPR
	rei

	.align	2
softintr_dispatch:
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	pushl	%r0
	calls	$0,_C_LABEL(krnlock)
	movl	(%sp)+,%r0
#endif
	movl	SHD_INTRS(%r0), %r0	# anything to do? (get first handler)
	beql	3f			# nope return
	pushl	%r7			# we need to use %r7 so save it
	movl	%r0, %r7			# move first item to %r7
1:	tstl	SH_PENDING(%r7)		# need call this one?
	beql	2f			# nope, go to next one
	clrl	SH_PENDING(%r7)		# clear pending flag
	pushl	SH_ARG(%r7)		# push function argument
	calls	$1, *SH_FUNC(%r7)	# call function
2:	movl	SH_NEXT(%r7), %r7		# get next handler
	bneq	1b			# if not null, process it
	movl	(%sp)+, %r7		# done, restore %r7
3:
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	calls	$0,_C_LABEL(krnunlock)
#endif
	rsb				# return to caller

TRAPCALL(ddbtrap, T_KDBTRAP)

SCBENTRY(hardclock)
	mtpr	$0xc1,$PR_ICCS		# Reset interrupt flag
#ifdef DDB
	tstl	0x80000100		# rpb wait element
	beql	1f			# set, jmp to debugger
	pushl	$0
	pushl	$T_KDBTRAP
	jbr	Xtrap
#endif
1:	pushr	$0x3f
	incl	_C_LABEL(clock_intrcnt)+EV_COUNT	# count the number of clock interrupts
	adwc	$0,_C_LABEL(clock_intrcnt)+EV_COUNT+4
	incl	_C_LABEL(uvmexp)+UVME_INTRS
#if VAX46 || VAXANY
	cmpl	_C_LABEL(vax_boardtype),$VAX_BTYP_46
	bneq	1f
	movl	_C_LABEL(ka46_cpu),%r0
	clrl	VC_DIAGTIMM(%r0)
#endif
1:	pushl	%sp
	addl2	$24,(%sp)
	calls	$1,_C_LABEL(hardclock)
#if NLEDS
	calls	$0,_C_LABEL(leds_intr)
#endif
	popr	$0x3f
	rei

/*
 * Main routine for traps; all go through this.
 * Note that we put USP on the frame here, which sometimes should
 * be KSP to be correct, but because we only alters it when we are 
 * called from user space it doesn't care.
 * _sret is used in cpu_set_kpc to jump out to user space first time.
 */
	.globl	_C_LABEL(sret)
Xtrap:	pushr	$0xfff
	mfpr	$PR_USP, -(%sp)
	pushl	%ap
	pushl	%fp
	pushl	%sp
	calls	$1, _C_LABEL(trap)
_C_LABEL(sret):
	movl	(%sp)+, %fp
	movl	(%sp)+, %ap
	mtpr	(%sp)+, $PR_USP
	popr	$0xfff
	addl2	$8, %sp
	mtpr	$IPL_HIGH, $PR_IPL	# Be sure we can REI
	rei

sbifltmsg:
	.asciz	"SBI fault"

#ifndef NO_INSN_EMULATE
/*
 * Table of emulated Microvax instructions supported by emulate.s.
 * Use noemulate to convert unimplemented ones to reserved instruction faults.
 */
	.globl	_C_LABEL(emtable)
_C_LABEL(emtable):
/* f8 */ .long _C_LABEL(EMashp);	.long _C_LABEL(EMcvtlp)
/* fa */ .long noemulate;		.long noemulate
/* fc */ .long noemulate;		.long noemulate
/* fe */ .long noemulate;		.long noemulate
/* 00 */ .long noemulate;		.long noemulate
/* 02 */ .long noemulate;		.long noemulate
/* 04 */ .long noemulate;		.long noemulate
/* 05 */ .long noemulate;		.long noemulate
/* 08 */ .long _C_LABEL(EMcvtps);	.long _C_LABEL(EMcvtsp)
/* 0a */ .long noemulate;		.long _C_LABEL(EMcrc)
/* 0c */ .long noemulate;		.long noemulate
/* 0e */ .long noemulate;		.long noemulate
/* 10 */ .long noemulate;		.long noemulate
/* 12 */ .long noemulate;		.long noemulate
/* 14 */ .long noemulate;		.long noemulate
/* 16 */ .long noemulate;		.long noemulate
/* 18 */ .long noemulate;		.long noemulate
/* 1a */ .long noemulate;		.long noemulate
/* 1c */ .long noemulate;		.long noemulate
/* 1e */ .long noemulate;		.long noemulate
/* 20 */ .long _C_LABEL(EMaddp4);	.long _C_LABEL(EMaddp6)
/* 22 */ .long _C_LABEL(EMsubp4);	.long _C_LABEL(EMsubp6)
/* 24 */ .long _C_LABEL(EMcvtpt);	.long _C_LABEL(EMmulp)
/* 26 */ .long _C_LABEL(EMcvttp);	.long _C_LABEL(EMdivp)
/* 28 */ .long noemulate;		.long _C_LABEL(EMcmpc3)
/* 2a */ .long _C_LABEL(EMscanc);	.long _C_LABEL(EMspanc)
/* 2c */ .long noemulate;		.long _C_LABEL(EMcmpc5)
/* 2e */ .long _C_LABEL(EMmovtc);	.long _C_LABEL(EMmovtuc)
/* 30 */ .long noemulate;		.long noemulate
/* 32 */ .long noemulate;		.long noemulate
/* 34 */ .long _C_LABEL(EMmovp);	.long _C_LABEL(EMcmpp3)
/* 36 */ .long _C_LABEL(EMcvtpl);	.long _C_LABEL(EMcmpp4)
/* 38 */ .long _C_LABEL(EMeditpc);	.long _C_LABEL(EMmatchc)
/* 3a */ .long _C_LABEL(EMlocc);	.long _C_LABEL(EMskpc)
#endif
/*
 * The following is called with the stack set up as follows:
 *
 *	  (%sp): Opcode
 *	 4(%sp): Instruction PC
 *	 8(%sp): Operand 1
 *	12(%sp): Operand 2
 *	16(%sp): Operand 3
 *	20(%sp): Operand 4
 *	24(%sp): Operand 5
 *	28(%sp): Operand 6
 *	32(%sp): Operand 7 (unused)
 *	36(%sp): Operand 8 (unused)
 *	40(%sp): Return PC
 *	44(%sp): Return PSL
 *	48(%sp): TOS before instruction
 *
 * Each individual routine is called with the stack set up as follows:
 *
 *	  (%sp): Return address of trap handler
 *	 4(%sp): Opcode (will get return PSL)
 *	 8(%sp): Instruction PC
 *	12(%sp): Operand 1
 *	16(%sp): Operand 2
 *	20(%sp): Operand 3
 *	24(%sp): Operand 4
 *	28(%sp): Operand 5
 *	32(%sp): Operand 6
 *	36(%sp): saved register 11
 *	40(%sp): saved register 10
 *	44(%sp): Return PC
 *	48(%sp): Return PSL
 *	52(%sp): TOS before instruction
 *	See the VAX Architecture Reference Manual, Section B-5 for more
 *	information.
 */

SCBENTRY(emulate)
#ifndef NO_INSN_EMULATE
	movl	%r11,32(%sp)		# save register %r11 in unused operand
	movl	%r10,36(%sp)		# save register %r10 in unused operand
	cvtbl	(%sp),%r10		# get opcode
	addl2	$8,%r10			# shift negative opcodes
	subl3	%r10,$0x43,%r11		# forget it if opcode is out of range
	bcs	noemulate
	movl	_C_LABEL(emtable)[%r10],%r10
				# call appropriate emulation routine
	jsb	(%r10)		# routines put return values into regs 0-5
	movl	32(%sp),%r11		# restore register %r11
	movl	36(%sp),%r10		# restore register %r10
	insv	(%sp),$0,$4,44(%sp)	# and condition codes in Opcode spot
	addl2	$40,%sp			# adjust stack for return
	rei
noemulate:
	addl2	$48,%sp			# adjust stack for
#endif
	.word	0xffff			# "reserved instruction fault"
