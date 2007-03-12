/*	$NetBSD: lock_stubs.s,v 1.3.2.2 2007/03/12 05:50:43 rmind Exp $	*/

/*-
 * Copyright (c) 2002, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Andrew Doran
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
 *
 * See sparc/include/mutex.h for additional comments.
 */

#include "opt_lockdebug.h"

#include <machine/asm.h>
#include <machine/param.h>

#include <sparc/sparc/vaddrs.h>

#include <sparc/psl.h>

#include "assym.h"

curlwp = CPUINFO_VA + CPUINFO_CURLWP

/*
 * Interlock hash table, used by the R/W lock helper routines.
 */
	.section .bss
	.align	1024
	.globl	_C_LABEL(_lock_hash)
OTYPE(_C_LABEL(_lock_hash))
_C_LABEL(_lock_hash):
	.space	1024

	.text

#ifndef	LOCKDEBUG

/*
 * void	mutex_enter(kmutex_t *mtx);
 */
_ENTRY(_C_LABEL(mutex_enter))
	sethi	%hi(curlwp), %o3
	ld	[%o3 + %lo(curlwp)], %o3	! current thread
	ldstub	[%o0], %o1			! try to acquire lock
mutex_enter_crit_start:	.globl	mutex_enter_crit_start
	tst	%o1
	bnz	_C_LABEL(mutex_vector_enter)	! nope, hard case

	/*
	 * We now own the lock, but the owner field is not
	 * set.  We need to update the lock word with the
	 * our thread pointer.  We rely on the fact that the
	 * mutex code will spin or sleep while the mutex is
	 * owned "anonymously".
	 */
	sra	%o3, 5, %o1			! curlwp >> 5
	sethi	%hi(0xff000000), %o2		! finish constructing
	or	%o1, %o2, %o1			!   lock word
	st	%o1, [%o0]
mutex_enter_crit_end:	.globl	mutex_enter_crit_end
	retl
	 nop
/*
 * void	mutex_exit(kmutex_t *mtx);
 *
 * Adaptive mutexes on sparc aren't as cheap as other platforms.
 * However, since we need to test the waiters condition, in the
 * non-DIAGNOSTIC case we can just clear the owner field.
 *
 * NetBSD on sparc uses TSO (total store order), so it's safe to
 * clear the lock and then test for waiters without worrying about
 * memory ordering issues.
 */
_ENTRY(_C_LABEL(mutex_exit))
	sethi	%hi(curlwp), %o3
	ld	[%o3 + %lo(curlwp)], %o3	! current thread
	sra	%o3, 5, %o1			! curlwp >> 5
	sethi	%hi(0xff000000), %o2		! finish constructing
	or	%o1, %o2, %o1			!   lock word
	ld	[%o0], %o2			! get lock word
	cmp	%o1, %o2			! -> 0 if we own lock
	bne	_C_LABEL(mutex_vector_exit)	! no, hard case
	 nop
	ldub	[%o0 + MTX_LOCK], %o3		! get has-waiters indicator
	tst	%o3				! has waiters?
	bnz	_C_LABEL(mutex_wakeup)		! yes, hard case
	 st	%g0, [%o0]			! and release lock
	retl
	 nop

/*
 * void mutex_spin_enter(kmutex_t *);
 */
_ENTRY(_C_LABEL(mutex_spin_enter))
	sethi	%hi(CPUINFO_VA), %o4
	ld	[ %o4 + CPUINFO_MTX_COUNT ], %o5
	sub	%o5, 1, %o1
	st	%o1, [ %o4 + CPUINFO_MTX_COUNT ]
	ldub	[ %o0 + MTX_IPL ], %o2
	rd	%psr, %o1
	sll	%o2, 8, %o2
	and	%o1, PSR_PIL, %o3
	cmp	%o3, %o2
	bge	1f
	 tst	%o5
	andn	%o1, PSR_PIL, %o1
	wr	%o2, %o1, %psr
	nop
	nop
	nop
	tst	%o5
1:
	bz,a	2f
	 st	%o3, [ %o4 + CPUINFO_MTX_OLDSPL ]
2:
#if defined(MULTIPROCESSOR) || defined(DIAGNOSTIC)
	ldstub  [ %o0 + MTX_LOCK ], %o2
	tst	%o2
	bnz	_C_LABEL(mutex_spin_retry)
	 nop
#endif
	retl
	 nop

/*
 * void mutex_spin_exit(kmutex_t *);
 */
_ENTRY(_C_LABEL(mutex_spin_exit))

#if defined(DIAGNOSTIC)
	ldub	[ %o0 + MTX_LOCK ], %o1
	tst	%o1
	bz	_C_LABEL(mutex_vector_exit)
	 nop
	clrb	[ %o0 + MTX_LOCK ]
#elif defined(MULTIPROCESSOR)
	clrb	[ %o0 + MTX_LOCK ]
#endif
	sethi	 %hi(CPUINFO_VA), %o2
	ld	[ %o2 + CPUINFO_MTX_OLDSPL ], %o3
	ld	[ %o2 + CPUINFO_MTX_COUNT ], %o1
	addcc	%o1, 1, %o4
	bnz	1f
	 st	%o4, [ %o2 + CPUINFO_MTX_COUNT ]
	rd	%psr, %o1
	andn	%o1, PSR_PIL, %o1
	wr	%o3, %o1, %psr
	nop
	nop
	nop
1:
	retl
	 nop

#endif	/* LOCKDEBUG */

/*
 * int _lock_cas(uintptr_t *ptr, uintptr_t old, uintptr_t new);
 *
 * Compare-and-set operation for RW locks.
 *
 * XXX On single CPU systems, this should use a restartable sequence:
 * XXX there we don't need the overhead of interlocking.
 */
_ENTRY(_C_LABEL(_lock_cas))
	rd	%psr, %o4			! disable interrupts
	or	%o4, PSR_PIL, %o5
	wr	%o5, 0, %psr
	nop
	nop
	nop
	srl	%o0, 3, %o5
	and	%o5, 1023, %o5
	set	_C_LABEL(_lock_hash), %o3
	add	%o5, %o3, %o5
1:
	ldstub	[%o5], %o3			! %o5 == interlock address
	tst	%o3
	bz,a	2f
	 nop
	nop
	nop
	b,a	1b				! spin
	 nop
2:
	ld	[%o0], %o3			! lock value
	cmp	%o1, %o3			! same as expected value?
	be,a	3f				! yes, store new value
	 st	%o2, [%o0]
	stb	%g0, [%o5]
	wr	%o4, 0, %psr			! enable interrupts
	nop
	nop
	nop
	retl
	 mov	%g0, %o0			! nope
3:
	stb	%g0, [%o5]
	wr	%o4, 0, %psr			! enable interrupts
	nop
	nop
	nop
	retl
	 or	%g0, 1, %o0
