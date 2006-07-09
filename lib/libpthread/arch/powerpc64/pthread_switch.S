/*	$NetBSD: pthread_switch.S,v 1.2 2006/07/09 01:07:38 ross Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Allen Briggs for Wasabi Systems, Inc.
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

#include <machine/asm.h>
#include "assym.h"

#define NOTREACHED

/*
 * PowerPC ABI says stack pointer must be 16-byte aligned.
 */
#define RND_CTXSIZE ((CONTEXTSIZE + 15) & 0xfffffff0)

/*
 * Define:
 *	void pthread__switch(pthread_t self, pthread_t next)
 *	void pthread__upcall_switch(pthread_t self, pthread_t next)
 *	void pthread__locked_switch(pthread_t self, pthread_t next,
 *	    pt_spin_t *lock)
 */


/*
 * Evil STACK_SWITCH()
 * See comments in ../i386/pthread_switch.S.
 */
#define STACK_SWITCH(pt,tmp,z)		  \
	ld	tmp, PT_TRAPUC(pt)	; \
	or.	tmp, tmp, tmp		; \
	bne	1f			; \
	ld	tmp, PT_UC(pt)		; \
1:	la	%r1, -STACKSPACE(tmp)	; \
	li	z, 0			; \
	std	z, PT_TRAPUC(pt)

/*
 * void
 * pthread__switch(pthread_t self (%r3), pthread_t next (%r4))
 *
 * Plain switch that doesn't do any special checking or handle spin-
 * preemption.  It isn't used much by normal code, actually; its main
 * purpose is to be a basic switch engine when the MI code is already
 * dealing with spin-preemption or other gunk.
 *
 * What we do here is allocate the ucontext for the 'self' thread on its
 * stack, saving the pointer in self's pthread structure, then swapping
 * context to 'next', effectively deallocating next's context on the way
 * out.
 *
 * Stack:
 *	------------------------------------------------------------------
 *	highest addresses
 *		saved r31	8		(offset is framesize - 8)
 *		pad		8	pad r31 space up to 16
 *		pad to round ucontext_t up to a 16 byte boundary
 *		ucontext_t			(offset is SF_SZ	)
 *		other elf stack frame		(SF_PARAM_SZ		)
 *		elf stack frame header		(SF_HEADER_SZ		)
 *	lowest addresses
 *	------------------------------------------------------------------
 */
ENTRY(pthread__switch)
	frameSize = SF_SZ + RND_CTXSIZE + 16 	# need +8 for r31, & align
	saved31 = frameSize - 8
.macro	init_frame
	stdu	%r1, -frameSize(%r1)	/* alloc stack space */
	mflr	%r0
	std	%r31,saved31(%r1)
	std	%r3, SF_PARAM+0(%r1)		/* self */
	std	%r4, SF_PARAM+8(%r1)		/* next */
	std	%r5, SF_PARAM+16(%r1)		/* lock, for locked_switch */
	std	%r0, frameSize+SF_LR(%r1)	/* Save return address */
	la	%r31, SF_SZ(%r1)		/* %r31 = ucontext */
.endm
	init_frame
	
.macro	switch_context
	/* Get the current context */
	mr	%r3, %r31
	CALL(_getcontext_u)
	ld	%r3, SF_PARAM+0(%r1)
	ld	%r4, SF_PARAM+8(%r1)
	ld	%r5, SF_PARAM+16(%r1)
	/*
	 * Edit the context to make it continue at switch_return instead of
	 * here.
	 */
	ld	%r6, pthread__switch_return_point@got(%r2)
	std	%r6, SF_SZ+_REG_PC(%r1)
	std	%r31, PT_UC(%r3)

	STACK_SWITCH(%r4, %r7, %r6)
.endm
	switch_context

.macro	set_context
	mr	%r3, %r7
	b	._setcontext_u
.endm
	set_context
	NOTREACHED



pthread__switch_away:
	STACK_SWITCH(%r4, %r6, %r7)

	or.	%r5, %r5, %r5
	beq	1f
	lwz	%r7, PT_SPINLOCKS(%r3)	# w OK
	addi	%r7, %r7, -1
	stw	%r7, PT_SPINLOCKS(%r3)	# w OK

1:	mr	%r3, %r6
	b	._setcontext_u
	NOTREACHED

/*
 * void
 * pthread__upcall_switch(pthread_t self (%r3), pthread_t next (%r4))
 */
ENTRY(pthread__upcall_switch)
	lwz	%r6, PT_SPINLOCKS(%r4)	# w OK
	addi	%r6, %r6, 1
	stw	%r6, PT_SPINLOCKS(%r4)	# w OK

	STACK_SWITCH(%r4, %r5, %r6)

	ld	%r7, PT_NEXT(%r3)
	or.	%r7, %r7, %r7
	beq	1f

	std	%r5, PT_SWITCHTOUC(%r3)
	std	%r4, PT_SWITCHTO(%r3)
	mr	%r3, %r4
	mr	%r4, %r7
	li	%r5, 1
	b	pthread__switch_away
	NOTREACHED

1:	mr	%r14, %r3
	mr	%r15, %r4
	mr	%r16, %r5
	CALL(pthread__sa_recycle)
	mr	%r3, %r14
	mr	%r4, %r15
	mr	%r5, %r16

	lwz	%r6, PT_SPINLOCKS(%r4)	# w OK
	addi	%r6, %r6, -1
	stw	%r6, PT_SPINLOCKS(%r4)	# w OK

	ld	%r7, PT_NEXT(%r4)
	or.	%r7, %r7, %r7
	beq	2f

	std	%r5, PT_SWITCHTOUC(%r4)
	std	%r4, PT_SWITCHTO(%r4)
	mr	%r3, %r4
	mr	%r4, %r7
	li	%r5, 0
	b	pthread__switch_away
	NOTREACHED

2:	mr	%r3, %r16
	b	._setcontext_u
	NOTREACHED

/*
 * void
 * pthread__locked_switch(pthread_t self (%r3), pthread_t next (%r4),
 *	    pt_spin_t *lock (%r5))
 *
 * THIS STACK FRAME APPLIES ONLY TO THE ORIGINAL ILP32 CODE
 *
 * Stack is:
 * high addr -- return addr		(          4 bytes)
 * %r1@call     caller saved %r1	(          4 bytes)
 *              any padding to make %r1 a multiple of 16 ... *
 *              saved %r31		(          4 bytes) *
 *              saved %r9		(          4 bytes) *
 *              saved %r5		(          4 bytes) *
 *              saved %r4		(          4 bytes) *
 *              saved %r3		(          4 bytes) *
 *              context			(RND_CTXSIZE bytes) *
 *              padding to 16 bytes	(         20 bytes) *
 *              space for callee ra	(          4 bytes) *
 * low addr     p__l_s saved %r1	(          4 bytes) *
 *
 *     STACKSPACE is the space between the bottom of the stack and
 * the ucontext on the stack.  i.e., 8, but we want to keep the stack
 * rounded to a multiple of 16, so it's really 16.
 */

ENTRY(pthread__locked_switch)

	init_frame

	/* increment spinlock to make sure that next gets continued */
	lwz	%r6, PT_SPINLOCKS(%r4)	# w OK
	addi	%r6, %r6, 1
	stw	%r6, PT_SPINLOCKS(%r4)	# w OK

	switch_context

	/* Check if the switcher was preempted and continued to here. */
	ld	%r8, PT_NEXT(%r3)
	or.	%r8, %r8, %r8
	beq	1f

	/*
	 * Yes, it was.  Stash the thread we were going to switch to,
	 * the lock the original thread was holding, and switch to the
	 * next thread in the continuation chain.  Mark the fact that
	 * this was a locked switch, and so the thread does not need to
	 * be put on a run queue.
	 * Don't release the lock.  It's possible that if we do so,
	 * PT_SWITCHTO will be stomped by another switch_lock and preemption.
	 */
	std	%r5, PT_HELDLOCK(%r3)
	std	%r7, PT_SWITCHTOUC(%r3)
	std	%r4, PT_SWITCHTO(%r3)
	lwz	%r6, PT_SPINLOCKS(%r3)	# w OK
	addi	%r6, %r6, -1
	stw	%r6, PT_SPINLOCKS(%r3)	# w OK

	mr	%r3, %r4
	mr	%r4, %r8
	li	%r5, 1
	b	pthread__switch_away
	NOTREACHED

	/* No locked old-preemption */
1:	/*
	 * We've moved to the new stack, and the old context has been
	 * saved.  The queue lock can be released.
	 */
	/* Reduce the lock count... */
	lwz	%r6, PT_SPINLOCKS(%r3)	# w OK
	addi	%r6, %r6, -1
	stw	%r6, PT_SPINLOCKS(%r3)	# w OK
	/* ... actually release the lock ... */
	sync
	xor	%r9,%r9,%r9
	std	%r9, 0(%r5)
	/* ... and remove the fake lock */
	lwz	%r6, PT_SPINLOCKS(%r4)	# w OK
	addi	%r6, %r6, -1
	stw	%r6, PT_SPINLOCKS(%r4)	# w OK

	/* Check to see if we were preempted while holding the fake lock */
	ld	%r8, PT_NEXT(%r4)
	or.	%r8, %r8, %r8
	beq	2f

	/* Yes, we were.  Go to the next element in the chain. */
	std	%r7, PT_SWITCHTOUC(%r4)
	std	%r4, PT_SWITCHTO(%r4)
	mr	%r3, %r4
	mr	%r4, %r8
	li	%r5, 0
	b	pthread__switch_away
	NOTREACHED

2:	set_context
	NOTREACHED


ENTRY(pthread__switch_return_point)
	ld	%r31, saved31(%r1)
	ld	%r0, frameSize+SF_LR(%r1)
	la	%r1, frameSize(%r1)
	mtlr	%r0
	blr
