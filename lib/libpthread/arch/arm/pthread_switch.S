/*	$NetBSD: pthread_switch.S,v 1.7 2003/09/07 14:47:48 cl Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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

#include <machine/asm.h>
#include "assym.h"

#define	NOTREACHED	/* nothing, for now */

/*
 * Define:
 *	void pthread__switch(pthread_t self, pthread_t next)
 *	void pthread__upcall_switch(pthread_t self, pthread_t next)
 *	void pthread__locked_switch(pthread_t self, pthread_t next,
 *	    pt_spin_t *lock)
 */

/*		*** WARNING ***
 * STACK_SWITCH is more subtle than it looks. Please go read the extended
 * comment in the i386 pthread_switch.S file.
 */

#define	STACK_SWITCH(pt, tmp, z)					\
	ldr	tmp, [pt, #(PT_TRAPUC)]				;	\
	teq	tmp, #0x00000000				;	\
	bne	1f						;	\
	ldr	tmp, [pt, #(PT_UC)]				;	\
1:	sub	sp, tmp, #(STACKSPACE)				;	\
	mov	z, #00000000					;	\
	str	z, [pt, #(PT_TRAPUC)]

/*    
 * void
 * pthread__switch(pthread_t self (r0), pthread_t next (r1))
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
	sub	sp, sp, #(RND_CTXSIZE)	/* push space for ucontext_t */

	mov	r3, sp			/* r3 = &context */

	/* Save r0-r2, r4-r5, lr */
	stmfd	sp!, {r0-r2, r4-r5, lr}

	/* Now r5 will point to &context for the duration. */
	mov	r5, r3

/*
 * Stack is:
 *		context			(RND_CTXSIZE bytes)
 *		saved lr		(          4 bytes)
 *		saved r5		(          4 bytes)
 *		saved r4		(          4 bytes)
 *		saved r2		(          4 bytes)
 *		saved r1		(          4 bytes)
 *		saved r0		(          4 bytes)
 *		Note, no space needed
 *		   for callees.
 */

	/* Get the current context. */
	mov	r0, r5			/* r0 = &context */
	bl	PIC_SYM(_C_LABEL(_getcontext_u), PLT)

	/* Edit the context to make it continue below, rather than here */
	add	r0, pc, #pthread__switch_return_point - . - 8
	str	r0, [r5, #(_REG_PC)]

	/* Get back r0-r2 */
	ldmfd	sp, {r0-r2}

	str	r5, [r0, #(PT_UC)]	/* self->pt_uc = &context */

	/* Side-effect: r4 = next->pt_uc */
	STACK_SWITCH(r1, r4, r3)

	/* NOTE: we can no longer recover registers stored on the stack */

	mov	r0, r4			/* r0 = next->pt_uc */
	b	PIC_SYM(_C_LABEL(_setcontext_u), PLT)
	NOTREACHED

ENTRY_NP(pthread__switch_return_point)
/*
 * Stack is:
 *		context			(RND_CTXSIZE bytes)
 *		saved lr		(          4 bytes)
 *		saved r5		(          4 bytes)
 *		saved r4		(          4 bytes)
 *		saved r2		(          4 bytes)
 *		saved r1		(          4 bytes)
 *		saved r0		(          4 bytes)
 *		Note, no space needed
 *		   for callees.
 */

	add	sp, sp, #12		/* skip over r0-r2 */
	ldmfd	sp!, {r4-r5, lr}	/* restore r4-r5, lr */
	add	sp, sp, #(RND_CTXSIZE)	/* pop ucontext_t */
	mov	pc, lr

/*
 * Helper routine; r0 = from, r1 = to, r2 = decr-spinlocks (boolean)
 */
pthread__switch_away:
	/* Side-effect: r3 = r1->pt_uc */
	STACK_SWITCH(r1, r3, r14)

	/*
	 * If we're invoked from the switch-to-next provisions of
	 * pthread__locked_switch() or pthread__upcall_switch(),
	 * there may be a fake spinlock-count set.  If so, they will
	 * set r2 to let us know, and we decrement it now that we're
	 * no longer using the old stack.
	 */
	teq	r2, #0x00000000
	beq	1f
	ldr	r2, [r0, #(PT_SPINLOCKS)]
	sub	r2, r2, #1
	str	r2, [r0, #(PT_SPINLOCKS)]

1:	mov	r0, r3
	b	PIC_SYM(_C_LABEL(_setcontext_u), PLT)
	NOTREACHED

/*
 * void 
 * pthread__upcall_switch(pthread_t self (r0), pthread_t next (r1))
 *
 * Quit an upcall, recycle it, and jump to the selected thread.
 *
 * Note: This code never returns, so we can use the callee-saved
 * registers to our heart's content (and they will be preserved
 * by functions that we call).
 */     
ENTRY(pthread__upcall_switch)
	/* Create a "fake" lock count so that this code will be continued */
	ldr	r3, [r1, #(PT_SPINLOCKS)]
	add	r3, r3, #1
	str	r3, [r1, #(PT_SPINLOCKS)]

	/* Side-effect: r7 = next->pt_uc */
	STACK_SWITCH(r1, r7, r2)

	/* Check if the upcall code was preempted and continued to here. */
	ldr	r3, [r0, #(PT_NEXT)]
	teq	r3, #0x00000000
	beq	1f

	/*
	 * Yes, it was.  Stash the tread we were going to switch to,
	 * and switch to the next thread in the continuation chain.
	 */
	str	r7, [r0, #(PT_SWITCHTOUC)]
	str	r1, [r0, #(PT_SWITCHTO)]
	mov	r0, r1		/* from = next */
	mov	r1, r3		/* to = self->pt_next */
	mov	r2, #0x00000001	/* decr-spinlocks = TRUE */
	b	pthread__switch_away
	NOTREACHED

1:	/* No old-upcall-preemption */
	mov	r4, r0		/* stash from */
	mov	r5, r1		/* stash to */
	bl	PIC_SYM(_C_LABEL(pthread__sa_recycle), PLT)
	mov	r0, r4		/* restore from */
	mov	r1, r5		/* restore to */

	/* We can now release the fake lock. */
	ldr	r3, [r1, #(PT_SPINLOCKS)]
	sub	r3, r3, #1
	str	r3, [r1, #(PT_SPINLOCKS)]

	/* Check if we were preempted and continued while faking the lock */
	ldr	r3, [r1, #(PT_NEXT)]
	teq	r3, #0x00000000
	beq	2f

	/*
	 * Yes, we were.  Stash the thread we were going to switch to
	 * in itself, and switch to the next thread in the continuation
	 * chain.
	 */
	str	r7, [r1, #(PT_SWITCHTOUC)]
	str	r1, [r1, #(PT_SWITCHTO)]
	mov	r0, r1		/* from = next */
	mov	r1, r3		/* to = next->pt_next */
	mov	r2, #0x00000000	/* decr-spinlocks = FALSE */
	b	pthread__switch_away
	NOTREACHED

2:	/* No new-upcall-preemption */
	mov	r0, r7		/* r0 = next->pt_uc */
	b	PIC_SYM(_C_LABEL(_setcontext_u), PLT)

/*
 * void
 * pthread__locked_switch(pthread_t self (r0), pthread_t next (r1),
 *          pt_spin_t *lock (r2))
 *
 * Total stack is:
 *		context			(RND_CTXSIZE bytes)
 *		saved lr		(          4 bytes)
 *		saved r5		(          4 bytes)
 *		saved r4		(          4 bytes)
 *		saved r2		(          4 bytes)
 *		saved r1		(          4 bytes)
 *		saved r0		(          4 bytes)
 *		Note, no space needed
 *		   for callees.
 *
 *     STACKSPACE is the space between the bottom of the stack and
 * the ucontext on the stack.  i.e., (6 * sizeof(reg)).
 *
 * Note: We use r4 as a temporary register, and r5 as a pointer to
 * the context on the stack.
 */     
ENTRY(pthread__locked_switch)
	sub	sp, sp, #(RND_CTXSIZE)	/* push space for ucontext_t */

	/* Make sure we get continued */
	ldr	r3, [r1, #(PT_SPINLOCKS)]
	add	r3, r3, #1
	str	r3, [r1, #(PT_SPINLOCKS)]

	mov	r3, sp			/* r3 = &context */

	/* Save r0-r2, r4-r5, lr */
	stmfd	sp!, {r0-r2, r4-r5, lr}

	/* Now r5 will point to &context for the duration. */
	mov	r5, r3

/*
 * Stack is:
 *		context			(RND_CTXSIZE bytes)
 *		saved lr		(          4 bytes)
 *		saved r5		(          4 bytes)
 *		saved r4		(          4 bytes)
 *		saved r2		(          4 bytes)
 *		saved r1		(          4 bytes)
 *		saved r0		(          4 bytes)
 *		Note, no space needed
 *		   for callees.
 */

	/* Get the current context. */
	mov	r0, r5			/* r0 = &context */
	bl	PIC_SYM(_C_LABEL(_getcontext_u), PLT)

	/* Edit the context to make it continue below, rather than here */
	add	r0, pc, #locked_return_point - . - 8
	str	r0, [r5, #(_REG_PC)]

	/* Get back r0-r2 */
	ldmfd	sp, {r0-r2}

	str	r5, [r0, #(PT_UC)]	/* self->pt_uc = &context */

	/* Side-effect: r4 = next->pt_uc */
	STACK_SWITCH(r1, r4, r3)

	/* NOTE: we can no longer recover registers stored on the stack */

	/* Check if the switcher was preempted and continued to here. */
	ldr	r3, [r0, #(PT_NEXT)]
	teq	r3, #0x00000000
	beq	1f

	/*
	 * Yes, it was.  Stash the thread we were going to switch to,
	 * the lock the original thread was holding, and switch to
	 * the next thread in the continuation chain.  Mark the fact
	 * that this was a locked switch, and so the thread does not
	 * need to be put on a run queue.
	 *
	 * DO NOT RELEASE THE LOCK.  It's possible that if we do so,
	 * PT_SWITCHTO will be stomped on by another switch_lock and
	 * preemption.
	 */
	str	r2, [r0, #(PT_HELDLOCK)]
	str	r4, [r0, #(PT_SWITCHTOUC)]
	str	r1, [r0, #(PT_SWITCHTO)]

	ldr	r4, [r0, #(PT_SPINLOCKS)]
	sub	r4, r4, #1
	str	r4, [r0, #(PT_SPINLOCKS)]

	mov	r0, r1			/* from = next */
	mov	r1, r3			/* to = self->pt_next */
	mov	r2, #0x00000001		/* decr-spinlocks = TRUE */
	b	pthread__switch_away
	NOTREACHED

1:	/* No locked-old-preemption */

	/*
	 * Note: r4 == next->pt_uc still.  We no longer need to
	 * reference the context on the stack, so put next->pt_uc
	 * in r5 so that we can use r4 as a temporary.
	 */
	mov	r5, r4

	/*
	 * We've moved to the new stack, and the old context has
	 * been saved.  The queue lock can be released.
	 */
	ldr	r4, [r0, #(PT_SPINLOCKS)]
	sub	r4, r4, #1
	str	r4, [r0, #(PT_SPINLOCKS)]

	/* ...actually release the lock... */
	mov	r4, #(__SIMPLELOCK_UNLOCKED)
	str	r4, [r2]

	/* ...and remove the fake lock... */
	ldr	r4, [r1, #(PT_SPINLOCKS)]
	sub	r4, r4, #1
	str	r4, [r1, #(PT_SPINLOCKS)]

	/* Check if we were preempted while holding the fake lock. */
	ldr	r3, [r1, #(PT_NEXT)]
	teq	r3, #0x00000000
	beq	2f

	/* Yes, we were.  Go to the next thread in the chain. */
	str	r5, [r1, #(PT_SWITCHTOUC)]
	str	r1, [r1, #(PT_SWITCHTO)]
	mov	r0, r1			/* from = next */
	mov	r1, r3			/* to = next->pt_next */
	mov	r2, #0x00000000		/* decr-spinlocks = FALSE */
	b	pthread__switch_away

2:	/* No locked-new-preemption */
	mov	r0, r5			/* r0 = next->pt_uc */
	b	PIC_SYM(_C_LABEL(_setcontext_u), PLT)
	NOTREACHED

locked_return_point:
/*
 * Stack is:
 *		context			(RND_CTXSIZE bytes)
 *		saved lr		(          4 bytes)
 *		saved r5		(          4 bytes)
 *		saved r4		(          4 bytes)
 *		saved r2		(          4 bytes)
 *		saved r1		(          4 bytes)
 *		saved r0		(          4 bytes)
 *		Note, no space needed
 *		   for callees.
 */

	add	sp, sp, #12		/* skip over r0-r2 */
	ldmfd	sp!, {r4-r5, lr}	/* restore r4-r5, lr */
	add	sp, sp, #(RND_CTXSIZE)	/* pop ucontext_t */
	mov	pc, lr
