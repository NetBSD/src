/*	$NetBSD: pthread_switch.S,v 1.1.2.1 2001/11/18 23:35:20 scw Exp $	*/

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
 * void	pthread__switch(pthread_t self, pthread_t next);
 *
 * Plain switch that doesn't do any special checking or handle
 * spin-preemption. It isn't used much by normal code, actually; it's
 * main purpose is to be a basic switch engine when the MI code is
 * already dealing with spin-preemption or other gunk.  
 */

ENTRY(pthread__switch)
	link	%a6,#-CONTEXTSIZE	/* Allocate space for the ucontext_t */
	movl	%a6@(8),%a0		/* a0 holds the current thread */
	movl	%a6@(12),%a1		/* a1 holds the current to switch to */
	movl	%sp,%a0@(PT_UC)
	movl	%a1@(PT_UC),%sp@-
	movl	%a0@(PT_UC),%sp@-
	jbsr	PIC_PLT(_C_LABEL(_swapcontext_u))
	addq	#8,%sp			/* Not strictly necessary, because */
	unlk	%a6			/* this `unlk' also resets sp */
	rts

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
	movl	%pt@(PT_UC), %tmp	;	\
	lea	%tmp@(-STACKSPACE), %sp


/*
 * Helper switch code used by pthread__locked_switch() and 
 * pthread__upcall_switch() when they discover spinlock preemption.
 */
Lpthread__switch_away:
	STACK_SWITCH(a3,a0)

	/* If we're invoked from the switch-to-next provisions of
	 * pthread__locked_switch or pthread__upcall_switch, there
	 * may be a fake spinlock-count set. If so, they will set %d0
	 * to let us know, and we decrement it once we're no longer using
	 * the old stack.
	 */ 
	tstl	%d0
	bles	Lpthread__switch_no_decrement
	subql	#1, %a2@(PT_SPINLOCKS)

Lpthread__switch_no_decrement:	
	pea	%a0@			/* ucontext_t *ucp */
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
	link	%a6,#-CONTEXTSIZE
	moveml	%d2/%a2-%a4,%sp@-
	movl	%a6@(8),%a2		/* a2 holds the current thread */
	movl	%a6@(12),%a3		/* a3 holds the thread to switch to */
	movl	%a6@(16),%a4		/* a4 holds the spinlock pointer */
	addql	#1,%a3@(PT_SPINLOCKS)	/* Make sure we get continued */
	lea	%a6@(-CONTEXTSIZE),%a0
	movl	%a0,%a2@(PT_UC)
	movl	%a0,%d2
	pea	%a0@
	jbsr	PIC_PLT(_C_LABEL(_getcontext_u))
	addql	#4,%sp

	/*
	 * Edit the context so that it continues as if returning from
	 * the _setcontext_u below.  
	 */
	lea	%pc@(Llocked_return_point),%a0
	movl	%a0,%a6@(-CONTEXTSIZE+UC_PC)

	STACK_SWITCH(a3,a1)
	
	/* Check if the original thread was preempted while holding
	 * its queue lock.
	 */
	tstl	%a2@(PT_NEXT)
	beqs	Llocked_no_old_preempt

	/* Yes, it was. Stash the thread we were going to
	 * switch to, the lock the original thread was holding, 
	 * and go to the next thread in the chain.  
	 * Mark the fact that this was a locked switch, and so the
	 * thread does not need to be put on a run queue.
	 * Don't release the lock. It's possible that if we do so,
	 * PT_SWITCHTO will be stomped by another switch_lock and
	 * preemption.
         */
	movl	%a3, %a2@(PT_SWITCHTO)
	movl	%a0, %a2@(PT_SWITCHTOUC)
	movl	%a4, %a2@(PT_HELDLOCK)
	subql	#1,%a2@(PT_SPINLOCKS)

	/* Save the context we previously stored in %a2@(PT_UC)
	 * that was overwritten when we were preempted and continued,
	 * so we need to put it somewhere. 
	 */
	movl	%d2,%a2@(PT_SLEEPUC)

	movl	%a2@(PT_NEXT),%d0
	movl	%a3,%a2
	movl	%d0,%a3
	moveql	#1,%d0
	bra	Lpthread__switch_away

Llocked_no_old_preempt:		
	/* We've moved to the new stack, and the old context has been 
	 * saved. The queue lock can be released. */
	subql	#1,%a2@(PT_SPINLOCKS)
	/* We happen to know that this is the right way to release a lock. */
	clrl	%a4@

	subql	#1,%a3@(PT_SPINLOCKS)
	/* Check if we were preempted while holding the fake lock. */
	tstl	%a3@(PT_NEXT)
	beqs	Llocked_no_new_preempt
	/* Yes, we were. Bummer. Go to the next element in the chain. */
	movl	%a3,%a3@(PT_SWITCHTO)
	movl	%d2,%a3@(PT_SWITCHTOUC)
	movl	%a3,%a2
	movl	%a3@(PT_NEXT),%a3
	moveql	#-2,%d0
	bra	Lpthread__switch_away

Llocked_no_new_preempt:		
	movl	%d2,%sp@-			/* ucontext_t *ucp */
	jbsr	PIC_PLT(_C_LABEL(_setcontext_u))
Llocked_return_point:
	/* We're back on the original stack. */
	moveml	%sp@+,%d2/%a2-%a4
	unlk	%a6
	rts



/*
 * void	pthread__upcall_switch(pthread_t self, pthread_t next);
 *
 * Quit an upcall, recycle it, and jump to the selected thread.
 */
ENTRY(pthread__upcall_switch)
	link	%a6,#0
	/* Save args into registers so we can stop using the old stack. */
	/* No need to save callee saved registers, we never return */
	movl	%a6@(8),%a2		/* a2 holds the upcall thread */
	movl	%a6@(12),%a3		/* a3 holds the thread to switch to */
	addql	#1,%a3@(PT_SPINLOCKS)

	STACK_SWITCH(a3,a0)
	
	/* Check if the upcall was preempted and continued. */
	tstl	%a2@(PT_NEXT)
	beqs	Lupcall_no_old_preempt
	/* Yes, it was. Stash the thread we were going to
	 * switch to, and go to the next thread in the chain.
	 */
	movl	%a3,%a2@(PT_SWITCHTO)
	movl	%a0,%a2@(PT_SWITCHTOUC)
	movl	#PT_STATE_RECYCLABLE,%a2@(PT_STATE)
	movl	%a2@(PT_NEXT),%d0
	movl	%a3,%a2
	movl	%d0,%a3
	moveql	#1,%d0
	bra	Lpthread__switch_away
	NOTREACHED

Lupcall_no_old_preempt:		
	pea	%a0@
	pea     %a3@		/* pthread_t new */
	pea     %a2@		/* pthread_t old */
	jbsr    PIC_PLT(_C_LABEL(pthread__sa_recycle))
	addql	#8,%sp
	subql	#1,%a3@(PT_SPINLOCKS)
	/* Check if we were preempted while holding the fake lock. */
	tstl	%a3@(PT_NEXT)
	beqs	Lupcall_no_new_preempt
	/* Yes, we were. Bummer. Go to the next element in the chain. */
	movl	%a3,%a3@(PT_SWITCHTO)
	movl	%sp@+,%a3@(PT_SWITCHTOUC)
	movl	%a3,%a2
	movl	%a3@(PT_NEXT),%a3
	moveql	#-1,%d0
	bra	pthread__switch_away
	NOTREACHED

Lupcall_no_new_preempt:		
	jbsr	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED
