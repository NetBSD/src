/*	$NetBSD: locore.S,v 1.35 2001/01/01 05:23:17 tsubai Exp $	*/

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

#if defined(MULTIPROCESSOR)
/*
 * Get varios per-cpu values.
 */
#ifdef notyet
#define GET_CPUINFO(r)	\
	mfsprg	r,0
#else
#define	GET_CPUINFO(r)						\
	mfspr	r,1023;			/* r = cpu_number() */	\
	mulli	r,r,CI_SIZE;					\
	addis	r,r,_C_LABEL(cpu_info)@ha;			\
	addi	r,r,_C_LABEL(cpu_info)@l;
#endif
#endif

#define	INTSTK   8192			/* 8K interrupt stack */
#define	SPILLSTK 4096			/* 4K spill stack */

/*
 * Globals
 */
	.data
GLOBAL(esym)
	.long	0			/* end of symbol table */
GLOBAL(proc0paddr)
	.long	0			/* proc0 p_addr */

GLOBAL(intrnames)
	.asciz	"irq0", "irq1", "irq2", "irq3"
	.asciz	"irq4", "irq5", "irq6", "irq7"
	.asciz	"irq8", "irq9", "irq10", "irq11"
	.asciz	"irq12", "irq13", "irq14", "irq15"
	.asciz	"irq16", "irq17", "irq18", "irq19"
	.asciz	"irq20", "irq21", "irq22", "irq23"
	.asciz	"irq24", "irq25", "irq26", "irq27"
	.asciz	"irq28", "irq29", "irq30", "irq31"
	.asciz	"irq32", "irq33", "irq34", "irq35"
	.asciz	"irq36", "irq37", "irq38", "irq39"
	.asciz	"irq40", "irq41", "irq42", "irq43"
	.asciz	"irq44", "irq45", "irq46", "irq47"
	.asciz	"irq48", "irq49", "irq50", "irq51"
	.asciz	"irq52", "irq53", "irq54", "irq55"
	.asciz	"irq56", "irq57", "irq58", "irq59"
	.asciz	"irq60", "irq61", "irq62", "irq63"
	.asciz	"clock", "softclock", "softnet", "softserial"
GLOBAL(eintrnames)
	.align	4
GLOBAL(intrcnt)
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0
GLOBAL(eintrcnt)

GLOBAL(ofmsr)
	.long	0			/* msr used in Open Firmware */

GLOBAL(powersave)
	.long	0

/*
 * File-scope for locore.S
 */
#if !defined(MULTIPROCESSOR)
idle_u:
	.long	0			/* fake uarea during idle after exit */
#endif
openfirmware_entry:
	.long	0			/* openfirmware entry point */
srsave:
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

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
	mfmsr	0
	lis	9,ofmsr@ha
	stw	0,ofmsr@l(9)

	li	0,0
	mtmsr	0			/* Disable FPU/MMU/exceptions */
	isync

/* compute end of kernel memory */
	lis	8,_C_LABEL(end)@ha
	addi	8,8,_C_LABEL(end)@l
#ifdef DDB
	/* skip symbol table */
	cmpwi	6,0
	beq	1f
	add	9,6,7			/* r9 = args + l */
	lwz	9,-8(9)			/* esym */
	cmpwi	9,0
	beq	1f
	mr	8,9
1:
#endif
	li	9,PGOFSET
	add	8,8,9
	andc	8,8,9
	lis	9,_C_LABEL(OF_buf)@ha
	stw	8,_C_LABEL(OF_buf)@l(9)
	addi	8,8,NBPG
#if defined(MULTIPROCESSOR)
	lis	9,_C_LABEL(cpu_info)@ha
	addi	9,9,_C_LABEL(cpu_info)@l
	addi	8,8,INTSTK
	stw	8,CI_INTSTK(9)
	addi	8,8,SPILLSTK
	stw	8,CI_SPILLSTK(9)
	stw	8,CI_IDLE_PCB(9)
#else
	lis	9,idle_u@ha
	stw	8,idle_u@l(9)
#endif
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

	li	0,0			/* clear battable translations */
	mtdbatu	2,0
	mtdbatu	3,0
	mtibatu	2,0
	mtibatu	3,0

	lis	4,srsave@ha		/* save old SR */
	addi	4,4,srsave@l
	li	5,0
1:	mfsrin	0,5
	stw	0,0(4)
	addi	4,4,4
	addis	5,5,0x10000000@h
	cmpwi	5,0
	bne	1b

	lis	4,_C_LABEL(ofw_pmap)@ha	/* load OFW SR */
	addi	4,4,_C_LABEL(ofw_pmap)@l
	lwz	0,PM_KERNELSR(4)
	cmpwi	0,0			/* pm_sr[KERNEL_SR] == 0? */
	beq	2f			/* then skip (not initialized yet) */
	li	5,0
1:	lwz	0,0(4)
	mtsrin	0,5
	addi	4,4,4
	addis	5,5,0x10000000@h
	cmpwi	5,0
	bne	1b
2:
	lis	4,ofmsr@ha		/* Open Firmware msr */
	lwz	4,ofmsr@l(4)
	mtmsr	4
	isync

	blrl				/* call Open Firmware */

	lis	4,srsave@ha		/* restore saved SR */
	addi	4,4,srsave@l
	li	5,0
1:	lwz	0,0(4)
	mtsrin	0,5
	addi	4,4,4
	addis	5,5,0x10000000@h
	cmpwi	5,0
	bne	1b

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
	bne+	.Lsw1			/* at least one queue non-empty */

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	bl	_C_LABEL(sched_unlock_idle)
#endif

	mfmsr	3
	ori	3,3,PSL_EE@l		/* reenable ints again */
	mtmsr	3
	isync

/* Check if we can use power saving mode */
	lis	8,_C_LABEL(powersave)@ha
	lwz	9,_C_LABEL(powersave)@l(8)

	or.	9,9,9
	beq	1f

	sync
	oris	3,3,PSL_POW@h		/* enter power saving mode */
	mtmsr	3
	isync
1:
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
#if defined(MULTIPROCESSOR)
	GET_CPUINFO(7)
	lwz	6,CI_IDLE_PCB(7)
	stw	6,CI_CURPCB(7)
#else
	lis	6,idle_u@ha
	lwz	6,idle_u@l(6)
	lis	7,_C_LABEL(curpcb)@ha
	stw	6,_C_LABEL(curpcb)@l(7)
#endif
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
#if defined(MULTIPROCESSOR)
	/* Switch to the idle PCB unless we're already running on it. */
	GET_CPUINFO(7)
	cmpwi	30,0			/* old process was exiting? */
	beq	1f

	mfsr	10,USER_SR		/* save USER_SR for copyin/copyout */
	mfcr	11			/* save cr */
	mr	12,2			/* save r2 */
	stwu	1,-SFRAMELEN(1)		/* still running on old stack */
	stmw	10,8(1)
	lwz	3,P_ADDR(30)
	stw	1,PCB_SP(3)		/* save SP */

	lwz	6,CI_IDLE_PCB(7)
	addi	1,6,USPACE-16		/* 16 bytes are reserved at stack top */

1:
	xor	31,31,31
	stw	31,CI_CURPROC(7)	/* Zero to not accumulate cpu time */
	lwz	31,CI_CURPCB(7)

	lwz	3,CI_CPL(7)
	stw	3,PCB_SPL(31)		/* save spl */
#else
	lis	3,_C_LABEL(curproc)@ha
	xor	31,31,31
	stw	31,_C_LABEL(curproc)@l(3) /* Zero to not accumulate cpu time */
	lis	3,_C_LABEL(curpcb)@ha
	lwz	31,_C_LABEL(curpcb)@l(3)
#endif

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
/* Release the sched_lock before processing interrupts. */
	bl	_C_LABEL(sched_unlock_idle)
#endif

	xor	3,3,3
	bl	_C_LABEL(lcsplx)
#if !defined(MULTIPROCESSOR)
	stw	3,PCB_SPL(31)		/* save spl */
#endif

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
#if defined(MULTIPROCESSOR)
	GET_CPUINFO(4)
	stw	3,CI_WANT_RESCHED(4)
#else
	lis	4,_C_LABEL(want_resched)@ha
	stw	3,_C_LABEL(want_resched)@l(4)
#endif

	stw	3,P_BACK(31)		/* probably superfluous */

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	/* Unlock the sched_lock, but leave interrupts off, for now. */
	bl	_C_LABEL(sched_unlock_idle)
#endif

#if defined(MULTIPROCESSOR)
	GET_CPUINFO(4)
	stw	4,P_CPU(31)		/* p->p_cpu = curcpu() */
#endif

	/* Process now running on a processor. */
	li	3,SONPROC		/* p->p_stat = SONPROC */
	stb	3,P_STAT(31)

	/* record new process */
#if defined(MULTIPROCESSOR)
	stw	31,CI_CURPROC(4)
#else
	lis	4,_C_LABEL(curproc)@ha
	stw	31,_C_LABEL(curproc)@l(4)
#endif

	mfmsr	3
	ori	3,3,PSL_EE@l		/* Now we can interrupt again */
	mtmsr	3

#if !defined(MULTIPROCESSOR)		/* XXX */
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
#endif

switch_exited:
	mfmsr	3
	andi.	3,3,~PSL_EE@l		/* disable interrupts while
					   actually switching */
	mtmsr	3

	/* indicate new pcb */
	lwz	4,P_ADDR(31)
#if defined(MULTIPROCESSOR)
	GET_CPUINFO(6)
	stw	4,CI_CURPCB(6)
#else
	lis	5,_C_LABEL(curpcb)@ha
	stw	4,_C_LABEL(curpcb)@l(5)
#endif

	/* save real pmap pointer for spill fill */
	lwz	5,PCB_PMR(4)
#if defined(MULTIPROCESSOR)
	stwu	5,CI_CURPM(6)
#else
	lis	6,_C_LABEL(curpm)@ha
	stwu	5,_C_LABEL(curpm)@l(6)
#endif
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
	bl	_C_LABEL(lcsplx)

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
 * Return to userspace via the trap return path.
 */
	.globl	_C_LABEL(fork_trampoline)
_C_LABEL(fork_trampoline):
#if defined(MULTIPROCESSOR)
	bl	_C_LABEL(proc_trampoline_mp)
#endif
	xor	3,3,3
	bl	_C_LABEL(lcsplx)
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
#if defined(MULTIPROCESSOR)
	GET_CPUINFO(4)
	lwz	4,CI_CURPCB(4)
#else
	lis	4,_C_LABEL(curpcb)@ha
	lwz	4,_C_LABEL(curpcb)@l(4)
#endif
	stw	3,PCB_FAULT(4)
	stw	0,0(3)
	stw	1,4(3)
	stw	2,8(3)
	stmw	12,12(3)
	xor	3,3,3
	blr

#if defined(MULTIPROCESSOR)
	.globl	_C_LABEL(cpu_spinup_trampoline)
_C_LABEL(cpu_spinup_trampoline):
	li	0,0
	mtmsr	0
	isync

	lis	3,cpu_hatchstack@ha
	lwz	1,cpu_hatchstack@l(3)

	bl	_C_LABEL(cpu_hatch)

/* Loop forever. */
1:
	sync
	mfmsr	3
	oris	3,3,PSL_POW@h
	mtmsr	3
	isync
	b	1b

#endif
