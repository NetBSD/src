/*	$NetBSD: trap_subr.S,v 1.12 2011/01/05 18:47:28 matt Exp $	*/

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
 * Data used during primary/secondary traps/interrupts
 */
#define	tempsave	0x2e0	/* primary save area for trap handling */
#define	disisave	0x3e0	/* primary save area for dsi/isi traps */
#define	exitsave	0x4e0	/* use this so trap return does not conflict */
/*
 * XXX Interrupt and spill stacks need to be per-CPU.
 */

#define	GET_PCB(rX)	\
	GET_CPUINFO(rX);	\
	lwz	rX,CI_CURPCB(rX)

#define	STANDARD_PROLOG(savearea)	\
	mtsprg1	%r1;			/* save SP */ 		\
	stmw	%r28,savearea(%r0);	/* free r28-r31 */	\
	mflr	%r28;			/* save LR */		\
	mfcr	%r29;			/* save CR */		\
	mfsrr1	%r31; /* Test whether we already had PR set */	\
	mtcr	%r31;						\
	bc	%cr4,17,1f;	/* branch if PSL_PR is clear */	\
	GET_PCB(%r1);						\
	addi	%r1,%r1,USPACE;	/* stack is top of user struct */ \
1:

#define	CRITICAL_PROLOG(savearea)	\
	mtsprg1	%r1;			/* save SP */ 		\
	stmw	%r28,savearea(%r0);	/* free r28-r31 */	\
	mflr	%r28;			/* save LR */		\
	mfcr	%r29;			/* save CR */		\
	mfsrr2	%r30; /* Fake a standard trap */		\
	mtsrr0	%r30;						\
	mfsrr3	%r31; /* Test whether we already had PR set */	\
	mtsrr1	%r31;						\
	mtcr	%r31;						\
	bc	%cr4,17,1f;	/* branch if PSL_PR is clear */	\
	GET_PCB(%r1);						\
	addi	%r1,%r1,USPACE;	/* stack is top of user struct */ \
1:


/* Standard handler saves r1,r28-31,LR,CR, sets up the stack and calls s_trap */
#define STANDARD_EXC_HANDLER(name)\
	.globl	_C_LABEL(name ## trap),_C_LABEL(name ## size) ;	\
_C_LABEL(name ## trap):						\
	STANDARD_PROLOG(tempsave);				\
	bla	s_trap  ;					\
_C_LABEL(name ## size) = .-_C_LABEL(name ## trap)

/* Access exceptions also need DEAR and ESR saved */
#define ACCESS_EXC_HANDLER(name)\
	.globl	_C_LABEL(name ## trap),_C_LABEL(name ## size) ;	\
_C_LABEL(name ## trap):						\
	STANDARD_PROLOG(tempsave);				\
	mfdear	%r30;						\
	mfesr	%r31;						\
	stmw	%r30,16+tempsave(%r0);				\
	bla	s_trap  ;					\
_C_LABEL(name ## size) = .-_C_LABEL(name ## trap)

/* Maybe this should call ddb.... */
#define CRITICAL_EXC_HANDLER(name)\
	.globl	_C_LABEL(name ## trap),_C_LABEL(name ## size) ;	\
_C_LABEL(name ## trap):						\
	CRITICAL_PROLOG(tempsave);				\
	bla	s_trap  ;					\
_C_LABEL(name ## size) = .-_C_LABEL(name ## trap)

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
	mtsprg1	%r1			/* save SP */
	stmw	%r28,tempsave(%r0)		/* free r28-r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	mfxer	%r30			/* save XER */
	GET_CPUINFO(%r1)
	lwz	%r31,CI_INTRDEPTH(%r1)	/* were we already running on intstk? */
	addic.	%r31,%r31,1
	stw	%r31,CI_INTRDEPTH(%r1)
	lwz	%r1,CI_INTSTK(%r1)	/* get intstk */
	beq	1f
	mfsprg1	%r1			/* yes, get old SP */
1:
	ba	extintr
_C_LABEL(extsize) = .-_C_LABEL(extint)


#if defined(DDB) || defined(KGDB)
#define	ddbsave	0xde0		/* primary save area for DDB */
/*
 * In case of DDB we want a separate trap catcher for it
 */
	.local	ddbstk
	.comm	ddbstk,INTSTK,8		/* ddb stack */

	.globl	_C_LABEL(ddblow),_C_LABEL(ddbsize)
_C_LABEL(ddblow):
	mtsprg1	%r1			/* save SP */
	stmw	%r28,ddbsave(%r0)		/* free r28-r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	lis	%r1,ddbstk+INTSTK@ha	/* get new SP */
	addi	%r1,%r1,ddbstk+INTSTK@l
	bla	ddbtrap
_C_LABEL(ddbsize) = .-_C_LABEL(ddblow)
#endif	/* DDB || KGDB */

#ifdef IPKDB
#define	ipkdbsave	0xde0		/* primary save area for IPKDB */
/*
 * In case of IPKDB we want a separate trap catcher for it
 */

	.local	ipkdbstk
	.comm	ipkdbstk,INTSTK,8		/* ipkdb stack */

	.globl	_C_LABEL(ipkdblow),_C_LABEL(ipkdbsize)
_C_LABEL(ipkdblow):
	mtsprg1	%r1			/* save SP */
	stmw	%r28,ipkdbsave(%r0)		/* free r28-r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	lis	%r1,ipkdbstk+INTSTK@ha	/* get new SP */
	addi	%r1,%r1,ipkdbstk+INTSTK@l
	bla	ipkdbtrap
_C_LABEL(ipkdbsize) = .-_C_LABEL(ipkdblow)
#endif	/* IPKDB */

#ifdef DEBUG
#define TRAP_IF_ZERO(r)	tweqi	r,0
#else
#define TRAP_IF_ZERO(r)
#endif

/*
 * FRAME_SETUP assumes:
 *	SPRG1		SP (r1)
 *	savearea	r28-r31,DEAR,ESR (DEAR & ESR only for DSI traps)
 *	28		LR
 *	29		CR
 *	1		kernel stack
 *	LR		trap type
 *	SRR0/1		as at start of trap
 */
#define	FRAME_SETUP(savearea)						\
/* Have to enable translation to allow access of kernel stack: */	\
	mfsrr0	%r30;							\
	mfsrr1	%r31;							\
	stmw	%r30,savearea+24(%r0);					\
	mfpid	%r30;							\
	li	%r31,KERNEL_PID;					\
	mtpid	%r31;							\
	mfmsr	%r31;							\
	ori	%r31,%r31,(PSL_DR|PSL_IR)@l;				\
	mtmsr	%r31;							\
	isync;								\
	mfsprg1	%r31;							\
	stwu	%r31,-FRAMELEN(%r1);					\
	stw	%r30,FRAME_PID+8(%r1);					\
	stw	%r0,FRAME_0+8(%r1);					\
	stw	%r31,FRAME_1+8(%r1);					\
	stw	%r28,FRAME_LR+8(%r1);					\
	stw	%r29,FRAME_CR+8(%r1);					\
	lmw	%r28,savearea(%r0);					\
	stmw	%r2,FRAME_2+8(%r1);					\
	lmw	%r28,savearea+16(%r0);					\
	mfxer	%r3;							\
	mfctr	%r4;							\
	mflr	%r5;							\
	andi.	%r5,%r5,0xff00;						\
	stw	%r3,FRAME_XER+8(%r1);					\
	stw	%r4,FRAME_CTR+8(%r1);					\
	stw	%r5,FRAME_EXC+8(%r1);					\
	stw	%r28,FRAME_DEAR+8(%r1);					\
	stw	%r29,FRAME_ESR+8(%r1);					\
	stw	%r30,FRAME_SRR0+8(%r1);					\
	stw	%r31,FRAME_SRR1+8(%r1)

#define	FRAME_LEAVE(savearea)						\
/* Now restore regs: */							\
	lwz	%r3,FRAME_PID+8(%r1);					\
	lwz	%r4,FRAME_SRR1+8(%r1);					\
	bl	_C_LABEL(ctx_setup);					\
	TRAP_IF_ZERO(%r3);						\
	stw	%r3,FRAME_PID+8(%r1);					\
	lmw	%r26,FRAME_LR+8(%r1);					\
	mtlr	%r26;							\
	mtcr	%r27;							\
	mtxer	%r28;							\
	mtctr	%r29;							\
	mtsrr0	%r30;							\
	mtsrr1	%r31;							\
	lmw	%r2,FRAME_2+8(%r1);					\
	lwz	%r0,FRAME_0+8(%r1);					\
	stmw	%r29,savearea(%r0);					\
	lwz	%r30,FRAME_PID+8(%r1);					\
	lwz	%r1,FRAME_1+8(%r1);					\
	mfmsr	%r31;							\
	li	%r29,(PSL_DR|PSL_IR)@l;					\
	andc	%r31,%r31,%r29;						\
	mfcr	%r29;							\
	mtcr	%r29;							\
	mtmsr	%r31;							\
	isync;								\
	TRAP_IF_ZERO(%r30);						\
	mtpid	%r30;							\
	lmw	%r29,savearea(%r0)

realtrap:	/* entry point after IPKDB is done with exception */
	/* Test whether we already had PR set */
	mfsrr1	%r1
	mtcr	%r1
	mfsprg1	%r1			/* restore SP (might have been
					   overwritten) */
	bc	%cr4,17,s_trap		/* branch if PSL_PR is false */
	GET_PCB(%r1)
	addi	%r1,%r1,USPACE		/* stack is top of user struct */
/*
 * Now the common trap catching code.
 */
s_trap:
	FRAME_SETUP(tempsave)
/* Now we can recover interrupts again: */
trapagain:
	wrteei	1			/* Enable interrupts */
/* Call C trap code: */
	addi	%r3,%r1,8
	bl	_C_LABEL(trap)
	.globl	_C_LABEL(trapexit)
_C_LABEL(trapexit):
	/* Disable interrupts: */
	wrteei	0
	/* Test AST pending: */
	lwz	%r5,FRAME_SRR1+8(%r1)
	mtcr	%r5
	bc	%cr4,17,1f		/* branch if PSL_PR is false */
	GET_CPUINFO(%r3)
	lwz	%r4,CI_ASTPENDING(%r3)
	andi.	%r4,%r4,1
	beq	1f
	li	%r6,EXC_AST
	stw	%r6,FRAME_EXC+8(%r1)
	b	trapagain
1:
	FRAME_LEAVE(exitsave)
	rfi
	ba	.	/* Protect against prefetch */



	.globl	_C_LABEL(sctrap),_C_LABEL(scsize),_C_LABEL(sctrapexit)
_C_LABEL(sctrap):
	STANDARD_PROLOG(tempsave);
	bla	s_sctrap
_C_LABEL(scsize) = .-_C_LABEL(sctrap)

s_sctrap:
	FRAME_SETUP(tempsave)
/* Now we can recover interrupts again: */
	wrteei	1			/* Enable interrupts */
/* Call the appropriate syscall handler: */
	addi	%r3,%r1,8
	GET_CPUINFO(%r4)
	lwz	%r4,CI_CURLWP(%r4)
	lwz	%r4,L_PROC(%r4)
	lwz	%r4,P_MD_SYSCALL(%r4)
	mtctr	%r4
	bctrl
_C_LABEL(sctrapexit):
/* Disable interrupts: */
	wrteei	0
/* Test AST pending: */
	lwz	%r5,FRAME_SRR1+8(%r1)
	mtcr	%r5
	bc	%cr4,17,1f		/* branch if PSL_PR is false */
	GET_CPUINFO(%r3)
	lwz	%r4,CI_ASTPENDING(%r3)
	andi.	%r4,%r4,1
	beq	1f
	li	%r6,EXC_AST
	stw	%r6,FRAME_EXC+8(%r1)
	b	trapagain
1:
	FRAME_LEAVE(exitsave)
	rfi
	ba	.	/* Protect against prefetch */


/*
 * External interrupt second level handler
 */

#define	INTRENTER							\
/* Save non-volatile registers: */					\
	stwu	%r1,-IFRAMELEN(%r1);	/* temporarily */		\
	stw	%r0,IFRAME_R0(%r1);					\
	mfsprg1	%r0;			/* get original SP */		\
	stw	%r0,IFRAME_R1(%r1);	/* and store it */		\
	stw	%r3,IFRAME_R3(%r1);					\
	stw	%r4,IFRAME_R4(%r1);					\
	stw	%r5,IFRAME_R5(%r1);					\
	stw	%r6,IFRAME_R6(%r1);					\
	stw	%r7,IFRAME_R7(%r1);					\
	stw	%r8,IFRAME_R8(%r1);					\
	stw	%r9,IFRAME_R9(%r1);					\
	stw	%r10,IFRAME_R10(%r1);					\
	stw	%r11,IFRAME_R11(%r1);					\
	stw	%r12,IFRAME_R12(%r1);					\
	stw	%r28,IFRAME_LR(%r1);	/* saved LR */			\
	stw	%r29,IFRAME_CR(%r1);	/* saved CR */			\
	stw	%r30,IFRAME_XER(%r1);	/* saved XER */			\
	lmw	%r28,tempsave(%r0);	/* restore r28-r31 */		\
	mfctr	%r6;							\
	GET_CPUINFO(%r5);						\
	lwz	%r5,CI_INTRDEPTH(%r5);					\
	mfsrr0	%r4;							\
	mfsrr1	%r3;							\
	stw	%r6,IFRAME_CTR(%r1);					\
	stw	%r5,IFRAME_INTR_DEPTH(%r1);				\
	stw	%r4,IFRAME_SRR0(%r1);					\
	stw	%r3,IFRAME_SRR1(%r1);					\
	mfpid	%r0;			/* get currect PID register */  \
	stw	%r0,IFRAME_PID(%r1);					\
	li	%r0,KERNEL_PID;						\
	mtpid	%r0;							\
/* interrupts are recoverable here, and enable translation */		\
	mfmsr	%r5;							\
	ori	%r5,%r5,(PSL_IR|PSL_DR);				\
	mtmsr	%r5;							\
	isync

	.globl	_C_LABEL(extint_call)
extintr:
	INTRENTER
_C_LABEL(extint_call):
	bl	_C_LABEL(extint_call)	/* to be filled in later */

intr_exit:
/* Disable interrupts (should already be disabled) and MMU here: */
	wrteei	0
	isync
	lwz	%r3,IFRAME_PID(%r1)
	lwz	%r4,IFRAME_SRR1(%r1)	/* Load srr1 */
	bl	_C_LABEL(ctx_setup)	/* Get proper ctx */
	mfmsr	%r5
	lis	%r4,(PSL_EE|PSL_DR|PSL_IR)@h
	ori	%r4,%r4,(PSL_EE|PSL_DR|PSL_IR)@l
	andc	%r5,%r5,4
	mtmsr	%r5
	isync
	mtpid	%r3			/* Load CTX */

/* restore possibly overwritten registers: */
	lwz	%r12,IFRAME_R12(%r1)
	lwz	%r11,IFRAME_R11(%r1)
	lwz	%r10,IFRAME_R10(%r1)
	lwz	%r9,IFRAME_R9(%r1)
	lwz	%r8,IFRAME_R8(%r1)
	lwz	%r7,IFRAME_R7(%r1)
	lwz	%r6,IFRAME_SRR1(%r1)
	lwz	%r5,IFRAME_SRR0(%r1)
	lwz	%r4,IFRAME_CTR(%r1)
	lwz	%r3,IFRAME_XER(%r1)
	mtsrr1	%r6
	mtsrr0	%r5
	mtctr	%r4
	mtxer	%r3
/* Returning to user mode? */
	GET_CPUINFO(%r5)
	lwz	%r4,CI_INTRDEPTH(%r5)
	addi	%r4,%r4,-1		/* adjust reentrancy count */
	stw	%r4,CI_INTRDEPTH(%r5)
	mtcr	%r6			/* saved SRR1 */
	bc	%cr4,17,1f		/* branch if PSL_PR is false */

	lwz	%r4,CI_ASTPENDING(%r5)	 /* Test AST pending */
	andi.	%r4,%r4,%r1
	beq	1f
/* Setup for entry to realtrap: */
	lwz	%r3,0(%r1)		/* get saved SP */
	mtsprg1	%r3
	li	%r6,EXC_AST
	stmw	%r28,tempsave(%r0)	/* establish tempsave again */
	mtlr	%r6
	lwz	%r28,IFRAME_LR(%r1)	/* saved LR */
	lwz	%r29,IFRAME_CR(%r1)	/* saved CR */
	lwz	%r6,IFRAME_R6(%r1)
	lwz	%r5,IFRAME_R5(%r1)
	lwz	%r4,IFRAME_R4(%r1)
	lwz	%r3,IFRAME_R3(%r1)
	lwz	%r0,IFRAME_R0(%r1)
	b	realtrap
1:
/* Here is the normal exit of extintr: */
	lwz	%r5,IFRAME_CR(%r1)
	lwz	%r6,IFRAME_LR(%r1)
	mtcr	%r5
	mtlr	%r6
	lwz	%r6,IFRAME_R6(%r1)
	lwz	%r5,IFRAME_R5(%r1)
	lwz	%r4,IFRAME_R4(%r1)
	lwz	%r3,IFRAME_R3(%r1)
	lwz	%r0,IFRAME_R0(%r1)
	lwz	%r1,IFRAME_R1(%r1)
	rfi
	ba	.	/* Protect against prefetch */

/*
 * PIT interrupt handler.
 */
	.align	5
_C_LABEL(pitint):
	mtsprg1	%r1			/* save SP */
	stmw	%r28,tempsave(%r0)	/* free r28-r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	mfxer	%r30			/* save XER */
	GET_CPUINFO(%r1)
	lwz	%r31,CI_INTRDEPTH(%r1)	/* were we already running on intstk? */
	addic.	%r31,%r31,1
	stw	%r31,CI_INTRDEPTH(%r1)
	lwz	%r1,CI_INTSTK(%r1)	/* get intstk */
	beq	1f
	mfsprg1	%r1			/* yes, get old SP */
1:
	INTRENTER
	addi	%r3,%r1,8		/* intr frame */
	bl	_C_LABEL(decr_intr)
	b	intr_exit

/*
 * FIT interrupt handler.
 */
	.align	5
fitint:
	mtsprg1	%r1			/* save SP */
	stmw	%r28,tempsave(%r0)	/* free r28-r31 */
	mflr	%r28			/* save LR */
	mfcr	%r29			/* save CR */
	mfxer	%r30			/* save XER */
	GET_CPUINFO(%r1)
	lwz	%r31,CI_INTRDEPTH(%r1)	/* were we already running on intstk? */
	addic.	%r31,%r31,1
	stw	%r31,CI_INTRDEPTH(%r1)
	lwz	%r1,CI_INTSTK(%r1)	/* get intstk */
	beq	1f
	mfsprg1	%r1			/* yes, get old SP */
1:
	INTRENTER
	addi	%r3,%r1,8		/* intr frame */
	bl	_C_LABEL(stat_intr)
	b	intr_exit

#if defined(DDB) || defined(KGDB)
/*
 * Deliberate entry to ddbtrap
 */
	.globl	_C_LABEL(ddb_trap)
_C_LABEL(ddb_trap):
	mtsprg1	%r1
	mfmsr	%r3
	mtsrr1	%r3
	wrteei	0			/* disable interrupts */
	isync
	stmw	%r28,ddbsave(%r0)
	mflr	%r28
	li	%r29,EXC_BPT
	mtlr	%r29
	mfcr	%r29
	mtsrr0	%r28

/*
 * Now the ddb/kgdb trap catching code.
 */
ddbtrap:
	FRAME_SETUP(ddbsave)
/* Call C trap code: */
	addi	%r3,%r1,8
	bl	_C_LABEL(ddb_trap_glue)
	or.	%r3,%r3,%r3
	bne	ddbleave
/* This wasn't for DDB, so switch to real trap: */
	lwz	%r3,FRAME_EXC+8(%r1)	/* save exception */
	stw	%r3,ddbsave+12(%r0)
	FRAME_LEAVE(ddbsave)
	mtsprg1	%r1			/* prepare for entrance to realtrap */
	stmw	%r28,tempsave(%r0)
	mflr	%r28
	mfcr	%r29
	lwz	%r31,ddbsave+12(%r0)
	mtlr	%r31
	b	realtrap
ddbleave:
	FRAME_LEAVE(ddbsave)
	rfi
	ba	.	/* Protect against prefetch */
#endif /* DDB || KGDB */

#ifdef IPKDB
/*
 * Deliberate entry to ipkdbtrap
 */
	.globl	_C_LABEL(ipkdb_trap)
_C_LABEL(ipkdb_trap):
	mtsprg1	%r1
	mfmsr	%r3
	mtsrr1	%r3
	wrteei	0			/* disable interrupts */
	isync
	stmw	%r28,ipkdbsave(%r0)
	mflr	%r28
	li	%r29,EXC_BPT
	mtlr	%r29
	mfcr	%r29
	mtsrr0	%r28

/*
 * Now the ipkdb trap catching code.
 */
ipkdbtrap:
	FRAME_SETUP(ipkdbsave)
/* Call C trap code: */
	addi	%r3,%r1,8
	bl	_C_LABEL(ipkdb_trap_glue)
	or.	%r3,%r3,%r3
	bne	ipkdbleave
/* This wasn't for IPKDB, so switch to real trap: */
	lwz	%r3,FRAME_EXC+8(%r1)	/* save exception */
	stw	%r3,ipkdbsave+8(%r0)
	FRAME_LEAVE(ipkdbsave)
	mtsprg1	%r1			/* prepare for entrance to realtrap */
	stmw	%r28,tempsave(%r0)
	mflr	%r28
	mfcr	%r29
	lwz	%r31,ipkdbsave+8(%r0)
	mtlr	%r31
	b	realtrap
ipkdbleave:
	FRAME_LEAVE(ipkdbsave)
	rfi
	ba	.	/* Protect against prefetch */

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
