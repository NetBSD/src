/*	$NetBSD: pthread_switch.S,v 1.6 2003/09/07 14:47:51 cl Exp $	*/

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
#include "assym.h"

#ifdef __ABICALLS__
	.abicalls
# define PIC_PROLOGUE(x,sr)	  \
	.set noreorder		; \
	.cpload sr		; \
	.set reorder

# define PIC_CALL(l,sr)		  \
	la sr, _C_LABEL(l)	; \
	jalr sr

#else
# define PIC_PROLOGUE(x,sr)
# define PIC_CALL(l,sr)		  \
	jalr  _C_LABEL(l)

#endif
	
/* Force an error when "notreached" code is reached. */
#define	NOTREACHED		  \
	break	1		; \
	/* NOTREACHED */

/* 		*** WARNING ***
 * STACK_SWITCH is more subtle than it looks. Please go read the extended
 * comment in the i386 pthread_switch.S file.
 */
	
#define STACK_SWITCH(pt,tmp)		  \
	lw	tmp, PT_TRAPUC(pt)	; \
	bnez	tmp, 1f			; \
	lw	tmp, PT_UC(pt)		; \
1:	la	sp, -STACKSPACE(tmp)	; \
	sw	zero, PT_TRAPUC(pt)

NESTED(pthread__switch, CALLFRAME_SIZ+CONTEXTSIZE, ra)
	.mask	0x80000000, (CALLFRAME_RA - CALLFRAME_SIZ)
	PIC_PROLOGUE(pthread__switch, t9)
	subu	sp, sp, (CALLFRAME_SIZ+CONTEXTSIZE)
	sw	ra, CALLFRAME_RA(sp)
	sw	a0, 0(sp)
	sw	a1, 4(sp)
	
	la	a0, CALLFRAME_SIZ(sp)
	PIC_CALL(_getcontext_u, t9)
	lw	a0, 0(sp)
	lw	a1, 4(sp)

	/* Edit the context to make it continue below, rather than here */
	la	t3, CALLFRAME_SIZ(sp)
	la	t1, pthread__switch_return_point
	sw	t1, UC_EPC(t3)
	sw	t3, PT_UC(a0)
	
	STACK_SWITCH(a1, t2)

	move	a0, t2
	PIC_CALL(_setcontext_u, t9)
	NOTREACHED
XNESTED(pthread__switch_return_point)
	lw	ra, CALLFRAME_RA(sp)
	addiu	sp, (CALLFRAME_SIZ+CONTEXTSIZE)
	j	ra
	END(pthread__switch)

/*
 * Helper switch code used by pthread__locked_switch() and 
 * pthread__upcall_switch() when they discover spinlock preemption.
 */

NESTED(pthread__switch_away, CALLFRAME_SIZ, ra)
	.mask	0x00000000, (CALLFRAME_RA - CALLFRAME_SIZ)
	STACK_SWITCH(a1, s2)

	/* If we're invoked from the switch-to-next provisions of
	 * pthread__locked_switch or pthread__upcall_switch, there may
	 * be a fake spinlock-count set. If so, they will set a2 to
	 * let us know, and we decrement it now that we're no longer
	 * using the old stack.  
	 */
	beqz	a2, 1f
	lw	t0, PT_SPINLOCKS(a0)
	subu	t0, t0, 1
	sw	t0, PT_SPINLOCKS(a0)

1:	move	a0, s2
	PIC_CALL(_setcontext_u, t9)
	NOTREACHED
	END(pthread__switch_away)
	
NESTED(pthread__locked_switch, CALLFRAME_SIZ+CONTEXTSIZE, ra)
	.mask	0x80000070, (CALLFRAME_RA - CALLFRAME_SIZ)
	PIC_PROLOGUE(pthread__locked_switch, t9)
	subu	sp, sp, (CALLFRAME_SIZ+CONTEXTSIZE)
	sw	ra, CALLFRAME_RA(sp)
	sw	a0, 0(sp)
	sw	a1, 4(sp)
	sw	a2, 8(sp)

	/* Make sure we get continuted */
	lw	t0, PT_SPINLOCKS(a1)
	addiu	t0, t0, 1
	sw	t0, PT_SPINLOCKS(a1)

	/* Get the current context */
	la	a0, CALLFRAME_SIZ(sp)
	PIC_CALL(_getcontext_u, t9)
	lw	a0, 0(sp)
	lw	a1, 4(sp)
	lw	a2, 8(sp)

	/* Edit the context to make it continue below, rather than here */
	la	t3, CALLFRAME_SIZ(sp)
	la	t1, locked_return_point
	sw	t1, UC_EPC(t3)
	sw	t3, PT_UC(a0)
	
	STACK_SWITCH(a1, t2)

	/* Check if the switcher was preempted and continued to here. */
	lw	t0, PT_NEXT(a0)
	beqz	t0, 1f
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
	sw	a2, PT_HELDLOCK(a0)
	sw	t2, PT_SWITCHTOUC(a0)
	sw	a1, PT_SWITCHTO(a0)
	lw	t4, PT_SPINLOCKS(a0)
	subu	t4, t4, 1
	sw	t4, PT_SPINLOCKS(a0)

	move	a0, a1
	move	a1, t0
	li	a2, 1
	j	pthread__switch_away
	NOTREACHED

	/* No locked old-preemption */
1:	/* We've moved to the new stack, and the old context has been 
	 * saved. The queue lock can be released. 
	 */
	/* Reduce the lock count... */
	lw	t0, PT_SPINLOCKS(a0)
	subu	t0, t0, 1
	sw	t0, PT_SPINLOCKS(a0)
	/* ... actually release the lock.. */
	nop # sync
	sw	zero, 0(a2)
	/* .. and remove the fake lock */
	lw	t0, PT_SPINLOCKS(a1)
	subu	t0, t0, 1
	sw	t0, PT_SPINLOCKS(a1)
	
	/* Check if we were preempted while holding the fake lock. */
	lw	t0, PT_NEXT(a1)
	beqz	t0, 2f
	nop

	/* Yes, we were. Go to the next element in the chain. */
	sw	t2, PT_SWITCHTOUC(a1)
	sw	a1, PT_SWITCHTO(a1)
	move	a0, a1
	move	a1, t0
	li	a2, 0
	j	pthread__switch_away
	NOTREACHED

2:	move	a0, t2
	PIC_CALL(_setcontext_u, t9)
	NOTREACHED

locked_return_point:
	lw	ra, CALLFRAME_RA(sp)
	addiu	sp, sp, (CALLFRAME_SIZ+CONTEXTSIZE)
	j	ra
	END(pthread__locked_switch)

/*
 * void	pthread__upcall_switch(pthread_t self, pthread_t next);
 *
 * Quit an upcall, recycle it, and jump to the selected thread.
 */

NESTED(pthread__upcall_switch, 0, ra)
	.mask	0x00000000, (CALLFRAME_RA - CALLFRAME_SIZ)
	PIC_PROLOGUE(pthread__upcall_switch, t9)

	/* Loading the global pointer is unnecessary;  this routine
	 * is only ever called from the pthreads module, and the C
	 * code will have gp set up.
	 *
	 * Also, this code never returns, so we can treat s0-s6 as
	 * convenient registers that will be saved for us by callees,
	 * but that we do not have to save.
	 */
	
	/* Create a "fake" lock count so that this code will be continued */
	lw	t0, PT_SPINLOCKS(a1)
	addiu	t0, t0, 1
	sw	t0, PT_SPINLOCKS(a1)

	STACK_SWITCH(a1,s2)

	/* Check if the upcall code was preempted and continued to here. */
	lw	t0, PT_NEXT(a0)
	beqz	t0, 1f

	/* Yes, it was. Stash the thread we were going to switch to,
	 * and switch to the next thread in the continuation chain.
	 */
	sw	s2, PT_SWITCHTOUC(a0)
	sw	a1, PT_SWITCHTO(a0)
	move	a0, a1
	move	a1, t0
	li	a2, 1
	j	pthread__switch_away
	NOTREACHED
	
	/* No old-upcall-preemption */
1:	move	s0, a0
	move	s1, a1
	PIC_CALL(pthread__sa_recycle, t9)
	move	a0, s0
	move	a1, s1
	
	/* We can now release the fake lock. */
	lw	t0, PT_SPINLOCKS(a1)
	subu	t0, t0, 1
	sw	t0, PT_SPINLOCKS(a1)

	/* Check if we were preempted and continued while faking the lock */
	lw	t0, PT_NEXT(a1)
	beqz	t0, 2f

	/* Yes, we were. Stash the to-be-switched-to context in our thread
	 * structure and go to the next link in the chain.
	 */
	sw	s2, PT_SWITCHTOUC(a1)
	sw	a1, PT_SWITCHTO(a1)
	move	a0, a1
	move	a1, t0
	li	a2, 0
	j	pthread__switch_away	

	/* No new-upcall-preemption */
2:	move	a0, s2
	PIC_CALL(_setcontext_u, t9)
	NOTREACHED
	END(pthread__upcall_switch)
