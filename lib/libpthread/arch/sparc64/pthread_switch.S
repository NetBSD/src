/*	$NetBSD: pthread_switch.S,v 1.6 2003/08/25 23:00:23 petrov Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#define STACK_ALIGN(x)	(((x) + 0xf) & ~0xf)

#define NOTREACHED	illtrap	10

/* 		*** WARNING ***
 * STACK_SWITCH is more subtle than it looks. Please go read the extended
 * comment in the i386 pthread_switch.S file.
 */


/*
 * pt register which contains *pthread, which stack will be used
 * tmp temporary register which will have *ucontext_t for that pthread
 */
#define STACK_SWITCH(pt,tmp)		  \
	ldx	[pt + PT_TRAPUC], tmp	; \
	brnz,a	tmp, 8f			; \
	 stx	%g0, [pt + PT_TRAPUC]	; \
	ldx	[pt + PT_UC], tmp	; \
8:	add	tmp, - STACK_ALIGN(STACKSPACE + CONTEXTSIZE) - BIAS, %sp ; \


ENTRY(pthread__switch)
	save	%sp, -STACK_ALIGN(CC64FSZ + CONTEXTSIZE), %sp

	/* Get the current context */
	add	%fp, BIAS - CONTEXTSIZE, %l3
	call	_C_LABEL(_getcontext_u)
	 mov %l3, %o0

	/* Edit the context to make it continue below, rather than here */
#ifdef PIC
	PICCY_SET(pthread__switch_return_point,%l1,%l5)
#else
	set pthread__switch_return_point, %l1
#endif
	stx %l1, [%l3 + UC_GREGS + _REG_PC * 8]
	stx	%l3, [%i0 + PT_UC]

	STACK_SWITCH(%i1, %l2)

	/* %l2 points to next thread's ucontext */
	call	_C_LABEL(_setcontext_u)
	 mov	 %l2, %o0
	NOTREACHED

ENTRY_NOPROFILE(pthread__switch_return_point)
	ret
	 restore


/*
 * Helper switch code used by pthread__locked_switch() and 
 * pthread__upcall_switch() when they discover spinlock preemption.
 */

/*
 * pthread__switch_away
 * %i0 who's lock to release
 * %i1 thread to switch to
 * %i2 flag to clear lock
 */

ENTRY(pthread__switch_away)

	STACK_SWITCH(%i1,%l2)

	/* If we're invoked from the switch-to-next provisions of
	 * pthread__locked_switch or pthread__upcall_switch, there may
	 * be a fake spinlock-count set. If so, they will set i2 to
	 * let us know, and we decrement it now that we're no longer
	 * using the old stack.  
	 */
	brz,a	%i2, 1f
	 nop
	ld	[%i0 + PT_SPINLOCKS], %l0
	dec	%l0
	st	%l0, [%i0 + PT_SPINLOCKS]

1:	call	_C_LABEL(_setcontext_u)
	 mov	%l2, %o0
	NOTREACHED
	
/*
 * void	pthread__upcall_switch(pthread_t self, pthread_t next);
 *
 * Quit an upcall, recycle it, and jump to the selected thread.
 */
ENTRY(pthread__upcall_switch)
	save	%sp, -CC64FSZ, %sp

	/* Create a "fake" lock count so that this code will be continued */
	ld	[%i1 + PT_SPINLOCKS], %l0
	inc	%l0
	st	%l0, [%i1 + PT_SPINLOCKS]

	/*
	 * switch to next thread stack so we could recycle self's one
	 */
	STACK_SWITCH(%i1,%l2)

	/* Check if the upcall code was preempted and continued to here. */
	ldx	[%i0 + PT_NEXT], %l0
	brz,a	%l0, 1f
	 nop

	/* Yes, it was. Stash the thread we were going to switch to,
	 * and switch to the next thread in the continuation chain.
	 */
	or	%g0, PT_STATE_RECYCLABLE, %l1
	st	%l1, [%i0 + PT_STATE]
	stx	%l2, [%i0 + PT_SWITCHTOUC]
	stx	%i1, [%i0 + PT_SWITCHTO]
	mov	%i1, %i0
	mov	%l0, %i1
	ba	pthread__switch_away
	 mov	1, %i2
	NOTREACHED
	
	/* No old-upcall-preemption */
1:	mov	%i0, %o0
	call	_C_LABEL(pthread__sa_recycle)
	 mov	%i1, %o1
	
	/* We can now release the fake lock. */
	ld	[%i1 + PT_SPINLOCKS], %l0
	dec	%l0
	st	%l0, [%i1 + PT_SPINLOCKS]

	/* Check if we were preempted and continued while faking the lock */
	ldx	[%i1 + PT_NEXT], %l0
	brz,a	%l0, 2f
	 nop

	/* Yes, we were. Stash the to-be-switched-to context in our thread
	 * structure and go to the next link in the chain.
	 */
	stx	%l2, [%i1 + PT_SWITCHTOUC]
	stx	%i1, [%i1 + PT_SWITCHTO]
	mov	%i1, %i0
	mov	%l0, %i1
	ba	pthread__switch_away
	 mov	0, %i2

	/* No new-upcall-preemption */
2:
	call	_C_LABEL(_setcontext_u)
	 mov	%l2, %o0
	NOTREACHED


/*
 * void	pthread__locked_switch(pthread_t self, pthread_t next, 
 *           pt_spin_t *lock);
 *
 * Switch away from a thread that is holding a lock on a queue (to
 * prevent being removed from the queue before being switched away).
 */
ENTRY(pthread__locked_switch)
	save	%sp, -STACK_ALIGN(CC64FSZ + CONTEXTSIZE), %sp

	/* Make sure we get continuted */
	ld	[%i1 + PT_SPINLOCKS], %l0
	inc	%l0
	st	%l0, [%i1 + PT_SPINLOCKS]

	/* Get the current context */
	add	%fp, BIAS - CONTEXTSIZE, %l3
	call	_C_LABEL(_getcontext_u)
	 mov %l3, %o0

	/* Edit the context to make it continue below, rather than here */
#ifdef PIC
	PICCY_SET(locked_return_point,%l1,%l5)
#else
	set locked_return_point, %l1
#endif
	stx %l1, [%l3 + UC_GREGS + _REG_PC * 8]
	stx	%l3, [%i0 + PT_UC]

	STACK_SWITCH(%i1, %l2)

	/* Check if the switcher was preempted and continued to here. */
	ldx	[%i0 + PT_NEXT], %l4
	brz,a	%l4, 1f
	 nop

	/* Yes, it was. Stash the thread we were going to switch to,
	 * the lock the original thread was holding, 
	 * and switch to the next thread in the continuation chain.
	 * Mark the fact that this was a locked switch, and so the
	 * thread does not need to be put on a run queue.
	 * Don't release the lock. It's possible that if we do so,
	 * PT_SWITCHTO will be stomped by another switch_lock and
	 * preemption.
	 */
	stx	%i2, [%i0 + PT_HELDLOCK]
	stx	%l2, [%i0 + PT_SWITCHTOUC]
	stx	%i1, [%i0 + PT_SWITCHTO]
	ld	[%i0 + PT_SPINLOCKS], %l0
	dec	%l0
	st	%l0, [%i0 + PT_SPINLOCKS]

	mov	%i1, %i0
	mov	%l4, %i1
	ba	pthread__switch_away
	 mov	1, %i2
	NOTREACHED

	/* No locked old-preemption */
1:	/* We've moved to the new stack, and the old context has been 
	 * saved. The queue lock can be released. 
	 */
	/* Reduce the lock count... */
	ld	[%i0 + PT_SPINLOCKS], %l0
	dec	%l0
	st	%l0, [%i0 + PT_SPINLOCKS]

	/* ... actually release the lock.. */
!	membar	#Sync

	st	%g0, [%i2]
	/* .. and remove the fake lock */
	ld	[%i1 + PT_SPINLOCKS], %l0
	dec	%l0
	st	%l0, [%i1 + PT_SPINLOCKS]
	
	/* Check if we were preempted while holding the fake lock. */
	ldx	[%i1 + PT_NEXT], %l0
	brz,a	%l0, 2f
	 nop

	/* Yes, we were. Go to the next element in the chain. */
	stx	%l2, [%i1 + PT_SWITCHTOUC]
	stx	%i1, [%i1 + PT_SWITCHTO]
	mov	%i1, %i0
	mov	%l0, %i1
	ba	pthread__switch_away
	 mov	%g0, %i2
	NOTREACHED

2:	/* %l2 points to next thread's ucontext */
	call	_C_LABEL(_setcontext_u)
	 mov	 %l2, %o0
	NOTREACHED

locked_return_point:
	ret
	 restore


