/*	$NetBSD: pthread_switch.S,v 1.5 2003/09/07 14:47:56 cl Exp $	*/

/*-
 * Copyright (c) 2001, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams and Matt Thomas.
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
#define	NOTREACHED		\
	halt;			\
	/* NOTREACHED */

/* 		*** WARNING ***
 * STACK_SWITCH is more subtle than it looks. Please go read the extended
 * comment in the i386 pthread_switch.S file.
 */
	
#define STACK_SWITCH(pt)	\
	movl	PT_TRAPUC(pt),%sp ; \
	bneq	1f; \
	movl	PT_UC(pt),%sp; \
1:	clrl	PT_TRAPUC(pt)

ENTRY(pthread__switch, 0)
	movab	-(CONTEXTSIZE)(%sp), %sp

	/* Get the current context */
	pushl	%sp
	calls	$1,_getcontext_u

	/* Edit the context to make it continue below, rather than here */
	movab	switch_return_point,(UC_GREGS + _REG_PC)(%sp)

	movq	4(%ap),%r2
	movl	%sp,PT_UC(%r2)		/* self->pt_uc = &context */

	STACK_SWITCH(%r3)		/* r2 = self; r3 = next */
pthread__switch_return_point: .globl pthread__switch_return_point
setcontext:
	pushl	%sp			/* sp = &context */
	calls	$1,_setcontext_u
	NOTREACHED
switch_return_point:	
	ret

/*
 * Helper switch code used by pthread__locked_switch() and 
 * pthread__upcall_switch() when they discover spinlock preemption.
 */

pthread__switch_away:
	STACK_SWITCH(%r3)

	/* If we're invoked from the switch-to-next provisions of
	 * pthread__locked_switch or pthread__upcall_switch, there may
	 * be a fake spinlock-count set. If so, they will set r4 to
	 * let us know, and we decrement it now that we're no longer
	 * using the old stack.  
	 */
	subl2	%r4,PT_SPINLOCKS(%r5)
	jbr	setcontext		/* sp = &context */

/*
 * void	pthread__locked_switch(pthread_t self, pthread_t next, 
 *           pt_spin_t *lock);
 */
ENTRY(pthread__locked_switch, 0)
	movab	-(CONTEXTSIZE)(%sp),%sp
	movl	8(%ap),%r5

	/* Make sure we get continuted */
	incl	PT_SPINLOCKS(%r5)

	/* Get the current context */
	pushl	%sp
	calls	$1, _getcontext_u

	movq	4(%ap),%r4

	/* Edit the context to make it continue below, rather than here */
	movab	locked_return_point, (UC_GREGS + _REG_PC)(%sp)
	movl	%sp, PT_UC(%r4)

	STACK_SWITCH(%r5)	/* sp = next->pt_uc */

	/* Check if the switcher was preempted and continued to here. */
	movl	PT_NEXT(%r4),%r3
	beql	1f

	/* Yes, it was. Stash the thread we were going to switch to,
	 * the lock the original thread was holding, 
	 * and switch to the next thread in the continuation chain.
	 * Mark the fact that this was a locked switch, and so the
	 * thread does not need to be put on a run queue.
	 * Don't release the lock. It's possible that if we do so,
	 * PT_SWITCHTO will be stomped by another switch_lock and
	 * preemption.
	 */
	movl	12(%ap), PT_HELDLOCK(%r4)
	movl	%sp, PT_SWITCHTOUC(%r4)
	movl	%r5, PT_SWITCHTO(%r4)
	incl	PT_SPINLOCKS(%r4)

	/* %r3 = self->pt_next */
	movl	$1, %r4
	brw	pthread__switch_away

	/* No locked old-preemption */
1:	/* We've moved to the new stack, and the old context has been 
	 * saved. The queue lock can be released. 
	 */
	/* Reduce the lock count... */
	decl	PT_SPINLOCKS(%r4)

	/* ... actually release the lock.. */
	clrl	*12(%ap)

	/* .. and remove the fake lock */
	incl	PT_SPINLOCKS(%r5)
	
	/* Check if we were preempted while holding the fake lock. */
	movl	PT_NEXT(%r5),%r3
	jeql	setcontext
	
	/* Yes, we were. Go to the next element in the chain. */
	movl	%sp, PT_SWITCHTOUC(%r5)
	movl	%r5, PT_SWITCHTO(%r5)
	clrl	%r4
	brw	pthread__switch_away
	NOTREACHED

locked_return_point:	
	ret

/*
 * void	pthread__upcall_switch(pthread_t self, pthread_t next);
 *
 * Quit an upcall, recycle it, and jump to the selected thread.
 */

ENTRY(pthread__upcall_switch, 0)
	movq	4(%ap),%r4

	/*
	 * this code never returns, so we can treat s0-s6 as
	 * convenient registers that will be saved for us by callees,
	 * but that we do not have to save.
	 */
	
	/* Create a "fake" lock count so that this code will be continued */
	incl	PT_SPINLOCKS(%r4)

	STACK_SWITCH(%r5)

	/* Check if the upcall code was preempted and continued to here. */
	movl	PT_NEXT(%r4),%r3
	beql	1f

	/* Yes, it was. Stash the thread we were going to switch to,
	 * and switch to the next thread in the continuation chain.
	 */
	movl	%sp, PT_SWITCHTOUC(%r4)
	movq	%r5,PT_SWITCHTO(%r4)
	movl	$1,%r4
	brw	pthread__switch_away
	
	/* No old-upcall-preemption */
1:	movq	%r4,-(%sp)
	calls	$2, pthread__sa_recycle

	/* grab the arguments again */
	movq	4(%ap),%r4

	/* We can now release the fake lock. */
	decl	PT_SPINLOCKS(%r5)

	/* Check if we were preempted and continued while faking the lock */
	movl	PT_NEXT(%r5),%r3
	jeql	setcontext		/* No new-upcall-preemption */

	/* Yes, we were. Stash the to-be-switched-to context in our thread
	 * structure and go to the next link in the chain.
	 */
	movl	%sp, PT_SWITCHTOUC(%r5)
	movl	%r5, PT_SWITCHTO(%r5)
	clrl	%r4
	brw	pthread__switch_away	
