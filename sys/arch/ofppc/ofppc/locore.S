/*	$NetBSD: locore.S,v 1.20 2000/11/16 05:34:04 thorpej Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_ddb.h"
#include "opt_ipkdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "assym.h"

#include <sys/syscall.h>

#include <machine/param.h>
#include <machine/pmap.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/asm.h>

/*
 * Some instructions gas doesn't understand (yet?)
 */
#define	bdneq	bdnzf 2,

/*
 * Globals
 */
GLOBAL(esym)
	.long	0			/* end of symbol table */
GLOBAL(proc0paddr)
	.long	0			/* proc0 p_addr */

/*
 * File-scope for locore.S
 */
	.data
idle_u:
	.long	0			/* fake uarea during idle after exit */
openfirmware_entry:
	.long	0			/* openfirmware entry point */

/*
 * This symbol is here for the benefit of kvm_mkdb, and is supposed to
 * mark the start of kernel text.
 */
	.text
	.globl	_C_LABEL(kernel_text)
_C_LABEL(kernel_text):

/*
 * Startup entry.  Note, this must be the first thing in the text
 * segment!
 */
	.text
	.globl	__start
__start:
#ifdef	FIRMWORKSBUGS
	mfmsr	0
	andi.	0,0,PSL_IR|PSL_DR
	beq	1f

	bl	_C_LABEL(ofwr_init)
1:
#endif
	li	0,0
	mtmsr	0			/* Disable FPU/MMU/exceptions */
	isync

/* compute end of kernel memory */
	lis	8,_C_LABEL(end)@ha
	addi	8,8,_C_LABEL(end)@l
#ifdef DDB
	/*
	 * XXX Account for symbols.
	 */
#endif
	li	9,PGOFSET
	add	8,8,9
	andc	8,8,9
	lis	9,_C_LABEL(OF_buf)@ha
	stw	8,_C_LABEL(OF_buf)@l(9)
	addi	8,8,NBPG
	lis	9,idle_u@ha
	stw	8,idle_u@l(9)
	addi	8,8,USPACE		/* space for idle_u */
	lis	9,_C_LABEL(proc0paddr)@ha
	stw	8,_C_LABEL(proc0paddr)@l(9)
	addi	1,8,USPACE-FRAMELEN	/* stackpointer for proc0 */
	mr	4,1			/* end of mem reserved for kernel */
	xor	0,0,0
	stwu	0,-16(1)		/* end of stack chain */
	
	lis	8,openfirmware_entry@ha
	stw	5,openfirmware_entry@l(8) /* save client interface handler */
	lis	3,__start@ha
	addi	3,3,__start@l
	mr	5,6			/* args string */
	bl	_C_LABEL(initppc)
	bl	_C_LABEL(main)
	b	_C_LABEL(OF_exit)

/*
 * OpenFirmware entry point
 */
ENTRY(openfirmware)
	mflr	0			/* save return address */
	stw	0,4(1)
	stwu	1,-16(1)		/* setup stack frame */

	mfmsr	4			/* save msr */
	stw	4,8(1)

	lis	4,openfirmware_entry@ha	/* get firmware entry point */
	lwz	4,openfirmware_entry@l(4)
	mtlr	4

	li	0,0			/* turn off any ints/mmu/etc. */
	mtmsr	0
	isync

	blrl				/* call OpenFirmware */

	lwz	4,8(1)			/* restore msr */
	mtmsr	4
	isync

	lwz	1,0(1)			/* and return */
	lwz	0,4(1)
	mtlr	0
	blr

/*
 * Switch to/from OpenFirmware real mode stack
 *
 * Note: has to be called as the very first thing in OpenFirmware interface
 * routines.
 * E.g.:
 * int
 * OF_xxx(arg1, arg2)
 * type arg1, arg2;
 * {
 *	static struct {
 *		char *name;
 *		int nargs;
 *		int nreturns;
 *		char *method;
 *		int arg1;
 *		int arg2;
 *		int ret;
 *	} args = {
 *		"xxx",
 *		2,
 *		1,
 *	};
 *
 *	ofw_stack();
 *	args.arg1 = arg1;
 *	args.arg2 = arg2;
 *	if (openfirmware(&args) < 0)
 *		return -1;
 *	return args.ret;
 * }
 */

	.local	firmstk
	.comm	firmstk,NBPG,8

ENTRY(ofw_stack)
	mfmsr	8			/* turn off interrupts */
	andi.	0,8,~(PSL_EE|PSL_RI)@l
	mtmsr	0
	stw	8,4(1)			/* abuse return address slot */

	lwz	5,0(1)			/* get length of stack frame */
	subf	5,1,5

	lis	7,firmstk+NBPG-8@ha
	addi	7,7,firmstk+NBPG-8@l
	lis	6,ofw_back@ha
	addi	6,6,ofw_back@l
	subf	4,5,7			/* make room for stack frame on
					   new stack */
	stw	6,-4(7)			/* setup return pointer */
	stwu	1,-8(7)
	
	stw	7,-8(4)

	addi	3,1,8
	addi	1,4,-8
	subi	5,5,8

	b	_C_LABEL(ofbcopy)	/* and copy it */

ofw_back:
	lwz	1,0(1)			/* get callers original stack pointer */

	lwz	0,4(1)			/* get saved msr from abused slot */
	mtmsr	0
	
	lwz	1,0(1)			/* return */
	lwz	0,4(1)
	mtlr	0
	blr


/*
 * No processes are runnable, so loop waiting for one.
 * Separate label here for accounting purposes.
 * When we get here, interrupts are off (MSR[EE]=0) and sched_lock is held.
 */
ASENTRY(Idle)
	lis	8,_C_LABEL(sched_whichqs)@ha
	lwz	9,_C_LABEL(sched_whichqs)@l(8)

	or.	9,9,9
	bne-	.Lsw1			/* at least one queue non-empty */
	
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	bl	_C_LABEL(sched_unlock_idle)
#endif

	mfmsr	3
	ori	3,3,PSL_EE@l		/* reenable ints again */
	mtmsr	3
	isync
	
/* May do some power saving here? */

	andi.	3,3,~PSL_EE@l		/* disable interrupts while
					   manipulating runque */
	mtmsr	3

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	bl	_C_LABEL(sched_lock_idle)
#endif
	b	_ASM_LABEL(Idle)

/*
 * switchexit gets called from cpu_exit to complete the exit procedure.
 */
ENTRY(switchexit)
/* First switch to the idle pcb/kernel stack */
	lis	6,idle_u@ha
	lwz	6,idle_u@l(6)
	lis	7,_C_LABEL(curpcb)@ha
	stw	6,_C_LABEL(curpcb)@l(7)
	addi	1,6,USPACE-16		/* 16 bytes are reserved at stack top */
	/*
	 * Schedule the vmspace and stack to be freed (the proc arg is
	 * already in r3).
	 */
	bl	_C_LABEL(exit2)

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	bl	_C_LABEL(sched_lock_idle)
#endif

/* Fall through to cpu_switch to actually select another proc */
	li	3,0			/* indicate exited process */

/*
 * void cpu_switch(struct proc *p)
 * Find a runnable process and switch to it.
 */
/* XXX noprofile?  --thorpej@netbsd.org */
ENTRY(cpu_switch)
	mflr	0			/* save lr */
	stw	0,4(1)
	stwu	1,-16(1)
	stw	31,12(1)
	stw	30,8(1)

	mr	30,3
	lis	3,_C_LABEL(curproc)@ha
	xor	31,31,31
	stw	31,_C_LABEL(curproc)@l(3) /* Zero to not accumulate cpu time */
	lis	3,_C_LABEL(curpcb)@ha
	lwz	31,_C_LABEL(curpcb)@l(3)

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
/* Release the sched_lock before processing interrupts. */
	bl	_C_LABEL(sched_unlock_idle)
#endif

	xor	3,3,3
	lis	4,_C_LABEL(machine_interface)+SPLX@ha
	lwz	0,_C_LABEL(machine_interface)+SPLX@l(4)
	mtlr	0
	blrl
	stw	3,PCB_SPL(31)		/* save spl */

/* Lock the scheduler. */
	mfmsr	3
	andi.	3,3,~PSL_EE@l		/* disable interrupts while
					   manipulating runque */
	mtmsr	3
	isync
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	bl	_C_LABEL(sched_lock_idle)
#endif

/* Find a new process */
	lis	8,_C_LABEL(sched_whichqs)@ha
	lwz	9,_C_LABEL(sched_whichqs)@l(8)

	or.	9,9,9
	beq-	_ASM_LABEL(Idle)	/* all queues empty */
.Lsw1:
	cntlzw	10,9
	lis	4,_C_LABEL(sched_qs)@ha
	addi	4,4,_C_LABEL(sched_qs)@l
	slwi	3,10,3
	add	3,3,4			/* select queue */
	
	lwz	31,P_FORW(3)		/* unlink first proc from queue */
	lwz	4,P_FORW(31)
	stw	4,P_FORW(3)
	stw	3,P_BACK(4)

	cmpl	0,3,4			/* queue empty? */
	bne	1f

	lis	3,0x80000000@h
	srw	3,3,10
	andc	9,9,3
	stw	9,_C_LABEL(sched_whichqs)@l(8) /* mark it empty */

1:
	/* just did this resched thing */
	xor	3,3,3
	lis	4,_C_LABEL(want_resched)@ha
	stw	3,_C_LABEL(want_resched)@l(4)

	stw	3,P_BACK(31)		/* probably superfluous */

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	/* Unlock the sched_lock, but leave interrupts off, for now. */
	bl	_C_LABEL(sched_unlock_idle)
#endif

#ifdef MULTIPROCESSOR
	/*
	 * XXXSMP
	 * p->p_cpu = curcpu();
	 */
#endif

	/* Process now running on a processor. */
	li	3,SONPROC		/* p->p_stat = SONPROC */
	stb	3,P_STAT(31)

	/* record new process */
	lis	4,_C_LABEL(curproc)@ha
	stw	31,_C_LABEL(curproc)@l(4)

	mfmsr	3
	ori	3,3,PSL_EE@l		/* Now we can interrupt again */
	mtmsr	3

	cmpl	0,31,30			/* is it the same process? */
	beq	switch_return

	or.	30,30,30		/* old process was exiting? */
	beq	switch_exited

	mfsr	10,USER_SR		/* save USER_SR for copyin/copyout */
	mfcr	11			/* save cr */
	mr	12,2			/* save r2 */
	stwu	1,-SFRAMELEN(1)		/* still running on old stack */
	stmw	10,8(1)
	lwz	3,P_ADDR(30)
	stw	1,PCB_SP(3)		/* save SP */

switch_exited:
	mfmsr	3
	andi.	3,3,~PSL_EE@l		/* disable interrupts while
					   actually switching */
	mtmsr	3

	/* indicate new pcb */
	lwz	4,P_ADDR(31)
	lis	5,_C_LABEL(curpcb)@ha
	stw	4,_C_LABEL(curpcb)@l(5)

	/* save real pmap pointer for spill fill */
	lwz	5,PCB_PMR(4)
	lis	6,_C_LABEL(curpm)@ha
	stwu	5,_C_LABEL(curpm)@l(6)
	stwcx.	5,0,6			/* clear possible reservation */

	addic.	5,5,64
	li	6,0
	mfsr	8,KERNEL_SR		/* save kernel SR */
1:
	addis	6,6,-0x10000000@ha	/* set new procs segment registers */
	or.	6,6,6			/* This is done from the real
					   address pmap */
	lwzu	7,-4(5)			/* so we don't have to worry */
	mtsrin	7,6			/* about accessibility */
	bne	1b
	mtsr	KERNEL_SR,8		/* restore kernel SR */
	isync

	lwz	1,PCB_SP(4)		/* get new procs SP */

	ori	3,3,PSL_EE@l		/* interrupts are okay again */
	mtmsr	3

	lmw	10,8(1)			/* get other regs */
	lwz	1,0(1)			/* get saved SP */
	mr	2,12			/* get saved r2 */
	mtcr	11			/* get saved cr */
	isync
	mtsr	USER_SR,10		/* get saved USER_SR */
	isync

switch_return:
	mr	30,7			/* save proc pointer */
	lwz	3,PCB_SPL(4)
	lis	4,_C_LABEL(machine_interface)+SPLX@ha
	lwz	0,_C_LABEL(machine_interface)+SPLX@l(4)
	mtlr	0
	blrl

	mr	3,30			/* get curproc for special fork
					   returns */

	lwz	31,12(1)
	lwz	30,8(1)
	addi	1,1,16
	lwz	0,4(1)
	mtlr	0
	blr

/*
 * Child comes here at the end of a fork.
 * Mostly similar to the above.
 */
	.globl	_C_LABEL(fork_trampoline)
_C_LABEL(fork_trampoline):
	xor	3,3,3
	lis	4,_C_LABEL(machine_interface)+SPLX@ha
	lwz	0,_C_LABEL(machine_interface)+SPLX@l(4)
	mtlr	0
	blrl
	mtlr	31
	mr	3,30
	blrl				/* jump indirect to r31 */
	b	trapexit

/*
 * Pull in common trap vector code.
 */
#include <powerpc/powerpc/trap_subr.S>

/*
 * int setfault()
 *
 * Similar to setjmp to setup for handling faults on accesses to user memory.
 * Any routine using this may only call bcopy, either the form below,
 * or the (currently used) C code optimized, so it doesn't use any non-volatile
 * registers.
 */
	.globl	_C_LABEL(setfault)
_C_LABEL(setfault):
	mflr	0
	mfcr	12
	lis	4,_C_LABEL(curpcb)@ha
	lwz	4,_C_LABEL(curpcb)@l(4)
	stw	3,PCB_FAULT(4)
	stw	0,0(3)
	stw	1,4(3)
	stw	2,8(3)
	stmw	12,12(3)
	xor	3,3,3
	blr
