/*	$NetBSD: lock_stubs.S,v 1.9.18.4 2009/09/05 18:52:32 matt Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#include "opt_cputype.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"

#include <machine/asm.h>
#include <machine/cpu.h>

#include "assym.h"

#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR)
#define	FULL
#endif

#if MIPS_HAS_LLSC != 0 && defined(MULTIPROCESSOR)
#define	SYNC		sync
#define	BDSYNC		sync
#else
#define	SYNC		/* nothing */
#define	BDSYNC		nop
#endif /* MIPS_HAS_LLSC != 0 && defined(MULTIPROCESSOR) */


#if MIPS_HAS_LLSC != 0

	.set	mips3
	.set	noreorder
	.set	noat

/*
 * unsigned long _atomic_cas_ulong(volatile unsigned long *val,
 *     unsigned long old, unsigned long new);
 */
LEAF(_atomic_cas_ulong)
1:
	LONG_LL	t0, (a0)
	bne	t0, a1, 2f
	 addu	t1, zero, a2
	LONG_SC	t1, (a0)
	beq	t1, zero, 1b
	 nop
	j	ra
	 addu	v0, zero, a1
2:
	j	ra
	 addu	v0, zero, t0
END(_atomic_cas_ulong)

STRONG_ALIAS(atomic_cas_ulong,_atomic_cas_ulong)
STRONG_ALIAS(_atomic_cas_ptr,_atomic_cas_ulong)
STRONG_ALIAS(atomic_cas_ptr,_atomic_cas_ulong)

STRONG_ALIAS(_atomic_cas_ulong_ni,_atomic_cas_ulong)
STRONG_ALIAS(atomic_cas_ulong_ni,_atomic_cas_ulong)
STRONG_ALIAS(_atomic_cas_ptr_ni,_atomic_cas_ulong)
STRONG_ALIAS(atomic_cas_ptr_ni,_atomic_cas_ulong)

#if defined(_LP64)
STRONG_ALIAS(_atomic_cas_64,_atomic_cas_ulong)
STRONG_ALIAS(atomic_cas_64,_atomic_cas_ulong)

STRONG_ALIAS(_atomic_cas_64_ni,_atomic_cas_ulong)
STRONG_ALIAS(atomic_cas_64_ni,_atomic_cas_ulong)
#endif

/*
 * unsigned int _atomic_cas_uint(volatile unsigned int *val,
 *    unsigned int old, unsigned int new);
 */
LEAF(_atomic_cas_uint)
1:
	INT_LL	t0, (a0)
	bne	t0, a1, 2f
	 addu	t1, zero, a2
	INT_SC	t1, (a0)
	beq	t1, zero, 1b
	 nop
	j	ra
	 addu	v0, zero, a1
2:
	j	ra
	 addu	v0, zero, t0
END(_atomic_cas_uint)

STRONG_ALIAS(_atomic_cas_32,_atomic_cas_uint)
STRONG_ALIAS(atomic_cas_32,_atomic_cas_uint)
STRONG_ALIAS(atomic_cas_uint,_atomic_cas_uint)

STRONG_ALIAS(_atomic_cas_32_ni,_atomic_cas_uint)
STRONG_ALIAS(atomic_cas_32_ni,_atomic_cas_uint)
STRONG_ALIAS(_atomic_cas_uint_ni,_atomic_cas_uint)
STRONG_ALIAS(atomic_cas_uint_ni,_atomic_cas_uint)

#ifndef LOCKDEBUG

/*
 * void	mutex_enter(kmutex_t *mtx);
 */
LEAF(mutex_enter)
	PTR_LL	t0, MTX_OWNER(a0)
1:
	bne	t0, zero, 2f
	 addu	t2, zero, MIPS_CURLWP
	PTR_SC	t2, MTX_OWNER(a0)
	beq	t2, zero, 1b
	 PTR_LL	t0, MTX_OWNER(a0)
	j	ra
	 BDSYNC
2:
	j	_C_LABEL(mutex_vector_enter)
	 nop
END(mutex_enter)

/*
 * void	mutex_exit(kmutex_t *mtx);
 */
LEAF(mutex_exit)
	PTR_LL	t0, MTX_OWNER(a0)
	BDSYNC
1:
	bne	t0, MIPS_CURLWP, 2f
	 addu	t2, zero, zero
	PTR_SC	t2, MTX_OWNER(a0)
	beq	t2, zero, 1b
	 PTR_LL	t0, MTX_OWNER(a0)
	j	ra
	 nop
2:
	j	_C_LABEL(mutex_vector_exit)
	 nop
END(mutex_exit)

/*
 * void	mutex_spin_enter(kmutex_t *mtx);
 */
LEAF(mutex_spin_enter)
	PTR_L	t2, L_CPU(MIPS_CURLWP)
	li	ta3, -1
	INT_L	ta2, CPU_INFO_MTX_COUNT(t2)
	INT_L	t1, MTX_IPL(a0)
	addu	ta2, ta2, ta3
	INT_S	ta2, CPU_INFO_MTX_COUNT(t2)
	mfc0	v0, MIPS_COP_0_STATUS
	and	t1, t1, MIPS_INT_MASK
	nor	t1, zero, t1
	and	t1, t1, v0
	DYNAMIC_STATUS_MASK(t1,t0)
	mtc0	t1, MIPS_COP_0_STATUS
	COP0_SYNC
	bne	ta2, ta3, 1f
	 and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	INT_S	v0, CPU_INFO_MTX_OLDSPL(t2)
1:
#if defined(FULL)
	INT_LL	t0, MTX_LOCK(a0)
	nop
2:
	bne	t0, zero, 3f
	 li	t1, 1
	INT_SC	t1, MTX_LOCK(a0)
	beq	t1, zero, 2b
	 INT_LL	t0, MTX_LOCK(a0)
	j	ra
	 BDSYNC
3:
	j	_C_LABEL(mutex_spin_retry)
	 nop
#else
	j	ra
	 nop
#endif
END(mutex_spin_enter)

#endif	/* !LOCKDEBUG */

#else	/* MIPS_HAS_LLSC != 0 */

#ifdef MULTIPROCESSOR
#error MULTIPROCESSOR not supported
#endif

/*
 * Lock stubs for MIPS1 and a couple of oddball MIPS3 CPUs.  These are
 * implemented using restartable sequences, since LL/SC are not available.
 *
 * The order of the generated code is particularly important here.  Some
 * assumptions:
 *
 * o All of the critical sections are 20 bytes in size, and the entry
 *   to each critical section is aligned on a 16 byte boundary (see
 *   top of _lock_ras() for why).  The entry is defined here as the
 *   point after where a restart occurs if we trap within the section.
 *
 * o The entire code block is aligned on a 128 byte boundary, and is
 *   128 bytes in size.  This is to allow us to do an pessimistic check
 *   after taking a trap with:
 *
 *	if ((addr & ~127) == _lock_ras_start)
 *		addr = _lock_ras(addr);
 *
 *   See definition of MIPS_LOCK_RAS_SIZE in asm.h.
 *
 * o In order to keep the size of the block down, the routines are run
 *   into each other.  Use objdump -d to check alignment after making
 *   changes.
 */
	.set	mips1
	.set	noreorder
	.set	noat

/*
 * unsigned long _atomic_cas_ulong(volatile unsigned long *val,
 *     unsigned long old, unsigned long new);
 */
	.text
	.align	7

EXPORT(_lock_ras_start)

	.space	12

LEAF_NOPROFILE(_atomic_cas_ulong)
	PTR_L	t0, (a0)	/* <- critical section start */
_atomic_cas_start:
	 nop
	bne	t0, a1, 1f
 	 nop
	PTR_S	a2, (a0)	/* <- critical section end */
	j	ra
	 addu	v0, zero, a1
1:
	j	ra
	 addu	v0, zero, t0
END(_atomic_cas_ulong)

STRONG_ALIAS(atomic_cas_ulong,_atomic_cas_ulong)
STRONG_ALIAS(_atomic_cas_32,_atomic_cas_ulong)
STRONG_ALIAS(atomic_cas_32,_atomic_cas_ulong)
STRONG_ALIAS(_atomic_cas_uint,_atomic_cas_ulong)
STRONG_ALIAS(atomic_cas_uint,_atomic_cas_ulong)
STRONG_ALIAS(_atomic_cas_ptr,_atomic_cas_ulong)
STRONG_ALIAS(atomic_cas_ptr,_atomic_cas_ulong)

STRONG_ALIAS(_atomic_cas_ulong_ni,_atomic_cas_ulong)
STRONG_ALIAS(atomic_cas_ulong_ni,_atomic_cas_ulong)
STRONG_ALIAS(_atomic_cas_32_ni,_atomic_cas_ulong)
STRONG_ALIAS(atomic_cas_32_ni,_atomic_cas_ulong)
STRONG_ALIAS(_atomic_cas_uint_ni,_atomic_cas_ulong)
STRONG_ALIAS(atomic_cas_uint_ni,_atomic_cas_ulong)
STRONG_ALIAS(_atomic_cas_ptr_ni,_atomic_cas_ulong)
STRONG_ALIAS(atomic_cas_ptr_ni,_atomic_cas_ulong)

#ifndef LOCKDEBUG
/*
 * int mutex_enter(kmutex_t *mtx);
 */
	nop
	nop
	nop
LEAF_NOPROFILE(mutex_enter)
	PTR_L	t0, (a0)	/* <- critical section start */
_mutex_enter_start:
	nop
	bne	t0, zero, 1f
	 nop			
	PTR_S	MIPS_CURLWP, (a0)/* <- critical section end */
	j	ra
	 nop
1:	j	_C_LABEL(mutex_vector_enter)
	 nop
END(mutex_enter)

/*
 * int mutex_exit(kmutex_t *mtx);
 */
	nop
	nop
	nop
LEAF_NOPROFILE(mutex_exit)
	PTR_L	t0, (a0)	/* <- critical section start */
_mutex_exit_start:
	 nop
	bne	t0, MIPS_CURLWP, 1f
	 nop			
	PTR_S	zero, (a0)	/* <- critical section end */
	j	ra
	 nop
1:	j	_C_LABEL(mutex_vector_exit)
END(mutex_exit)
	 nop

	.align	7			/* Get out of the RAS block */

/*
 * void	mutex_spin_enter(kmutex_t *mtx);
 */
LEAF(mutex_spin_enter)
	PTR_L	t2, L_CPU(MIPS_CURLWP)
	li	ta3, -1
	INT_L	ta2, CPU_INFO_MTX_COUNT(t2)
	INT_L	t1, MTX_IPL(a0)
	addu	ta2, ta2, ta3
	INT_S	ta2, CPU_INFO_MTX_COUNT(t2)
	mfc0	v0, MIPS_COP_0_STATUS
	and	t1, t1, MIPS_INT_MASK
	nor	t1, zero, t1
	and	t1, t1, v0
	DYNAMIC_STATUS_MASK(t1,t0)
	mtc0	t1, MIPS_COP_0_STATUS
	COP0_SYNC
	bne	ta2, ta3, 1f
	 and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	INT_S	v0, CPU_INFO_MTX_OLDSPL(t2)
1:
#if defined(DIAGNOSTIC)
	INT_L	t0, MTX_LOCK(a0)
	li	t1, 1
	bne	t0, zero, 2f
	 nop
	j	ra
	 INT_S	t1, MTX_LOCK(a0)
2:
	j	_C_LABEL(mutex_spin_retry)
	 nop
#else	/* DIAGNOSTIC */
	j	ra
	 nop
#endif	/* DIAGNOSTIC */
END(mutex_spin_enter)

#endif	/* !LOCKDEBUG */

/*
 * Patch up the given address.  We arrive here if we might have trapped
 * within one of the critical sections above.  Do:
 *
 *	if ((addr & ~15) == ras)
 *		return ras - 4;
 *	... check next ...
 *	return addr;
 *
 * Registers on entry:
 *
 *	k1	fault PC
 *	ra	return address
 *
 * On exit:
 *
 *	k1	adjusted fault PC
 *	ra	return address
 *	t0	clobbered
 *	t1	clobbered
 *	t2	clobbered
 */

LEAF_NOPROFILE(_lock_ras)
	li	t1, -16
	and	t2, k1, t1
	la	t0, _atomic_cas_start
	beq	t2, t0, 1f
	 nop
#ifndef LOCKDEBUG
	addiu	t0, _mutex_enter_start - _atomic_cas_start
	beq	t2, t0, 1f
	 nop
	addiu	t0, _mutex_exit_start - _mutex_enter_start
	beq	t2, t0, 1f
	 nop
#endif	/* !LOCKDEBUG */
	j	ra
	 nop
1:
	j	ra
	 addiu	k1, t0, -4
END(_lock_ras)

#endif	/* MIPS_HAS_LLSC == 0 */

#ifndef LOCKDEBUG

/*
 * void	mutex_spin_exit(kmutex_t *mtx);
 */
LEAF(mutex_spin_exit)
#if defined(DIAGNOSTIC)
	INT_L	t0, MTX_LOCK(a0)
	SYNC
	beq	t0, zero, 2f
	 PTR_L	t1, L_CPU(MIPS_CURLWP)
	INT_S	zero, MTX_LOCK(a0)
#elif defined(MULTIPROCESSOR)
	SYNC
	PTR_L	t1, L_CPU(MIPS_CURLWP)
	INT_S	zero, MTX_LOCK(a0)
#else
	PTR_L	t1, L_CPU(MIPS_CURLWP)
	nop
#endif
	INT_L	t0, CPU_INFO_MTX_COUNT(t1)
	INT_L	a0, CPU_INFO_MTX_OLDSPL(t1)
	addiu	t0, t0, 1
	INT_S	t0, CPU_INFO_MTX_COUNT(t1)
	bne	t0, zero, 1f
	 mfc0	v0, MIPS_COP_0_STATUS
	and	a0, a0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	li	v1, ~(MIPS_INT_MASK | MIPS_SR_INT_IE)
	and	v1, v1, v0
	or	v1, v1, a0
	DYNAMIC_STATUS_MASK(v1,t0)
	mtc0	v1, MIPS_COP_0_STATUS
	COP0_SYNC
1:
	j	ra
	 nop
#if defined(DIAGNOSTIC)
2:
	j	_C_LABEL(mutex_vector_exit)
	 nop
#endif
END(mutex_spin_exit)

#endif	/* !LOCKDEBUG */
