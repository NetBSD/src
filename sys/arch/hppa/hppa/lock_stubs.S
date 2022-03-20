/*	$NetBSD: lock_stubs.S,v 1.28 2022/03/20 20:19:34 andvar Exp $	*/

/*-
 * Copyright (c) 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran and Nick Hudson.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"

#define __MUTEX_PRIVATE

#include <machine/asm.h>
#include <machine/mutex.h>
#include <machine/reg.h>
#include <machine/cpu.h>

#include "assym.h"

/*
 * uintptr_t _lock_cas(volatile uintptr_t *ptr, uintptr_t old, uintptr_t new);
 *
 * Perform an atomic compare-and-swap operation.
 *
 * On single CPU systems this can use a restartable sequence as there
 * we don't need the overhead of interlocking.
 */


#ifndef LOCKDEBUG

	.global mutex_enter_crit_start
	.global mutex_enter_crit_end

	.import mutex_vector_enter, code
	.import mutex_vector_exit, code
	.import mutex_wakeup, code

/*
 * void mutex_exit(kmutex_t *mtx);
 *
 */

LEAF_ENTRY(mutex_exit)
	/*
	 * If it's a spin mutex or unowned, we have to take the slow path.
	 */
	ldi	MUTEX_ADAPTIVE_UNOWNED, %t1
	ldw	MTX_OWNER(%arg0),%t2
	depi	0, 27, 1, %t2			/* bit27 = 0 */
	comb,=	%t1, %t2, .Lexit_slowpath
	 nop

	/*
	 * We know that it's an adapative mutex.  Clear the owners
	 * field and release the lock.
	 */

	ldi	__SIMPLELOCK_RAW_UNLOCKED, %t2	/* unlocked = 1 */
	ldo	(MTX_LOCK + HPPA_LDCW_ALIGN - 1)(%arg0), %t3
	depi	0, 31, 4, %t3			/* bits[28-31] = 0 */
	stw	%t1, MTX_OWNER(%arg0)
	stw	%t2, 0(%t3)			/* %t3 is properly aligned */
	sync

	/*
	 * We have posted a read memory barrier so the check of mtx_waiters
	 * will happen in sequence.  If it's set then trap into mutex_wakeup()
	 * to wake up any threads sleeping on the lock.
	 */
	ldb	MTX_WAITERS(%arg0), %t4
	comib,=	0, %t4, .Lexit_done
	 nop

	ldil	L%mutex_wakeup, %t1
	ldo	R%mutex_wakeup(%t1), %t1
	.call
	bv,n	%r0(%t1)

.Lexit_slowpath:
	ldil	L%mutex_vector_exit, %t1
	ldo	R%mutex_vector_exit(%t1), %t1
	.call
	bv,n	%r0(%t1)
.Lexit_done:
	bv,n	%r0(%rp)

EXIT(mutex_exit)

/*
 * void mutex_enter(kmutex_t *mtx)
 */

LEAF_ENTRY(mutex_enter)

	/*
	 * It might be a spin lock (c.f. MUTEX_SPIN_FLAG) or might be
	 * already owned.  We short circuit the request and go straight
	 * into mutex_vector_enter() if the owners field is not 'unowned'.
	 */

	ldi	MUTEX_ADAPTIVE_UNOWNED, %t1
	ldw	MTX_OWNER(%arg0), %t2
	comb,=,n %t1, %t2, .Lmutexunowned
.Lenter_slowpath:

	ldil	L%mutex_vector_enter, %t1
	ldo	R%mutex_vector_enter(%t1), %t1
	.call
	bv,n	%r0(%t1)
	 nop

	/*
	 * We now know that it's an adaptive mutex.  Grab the spin
	 * lock, which is an atomic operation.  Once we have that,
	 * we can set the owner field.  If we can't get it, we
	 * need to go the slow path.
	 *
	 * Even if we are preempted between acquiring the lock and
	 * setting the owners field, there is no problem - noone
	 * else can acquire the mutex while the lock is held.
	 */
.Lmutexunowned:
	ldo	(MTX_LOCK + HPPA_LDCW_ALIGN - 1)(%arg0), %t1
	depi	0, 31, 4, %t1
	ldcw	0(%t1), %ret0

mutex_enter_crit_start:
	comib,=	0, %ret0, .Lenter_slowpath

	GET_CURLWP(%t2)

	bv	%r0(%rp)
mutex_enter_crit_end:
	 stw	%t2, MTX_OWNER(%arg0)
EXIT(mutex_enter)

#endif	/* !LOCKDEBUG */

#ifndef MULTIPROCESSOR

	.global _lock_cas_ras_start
	.global _lock_cas_ras_end

LEAF_ENTRY(_lock_cas)
_lock_cas_ras_start:
	ldw	0(%arg0),%t1
	comb,<>	%arg1, %t1, 1f
	 copy	%t1,%ret0
_lock_cas_ras_end:
	stw	%arg2,0(%arg0)
	copy	%arg1,%ret0
1:
	bv,n	%r0(%rp)

EXIT(_lock_cas)

STRONG_ALIAS(_atomic_cas_ulong,_lock_cas)
STRONG_ALIAS(atomic_cas_ulong,_lock_cas)
STRONG_ALIAS(_atomic_cas_32,_lock_cas)
STRONG_ALIAS(atomic_cas_32,_lock_cas)
STRONG_ALIAS(_atomic_cas_uint,_lock_cas)
STRONG_ALIAS(atomic_cas_uint,_lock_cas)
STRONG_ALIAS(_atomic_cas_ptr,_lock_cas)
STRONG_ALIAS(atomic_cas_ptr,_lock_cas)

STRONG_ALIAS(_atomic_cas_ulong_ni,_lock_cas)
STRONG_ALIAS(atomic_cas_ulong_ni,_lock_cas)
STRONG_ALIAS(_atomic_cas_32_ni,_lock_cas)
STRONG_ALIAS(atomic_cas_32_ni,_lock_cas)
STRONG_ALIAS(_atomic_cas_uint_ni,_lock_cas)
STRONG_ALIAS(atomic_cas_uint_ni,_lock_cas)
STRONG_ALIAS(_atomic_cas_ptr_ni,_lock_cas)
STRONG_ALIAS(atomic_cas_ptr_ni,_lock_cas)

#else  /* !MULTIPROCESSOR */

/*
 * uintptr_t _lock_cas(volatile uintptr_t *ptr, uintptr_t old, uintptr_t new);
 *
 * Perform an atomic compare-and-swap operation.
 *
 * On multi-CPU systems, this has to use an interlock and disable interrupts.
 * The interlock is to protect against another CPU attempting to perform the
 * cas.  Disabling interrupts is to prevent deadlocks on the current CPU.  That
 * is, we don't want an interrupts attempting to perform a cas on the interlock
 * at the same time.
 *
 */

#define IL	\
	.word	__SIMPLELOCK_RAW_UNLOCKED ! \
	.word	__SIMPLELOCK_RAW_UNLOCKED ! \
	.word	__SIMPLELOCK_RAW_UNLOCKED ! \
	.word	__SIMPLELOCK_RAW_UNLOCKED ! \

#define I8	\
	IL IL IL IL IL IL IL IL

#define I64	\
	I8 I8 I8 I8 I8 I8 I8 I8

	.section .data
	.align 4096
	.export _lock_hash, data
_lock_hash:
	I64 I64
	I64 I64
	I64 I64
	I64 I64
	I64 I64
	I64 I64
	I64 I64
	I64 I64

LEAF_ENTRY(_lock_cas)
ALTENTRY(_lock_cas_mp)

	mfctl	%eiem, %t1
	mtctl	%r0, %eiem			/* disable interrupts */

	extru	%arg0, 21+8-1, 8, %ret0
	ldil	L%_lock_hash, %r1
	zdep	%ret0, 27, 28, %ret0
	ldo	R%_lock_hash(%r1), %r1

	addl	%ret0, %r1, %ret0
	ldo	15(%ret0), %ret0
	copy	%ret0, %t3
	depi	0, 31, 4, %t3

	/* %t3 is the interlock address */
	ldcw	0(%t3), %ret0
	comib,<>,n	0, %ret0, _lock_cas_mp_interlocked
_lock_cas_mp_spin:
	 ldw	0(%t3), %ret0
	comib,= 0, %ret0, _lock_cas_mp_spin
	 nop
	ldcw	0(%t3), %ret0
	comib,= 0, %ret0, _lock_cas_mp_spin
	 nop

_lock_cas_mp_interlocked:
	ldw	0(%arg0), %ret0
	comclr,<> %arg1, %ret0, %r0	/* If *ptr != old, then nullify */
	stw	%arg2, 0(%arg0)

	sync

	ldi	__SIMPLELOCK_RAW_UNLOCKED, %t4
	stw	%t4, 0(%t3)
	bv	%r0(%r2)
	 mtctl	%t1, %eiem		/* enable interrupts */

EXIT(_lock_cas)

STRONG_ALIAS(_atomic_cas_ulong,_lock_cas_mp)
STRONG_ALIAS(atomic_cas_ulong,_lock_cas_mp)
STRONG_ALIAS(_atomic_cas_32,_lock_cas_mp)
STRONG_ALIAS(atomic_cas_32,_lock_cas_mp)
STRONG_ALIAS(_atomic_cas_uint,_lock_cas_mp)
STRONG_ALIAS(atomic_cas_uint,_lock_cas_mp)
STRONG_ALIAS(_atomic_cas_ptr,_lock_cas_mp)
STRONG_ALIAS(atomic_cas_ptr,_lock_cas_mp)

STRONG_ALIAS(_atomic_cas_ulong_ni,_lock_cas_mp)
STRONG_ALIAS(atomic_cas_ulong_ni,_lock_cas_mp)
STRONG_ALIAS(_atomic_cas_32_ni,_lock_cas_mp)
STRONG_ALIAS(atomic_cas_32_ni,_lock_cas_mp)
STRONG_ALIAS(_atomic_cas_uint_ni,_lock_cas_mp)
STRONG_ALIAS(atomic_cas_uint_ni,_lock_cas_mp)
STRONG_ALIAS(_atomic_cas_ptr_ni,_lock_cas_mp)
STRONG_ALIAS(atomic_cas_ptr_ni,_lock_cas_mp)

#endif /* MULTIPROCESSOR */
