/*	$NetBSD: pthread_switch.S,v 1.1.2.1 2001/09/25 19:49:06 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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

#include <machine/asm.h>
#include "assym.h"

/* Force an error when "notreached" code is reached. */
#define	NOTREACHED			  \
	call_pal PAL_bugchk		; \
	/* NOTREACHED */

NESTED(pthread__switch, 2, CONTEXTSIZE + 8, ra, 0, 0)
	LDGP(pv)
	/* Don't need LDGP(); this is only called from within
	 * the same module.
	 */
	lda	sp, -(CONTEXTSIZE + 8)(sp)
	stq	ra, 0(sp)
	lda	t0, 8(sp)
	stq	t0, PT_UC(a0)
	mov	t0, a0
	ldq	a1, PT_UC(a1)
	CALL(_swapcontext_u)
	ldq	ra, 0(sp)
	lda	sp, (CONTEXTSIZE + 8)(sp)
	RET
	END(pthread__switch)

/* 		*** WARNING ***
 * STACK_SWITCH is more subtle than it looks. Please go read the extended
 * comment in the i386 pthread_switch.S file.
 */
	
#define STACK_SWITCH(pt,tmp)		  \
	ldq	tmp, PT_UC(pt)		; \
	lda	sp, -STACKSPACE(tmp)

/*
 * Helper switch code used by pthread__locked_switch() and 
 * pthread__upcall_switch() when they discover spinlock preemption.
 */

NESTED(pthread__switch_away, 3, 0, ra, 0, 0)
	STACK_SWITCH(a1, s2)

	/* If we're invoked from the switch-to-next provisions of
	 * pthread__locked_switch or pthread__upcall_switch, there may
	 * be a fake spinlock-count set. If so, they will set a2 to
	 * let us know, and we decrement it now that we're no longer
	 * using the old stack.  
	 */
	beq	a2, 1f
	ldl	t0, PT_SPINLOCKS(a0)
	subl	t0, 1, t0
	stl	t0, PT_SPINLOCKS(a0)

1:	mov	s2, a0
	CALL(_setcontext_u)
	NOTREACHED
	END(pthread__switch_away)
	
NESTED(pthread__locked_switch, 3, CONTEXTSIZE + 32, ra, 0x00030000, 0)
	LDGP(pv)
	lda	sp, -(CONTEXTSIZE + 32)(sp)
	stq	ra, 0(sp)
	stq	a0, 8(sp)
	stq	a1, 16(sp)
	stq	a2, 24(sp)

	/* Make sure we get continuted */
	ldl	t0, PT_SPINLOCKS(a1)
	addl	t0, 1, t0
	stl	t0, PT_SPINLOCKS(a1)

	/* Get the current context */
	lda	t0, 32(sp)
	stq	t0, PT_UC(a0)
	mov	t0, a0
	CALL(_getcontext_u)
	ldq	a0, 8(sp)
	ldq	a1, 16(sp)
	ldq	a2, 24(sp)

	/* Edit the context to make it continue below, rather than here */
	lda	t3, 32(sp)
	lda	t1, locked_return_point
	stq	t1, UC_PC(t3)
	
	STACK_SWITCH(a1, t2)

	/* Check if the switcher was preempted and continued to here. */
	ldq	t0, PT_NEXT(a0)
	beq	t0, 1f

	/* Yes, it was. Stash the thread we were going to switch to,
	 * the lock the original thread was holding, 
	 * and switch to the next thread in the continuation chain.
	 * Mark the fact that this was a locked switch, and so the
	 * thread does not need to be put on a run queue.
	 * Don't release the lock. It's possible that if we do so,
	 * PT_SWITCHTO will be stomped by another switch_lock and
	 * preemption.
	 */
	stq	a1, PT_SWITCHTO(a0)
	stq	t2, PT_SWITCHTOUC(a0)
	stq	a2, PT_HELDLOCK(a0)
	ldl	t4, PT_SPINLOCKS(a0)
	subl	t4, 1, t4
	stl	t4, PT_SPINLOCKS(a0)

	/* Save the context we previously stored in PT_UC(a0); 
	 * that was overwritten when we were preempted and continued,
	 * so we need to put it somewhere. 
	 */
	stq	t3, PT_SLEEPUC(a0)

	mov	a1, a0
	mov	t0, a1
	mov	1, a2
	jmp	zero, pthread__switch_away
	NOTREACHED

	/* No locked old-preemption */
1:	/* We've moved to the new stack, and the old context has been 
	 * saved. The queue lock can be released. 
	 */
	/* Reduce the lock count... */
	ldl	t0, PT_SPINLOCKS(a0)
	subl	t0, 1, t0
	stl	t0, PT_SPINLOCKS(a0)
	/* ... actually release the lock.. */
	mb
	stl	zero, 0(a2)
	/* .. and remove the fake lock */
	ldl	t0, PT_SPINLOCKS(a1)
	subl	t0, 1, t0
	stl	t0, PT_SPINLOCKS(a1)
	
	/* Check if we were preempted while holding the fake lock. */
	ldq	t0, PT_NEXT(a1)
	beq	t0, 2f
	
	/* Yes, we were. Go to the next element in the chain. */
	stq	a1, PT_SWITCHTO(a1)
	stq	t2, PT_SWITCHTOUC(a1)
	mov	a1, a0
	mov	t0, a1
	mov	zero, a2
	jmp	zero, pthread__switch_away
	NOTREACHED

2:	mov	 t2, a0
	CALL(_setcontext_u)
locked_return_point:	
	ldq	ra, 0(sp)
	lda	sp, (CONTEXTSIZE + 32)(sp)
	RET
	END(pthread__locked_switch)

/*
 * void	pthread__upcall_switch(pthread_t self, pthread_t next);
 *
 * Quit an upcall, recycle it, and jump to the selected thread.
 */

NESTED(pthread__upcall_switch,2,0,ra,0,0)
	LDGP(pv)
	/* Loading the global pointer is unnecessary;  this routine
	 * is only ever called from the pthreads module, and the C
	 * code will have gp set up.
	 *
	 * Also, this code never returns, so we can treat s0-s6 as
	 * convenient registers that will be saved for us by callees,
	 * but that we do not have to save.
	 */
	
	/* Create a "fake" lock count so that this code will be continued */
	ldl	t0, PT_SPINLOCKS(a1)
	addl	t0, 1, t0
	stl	t0, PT_SPINLOCKS(a1)

	STACK_SWITCH(a1,s2)

	/* Check if the upcall code was preempted and continued to here. */
	ldq	t0, PT_NEXT(a0)
	beq	t0, 1f

	/* Yes, it was. Stash the thread we were going to switch to,
	 * and switch to the next thread in the continuation chain.
	 */
	stq	a1, PT_SWITCHTO(a0)
	stq	s2, PT_SWITCHTOUC(a0)
	mov	PT_STATE_RECYCLABLE, t1
	stl	t1, PT_STATE(a0)
	mov	a1, a0
	mov	t0, a1
	mov	1, a2
	jmp	zero, pthread__switch_away
	NOTREACHED
	
	/* No old-upcall-preemption */
1:	mov	a0, s0
	mov	a1, s1
	CALL(pthread__sa_recycle)
	mov	s0, a0
	mov	s1, a1
	
	/* We can now release the fake lock. */
	ldl	t0, PT_SPINLOCKS(a1)
	subl	t0, 1, t0
	stl	t0, PT_SPINLOCKS(a1)

	/* Check if we were preempted and continued while faking the lock */
	ldq	t0, PT_NEXT(a1)
	beq	t0, 2f

	/* Yes, we were. Stash the to-be-switched-to context in our thread
	 * structure and go to the next link in the chain.
	 */
	stq	a1, PT_SWITCHTO(a1)
	stq	s2, PT_SWITCHTOUC(a1)
	mov	a1, a0
	mov	t0, a1
	mov	0, a2
	jmp	zero, pthread__switch_away	

	/* No new-upcall-preemption */
2:	mov	s2, a0
	CALL(_setcontext_u)
	NOTREACHED
	END(pthread__upcall_switch)
