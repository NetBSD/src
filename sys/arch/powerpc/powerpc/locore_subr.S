/*	$NetBSD: locore_subr.S,v 1.14 2003/07/31 15:30:41 matt Exp $	*/

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
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"

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
	lis	%r8,_C_LABEL(sched_whichqs)@ha
	lwz	%r9,_C_LABEL(sched_whichqs)@l(%r8)

	or.	%r9,%r9,%r9
	bne+	.Lsw1			/* at least one queue non-empty */

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	bl	_C_LABEL(sched_unlock_idle)
#endif

#if defined(PPC_IBM4XX)
	wrteei	1			/* reenable ints again */
#else /* PPC_OEA */
	mfmsr	%r3
	ori	%r3,%r3,PSL_EE@l	/* reenable ints again */
	mtmsr	%r3
	isync
#endif

/* Check if we can use power saving mode */
	lis	%r8,_C_LABEL(powersave)@ha
	lwz	%r9,_C_LABEL(powersave)@l(%r8)

	add.	%r9,%r9,%r9
	ble	1f

#if defined(PPC_OEA)
	sync
	oris	%r3,%r3,PSL_POW@h	/* enter power saving mode */
	mtmsr	%r3
	isync
#endif /* PPC_OEA */

1:
#if defined(PPC_IBM4XX)
	wrteei	0			/* disable interrupts while
					   manipulating runque */
#else /* PPC_OEA */
	andi.	%r3,%r3,~PSL_EE@l	/* disable interrupts while
					   manipulating runque */
	mtmsr	%r3
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
	GET_CPUINFO(%r7)
	lwz	%r6,CI_IDLE_PCB(%r7)
	stw	%r6,CI_CURPCB(%r7)

	/*
	 * Adjust the stack to provide space for the callee to save LR.
	 */
	addi	%r1,%r6,USPACE-CALLFRAMELEN
	/*
	 * Schedule the vmspace and stack to be freed (the proc arg is
	 * already in r3).  Function to call is in r4.
	 */
	mtctr	%r4
	bctrl

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	bl	_C_LABEL(sched_lock_idle)
#endif

/* Fall through to cpu_switch to actually select another proc */
	li	%r3,0			/* indicate exited process */

/*
 * void cpu_switch(struct lwp *l)
 * Find a runnable process and switch to it.
 */
ENTRY_NOPROFILE(cpu_switch)
	mflr	%r0			/* save lr */
	stw	%r0,SZREG(%r1)
	stwu	%r1,-CALLFRAMELEN(%r1)
	stw	%r31,(3*SZREG)(%r1)
	stw	%r30,(2*SZREG)(%r1)

	mr	%r30,%r3
#if defined(MULTIPROCESSOR)
	/* Switch to the idle PCB unless we're already running on it. */
	GET_CPUINFO(%r7)
	cmpwi	%r30,0			/* old process was exiting? */
	beq	1f

#if defined(PPC_OEA) && !defined(_LP64)
	mfsr	%r10,USER_SR		/* save USER_SR for copyin/copyout */
#else
	li	%r10,0			/* no USER_SR needed */
#endif
	mfcr	%r11			/* save cr */
	mr	%r12,%r2		/* save r2 */
	stwu	%r1,-SFRAMELEN(%r1)	/* still running on old stack */
	stmw	%r10,(2*SZREG)(%r1)	/* save USER_SR, CR, R2, non-volatile */
	lwz	%r3,L_ADDR(%r30)	/* get PCB address */
	stw	%r1,PCB_SP(%r3)		/* save SP */

	lwz	%r6,CI_IDLE_PCB(%r7)
	addi	%r1,%r6,USPACE-CALLFRAMELEN /* callframe rsvd at stack top */

1:
	li	%r31,0
	stw	%r31,CI_CURLWP(%r7)	/* Zero to not accumulate cpu time */
	lwz	%r31,CI_CURPCB(%r7)

	lwz	%r3,CI_CPL(%r7)
	stw	%r3,PCB_SPL(%r31)	/* save spl */
#else
	GET_CPUINFO(%r3)
	li	%r31,0
	stw	%r31,CI_CURLWP(%r3)	/* Zero to not accumulate cpu time */
	lwz	%r31,CI_CURPCB(%r3)
#endif

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
/* Release the sched_lock before processing interrupts. */
	bl	_C_LABEL(sched_unlock_idle)
#endif

	li	%r3,0			/* spl0() */
	bl	_C_LABEL(lcsplx)
#if !defined(MULTIPROCESSOR)
	stw	%r3,PCB_SPL(%r31)	/* save spl */
#endif

/* Lock the scheduler. */
#if defined(PPC_IBM4XX)
	wrteei	0			/* disable interrupts while
					   manipulating runque */
#else /* PPC_OEA */
	mfmsr	%r3
	andi.	%r3,%r3,~PSL_EE@l	/* disable interrupts while
					   manipulating runque */
	mtmsr	%r3
	isync
#endif
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	bl	_C_LABEL(sched_lock_idle)
#endif

/* Find a new process */
	lis	%r8,_C_LABEL(sched_whichqs)@ha
	lwz	%r9,_C_LABEL(sched_whichqs)@l(%r8)

	or.	%r9,%r9,%r9
	beq-	_ASM_LABEL(Idle)	/* all queues empty */
.Lsw1:
	cntlzw	%r10,%r9
	lis	%r4,_C_LABEL(sched_qs)@ha
	addi	%r4,%r4,_C_LABEL(sched_qs)@l
	slwi	%r3,%r10,3
	add	%r3,%r3,%r4		/* select queue */

	lwz	%r31,L_FORW(%r3)	/* unlink first proc from queue */
	lwz	%r4,L_FORW(%r31)
	stw	%r4,L_FORW(%r3)
	stw	%r3,L_BACK(%r4)

	cmpl	%cr0,%r3,%r4		/* queue empty? */
	bne	1f

	lis	%r3,0x80000000@h
	srw	%r3,%r3,%r10
	andc	%r9,%r9,%r3
	stw	%r9,_C_LABEL(sched_whichqs)@l(%r8) /* mark it empty */

switch_common:
1:
	/* just did this resched thing */
	li	%r3,0
	GET_CPUINFO(%r4)
	stw	%r3,CI_WANT_RESCHED(%r4)
	stw	%r3,L_BACK(%r31)	/* probably superfluous */

#ifdef MULTIPROCESSOR
	stw	%r4,L_CPU(%r31)		/* l->l_cpu = curcpu() */
#endif

	/* Process now running on a processor. */
	li	%r3,LSONPROC		/* l->l_stat = LSONPROC */
	stw	%r3,L_STAT(%r31)

	/* record new process */
	stw	%r31,CI_CURLWP(%r4)
	lwz	%r4,L_ADDR(%r31)

#if !defined(MULTIPROCESSOR)		/* XXX */
	li	%r3,0			/* if it is the same lwp, return 0 */
	cmpl	%cr0,%r31,%r30		/* is it the same lwp? */
	beq	switch_return

	or.	%r30,%r30,%r30		/* old lwp was exiting? */
	beq	switch_exited

#if defined(PPC_OEA) && !defined(_LP64)
	mfsr	%r10,USER_SR		/* save USER_SR for copyin/copyout */
#else /* PPC_OEA */
	li	%r10,0			/* no SR needed */
#endif
	mfcr	%r11			/* save cr */
	mr	%r12,%r2		/* save r2 */
	stwu	%r1,-SFRAMELEN(%r1)	/* still running on old stack */
	stmw	%r10,(2*SZREG)(%r1)
	lwz	%r3,L_ADDR(%r30)
	stw	%r1,PCB_SP(%r3)		/* save SP */

switch_exited:
#endif

	/* indicate new pcb */
	GET_CPUINFO(%r6)
	stw	%r4,CI_CURPCB(%r6)

	/* save real pmap pointer for spill fill */
	lwz	%r5,PCB_PMR(%r4)
	stwu	%r5,CI_CURPM(%r6)
	stwcx.	%r5,%r0,%r6		/* clear possible reservation */
	isync

	lwz	%r1,PCB_SP(%r4)		/* get new procs SP */
	lmw	%r10,(2*SZREG)(%r1)	/* get non-volatile, CR, R2, USER_SR */
	lwz	%r1,0(%r1)		/* get saved SP */
	mr	%r2,%r12		/* get saved r2 */
	mtcr	%r11			/* get saved cr */
	isync
#if defined(PPC_OEA) && !defined(_LP64)
	mtsr	USER_SR,%r10		/* get saved USER_SR */
	isync
#endif

	li	%r3,1			/* switched lwps */

switch_return:
	mr	%r30,%r3		/* save return value */
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	/* Unlock the sched_lock, but leave interrupts off, for now. */
	bl	_C_LABEL(sched_unlock_idle)
#endif

#if defined(PPC_IBM4XX)
	wrteei	1			/* interrupts are okay again */
#else /* PPC_OEA */
	mfmsr	%r3
	ori	%r3,%r3,PSL_EE@l	/* interrupts are okay again */
	mtmsr	%r3
#endif

	lwz	%r3,PCB_SPL(%r4)
	bl	_C_LABEL(lcsplx)

#if defined(PPC_IBM4XX)
0:
	GET_CPUINFO(%r3)
	lwz	%r3,CI_CURPM(%r3)	/* Do we need a context? */
	lwz	%r4,PM_CTX(%r3)
	cmpwi	%r4,0
#	mtspr	SPR_SPR0,4		/* Always keep the current ctx here */
	bne	1f
	bl	_C_LABEL(ctx_alloc)
	b	0b			/* reload */
1:
#endif
	mr	%r3,%r30		/* restore return value */

	lwz	%r31,(3*SZREG)(%r1)
	lwz	%r30,(2*SZREG)(%r1)
	addi	%r1,1,CALLFRAMELEN
	lwz	%r0,SZREG(%r1)
	mtlr	%r0
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
	mflr	%r0			/* save lr */
	stw	%r0,SZREG(%r1)
	stwu	%r1,-CALLFRAMELEN(%r1)
	stw	%r31,(3*SZREG)(%r1)
	stw	%r30,(2*SZREG)(%r1)

	stwu	%r1,-CALLFRAMELEN(%r1)
	stw	%r29,(2*SZREG)(%r1)
	mr	%r30,%r3			/* r30 = curlwp */
	mr	%r29,%r4			/* r29 = newlwp */

#if defined(MULTIPROCESSOR)
	/* Switch to the idle PCB unless we're already running on it. */
	GET_CPUINFO(%r7)
	cmpwi	%r30,0			/* old process was exiting? */
	beq	1f

#if defined(PPC_OEA) && !defined(_LP64)
	mfsr	%r10,USER_SR		/* save USER_SR for copyin/copyout */
#else
	li	%r10,0			/* USER_SR not needed */
#endif
	mfcr	%r11			/* save cr */
	mr	%r12,%r2		/* save r2 */
	stwu	%r1,-SFRAMELEN(%r1)	/* still running on old stack */
	stmw	%r10,(2*SZREG)(%r1)	/* save USER_SR, CR, R2, non-volatile */
	lwz	%r3,L_ADDR(%r30)	/* get PCB */
	stw	%r1,PCB_SP(%r3)		/* save SP */

	lwz	%r6,CI_IDLE_PCB(%r7)
	addi	%r1,%r6,USPACE-CALLFRAMELEN /* callframe rsvd at stack top */

1:
	li	%r31,0
	stw	%r31,CI_CURLWP(%r7)	/* Zero to not accumulate cpu time */
	lwz	%r31,CI_CURPCB(%r7)

	lwz	%r3,CI_CPL(%r7)
	stw	%r3,PCB_SPL(%r31)	/* save spl */
#else
	GET_CPUINFO(%r3)
	li	%r31,0
	stw	%r31,CI_CURLWP(%r3)	/* Zero to not accumulate cpu time */
	lwz	%r31,CI_CURPCB(%r3)
#endif

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
/* Release the sched_lock before processing interrupts. */
	bl	_C_LABEL(sched_unlock_idle)
#endif

	li	%r3,0
	bl	_C_LABEL(lcsplx)
#if !defined(MULTIPROCESSOR)
	stw	%r3,PCB_SPL(%r31)	/* save spl */
#endif

/* Lock the scheduler. */
#if defined(PPC_IBM4XX)
	wrteei	0			/* disable interrupts while
					   manipulating runque */
#else /* PPC_OEA */
	mfmsr	%r3
	andi.	%r3,%r3,~PSL_EE@l	/* disable interrupts while
					   manipulating runque */
	mtmsr	%r3
	isync
#endif
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	bl	_C_LABEL(sched_lock_idle)
#endif

	/* Make stack normal, r31 newlwp, r30 oldlwp */
	mr	%r31,%r29
	lwz	%r29,(2*SZREG)(%r1)
	addi	%r1,%r1,CALLFRAMELEN

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
	li	%r3,0
	bl	_C_LABEL(lcsplx)
	mtlr	%r31
	mr	%r3,%r30
	blrl				/* jump indirect to r31 */
	b	trapexit
