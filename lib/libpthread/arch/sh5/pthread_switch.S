/*	$NetBSD: pthread_switch.S,v 1.3 2003/01/22 13:52:48 scw Exp $	*/

/*-
 * Copyright (c) 2001, 2003 The NetBSD Foundation, Inc.
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

#include <machine/asm.h>
#include "assym.h"

/*
 * This file implements low-level routines that are exported to
 * the machine-independent parts of the thread library. The routines are:
 * 
 * void	pthread__switch(pthread_t self, pthread_t next);
 * void	pthread__upcall_switch(pthread_t self, pthread_t next);
 * void	pthread__locked_switch(pthread_t self, pthread_t next, pt_spin_t *lock);
 *	   
 * as well as some utility code used by these routines. 
 */

/*
 * Utility macro to switch to the stack of another thread.
 */
#define STACK_SWITCH(pt,tmp)		\
	LDPTR	pt, PT_UC, tmp		;\
	addi	tmp, -STACKSPACE, r15


/*
 * void	pthread__switch(pthread_t self, pthread_t next);
 *
 * Plain switch that doesn't do any special checking or handle
 * spin-preemption. It isn't used much by normal code, actually; it's
 * main purpose is to be a basic switch engine when the MI code is
 * already dealing with spin-preemption or other gunk.  
 */
ENTRY(pthread__switch)
	pta/l	_C_LABEL(_swapcontext_u), tr0
	movi	RND_CTXSIZE+8, r0
	sub	r15, r0, r15
	st.q	r15, RND_CTXSIZE, r18
	STPTR	r2, PT_UC, r15
	LDPTR	r3, PT_UC, r3
	or	r15, r63, r2
	blink	tr0, r18
	ld.q	r15, RND_CTXSIZE, r18
	ptabs/l	r18, tr0
	movi	RND_CTXSIZE+8, r0
	add	r15, r0, r15
	blink	tr0, r63


/*
 * void	pthread__locked_switch(pthread_t self, pthread_t next, pt_spin_t *lock);
 *
 * Switch away from a thread that is holding a lock on a queue (to
 * prevent being removed from the queue before being switched away).
 *
 * r2 = self, r3 = next, r4 = lock
 */
ENTRY(pthread__locked_switch)
	movi	RND_CTXSIZE+32, r0
	sub	r15, r0, r15
	st.q	r15, RND_CTXSIZE, r28
	st.q	r15, RND_CTXSIZE+8, r29
	st.q	r15, RND_CTXSIZE+16, r30
	st.q	r15, RND_CTXSIZE+24, r18

	/* Make sure we get continued */
	ld.l	r3, PT_SPINLOCKS, r0
	addi	r0, 1, r0
	st.l	r3, PT_SPINLOCKS, r0

	/*
	 * Keep r2-r4, and r15 safe for later.
	 */
	pta/l	_C_LABEL(_getcontext_u), tr0
	or	r2, r63, r28		/* self */
	or	r3, r63, r29		/* next */
	or	r4, r63, r30		/* lock */
	STPTR	r2, PT_UC, r15		/* self->pt_uc = ctx */

	/* Save the current context */
	or	r15, r63, r2
	blink	tr0, r18

	/*
	 * Edit the context so that it continues as if returning from
	 * the _setcontext_u below.  
	 */
	pta/u	Llocked_return_point, tr0
	gettr	tr0, r0
	st.q	r15, UC_REGS_PC, r0
	or	r15, r63 ,r3

	pta/l	Llocked_no_old_preempt, tr0
	STACK_SWITCH(r29, r2)		/* r2 = next->pt_uc */

	/* Now running on `next's stack. */

	/* Check if the original thread was preempted while holding
	 * its queue lock.
	 */
	LDPTR	r28, PT_NEXT, r1		/* self->pt_next == NULL? */
	beq/l	r1, r63, tr0			/* Jump if not preempted */

	/* Yes, it was. Stash the thread we were going to
	 * switch to, the lock the original thread was holding, 
	 * and go to the next thread in the chain.  
	 * Mark the fact that this was a locked switch, and so the
	 * thread does not need to be put on a run queue.
	 * Don't release the lock. It's possible that if we do so,
	 * PT_SWITCHTO will be stomped by another switch_lock and
	 * preemption.
         */
	STPTR	r28, PT_SWITCHTO, r29		/* self->pt_switchto = next */
	STPTR	r28, PT_SWITCHTOUC, r2		/* self->pt_switchtouc = uc */
	STPTR	r28, PT_HELDLOCK, r30		/* self->pt_heldlock = lock */
	ld.l	r28, PT_SPINLOCKS, r0
	addi	r0, -1, r0
	st.l	r28, PT_SPINLOCKS, r0

	/* Save the context we previously stored in self->pt_uc
	 * that was overwritten when we were preempted and continued,
	 * so we need to put it somewhere. 
	 */
	STPTR	r28, PT_SLEEPUC, r3

	pta/l	_C_LABEL(_setcontext_u), tr0
	STACK_SWITCH(r1, r2)

	/* If we're invoked from the switch-to-next provisions of
	 * pthread__locked_switch or pthread__upcall_switch, there
	 * may be a fake spinlock-count set. If so, we decrement it
	 * once we're no longer using the old stack.
	 */ 
	ld.l	r29, PT_SPINLOCKS, r0
	addi	r0, -1, r0
	st.l	r29, PT_SPINLOCKS, r0
	blink	tr0, r63
	/* NOTREACHED */

Llocked_no_old_preempt:
	/* r30 = lock, r29 = next, r28 = self, r2 = next->pt_uc */

	/* We've moved to the new stack, and the old context has been 
	 * saved. The queue lock can be released.
	 */
	ld.l	r28, PT_SPINLOCKS, r0	/* self->pt_spinlocks-- */
	addi	r0, -1, r0
	st.l	r28, PT_SPINLOCKS, r0

	/* We happen to know that this is the right way to release a lock. */
	st.q	r30, 0, r63		/* *lock = 0 */
	pta/l	_C_LABEL(_setcontext_u), tr0

	/* Remove the fake lock */
	ld.l	r29, PT_SPINLOCKS, r0	/* next->pt_spinlocks-- */
	addi	r0, -1, r0
	st.l	r29, PT_SPINLOCKS, r0

	/* Check if we were preempted while holding the fake lock. */
	LDPTR	r29, PT_NEXT, r1	/* next->pt_next == NULL? */
	beq/l	r1, r63, tr0		/* _setcontext() if not preempted */

	/* Yes, we were. Bummer. Go to the next element in the chain. */
	STPTR	r29, PT_SWITCHTO, r29	/* next->pt_switchto = next */
	STPTR	r29, PT_SWITCHTOUC, r2
	STACK_SWITCH(r1, r2)
	blink	tr0, r63
	/* NOTREACHED */

Llocked_return_point:
	/* We're back on the original stack. */
	ld.q	r15, RND_CTXSIZE+24, r18
	movi	RND_CTXSIZE+32, r0
	ld.q	r15, RND_CTXSIZE+16, r30
	ptabs/l	r18, tr0
	ld.q	r15, RND_CTXSIZE+8, r29
	ld.q	r15, RND_CTXSIZE, r28
	add	r15, r0, r15
	blink	tr0, r18


/*
 * void	pthread__upcall_switch(pthread_t self, pthread_t next);
 *
 * Quit an upcall, recycle it, and jump to the selected thread.
 * Since this code never returns, we are free to use callee-saved
 * registers.
 */
ENTRY(pthread__upcall_switch)
	/* Create a `fake' lock count so we are `continued' */
	ld.l	r3, PT_SPINLOCKS, r0
	pta/l	Lupcall_no_old_preempt, tr0
	addi	r0, 1, r0
	st.l	r3, PT_SPINLOCKS, r0

	STACK_SWITCH(r3, r28)		/* r28 = next->pt_uc */

	/* Check if the upcall was preempted and continued. */
	pta/l	_C_LABEL(pthread__sa_recycle), tr1
	LDPTR	r2, PT_NEXT, r1
	beq/l	r1, r63, tr0

	/* Yes, it was. Stash the thread we were going to
	 * switch to, and go to the next thread in the chain.
	 */
	movi	PT_STATE_RECYCLABLE, r0
	STPTR	r2, PT_SWITCHTO, r3
	STPTR	r2, PT_SWITCHTOUC, r28
	st.l	r2, PT_STATE, r0
	pta/l	_C_LABEL(_setcontext_u), tr0
	STACK_SWITCH(r1, r2)
	ld.l	r3, PT_SPINLOCKS, r0
	addi	r0, -1, r0
	st.l	r3, PT_SPINLOCKS, r0
	blink	tr0, r63

Lupcall_no_old_preempt:		
	or	r3, r63, r29
	/* pthread__sa_recycle(old, new) */
	blink	tr1, r18

	/* Release the fake lock */
	ld.l	r29, PT_SPINLOCKS, r0
	pta/l	_C_LABEL(_setcontext_u), tr0
	addi	r0, -1, r0
	st.l	r29, PT_SPINLOCKS, r0

	/* Check if we were preempted while holding the fake lock. */
	or	r28, r63, r2
	LDPTR	r29, PT_NEXT, r1
	beq/l	r1, r63, tr0

	/* Yes, we were. Bummer. Go to the next element in the chain. */
	STPTR	r29, PT_SWITCHTO, r29
	STPTR	r29, PT_SWITCHTOUC, r2
	STACK_SWITCH(r1, r2)
	blink	tr0, r63
