/*	$NetBSD: pthread_switch.S,v 1.5 2003/09/07 14:47:49 cl Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams and Steve C. Woodford.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Based on the i386 version.
 */

#include <machine/asm.h>
#include "assym.h"

/*
 * This file implements low-level routines that are exported to
 * the machine-independent parts of the thread library. The routines are:
 * 
 * void	pthread__switch(pthread_t self, pthread_t next);
 * void	pthread__upcall_switch(pthread_t self, pthread_t next);
 * void	pthread__locked_switch(pthread_t self, pthread_t next, 
 *           pt_spin_t *lock);
 *	   
 * as well as some utility code used by these routines. 
 */

/* Force an error when "notreached" code is reached. */
#define	NOTREACHED	\
	illegal

/*
 * Utility macro to switch to the stack of another thread.
 * WARNING: This is more subtle than it looks.
 *
 * There are a couple of motivations for stack switching. One is that
 * when exiting an upcall and running a thread, we want to recycle the
 * state of the upcall before continuing the thread. However, the
 * stack is part of the upcall state, and we can't be on the upcall
 * stack when we want to recycle it. Therefore, we pre-switch to the
 * stack of the thread we're about to switch to and borrow some space
 * on its stack so that we can recycle the upcall.
 *
 * Another is that when performing the slightly sensitive operation of
 * putting the currently running thread on a (locked) sleep queue then
 * switching to another thread, we don't want to release the queue
 * lock until we are no longer on the original stack - if we released
 * it any earlier, it's possible (epecially on a multiprocessor
 * system) that the original thread could get taken off the queue and
 * restarted while we're still using its stack, which would be bad
 * ("Important saftey tip. Thanks, Egon").
 *
 * The subtlety comes from the contents of the stack of the thread
 * being switched to. Our convention is that the ucontext_t of the
 * thread is stored near the top of the stack. When the kernel
 * preempts a thread, it stores the ucontext_t just above the current
 * top-of-stack and passes the upcall handler the pointer to the
 * ucontext_t, and the upcall handler stores it in the the pt_uc field
 * of the thread structure. When user-level code voluntairly switches
 * from one thread to another, the ucontext_t is just below the top of
 * the stack, and we impose a limit on the amount of stack space above
 * the ucontext_t that may be used. This way, we can perform the stack
 * switch safely by setting the stack pointer to thread->pt_uc -
 * STACKSPACE.
 * Note carefully that we do the subtraction *before* putting the
 * value in the stack pointer. Since preemption can occur at any time,
 * and the kernel will happily write the new ucontext_t below the
 * current stack pointer, it is unsafe for us to ever set the stack
 * pointer above potentially valid data, even for one instruction.
 *
 * Got it? Good. Now go read it again.
 * 
 */

#define STACK_SWITCH(pt,tmp)			\
	movl	%pt@(PT_TRAPUC),%tmp		; \
	bne	1f				; \
	movl	%pt@(PT_UC),%tmp		; \
1:	lea	%tmp@(-STACKSPACE),%sp		; \
	clr	%pt@(PT_TRAPUC)
		
/*
 * void	pthread__switch(pthread_t self, pthread_t next);
 *
 * Plain switch that doesn't do any special checking or handle
 * spin-preemption. It isn't used much by normal code, actually; it's
 * main purpose is to be a basic switch engine when the MI code is
 * already dealing with spin-preemption or other gunk.  
 */

ENTRY(pthread__switch)
	lea	%sp@(-RND_CTXSIZE),%sp	/* Allocate space for the ucontext_t */

	/*
	 * Save `self's context
	 */
	movl	%sp,%sp@-
	jbsr	PIC_PLT(_C_LABEL(_getcontext_u))
	addql	#4,%sp

	/*
	 * Edit the context so that it continues as if returning from
	 * the _setcontext_u below.  
	 */
	lea	%pc@(Lswitch_return_point),%a0
	movl	%a0,%sp@(UC_PC)

	movl	%sp@(RND_CTXSIZE+4),%a0	/* `self': Thread to switch from */
	movl	%sp@(RND_CTXSIZE+8),%a1	/* `next': Thread to switch to */
	movl	%sp,%a2			/* a3 = &context */
	movl	%a2,%a0@(PT_UC)		/* self->pt_uc = &context */

	STACK_SWITCH(a1,a3)		/* side effect: a3 = next->pt_uc */


	/*
	 * Restore `next's context
	 */
	pea	%a3@
	jbsr	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED

ENTRY_NOPROFILE(pthread__switch_return_point)
Lswitch_return_point:
	/* We're back on the original stack. */
	lea	%sp@(RND_CTXSIZE+4),%sp	/* Ditch the ucontext_t */
	rts


/*
 * Helper switch code used by pthread__locked_switch() and 
 * pthread__upcall_switch() when they discover spinlock preemption.
 *
 * Call with:
 *   a1 = from, a2 = to
 */
Lpthread__switch_away_decrement:
	STACK_SWITCH(a2,a0)		/* side effect: a0 = to->pt_uc */

	/* If we're invoked from the switch-to-next provisions of
	 * pthread__locked_switch or pthread__upcall_switch, there
	 * may be a fake spinlock-count set. If so, we decrement it
	 * once we're no longer using the old stack.
	 */ 
	subql	#1, %a1@(PT_SPINLOCKS)
	pea	%a0@
	jbsr	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED

Lpthread__switch_away_no_decrement:
	STACK_SWITCH(a2,a0)		/* side effect: a0 = to->pt_uc */
	pea	%a0@
	jbsr	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED


/*
 * void	pthread__locked_switch(pthread_t self, pthread_t next, 
 *           pt_spin_t *lock);
 *
 * Switch away from a thread that is holding a lock on a queue (to
 * prevent being removed from the queue before being switched away).
 */
ENTRY(pthread__locked_switch)
	lea	%sp@(-RND_CTXSIZE),%sp	/* Space for ucontext_t */
	movl	%sp@(RND_CTXSIZE+8),%a0	/* `next': Thread to switch to */
	addql	#1,%a0@(PT_SPINLOCKS)	/* Make sure we get continued */
	movl	%sp@(RND_CTXSIZE+4),%a0	/* `self': current thread */
	movl	%sp,%a0@(PT_UC)		/* self->pt_uc = &context */

	/*
	 * Save `self's context
	 */
	movl	%sp,%sp@-
	jbsr	PIC_PLT(_C_LABEL(_getcontext_u))
	addql	#4,%sp

	/*
	 * Edit the context so that it continues as if returning from
	 * the _setcontext_u below.  
	 */
	lea	%pc@(Llocked_return_point),%a0
	movl	%a0,%sp@(UC_PC)

	/*
	 * Fetch function parameters now that callee-saved registers
	 * are available. (They're saved in the context)
	 */
	movl	%sp@(RND_CTXSIZE+4),%a2		/* a2 = self */
	movl	%sp@(RND_CTXSIZE+8),%a1		/* a1 = next */
	movl	%sp@(RND_CTXSIZE+12),%a0	/* a0 = lock */
	movl	%sp,%a4				/* a4 = &context */

	STACK_SWITCH(a1,a3)		/* side effect: a3 = next->pt_uc */

	/* Now running on `next's stack. */

	/* Check if the original thread was preempted while holding
	 * its queue lock.
	 */
	movl	%a2@(PT_NEXT),%d0		/* self->pt_next == 0? */
	beqs	Llocked_no_old_preempt		/* Yup */

	/* Yes, it was. Stash the thread we were going to
	 * switch to, the lock the original thread was holding, 
	 * and go to the next thread in the chain.  
	 * Mark the fact that this was a locked switch, and so the
	 * thread does not need to be put on a run queue.
	 * Don't release the lock. It's possible that if we do so,
	 * PT_SWITCHTO will be stomped by another switch_lock and
	 * preemption.
         */
	movl	%a0,%a2@(PT_HELDLOCK)
	movl	%a3,%a2@(PT_SWITCHTOUC)
	movl	%a1,%a2@(PT_SWITCHTO)
	subql	#1,%a2@(PT_SPINLOCKS)

	/* a1 already == from */
	movl	%d0,%a2			/* to = self->pt_next */
	jbra	Lpthread__switch_away_decrement
	/* NOTREACHED */

Llocked_no_old_preempt:
	/* a0 = lock, a1 = next, a2 = self, a3 = next->pt_uc */

	/*
	 * We've moved to the new stack, and the old context has been 
	 * saved. The queue lock can be released.
	 */
	subql	#1,%a2@(PT_SPINLOCKS)	/* self->pt_spinlocks-- */

	/* We happen to know that this is the right way to release a lock. */
	clrb	%a0@			/* *lock = 0 */

	/* Remove the fake lock */
	subql	#1,%a1@(PT_SPINLOCKS)	/* next->pt_spinlocks-- */

	/* Check if we were preempted while holding the fake lock. */
	movl	%a1@(PT_NEXT),%d0	/* (%d0 = next->pt_next) == NULL? */
	beqs	Llocked_no_new_preempt	/* Yup */

	/* Yes, we were. Bummer. Go to the next element in the chain. */
	movl	%a3,%a1@(PT_SWITCHTOUC)
	movl	%a1,%a1@(PT_SWITCHTO)	/* next->pt_switchto = next */
	movl	%d0,%a2
	jbra	Lpthread__switch_away_no_decrement

Llocked_no_new_preempt:		
	pea	%a3@			/* _setcontext_u(next->pt_uc) */
	jbsr	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED

Llocked_return_point:
	/* We're back on the original stack. */
	lea	%sp@(RND_CTXSIZE+4),%sp	/* Ditch the ucontext_t */
	rts



/*
 * void	pthread__upcall_switch(pthread_t self, pthread_t next);
 *
 * Quit an upcall, recycle it, and jump to the selected thread.
 * Since this code never returns, we are free to use callee-saved
 * registers.
 */
ENTRY(pthread__upcall_switch)
	/* Save args into registers so we can stop using the old stack. */
	movl	%sp@(4),%a2		/* a2 = self (the upcall thread) */
	movl	%sp@(8),%a3		/* a3 = next */

	/* Create a `fake' lock count so we are `continued' */
	addql	#1,%a3@(PT_SPINLOCKS)

	STACK_SWITCH(a3,a4)		/* side effect: a4 = next->pt_uc */

	/* Check if the upcall was preempted and continued. */
	movl	%a2@(PT_NEXT),%d0
	beqs	Lupcall_no_old_preempt

	/* Yes, it was. Stash the thread we were going to
	 * switch to, and go to the next thread in the chain.
	 */
	movl	%a4,%a2@(PT_SWITCHTOUC)
	movl	%a3,%a2@(PT_SWITCHTO)
	movl	%a3,%a1			/* from(a1) = next */
	movl	%d0,%a2			/* to(a2)   = self->pt_next */
	jbra	Lpthread__switch_away_decrement
	/* NOTREACHED */

Lupcall_no_old_preempt:		
	/* pthread__sa_recycle(old, new) */
	pea     %a3@		/* new = next(a3) */
	pea     %a2@		/* old = self(a2) */
	jbsr    PIC_PLT(_C_LABEL(pthread__sa_recycle))
	addql	#8,%sp

	/* Release the fake lock */
	subql	#1,%a3@(PT_SPINLOCKS)

	/* Check if we were preempted while holding the fake lock. */
	movl	%a3@(PT_NEXT),%d0
	beqs	Lupcall_no_new_preempt

	/* Yes, we were. Bummer. Go to the next element in the chain. */
	movl	%a4,%a3@(PT_SWITCHTOUC)
	movl	%a3,%a3@(PT_SWITCHTO)
	movl	%d0,%a2			/* to(a2)   = self->pt_next */
	jbra	Lpthread__switch_away_no_decrement
	/* NOTREACHED */

Lupcall_no_new_preempt:		
	pea	%a4@
	jbsr	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED

