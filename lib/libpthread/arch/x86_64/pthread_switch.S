/*	$NetBSD: pthread_switch.S,v 1.5 2003/07/26 19:25:07 salo Exp $	*/

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
 *
 * Adapted for x86_64 by fvdl@NetBSD.org
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
#define	NOTREACHED			  \
	int3


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
 * ("Important safety tip. Thanks, Egon").
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

#define STACK_SWITCH			  \
	movq	PT_TRAPUC(%r13), %r11	; \
	cmpq	$0, %r11		; \
	jne	1f			; \
	movq	PT_UC(%r13), %r11	; \
1:	leaq	(-STACKSPACE)(%r11), %rsp ; \
	movq	$0, PT_TRAPUC(%r13)


/*
 * void	pthread__switch(pthread_t self, pthread_t next);
 *
 * Plain switch that doesn't do any special checking or handle
 * spin-preemption. It isn't used much by normal code, actually; it's
 * main purpose is to be a basic switch engine when the MI code is
 * already dealing with spin-preemption or other gunk.  
 */
ENTRY(pthread__switch)
	pushq	%r12
	pushq	%r13
	pushq	%r14
	movq	%rdi,%r12
	movq	%rsi,%r13
	/* r12 (eax) holds the current thread	     */
	/* r13 (ecx) holds the thread to switch to */
	subq	$CONTEXTSIZE+12, %rsp	/* Allocate space for the ucontext_t */
	leaq	12(%rsp), %r14
	andq	$~(0xf), %r14		/* 16-byte-align the ucontext_t area */
	/*
	 * Note: Alignment space below the ucontext_t can consume up to
	 * 12 bytes of STACKSPACE.
	 */
	movq	%r14,%rdi
	call	PIC_PLT(_C_LABEL(_getcontext_u))
	/*
	 * Edit the context so that it continues as if returning from
	 * the _setcontext_u below.  
	 */
#ifdef PIC
	movq	PIC_GOT(pthread__switch_return_point), %r11
#else
	leaq	pthread__switch_return_point, %r11
#endif
	movq	%r11, UC_RIP(%r14)
	movq	%r14, PT_UC(%r12)
	
	STACK_SWITCH

	movq	%r11, %rdi		/* ucontext_t *ucp */
	call	PIC_PLT(_C_LABEL(_setcontext_u))
NENTRY(pthread__switch_return_point)
	/* We're back on the original stack. */
	addq	$CONTEXTSIZE+12, %rsp
	popq	%r14
	popq	%r13
	popq	%r12
	ret

/*
 * Helper switch code used by pthread__locked_switch() and 
 * pthread__upcall_switch() when they discover spinlock preemption.
 */
.globl pthread__switch_away
pthread__switch_away:
	STACK_SWITCH

	/*
	 * If we're invoked from the switch-to-next provisions of
	 * pthread__locked_switch or pthread__upcall_switch, there
	 * may be a fake spinlock-count set. If so, they will set %edx
	 * to let us know, and we decrement it once we're no longer using
	 * the old stack.
	 */ 
	cmpl	$0, %r10d
	jle pthread__switch_no_decrement
	decl	PT_SPINLOCKS(%r12)

pthread__switch_no_decrement:	
	movq	%r15,%rdi		/* ucontext_t *ucp */
	call	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED

/*
 * void	pthread__locked_switch(pthread_t self, pthread_t next, 
 *           pt_spin_t *lock);
 *
 * Switch away from a thread that is holding a lock on a queue (to
 * prevent being removed from the queue before being switched away).
 */
ENTRY(pthread__locked_switch)
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15

	movq	%rdi, %r12
	movq	%rsi, %r13
	movq	%rdx, %r14
	incl	PT_SPINLOCKS(%r13)	/* Make sure we get continued	     */
	subq	$CONTEXTSIZE+12, %rsp	/* Allocate space for the ucontext_t */
	leaq	12(%rsp), %r15
	andq	$~(0xf), %r15		/* 16-byte-align the ucontext_t area */
	/*
	 * Note: Alignment space below the ucontext_t can consume up to
	 * 12 bytes of STACKSPACE.
	 */
	movq	%r15, %rdi
	call	PIC_PLT(_C_LABEL(_getcontext_u))
	/*
	 * Edit the context so that it continues as if returning from
	 * the _setcontext_u below.  
	 */
#ifdef PIC
	movq	PIC_GOT(locked_return_point), %r11
#else
	leaq	locked_return_point, %r11
#endif
	movq	%r11, UC_RIP(%r15)
	movq	%r15, PT_UC(%r12)
	
	STACK_SWITCH
	
	/*
	 * Check if the original thread was preempted while holding
	 * its queue lock.
	 */
	cmpq	$0, PT_NEXT(%r12)
	je	locked_no_old_preempt

	/*
	 * Yes, it was. Stash the thread we were going to
	 * switch to, the lock the original thread was holding, 
	 * and go to the next thread in the chain.  
	 * Mark the fact that this was a locked switch, and so the
	 * thread does not need to be put on a run queue.
	 * Don't release the lock. It's possible that if we do so,
	 * PT_SWITCHTO will be stomped by another switch_lock and
	 * preemption.
         */
	movq	%r11, PT_SWITCHTOUC(%r12)
	movq	%r13, PT_SWITCHTO(%r12)
	movq	%r14, PT_HELDLOCK(%r12)
	decl	PT_SPINLOCKS(%r12)

	movq	PT_NEXT(%r12), %r11
	movq	%r13, %r12
	movq	%r11, %r13
	movl	$1, %r10d
	jmp	pthread__switch_away
	NOTREACHED

locked_no_old_preempt:		
	/*
	 * We've moved to the new stack, and the old context has been 
	 * saved. The queue lock can be released.
	 */
	decl	PT_SPINLOCKS(%r12)
	/* We happen to know that this is the right way to release a lock. */
	movl	$0, 0(%r14)

	decl	PT_SPINLOCKS(%r13)
	/* Check if we were preempted while holding the fake lock. */
	cmpq	$0, PT_NEXT(%r13)
	je	locked_no_new_preempt
	/* Yes, we were. Bummer. Go to the next element in the chain. */
	movq	%r11, PT_SWITCHTOUC(%r13)
	movq	%r13, PT_SWITCHTO(%r13)
	movq	%r13, %r12
	movq	PT_NEXT(%r13), %r13
	movl	$-2, %r10d
	jmp	pthread__switch_away
	NOTREACHED
	
locked_no_new_preempt:		
	movq	%r11, %rdi		/* ucontext_t *ucp */
	call	PIC_PLT(_C_LABEL(_setcontext_u))
locked_return_point:
	/* We're back on the original stack. */
	addq	$CONTEXTSIZE+12, %rsp
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	ret

	
	
/*
 * void	pthread__upcall_switch(pthread_t self, pthread_t next);
 *
 * Quit an upcall, recycle it, and jump to the selected thread.
 */
ENTRY(pthread__upcall_switch)
	movq	%rdi, %r12
	movq	%rsi, %r13
	incl	PT_SPINLOCKS(%r13)
	
	STACK_SWITCH
	
	/* Check if the upcall was preempted and continued. */
	cmpl	$0, PT_NEXT(%r12)
	je	upcall_no_old_preempt
	/*
	 * Yes, it was. Stash the thread we were going to
	 * switch to, and go to the next thread in the chain.
	 */
	movl	$PT_STATE_RECYCLABLE, PT_STATE(%r12)
	movq	%r11, PT_SWITCHTOUC(%r12)
	movq	%r13, PT_SWITCHTO(%r12)
	movq	PT_NEXT(%r12), %r10
	movq	%r13, %r12
	movq	%r10, %r13
	movl	$1, %r10d
	jmp	pthread__switch_away
	NOTREACHED
	
upcall_no_old_preempt:		
	movq	%r12,%rdi
	movq	%r11,%r14
	call    PIC_PLT(_C_LABEL(pthread__sa_recycle))
	decl	PT_SPINLOCKS(%r13)
	/* Check if we were preempted while holding the fake lock. */
	cmpq	$0, PT_NEXT(%r13)
	je	upcall_no_new_preempt
	/* Yes, we were. Bummer. Go to the next element in the chain. */
	movq	%r14, PT_SWITCHTOUC(%r13)
	movq	%r13, PT_SWITCHTO(%r13)
	movq	%r13, %r12
	movq	PT_NEXT(%r13), %r13
	movl	$-1, %r10d
	jmp	pthread__switch_away
	NOTREACHED
	
upcall_no_new_preempt:		
	movq	%r14, %rdi
	call	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED
