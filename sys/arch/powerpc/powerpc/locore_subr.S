/*	$NetBSD: locore_subr.S,v 1.55.12.1 2018/05/21 04:36:01 pgoyette Exp $	*/

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
#include "opt_modular.h"

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
     /* streg	%r13,(SFRAME_R13)(t0); */	/* volatile */	\
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
     /* ldreg	%r13,(SFRAME_R13)(t0); */	/* volatile */	\
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

#ifdef MODULAR
	.global	__USRSTACK
	.equ	__USRSTACK, USRSTACK
	.global	__CPU_MAXNUM
	.equ	__CPU_MAXNUM, CPU_MAXNUM
#endif

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

#ifdef PPC_BOOKE
	mfmsr	%r0
	andis.	%r0,%r0,PSL_CE@h
	tweqi	%r0,0	
	mfmsr	%r0
	andi.	%r0,%r0,PSL_DE@l
	tweqi	%r0,0	
#endif

	/* 
	 * If the oldlwp was null, don't bother saving the switch state.
	 */
	cmpwi	%r30,0
	beq	switchto_restore

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	mfsr	%r10,USER_SR		/* save USER_SR for copyin/copyout */
#else
	li	%r10,0			/* USER_SR not needed */
#endif
	mfcr	%r11			/* save cr */
	mr	%r12,%r2		/* save r2 */
	CFRAME_LRSAVE(%r1)
	stptru	%r1,-SFRAMELEN(%r1)	/* still running on old stack */
	SWITCHFRAME_SAVE(%r1)		/* save USER_SR, CR, R2, non-volatile */
	ldptr	%r4,L_PCB(%r30)		/* put PCB addr in r4 */
	streg	%r1,PCB_SP(%r4)		/* store old lwp's SP */
#if defined(PPC_BOOKE)
	mfspr	%r9,SPR_USPRG0
	streg	%r9,PCB_USPRG0(%r4)	/* save in PCB, not switchframe. */
#endif

switchto_restore:
/* Lock the scheduler. */
#if defined(PPC_IBM4XX) || defined(PPC_BOOKE)
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
	mr	%r13,%r31
#ifdef PPC_BOOKE
	mtsprg2	%r31			/* save curlwp in sprg2 */
#endif
#ifdef MULTIPROCESSOR
	stptr	%r7,L_CPU(%r31)		/* update cpu pointer */
#endif
	ldptr	%r4,L_PCB(%r31)		/* put PCB addr in r4 */
	stptr	%r4,CI_CURPCB(%r7)	/* using a new pcb */
	ldptr	%r3,PCB_PM(%r4)
	stptr	%r3,CI_CURPM(%r7)	/* and maybe a new pmap */

	/*
	 * Now restore the register state.
	 */
	ldreg	%r1,PCB_SP(%r4)		/* get new lwp's SP */
	SWITCHFRAME_RESTORE(%r1)	/* get non-volatile, CR, R2, USER_SR */
#if defined(PPC_BOOKE)
	ldreg	%r9,PCB_USPRG0(%r4)
	mtspr	SPR_USPRG0,%r9
#endif
	ldreg	%r1,0(%r1)		/* get saved SP */
	mr	%r2,%r12		/* get saved r2 */
	mtcr	%r11			/* get saved cr */
#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	mtsr	USER_SR,%r10		/* get saved USER_SR */
#endif
	isync

#if defined(PPC_IBM4XX) || defined(PPC_BOOKE)
	wrteei	1			/* interrupts are okay again */
#else /* PPC_OEA */
	mfmsr	%r4
	ori	%r4,%r4,PSL_EE@l	/* interrupts are okay again */
	mtmsr	%r4
#endif

#if defined(PPC_IBM4XX)
0:
	ldreg	%r3,CI_CURPM(%r7)	/* Do we need a context? */
	ldreg	%r4,PM_CTX(%r3)
	cmpwi	%r4,0
	bne	1f
	bl	_C_LABEL(ctx_alloc)
	GET_CPUINFO(%r7)
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
	stwcx.	%r1,0,%r1		/* clear reservation */
#if 1
	addi	%r1,%r1,CALLFRAMELEN
#else
	ldreg	%r1,CFRAME_SP(%r1)	/* pop stack frmae */
#endif
	ldreg	%r0,CFRAME_LR(%r1)
	mtlr	%r0
	blr				/* CPUINIT needs a raw blr */

ENTRY_NOPROFILE(emptyidlespin)
#ifdef DIAGNOSTIC
	GET_CPUINFO(%r3)
	lbz	%r4,CI_CPL(%r3)
	twnei	%r4,IPL_NONE
	mfmsr	%r5
	andi.	%r5,%r5,PSL_EE@l
	tweqi	%r5,0
#endif
	blr				/* CPUINIT needs a raw blr */

#ifdef __HAVE_FAST_SOFTINTS
	/*
	 * This gets executed if the softint thread blocks.
	 * cpu_switchto has restored r30/r31 for us.
	 */
_ENTRY(softint_cleanup)
	GET_CPUINFO(%r7)
	ldint	%r5, CI_MTX_COUNT(%r7)
	addi	%r5, %r5, 1
	stint	%r5, CI_MTX_COUNT(%r7)
	li	%r0, 0
	stptr	%r0, L_CTXSWTCH(%r3)	/* clear ctxswitch of old lwp */
	ldreg	%r0, CFRAME_R31(%r1)	/* get saved MSR */
#if defined(PPC_IBM4XX) || defined(PPC_BOOKE)
	wrtee	%r0			/* restore EE */
#endif
#if defined(PPC_OEA) || defined(PPC_OEA64_BRIDGE) || defined(PPC_OEA64)
	mtmsr	%r0
	isync
#endif
	stwcx.	%r1,0,%r1		/* clear reservation */
	addi	%r1, %r1, CALLFRAMELEN
	ldreg	%r0, CFRAME_LR(%r1)
	mtlr	%r0
#if IPL_SCHED != IPL_HIGH
	li	%r3, IPL_HIGH
	b	_C_LABEL(splraise)
#else
	blr
#endif /* IPL SCHED != IPL_HIGH */

_ENTRY(softint_fast_dispatch)
	/*
	 * Our call frame which softint will grab LR from.
	 */
	mflr	%r0
	streg	%r0, CFRAME_LR(%r1)
	stptru	%r1, -CALLFRAMELEN(%r1)
	mfmsr	%r0
	streg	%r0, CFRAME_R31(%r1)

	/*
	 * We need a 2nd callframe from which cpu_switchto will consume
	 * if the softint thread blocks.
	 */
	lis	%r8, _C_LABEL(softint_cleanup)@ha
	addi	%r8, %r8, _C_LABEL(softint_cleanup)@l
	streg	%r8, CFRAME_LR(%r1)
	stptru	%r1, -CALLFRAMELEN(%r1)
	streg	%r30, CFRAME_R30(%r1)
	streg	%r31, CFRAME_R31(%r1)

	GET_CPUINFO(%r7)
	mr	%r30, %r13		/* curlwp is now in r13 */

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	mfsr	%r10,USER_SR		/* save USER_SR for copyin/copyout */
#else
	li	%r10,0			/* USER_SR not needed */
#endif
	mfcr	%r11			/* save cr */
	mr	%r12,%r2		/* save r2 */
	CFRAME_LRSAVE(%r1)
	stptru	%r1, -SFRAMELEN(%r1)	/* still running on old stack */
	SWITCHFRAME_SAVE(%r1)		/* save USER_SR, CR, R2, non-volatile */
	mr	%r31, %r1
	ldptr	%r5, L_PCB(%r30)	/* put PCB addr in r5 */
	streg	%r1, PCB_SP(%r5)	/* store old lwp's SP */
#if defined(PPC_BOOKE)
	mfspr	%r9,SPR_USPRG0
	streg	%r9,PCB_USPRG0(%r5)	/* save in PCB, not switchframe. */
#endif

	mfmsr	%r29
#if defined(PPC_IBM4XX) || defined(PPC_BOOKE)
	wrteei	0			/* disable interrupts while
					   manipulating runque */
#else /* PPC_OEA */
	andi.	%r28,%r29,~PSL_EE@l	/* disable interrupts while
					   manipulating runque */
	mtmsr	%r28
	isync
#endif

	/*
	 * We don't need a ctx for ibm4xx since we are switching
	 * to a kernel thread
	 */

	stptr	%r3, CI_CURLWP(%r7)
	mr	%r13, %r3
#ifdef PPC_BOOKE
	mtsprg2	%r3
#endif
	ldptr	%r5, L_PCB(%r3)
	stptr	%r5, CI_CURPCB(%r7)
	ldptr	%r0, PCB_PM(%r5)
	stptr	%r0, CI_CURPM(%r7)	/* and maybe a new pmap */
	addi	%r1, %r5, USPACE - FRAMELEN - CALLFRAMELEN

#if defined(PPC_IBM4XX) || defined(PPC_BOOKE)
	wrteei	1			/* interrupts are okay again */
#else /* PPC_OEA */
	mtmsr	%r29
#endif

	/*
	 * %r3 = softint thread
	 * %r4 = ipl
	 */
	mr	%r3, %r30		/* pass former curlwp */
	bl	_C_LABEL(softint_dispatch)

#if defined(PPC_IBM4XX) || defined(PPC_BOOKE)
	wrteei	0			/* disable interrupts while
					   manipulating runque */
#else /* PPC_OEA */
	mtmsr	%r28			/* disable interrupts while
					   manipulating runque */
	isync
#endif

	GET_CPUINFO(%r7)
	stptr	%r30, CI_CURLWP(%r7)
	mr	%r13, %r30
#ifdef PPC_BOOKE
	mtsprg2	%r30
#endif
	ldptr	%r5, L_PCB(%r30)
	stptr	%r5, CI_CURPCB(%r7)
	ldptr	%r0, PCB_PM(%r5)
	stptr	%r0, CI_CURPM(%r7)	/* and maybe a new pmap */
	mr	%r1, %r31		/* get saved SP */

#if defined(PPC_IBM4XX) || defined(PPC_BOOKE)
	wrtee	%r29			/* interrupts are okay again */
#else /* PPC_OEA */
	mtmsr	%r29
#endif

	/*
	 * Since softint_dispatch returned to us, we know that the callee-saved
	 * registers are intact and thus don't have to restored (except for 
	 * r28/r29/r30/r31 which we used).
	 * Since softint's can't copyin/copyout USER_SR couldn't have been
	 * modified either and CR/R2 could not have been changed as well.
	 * We can just eat the switchframe and continue.
	 */
#if 0
#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	ldreg	%r10,SFRAME_USER_SR(%r1) /* USER_SR */
	mtsr	USER_SR,%r10		/* restore USER_SR for copyin/copyout */
#endif
	ldreg	%r28,SFRAME_R28(%r1)	/* R28 */
	ldreg	%r29,SFRAME_R29(%r1)	/* R29 */
	ldreg	%r11,SFRAME_CR(%r1)	/* CR */
	mtcr	%r11
	ldreg	%r2,SFRAME_R2(%r1)	/* R2 */
#endif
#if 0
	addi	%r1,%r1,SFRAMELEN	/* remove switch frame */

	ldreg	%r31,CFRAME_R31(%r1)	/* restore saved registers */
	ldreg	%r30,CFRAME_R30(%r1)	/* from switchto callframe */
	addi	%r1,%r1,CALLFRAMELEN	/* remove switchto call frame */
	addi	%r1,%r1,CALLFRAMELEN	/* remove our call frame */
#else
	ldreg	%r28,SFRAME_R28(%r1)	/* R28 */
	ldreg	%r29,SFRAME_R29(%r1)	/* R29 */
	ldreg	%r31,SFRAMELEN+CFRAME_R31(%r1)	/* restore saved registers */
	ldreg	%r30,SFRAMELEN+CFRAME_R30(%r1)
	stwcx.	%r1,0,%r1		/* clear reservation */
	addi	%r1,%r1,SFRAMELEN+2*CALLFRAMELEN /* remove switch & callframes */
#endif
	ldreg	%r0,CFRAME_LR(%r1)
	mtlr	%r0
	blr
#endif /* __HAVE_FAST_SOFTINTS */

/*
 * Child comes here at the end of a fork.
 * Return to userspace via the trap return path.
 */
	.globl	_C_LABEL(cpu_lwp_bootstrap)
_ENTRY(cpu_lwp_bootstrap)
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
	lwz	%r31, FRAME_SRR1(%r1)	/* trapexit wants srr1 in r31 */
#ifdef PPC_BOOKE
	lis	%r30, 0xbeeffeed@h
	ori	%r30, %r30, 0xbeeffeed@l
	andis.	%r0,%r31,PSL_CE@h
	tweqi	%r0,0
	andi.	%r0,%r31,PSL_DE@l
	tweqi	%r0,0
#endif
	li	%r4, 1			/* make sure userret gets called */
	stint	%r4, L_MD_ASTPENDING(%r13)
	b	trapexit

#if defined(MULTIPROCESSOR) && (defined(PPC_OEA) || defined (PPC_OEA64_BRIDGE))
ENTRY(cpu_spinup_trampoline)
	li	%r0,0
	mtmsr	%r0

	lis	%r5,oeacpufeat@ha
	lwz	%r5,oeacpufeat@l(%r5)
	andi.	%r5,%r5,OEACPU_64_BRIDGE
	beq	6f
	sync
	slbia
	sync
	isync
	clrldi	%r0,%r0,32
	mtmsrd	%r0
6:
	isync

	lis	%r4,_C_LABEL(cpu_hatch_stack)@ha
	lwz	%r1,_C_LABEL(cpu_hatch_stack)@l(%r4)

	bl	_C_LABEL(cpu_hatch)
	mr	%r1,%r3
	b	_C_LABEL(idle_loop)

ENTRY(cpu_spinstart)
	li	%r0,0
	mtmsr	%r0
	lis	%r5,oeacpufeat@ha
	lwz	%r5,oeacpufeat@l(%r5)
	andi.	%r5,%r5,OEACPU_64_BRIDGE
	beq	4f
	sync
	slbia
	sync
	isync
	clrldi	%r0,%r0,32
	mtmsrd	%r0
	mtspr	SPR_ASR,%r0
4:
	lis	%r5,_C_LABEL(cpu_spinstart_ack)@ha
	addi	%r5,%r5,_C_LABEL(cpu_spinstart_ack)@l
	stw	%r3,0(%r5)
	dcbf	0,%r5
	lis	%r6,_C_LABEL(cpu_spinstart_cpunum)@ha
5:
	lwz	%r4,_C_LABEL(cpu_spinstart_cpunum)@l(%r6)
	cmpw	%r4,%r3
	bne	5b
	lis	%r4,_C_LABEL(cpu_hatch_stack)@ha
	lwz	%r1,_C_LABEL(cpu_hatch_stack)@l(%r4)
	bla	_C_LABEL(cpu_hatch)
	mr	%r1,%r3	/* move the return from cpu_hatch to the stackpointer*/
	b	_C_LABEL(idle_loop)

#endif /*MULTIPROCESSOR + OEA*/

/*
 * int do_ucas_32(int32_t *uptr, int32_t old, int32_t new, int32_t *ret);
 */
ENTRY(do_ucas_32)
1:
	lwarx	%r10,0,%r3
	cmpw	%r10, %r4
	bne	2f
	stwcx.	%r5,0,%r3
	bne	1b
	mr	%r5,%r10
2:
	li	%r3,0
	stw	%r10,0(%r6)
	blr
