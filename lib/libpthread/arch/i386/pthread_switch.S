/*	$NetBSD: pthread_switch.S,v 1.1.2.5 2001/07/20 19:20:40 nathanw Exp $	*/

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

#include <machine/asm.h>
#include "assym.h"

#define STACK_SWITCH			  \
	movl	PT_UC(%ecx),%esi	; \
	movl	%esi, %esp		; \
	subl	$CONTEXTSIZE, %esp /* XXX dodge kernel-placed ucontext */

#define	NOTREACHED			  \
	/* force error */		  \
	int3

/* Simple versions that don't handle spin-preempt cases */

/* Plain switch that doesn't do any special checking. */
ENTRY(pthread__switch)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi	/* callee save */
	PIC_PROLOGUE
	movl	8(%ebp), %eax  /* eax holds the current thread */
	movl	12(%ebp), %ecx /* ecx holds the thread to switch to  */
	movl	$0, %edx       /* edx indicates whether eax's lockcount
				* should be decremented once its stack
				* isn't current.
				*/
pthread__switch_no_save:
	subl	$CONTEXTSIZE, %esp
	movl	%esp, PT_UC(%eax)
	movl	%esp, %edi

	STACK_SWITCH

	/* If we're invoked from the switch-to-next provisions of
	 * pthread__locked_switch or pthread__upcall_switch, there
	 * may be a fake spinlock present. If so, they will set %edx
	 * to let us know, and we decrement it once we're no longer using
	 * the old stack.
	 */ 
	cmpl	$0, %edx
	je pthread__switch_no_decrement
	decl	PT_SPINLOCKS(%eax)

pthread__switch_no_decrement:	
	pushl	%esi	/* ucontext_t *ucp */
	pushl	%edi	/* ucontext_t *oucp */
	call	PIC_PLT(_C_LABEL(_swapcontext_u))
	addl	$8, %esp
	movl	%edi, %esp	/* Switches back to the old stack! */
	addl	$CONTEXTSIZE, %esp
	PIC_EPILOGUE
	popl	%edi
	movl	%ebp, %esp
	popl	%ebp
	ret

/* Switch away from a thread that is holding a lock on a queue (to
 * prevent being removed from the queue before being switched away).
 */
ENTRY(pthread__locked_switch)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi	/* callee save */
	pushl	%edi	/* callee save */
	PIC_PROLOGUE	
	movl	8(%ebp), %eax  /* eax holds the current thread */
	movl	12(%ebp), %ecx /* ecx holds the thread to switch to  */
	movl	16(%ebp), %edx /* edx holds the pointer to the spinlock */
	incl	PT_SPINLOCKS(%ecx) /* Make sure we get continued */
	subl	$CONTEXTSIZE, %esp
	movl	%esp, PT_UC(%eax)
	movl	%esp, %edi

	pushl	%eax	/* caller save */
	pushl	%ecx	/* caller save */
	pushl	%edx	/* caller save */
	pushl	%edi	/* ucontext_t *ucp */
	call	PIC_PLT(_C_LABEL(_getcontext_u))
	addl	$4, %esp
	popl	%edx
	popl	%ecx	
	popl	%eax
	/* Major-league cheating. Edit the context so that it continues
	 * as if returning from the _setcontext_u below.
	 */
#ifdef PIC
	movl	PIC_GOT(locked_return_point), %esi
#else
	leal	locked_return_point, %esi
#endif
	movl	%esi, 92(%edi)
	
	STACK_SWITCH
	
	/* Check if the original thread was preempted while holding
	 * its queue lock.
	 */
	cmpl	$0, PT_NEXT(%eax)
	je	locked_no_old_preempt

	/* Yes, it was. Stash the thread we were going to
	 * switch to, the lock the original thread was holding, 
	 * and go to the next thread in the chain.  
	 * Mark the fact that this was a locked switch, and so the
	 * thread does not need to be put on a run queue.
	 * Don't release the lock. It's possible that if we do so,
	 * PT_SWITCHTO will be stomped by another switch_lock and
	 * preemption.
         */
	movl	%ecx, PT_SWITCHTO(%eax)
	movl	%esi, PT_SWITCHTOUC(%eax)
	movl	%edx, PT_HELDLOCK(%eax)
	decl	PT_SPINLOCKS(%eax)
	
	movl	PT_NEXT(%eax), %edx
	movl	%ecx, %eax
	movl	%edx, %ecx
	movl	$1, %edx
	jmp	pthread__switch_no_save
	NOTREACHED
	
locked_no_old_preempt:		
	/* We've moved to the new stack, and the old context has been 
	 * saved. The queue lock can be released. */
	decl	PT_SPINLOCKS(%eax)
	/* We happen to know that this is the right way to release a lock. */
	movl	$0, 0(%edx)

	decl	PT_SPINLOCKS(%ecx)
	/* Check if we were preempted while holding the fake lock. */
	cmpl	$0, PT_NEXT(%ecx)
	je	locked_no_new_preempt
	/* Yes, we were. Bummer. Go to the next element in the chain. */
	movl	PT_NEXT(%ecx), %edx
	movl	%ecx, %eax
	movl	%edx, %ecx
	movl	$0, %edx
	jmp	pthread__switch_no_save
	NOTREACHED
	
locked_no_new_preempt:		
	pushl	%esi	/* ucontext_t *ucp */
	call	PIC_PLT(_C_LABEL(_setcontext_u))
locked_return_point:
	/* We're back on the original stack. */
	addl	$CONTEXTSIZE+16, %esp
	PIC_EPILOGUE
	popl	%edi
	popl	%esi
	movl	%ebp, %esp
	popl	%ebp
	ret

	
	
	
/* Quit an upcall, recycle it, and jump to the next thing. */
ENTRY(pthread__upcall_switch)
	/* Save args into registers so we can stop using the old stack. */
	pushl	%ebp
	movl	%esp, %ebp
	/* No need to save %esi, %edi;  this function never returns */
	PIC_PROLOGUE
	movl	8(%ebp), %eax  /* eax holds the upcall thread */
	movl	12(%ebp), %ecx /* ecx holds the thread to switch to  */
	incl	PT_SPINLOCKS(%ecx)
	
	STACK_SWITCH
	
	/* Check if the upcall was preempted and continued. */
	cmpl	$0, PT_NEXT(%eax)
	je	upcall_no_old_preempt
	/* Yes, it was. Stash the thread we were going to
	 * switch to, and go to the next thread in the chain.
	 */
	movl	%ecx, PT_SWITCHTO(%eax)
	movl	%esi, PT_SWITCHTOUC(%eax)
	movl	$PT_STATE_RECYCLABLE, PT_STATE(%eax)
	movl	PT_NEXT(%eax), %edx
	movl	%ecx, %eax
	movl	%edx, %ecx
	movl	$1, %edx
	jmp	pthread__switch_no_save
	NOTREACHED
	
upcall_no_old_preempt:		
	pushl   %ecx
	pushl   %eax
	call    PIC_PLT(_C_LABEL(pthread__sa_recycle))
	popl	%eax
	popl    %ecx
	decl	PT_SPINLOCKS(%ecx)
	/* Check if we were preempted while holding the fake lock. */
	cmpl	$0, PT_NEXT(%ecx)
	je	upcall_no_new_preempt
	/* Yes, we were. Bummer. Go to the next element in the chain. */
	movl	PT_NEXT(%ecx), %edx
	movl	%ecx, %eax
	movl	%edx, %ecx
	movl	$0, %edx
	jmp	pthread__switch_no_save
	NOTREACHED
	
upcall_no_new_preempt:		
	pushl	%esi
	call	PIC_PLT(_C_LABEL(_setcontext_u))
	NOTREACHED
