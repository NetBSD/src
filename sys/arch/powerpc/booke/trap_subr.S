/*	$NetBSD: trap_subr.S,v 1.14 2022/05/07 04:19:22 rin Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

RCSID("$NetBSD: trap_subr.S,v 1.14 2022/05/07 04:19:22 rin Exp $")

#ifdef _KERNEL_OPT
#include "opt_altivec.h"
#include "opt_ddb.h"
#include "opt_mpc85xx.h"
#include "opt_multiprocessor.h"
#endif

	.globl	_C_LABEL(sctrapexit), _C_LABEL(trapexit), _C_LABEL(intrcall)

	/*
	 * We have a problem with critical (MSR[CE] or machine check (MSR[ME])
	 * or debug (MSR[DE]) interrupts/exception in that they could happen
	 * inbetween the mtsprg1 %r2 and mfsprg1 %r2.  If that happens, %r2
	 * will be lost.  Even if we moved to a different sprg, subsequent
	 * expceptions would use SPRG1 and its value would be lost.  The only
	 * way to be safe for CE/ME/DE faults to save and restore SPRG1.
	 *
	 * Since CE/ME/DE faults may happen anytime, we need r1 to always
	 * contain a valid kernel stack pointer.  Therefore we use r2 as
	 * our temporary register.
	 *
	 * To prevent %r2 being overwritten, each "level" (normal, critical,
	 * mchk) uses a unique sprg to save %r2 (sprg1, sprg4, sprg5).
	 *
	 * Since we can't control how many nested exceptions we might get,
	 * we don't use a dedicated save area.  Instead we have a upwards
	 * growing "stack" of them; the pointer to which is kept in sprg3.
	 *
	 * To allocate from the stack, one fetches sprg3, adds the amount
	 * needed, saves sprg3, and then refers to the save using a
	 * displacement of -amount.
	 */
	/*
	 * XXXclang
	 * At the moment, clang cannot correctly assemble m[ft]sprgN
	 * (N >= 4); use m[ft]spr for SPR_SPRGN instead.
	 */
#define	FRAME_EXC_PROLOGUE(start, sprg, srr)				\
	mtspr	sprg,%r2;		/* save r2 */			\
	mfsprg3	%r2;			/* get save_area pointer */	\
	addi	%r2,%r2,4*(32-start);					\
					/* allocate save area */	\
	mtsprg3	%r2;			/* save updated pointer */	\
	stmw	%r##start,-4*(32-start)(%r2);				\
					/* free r24-r31 for use */	\
	mfspr	%r26,sprg;		/* get saved r2 */		\
	mfcr	%r27;			/* get Condition Register */	\
	mfxer	%r28;			/* get XER */			\
	mfspr	%r30, SPR_##srr##0;	/* get SRR0 */			\
	mfspr	%r31, SPR_##srr##1	/* get SRR1 */

#define	PROLOGUE_GET_DEAR	mfspr	%r24, SPR_DEAR
#define	PROLOGUE_GET_ESR	mfspr	%r25, SPR_ESR
#define	PROLOGUE_GET_SRRS	mfsrr0	%r24; \
				mfsrr1	%r25
#define	PROLOGUE_GET_SPRG1	mfsprg1	%r29
#define	PROLOGUE_GET_DBSR	mfspr	%r25, SPR_DBSR
#define	SAVE_ESR		stw	%r25, FRAME_ESR(%r1)
#define	SAVE_DEAR		stw	%r24, FRAME_DEAR(%r1)
#define	SAVE_DEAR_ESR		SAVE_ESR; SAVE_DEAR
#define	SAVE_SRRS		SAVE_DEAR_ESR
#define	SAVE_SPRG1		stw	%r29, FRAME_SPRG1(%r1)
#define	SAVE_DBSR		stw	%r25, FRAME_DBSR(%r1)
#define	SAVE_NOTHING		/* nothing */
#define	RESTORE_SPRG1(r)	lwz	r, FRAME_SPRG1(%r1); \
				mtsprg1 r
#define	RESTORE_SRR0(r)		lwz	r, FRAME_DEAR(%r1); \
				mtsrr0	r
#define	RESTORE_SRR1(r)		lwz	r, FRAME_ESR(%r1); \
				mtsrr1	r

#define	FRAME_PROLOGUE		\
	FRAME_EXC_PROLOGUE(26, SPR_SPRG1, SRR)

#define	FRAME_PROLOGUE_DEAR_ESR	\
	FRAME_EXC_PROLOGUE(24, SPR_SPRG1, SRR); \
	PROLOGUE_GET_ESR; \
	PROLOGUE_GET_DEAR

#define	FRAME_PROLOGUE_ESR	\
	FRAME_EXC_PROLOGUE(25, SPR_SPRG1, SRR); \
	PROLOGUE_GET_ESR

#define	FRAME_TLBPROLOGUE	\
	FRAME_EXC_PROLOGUE(20, SPR_SPRG1, SRR); \
	PROLOGUE_GET_ESR; \
	PROLOGUE_GET_DEAR

#define	FRAME_INTR_PROLOGUE	\
	FRAME_EXC_PROLOGUE(26, SPR_SPRG1, SRR)

/*
 * These need to save SRR0/SRR1 as well their SRR0/SRR1 in case normal
 * exceptions happened during their execution.
 */
#define	FRAME_CRIT_PROLOGUE	\
	FRAME_EXC_PROLOGUE(24, SPR_SPRG4, CSRR); \
	PROLOGUE_GET_SPRG1; \
	PROLOGUE_GET_SRRS

#define	FRAME_MCHK_PROLOGUE	\
	FRAME_EXC_PROLOGUE(24, SPR_SPRG5, MCSRR); \
	PROLOGUE_GET_SPRG1; \
	PROLOGUE_GET_SRRS

#define	FRAME_DEBUG_PROLOGUE	\
	FRAME_EXC_PROLOGUE(24, SPR_SPRG4, CSRR); \
	PROLOGUE_GET_SPRG1; \
	PROLOGUE_GET_SRRS

/*
 * DDB expects to fetch the LR from the previous frame.  But it also
 * expects to be pointing at the instruction after the branch link.  Since
 * we didn't branch, we need to advance it by to fake out DDB.  But there's
 * problem.  If the routine is in either its first or last two instructions
 * (before or after its adjusted its stack pointer), we could possibly
 * overwrite stored return address.  So that stored return address needs to
 * saved and restored.
 */
#if defined(DDB)
#define FRAME_SAVE_SRR0_FOR_DDB						\
	lwz	%r29, FRAMELEN+CFRAME_LR(%r1);	/* fetch old return address */\
	stw	%r29, FRAME_CFRAME_LR(%r1);	/* save it */		 \
	addi	%r30, %r30, 4;			/* point to s the next insn */ \
	stw	%r30, FRAMELEN+CFRAME_LR(%r1)	/* appease ddb stacktrace */
#define FRAME_RESTORE_RETURN_ADDRESS					\
	lwz	%r3, FRAME_CFRAME_LR(%r1);	/* fetch old return address */ \
	stw	%r3, FRAMELEN+CFRAME_LR(%r1)	/* restore it */
#else
#define FRAME_SAVE_SRR0_FOR_DDB
#define FRAME_RESTORE_RETURN_ADDRESS
#endif

#ifdef PPC_HAVE_SPE
#define	FRAME_SAVE_SPEFSCR						\
	mfspefscr %r0;			/* get spefscr */		\
	stw	%r0, FRAME_SPEFSCR(%r1) /* save into trapframe */
#define	FRAME_RESTORE_SPEFSCR						\
	lwz	%r0, FRAME_SPEFSCR(%r1); /* fetch from trapframe */	\
	mtspefscr %r0			/* save spefscr */
#else
#define	FRAME_SAVE_SPEFSCR
#define	FRAME_RESTORE_SPEFSCR
#endif
/*
 * Before the first memory refernence, we must have our state inside registers
 * since the first memory access might cause an exception which would cause
 * SRR0/SRR1 and DEAR/ESR to become unrecoverable.  CR and XER also need to be
 * saved early since they will modified by instrction flow.  The saved stack
 * pointer is also critical but LR and CTR can be deferred being saved until
 * we are actually filling a trapframe.
 */
#define	FRAME_EXC_ENTER(exc, tf, start, save_prologue)			\
	mtcr	%r31;			/* user mode exception? */	\
	mr	%r31, %r1;		/* save SP (SRR1 is safe in CR) */ \
	bf	MSR_PR, 1f;		/*   nope, sp is good */	\
	mfsprg2	%r2;			/* get curlwp */		\
	lwz	%r2, L_PCB(%r2);	/* get uarea of curlwp */	\
	addi	%r1, %r2, USPACE-CALLFRAMELEN;				\
					/* start stack at top of it */	\
1:									\
	stwu	%r31, -FRAMELEN(%r1);	/* get space for trapframe */	\
	stw	%r0, FRAME_R0(%r1);	/* save r0 */			\
	stw	%r31, FRAME_R1(%r1);	/* save (saved) r1 */		\
	stw	%r26, FRAME_R2(%r1);	/* save (saved) r2 */		\
	save_prologue;			/* save SPRG1/ESR/DEAR */	\
	/* At this point, r26, r29, and r31 have been saved so we */	\
	/* can use them for LR, CTR, and SRR1.			  */	\
	mflr	%r26;			/* get Link Register */		\
	mfctr	%r29;			/* get CTR */			\
	mfcr	%r31;			/* get SRR1 */			\
	stmw	%r26, FRAME_LR(%r1);	/* save LR CR XER CTR SRR0/1 */	\
	FRAME_SAVE_SRR0_FOR_DDB;					\
	mr	%r0, %r31;		/* save SRR1 for a bit */	\
	mfsprg3	%r2;			/* get save_area pointer */	\
	addi	%r2,%r2,-4*(32-start);	/* find our save area */	\
	lmw	%r##start,0(%r2);	/* get start-r31 */		\
	mtsprg3	%r2;			/* save updated pointer */	\
	stmw	%r3, FRAME_R3(%r1);	/* save r2-r31 */		\
	/* Now everything has been saved */				\
	mr	%r31, %r0;		/* move SRR1 back to r31 */	\
	mfsprg2	%r13;			/* put curlwp in r13 */		\
	FRAME_SAVE_SPEFSCR;						\
	li	%r7, exc;		/* load EXC_* */		\
	stw	%r7, FRAME_EXC(%r1);	/* save into trapframe */	\
	addi	tf, %r1, FRAME_TF	/* get address of trap frame */

#define FRAME_EXC_EXIT(rfi, srr)					\
	FRAME_RESTORE_RETURN_ADDRESS;	/* restore return address */	\
	lmw	%r26, FRAME_LR(%r1);	/* get LR CR XER CTR SRR0/1 */	\
	oris	%r31,%r31,PSL_CE@h;					\
	mtspr	SPR_##srr##1, %r31;	/* restore SRR1 */		\
	mtspr	SPR_##srr##0, %r30;	/* restore SRR0 */		\
	FRAME_RESTORE_SPEFSCR;						\
	mtctr	%r29;			/* restore CTR */		\
	mtxer	%r28;			/* restore XER */		\
	mtcr	%r27;			/* restore CR */		\
	mtlr	%r26;			/* restore LR */		\
	lmw	%r2, FRAME_R2(%r1);	/* restore r2-r31 */		\
	lwz	%r0, FRAME_R0(%r1);	/* restore r0 */		\
	lwz	%r1, FRAME_R1(%r1);	/* restore r1 */		\
	rfi				/* return from interrupt */


#define	FRAME_ENTER(exc, tf)		\
	FRAME_EXC_ENTER(exc, tf, 26, SAVE_NOTHING)

#define	FRAME_ENTER_ESR(exc, tf)	\
	FRAME_EXC_ENTER(exc, tf, 25, SAVE_ESR)

#define	FRAME_ENTER_DEAR_ESR(exc, tf)	\
	FRAME_EXC_ENTER(exc, tf, 24, SAVE_DEAR_ESR)

#define	FRAME_EXIT		FRAME_EXC_EXIT(rfi, SRR)

#define	FRAME_TLBENTER(exc)	\
	FRAME_EXC_ENTER(exc, %r4, 20, SAVE_DEAR_ESR)
#define	FRAME_TLBEXIT		FRAME_EXC_EXIT(rfi, SRR)

#define	FRAME_MCHK_ENTER(exc)	\
	FRAME_EXC_ENTER(exc, %r3, 26, SAVE_SPRG1; SAVE_SRRS)
#define	FRAME_MCHK_EXIT		\
	RESTORE_SRR0(%r28);	\
	RESTORE_SRR1(%r27);	\
	RESTORE_SPRG1(%r26);	\
	FRAME_EXC_EXIT(rfmci, MCSRR)

#define	FRAME_DEBUG_ENTER(exc)	\
	FRAME_EXC_ENTER(exc, %r4, 26, SAVE_SPRG1; SAVE_SRRS)
#define	FRAME_DEBUG_EXIT	\
	RESTORE_SPRG1(%r26); FRAME_EXC_EXIT(rfci, CSRR)

#define	FRAME_INTR_SP							\
	bf	MSR_PR, 1f;		/*    nope, sp is good */	\
	mfsprg2	%r2;			/* get curlwp */		\
	lwz	%r2, L_PCB(%r2);	/* get uarea of curlwp */	\
	addi	%r1, %r2, USPACE-CALLFRAMELEN;				\
					/* start stack at top of it */	\
1:

#define	FRAME_INTR_SP_NEW(sym)						\
	lis	%r2,(sym)@ha;						\
	addi	%r1,%r2,(sym)@l

#define	FRAME_INTR_XENTER(exc, start, get_intr_sp, save_prologue)	\
	mtcr	%r31;			/* user mode exception? */	\
	mr	%r31, %r1;		/* save SP (SRR1 is safe in CR) */ \
	get_intr_sp;			/* get kernel stack pointer */	\
	stwu	%r31, -FRAMELEN(%r1);	/* get space for trapframe */	\
	stw	%r0, FRAME_R0(%r1);	/* save r0 */			\
	stw	%r31, FRAME_R1(%r1);	/* save (saved) r1 */		\
	stw	%r26, FRAME_R2(%r1);	/* save (saved) r2 */		\
	save_prologue;			/* save SPRG1 (maybe) */	\
	mflr	%r26;			/* get LR */			\
	mfctr	%r29;			/* get CTR */			\
	mfcr	%r31;			/* get SRR1 */			\
	stmw	%r26, FRAME_LR(%r1);	/* save LR CR XER CTR SRR0/1 */	\
	FRAME_SAVE_SRR0_FOR_DDB;					\
	stw	%r3, FRAME_R3(%r1);	/* save r3 */			\
	stw	%r4, FRAME_R4(%r1);	/* save r4 */			\
	stw	%r5, FRAME_R5(%r1);	/* save r5 */			\
	stw	%r6, FRAME_R6(%r1);	/* save r6 */			\
	stw	%r7, FRAME_R7(%r1);	/* save r7 */			\
	stw	%r8, FRAME_R8(%r1);	/* save r8 */			\
	stw	%r9, FRAME_R9(%r1);	/* save r9 */			\
	stw	%r10, FRAME_R10(%r1);	/* save r10 */			\
	stw	%r11, FRAME_R11(%r1);	/* save r11 */			\
	stw	%r12, FRAME_R12(%r1);	/* save r12 */			\
	stw	%r13, FRAME_R13(%r1);	/* save r13 */			\
	mfsprg3	%r2;			/* get save_area pointer */	\
	addi	%r2,%r2,-4*(32-start);	/* find our save area */	\
	lmw	%r##start,0(%r2);	/* get start-r31 */		\
	mtsprg3	%r2;			/* save updated pointer */	\
	mfsprg2	%r13;			/* put curlwp into r13 */	\
	li	%r7, exc;		/* load EXC_* */		\
	stw	%r7, FRAME_EXC(%r1);	/* save into trapframe */	\
	addi	%r3, %r1, FRAME_TF	/* only argument is trapframe */

#define FRAME_INTR_XEXIT(rfi, srr)					\
	FRAME_RESTORE_RETURN_ADDRESS;	/* restore return address */	\
	lwz	%r8, FRAME_LR(%r1);	/* get LR */			\
	lwz	%r9, FRAME_CR(%r1);	/* get CR */			\
	lwz	%r10, FRAME_XER(%r1);	/* get XER */			\
	lwz	%r11, FRAME_CTR(%r1);	/* get CTR */			\
	lwz	%r12, FRAME_SRR0(%r1);	/* get SRR0 */			\
	lwz	%r13, FRAME_SRR1(%r1);	/* get SRR1 */			\
	mtspr	SPR_##srr##1, %r13;	/* restore SRR1 */		\
	mtspr	SPR_##srr##0, %r12;	/* restore SRR0 */		\
	mtctr	%r11;			/* restore CTR */		\
	mtxer	%r10;			/* restore XER */		\
	mtcr	%r9;			/* restore CR */		\
	mtlr	%r8;			/* restore LR */		\
	lwz	%r13, FRAME_R13(%r1);	/* restore r13 */		\
	lwz	%r12, FRAME_R12(%r1);	/* restore r12 */		\
	lwz	%r11, FRAME_R11(%r1);	/* restore r11 */		\
	lwz	%r10, FRAME_R10(%r1);	/* restore r10 */		\
	lwz	%r9, FRAME_R9(%r1);	/* restore r9 */		\
	lwz	%r8, FRAME_R8(%r1);	/* restore r8 */		\
	lwz	%r7, FRAME_R7(%r1);	/* restore r7 */		\
	lwz	%r6, FRAME_R6(%r1);	/* restore r6 */		\
	lwz	%r5, FRAME_R5(%r1);	/* restore r5 */		\
	lwz	%r4, FRAME_R4(%r1);	/* restore r4 */		\
	lwz	%r3, FRAME_R3(%r1);	/* restore r3 */		\
	lwz	%r2, FRAME_R2(%r1);	/* restore r2 */		\
	lwz	%r0, FRAME_R0(%r1);	/* restore r0 */		\
	lwz	%r1, FRAME_R1(%r1);	/* restore r1 */		\
	rfi				/* return from interrupt */

#define	FRAME_INTR_ENTER(exc)	\
	FRAME_INTR_XENTER(exc, 26, FRAME_INTR_SP, SAVE_NOTHING)
#define	FRAME_INTR_EXIT		\
	FRAME_INTR_XEXIT(rfi, SRR)
#define	FRAME_CRIT_ENTER(exc)	\
	FRAME_INTR_XENTER(exc, 24, FRAME_INTR_SP, SAVE_SPRG1)
#define	FRAME_WDOG_ENTER(exc, sym)	\
	FRAME_INTR_XENTER(exc, 24, FRAME_INTR_SP_NEW(sym), SAVE_SPRG1)
#define	FRAME_CRIT_EXIT		\
	RESTORE_SRR0(%r4);	\
	RESTORE_SRR1(%r5);	\
	RESTORE_SPRG1(%r6);	\
	FRAME_INTR_XEXIT(rfci, CSRR)

#if defined(MULTIPROCESSOR)
#define	FRAME_TLBMISSLOCK						\
	GET_CPUINFO(%r23);						\
	ldint	%r22, CI_MTX_COUNT(%r23);				\
	subi	%r22, %r22, 1;						\
	stint	%r22, CI_MTX_COUNT(%r23);				\
	isync;								\
	cmpwi	%r22, 0;						\
	bne	1f;							\
	ldint	%r22, CI_CPL(%r23);					\
	stint	%r22, CI_MTX_OLDSPL(%r23);				\
1:	lis	%r23, _C_LABEL(pmap_tlb_miss_lock)@h;			\
	ori	%r23, %r23, _C_LABEL(pmap_tlb_miss_lock)@l;		\
	li	%r20, MTX_LOCK;						\
2:	lwarx	%r22, %r20, %r23;					\
	cmpwi	%r22, __SIMPLELOCK_UNLOCKED;				\
	beq+	4f;							\
3:	lwzx	%r22, %r20, %r23;					\
	cmpwi	%r22, __SIMPLELOCK_UNLOCKED;				\
	beq+	2b;							\
	b	3b;							\
4:	li	%r21, __SIMPLELOCK_LOCKED;				\
	stwcx.	%r21, %r20, %r23;					\
	bne-	2b;							\
	isync;								\
	msync;
#define	FRAME_TLBMISSUNLOCK						\
	sync;								\
	lis	%r23, _C_LABEL(pmap_tlb_miss_lock)@h;			\
	ori	%r23, %r23, _C_LABEL(pmap_tlb_miss_lock)@l;		\
	li	%r22, __SIMPLELOCK_UNLOCKED;				\
	stw	%r22, MTX_LOCK(%r23);					\
	isync;								\
	msync;								\
	GET_CPUINFO(%r23);						\
	ldint	%r22, CI_MTX_COUNT(%r23);				\
	addi	%r22, %r22, 1;						\
	stint	%r22, CI_MTX_COUNT(%r23);				\
	isync;
#else	/* !MULTIPROCESSOR */
#define	FRAME_TLBMISSLOCK
#define	FRAME_TLBMISSUNLOCK
#endif	/* MULTIPROCESSOR */

	.text
	.p2align 4
_C_LABEL(critical_input_vector):
	/* MSR[ME] is unchanged, all others cleared */
	FRAME_CRIT_PROLOGUE			/* save SP r26-31 CR LR XER */
	FRAME_CRIT_ENTER(EXC_CII)
	bl	_C_LABEL(intr_critintr)		/* critintr(tf) */
	FRAME_CRIT_EXIT

	.p2align 4
_C_LABEL(machine_check_vector):
	/* all MSR bits are cleared */
	FRAME_MCHK_PROLOGUE			/* save SP r25-31 CR LR XER */
	FRAME_MCHK_ENTER(EXC_MCHK)
	/*
	 * MCAR/MCSR don't need to be saved early since MSR[ME] is cleared
	 * on entry.
	 */
	mfspr	%r7, SPR_MCAR
	mfspr	%r6, SPR_MCSR
	stw	%r6, FRAME_MCSR(%r1)
	stw	%r7, FRAME_MCAR(%r1)
	li	%r3, T_MACHINE_CHECK
	bl	_C_LABEL(trap)			/* trap(T_MACHINE_CHECK, tf) */
	FRAME_MCHK_EXIT

	.p2align 4
_C_LABEL(data_storage_vector):
	/* MSR[CE], MSR[ME], MSR[DE] are unchanged, all others cleared */
	FRAME_PROLOGUE_DEAR_ESR		/* save r2 DEAR ESR r24-31 CR XER SRR */
	FRAME_ENTER_DEAR_ESR(EXC_DSI, %r4)
	li	%r3, T_DSI
	/* FRAME_ENTER leaves SRR1 in %r31 */
trapenter:
trapagain:
	wrtee	%r31				/* restore MSR[EE] */

	bl	_C_LABEL(trap)			/* trap(trapcode, tf) */
_C_LABEL(trapexit):
	wrteei	0				/* disable interrupts */
#	andis.	%r0, %r31, PSL_CE@h
#	tweqi	%r0, 0
	andi.	%r4, %r31, PSL_PR		/* lets look at PSL_PR */
	beq	trapdone			/* if clear, skip to exit  */
	lwz	%r4, L_MD_ASTPENDING(%r13)	/* get ast pending */
	cmplwi	%r4, 0				/* is there an ast pending */
	beq+	trapdone			/*    nope, proceed to exit */
	li	%r6, EXC_AST			/* yes. */
	stw	%r6, FRAME_EXC(%r1)		/* pretend this is an AST */
	addi	%r4, %r1, FRAME_TF		/* get address of trap frame */
	li	%r3, T_AST
	b	trapagain			/* and deal with it */
trapdone:
	FRAME_EXIT

	.p2align 4
_C_LABEL(instruction_storage_vector):
	/* MSR[CE], MSR[ME], MSR[DE] are unchanged, all others cleared */
	FRAME_PROLOGUE_ESR		/* save ESR r2 r25-31 CR XER SRR0/1 */
	FRAME_ENTER_ESR(EXC_ISI, %r4)
	li	%r3, T_ISI
	b	trapenter

	.p2align 4
_ENTRY(external_input_vector)
	/* MSR[CE], MSR[ME], MSR[DE] are unchanged, all others cleared */
	FRAME_INTR_PROLOGUE			/* save SP r25-31 CR LR XER */
	FRAME_INTR_ENTER(EXC_EXI)

	bl	_C_LABEL(intr_extintr)
_C_LABEL(intrcall):
	GET_CPUINFO(%r6)			/* get curcpu() */
	lwz	%r5, FRAME_SRR1(%r1)		/* get saved SRR1 */
#	andis.	%r0, %r5, PSL_CE@h
#	tweqi	%r0, 0
	andi.	%r4, %r5, PSL_PR		/* lets look at PSL_PR */
	beq	intrexit			/* if clear, skip to exit  */
	lwz	%r4, L_MD_ASTPENDING(%r13)	/* get ast pending */
	cmplwi	%r4, 0				/* is there an ast pending */
	beq+	intrexit			/*    nope, proceed to exit */
	stmw	%r14, FRAME_R14(%r1)		/* save rest of registers */
	FRAME_SAVE_SPEFSCR
	mr	%r31, %r5			/* needed for trapagain */
	li	%r4, EXC_AST			/* */
	stw	%r4, FRAME_EXC(%r1)		/* pretend this is an AST */
	addi	%r4, %r1, FRAME_TF		/* get address of trap frame */
	li	%r3, T_AST
	b	trapagain			/* and deal with it */
intrexit:
	FRAME_INTR_EXIT

	.p2align 4
_C_LABEL(alignment_vector):
	/* MSR[CE], MSR[ME], MSR[DE] are unchanged, all others cleared */
	FRAME_PROLOGUE_DEAR_ESR			/* save SP r25-31 CR LR XER */
	FRAME_ENTER_DEAR_ESR(EXC_ALI, %r4)
	li	%r3, T_ALIGNMENT
	b	trapenter

	.p2align 4
_C_LABEL(program_vector):
	/* MSR[CE], MSR[ME], MSR[DE] are unchanged, all others cleared */
	FRAME_PROLOGUE_ESR			/* save SP r25-31 CR LR XER */
	FRAME_ENTER_ESR(EXC_PGM, %r4)
	li	%r3, T_PROGRAM
	b	trapenter

#ifdef SPR_IVOR7
	.p2align 4
_C_LABEL(fp_unavailable_vector):
	/* MSR[CE], MSR[ME], MSR[DE] are unchanged, all others cleared */
	FRAME_PROLOGUE_ESR			/* save SP r25-31 CR LR XER */
	FRAME_ENTER_ESR(EXC_FPU, %r4)
	li	%r3, T_FP_UNAVAILABLE
	b	trapenter
#endif

	.p2align 4
_C_LABEL(system_call_vector):
	/* MSR[CE], MSR[ME], MSR[DE] are unchanged, all others cleared */
	FRAME_PROLOGUE				/* save SP r26-31 CR LR XER */
	FRAME_ENTER(EXC_SC, %r3)

	wrteei	1				/* enable interrupts */
	lwz	%r7, L_PROC(%r13)		/* get proc for lwp */
	lwz	%r8, P_MD_SYSCALL(%r7)		/* get syscall */
	mtlr	%r8				/* need to call indirect */
	blrl					/* syscall(tf) */
_C_LABEL(sctrapexit):
	wrteei	0				/* disable interrupts */
	lwz	%r4, L_MD_ASTPENDING(%r13)	/* get ast pending */
	cmplwi	%r4, 0				/* is there an ast pending */
	beq+	trapdone			/*    nope, proceed to exit */
	li	%r0, EXC_AST			/* yes. */
	stw	%r0, FRAME_EXC(%r1)		/* pretend this is an AST */
	addi	%r4, %r1, FRAME_TF		/* get address of trap frame */
	li	%r3, T_AST
	b	trapenter			/* and deal with it */

#ifdef SPR_IVOR9
	.p2align 4
_C_LABEL(ap_unavailable_vector):
	/* MSR[CE], MSR[ME], MSR[DE] are unchanged, all others cleared */
	FRAME_PROLOGUE				/* save SP r25-31 CR LR XER */
	FRAME_ENTER(EXC_PGM, %r4)
	li	%r3, T_AP_UNAVAILABLE
	b	trapenter
#endif

	.p2align 4
_C_LABEL(decrementer_vector):
	/* MSR[CE], MSR[ME], MSR[DE] are unchanged, all others cleared */
	FRAME_INTR_PROLOGUE			/* save SP r25-31 CR LR XER */
	FRAME_INTR_ENTER(EXC_DECR)

	bl	_C_LABEL(intr_decrintr)
	b	intrexit

	.p2align 4
_C_LABEL(fixed_interval_timer_vector):
	/* MSR[CE], MSR[ME], MSR[DE] are unchanged, all others cleared */
	FRAME_PROLOGUE				/* save SP r25-31 CR LR XER */
	FRAME_INTR_ENTER(EXC_FIT)

	bl	_C_LABEL(intr_fitintr)
	b	intrexit

#ifdef E500_WDOG_STACK
	.data
	.lcomm wdogstk,4096
#endif
	.text
	.p2align 4
_C_LABEL(watchdog_timer_vector):
	/* MSR[CE], MSR[ME], MSR[DE] are unchanged, all others cleared */
	FRAME_CRIT_PROLOGUE			/* save SP r25-31 CR LR XER */
#ifdef E500_WDOG_STACK
	FRAME_WDOG_ENTER(EXC_WDOG, wdogstk+4096-CALLFRAMELEN)
#else
	FRAME_CRIT_ENTER(EXC_WDOG);
#endif

	bl	_C_LABEL(intr_wdogintr)
	FRAME_CRIT_EXIT

	.p2align 4
_C_LABEL(data_tlb_error_vector):
	/* MSR[CE], MSR[ME], MSR[DE] are unchanged, all others cleared */
	FRAME_TLBPROLOGUE
	FRAME_TLBMISSLOCK
	/*
	 * Registers as this point:
	 *
	 * r2  = cpu_info
	 * r20 = scratch
	 * r21 = scratch
	 * r22 = scratch
	 * r23 = scratch
	 * r24 = DEAR
	 * r25 = ESR
	 * r26 = saved r2
	 * r27 = CR
	 * r28 = XER
	 * r29 = scratch
	 * r30 = SRR0
	 * r31 = SRR1
	 *
	 * Except for r29, these values must be retained.  However we must
	 * be cognizant of nesting.  There are two cases here, both related.
	 *
	 * We get a critical input or machine check exception and the kernel
	 * stack doesn't have a TLB entry so we take an exception.  The other
	 * nesting path is some page used by the exception handler will cause
	 * a TLB data error.
	 *
	 * The second case (more probable) is that the PTE loading will fail
	 * so we will have to do a hard trap to resolve it.  But in doing so
	 * we need to save a trapframe which could result in another DTLB
	 * fault.
	 *
	 * In all cases, the save area stack shall protect us.
	 */
	/*
	 * Attempt to update the TLB from the page table.
	 */
	mflr	%r29				/* save LR */
	mr	%r23, %r24			/* address of exception */
	rlwinm	%r22, %r31,			/* index into ci_pmap_segtab */\
		MSR_DS+PTR_SCALESHIFT+1, \
		31-PTR_SCALESHIFT, \
		31-PTR_SCALESHIFT		/* move PSL_DS[27] to bit 29 */
	bl	pte_load
	FRAME_TLBMISSUNLOCK
	mtlr	%r29				/* restore LR */
	/*
	 * If we returned, pte load failed so let trap deal with it but
	 * has kept the contents of r24-r31 (expect r29) intact.
	 */
	FRAME_TLBENTER(EXC_DSI)
	li	%r3, T_DATA_TLB_ERROR
	b	trapenter

	.p2align 4
_C_LABEL(instruction_tlb_error_vector):
	/* MSR[CE], MSR[ME], MSR[DE] are unchanged, all others cleared */
	FRAME_TLBPROLOGUE
	FRAME_TLBMISSLOCK
	/*
	 * Attempt to update the TLB from the page table.
	 */
	mflr	%r29				/* save LR */
	mr	%r23, %r30			/* PC of exception */
	rlwinm	%r22, %r31,			/* index into ci_pmap_segtab */\
		MSR_IS+PTR_SCALESHIFT+1, \
		31-PTR_SCALESHIFT, \
		31-PTR_SCALESHIFT		/* move PSL_IS[26] to bit 29 */
	bl	pte_load
	FRAME_TLBMISSUNLOCK
	mtlr	%r29				/* restore LR */
	/*
	 * If we returned, pte load failed so let trap deal with it but
	 * has kept the contents of r24-r31 (expect r29) intact.
	 */
	FRAME_TLBENTER(EXC_ISI)
	li	%r3, T_INSTRUCTION_TLB_ERROR
	b	trapenter

	.p2align 4
_C_LABEL(debug_vector):
	FRAME_CRIT_PROLOGUE			/* save SP r25-31 CR LR XER */
	FRAME_CRIT_ENTER(EXC_DEBUG)
	mfspr	%r6, SPR_DBSR
	stw	%r6, FRAME_ESR(%r1)
	li	%r3, T_DEBUG
	bl	_C_LABEL(trap)
	FRAME_CRIT_EXIT

	.p2align 4
_C_LABEL(spv_unavailable_vector):
	FRAME_PROLOGUE_ESR			/* save SP r25-31 CR LR XER */
	FRAME_ENTER_ESR(EXC_VEC, %r4)
	li	%r3, T_SPE_UNAVAILABLE
	b	trapenter

	.p2align 4
_C_LABEL(fpdata_vector):
	FRAME_PROLOGUE_ESR			/* save SP r25-31 CR LR XER */
	FRAME_ENTER_ESR(EXC_FPA, %r4)
	li	%r3, T_EMBEDDED_FP_DATA
	b	trapenter

	.p2align 4
_C_LABEL(fpround_vector):
	FRAME_PROLOGUE_ESR			/* save SP r25-31 CR LR XER */
	FRAME_ENTER_ESR(EXC_FPA, %r4)
	li	%r3, T_EMBEDDED_FP_ROUND
	b	trapenter

	.p2align 4
_C_LABEL(perfmon_vector):
	FRAME_PROLOGUE_ESR			/* save SP r25-31 CR LR XER */
	FRAME_ENTER_ESR(EXC_PERF, %r4)
	li	%r3, T_EMBEDDED_PERF_MONITOR
	b	trapenter

	.p2align 4
pte_load:
	/*
	 * r2 = scratch
	 * r20 = scratch
	 * r21 = scratch
	 * r22 = index into ci_pmap_{kern,user}_segtab
	 * r23 = faulting address
	 * The rest are for reference and aren't modifiable.  If the load
	 * fails, they will be used by FRAME_TLBENTER to create the trapframe.
	 * r24 = DEAR
	 * r25 = ESR
	 * r26 = saved r2
	 * r27 = CR
	 * r28 = XER
	 * r29 = LR
	 * r30 = SRR0
	 * r31 = SRR1
	 */
	cmplwi	%cr2, %r22, 0		/* remember address space */
	GET_CPUINFO(%r2)
	addi	%r22, %r22, CI_PMAP_SEGTAB /* index into segtab(s) */
	lwzx	%r20, %r22, %r2		/* load kern/user L1 PT addr */
	cmplwi	%r20, 0			/* is segtab null? */
	beqlr	%cr0			/* yes, return to fallback to trap */

	rlwinm	%r22, %r23, NSEGPG_SCALESHIFT + PTR_SCALESHIFT, \
		31-(NSEGPG_SCALESHIFT + PTR_SCALESHIFT - 1), \
		31-PTR_SCALESHIFT	/* extract addr bits [0:9] to [20:29] */
	lwzx	%r20, %r22, %r20	/* load address of page table page */
	cmplwi	%r20, 0			/* is page null? */
	beqlr	%cr0			/* yes, return to fallback to trap */

	rlwinm	%r22, %r23, \
		NSEGPG_SCALESHIFT + NPTEPG_SCALESHIFT + PTE_SCALESHIFT, \
		31-(NPTEPG_SCALESHIFT + PTE_SCALESHIFT - 1), \
		31-PTE_SCALESHIFT	/* extract addr bits [10:19] to [20:29] */
	lwzx	%r20, %r22, %r20	/* load PTE from page table page */
	cmplwi	%r20, 0			/* is there a valid PTE? */
	beqlr	%cr0			/* no, return to fallback to trap */

#if (PTE_UNSYNCED << 1) != PTE_xX
#error PTE_UNSYNCED definition error
#endif
#if (PTE_UNMODIFIED << 1) != PTE_xW
#error PTE_UNMODIFIED definition error
#endif
	andi.	%r22, %r20, (PTE_UNSYNCED|PTE_UNMODIFIED)
					/* Does the PTE need to be changed? */
	rotlwi	%r22, %r22, 1		/* if so, clear the right PTE bits */
	andc	%r20, %r20, %r22	/* pte &= ~((pte & (PTE_UNSYNCED|PTE_UNMODIFIED)) << 1)*/

	/*
	 * r24-r32 = (no touch)
	 * r23 = scratch (was fault addr)
	 * r22 = scratch
	 * r21 = scratch
	 * r20 = pte
	 * cr2 = AS 0=eq/!0=ne
	 */

	/*
	 * This is all E500 specific.  We should have a patchable branch
	 * to support other BookE (440) implementations.
	 */
e500_pte_load:
	bne+	%cr2, 1f		/* user access? MAS1 is ok. */
	mfspr	%r22, SPR_MAS1		/* get MAS1 */
	lis	%r21, MAS1_TID@h	/* get TID mask */
	andc	%r22, %r22, %r21	/* clear TID */
	mtspr	SPR_MAS1, %r22		/* save MAS1 */
1:
	andi.	%r21, %r20, PTE_WIMGE_MASK /* extract WIMGE from PTE */
	cmplwi	%r21, PTE_M		/* if just PTE_M is set, */
	beq+	%cr0, 2f		/*    skip munging mas2 */
	mfspr	%r22, SPR_MAS2		/* get MAS2 (updated by error) */
	clrrwi	%r22, %r22, PTE_RWX_SHIFT /* clear WIMGE bits */
	or	%r22, %r22, %r21	/* combine with MAS2 contents */
	mtspr	SPR_MAS2, %r22 		/* put back into MAS2 */
2:
	/*
	 * r23 = fault addr
	 * r22 = scratch
	 * r21 = scratch
	 * r20 = pte
	 */

	/*
	 * In MAS3, the protection bits are in the low 6 bits:
	 *    UX SX UW SW UR SR
	 * The User bits are 1 bit left of their Supervisor counterparts.
	 * Rotate the PTE protection bits left until they wrap around to become
	 * the least significant bits, where the Supervisor protection bits
	 * are located.  Increase the rotate amount by 1 to place them where
	 * the User protection bits are located.  We get that 1 by extracting
	 * the MAS1[TS] (set for User access) and moving it to bit 31 (LSB).
	 */
	mfspr	%r21, SPR_MAS1		/* get MAS1 which has TS bit */
	extrwi	%r21, %r21, 1, 31-MAS1_TS_SHIFT
					/* extract MAS1_TS to LSB */
	clrrwi	%r23, %r20, PAGE_SHIFT	/* clear non-RPN bits from PTE */
	andi.	%r20, %r20, PTE_RWX_MASK /* isolate protection bits */
	rotrwi	%r20, %r20, PTE_RWX_SHIFT
	andi.	%r22, %r20, (MAS3_SW|MAS3_SR) /* user pages need to be R/W by kernel */
	rotlw	%r20, %r20, %r21	/* rotate protection to correct loc */
	or	%r20, %r20, %r22	/* combine system protection bits */
	or	%r23, %r23, %r20	/* combine RPN and protection bits */
	mtspr	SPR_MAS3, %r23		/* put into MAS3 */
	isync				/* because ECORE500RM tells us too */
	tlbwe				/* write the TLB entry */
	/*
	 * Increment a counter to show how many tlb misses we've handled here.
	 */
	lmw	%r30, CI_EV_TLBMISS_SOFT(%r2)
	addic	%r31, %r31, 1
	addze	%r30, %r30
	stmw	%r30, CI_EV_TLBMISS_SOFT(%r2)

	FRAME_TLBMISSUNLOCK

	/*
	 * Cleanup and leave.  We know any higher priority exception will
	 * save and restore SPRG1 and %r2 thereby preserving their values.
	 *
	 * r24 = DEAR (don't care)
	 * r25 = ESR (don't care)
	 * r26 = saved r2
	 * r27 = CR
	 * r28 = XER
	 * r29 = LR
	 * r30 = LSW of counter
	 * r31 = MSW of counter
	 */
	mtlr	%r29			/* restore Link Register */
	mtxer	%r28			/* restore XER */
	mtcr	%r27			/* restore Condition Register */
	mtsprg1 %r26			/* save saved r2 across load multiple */
	mfsprg3 %r2			/* get end of save area */
	addi	%r2,%r2,-4*(32-20)	/* adjust save area down */
	lmw	%r20,0(%r2)		/* restore r20-r31 */
	mtsprg3 %r2			/* save new end of save area */
	mfsprg1	%r2			/* restore r2 */
	rfi

	.p2align 4
	.globl _C_LABEL(exception_init)
_C_LABEL(exception_init):
	lis	%r6,_C_LABEL(critical_input_vector)@h
	mtspr	SPR_IVPR, %r6

	ori	%r5,%r6,_C_LABEL(critical_input_vector)@l
	mtspr	SPR_IVOR0, %r5

	ori	%r5,%r6,_C_LABEL(machine_check_vector)@l
	mtspr	SPR_IVOR1, %r5

	ori	%r5,%r6,_C_LABEL(data_storage_vector)@l
	mtspr	SPR_IVOR2, %r5

	ori	%r5,%r6,_C_LABEL(instruction_storage_vector)@l
	mtspr	SPR_IVOR3, %r5

	ori	%r5,%r6,_C_LABEL(external_input_vector)@l
	mtspr	SPR_IVOR4, %r5

	ori	%r5,%r6,_C_LABEL(alignment_vector)@l
	mtspr	SPR_IVOR5, %r5

	ori	%r5,%r6,_C_LABEL(program_vector)@l
	mtspr	SPR_IVOR6, %r5

#ifdef SPR_IVOR7
	ori	%r5,%r6,_C_LABEL(fp_unavailable_vector)@l
	mtspr	SPR_IVOR7, %r5
#endif

	ori	%r5,%r6,_C_LABEL(system_call_vector)@l
	mtspr	SPR_IVOR8, %r5

#ifdef SPR_IVOR9
	ori	%r5,%r6,_C_LABEL(ap_unavailable_vector)@l
	mtspr	SPR_IVOR9, %r5
#endif

	ori	%r5,%r6,_C_LABEL(decrementer_vector)@l
	mtspr	SPR_IVOR10, %r5

	ori	%r5,%r6,_C_LABEL(fixed_interval_timer_vector)@l
	mtspr	SPR_IVOR11, %r5

	ori	%r5,%r6,_C_LABEL(watchdog_timer_vector)@l
	mtspr	SPR_IVOR12, %r5

	ori	%r5,%r6,_C_LABEL(data_tlb_error_vector)@l
	mtspr	SPR_IVOR13, %r5

	ori	%r5,%r6,_C_LABEL(instruction_tlb_error_vector)@l
	mtspr	SPR_IVOR14, %r5

	ori	%r5,%r6,_C_LABEL(debug_vector)@l
	mtspr	SPR_IVOR15, %r5

	ori	%r5,%r6,_C_LABEL(spv_unavailable_vector)@l
	mtspr	SPR_IVOR32, %r5

	ori	%r5,%r6,_C_LABEL(fpdata_vector)@l
	mtspr	SPR_IVOR33, %r5

	ori	%r5,%r6,_C_LABEL(fpround_vector)@l
	mtspr	SPR_IVOR34, %r5

	ori	%r5,%r6,_C_LABEL(perfmon_vector)@l
	mtspr	SPR_IVOR35, %r5

	mfspr	%r5, SPR_PIR	/* get Processor ID register */
	cmplwi	%r5,0
	bnelr			/* return if non-0 (non-primary) */

	lis	%r5,_C_LABEL(powerpc_intrsw)@ha
	stw	%r3,_C_LABEL(powerpc_intrsw)@l(%r5)

	blr

#ifdef notyet
	.data
	.lcomm	ddbstk,4096
	.text

_ENTRY(cpu_Debugger)
	mflr	%r0
	stw	%r0, CFRAME_LR(%r1)
	
	mfmsr	%r3
	wrteei	0
	mr	%r4,%r1
	lis	%r10,ddbstk@ha
	addi	%r10,%r10,ddbstk@l
	sub	%r5,%r1,%r10
	cmplwi	%r5,4096
	blt	%cr0, 1f
	addi	%r1,%r10,4096-CALLFRAMELEN
1:
	stwu	%r4,-FRAMELEN(%r1)
	stw	%r4,FRAME_R1(%r1)
	stmw	%r13,FRAME_R13(%r1)
	mr	%r26,%r0
	mfcr	%r27
	mfxer	%r28
	mfctr	%r29
	mr	%r30,%r0
	mr	%r31,%r3
	stmw	%r26,FRAME_LR(%r1)
	mr	%r31,%r1
	mr	%r1,%r10
	addi	%r4,%r1,FRAME_TF
	li	%r3,EXC_PGM
	stw	%r3,FRAME_EXC(%r1)
	li	%r3,T_PROGRAM
	bl	_C_LABEL(trap)
	lmw	%r26,FRAME_LR(%r1)
	mtlr	%r26
	mtcr	%r27
	mtxer	%r28
	mtctr	%r29
	mr	%r0,%r31
	lmw	%r13,FRAME_R13(%r1)
	lwz	%r1,FRAME_R1(%r1)
	wrtee	%r0
	blr
#endif /* notyet */
