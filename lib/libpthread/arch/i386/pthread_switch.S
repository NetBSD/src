/* $Id: pthread_switch.S,v 1.1.2.2 2001/03/29 18:37:58 nathanw Exp $ */

/* Copyright */

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
	PIC_PROLOGUE
	movl	8(%ebp), %eax  /* eax holds the current thread */
	movl	12(%ebp), %ecx /* ecx holds the thread to switch to  */
pthread__switch_no_save:
	subl	$CONTEXTSIZE, %esp
	movl	%esp, PT_UC(%eax)
	movl	%esp, %edi

	STACK_SWITCH

	pushl	%esi
	pushl	%edi
	call	PIC_PLT(_C_LABEL(_swapcontext_u))
	popl	%edi
	popl	%esi
	movl	%edi, %esp	/* Switches back to the old stack! */
	addl	$CONTEXTSIZE, %esp
	PIC_EPILOGUE
	movl	%ebp, %esp
	popl	%ebp
	ret

/* Switch away from a thread that is holding a lock on a queue (to
 * prevent being removed from the queue before being switched away).
 */
ENTRY(pthread__locked_switch)
	pushl	%ebp
	movl	%esp, %ebp
	PIC_PROLOGUE	
	movl	8(%ebp), %eax  /* eax holds the current thread */
	movl	12(%ebp), %ecx /* ecx holds the thread to switch to  */
	movl	16(%ebp), %edx /* edx holds the pointer to the spinlock */
	incl	PT_SPINLOCKS(%ecx) /* Make sure we get continued */
	subl	$CONTEXTSIZE, %esp
	movl	%esp, PT_UC(%eax)
	movl	%esp, %edi

	pushl	%eax
	pushl	%ecx
	pushl	%edx
	pushl	%edi
	call	PIC_PLT(_C_LABEL(_getcontext_u))
	popl	%edi
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
	pushl	%esi
	call	PIC_PLT(_C_LABEL(_setcontext_u))
locked_return_point:
	/* We're back on the original stack. */
	addl	$CONTEXTSIZE+16, %esp
	PIC_EPILOGUE	
	movl	%ebp, %esp
	popl	%ebp
	ret

	
	
	
/* Quit an upcall, recycle it, and jump to the next thing. */
ENTRY(pthread__upcall_switch)
	/* Save args into registers so we can stop using the old stack. */
	pushl	%ebp
	movl	%esp, %ebp
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
