/*	$NetBSD: locore_subr.S,v 1.12 2003/03/12 06:00:36 matt Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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

/*
 * NOTICE: This is not a standalone file.  to use it, #include it in
 * your port's locore.S, like so:
 *
 *	#include <powerpc/powerpc/locore_subr.S>
 */

#include "opt_ppcarch.h"

	.data
GLOBAL(powersave)
	.long	-1

	.text
	.align 2
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

#if defined(PPC_IBM4XX)
	wrteei	1			/* reenable ints again */
#else /* PPC_OEA */
	mfmsr	3
	ori	3,3,PSL_EE@l		/* reenable ints again */
	mtmsr	3
	isync
#endif

/* Check if we can use power saving mode */
	lis	8,_C_LABEL(powersave)@ha
	lwz	9,_C_LABEL(powersave)@l(8)

	add.	9,9,9
	ble	1f

#if defined(PPC_OEA)
	sync
	oris	3,3,PSL_POW@h		/* enter power saving mode */
	mtmsr	3
	isync
#endif /* PPC_OEA */

1:
#if defined(PPC_IBM4XX)
	wrteei	0			/* disable interrupts while
					   manipulating runque */
#else /* PPC_OEA */
	andi.	3,3,~PSL_EE@l		/* disable interrupts while
					   manipulating runque */
	mtmsr	3
#endif

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	bl	_C_LABEL(sched_lock_idle)
#endif
	b	_ASM_LABEL(Idle)

/*
 * switch_exit(struct lwp *l, void (*exit)(struct lwp *))
 * switch_exit gets called from cpu_exit to complete the exit procedure.
 */
ENTRY(switch_exit)
/* First switch to the idle pcb/kernel stack */
	GET_CPUINFO(7)
	lwz	6,CI_IDLE_PCB(7)
	stw	6,CI_CURPCB(7)

	/*
	 * Adjust the stack to provide space for the callee to save LR.
	 */
	addi	1,6,USPACE-16
	/*
	 * Schedule the vmspace and stack to be freed (the proc arg is
	 * already in r3).  Function to call is in r4.
	 */
	mtctr	4
	bctrl

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	bl	_C_LABEL(sched_lock_idle)
#endif

/* Fall through to cpu_switch to actually select another proc */
	li	3,0			/* indicate exited process */

/*
 * void cpu_switch(struct lwp *l)
 * Find a runnable process and switch to it.
 */
ENTRY_NOPROFILE(cpu_switch)
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
	lwz	3,L_ADDR(30)
	stw	1,PCB_SP(3)		/* save SP */

	lwz	6,CI_IDLE_PCB(7)
	addi	1,6,USPACE-16		/* 16 bytes are reserved at stack top */

1:
	li	31,0
	stw	31,CI_CURLWP(7)		/* Zero to not accumulate cpu time */
	lwz	31,CI_CURPCB(7)

	lwz	3,CI_CPL(7)
	stw	3,PCB_SPL(31)		/* save spl */
#else
	GET_CPUINFO(3)
	li	31,0
	stw	31,CI_CURLWP(3)		/* Zero to not accumulate cpu time */
	lwz	31,CI_CURPCB(3)
#endif

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
/* Release the sched_lock before processing interrupts. */
	bl	_C_LABEL(sched_unlock_idle)
#endif

	li	3,0			/* spl0() */
	bl	_C_LABEL(lcsplx)
#if !defined(MULTIPROCESSOR)
	stw	3,PCB_SPL(31)		/* save spl */
#endif

/* Lock the scheduler. */
#if defined(PPC_IBM4XX)
	wrteei	0			/* disable interrupts while
					   manipulating runque */
#else /* PPC_OEA */
	mfmsr	3
	andi.	3,3,~PSL_EE@l		/* disable interrupts while
					   manipulating runque */
	mtmsr	3
	isync
#endif
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

switch_common:
1:
	/* just did this resched thing */
	li	3,0
	GET_CPUINFO(4)
	stw	3,CI_WANT_RESCHED(4)
	stw	3,L_BACK(31)		/* probably superfluous */

#ifdef MULTIPROCESSOR
	stw	4,L_CPU(31)		/* l->l_cpu = curcpu() */
#endif

	/* Process now running on a processor. */
	li	3,LSONPROC		/* l->l_stat = LSONPROC */
	stw	3,L_STAT(31)

	/* record new process */
	stw	31,CI_CURLWP(4)
	lwz	4,L_ADDR(31)

#if !defined(MULTIPROCESSOR)		/* XXX */
	li	3,0			/* if it is the same lwp, return 0 */
	cmpl	0,31,30			/* is it the same lwp? */
	beq	switch_return

	or.	30,30,30		/* old lwp was exiting? */
	beq	switch_exited

#if defined(PPC_IBM4XX)
	li	10,0			/* no SR for 4xx CPUs */
#else /* PPC_OEA */
	mfsr	10,USER_SR		/* save USER_SR for copyin/copyout */
#endif
	mfcr	11			/* save cr */
	mr	12,2			/* save r2 */
	stwu	1,-SFRAMELEN(1)		/* still running on old stack */
	stmw	10,8(1)
	lwz	3,L_ADDR(30)
	stw	1,PCB_SP(3)		/* save SP */

switch_exited:
#endif

	/* indicate new pcb */
	GET_CPUINFO(6)
	stw	4,CI_CURPCB(6)

	/* save real pmap pointer for spill fill */
	lwz	5,PCB_PMR(4)
	stwu	5,CI_CURPM(6)
	stwcx.	5,0,6			/* clear possible reservation */
	isync

	lwz	1,PCB_SP(4)		/* get new procs SP */
	lmw	10,8(1)			/* get other regs */
	lwz	1,0(1)			/* get saved SP */
	mr	2,12			/* get saved r2 */
	mtcr	11			/* get saved cr */
	isync
#if defined(PPC_IBM4XX)
	/*
	 * Don't have to do anything here; TLB PID gets updated on
	 * return from trap.
	 */
#else /* PPC_OEA */
	mtsr	USER_SR,10		/* get saved USER_SR */
	isync
#endif

	li	3,1			/* switched lwps */

switch_return:
	mr	30,3			/* save return value */
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	/* Unlock the sched_lock, but leave interrupts off, for now. */
	bl	_C_LABEL(sched_unlock_idle)
#endif

#if defined(PPC_IBM4XX)
	wrteei	1			/* interrupts are okay again */
#else /* PPC_OEA */
	mfmsr	3
	ori	3,3,PSL_EE@l		/* interrupts are okay again */
	mtmsr	3
#endif

	lwz	3,PCB_SPL(4)
	bl	_C_LABEL(lcsplx)

#if defined(PPC_IBM4XX)
0:
	GET_CPUINFO(3)
	lwz	3,CI_CURPM(3)		/* Do we need a context? */
	lwz	4,PM_CTX(3)
	cmpwi	4,0
#	mtspr	SPR_SPR0,4		/* Always keep the current ctx here */
	bne	1f
	bl	_C_LABEL(ctx_alloc)
	b	0b			/* reload */
1:
#endif
	mr	3,30			/* restore return value */

	lwz	31,12(1)
	lwz	30,8(1)
	addi	1,1,16
	lwz	0,4(1)
	mtlr	0
	blr

/*
 * void
 * cpu_switchto(struct lwp *current, struct lwp *new)
 * Variant of cpu_switch, but instead of picking a new LWP from the run
 * queue, switch to the indicated new LWP.
 * 	r3 - current LWP
 * 	r4 - LWP to switch to
 */
ENTRY(cpu_switchto)
	mflr	0			/* save lr */
	stw	0,4(1)
	stwu	1,-16(1)
	stw	31,12(1)
	stw	30,8(1)

	stwu	1,-16(1)
	stw	29,8(1)
	mr	30,3			/* r30 = curlwp */
	mr	29,4			/* r29 = newlwp */

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
	lwz	3,L_ADDR(30)
	stw	1,PCB_SP(3)		/* save SP */

	lwz	6,CI_IDLE_PCB(7)
	addi	1,6,USPACE-16		/* 16 bytes are reserved at stack top */

1:
	li	31,0
	stw	31,CI_CURLWP(7)		/* Zero to not accumulate cpu time */
	lwz	31,CI_CURPCB(7)

	lwz	3,CI_CPL(7)
	stw	3,PCB_SPL(31)		/* save spl */
#else
	GET_CPUINFO(3)
	li	31,0
	stw	31,CI_CURLWP(3)		/* Zero to not accumulate cpu time */
	lwz	31,CI_CURPCB(3)
#endif

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
/* Release the sched_lock before processing interrupts. */
	bl	_C_LABEL(sched_unlock_idle)
#endif

	li	3,0
	bl	_C_LABEL(lcsplx)
#if !defined(MULTIPROCESSOR)
	stw	3,PCB_SPL(31)		/* save spl */
#endif

/* Lock the scheduler. */
#if defined(PPC_IBM4XX)
	wrteei	0			/* disable interrupts while
					   manipulating runque */
#else /* PPC_OEA */
	mfmsr	3
	andi.	3,3,~PSL_EE@l		/* disable interrupts while
					   manipulating runque */
	mtmsr	3
	isync
#endif
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	bl	_C_LABEL(sched_lock_idle)
#endif

	/* Make stack normal, r31 newlwp, r30 oldlwp */
	mr	31,29
	lwz	29,8(1)
	addi	1,1,16

	b	switch_common
/* NOTREACHED */

/*
 * Child comes here at the end of a fork.
 * Return to userspace via the trap return path.
 */
	.globl	_C_LABEL(fork_trampoline)
_C_LABEL(fork_trampoline):
#if defined(MULTIPROCESSOR)
	bl	_C_LABEL(proc_trampoline_mp)
#endif
	li	3,0
	bl	_C_LABEL(lcsplx)
	mtlr	31
	mr	3,30
	blrl				/* jump indirect to r31 */
	b	trapexit
