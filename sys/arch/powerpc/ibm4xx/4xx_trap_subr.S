/*	$NetBSD: 4xx_trap_subr.S,v 1.1 2001/06/13 06:01:48 simonb Exp $	*/

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

/* This file provides necessary handlers for 405GP CPU 
 * It should be included in locore.S after powerpc/powerpc/trap_subr.S
 */

	.text
	.globl	_C_LABEL(pitfitwdog),_C_LABEL(pitfitwdogsize)

	.align 4
_C_LABEL(pitfitwdog):
	sync
	ba	pitint
	.align 4
	sync
	ba	fitint
	.align 4
	sync
	ba	wdoghandler
_C_LABEL(pitfitwdogsize) = .-_C_LABEL(pitfitwdog)

pithandler:
	rfi
	ba	.	/* Protect against prefetch */
	
wdoghandler:
	rfi
	ba	.	/* Protect against prefetch */

#define	tlbstacksize	0x1000
#define	tlbsave		0x3000
#define tlbstack	tlbsave+tlbstacksize

/* If an unaligned excception (0x600) and DTLB miss exception (0x1100) 
   occur at the same time, the interrupt vector offsets of the two 
   exceptions are logically OR'ed together to produce 0x1700.
   See PPC405GP Rev D/E Errata item 51 */
	
	.globl _C_LABEL(errata51handler),_C_LABEL(errata51size)
_C_LABEL(errata51handler):
	ba	0x1100
_C_LABEL(errata51size) = .-_C_LABEL(errata51handler)			

	.globl _C_LABEL(tlbdmiss4xx),_C_LABEL(tlbdm4size)
_C_LABEL(tlbdmiss4xx):
	STANDARD_PROLOG(tlbsave)
	mfdear	r30		/* Get fault address */
	mfesr	r31
	stmw	r30,16+tlbsave(0)
	bla	s4xx_miss
_C_LABEL(tlbdm4size) = .-_C_LABEL(tlbdmiss4xx)			

	.globl _C_LABEL(tlbimiss4xx),_C_LABEL(tlbim4size)
_C_LABEL(tlbimiss4xx):	
	STANDARD_PROLOG(tlbsave)
	mfsrr0	r30		/* XXX Get fault address */
	mfesr	r31
	stmw	r30,16+tlbsave(0)
	bla	s4xx_miss
_C_LABEL(tlbim4size) = .-_C_LABEL(tlbdmiss4xx)			

s4xx_miss:
	.globl	_C_LABEL(pmap_tlbmiss)

	/* If the kernel stack would fault, don't use it. */
	mfpid	r30
	li	r31,KERNEL_PID
	mtpid	r31
	li	r31,-FRAMELEN
	tlbsx.	r31,r31,r1
	mtpid	r30
	beq	1f

	/* 
	 * The kernel we want to switch to is not in the TLB.
	 * To solve this problem, we will simulate a kernel
	 * fault on the kernel stack and let the miss handler
	 * bring it in, and return from the trap handler.  The
	 * processor will immediately take the original fault,
	 * which we should be able to handle with the now-valid
	 * kernel stack.
	 */

	/* Switch to tlbstack */
	addi	r30,r1,-FRAMELEN
	lis	r1,tlbstack@ha
	addi	r1,r1,tlbstack@l
	stw	r30,4(1)
	
	FRAME_SETUP(tlbsave)

	/* Take an explicit fault at (kernelstack,pid) */
	lwz	r3, tlbstack+4(0)
	li	r4,KERNEL_PID
	bl	_C_LABEL(pmap_tlbmiss)
	/* 
	 * We can retry the old fault or switch stacks and
	 * take it now.  It's easier to retry.
	 */
	mr.	r3,r3
	beq	2f

	/* kernel stack not in the pmap? we should panic */
	trap
	ba	trapagain
1:	
	FRAME_SETUP(tlbsave)
	lwz	r3,FRAME_DEAR+8(1)
	lwz	r4,FRAME_PID+8(1)
	bl	_C_LABEL(pmap_tlbmiss)
	mr.	r3,r3
	beq	2f

	/* XXX DEBUG -- make sure we're not on tlbstack */
	addi	r7,r1,-tlbsave
	twllei	r7,(tlbstacksize)
	
	/* PTE not found, time to cause a fault */		
	ba	trapagain
2:	
	FRAME_LEAVE(tlbsave)
	rfi
	ba	.	/* Protect against prefetch */
