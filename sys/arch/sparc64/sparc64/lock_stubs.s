/*	$NetBSD: lock_stubs.s,v 1.1.2.1 2002/03/19 02:32:07 thorpej Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Assembly language lock stubs.  These handle the common ("easy")
 * cases for mutexes, and provide helper routines for rwlocks.
 */

#include "opt_multiprocessor.h"

#include <machine/asm.h>

#include "assym.h"

#undef CURPROC
#if defined(MULTIPROCESSOR)
#define	CURPROC	(CPUINFO_VA+CI_CURPROC)
#else
#define	CURPROC	_C_LABEL(curproc)
#endif

#ifdef __arch64__
#define	CASPTR	casx
#define	LDPTR	ldx
#define	STPTR	stx
#else
#define	CASPTR	cas
#define	LDPTR	ld
#define	STPTR	st
#endif /* __arch64__ */

_ENTRY(_C_LABEL(mutex_enter))
	sethi	%hi(CURPROC), %o3
	ld	[%o3 + %lo(CURPROC)], %o3	! current thread

	membar	#MemIssue
	CASPTR	[%o0], %g0, %o3			! compare-and-swap
	membar	#MemIssue
	tst	%o3				! lock was unowned?
	bnz	_C_LABEL(mutex_vector_enter)	! nope, hard case
	 nop
	retl
	 nop

_ENTRY(_C_LABEL(mutex_tryenter))
	sethi	%hi(CURPROC), %o3
	ld	[%o3 + %lo(CURPROC)], %o3	! current thread

	membar	#MemIssue
	CASPTR	[%o0], %g0, %o3			! compare-and-swap
	membar	#MemIssue
	tst	%o3				! lock was unowned?
	bnz	_C_LABEL(mutex_vector_tryenter)	! nope, hard case
	 nop
	retl
	 or	%g0, 1, %o0

/*
 * THIS IS IMPORTANT WHEN WE ADD SUPPORT FOR KERNEL PREEMPTION.
 *
 * There is a critical section within mutex_exit(); if we are
 * preempted, between checking for waiters and releasing the
 * lock, then we must check for waiters again.
 *
 * On some architectures, we could simply disable interrupts
 * (and, thus, preemption) for the short duration of this 
 * critical section.
 *      
 * On others, where disabling interrupts might be too expensive,
 * a restartable sequence could be used; in the interrupt handler,
 * if the PC is within the critical section, then then PC should
 * be reset to the beginning of the critical section so that the
 * sequence will be restarted when we are resumed.  NOTE: In this
 * case, it is very important that the insn that actually clears
 * the lock must NEVER be executed twice.
 *
 * On the sparc, we will use a restartable sequence.
 * XXX BUT WE DON'T RIGHT NOW!
 */
_ENTRY(_C_LABEL(mutex_exit))
	sethi	%hi(CURPROC), %o3
	ld	[%o3 + %lo(CURPROC)], %o3	! current thread
	mov	%g0, %o4			! new value (0)

	/* BEGIN CRITICAL SECTION */
	membar	#MemIssue
	CASPTR	[%o0], %o3, %o4			! compare-and-swap
	membar	#MemIssue
	/* END CRITICAL SECTION */

	cmp	%o3, %o4			! were they the same?
	bne	_C_LABEL(mutex_vector_exit)	! nope, hard case
	 nop
	retl
	 nop

_ENTRY(_C_LABEL(_mutex_set_waiters))
1:	membar	#MemIssue
	LDPTR	[%o0], %o3			! lock value
	tst	%o3				! lock unowned?
	bz	2f				! yup, bail
	 or	%o3, MUTEX_WAITERS, %o4		! new value
	CASPTR	[%o0], %o3, %o4			! compare-and-swap
	membar	#MemIssue
	cmp	%o4, %o3			! expected == actual?
	bne	1b				! nope, try again
	 nop
2:	retl
	 nop

_ENTRY(_C_LABEL(_rwlock_cas))
	membar	#MemIssue
	CASPTR	[%o0], %o1, %o2			! compare-and-swap
	membar	#MemIssue
	cmp	%o1, %o2			! expected == actual?
	bne	1f				! nope
	 or	%g0, 1, %o0
	retl
	 nop
1:	retl
	 mov	%g0, %o0

_ENTRY(_C_LABEL(_rwlock_set_waiters))
1:	membar	#MemIssue
	LDPTR	[%o0], %o4			! lock value
	andcc	%o4, %o1, %g0			! need_wait set?
	bz	2f				! nope, bail out
	 or	%o4, %o2, %o5			! set set_wait bits
	CASPTR	[%o0], %o4, %o5			! compare-and-swap
	membar	#MemIssue
	cmp	%o4, %o5			! expected == actual?
	bne	1b				! nope, try again
	 nop
2:	retl
	 nop
