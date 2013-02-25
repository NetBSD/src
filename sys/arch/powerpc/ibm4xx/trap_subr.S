/*	$NetBSD: trap_subr.S,v 1.22.6.1 2013/02/25 00:28:53 tls Exp $	*/

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

/*
 * NOTICE: This is not a standalone file.  to use it, #include it in
 * your port's locore.S, like so:
 *
 *	#include <powerpc/ibm4xx/trap_subr.S>
 */

/*
 * XXX Interrupt and spill stacks need to be per-CPU.
 */

#define	GET_PCB(rX)	\
	GET_CPUINFO(rX);	\
	lwz	rX,CI_CURPCB(rX)

#define	STANDARD_PROLOG(savearea)				\
	mtsprg1	%r1;			/* save SP */ 		\
	GET_CPUINFO(%r1);					\
	stmw	%r28,(savearea+CPUSAVE_R28)(%r1); /* free r28-r31 */ \
	mflr	%r28;			/* save LR */		\
	mfcr	%r29;			/* save CR */		\
	mfsrr0	%r30;						\
	mfsrr1	%r31; /* Test whether we already had PR set */	\
	stmw	%r30,(savearea+CPUSAVE_SRR0)(%r1); /* save srr0/srr1 */	\
	mfsprg1	%r1;			/* restore SP */ 	\
	mtcr	%r31;						\
	bf	MSR_PR,1f;		/* branch if MSR[PR] is clear */ \
	GET_PCB(%r1);						\
	addi	%r1,%r1,USPACE-CALLFRAMELEN; /* stack is top of user struct */ \
1:

#define	ACCESS_PROLOG(savearea)					\
	mtsprg1	%r1;			/* save SP temporalily */ \
	GET_CPUINFO(%r1);					\
	stmw	%r28,(savearea+CPUSAVE_R28)(%r1); /* free r28-r31 */ \
	mflr	%r28;			/* save LR */		\
	mfcr	%r29;			/* save CR */		\
	mfdear	%r30;						\
	mfesr	%r31;						\
	stmw	%r30,(savearea+CPUSAVE_DEAR)(%r1);		\
	mfsrr0	%r30;						\
	mfsrr1	%r31; /* Test whether we already had PR set */	\
	stmw	%r30,(savearea+CPUSAVE_SRR0)(%r1); /* save srr0/srr1 */	\
	mfsprg1	%r1;			/* restore SP */ 	\
	mtcr	%r31;						\
	bf	MSR_PR,1f;		/* branch if MSR[PR] is clear */ \
	GET_PCB(%r1);						\
	addi	%r1,%r1,USPACE-CALLFRAMELEN; /* stack is top of user struct */ \
1:

#define	CRITICAL_PROLOG(savearea)				\
	mtsprg1	%r1;			/* save SP */ 		\
	GET_CPUINFO(%r1);					\
	stmw	%r28,(savearea+CPUSAVE_R28)(%r1); /* free r28-r31 */ \
	mflr	%r28;			/* save LR */		\
	mfcr	%r29;			/* save CR */		\
	mfsrr2	%r30; /* Fake a standard trap */		\
	mfsrr3	%r31; /* Test whether we already had PR set */	\
	stmw	%r30,(savearea+CPUSAVE_SRR0)(%r1); /* save srr0/srr1 */	\
	mfsprg1	%r1;			/* restore SP */ 	\
	mtcr	%r31;						\
	bf	MSR_PR,1f;		/* branch if MSR[PR] is clear */ \
	GET_PCB(%r1);						\
	addi	%r1,%r1,USPACE-CALLFRAMELEN; /* stack is top of user struct */ \
1:


/* Standard handler saves r1,r28-31,LR,CR, sets up the stack and calls s_trap */
#define STANDARD_EXC_HANDLER(name)\
	.globl	_C_LABEL(name ## trap),_C_LABEL(name ## size) ;	\
_C_LABEL(name ## trap):						\
	STANDARD_PROLOG(CI_TEMPSAVE);				\
	bla	s_trap;						\
_C_LABEL(name ## size) = .-_C_LABEL(name ## trap)

/* Access exceptions also need DEAR and ESR saved */
#define ACCESS_EXC_HANDLER(name)\
	.globl	_C_LABEL(name ## trap),_C_LABEL(name ## size) ;	\
_C_LABEL(name ## trap):						\
	ACCESS_PROLOG(CI_TEMPSAVE);				\
	bla	s_trap;						\
_C_LABEL(name ## size) = .-_C_LABEL(name ## trap)

/* Maybe this should call ddb.... */
#define CRITICAL_EXC_HANDLER(name)\
	.globl	_C_LABEL(name ## trap),_C_LABEL(name ## size) ;	\
_C_LABEL(name ## trap):						\
	CRITICAL_PROLOG(CI_TEMPSAVE);				\
	bla	s_trap;						\
_C_LABEL(name ## size) = .-_C_LABEL(name ## trap)

#define	INTR_PROLOG(tempsave)					\
	mtsprg1	%r1;			/* save SP */		\
	GET_CPUINFO(%r1);					\
	stmw	%r28,(tempsave+CPUSAVE_R28)(%r1); /* free r28-r31 */ \
	mflr	%r28;			/* save LR */		\
	mfcr	%r29;			/* save CR */		\
	mfxer	%r30;			/* save XER */		\
	mfsrr1	%r31;						\
	mtcr	%r31;						\
	mfsprg1	%r1;			/* restore SP */ 	\
	bf	MSR_PR,1f;		/* branch if PSL_PR is false */ \
	GET_PCB(%r1);						\
	addi	%r1,%r1,USPACE-CALLFRAMELEN; /* stack is top of user struct */ \
1:

/*
 * This code gets copied to all the trap vectors
 * (except ISI/DSI, ALI, the interrupts, and possibly the debugging
 * traps when using IPKDB).
 */
	.text
	STANDARD_EXC_HANDLER(default)
	ACCESS_EXC_HANDLER(ali)
	ACCESS_EXC_HANDLER(dsi)
	ACCESS_EXC_HANDLER(isi)
	STANDARD_EXC_HANDLER(debug)
	CRITICAL_EXC_HANDLER(mchk)

/*
 * This one for the external interrupt handler.
 */
	.globl	_C_LABEL(extint),_C_LABEL(extsize)
_C_LABEL(extint):
	INTR_PROLOG(CI_TEMPSAVE)
	ba	extintr
_C_LABEL(extsize) = .-_C_LABEL(extint)


#if defined(DDB) || defined(KGDB)
/*
 * In case of DDB we want a separate trap catcher for it
 */
	.lcomm	ddbstk,INTSTK,16	/* ddb stack */

	.globl	_C_LABEL(ddblow),_C_LABEL(ddbsize)
_C_LABEL(ddblow):
	mtsprg1	%r1			/* save SP */
	GET_CPUINFO(%r1)
	stmw	%r28,CI_DDBSAVE(%r1)	/* free r28-r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	mfsrr0	%r30
	mfsrr1	%r31
	stmw	%r30,(CI_DDBSAVE+CPUSAVE_SRR0)(%r1)	/* save srr0/srr1 */
	lis	%r1,ddbstk+INTSTK-CALLFRAMELEN@ha	/* get new SP */
	addi	%r1,%r1,ddbstk+INTSTK-CALLFRAMELEN@l
	bla	ddbtrap
_C_LABEL(ddbsize) = .-_C_LABEL(ddblow)
#endif	/* DDB || KGDB */

#ifdef IPKDB
/*
 * In case of IPKDB we want a separate trap catcher for it
 */

	.lcomm	ipkdbstk,INTSTK,16	/* ipkdb stack */

	.globl	_C_LABEL(ipkdblow),_C_LABEL(ipkdbsize)
_C_LABEL(ipkdblow):
	mtsprg1	%r1			/* save SP */
	GET_CPUINFO(%r1)
	stmw	%r28,CI_IPKDBSAVE(%r1)	/* free r28-r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	mfsrr0	%r30
	mfsrr1	%r31
	stmw	%r30,(CI_IPKDBSAVE+CPUSAVE_SRR0)(%r1)	/* save srr0/srr1 */
	lis	%r1,ipkdbstk+INTSTK-CALLFRAMELEN@ha	/* get new SP */
	addi	%r1,%r1,ipkdbstk+INTSTK-CALLFRAMELEN@l
	bla	ipkdbtrap
_C_LABEL(ipkdbsize) = .-_C_LABEL(ipkdblow)
#endif	/* IPKDB */

#ifdef DEBUG
#define TRAP_IF_ZERO(r)	tweqi	r,0
#else
#define TRAP_IF_ZERO(r)
#endif

#define	ENABLE_TRANSLATION(pidreg,tmpreg)				\
	mfpid	pidreg;							\
	li	tmpreg,KERNEL_PID;					\
	mtpid	tmpreg;							\
	mfmsr	tmpreg;							\
	ori	tmpreg,tmpreg,(PSL_DR|PSL_IR)@l;			\
	mtmsr	tmpreg;							\
	isync

/*
 * FRAME_SETUP assumes:
 *	SPRG1		SP (r1)
 *	savearea	r28-r31,DEAR,ESR,SRR0,SRR1 (DEAR & ESR only for DSI traps)
 *	%r28		LR
 *	%r29		CR
 *	%r1		kernel stack
 *	LR		trap type
 */
#define	FRAME_SETUP(savearea)						\
/* Have to enable translation to allow access of kernel stack: */	\
	ENABLE_TRANSLATION(%r30,%r31);					\
	mfsprg1	%r31;							\
	stwu	%r31,-FRAMELEN(%r1);					\
	stw	%r30,FRAME_PID(%r1);					\
	stw	%r0,FRAME_R0(%r1);					\
	stw	%r31,FRAME_R1(%r1);					\
	stw	%r2,FRAME_R2(%r1);					\
	GET_CPUINFO(%r2);						\
	stw	%r28,FRAME_LR(%r1);					\
	stw	%r29,FRAME_CR(%r1);					\
	lmw	%r28,(savearea+CPUSAVE_R28)(%r2);			\
	stmw	%r3,FRAME_R3(%r1);					\
	lmw	%r28,(savearea+CPUSAVE_DEAR)(%r2);			\
	lwz	%r13,CI_CURLWP(%r2);					\
	mfxer	%r3;							\
	mfctr	%r4;							\
	mflr	%r5;							\
	andi.	%r5,%r5,0xff00;						\
	stw	%r3,FRAME_XER(%r1);					\
	stw	%r4,FRAME_CTR(%r1);					\
	stw	%r5,FRAME_EXC(%r1);					\
	stw	%r28,FRAME_DEAR(%r1);					\
	stw	%r29,FRAME_ESR(%r1);					\
	stw	%r30,FRAME_SRR0(%r1);					\
	stw	%r31,FRAME_SRR1(%r1)

#define	FRAME_SAVE_CALLEE						\
	stmw	%r14,FRAME_R14(%r1)

#define	FRAME_RESTORE							\
	lwz	%r6,FRAME_LR(%r1);					\
	lwz	%r7,FRAME_CR(%r1);					\
	lwz	%r8,FRAME_XER(%r1);					\
	lwz	%r9,FRAME_CTR(%r1);					\
	lwz	%r10,FRAME_SRR0(%r1);					\
	lwz	%r11,FRAME_SRR1(%r1);					\
	mtlr	%r6;							\
	mtcr	%r7;							\
	mtxer	%r8;							\
	mtctr	%r9;							\
	mtsrr0	%r10;							\
	mtsrr1	%r11;							\
	lwz	%r13,FRAME_R13(%r1);					\
	lwz	%r12,FRAME_R12(%r1);					\
	lwz	%r11,FRAME_R11(%r1);					\
	lwz	%r10,FRAME_R10(%r1);					\
	lwz	%r9,FRAME_R9(%r1);					\
	lwz	%r8,FRAME_R8(%r1);					\
	lwz	%r7,FRAME_R7(%r1);					\
	lwz	%r6,FRAME_R6(%r1);					\
	lwz	%r5,FRAME_R5(%r1);					\
	lwz	%r4,FRAME_R4(%r1);					\
	lwz	%r3,FRAME_R3(%r1);					\
	lwz	%r2,FRAME_R2(%r1);					\
	lwz	%r0,FRAME_R1(%r1);					\
	mtsprg1	%r0;							\
	lwz	%r0,FRAME_R0(%r1)

/*
 * Now the common trap catching code.
 */
s_trap:
	FRAME_SETUP(CI_TEMPSAVE)
	/* R31 = SRR1 */
/* Now we can recover interrupts again: */
trapagain:
	wrtee	%r31			/* reenable interrupts */
/* Call C trap code: */
	addi	%r3,%r1,FRAME_TF
	bl	_C_LABEL(trap)
	.globl	_C_LABEL(trapexit)
_C_LABEL(trapexit):
	/* Disable interrupts: */
	wrteei	0

	/* Test AST pending: */
	mtcr	%r31
	bf	MSR_PR,trapleave_to_kernel /* branch if MSR[PR] is false */

	lwz	%r4,L_MD_ASTPENDING(%r13)
	andi.	%r4,%r4,1
	beq	trapleave_to_user

	li	%r6,EXC_AST
	stw	%r6,FRAME_EXC(%r1)
	b	trapagain

trapleave_to_kernel:
	lmw	%r14, FRAME_R14(%r1)	/* restore callee registers */

intrleave_to_kernel:
	FRAME_RESTORE		/* old SP is now in sprg1 */

	mtsprg2	%r30
	mtsprg3	%r31
	mfmsr	%r30
	li	%r31,(PSL_DR|PSL_IR)@l
	andc	%r30,%r30,%r31
	lwz	%r31,FRAME_PID(%r1)
	TRAP_IF_ZERO(%r31)
	/*
	 * Now that we are done with the trapframe, we can load the original SP
	 */
	mfsprg1	%r1
	mtmsr	%r30	/* disable translation */
	isync
	mtpid	%r31
	mfsprg3	%r31
	mfsprg2	%r30
	rfi
	ba	.		/* Protect against prefetch */

trapleave_to_user:
	lmw	%r14, FRAME_R14(%r1)	/* restore callee registers */

intrleave_to_user:
/* Now restore regs: */
	lwz	%r3,FRAME_PID(%r1)
	lwz	%r4,FRAME_SRR1(%r1)
	bl	_C_LABEL(ctx_setup)
	TRAP_IF_ZERO(%r3)
	stw	%r3,FRAME_PID(%r1)

	FRAME_RESTORE		/* old SP is now in sprg1 */

	/*
	 * We are returning to userspace so we need to switch PIDs.
	 * Since the kernel executes out of what would be userspace,
	 * we need to turn off translation before we set the PID.
	 *
	 * Alterantively, we could map a kernel page at 0xfffff000
	 * that had the mtpid code in it and branch to it and avoid
	 * all this.  (ba foo; foo: mtpid %r31; mfsprg3 %r31; rfi;)
	 */
	mtsprg2	%r30
	mtsprg3	%r31
	mfmsr	%r30
	li	%r31,(PSL_DR|PSL_IR)@l
	andc	%r30,%r30,%r31
	lwz	%r31,FRAME_PID(%r1)
	TRAP_IF_ZERO(%r31)
	/*
	 * Now that we are done with the trapframe, we can load the original SP
	 */
	mfsprg1	%r1
	mtmsr	%r30	/* disable translation */
	isync
	mtpid	%r31
	mfsprg3	%r31
	mfsprg2	%r30
	rfi
	ba	.	/* Protect against prefetch */


	.globl	_C_LABEL(sctrap),_C_LABEL(scsize),_C_LABEL(sctrapexit)
_C_LABEL(sctrap):
	STANDARD_PROLOG(CI_TEMPSAVE)
	bla	s_sctrap
_C_LABEL(scsize) = .-_C_LABEL(sctrap)

s_sctrap:
	FRAME_SETUP(CI_TEMPSAVE)
/* Now we can recover interrupts again: */
	wrteei	1			/* Enable interrupts */
/* Call the appropriate syscall handler: */
	addi	%r3,%r1,FRAME_TF
	lwz	%r4,L_PROC(%r13)
	lwz	%r4,P_MD_SYSCALL(%r4)
	mtctr	%r4
	bctrl
_C_LABEL(sctrapexit):
	b	trapexit

/*
 * External interrupt second level handler
 */

#define	INTR_SAVE(tempsave)						\
/* Save non-volatile registers: */					\
	stwu	%r1,-FRAMELEN(%r1);	/* temporarily */		\
	stw	%r0,FRAME_R0(%r1);					\
	mfsprg1	%r0;			/* get original SP */		\
	stw	%r0,FRAME_R1(%r1);	/* and store it */		\
	stw	%r2,FRAME_R2(%r1);					\
	stw	%r3,FRAME_R3(%r1);					\
	stw	%r4,FRAME_R4(%r1);					\
	stw	%r5,FRAME_R5(%r1);					\
	stw	%r6,FRAME_R6(%r1);					\
	stw	%r7,FRAME_R7(%r1);					\
	stw	%r8,FRAME_R8(%r1);					\
	stw	%r9,FRAME_R9(%r1);					\
	stw	%r10,FRAME_R10(%r1);					\
	stw	%r11,FRAME_R11(%r1);					\
	stw	%r12,FRAME_R12(%r1);					\
	stw	%r13,FRAME_R13(%r1);					\
	mfctr	%r31;							\
	stmw	%r28,FRAME_LR(%r1);	/* save LR, CR, XER, CTR */	\
	GET_CPUINFO(%r5);						\
	lmw	%r28,(tempsave+CPUSAVE_R28)(%r5); /* restore r28-r31 */	\
	lwz	%r13,CI_CURLWP(%r5);					\
	lwz	%r5,CI_IDEPTH(%r5);					\
	mfsrr0	%r4;							\
	mfsrr1	%r3;							\
	stw	%r5,FRAME_IDEPTH(%r1);					\
	stw	%r4,FRAME_SRR0(%r1);					\
	stw	%r3,FRAME_SRR1(%r1);					\
/* interrupts are recoverable here, and enable translation */		\
	ENABLE_TRANSLATION(%r0,%r5);					\
	stw	%r0,FRAME_PID(%r1);

	.globl	_C_LABEL(extint_call)
extintr:
	INTR_SAVE(CI_TEMPSAVE)
_C_LABEL(extint_call):
	bl	_C_LABEL(extint_call)	/* to be filled in later */

intr_exit:
/* Disable interrupts */
	wrteei	0
	isync

	lwz	%r4,FRAME_SRR1(%r1)
/* Returning to user mode? */
	mtcr	%r4			/* saved SRR1 */
	bf	MSR_PR,intrleave_to_kernel /* branch if MSR[PR] is false */

	lwz	%r4,L_MD_ASTPENDING(%r13)/* Test AST pending */
	andi.	%r4,%r4,1
	beq	intrleave_to_user

	FRAME_SAVE_CALLEE		/* save rest of callee registers */
	li	%r6,EXC_AST
	stw	%r6,FRAME_EXC(%r1)
	lwz	%r31,FRAME_SRR1(%r1)	/* move SRR1 to R31 */
	b	trapagain

/*
 * PIT interrupt handler.
 */
	.align	5
_C_LABEL(pitint):
	INTR_PROLOG(CI_TEMPSAVE)
	INTR_SAVE(CI_TEMPSAVE)
	addi	%r3,%r1,FRAME_CF	/* clock frame */
	bl	_C_LABEL(decr_intr)
	b	intr_exit

/*
 * FIT interrupt handler.
 */
	.align	5
_C_LABEL(fitint):
	INTR_PROLOG(CI_TEMPSAVE)
	INTR_SAVE(CI_TEMPSAVE)
	addi	%r3,%r1,FRAME_CF	/* clock frame */
	bl	_C_LABEL(stat_intr)
	b	intr_exit

#if defined(DDB) || defined(KGDB)
/*
 * Deliberate entry to ddbtrap
 */
	.globl	_C_LABEL(ddb_trap)
_C_LABEL(ddb_trap):
	mtsprg1	%r1
	GET_CPUINFO(%r4)
	mfmsr	%r3
	stw	%r3,(CI_DDBSAVE+CPUSAVE_SRR1)(%r4)
	wrteei	0			/* disable interrupts */
	isync
	stmw	%r28,CI_DDBSAVE(%r4)
	mflr	%r28
	stw	%r28,(CI_DDBSAVE+CPUSAVE_SRR0)(%r4)
	li	%r29,EXC_BPT
	mtlr	%r29
	mfcr	%r29

/*
 * Now the ddb/kgdb trap catching code.
 */
ddbtrap:
	FRAME_SETUP(CI_DDBSAVE)
/* Call C trap code: */
	addi	%r3,%r1,FRAME_TF
	bl	_C_LABEL(ddb_trap_glue)
	or.	%r3,%r3,%r3
	beq	trapagain
	b	trapexit
#endif /* DDB || KGDB */

#ifdef IPKDB
/*
 * Deliberate entry to ipkdbtrap
 */
	.globl	_C_LABEL(ipkdb_trap)
_C_LABEL(ipkdb_trap):
	mtsprg1	%r1
	GET_CPUINFO(%r4)
	mfmsr	%r3
	stw	%r3,(CI_IPKDBSAVE+CPUSAVE_SRR1)(%r4)
	wrteei	0			/* disable interrupts */
	isync
	stmw	%r28,CI_IPKDBSAVE(%r4)
	mflr	%r28
	stw	%r28,(CI_IPKDBSAVE+CPUSAVE_SRR0)(%r4)
	li	%r29,EXC_BPT
	mtlr	%r29
	mfcr	%r29

/*
 * Now the ipkdb trap catching code.
 */
ipkdbtrap:
	FRAME_SETUP(CI_IPKDBSAVE)
/* Call C trap code: */
	addi	%r3,%r1,FRAME_TF
	bl	_C_LABEL(ipkdb_trap_glue)
	or.	%r3,%r3,%r3
	beq	trapagain
	b	trapexit

ipkdbfault:
	ba	_ipkdbfault
_ipkdbfault:
	mfsrr0	%r3
	addi	%r3,%r3,4
	mtsrr0	%r3
	li	%r3,-1
	rfi
	ba	.	/* Protect against prefetch */

/*
 * int ipkdbfbyte(unsigned char *p)
 */
	.globl	_C_LABEL(ipkdbfbyte)
_C_LABEL(ipkdbfbyte):
	li	%r9,EXC_DSI		/* establish new fault routine */
	lwz	%r5,0(%r9)
	lis	%r6,ipkdbfault@ha
	lwz	%r6,ipkdbfault@l(%r6)
	stw	%r6,0(%r9)
#ifdef	IPKDBUSERHACK
#ifndef PPC_IBM4XX
	lis	%r8,_C_LABEL(ipkdbsr)@ha
	lwz	%r8,_C_LABEL(ipkdbsr)@l(%r8)
	mtsr	USER_SR,%r8
	isync
#endif
#endif
	dcbst	%r0,%r9			/* flush data... */
	sync
	icbi	%r0,%r9			/* and instruction caches */
	lbz	%r3,0(%r3)		/* fetch data */
	stw	%r5,0(%r9)		/* restore previous fault handler */
	dcbst	%r0,%r9			/* and flush data... */
	sync
	icbi	%r0,%r9			/* and instruction caches */
	blr

/*
 * int ipkdbsbyte(unsigned char *p, int c)
 */
	.globl	_C_LABEL(ipkdbsbyte)
_C_LABEL(ipkdbsbyte):
	li	%r9,EXC_DSI		/* establish new fault routine */
	lwz	%r5,0(%r9)
	lis	%r6,ipkdbfault@ha
	lwz	%r6,ipkdbfault@l(%r6)
	stw	%r6,0(%r9)
#ifdef	IPKDBUSERHACK
#ifndef PPC_IBM4XX
	lis	%r8,_C_LABEL(ipkdbsr)@ha
	lwz	%r8,_C_LABEL(ipkdbsr)@l(%r8)
	mtsr	USER_SR,%r8
	isync
#endif
#endif
	dcbst	%r0,%r9			/* flush data... */
	sync
	icbi	%r0,%r9			/* and instruction caches */
	mr	%r6,%r3
	xor	%r3,%r3,%r3
	stb	%r4,0(%r6)
	dcbst	%r0,%r6			/* Now do appropriate flushes
					   to data... */
	sync
	icbi	%r0,%r6			/* and instruction caches */
	stw	%r5,0(%r9)		/* restore previous fault handler */
	dcbst	%r0,%r9			/* and flush data... */
	sync
	icbi	%r0,%r9			/* and instruction caches */
	blr
#endif	/* IPKDB */
