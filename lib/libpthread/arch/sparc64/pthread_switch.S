/*	$NetBSD: pthread_switch.S,v 1.1.2.3 2002/05/21 01:24:10 petrov Exp $ */


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

#if 0
#define DBP(n)		\
	or	%g0,n,%o3	; \
	mov	%i0,%o0		; \
	mov	%i1,%o1		; \
	call	_C_LABEL(pthread__md_debug); \
	 mov	%i2,%o2

#define DBR(n,a0,a1,a2)		\
	or	%g0,n,%o3	; \
	mov	a0,%o0		; \
	mov	a1,%o1		; \
	call	_C_LABEL(pthread__md_debug); \
	 mov	a2,%o2

#define DBSNAP(x)		\
	call _C_LABEL(pthread__md_snapshot); \
	 mov	x,%o0

#else
#define DBP(n)
#define DBR(n,a0,a1,a2)
#define DBSNAP(x)
#endif

ENTRY(pthread__switch)
	/*
	 * XXX Get space on stack for return address and ucontext,
	 * save return address, put ucontext address into pthread
	 * then swapcontext
	 */
	save	%sp, -STACK_ALIGN(CC64FSZ + CONTEXTSIZE), %sp

DBP(1)

	add	%fp, BIAS - CONTEXTSIZE, %l0
	stx	%l0, [%i0 + PT_UC]
	ldx	[%i1 + PT_UC], %o1
	call	_C_LABEL(_swapcontext_u)
	 mov	%l0, %o0

	ret
	 restore


/* 		*** WARNING ***
 * STACK_SWITCH is more subtle than it looks. Please go read the extended
 * comment in the i386 pthread_switch.S file.
 */


/*
 * pt register which contains *pthread, which stack will be used
 * tmp temporary register which will have *ucontext_t for that pthread
 */
#define STACK_SWITCH(pt,tmp)		  \
	ldx	[pt + PT_UC], tmp		; \
	add	tmp, - STACK_ALIGN(STACKSPACE + CONTEXTSIZE) - BIAS, %sp


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

DBP(2)

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

DBP(3)
	/*
	 * switch to next thread stack so we could recycle self's one
	 */
	STACK_SWITCH(%i1,%l2)

DBP(4)

	/* Check if the upcall code was preempted and continued to here. */
	ldx	[%i0 + PT_NEXT], %l0
	brz,a	%l0, 1f
	 nop

	/* Yes, it was. Stash the thread we were going to switch to,
	 * and switch to the next thread in the continuation chain.
	 */
	stx	%i1, [%i0 + PT_SWITCHTO]
	stx	%l2, [%i0 + PT_SWITCHTOUC]
	or	%g0, PT_STATE_RECYCLABLE, %l1
	st	%l1, [%i0 + PT_STATE]
	mov	%i1, %i0
	mov	%l0, %i1
	mov	1, %i2
	ba	pthread__switch_away
	 nop
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
	stx	%i1, [%i1 + PT_SWITCHTO]
	stx	%l2, [%i1 + PT_SWITCHTOUC]
	mov	%i1, %i0
	mov	%l0, %i1
	mov	0, %i2
	ba	pthread__switch_away
	 nop

	/* No new-upcall-preemption */
2:
	call	_C_LABEL(_setcontext_u)
	 ldx	[%i1 + PT_UC], %o0
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

DBR(5,%i0,%i1,%fp)

	/* Make sure we get continuted */
	ld	[%i1 + PT_SPINLOCKS], %l0
	inc	%l0
	st	%l0, [%i1 + PT_SPINLOCKS]

	/* Get the current context */
	add	%fp, BIAS - CONTEXTSIZE, %l3
	stx	%l3, [%i0 + PT_UC]
	call	_C_LABEL(_getcontext_u)
	 mov %l3, %o0

	/* Edit the context to make it continue below, rather than here */
	set _C_LABEL(locked_return_point), %l1
	stx %l1, [%l3 + UC_GREGS + _REG_PC * 8]

DBR(6,%i0,%sp,%l3)
DBSNAP(%i0)
DBSNAP(%i1)

	STACK_SWITCH(%i1, %l2)

DBR(61,%i0, %sp, %l2)

DBSNAP(%i1)

	/* Check if the switcher was preempted and continued to here. */
	ldx	[%i0 + PT_NEXT], %l0
	brz,a	%l0, 1f
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
	stx	%i1, [%i0 + PT_SWITCHTO]
	stx	%l2, [%i0 + PT_SWITCHTOUC]
	stx	%i2, [%i0 + PT_HELDLOCK]
	ld	[%i0 + PT_SPINLOCKS], %l0
	dec	%l0
	st	%l0, [%i0 + PT_SPINLOCKS]

	/* Save the context we previously stored in PT_UC(a0); 
	 * that was overwritten when we were preempted and continued,
	 * so we need to put it somewhere. 
	 */
	stx	%l3, [%i0 + PT_SLEEPUC]

	mov	%i1, %i0
	mov	%l0, %i1
	mov	1, %i2
	ba	pthread__switch_away
	 nop
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
	stx	%i1, [%i1 + PT_SWITCHTO]
	stx	%l2, [%i1 + PT_SWITCHTOUC]
	mov	%i1, %i0
	mov	%l0, %i1
	mov	%g0, %i2
	ba	pthread__switch_away
	 nop
	NOTREACHED

2:	/* %l2 points to next thread's ucontext */
#if 0
DBSNAP(%i1)
#endif
DBR(7,%l2,%fp,%i0)
DBSNAP(%i0)
DBSNAP(%i1)

	call	_C_LABEL(_setcontext_u)
	 mov	 %l2, %o0
	NOTREACHED

locked_return_point:
DBR(8,%i0,%o7,%fp)
	ret
	 restore


ENTRY(__caller_pc)
	retl
	 mov	%i7, %o0

ENTRY(__get_fp)
	retl
	 mov %fp, %o0
