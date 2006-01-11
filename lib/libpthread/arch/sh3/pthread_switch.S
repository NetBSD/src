/*	$NetBSD: pthread_switch.S,v 1.5.2.1 2006/01/11 13:53:07 tron Exp $ */
	
/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
 * Enter debugger or die with SIGTRAP if "notreached" code is reached
 */
#define NOTREACHED	trapa #0xc3


/*
 * To avoid superfluous memory loads, or consuming extra registers
 * this code makes assumptions that certain constants are small
 * enough to be used as immediate data in certain instructions or
 * addressing formats.  It is expected that the layout of pthread
 * internal structure is not going to be changed frequently.
 */
#define ASSERT_LONG_DISPLACEMENT_OK(i)	.ifgt (i) -  60; .err; .endif
#define ASSERT_IMMEDIATE_7BIT_OK(i)	.ifgt (i) - 127; .err; .endif
#define ASSERT_IMMEDIATE_8BIT_OK(i)	.ifgt (i) - 255; .err; .endif

ASSERT_LONG_DISPLACEMENT_OK(UC_PC)
ASSERT_LONG_DISPLACEMENT_OK(PT_SPINLOCKS)

ASSERT_IMMEDIATE_7BIT_OK(PT_UC)
ASSERT_IMMEDIATE_7BIT_OK(PT_TRAPUC)


/*
 * Utility macro to switch to the stack of another thread.
 * Sets tmp to the thread's ucontext to switch to.
 * Clobbers r0 and r1.
 *
 * 		*** WARNING ***
 * STACK_SWITCH is more subtle than it looks. Please go read the extended
 * comment in the i386 pthread_switch.S file.
 */
#define STACK_SWITCH(pt,tmp)					  \
	mov	#PT_TRAPUC, r0					; \
	mov.l	@(r0,pt), tmp	/* check pt_trapuc first */	; \
	mov	#0, r1						; \
	tst	tmp, tmp	/* pt_trapuc == NULL? */	; \
	bf/s	100f		/* if not, use it */		; \
	 mov.l	r1, @(r0,pt)	/* clear pt_trapuc */		; \
	mov	#PT_UC, r0					; \
	mov.l	@(r0,pt), tmp	/* else, switch to pt_uc */	; \
100:								  \
.ifeq STACKSPACE						; \
	mov	tmp, sp						; \
.else								; \
	mov	tmp, r0						; \
	add	#-STACKSPACE, r0				; \
	mov	r0, sp						; \
.endif



/*
 * void	pthread__switch(pthread_t self, pthread_t next);
 *
 * Plain switch that doesn't do any special checking or handle
 * spin-preemption. It isn't used much by normal code, actually; it's
 * main purpose is to be a basic switch engine when the MI code is
 * already dealing with spin-preemption or other gunk.  
 */
NENTRY(pthread__switch)
	sts.l	pr, @-sp
	mov	r4, r7	/* free r4 for function calls */

	mov.l	.L_rnd_ctxsize, r0
	sub	r0, sp			/* auto ucontext_t cntx; */

	mov.l	.L__getcontext_u_11, r0
11:	CALL	r0			/* _getcontext_u(&cntx); */
	 mov	sp, r4

	/*
	 * Edit the context so that it continues as if returning from
	 * the _setcontext_u below:
	 */
	mova	pthread__switch_return_point, r0
	mov.l	r0, @(UC_PC, sp)

	mov	#PT_UC, r0
	mov.l	sp, @(r0, r7)		/* self->pt_uc = &cntx; */

	STACK_SWITCH(r5, r4)		/* r4 = next->pt_uc; */

	mov.l	.L__setcontext_u_12, r0
12:	CALL	r0			/* _setcontext_u(next->pt_uc); */
	 nop				/* r4 already set by STACK_SWITCH */
	NOTREACHED


	.align 2 /* mova target */
ALTENTRY(pthread__switch_return_point)
	/* we're back on the original stack */
	mov.l	.L_rnd_ctxsize, r0
	add	r0, sp			/* ditch the ucontext_t */
	lds.l	@sp+, pr
	rts
	 nop


	.align	2
.L__getcontext_u_11:
	CALL_DATUM_LOCAL(_C_LABEL(_getcontext_u), 11b)
.L__setcontext_u_12:
	CALL_DATUM_LOCAL(_C_LABEL(_setcontext_u), 12b)



/*
 * Helper switch code used by pthread__locked_switch() and 
 * pthread__upcall_switch() when they discover spinlock preemption.
 *
 * Call with:
 *   r2 = from, r3 = to
 */
Lpthread__switch_away_decrement:
	STACK_SWITCH(r3, r4)		/* side effect: r4 = to->pt_uc */

	/*
	 * If we're invoked from the switch-to-next provisions of
	 * pthread__locked_switch or pthread__upcall_switch, there
	 * may be a fake spinlock-count set.  If so, we decrement it
	 * once we're no longer using the old stack.
	 */
	mov.l	@(PT_SPINLOCKS, r2), r0
	mov.l	.L__setcontext_u_21, r1
	add	#-1, r0			/* --from->pt_spinlocks; */
21:	CALL	r1			/* _setcontext_u(to->pt_uc); */
	 mov.l	r0, @(PT_SPINLOCKS, r2)
	NOTREACHED

	.align	2
.L__setcontext_u_21:
	CALL_DATUM_LOCAL(_C_LABEL(_setcontext_u), 21b)



/*
 * Helper switch code used by pthread__locked_switch() and 
 * pthread__upcall_switch().
 *
 * Call with:
 *   r3 = to
 */
Lpthread__switch_away_no_decrement:
	STACK_SWITCH(r3, r4)		/* side effect: r4 = to->pt_uc */

	mov.l	.L__setcontext_u_31, r1
31:	CALL	r1			/* _setcontext_u(to->pt_uc); */
	 nop
	NOTREACHED

	.align	2
.L__setcontext_u_31:
	CALL_DATUM_LOCAL(_C_LABEL(_setcontext_u), 31b)



/*
 * void	pthread__locked_switch(pthread_t self, pthread_t next, pt_spin_t *lock);
 *
 * Switch away from a thread that is holding a lock on a queue (to
 * prevent being removed from the queue before being switched away).
 */
NENTRY(pthread__locked_switch)
	sts.l	pr, @-sp
	mov	r4, r7	/* free r4 for function calls */

	mov.l	.L_rnd_ctxsize, r0
	sub	r0, sp			/* auto ucontext_t cntx; */ 

	mov.l	@(PT_SPINLOCKS, r5), r0	/* make sure we get continued */
	add	#1, r0
	mov.l	r0, @(PT_SPINLOCKS, r5)	/* ++next->pt_spinlocks */

	mov.l	.L__getcontext_u_41, r0
41:	CALL	r0			/* _getcontext_u(self->pt_uc); */
	 mov	sp, r4

	/*
	 * Edit the context so that it continues as if returning from
	 * the _setcontext_u below.  
	 */
	mova	Lpthread__locked_switch_return_point, r0
	mov.l	r0, @(UC_PC, sp)

	mov	#PT_UC, r0
	mov.l	sp, @(r0, r7)		/* self->pt_uc = &cntx; */

	STACK_SWITCH(r5, r4)		/* r4 = next->pt_uc; */

	/*
	 * Check if the original thread was preempted while holding
	 * its queue lock.
	 */
	mov.l	.L_pt_next, r0
	mov.l	@(r0, r7), r3
	tst	r3, r3			/* self->pt_next == NULL? */
	bt	Llocked_no_old_preempt

	/*
	 * Yes, it was.  Stash the thread we were going to switch to,
	 * the lock the original thread was holding, and go to the
	 * next thread in the chain.  Mark the fact that this was a
	 * locked switch, and so the thread does not need to be put on
	 * a run queue.  Don't release the lock.  It's possible that
	 * if we do so, PT_SWITCHTO will be stomped by another
	 * switch_lock and preemption.
         */

	mov.l	.L_pt_heldlock, r0
	mov.l	r6, @(r0, r7)		/* self->pt_heldlock = lock; */

	mov.l	.L_pt_switchtouc, r0
	mov.l	r4, @(r0, r7)		/* self->pt_switchtouc= next->pt_uc; */

	mov.l	.L_pt_switchto, r0
	mov.l	r5, @(r0, r7)		/* self->pt_switchto = next; */

	mov.l	@(PT_SPINLOCKS, r7), r0
	add	#-1, r0
	mov.l	r0, @(PT_SPINLOCKS, r7)	/* --self->pt_spinlocks */

	/*
	 * r2: from = next
	 * r3: to = self->pt_next (already)
	 */
	bra	Lpthread__switch_away_decrement
	 mov	r5, r2
	NOTREACHED


Llocked_no_old_preempt:
	/*
	 * We've moved to the new stack, and the old context has been 
	 * saved.  The queue lock can be released.
	 */
	mov.l	@(PT_SPINLOCKS, r7), r0
	add	#-1, r0
	mov.l	r0, @(PT_SPINLOCKS, r7)	/* --self->pt_spinlocks */

	/* We happen to know that this is the right way to release a lock. */
	mov	#0, r0
	mov.b	r0, @r6		/* *lock = 0; */

	/* Remove the fake lock. */
	mov.l	@(PT_SPINLOCKS, r5), r0
	add	#-1, r0
	mov.l	r0, @(PT_SPINLOCKS, r5)	/* --next->pt_spinlocks */

	/* Check if we were preempted while holding the fake lock. */
	mov.l	.L_pt_next, r0
	mov.l	@(r0, r5), r3
	tst	r3, r3			/* next->pt_next == NULL? */
	bt	Llocked_no_new_preempt

	/* Yes, we were. Bummer. Go to the next element in the chain. */
	mov.l	.L_pt_switchtouc, r0
	mov.l	r4, @(r0, r5)		/* next->pt_switchtouc= next->pt_uc; */

	mov.l	.L_pt_switchto, r0
	mov.l	r5, @(r0, r5)		/* next->pt_switchto = next; */

	/* r3: to = next->pt_next (already) */
	bra	Lpthread__switch_away_no_decrement
	 nop
	NOTREACHED


Llocked_no_new_preempt:
	mov.l	.L__setcontext_u_42, r0
42:	CALL	r0			/* _setcontext_u(next->pt_uc) */
	 nop
	NOTREACHED


	.align 2 /* mova target */
Lpthread__locked_switch_return_point:
	/* we're back on the original stack */
	mov.l	.L_rnd_ctxsize, r0
	add	r0, sp			/* ditch the ucontext_t */
	lds.l	@sp+, pr
	rts
	 nop


	.align	2
.L__getcontext_u_41:
	CALL_DATUM_LOCAL(_C_LABEL(_getcontext_u), 41b)
.L__setcontext_u_42:
	CALL_DATUM_LOCAL(_C_LABEL(_setcontext_u), 42b)



/*
 * void	pthread__upcall_switch(pthread_t self, pthread_t next);
 *
 * Quit an upcall, recycle it, and jump to the selected thread.
 * Since this code never returns, we are free to use callee-saved
 * registers.
 */
NENTRY(pthread__upcall_switch)
	mov	r4, r8	/* self */
	mov	r5, r9	/* next */

	/* Create a `fake' lock count so we are `continued' */
	mov.l	@(PT_SPINLOCKS, r9), r0
	add	#1, r0
	mov.l	r0, @(PT_SPINLOCKS, r9)	/* ++next->pt_spinlocks */

	STACK_SWITCH(r9, r11)		/* r11 = next->pt_uc; */

	/* Check if the upcall was preempted and continued. */
	mov.l	.L_pt_next, r0
	mov.l	@(r0, r8), r3
	tst	r3, r3			/* self->pt_next == NULL? */
	bt	Lupcall_no_old_preempt

	/*
	 * Yes, it was.  Stash the thread we were going to switch to,
	 * and go to the next thread in the chain.
	 */
	mov.l	.L_pt_switchtouc, r0
	mov.l	r11, @(r0, r8)		/* self->pt_switchtouc= next->pt_uc; */

	mov.l	.L_pt_switchto, r0
	mov.l	r9, @(r0, r8)		/* self->pt_switchto = next; */

	/*
	 * r2: from = next
	 * r3: to = self->pt_next (already)
	 */
	bra	Lpthread__switch_away_decrement
	 mov	r9, r2
	NOTREACHED


Lupcall_no_old_preempt:
	/* pthread__sa_recycle(old, new) */
	mov.l	.L_pthread__sa_recycle_51, r0
	mov	r8, r4		/* old = self */
51:	CALL	r0
	 mov	r9, r5		/* new = next */

	/* Release the fake lock */
	mov.l	@(PT_SPINLOCKS, r9), r0
	add	#-1, r0
	mov.l	r0, @(PT_SPINLOCKS, r9)	/* --next->pt_spinlocks */

	/* Check if we were preempted while holding the fake lock. */
	mov.l	.L_pt_next, r0
	mov.l	@(r0, r9), r3
	tst	r3, r3			/* next->pt_next == NULL? */
	bt	Lupcall_no_new_preempt

	/* Yes, we were. Bummer. Go to the next element in the chain. */
	mov.l	.L_pt_switchtouc, r0
	mov.l	r11, @(r0, r9)		/* next->pt_switchtouc= next->pt_uc; */

	mov.l	.L_pt_switchto, r0
	mov.l	r9, @(r0, r9)		/* next->pt_switchto = next; */

	/* r3: to = next->pt_next (already) */
	bra	Lpthread__switch_away_no_decrement
	 nop
	NOTREACHED


Lupcall_no_new_preempt:
	mov.l	.L__setcontext_u_52, r0
52:	CALL	r0			/* _setcontext_u(next->pt_uc); */
	 mov	r11, r4
	NOTREACHED

	.align	2
.L_pthread__sa_recycle_51:
	CALL_DATUM_LOCAL(_C_LABEL(pthread__sa_recycle), 51b)
.L__setcontext_u_52:
	CALL_DATUM_LOCAL(_C_LABEL(_setcontext_u), 52b)



/*
 * Data shared by all the functions in this file
 */
	.align	2
.L_rnd_ctxsize:		.long	RND_CTXSIZE

.L_pt_heldlock:		.long	PT_HELDLOCK
.L_pt_next:		.long	PT_NEXT
.L_pt_switchto:		.long	PT_SWITCHTO
.L_pt_switchtouc:	.long	PT_SWITCHTOUC
