/*	$NetBSD: pthread_switch.S,v 1.8 2003/09/07 14:47:52 cl Exp $	*/

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
	lwz	tmp, PT_TRAPUC(pt)	; \
	or.	tmp, tmp, tmp		; \
	bne	1f			; \
	lwz	tmp, PT_UC(pt)		; \
1:	la	%r1, -STACKSPACE(tmp)	; \
	li	z, 0			; \
	stw	z, PT_TRAPUC(pt)

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
 */
ENTRY(pthread__switch)
#ifdef PIC
	mflr	%r10
	bl	_GLOBAL_OFFSET_TABLE_@local-4
	mflr	%r9
	mtlr	%r10
#endif
	stwu	%r1, -(RND_CTXSIZE + 48)(%r1)	/* alloc stack space */
	mflr	%r0
	stw	%r31,(RND_CTXSIZE + 44)(%r1)	/* saved %r31 */
#ifdef PIC
	stw	%r9, (RND_CTXSIZE + 40)(%r1)
#endif
	stw	%r4, (RND_CTXSIZE + 32)(%r1)	/* next */
	stw	%r3, (RND_CTXSIZE + 28)(%r1)	/* self */
	stw	%r0, (RND_CTXSIZE + 52)(%r1)	/* Save return address */
	addi	%r31, %r1, 16			/* %r31 = ucontext */
	
	/* Get the current context */
	mr	%r3, %r31
	bl	PIC_PLT(_C_LABEL(_getcontext_u))
	lwz	%r3, (RND_CTXSIZE + 28)(%r1)
	lwz	%r4, (RND_CTXSIZE + 32)(%r1)
#ifdef PIC
	lwz	%r9, (RND_CTXSIZE + 40)(%r1)
#endif

	/*
	 * Edit the context to make it continue at switch_return instead of
	 * here.
	 */
#ifdef PIC
	lwz	%r6, pthread__switch_return_point@got(%r9)
#else
	lis	%r6, pthread__switch_return_point@ha
	addi	%r6, %r6, pthread__switch_return_point@l
#endif
	stw	%r6, (_REG_PC + 16)(%r1)
	stw	%r31, PT_UC(%r3)

	STACK_SWITCH(%r4, %r7, %r6)

2:	mr	%r3, %r7
	b	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED

ENTRY_NOPROFILE(pthread__switch_return_point)
	lwz	%r31, (RND_CTXSIZE + 44)(%r1)
	addi	%r1, %r1, (RND_CTXSIZE + 48)
	lwz	%r0, 4(%r1)
	mtlr	%r0
	blr



pthread__switch_away:
	STACK_SWITCH(%r4, %r6, %r7)

	or.	%r5, %r5, %r5
	beq	1f
	lwz	%r7, PT_SPINLOCKS(%r3)
	addi	%r7, %r7, -1
	stw	%r7, PT_SPINLOCKS(%r3)

1:	mr	%r3, %r6
	b	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED

/*
 * void
 * pthread__upcall_switch(pthread_t self (%r3), pthread_t next (%r4))
 */
ENTRY(pthread__upcall_switch)
	lwz	%r6, PT_SPINLOCKS(%r4)
	addi	%r6, %r6, 1
	stw	%r6, PT_SPINLOCKS(%r4)

	STACK_SWITCH(%r4, %r5, %r6)

	lwz	%r7, PT_NEXT(%r3)
	or.	%r7, %r7, %r7
	beq	1f

	stw	%r5, PT_SWITCHTOUC(%r3)
	stw	%r4, PT_SWITCHTO(%r3)
	mr	%r3, %r4
	mr	%r4, %r7
	li	%r5, 1
	b	pthread__switch_away
	NOTREACHED

1:	mr	%r14, %r3
	mr	%r15, %r4
	mr	%r16, %r5
	bl	PIC_PLT(_C_LABEL(pthread__sa_recycle))
	mr	%r3, %r14
	mr	%r4, %r15
	mr	%r5, %r16

	lwz	%r6, PT_SPINLOCKS(%r4)
	addi	%r6, %r6, -1
	stw	%r6, PT_SPINLOCKS(%r4)

	lwz	%r7, PT_NEXT(%r4)
	or.	%r7, %r7, %r7
	beq	2f

	stw	%r5, PT_SWITCHTOUC(%r4)
	stw	%r4, PT_SWITCHTO(%r4)
	mr	%r3, %r4
	mr	%r4, %r7
	li	%r5, 0
	b	pthread__switch_away
	NOTREACHED

2:	mr	%r3, %r16
	b	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED

/*
 * void
 * pthread__locked_switch(pthread_t self (%r3), pthread_t next (%r4),
 *	    pt_spin_t *lock (%r5))
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

	.type locked_return,@function
	
ENTRY(pthread__locked_switch)
#ifdef PIC
	mflr	%r10
	bl	_GLOBAL_OFFSET_TABLE_@local-4
	mflr	%r9
	mtlr	%r10
#endif
	stwu	%r1, -(RND_CTXSIZE + 48)(%r1)	/* alloc stack space */
	mflr	%r0
	stw	%r31,(RND_CTXSIZE + 44)(%r1)	/* saved %r31 */
#ifdef PIC
	stw	%r9, (RND_CTXSIZE + 40)(%r1)
#endif
	stw	%r5, (RND_CTXSIZE + 36)(%r1)	/* lock */
	stw	%r4, (RND_CTXSIZE + 32)(%r1)	/* next */
	stw	%r3, (RND_CTXSIZE + 28)(%r1)	/* self */
	stw	%r0, (RND_CTXSIZE + 52)(%r1)	/* Save return address */
	addi	%r31, %r1, 16			/* %r31 = ucontext */
	
	/* increment spinlock to make sure that next gets continued */
	lwz	%r6, PT_SPINLOCKS(%r4)
	addi	%r6, %r6, 1
	stw	%r6, PT_SPINLOCKS(%r4)

	/* Get the current context */
	mr	%r3, %r31
	bl	PIC_PLT(_C_LABEL(_getcontext_u))
	lwz	%r3, (RND_CTXSIZE + 28)(%r1)
	lwz	%r4, (RND_CTXSIZE + 32)(%r1)
	lwz	%r5, (RND_CTXSIZE + 36)(%r1)
#ifdef PIC
	lwz	%r9, (RND_CTXSIZE + 40)(%r1)
#endif

	/*
	 * Exit the context to make it continue at locked_return instead of
	 * here.
	 */
#ifdef PIC
	lwz	%r6, locked_return@got(%r9)
#else
	lis	%r6, locked_return@ha
	addi	%r6, %r6, locked_return@l
#endif
	stw	%r6, (_REG_PC + 16)(%r1)
	stw	%r31, PT_UC(%r3)

	STACK_SWITCH(%r4, %r7, %r6)

	/* Check if the switcher was preempted and continued to here. */
	lwz	%r8, PT_NEXT(%r3)
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
	stw	%r5, PT_HELDLOCK(%r3)
	stw	%r7, PT_SWITCHTOUC(%r3)
	stw	%r4, PT_SWITCHTO(%r3)
	lwz	%r6, PT_SPINLOCKS(%r3)
	addi	%r6, %r6, -1
	stw	%r6, PT_SPINLOCKS(%r3)

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
	lwz	%r6, PT_SPINLOCKS(%r3)
	addi	%r6, %r6, -1
	stw	%r6, PT_SPINLOCKS(%r3)
	/* ... actually release the lock ... */
	sync
	xor	%r9,%r9,%r9
	stw	%r9, 0(%r5)
	/* ... and remove the fake lock */
	lwz	%r6, PT_SPINLOCKS(%r4)
	addi	%r6, %r6, -1
	stw	%r6, PT_SPINLOCKS(%r4)

	/* Check to see if we were preempted while holding the fake lock */
	lwz	%r8, PT_NEXT(%r4)
	or.	%r8, %r8, %r8
	beq	2f

	/* Yes, we were.  Go to the next element in the chain. */
	stw	%r7, PT_SWITCHTOUC(%r4)
	stw	%r4, PT_SWITCHTO(%r4)
	mr	%r3, %r4
	mr	%r4, %r8
	li	%r5, 0
	b	pthread__switch_away
	NOTREACHED

2:	mr	%r3, %r7
	b	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED

locked_return:
	lwz	%r31, (RND_CTXSIZE + 44)(%r1)
	addi	%r1, %r1, (RND_CTXSIZE + 48)
	lwz	%r0, 4(%r1)
	mtlr	%r0
	blr
