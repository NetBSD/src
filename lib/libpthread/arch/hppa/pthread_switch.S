/*	$NetBSD: pthread_switch.S,v 1.1 2004/07/19 03:39:02 chs Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles.
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
#include <machine/frame.h>
#include "assym.h"

#define CTXFRAMESIZE \
	(HPPA_FRAME_SIZE * 2 + HPPA_FRAME_ROUND(CONTEXTSIZE))

/* Force an error when "notreached" code is reached. */
#define	NOTREACHED		  \
	break	0,0		! \
	/* NOTREACHED */

/* 		*** WARNING ***
 * STACK_SWITCH is more subtle than it looks. Please go read the extended
 * comment in the i386 pthread_switch.S file.
 */

#define STACK_SWITCH(pt,tmp)		  \
	ldw	PT_TRAPUC(pt), tmp	! \
	comb,<>,n %r0, tmp, 99f		! \
	ldw	PT_UC(pt), tmp		! \
99:	ldo	CTXFRAMESIZE(tmp), %sp	! \
	stw	%r0, PT_TRAPUC(pt)

/*
 * void	pthread__switch(pthread_t self, pthread_t next);
 */

ENTRY(pthread__switch, CTXFRAMESIZE)
	stw	%rp, HPPA_FRAME_CRP(%sp)
	copy	%sp, %r1
	ldo	CTXFRAMESIZE(%sp), %sp
	stw	%arg0, HPPA_FRAME_ARG(0)(%sp)
	stw	%arg1, HPPA_FRAME_ARG(1)(%sp)

	copy	%r1, %arg0
	stw	%r19, HPPA_FRAME_ARG(2)(%sp)
	PIC_CALL(_getcontext_u)
	ldw	HPPA_FRAME_ARG(2)(%sp), %r19

	ldw	HPPA_FRAME_ARG(0)(%sp), %arg0
	ldw	HPPA_FRAME_ARG(1)(%sp), %arg1

	/* Edit the context to make it continue below, rather than here */
	ldo	-CTXFRAMESIZE(%sp), %t1
	stw	%t1, PT_UC(%arg0)
	addil	LT%pthread__switch_return_point, %r19
	ldo	RT%pthread__switch_return_point(%r1), %r1
	ldw	0(%r1), %t2
	stw	%t2, UC_PCOQH(%t1)

	STACK_SWITCH(%arg1, %arg0)

	PIC_CALL(_setcontext_u)
	NOTREACHED
EXIT(pthread__switch)
ENTRY_NOPROFILE(pthread__switch_return_point, 0)
9:
	ldw	(-CTXFRAMESIZE + HPPA_FRAME_CRP)(%sp), %rp
	bv	%r0(%rp)
	ldo	-CTXFRAMESIZE(%sp), %sp
EXIT(pthread__switch_return_point)

/*
 * Helper switch code used by pthread__locked_switch() and 
 * pthread__upcall_switch() when they discover spinlock preemption.
 */

ENTRY(pthread__switch_away, HPPA_FRAME_SIZE)
	STACK_SWITCH(%arg1, %t1)

	/*
	 * If we're invoked from the switch-to-next provisions of
	 * pthread__locked_switch or pthread__upcall_switch, there may
	 * be a fake spinlock-count set. If so, they will set %arg2 to
	 * let us know, and we decrement it now that we're no longer
	 * using the old stack.  
	 */
	comib,=,n 0, %arg2, 1f
	ldw	PT_SPINLOCKS(%arg0), %t2
	addi	-1, %t2, %t2
	stw	%t2, PT_SPINLOCKS(%arg0)

1:	copy	%t1, %arg0

	PIC_CALL(_setcontext_u)
	NOTREACHED
EXIT(pthread__switch_away)

/*
 * void	pthread__locked_switch(pthread_t self, pthread_t next, 
 *           pthread_spin_t *lock);
 *
 * Switch away from a thread that is holding a lock on a queue (to
 * prevent being removed from the queue before being switched away).
 */

ENTRY(pthread__locked_switch, CTXFRAMESIZE)
	stw	%rp, HPPA_FRAME_CRP(%sp)
	ldo	CTXFRAMESIZE(%sp), %sp
	stw	%arg0, HPPA_FRAME_ARG(0) - CTXFRAMESIZE(%sp)
	stw	%arg1, HPPA_FRAME_ARG(1) - CTXFRAMESIZE(%sp)
	stw	%arg2, HPPA_FRAME_ARG(2) - CTXFRAMESIZE(%sp)

	/* Make sure we get continued */
	ldw	PT_SPINLOCKS(%arg1), %t1
	addi	1, %t1, %t1
	stw	%t1, PT_SPINLOCKS(%arg1)

	/* Get the current context */
	ldo	-CTXFRAMESIZE(%sp), %arg0
	PIC_CALL(_getcontext_u)
	ldw	HPPA_FRAME_ARG(0) - CTXFRAMESIZE(%sp), %arg0
	ldw	HPPA_FRAME_ARG(1) - CTXFRAMESIZE(%sp), %arg1
	ldw	HPPA_FRAME_ARG(2) - CTXFRAMESIZE(%sp), %arg2

	/* Edit the context to make it continue below, rather than here */
	ldo	-CTXFRAMESIZE(%sp), %t1
	bl	1f, %t2
	nop
1:	ldo	(locked_return_point - 1b)(%t2), %t2
	stw	%t2, UC_PCOQH(%t1)
	stw	%t1, PT_UC(%arg0)

	STACK_SWITCH(%arg1, %t2)

	/* Check if the switcher was preempted and continued to here. */
	ldw	PT_NEXT(%arg0), %t1
	comib,=,n 0, %t1, 1f
	nop

	/*
	 * Yes, it was. Stash the thread we were going to switch to,
	 * the lock the original thread was holding, 
	 * and switch to the next thread in the continuation chain.
	 * Mark the fact that this was a locked switch, and so the
	 * thread does not need to be put on a run queue.
	 * Don't release the lock. It's possible that if we do so,
	 * PT_SWITCHTO will be stomped by another switch_lock and
	 * preemption.
	 */

	stw	%arg2, PT_HELDLOCK(%arg0)
	stw	%t2, PT_SWITCHTOUC(%arg0)
	stw	%arg1, PT_SWITCHTO(%arg0)

	ldw	PT_SPINLOCKS(%arg0), %t2
	addi	-1, %t2, %t2
	stw	%t2, PT_SPINLOCKS(%arg0)

	copy	%arg1, %arg0
	copy	%t1, %arg1
	ldi	1, %arg2
	b,n	pthread__switch_away
	NOTREACHED

	/* No locked old-preemption */
1:	/* We've moved to the new stack, and the old context has been 
	 * saved. The queue lock can be released. 
	 */
	/* Reduce the lock count... */
	ldw	PT_SPINLOCKS(%arg0), %t1
	addi	-1, %t1, %t1
	stw	%t1, PT_SPINLOCKS(%arg0)
	/* ... actually release the lock.. */
	nop
	ldi	1, %t1
	stw	%t1, 0(%arg2)
	/* .. and remove the fake lock */
	ldw	PT_SPINLOCKS(%arg1), %t1
	addi	-1, %t1, %t1
	stw	%t1, PT_SPINLOCKS(%arg1)

	/* Check if we were preempted while holding the fake lock. */
	ldw	PT_NEXT(%arg1), %t1
	comib,=,n 0, %t1, 2f
	nop

	/* Yes, we were. Go to the next element in the chain. */
	stw	%t2, PT_SWITCHTOUC(%arg1)
	stw	%arg1, PT_SWITCHTO(%arg1)
	copy	%arg1, %arg0
	copy	%t1, %arg1
	copy	%r0, %arg2
	b,n	pthread__switch_away
	NOTREACHED

2:	copy	%t2, %arg0

	PIC_CALL(_setcontext_u)
	NOTREACHED

locked_return_point:
	ldo	-CTXFRAMESIZE(%sp), %sp
	ldw	HPPA_FRAME_CRP(%sp), %rp
	bv,n	%r0(%rp)
EXIT(pthread__locked_switch)

/*
 * void	pthread__upcall_switch(pthread_t self, pthread_t next);
 *
 * Quit an upcall, recycle it, and jump to the selected thread.
 */

ENTRY(pthread__upcall_switch, 0)

	/*
	 * This code never returns, so we don't need to save the
	 * callee-saved registers.
	 */

	/* Create a "fake" lock count so that this code will be continued */
	ldw	PT_SPINLOCKS(%arg1), %t1
	addi	1, %t1, %t1
	stw	%t1, PT_SPINLOCKS(%arg1)

	STACK_SWITCH(%arg1, %r12)

	/* Check if the upcall code was preempted and continued to here. */
	ldw	PT_NEXT(%arg0), %t1
	comib,=,n 0, %t1, 1f

	/*
	 * Yes, it was. Stash the thread we were going to switch to,
	 * and switch to the next thread in the continuation chain.
	 */
	stw	%r12, PT_SWITCHTOUC(%arg0)
	stw	%arg1, PT_SWITCHTO(%arg0)
	copy	%arg1, %arg0
	copy	%t1, %arg1
	ldi	1, %arg2
	b,n	pthread__switch_away
	NOTREACHED

	/* No old-upcall-preemption */
1:	copy	%arg0, %r3
	copy	%arg1, %r4
	PIC_CALL(pthread__sa_recycle)
	copy	%r3, %arg0
	copy	%r4, %arg1

	/* We can now release the fake lock. */
	ldw	PT_SPINLOCKS(%arg1), %t1
	addi	-1, %t1, %t1
	stw	%t1, PT_SPINLOCKS(%arg1)

	/* Check if we were preempted and continued while faking the lock */
	ldw	PT_NEXT(%arg1), %t1
	comib,=,n 0, %t1, 2f

	/*
	 * Yes, we were. Stash the to-be-switched-to context in our thread
	 * structure and go to the next link in the chain.
	 */
	stw	%r12, PT_SWITCHTOUC(%arg1)
	stw	%arg1, PT_SWITCHTO(%arg1)
	copy	%arg1, %arg0
	copy	%t1, %arg1
	copy	%r0, %arg2
	b,n	pthread__switch_away

	/* No new-upcall-preemption */
2:	copy	%r12, %arg0

	PIC_CALL(_setcontext_u)
	NOTREACHED
EXIT(pthread__upcall_switch)
