/*	$NetBSD: locore.S,v 1.3.2.6 2002/06/24 22:09:03 nathanw Exp $	*/
/*	$OpenBSD: locore.S,v 1.4 1997/01/26 09:06:38 rahnds Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

#undef PPC_4XX_NOCACHE

#include "opt_ddb.h"
#include "fs_kernfs.h"
#include "opt_ipkdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "opt_ppcarch.h"
#include "assym.h"

#include <sys/syscall.h>

#include <machine/param.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/asm.h>
#include <machine/dcr.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/pmap.h>

/*
 * Some instructions gas doesn't understand (yet?)
 */
#define	bdneq	bdnzf 2,

#define	INTSTK	(8*1024)	/* 8K interrupt stack */
#define	SPILLSTK 1024		/* 1K spill stack */

/*
 * Globals
 */
GLOBAL(proc0paddr)
	.long	0			/* proc0 p_addr */

GLOBAL(intrnames)
	.asciz	"clock", "irq1", "irq2", "irq3"
	.asciz	"irq4", "irq5", "irq6", "irq7"
	.asciz	"irq8", "irq9", "irq10", "irq11"
	.asciz	"irq12", "irq13", "irq14", "irq15"
	.asciz	"irq16", "irq17", "irq18", "irq19"
	.asciz	"irq20", "irq21", "irq22", "irq23"
	.asciz	"irq24", "irq25", "irq26", "irq27"
	.asciz	"irq28", "softnet", "softclock", "softserial"
	.asciz  "statclock"
GLOBAL(eintrnames)
	.align	4
GLOBAL(intrcnt)
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.long	0
GLOBAL(eintrcnt)

GLOBAL(powersave)
	.long	0

/*
 * File-scope for locore.S
 */
	.data
idle_u:
	.long	0			/* fake uarea during idle after exit */

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
	b	1f
	/* Reserve some space for info_block required for IBM eval board bootloader */
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

1:
	mr	31,3			/* Save address of PROM info_block */
	li	0,0
	mtmsr	0			/* Disable FPU/MMU/exceptions */
	isync

	/* PPC405GP errata, item #58.
	 * Load string instructions may write incorrect data into the last GPR
	 * targeted in the operation.
	 * Workaround: set OCM0_DSCNTL[DSEN]=0 and OCM0_DSCNTL[DOF]=0 */
	mtdcr	DCR_OCM0_DSCNTL, 0  	/* Disable Data access to OCM */
	mtdcr	DCR_OCM0_ISCNTL, 0  	/* Disable Instruction access to OCM. Just in case */
/*
 * Cpu detect.
 *
 */
__start_cpu0:
#ifdef PPC_4XX_NOCACHE
	/* Disable all caches for physical addresses */
	li	0,0
#else
	/* Allow cacheing for only the first 2GB of RAM */
	lis	0,0xffff
#endif
	mtdccr	0
	mticcr	0

	/* Invalidate all TLB entries */
	tlbia
	sync
	isync

/* compute end of kernel memory */
	lis	8,_C_LABEL(end)@ha
	addi	8,8,_C_LABEL(end)@l
#if defined(DDB) || defined(KERNFS)
	/* If we had symbol table location we'd store it here and would've adjusted r8 here */
	lis	7,_C_LABEL(startsym)@ha
	addi	7,7,_C_LABEL(startsym)@l
	stw	8,0(7)
	lis	7,_C_LABEL(endsym)@ha
	addi	7,7,_C_LABEL(endsym)@l
	stw	8,0(7)
#endif

	/*
	 * Set up TLB entry to cover kernel addresses.
	 *
	 * XXX: Skip TLB 0 for now, due to unresolved TLB 0 replacement
	 *	and hard hangs
	 */
	li	0,1
	mtpid	0
	sync

	li	0,0
#ifdef PPC_4XX_NOCACHE
	li	4,TLB_EX|TLB_WR|TLB_I /* |TLB_W */
#else
	li	4,TLB_EX|TLB_WR /* |TLB_W */
#endif
        li	3,TLB_VALID|TLB_PG_16M
        tlbwe	4,0,1			/* Load the data(Low) portion of the entry */
        tlbwe	3,0,0			/* Load the tag(High) portion of the entry */

#if 1
	/* Damn. Have to be able to access all real memory.... Hardcode for 32M for now. */
	li	0,1
	lis	4,0x01000000@h
	ori	3,4,0
#ifdef PPC_4XX_NOCACHE
	addi	4,4,TLB_EX|TLB_WR|TLB_I /* |TLB_W */
#else
	addi	4,4,TLB_EX|TLB_WR /* |TLB_W */
#endif
	addi	3,3,TLB_VALID|TLB_PG_16M
	tlbwe	4,0,1			/* Load the data(Low) portion of the entry */
	tlbwe	3,0,0			/* Load the tag(High) portion of the entry */
#endif

	/* set up a TLB mapping to cover uart0 */
	lis	3,0xef000000@h		/* Load the virtual address */
	ori	4,3,0			/* Load the physical address */

	clrrwi	4,4,10			/* Mask off the real page number */
	/* write, execute, cache inhibit, guarded */
	ori	4,4,(TLB_WR|TLB_EX|TLB_I|TLB_G)

	clrrwi	3,3,10			/* Mask off the effective page number */
	ori	3,3,(TLB_VALID|TLB_PG_16M)

	li	0,2

	tlbwe	4,0,1			/* Load the data portion of the entry */
	tlbwe	3,0,0			/* Load the tag portion of the entry */

	/* END of TLB setup */

	li	9,PGOFSET
	add	8,8,9
	andc	8,8,9
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

	lis	3,__start@ha
	addi	3,3,__start@l

	mr	6,31			/* info_block address */
	bl	_C_LABEL(initppc)
	bl	_C_LABEL(main)

loop:	b	loop			/* XXX not reached */
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

	wrteei	1			/* reenable ints again */
	
/* May do some power saving here? */
/* Check if we can use power saving mode */
	lis	8,_C_LABEL(powersave)@ha
	lwz	9,_C_LABEL(powersave)@l(8)

	or.	9,9,9
	beq	1f

	/* TODO:  Enter power saving mode here */
1:

	wrteei	0			/* disable interrupts while manipulating runque */ 

	b	_ASM_LABEL(Idle)

/*
 * switch_exit(struct lwp *l)
 * switch_exit gets called from cpu_exit to complete the exit procedure.
 */
ENTRY(switch_lwp_exit)
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
	bl	_C_LABEL(lwp_exit2)

	b	finish_switch_exit

/*
 * switch_exit(struct lwp *l)
 * switch_exit gets called from cpu_exit to complete the exit procedure.
 */
ENTRY(switch_exit)
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

finish_switch_exit:
/* Fall through to cpu_switch to actually select another proc */
	li	3,0			/* indicate exited process */

/*
 * void cpu_switch(struct lwp *l)
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
	lis	3,_C_LABEL(curlwp)@ha
	xor	31,31,31
	stw	31,_C_LABEL(curlwp)@l(3) /* Zero to not accumulate cpu time */
	lis	3,_C_LABEL(curpcb)@ha
	lwz	31,_C_LABEL(curpcb)@l(3)

	xor	3,3,3
	bl	_C_LABEL(lcsplx)
	stw	3,PCB_SPL(31)		/* save spl */

	wrteei	0			/* disable interrupts while manipulating runque */
	
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

	lwz	31,L_FORW(3)		/* unlink first proc from queue */
	lwz	4,L_FORW(31)
	stw	4,L_FORW(3)
	stw	3,L_BACK(4)

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

	stw	3,L_BACK(31)		/* probably superfluous */

	/* Process now running on a processor. */
	li	3,LSONPROC		/* l->l_stat = LSONPROC */
	stw	3,L_STAT(31)

	/* record new process */
	lis	4,_C_LABEL(curlwp)@ha
	stw	31,_C_LABEL(curlwp)@l(4)

	wrteei	1			/* Now we can interrupt again */

	li	3,0			/* if it's the same lwp, return 0 */
	cmpl	0,31,30			/* is it the same process? */
	beq	switch_return

	or.	30,30,30		/* old process was exiting? */
	beq	switch_exited

#ifndef PPC_IBM4XX
	mfsr	10,USER_SR		/* save USER_SR for copyin/copyout */
#else
	li	10,0			/* no SR for 4xx CPUs */
#endif
	mfcr	11			/* save cr */
	mr	12,2			/* save r2 */
	stwu	1,-SFRAMELEN(1)		/* still running on old stack */
	stmw	10,8(1)
	lwz	3,L_ADDR(30)
	stw	1,PCB_SP(3)		/* save SP */

switch_exited:
	wrteei	0			/* disable interrupts while actually switching */

	/* indicate new pcb */
	lwz	4,L_ADDR(31)
	lis	5,_C_LABEL(curpcb)@ha
	stw	4,_C_LABEL(curpcb)@l(5)

	/* save real pmap pointer for spill fill */
	lwz	5,PCB_PMR(4)
	lis	6,_C_LABEL(curpm)@ha
	stwu	5,_C_LABEL(curpm)@l(6)
	stwcx.	5,0,6			/* clear possible reservation */

	/* Switch to the new virtual space */
	/* Do not have to do anything here. TLB PID gets updated on return from trap. */

	lwz	1,PCB_SP(4)		/* get new procs SP */

	wrteei	1			/* interrupts are okay again */

	lmw	10,8(1)			/* get other regs */
	lwz	1,0(1)			/* get saved SP */
	mr	2,12			/* get saved r2 */
	mtcr	11			/* get saved cr */
	isync
	li	3,1			/* switched lwps */

switch_return:
	mr	30,3			/* save return value */

	lwz	3,PCB_SPL(4)
	bl	_C_LABEL(lcsplx)

0:
	lis	3,_C_LABEL(curpm)@ha
	addi	3,3,_C_LABEL(curpm)@l	/* Do we need a context? */
	lwz	4,PM_CTX(3)
	cmpwi	4,0
#	mtspr	SPR_SPR0,4		/* Always keep the current ctx here */
	mr	3,30
	bne	1f
	bl	_C_LABEL(ctx_alloc)
	b	0b			/* Reload */
1:
	lwz	31,12(1)
	lwz	30,8(1)
	addi	1,1,16
	lwz	0,4(1)
	mtlr	0
	blr

/*
 * void cpu_preempt(struct lwp *current, struct lwp *new))
 * Find a runnable process and switch to it.
 */
ENTRY(cpu_preempt)
	mflr	0			/* save lr */
	stw	0,4(1)
	stwu	1,-20(1)
	stw	31,16(1)
	stw	30,12(1)
	stw	29,8(1)

	mr	30,3
	mr	29,4
	lis	3,_C_LABEL(curlwp)@ha
	xor	31,31,31
	stw	31,_C_LABEL(curlwp)@l(3) /* Zero to not accumulate cpu time */
	lis	3,_C_LABEL(curpcb)@ha
	lwz	31,_C_LABEL(curpcb)@l(3)

	xor	3,3,3
	bl	_C_LABEL(lcsplx)
	stw	3,PCB_SPL(31)		/* save spl */

	wrteei	0			/* disable interrupts while manipulating runque */
	
/* Find a new process */
	xor	3,3,3

	mr	31,29			/* r31 = newlwp */
	lwz	4,L_BACK(31)		/* r4 = newlwp->l_back */
	stw	3,L_BACK(31)		/* newlwp->l_back = NULL */
	lwz	3,L_FORW(31)		/* r3 = newlwp->l_forw */
	stw	4,L_BACK(3)		/* r3->l_back = r4 */
	stw	3,L_FORW(4)		/* r4->l_forw = r3 */

	cmpl	0,4,3			/* queue empty? */
	bne	1f

	/* so now r31 is newlwp and r3/r4 are scratch again */

	lis	8,_C_LABEL(sched_whichqs)@ha
	lwz	4,_C_LABEL(sched_whichqs)@l(8)

	lwz	3,L_PRIORITY(31)
	li	9,2
	srw	3,3,9			/* r3 = newlwp->l_priority / 4 */
	li	9,1
	slw	3,9,3			/* r3 = 1 << r3 */
	andc	4,4,3			/* sched_whichqs &= ~r3 */
	stw	4,_C_LABEL(sched_whichqs)@l(8) /* mark it empty */

1:
	/* just did this resched thing */
	xor	3,3,3
	lis	4,_C_LABEL(want_resched)@ha
	stw	3,_C_LABEL(want_resched)@l(4)

	stw	3,L_BACK(31)		/* probably superfluous */

	/* Process now running on a processor. */
	li	3,LSONPROC		/* l->l_stat = LSONPROC */
	stw	3,L_STAT(31)

	/* record new process */
	lis	4,_C_LABEL(curlwp)@ha
	stw	31,_C_LABEL(curlwp)@l(4)

	wrteei	1			/* Now we can interrupt again */

	cmpl	0,31,30			/* is it the same process? */
	beq	preempt_return

	or.	30,30,30		/* old process was exiting? */
	beq	preempt_exited

#ifndef PPC_IBM4XX
	mfsr	10,USER_SR		/* save USER_SR for copyin/copyout */
#else
	li	10,0			/* no SR for 4xx CPUs */
#endif
	mfcr	11			/* save cr */
	mr	12,2			/* save r2 */
	stwu	1,-SFRAMELEN(1)		/* still running on old stack */
	stmw	10,8(1)
	lwz	3,L_ADDR(30)
	stw	1,PCB_SP(3)		/* save SP */

preempt_exited:
	wrteei	0			/* disable interrupts while actually switching */

	/* indicate new pcb */
	lwz	4,L_ADDR(31)
	lis	5,_C_LABEL(curpcb)@ha
	stw	4,_C_LABEL(curpcb)@l(5)

	/* save real pmap pointer for spill fill */
	lwz	5,PCB_PMR(4)
	lis	6,_C_LABEL(curpm)@ha
	stwu	5,_C_LABEL(curpm)@l(6)
	stwcx.	5,0,6			/* clear possible reservation */

	/* Switch to the new virtual space */
	/* Do not have to do anything here. TLB PID gets updated on return from trap. */

	lwz	1,PCB_SP(4)		/* get new procs SP */

	wrteei	1			/* interrupts are okay again */

	lmw	10,8(1)			/* get other regs */
	lwz	1,0(1)			/* get saved SP */
	mr	2,12			/* get saved r2 */
	mtcr	11			/* get saved cr */
	isync

preempt_return:
	lwz	3,PCB_SPL(4)
	bl	_C_LABEL(lcsplx)

0:
	lis	3,_C_LABEL(curpm)@ha
	addi	3,3,_C_LABEL(curpm)@l	/* Do we need a context? */
	lwz	4,PM_CTX(3)
	cmpwi	4,0
#	mtspr	SPR_SPR0,4		/* Always keep the current ctx here */
	mr	3,30
	bne	1f
	bl	_C_LABEL(ctx_alloc)
	b	0b			/* Reload */
1:
	lwz	31,16(1)
	lwz	30,12(1)
	lwz	29,8(1)
	addi	1,1,20
	lwz	0,4(1)
	mtlr	0
	blr

/*
 * Child comes here at the end of a fork.
 * Return to userspace via the trap return path.
 */
	.globl	_C_LABEL(fork_trampoline)
_C_LABEL(fork_trampoline):
	xor	3,3,3
	bl	_C_LABEL(lcsplx)
	mtlr	31
	mr	3,30
	blrl				/* jump indirect to r31 */
	b	trapexit

/*
 * Pull in common trap vector code.
 */
#include <powerpc/ibm4xx/trap_subr.S>
#include <powerpc/ibm4xx/4xx_trap_subr.S>

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

	.globl	_C_LABEL(ppc4xx_reset)
_C_LABEL(ppc4xx_reset):
	mfspr	3,SPR_DBCR0
	oris	3,r13,DBCR0_RST_SYSTEM@h
	mtspr	SPR_DBCR0,3
	ba	0

#if 0
/*
 * XXXX the following doesn't quite work right yet.
 */
/*
 * void bcopy(const void *src, void *dst, size_t len);
 *
 * swap r3 and r4 and fall through to memcopy.
 */
	.globl _C_LABEL(bcopy)
_C_LABEL(bcopy):
	mr	r0,r3
	mr	r3,r4
	mr	r4,r0
	/* FALLTHROUGH */

/*
 * void * memcpy(void *dst (r3), const void *src (r4), size_t len (r5));
 *
 * Copy memory (obviously)
 *
 * We will try to do data cache block aligned stores so we
 * can use block allocate and not have to read from the
 * destination.
 *
 * Register use:
 *
 *	r1		stack (of course)
 *	r3		dst
 *	r4		src
 *	r5		len
 *	r6		tmp
 *	r7		holds 32
 *	r8		holds dst
 *	r24-r31		block move regs
 *
 */

ENTRY(memcpy)
	stwu	r1,-(10*4)(r1)	/* Allocate some RAM to save 8 regs to. */
	cmpwi	r5, 32		/* Less than 32 bytes ? */
	stmw	r24,8(r1)	/* Save ALL regs (could be optimized) */

	mr	r8,r3		/* save dst */
	li	r7,32

	dcbt	0,r4		/* Start bringing in cache line. */
	blt	1f		/* Finish up */

	neg	r6,r3		/* Find how far unaligned we are... */
	andi.	r6,r6,31	/* Cache-align dest. */
	mtxer	r6
	sub	r5,r5,r6	/* subtract count */
	lswx	r24,0,r4	/* Load some. */
	add	r4,r4,r6
	dcbt	0,r4		/* Fetch next line */
	stswx	r24,0,r3	/* Store some */
	add	r3,r3,r6
	addic.	r6,r5,-32	/* Pre-decrement next line */
	ble	1f		/* Less than 32-bytes? finishup */

	/* Dest should not be cache line aligned. */
	/* XXX need gas 2.11 to grok dcba insn */
#ifdef GAS_2_11
	dcba	0,r3		/* Allocate a line */
#else
	.long	0x7c001dec	/* dcba	0,r3 */
#endif
0:
	dcbt	r7,r4		/* Bring in the next line, too */

	lswi	r24,r4,32
	addi	r4,r4,32	/* Inc src */
	mr	r5,r6

	addic.	r6,r5,-32
	stswi	r24,r3,32
	addi	r3,r3,32	/* Inc dst */
#ifdef GAS_2_11
	dcba	0,r3		/* Allocate another line */
#else
	.long	0x7c071dec	/* dcba	r7,r3 */
#endif	
	bgt	0b
1:
	mtxer	r5		/* Store byte count */
	lswx	r24,0,r4	/* Load up to 32 bytes */
	stswx	r24,0,r3	/* Store up to 32 bytes */

	mr	r3,r8		/* Return dst */

	lmw	r24,8(r1)
	addi	r1,r1,(10*4)
	blr
#endif
