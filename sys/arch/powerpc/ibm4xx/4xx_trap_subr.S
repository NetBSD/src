/*	$NetBSD: 4xx_trap_subr.S,v 1.5.100.2 2011/05/31 03:04:13 rmind Exp $	*/

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

#define	TLBSTK	0x1000
	.lcomm	tlbstack,TLBSTK,4
	.type	tlbstack,@object

	.text
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
	ACCESS_PROLOG(CI_TLBMISSSAVE)
	bla	s4xx_miss
_C_LABEL(tlbdm4size) = .-_C_LABEL(tlbdmiss4xx)

	.globl _C_LABEL(tlbimiss4xx),_C_LABEL(tlbim4size)
_C_LABEL(tlbimiss4xx):
	ACCESS_PROLOG(CI_TLBMISSSAVE)
	bla	s4xx_miss
_C_LABEL(tlbim4size) = .-_C_LABEL(tlbimiss4xx)

s4xx_miss:
	.globl	_C_LABEL(pmap_tlbmiss)

	/* If the kernel stack would fault, don't use it. */
	mfpid	%r30
	li	%r31,KERNEL_PID
	mtpid	%r31
	li	%r31,-FRAMELEN
	tlbsx.	%r31,%r31,%r1
	mtpid	%r30
	beq	1f

	/*
	 * The kernel stack we want to switch to is not in the TLB.
	 * To solve this problem, we will simulate a kernel
	 * fault on the kernel stack and let the miss handler
	 * bring it in, and return from the trap handler.  The
	 * processor will immediately take the original fault,
	 * which we should be able to handle with the now-valid
	 * kernel stack.
	 */

	/* Switch to tlbstack */
	addi	%r30,%r1,-FRAMELEN
	lis	%r1,tlbstack+TLBSTK-CALLFRAMELEN@ha
	addi	%r1,%r1,tlbstack+TLBSTK-CALLFRAMELEN@l
	stw	%r30,0(%r1)

	FRAME_SETUP(CI_TLBMISSSAVE)

	/* Take an explicit fault at (kernelstack,pid) */
	lwz	%r3,FRAMELEN(%r1)
	li	%r4,KERNEL_PID
	bl	_C_LABEL(pmap_tlbmiss)
	/*
	 * We can retry the old fault or switch stacks and
	 * take it now.  It's easier to retry.
	 */
	mr.	%r3,%r3
	beq	trapexit

	/* kernel stack not in the pmap? we should panic */
	trap
	b	trapagain
1:
	FRAME_SETUP(CI_TLBMISSSAVE)
	li	%r3,EXC_DTMISS
	lwz	%r4,FRAME_EXC(%r1)
	cmpw	%r3,%r4
	lwz	%r3,FRAME_DEAR(%r1)
	beq	2f
	lwz	%r3,FRAME_SRR0(%r1) /* ITMISS case, TLB miss address in SRR0 */
2:
	lwz	%r4,FRAME_PID(%r1)
	bl	_C_LABEL(pmap_tlbmiss)
	mr.	%r3,%r3
	beq	trapexit

	/* XXX DEBUG -- make sure we're not on tlbstack */
	lis	%r3,tlbstack@ha
	addi	%r3,%r3,tlbstack@l
	sub	%r7,%r1,%r3
	twllei	%r7,TLBSTK

	/* PTE not found, time to cause a fault */
	b	trapagain
