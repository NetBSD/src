/*	$NetBSD: pthread_switch.S,v 1.1.2.1 2001/11/13 20:21:24 briggs Exp $	*/

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
 * void
 * pthread__switch(pthread_t self (r3), pthread_t next (r4))
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
	stwu	1, -(RND_CTXSIZE + 16)(1)	/* alloc ucontext_t + r1 */
	mflr	0
	stw	0, (RND_CTXSIZE + 20)(1)	/* Save return address */
	
	/* Save pointer to saved space on stack in self->pt_uc */
	la	5, 8(1)
	stw	5, PT_UC(3)	

	mr	3, 5				/* self->pt_uc */
	lwz	4, PT_UC(4)			/* next->pt_uc */
	bl	PIC_PLT(_C_LABEL(_swapcontext_u))

	addi	1, 1, (RND_CTXSIZE + 16)
	lwz	0, 4(1)
	mtlr	0
	blr

/*
 * Evil STACK_SWITCH()
 * See comments in ../i386/pthread_switch.S.
 */
#define STACK_SWITCH(pt,tmp)		  \
	lwz	tmp, PT_UC(pt)		; \
	la	1, -STACKSPACE(tmp)

pthread__switch_away:
	STACK_SWITCH(4,6)

	or.	5, 5, 5
	beq	1f
	lwz	0, PT_SPINLOCKS(3)
	addi	0, 0, -1
	stw	0, PT_SPINLOCKS(3)

1:	mr	3, 6
	b	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED

/*
 * void
 * pthread__upcall_switch(pthread_t self (r3), pthread_t next (r4))
 */
ENTRY(pthread__upcall_switch)
	lwz	6, PT_SPINLOCKS(4)
	addi	6, 6, 1
	stw	6, PT_SPINLOCKS(4)

	STACK_SWITCH(4,5)

	lwz	7, PT_NEXT(3)
	or.	7, 7, 7
	beq	1f

	stw	4, PT_SWITCHTO(3)
	stw	5, PT_SWITCHTOUC(3)
	li	5, PT_STATE_RECYCLABLE
	stw	5, PT_STATE(3)
	mr	3, 4
	mr	4, 7
	li	5, 1
	b	pthread__switch_away
	NOTREACHED

1:	mr	14, 3
	mr	15, 4
	mr	16, 5
	bl	PIC_PLT(_C_LABEL(pthread__sa_recycle))
	mr	3, 14
	mr	4, 15

	lwz	6, PT_SPINLOCKS(4)
	addi	6, 6, -1
	stw	6, PT_SPINLOCKS(4)

	lwz	7, PT_NEXT(4)
	or.	7, 7, 7
	beq	2f

	stw	4, PT_SWITCHTO(4)
	stw	5, PT_SWITCHTOUC(4)
	mr	3, 4
	mr	4, 7
	li	5, 0
	b	pthread__switch_away
	NOTREACHED

2:	mr	3, 16
	b	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED

/*
 * void
 * pthread__locked_switch(pthread_t self (r3), pthread_t next (r4),
 *	    pt_spin_t *lock (r5))
 *
 * Stack is:
 * high addr -- return addr		(          4 bytes)
 * r1@call      caller saved r1		(          4 bytes)
 *              any padding to make r1 a multiple of 16 ... *
 *              saved r31		(          4 bytes) *
 *              saved r5		(          4 bytes) *
 *              saved r4		(          4 bytes) *
 *              saved r3		(          4 bytes) *
 *              context			(RND_CTXSIZE bytes) *
 *              space for callee ra	(          4 bytes) *
 * low addr     p__l_s saved r1		(          4 bytes) *
 *
 *     STACKSPACE is the space between the bottom of the stack and
 * the ucontext on the stack.  i.e., 8.
 */
ENTRY(pthread__locked_switch)
	stwu	1, -(RND_CTXSIZE + 32)(1)	/* alloc stack space */
	mflr	0
	stw	31,(RND_CTXSIZE + 20)(1)	/* saved r31 */
	stw	5, (RND_CTXSIZE + 16)(1)	/* lock */
	stw	4, (RND_CTXSIZE + 12)(1)	/* next */
	stw	3, (RND_CTXSIZE +  8)(1)	/* self */
	stw	0, (RND_CTXSIZE + 36)(1)	/* Save return address */
	addi	31, 1, 8			/* r31 = ucontext */
	
	/* increment spinlock to make sure that next gets continued */
	lwz	6, PT_SPINLOCKS(4)
	addi	6, 6, 1
	stw	6, PT_SPINLOCKS(4)

	/* Get the current context */
	stw	31, PT_UC(3)
	mr	3, 31
	bl	PIC_PLT(_C_LABEL(_getcontext_u))
	lwz	3, (RND_CTXSIZE +  8)(1)
	lwz	4, (RND_CTXSIZE + 12)(1)
	lwz	5, (RND_CTXSIZE + 16)(1)

	/*
	 * Exit the context to make it continue at locked_return instead of
	 * here.
	 */
	lis	6, locked_return@ha
	addi	6, 6, locked_return@l
	stw	6, (_REG_PC + 8)(1)

	STACK_SWITCH(4, 7)

	/* Check if the switcher was preempted and continued to here. */
	lwz	8, PT_NEXT(3)
	or.	8, 8, 8
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
	stw	4, PT_SWITCHTO(3)
	stw	7, PT_SWITCHTOUC(3)
	stw	5, PT_HELDLOCK(3)
	lwz	6, PT_SPINLOCKS(3)
	addi	6, 6, -1
	stw	6, PT_SPINLOCKS(3)

	/*
	 * Save the context we previously stored in PT_UC(3);
	 * that was overwritten when we were preempted and continued,
	 * so we need to put it somewhere.
	 */
	stw	1, PT_SLEEPUC(3)
	addi	1, 1, -STACKSPACE	/* Convert to stack */

	mr	3, 4
	mr	4, 8
	li	5, 1
	b	pthread__switch_away
	NOTREACHED

	/* No locked old-preemption */
1:	/*
	 * We've moved to the new stack, and the old context has been
	 * saved.  The queue lock can be released.
	 */
	/* Reduce the lock count... */
	lwz	6, PT_SPINLOCKS(3)
	addi	6, 6, -1
	stw	6, PT_SPINLOCKS(3)
	/* ... actually release the lock ... */
	sync
	xor	9,9,9
	stw	9, 0(5)
	/* ... and remove the fake lock */
	lwz	6, PT_SPINLOCKS(4)
	addi	6, 6, -1
	stw	6, PT_SPINLOCKS(4)

	/* Check to see if we were preempted while holding the fake lock */
	lwz	8, PT_NEXT(4)
	or.	8, 8, 8
	beq	2f

	/* Yes, we were.  Go to the next element in the chain. */
	stw	4, PT_SWITCHTO(4)
	stw	7, PT_SWITCHTOUC(4)
	mr	3, 4
	mr	4, 8
	li	5, 0
	b	pthread__switch_away
	NOTREACHED

2:	mr	3, 7
	b	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED

locked_return:
	lwz	31, (RND_CTXSIZE + 20)(1)
	addi	1, 1, (RND_CTXSIZE + 32)
	lwz	0, 4(1)
	mtlr	0
	blr
