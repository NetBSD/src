/*	$NetBSD: locore_subr.S,v 1.34 2008/02/05 18:10:48 garbled Exp $	*/

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
#include "opt_ddb.h"

#ifdef DDB
#define	CFRAME_LRSAVE(t0)					\
	bl	90f;				/* get the LR */\
90:	mflr	%r0;						\
	streg	%r0,(CFRAME_LR)(t0)
#else
#define	CFRAME_LRSAVE(t0)	/* nothing */
#endif

/*
 * We don't save r30&r31 since they saved in the callframe.
 * We also want the "current" value of r30 instead of the saved value
 * since we need to return the LWP that ran before us, not ourselves.
 * if we save r30, when we got restored, that would be the r30 that
 * would have been saved when we were replaced which would be ourself.
 */
#define	SWITCHFRAME_SAVE(t0)					\
	streg	%r10,(SFRAME_USER_SR)(t0);	/* USER_SR */	\
	streg	%r11,(SFRAME_CR)(t0);		/* CR */	\
	streg	%r12,(SFRAME_R2)(t0);		/* R2 */	\
	streg	%r13,(SFRAME_R13)(t0);		/* volatile */	\
	streg	%r14,(SFRAME_R14)(t0);				\
	streg	%r15,(SFRAME_R15)(t0);				\
	streg	%r16,(SFRAME_R16)(t0);				\
	streg	%r17,(SFRAME_R17)(t0);				\
	streg	%r18,(SFRAME_R18)(t0);				\
	streg	%r19,(SFRAME_R19)(t0);				\
	streg	%r20,(SFRAME_R20)(t0);				\
	streg	%r21,(SFRAME_R21)(t0);				\
	streg	%r22,(SFRAME_R22)(t0);				\
	streg	%r23,(SFRAME_R23)(t0);				\
	streg	%r24,(SFRAME_R24)(t0);				\
	streg	%r25,(SFRAME_R25)(t0);				\
	streg	%r26,(SFRAME_R26)(t0);				\
	streg	%r27,(SFRAME_R27)(t0);				\
	streg	%r28,(SFRAME_R28)(t0);				\
	streg	%r29,(SFRAME_R29)(t0)

#define	SWITCHFRAME_RESTORE(t0)					\
	ldreg	%r10,(SFRAME_USER_SR)(t0);	/* USER_SR */	\
	ldreg	%r11,(SFRAME_CR)(t0);		/* CR */	\
	ldreg	%r12,(SFRAME_R2)(t0);		/* R2 */	\
	ldreg	%r13,(SFRAME_R13)(t0);		/* volatile */	\
	ldreg	%r14,(SFRAME_R14)(t0);				\
	ldreg	%r15,(SFRAME_R15)(t0);				\
	ldreg	%r16,(SFRAME_R16)(t0);				\
	ldreg	%r17,(SFRAME_R17)(t0);				\
	ldreg	%r18,(SFRAME_R18)(t0);				\
	ldreg	%r19,(SFRAME_R19)(t0);				\
	ldreg	%r20,(SFRAME_R20)(t0);				\
	ldreg	%r21,(SFRAME_R21)(t0);				\
	ldreg	%r22,(SFRAME_R22)(t0);				\
	ldreg	%r23,(SFRAME_R23)(t0);				\
	ldreg	%r24,(SFRAME_R24)(t0);				\
	ldreg	%r25,(SFRAME_R25)(t0);				\
	ldreg	%r26,(SFRAME_R26)(t0);				\
	ldreg	%r27,(SFRAME_R27)(t0);				\
	ldreg	%r28,(SFRAME_R28)(t0);				\
	ldreg	%r29,(SFRAME_R29)(t0)

	.data
GLOBAL(powersave)
	.long	-1

	.text
	.align 2
/*
 * struct lwp *
 * cpu_switchto(struct lwp *current, struct lwp *new)
 * switch to the indicated new LWP.
 * 	r3 - current LWP (maybe NULL, if so don't save state)
 * 	r4 - LWP to switch to
 *	scheduler lock held
 *	SPL is IPL_SCHED.
 */
ENTRY(cpu_switchto)
	mflr	%r0			/* save lr */
	streg	%r0,CFRAME_LR(%r1)
	stptru	%r1,-CALLFRAMELEN(%r1)
	streg	%r31,CFRAME_R31(%r1)
	streg	%r30,CFRAME_R30(%r1)
	mr	%r30,%r3		/* r30 = curlwp */
	mr	%r31,%r4		/* r31 = newlwp */

	/* 
	 * If the oldlwp was null, don't bother saving the switch state.
	 */
	cmpwi	%r30,0
	beq	switchto_restore

#if !defined(PPC_OEA64) && !defined (PPC_IBM4XX)
	mfsr	%r10,USER_SR		/* save USER_SR for copyin/copyout */
#else
	li	%r10,0			/* USER_SR not needed */
#endif
	mfcr	%r11			/* save cr */
	mr	%r12,%r2		/* save r2 */
	CFRAME_LRSAVE(%r1)
	stptru	%r1,-SFRAMELEN(%r1)	/* still running on old stack */
	SWITCHFRAME_SAVE(%r1)		/* save USER_SR, CR, R2, non-volatile */
	ldptr	%r4,L_ADDR(%r30)	/* put PCB addr in r4 */
	streg	%r1,PCB_SP(%r4)		/* store old lwp's SP */

switchto_restore:
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

	/*
	 * r31 = lwp now running on this cpu
	 * r30 = previous lwp (maybe be NULL)
	 * scheduler lock is held.
	 * spl is IPL_SCHED.
	 * MSR[EE] == 0 (interrupts are off)
	 */

	GET_CPUINFO(%r7)
	stptr	%r31,CI_CURLWP(%r7)
	ldptr	%r4,L_ADDR(%r31)	/* put PCB addr in r4 */
	stptr	%r4,CI_CURPCB(%r7)	/* using a new pcb */
	ldptr	%r3,PCB_PM(%r4)
	stptr	%r3,CI_CURPM(%r7)	/* and maybe a new pmap */

	/*
	 * Now restore the register state.
	 */
	ldreg	%r1,PCB_SP(%r4)		/* get new lwp's SP */
	SWITCHFRAME_RESTORE(%r1)	/* get non-volatile, CR, R2, USER_SR */
	ldreg	%r1,0(%r1)		/* get saved SP */
	mr	%r2,%r12		/* get saved r2 */
	mtcr	%r11			/* get saved cr */
#if !defined(PPC_OEA64) && !defined (PPC_IBM4XX)
	mtsr	USER_SR,%r10		/* get saved USER_SR */
#endif
	isync

#if defined(PPC_IBM4XX)
	wrteei	1			/* interrupts are okay again */
#else /* PPC_OEA */
	mfmsr	%r4
	ori	%r4,%r4,PSL_EE@l	/* interrupts are okay again */
	mtmsr	%r4
#endif

#if defined(PPC_IBM4XX)
0:
	GET_CPUINFO(%r3)
	ldreg	%r3,CI_CURPM(%r3)	/* Do we need a context? */
	ldreg	%r4,PM_CTX(%r3)
	cmpwi	%r4,0
#	mtspr	SPR_SPR0,4		/* Always keep the current ctx here */
	bne	1f
	bl	_C_LABEL(ctx_alloc)
	b	0b			/* reload */
1:
#endif

	/*
	 * Move back old-lwp and new-lwp to r3 and r4.  We need to return
	 * r3.  However, lwp_startup needs r4 and we return to fork_trampoline
	 * will directly invoke lwp_startup.  So we "waste" an instruction by
	 * always doing it here.
	 */
	mr	%r3,%r30
	mr	%r4,%r31

/*
 * Note that callframe linkages are setup in cpu_lwp_fork().
 */
	ldreg	%r31,CFRAME_R31(%r1)	/* restore saved registers */
	ldreg	%r30,CFRAME_R30(%r1)
#if 1
	addi	%r1,%r1,CALLFRAMELEN
#else
	ldreg	%r1,CFRAME_SP(%r1)	/* pop stack frmae */
#endif
	ldreg	%r0,CFRAME_LR(%r1)
	mtlr	%r0
ENTRY_NOPROFILE(emptyidlespin)
	blr				/* CPUINIT needs a raw blr */

/*
 * Child comes here at the end of a fork.
 * Return to userspace via the trap return path.
 */
	.globl	_C_LABEL(cpu_lwp_bootstrap)
_C_LABEL(cpu_lwp_bootstrap):
#if defined(MULTIPROCESSOR) && 0
	mr	%r28,%r3
	mr	%r29,%r4
	bl	_C_LABEL(proc_trampoline_mp)
	mr	%r4,%r29
	mr	%r3,%r28
#endif
	/*
	 * r3 (old lwp) and r4 (new lwp) are setup in cpu_switchto.
	 */
	bl	_C_LABEL(lwp_startup)

	mtlr	%r31
	mr	%r3,%r30
	blrl				/* jump indirect to r31 */
	b	trapexit

#if defined(MULTIPROCESSOR) && !defined(PPC_OEA64) && !defined (PPC_IBM4XX)
ENTRY(cpu_spinup_trampoline)
	li	0,0
	mtmsr	0
	isync

	lis	3,_C_LABEL(cpu_hatch_stack)@ha
	lwz	1,_C_LABEL(cpu_hatch_stack)@l(3)

	bl	_C_LABEL(cpu_hatch)
	b	_C_LABEL(idle_loop)
#endif /*MULTIPROCESSOR + OEA*/
