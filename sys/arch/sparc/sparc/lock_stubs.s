/*	$NetBSD: lock_stubs.s,v 1.1.2.2 2002/03/18 17:23:25 thorpej Exp $	*/

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

#include <machine/asm.h>
#include <machine/param.h>
#include <sparc/sparc/vaddrs.h>

#include "assym.h"

curproc = CPUINFO_VA + CPUINFO_CURPROC

_ENTRY(_C_LABEL(mutex_enter))
	sethi	%hi(curproc), %o3
	ld	[%o3 + %lo(curproc)], %o3	! current thread

	ldstub	[%o0], %o1			! try to acquire lock
	tst	%o1
	bnz	_C_LABEL(mutex_vector_enter)	! nope, hard case
	/*
	 * We now own the lock, but the owner field is not
	 * set.  We need to update the lock word with the
	 * our thread pointer.  We rely on the fact that the
	 * mutex code will spin while the mutex is owned
	 * "anonymously".
	 */
	sra	%o3, 4, %o1			! curproc >> 4
	sethi	%hi(0xff000000), %o2		! finish constructing
	or	%o1, %o2, %o1			!   lock word
	retl
	 st	%o1, [%o0]

_ENTRY(_C_LABEL(mutex_tryenter))
	sethi	%hi(curproc), %o3
	ld	[%o3 + %lo(curproc)], %o3	! current thread

	ldstub	[%o0], %o1			! try to acquire lock
	tst	%o1
	bnz	_C_LABEL(mutex_vector_tryenter)	! nope, hard case
	/*
	 * See comment above.
	 */
	sra	%o3, 4, %o1			! curproc >> 4
	sethi	%hi(0xff000000), %o2		! finish constructing
	or	%o1, %o2, %o1			!   lock word
	st	%o1, [%o0]
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
	sethi	%hi(curproc), %o3
	ld	[%o3 + %lo(curproc)], %o3	! current thread
	sra	%o3, 4, %o1			! curproc >> 4
	sethi	%hi(0xff000000), %o2		! finish constructing
	or	%o1, %o2, %o1			!   lock word

	/* BEGIN CRITICAL SECTION */
	ld	[%o0], %o2			! get lock word
	ldub	[%o0 + MTX_WAITERS], %o3	! get has-waiters indicator
	sub	%o1, %o2, %o4			! -> 0 if we own lock
	orcc	%o3, %o4, %g0			! has-waiters also zero?
	bz,a	1f				! yes, get out
	 st	%g0, [%o0]			!   and release lock
	/* END CRITICAL SECTION */

	b,a	_C_LABEL(mutex_vector_exit)	! no, hard case
1:	retl
	 nop

/*
 * Interlock hash table, used by the rwlock helper routines.
 */
	.section .bss
	.align	1024
	.globl	_C_LABEL(_rwlock_hash)
OTYPE(_C_LABEL(_rwlock_hash))
_C_LABEL(_rwlock_hash):
	.space	1024

	.text

/*
 * INTERLOCK_ACQUIRE expects the lock address to be in %o0.  %o0,
 * %o1, and %o2 are left alone.  %o6 must be preserved by the caller,
 * as INTERLOCK_RELEASE will use it.
 *
 * XXX Should also block interrupts, but that will only matter when
 * XXX we have kernel preemption.
 */
#define	INTERLOCK_ACQUIRE						\
	srl	%o0, 2, %o6					;	\
	and	%o6, 1023, %o6					;	\
	set	_C_LABEL(_rwlock_hash), %o3			;	\
	add	%o6, %o3, %o6					;	\
	/* %o6 == interlock address */					\
11:	ldstub	[%o6], %o3					;	\
	tst	%o3						;	\
	bz	33f						;	\
	/* spin until the lock clears */				\
22:	ldub	[%o6], %o3					;	\
	tst	%o3						;	\
	bz	11b						;	\
	 nop							;	\
	b,a	22b						;	\
33:	/* okay, have interlock! */

#define	INTERLOCK_RELEASE						\
	st	%g0, [%o6]

_ENTRY(_C_LABEL(_rwlock_cas))
	INTERLOCK_ACQUIRE			! don't touch %o6!
	ld	[%o0], %o4			! lock value
	cmp	%o1, %o4			! same as expected value?
	be,a	1f				! yes, store new value
	 st	%o2, [%o0]
	INTERLOCK_RELEASE
	retl
	 mov	%g0, %o0			! nope
1:	INTERLOCK_RELEASE
	retl
	 or	%g0, 1, %o0

_ENTRY(_C_LABEL(_rwlock_set_waiters))
	INTERLOCK_ACQUIRE			! don't touch %o6!
	ld	[%o0], %o4			! lock value
	andcc	%o4, %o1, %g0			! need_wait set?
	bz	1f				! nope, bail out
	 or	%o4, %o2, %o4			! set set_wait bits
	st	%o4, [%o0]			! store new value
1:	INTERLOCK_RELEASE
	retl
	 nop
