/*	$NetBSD: trap_subr.S,v 1.9 2003/08/11 05:13:21 chs Exp $	*/

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
	mtsprg	1,1;			/* save SP */ 		\
	stmw	28,savearea(0);		/* free r28-r31 */	\
	mflr	28;			/* save LR */		\
	mfcr	29;			/* save CR */		\
	mfsrr1	31; /* Test whether we already had PR set */	\
	mtcr	31;						\
	bc	4,17,1f;	/* branch if PSL_PR is clear */	\
	GET_PCB(1);						\
	addi	1,1,USPACE;	/* stack is top of user struct */ \
1:

#define	CRITICAL_PROLOG(savearea)	\
	mtsprg	1,1;			/* save SP */ 		\
	stmw	28,savearea(0);		/* free r28-r31 */	\
	mflr	28;			/* save LR */		\
	mfcr	29;			/* save CR */		\
	mfsrr2	30; /* Fake a standard trap */			\
	mtsrr0	30;						\
	mfsrr3	31; /* Test whether we already had PR set */	\
	mtsrr1	31;						\
	mtcr	31;						\
	bc	4,17,1f;	/* branch if PSL_PR is clear */	\
	GET_PCB(1);						\
	addi	1,1,USPACE;	/* stack is top of user struct */ \
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
	mfdear	30;						\
	mfesr	31;						\
	stmw	30,16+tempsave(0);				\
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
	mtsprg	1,1			/* save SP */
	stmw	28,tempsave(0)		/* free r28-r31 */
	mflr	28			/* save LR */
	mfcr	29			/* save CR */
	mfxer	30			/* save XER */
	GET_CPUINFO(1)
	lwz	31,CI_INTRDEPTH(1)	/* were we already running on intstk? */
	addic.	31,31,1
	stw	31,CI_INTRDEPTH(1)
	lwz	1,CI_INTSTK(1)		/* get intstk */
	beq	1f
	mfsprg	1,1			/* yes, get old SP */
1:
	ba	extintr
_C_LABEL(extsize) = .-_C_LABEL(extint)


#ifdef DDB
#define	ddbsave	0xde0		/* primary save area for DDB */
/*
 * In case of DDB we want a separate trap catcher for it
 */
	.local	ddbstk
	.comm	ddbstk,INTSTK,8		/* ddb stack */

	.globl	_C_LABEL(ddblow),_C_LABEL(ddbsize)
_C_LABEL(ddblow):
	mtsprg	1,1			/* save SP */
	stmw	28,ddbsave(0)		/* free r28-r31 */
	mflr	28			/* save LR */
	mfcr	29			/* save CR */
	lis	1,ddbstk+INTSTK@ha	/* get new SP */
	addi	1,1,ddbstk+INTSTK@l
	bla	ddbtrap
_C_LABEL(ddbsize) = .-_C_LABEL(ddblow)
#endif	/* DDB */

#ifdef IPKDB
#define	ipkdbsave	0xde0		/* primary save area for IPKDB */
/*
 * In case of IPKDB we want a separate trap catcher for it
 */

	.local	ipkdbstk
	.comm	ipkdbstk,INTSTK,8		/* ipkdb stack */

	.globl	_C_LABEL(ipkdblow),_C_LABEL(ipkdbsize)
_C_LABEL(ipkdblow):
	mtsprg	1,1			/* save SP */
	stmw	28,ipkdbsave(0)		/* free r28-r31 */
	mflr	28			/* save LR */
	mfcr	29			/* save CR */
	lis	1,ipkdbstk+INTSTK@ha	/* get new SP */
	addi	1,1,ipkdbstk+INTSTK@l
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
 *	SPRG1		SP (1)
 *	savearea	r28-r31,DEAR,ESR (DEAR & ESR only for DSI traps)
 *	28		LR
 *	29		CR
 *	1		kernel stack
 *	LR		trap type
 *	SRR0/1		as at start of trap
 */
#define	FRAME_SETUP(savearea)						\
/* Have to enable translation to allow access of kernel stack: */	\
	mfsrr0	30;							\
	mfsrr1	31;							\
	stmw	30,savearea+24(0);					\
	mfpid	30;							\
	li	31,KERNEL_PID;						\
	mtpid	31;							\
	mfmsr	31;							\
	ori	31,31,(PSL_DR|PSL_IR)@l;				\
	mtmsr	31;							\
	isync;								\
	mfsprg	31,1;							\
	stwu	31,-FRAMELEN(1);					\
	stw	30,FRAME_PID+8(1);					\
	stw	0,FRAME_0+8(1);						\
	stw	31,FRAME_1+8(1);					\
	stw	28,FRAME_LR+8(1);					\
	stw	29,FRAME_CR+8(1);					\
	lmw	28,savearea(0);						\
	stmw	2,FRAME_2+8(1);						\
	lmw	28,savearea+16(0);					\
	mfxer	3;							\
	mfctr	4;							\
	mflr	5;							\
	andi.	5,5,0xff00;						\
	stw	3,FRAME_XER+8(1);					\
	stw	4,FRAME_CTR+8(1);					\
	stw	5,FRAME_EXC+8(1);					\
	stw	28,FRAME_DEAR+8(1);					\
	stw	29,FRAME_ESR+8(1);					\
	stw	30,FRAME_SRR0+8(1);					\
	stw	31,FRAME_SRR1+8(1)

#define	FRAME_LEAVE(savearea)						\
/* Now restore regs: */							\
	lwz	3,FRAME_PID+8(1);					\
	lwz	4,FRAME_SRR1+8(1);					\
	bl	_C_LABEL(ctx_setup);					\
	TRAP_IF_ZERO(3);						\
	stw	3,FRAME_PID+8(1);					\
	lmw	26,FRAME_LR+8(1);					\
	mtlr	26;							\
	mtcr	27;							\
	mtxer	28;							\
	mtctr	29;							\
	mtsrr0	30;							\
	mtsrr1	31;							\
	lmw	2,FRAME_2+8(1);						\
	lwz	0,FRAME_0+8(1);						\
	stmw	29,savearea(0);						\
	lwz	30,FRAME_PID+8(1);					\
	lwz	1,FRAME_1+8(1);						\
	mfmsr	31;							\
	li	29,(PSL_DR|PSL_IR)@l;					\
	andc	31,31,29;						\
	mfcr	29;							\
	mtcr	29;							\
	mtmsr	31;							\
	isync;								\
	TRAP_IF_ZERO(30);						\
	mtpid	30;							\
	lmw	29,savearea(0)

realtrap:	/* entry point after IPKDB is done with exception */
	/* Test whether we already had PR set */
	mfsrr1	1
	mtcr	1
	mfsprg	1,1			/* restore SP (might have been
					   overwritten) */
	bc	4,17,s_trap		/* branch if PSL_PR is false */
	GET_PCB(1)
	addi	1,1,USPACE		/* stack is top of user struct */
/*
 * Now the common trap catching code.
 */
s_trap:
	FRAME_SETUP(tempsave)
/* Now we can recover interrupts again: */
trapagain:
	wrteei	1			/* Enable interrupts */
/* Call C trap code: */
	addi	3,1,8
	bl	_C_LABEL(trap)
	.globl	_C_LABEL(trapexit)
_C_LABEL(trapexit):
	/* Disable interrupts: */
	wrteei	0
	/* Test AST pending: */
	lwz	5,FRAME_SRR1+8(1)
	mtcr	5
	bc	4,17,1f			/* branch if PSL_PR is false */
	GET_CPUINFO(3)
	lwz	4,CI_ASTPENDING(3)
	andi.	4,4,1
	beq	1f
	li	6,EXC_AST
	stw	6,FRAME_EXC+8(1)
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
	addi	3,1,8
	GET_CPUINFO(4)
	lwz	4,CI_CURLWP(4)
	lwz	4,L_PROC(4)
	lwz	4,P_MD_SYSCALL(4)
	mtctr	4
	bctrl
_C_LABEL(sctrapexit):
/* Disable interrupts: */
	wrteei	0
/* Test AST pending: */
	lwz	5,FRAME_SRR1+8(1)
	mtcr	5
	bc	4,17,1f			/* branch if PSL_PR is false */
	GET_CPUINFO(3)
	lwz	4,CI_ASTPENDING(3)
	andi.	4,4,1
	beq	1f
	li	6,EXC_AST
	stw	6,FRAME_EXC+8(1)
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
	stwu	1,-IFRAMELEN(1);	/* temporarily */		\
	stw	0,IFRAME_R0(1);						\
	mfsprg	0,1;			/* get original SP */		\
	stw	0,IFRAME_R1(1);		/* and store it */		\
	stw	3,IFRAME_R3(1);						\
	stw	4,IFRAME_R4(1);						\
	stw	5,IFRAME_R5(1);						\
	stw	6,IFRAME_R6(1);						\
	stw	7,IFRAME_R7(1);						\
	stw	8,IFRAME_R8(1);						\
	stw	9,IFRAME_R9(1);						\
	stw	10,IFRAME_R10(1);					\
	stw	11,IFRAME_R11(1);					\
	stw	12,IFRAME_R12(1);					\
	stw	28,IFRAME_LR(1);	/* saved LR */			\
	stw	29,IFRAME_CR(1);	/* saved CR */			\
	stw	30,IFRAME_XER(1);	/* saved XER */			\
	lmw	28,tempsave(0);		/* restore r28-r31 */		\
	mfctr	6;							\
	GET_CPUINFO(5);							\
	lwz	5,CI_INTRDEPTH(5);					\
	mfsrr0	4;							\
	mfsrr1	3;							\
	stw	6,IFRAME_CTR(1);					\
	stw	5,IFRAME_INTR_DEPTH(1);					\
	stw	4,IFRAME_SRR0(1);					\
	stw	3,IFRAME_SRR1(1);					\
	mfpid	0;			/* get currect PID register */  \
	stw	0,IFRAME_PID(1);					\
	li	0,KERNEL_PID;						\
	mtpid	0;							\
/* interrupts are recoverable here, and enable translation */		\
	mfmsr	5;							\
	ori	5,5,(PSL_IR|PSL_DR);					\
	mtmsr	5;							\
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
	lwz	3,IFRAME_PID(1)
	lwz	4,IFRAME_SRR1(1)	/* Load srr1 */
	bl	_C_LABEL(ctx_setup)	/* Get proper ctx */
	mfmsr	5
	lis	4,(PSL_EE|PSL_DR|PSL_IR)@h
	ori	4,4,(PSL_EE|PSL_DR|PSL_IR)@l
	andc	5,5,4
	mtmsr	5
	isync
	mtpid	3			/* Load CTX */

/* restore possibly overwritten registers: */
	lwz	12,IFRAME_R12(1)
	lwz	11,IFRAME_R11(1)
	lwz	10,IFRAME_R10(1)
	lwz	9,IFRAME_R9(1)
	lwz	8,IFRAME_R8(1)
	lwz	7,IFRAME_R7(1)
	lwz	6,IFRAME_SRR1(1)
	lwz	5,IFRAME_SRR0(1)
	lwz	4,IFRAME_CTR(1)
	lwz	3,IFRAME_XER(1)
	mtsrr1	6
	mtsrr0	5
	mtctr	4
	mtxer	3
/* Returning to user mode? */
	GET_CPUINFO(5)
	lwz	4,CI_INTRDEPTH(5)
	addi	4,4,-1			/* adjust reentrancy count */
	stw	4,CI_INTRDEPTH(5)
	mtcr	6			/* saved SRR1 */
	bc	4,17,1f			/* branch if PSL_PR is false */

	lwz	4,CI_ASTPENDING(5)	 /* Test AST pending */
	andi.	4,4,1
	beq	1f
/* Setup for entry to realtrap: */
	lwz	3,0(1)			/* get saved SP */
	mtsprg	1,3
	li	6,EXC_AST
	stmw	28,tempsave(0)		/* establish tempsave again */
	mtlr	6
	lwz	28,IFRAME_LR(1)		/* saved LR */
	lwz	29,IFRAME_CR(1)		/* saved CR */
	lwz	6,IFRAME_R6(1)
	lwz	5,IFRAME_R5(1)
	lwz	4,IFRAME_R4(1)
	lwz	3,IFRAME_R3(1)
	lwz	0,IFRAME_R0(1)
	b	realtrap
1:
/* Here is the normal exit of extintr: */
	lwz	5,IFRAME_CR(1)
	lwz	6,IFRAME_LR(1)
	mtcr	5
	mtlr	6
	lwz	6,IFRAME_R6(1)
	lwz	5,IFRAME_R5(1)
	lwz	4,IFRAME_R4(1)
	lwz	3,IFRAME_R3(1)
	lwz	0,IFRAME_R0(1)
	lwz	1,IFRAME_R1(1)
	rfi
	ba	.	/* Protect against prefetch */

/*
 * PIT interrupt handler.
 */
	.align	5
_C_LABEL(pitint):
	mtsprg	1,1			/* save SP */
	stmw	28,tempsave(0)		/* free r28-r31 */
	mflr	28			/* save LR */
	mfcr	29			/* save CR */
	mfxer	30			/* save XER */
	GET_CPUINFO(1)
	lwz	31,CI_INTRDEPTH(1)	/* were we already running on intstk? */
	addic.	31,31,1
	stw	31,CI_INTRDEPTH(1)
	lwz	1,CI_INTSTK(1)		/* get intstk */
	beq	1f
	mfsprg	1,1			/* yes, get old SP */
1:
	INTRENTER
	addi	3,1,8			/* intr frame */
	bl	_C_LABEL(decr_intr)
	b	intr_exit

/*
 * FIT interrupt handler.
 */
	.align	5
fitint:
	mtsprg	1,1			/* save SP */
	stmw	28,tempsave(0)		/* free r28-r31 */
	mflr	28			/* save LR */
	mfcr	29			/* save CR */
	mfxer	30			/* save XER */
	GET_CPUINFO(1)
	lwz	31,CI_INTRDEPTH(1)	/* were we already running on intstk? */
	addic.	31,31,1
	stw	31,CI_INTRDEPTH(1)
	lwz	1,CI_INTSTK(1)		/* get intstk */
	beq	1f
	mfsprg	1,1			/* yes, get old SP */
1:
	INTRENTER
	addi	3,1,8			/* intr frame */
	bl	_C_LABEL(stat_intr)
	b	intr_exit

#ifdef DDB
/*
 * Deliberate entry to ddbtrap
 */
	.globl	_C_LABEL(ddb_trap)
_C_LABEL(ddb_trap):
	mtsprg	1,1
	mfmsr	3
	mtsrr1	3
	wrteei	0			/* disable interrupts */
	isync
	stmw	28,ddbsave(0)
	mflr	28
	li	29,EXC_BPT
	mtlr	29
	mfcr	29
	mtsrr0	28

/*
 * Now the ddb trap catching code.
 */
ddbtrap:
	FRAME_SETUP(ddbsave)
/* Call C trap code: */
	addi	3,1,8
	bl	_C_LABEL(ddb_trap_glue)
	or.	3,3,3
	bne	ddbleave
/* This wasn't for DDB, so switch to real trap: */
	lwz	3,FRAME_EXC+8(1)	/* save exception */
	stw	3,ddbsave+12(0)
	FRAME_LEAVE(ddbsave)
	mtsprg	1,1			/* prepare for entrance to realtrap */
	stmw	28,tempsave(0)
	mflr	28
	mfcr	29
	lwz	31,ddbsave+12(0)
	mtlr	31
	b	realtrap
ddbleave:
	FRAME_LEAVE(ddbsave)
	rfi
	ba	.	/* Protect against prefetch */
#endif /* DDB */

#ifdef IPKDB
/*
 * Deliberate entry to ipkdbtrap
 */
	.globl	_C_LABEL(ipkdb_trap)
_C_LABEL(ipkdb_trap):
	mtsprg	1,1
	mfmsr	3
	mtsrr1	3
	wrteei	0			/* disable interrupts */
	isync
	stmw	28,ipkdbsave(0)
	mflr	28
	li	29,EXC_BPT
	mtlr	29
	mfcr	29
	mtsrr0	28

/*
 * Now the ipkdb trap catching code.
 */
ipkdbtrap:
	FRAME_SETUP(ipkdbsave)
/* Call C trap code: */
	addi	3,1,8
	bl	_C_LABEL(ipkdb_trap_glue)
	or.	3,3,3
	bne	ipkdbleave
/* This wasn't for IPKDB, so switch to real trap: */
	lwz	3,FRAME_EXC+8(1)	/* save exception */
	stw	3,ipkdbsave+8(0)
	FRAME_LEAVE(ipkdbsave)
	mtsprg	1,1			/* prepare for entrance to realtrap */
	stmw	28,tempsave(0)
	mflr	28
	mfcr	29
	lwz	31,ipkdbsave+8(0)
	mtlr	31
	b	realtrap
ipkdbleave:
	FRAME_LEAVE(ipkdbsave)
	rfi
	ba	.	/* Protect against prefetch */

ipkdbfault:
	ba	_ipkdbfault
_ipkdbfault:
	mfsrr0	3
	addi	3,3,4
	mtsrr0	3
	li	3,-1
	rfi
	ba	.	/* Protect against prefetch */

/*
 * int ipkdbfbyte(unsigned char *p)
 */
	.globl	_C_LABEL(ipkdbfbyte)
_C_LABEL(ipkdbfbyte):
	li	9,EXC_DSI		/* establish new fault routine */
	lwz	5,0(9)
	lis	6,ipkdbfault@ha
	lwz	6,ipkdbfault@l(6)
	stw	6,0(9)
#ifdef	IPKDBUSERHACK
#ifndef PPC_IBM4XX
	lis	8,_C_LABEL(ipkdbsr)@ha
	lwz	8,_C_LABEL(ipkdbsr)@l(8)
	mtsr	USER_SR,8
	isync
#endif
#endif
	dcbst	0,9			/* flush data... */
	sync
	icbi	0,9			/* and instruction caches */
	lbz	3,0(3)			/* fetch data */
	stw	5,0(9)			/* restore previous fault handler */
	dcbst	0,9			/* and flush data... */
	sync
	icbi	0,9			/* and instruction caches */
	blr

/*
 * int ipkdbsbyte(unsigned char *p, int c)
 */
	.globl	_C_LABEL(ipkdbsbyte)
_C_LABEL(ipkdbsbyte):
	li	9,EXC_DSI		/* establish new fault routine */
	lwz	5,0(9)
	lis	6,ipkdbfault@ha
	lwz	6,ipkdbfault@l(6)
	stw	6,0(9)
#ifdef	IPKDBUSERHACK
#ifndef PPC_IBM4XX
	lis	8,_C_LABEL(ipkdbsr)@ha
	lwz	8,_C_LABEL(ipkdbsr)@l(8)
	mtsr	USER_SR,8
	isync
#endif
#endif
	dcbst	0,9			/* flush data... */
	sync
	icbi	0,9			/* and instruction caches */
	mr	6,3
	xor	3,3,3
	stb	4,0(6)
	dcbst	0,6			/* Now do appropriate flushes
					   to data... */
	sync
	icbi	0,6			/* and instruction caches */
	stw	5,0(9)			/* restore previous fault handler */
	dcbst	0,9			/* and flush data... */
	sync
	icbi	0,9			/* and instruction caches */
	blr
#endif	/* IPKDB */
