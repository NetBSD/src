/*	$NetBSD: pthread_switch.S,v 1.1.2.1 2002/02/22 03:21:29 petrov Exp $ */


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

#define NOTREACHED	illtrap	210; \
	/* NOTREACHED */

/*
 * XXX local storage area is right below %fp
 * besides saving of return address, there is no need in assembler
 */
ENTRY(pthread__switch)
	/*
	 * Get space on stack for return address and ucontext,
	 * save return address, put ucontext address into pthread
	 * then swapcontext
	 */
	save	%sp, -STACK_ALIGN(CC64FSZ + 8 + CONTEXTSIZE), %sp
	/* XXX check ABI */
	stx	%i7, [%fp + BIAS - 8]
	add	%fp, BIAS - (CONTEXTSIZE + 8), %l0
	stx	%l0, [%i0 + PT_UC]
	mov	%l0, %o0
	call	_C_LABEL(_swapcontext_u)
	 ldx	[%i1 + PT_UC], %o1
	ldx	[%fp + BIAS - 8], %i7
	ret
	 restore



ENTRY(pthread__upcall_switch)	
	ta 5
	 nop
	retl
	 nop


ENTRY(pthread__locked_switch)
	ta 5
	 nop
	retl
	 nop

