/*	$NetBSD: lock_stubs.S,v 1.9.18.10 2010/02/28 03:28:54 matt Exp $	*/

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

#include <sys/errno.h>

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
	 move t1, a2
	LONG_SC	t1, (a0)
	beqz	t1, 1b
	 nop
	j	ra
	 move	v0, a1
2:
	j	ra
	 move	v0, t0
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
	 move	t1, a2
	INT_SC	t1, (a0)
	beqz	t1, 1b
	 nop
	j	ra
	 move	v0, a1
2:
	j	ra
	 move	v0, t0
END(_atomic_cas_uint)

STRONG_ALIAS(_atomic_cas_32,_atomic_cas_uint)
STRONG_ALIAS(atomic_cas_32,_atomic_cas_uint)
STRONG_ALIAS(atomic_cas_uint,_atomic_cas_uint)

STRONG_ALIAS(_atomic_cas_32_ni,_atomic_cas_uint)
STRONG_ALIAS(atomic_cas_32_ni,_atomic_cas_uint)
STRONG_ALIAS(_atomic_cas_uint_ni,_atomic_cas_uint)
STRONG_ALIAS(atomic_cas_uint_ni,_atomic_cas_uint)

/*
 * int ucas_32(volatile uint32_t *ptr, uint32_t old, uint32_t new, uint32_t *ret)
 */
LEAF(ucas_32)
	.set at
	PTR_LA	v0, _C_LABEL(ucaserr)
	.set noat
	PTR_L	v1, L_ADDR(MIPS_CURLWP)
	PTR_S	v0, U_PCB_ONFAULT(v1)
	bltz	a0, _C_LABEL(ucaserr)
	 nop
	move	v0, zero

1:	INT_LL	t0, 0(a0)
	 nop
	bne	t0, a1, 2f
	 move	t1, a2
	INT_SC	t1, 0(a0)
	beqz	t1, 1b
	 nop

2:	PTR_S	zero, U_PCB_ONFAULT(v1)
	j	ra
	 INT_S	t0, 0(a3)
END(ucas_32)

STRONG_ALIAS(ucas_int, ucas_32)
STRONG_ALIAS(ucas_uint, ucas_32)
#ifndef _LP64
STRONG_ALIAS(ucas_long, ucas_32)
STRONG_ALIAS(ucas_ulong, ucas_32)
STRONG_ALIAS(ucas_ptr, ucas_32)
#endif

#ifndef __mips_o32
/*
 * int ucas_64(volatile uint64_t *ptr, uint64_t old, uint64_t new, uint64_t *ret)
 */
LEAF(ucas_64)
	.set at
	PTR_LA	v0, _C_LABEL(ucaserr)
	.set noat
	PTR_L	v1, L_ADDR(MIPS_CURLWP)
	PTR_S	v0, U_PCB_ONFAULT(v1)
	bltz	a0, _C_LABEL(ucaserr)
	 nop
	move	v0, zero

1:	REG_LL	t0, 0(a0)
	 nop
	bne	t0, a1, 2f
	 move	t1, a2
	REG_SC	t1, 0(a0)
	beqz	t1, 1b
	 nop

2:	PTR_S	zero, U_PCB_ONFAULT(v1)
	j	ra
	 REG_S	t0, 0(a3)
END(ucas_64)
#ifdef _LP64
STRONG_ALIAS(ucas_ptr, ucas_64)
STRONG_ALIAS(ucas_long, ucas_64)
STRONG_ALIAS(ucas_ulong, ucas_64)
#endif

#endif /* !__mips_o32 */

#ifndef LOCKDEBUG

/*
 * void	mutex_enter(kmutex_t *mtx);
 */
LEAF(mutex_enter)
	PTR_LL	t0, MTX_OWNER(a0)
1:
	bnez	t0, 2f
	 move	t2, MIPS_CURLWP
	PTR_SC	t2, MTX_OWNER(a0)
	beqz	t2, 1b
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
	 move	t2, zero
	PTR_SC	t2, MTX_OWNER(a0)
	beqz	t2, 1b
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
NESTED(mutex_spin_enter, CALLFRAME_SIZ, ra)
	move	t0, a0
	PTR_L	t2, L_CPU(MIPS_CURLWP)
	INT_L	ta1, CPU_INFO_CPL(t2)
	INT_L	ta0, MTX_IPL(t0)

	/*
	 * If the current IPL is less than the mutex's IPL, we need to raise
	 * our IPL to the mutex'es 
	 */
	sltu	v1, ta1, ta0		# v1 = ta1 < ta0
	beqz	v1, 1f
	 nop
	 

	/*
	 * We need to raise our IPL.  But it means calling another routine
	 * but it's written to have little overhead.  call splraise
	 * (only uses a0-a3 and v0-v1)
	 */
	move	t3, ra			# need to save ra
	jal	_C_LABEL(splraise)
	 move	a0, ta0
	move	ra, t3			# move ra back
#ifdef PARANOIA
10:	bne	ta1, v0, 10b		# loop forever if v0 != ta1
	 nop
#endif /* PARANOIA */

	/*
	 * If this is the first lock of the mutex, store the previous IPL for
	 * exit.  Even if an interrupt happens, the mutex count will not change.
	 */
1:
	INT_L	ta2, CPU_INFO_MTX_COUNT(t2)
	INT_ADDU ta3, ta2, -1
	INT_S	ta3, CPU_INFO_MTX_COUNT(t2)
	bltz	ta2, 2f
	 nop
	INT_S	ta1, CPU_INFO_MTX_OLDSPL(t2)
2:
#ifdef PARANOIA
	INT_L	ta1, CPU_INFO_MTX_OLDSPL(t2)
	INT_L	ta2, CPU_INFO_CPL(t2)	# get updated CPL
	sltu	v0, ta2, ta0		# v0 = cpl < mtx_ipl
	sltu	v1, ta2, ta1		# v1 = cpl < oldspl
	sll	v0, 1
	or	v0, v1
12:	bnez	v0, 12b			# loop forever if any are true
	 nop
#endif /* PARANOIA */
#if defined(FULL)
	INT_LL	t3, MTX_LOCK(t0)
	nop
3:
	bnez	t3, 4f
	 li	t1, 1
	INT_SC	t1, MTX_LOCK(t0)
	beqz	t1, 3b
	 INT_LL	t3, MTX_LOCK(t0)
	j	ra
	 BDSYNC
4:
	move	a0, t0
	j	_C_LABEL(mutex_spin_retry)
	 nop
#else
	j	ra
	 nop
#endif /* FULL */
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
	.p2align 7

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
	 move	v0, a1
1:
	j	ra
	 move	v0, t0
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

	.p2align 5
	nop
	nop
	nop
LEAF_NOPROFILE(_ucas_32_ras)
	PTR_L	t0, (a0)	/* <- critical section start */
_ucas_32_ras_start:
	 nop
	bne	t0, a1, 1f
 	 nop
	PTR_S	a2, (a0)	/* <- critical section end */
1:
	PTR_S	zero, U_PCB_ONFAULT(v1)
	j	ra
	 INT_S	t0, 0(a3)
END(_ucas_32_ras)


#ifndef LOCKDEBUG
	.p2align 5
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
	bnez	t0, 1f
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
	.p2align 5
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

	.p2align 7			/* Get out of the RAS block */

/*
 * void	mutex_spin_enter(kmutex_t *mtx);
 */
NESTED(mutex_spin_enter, CALLFRAME_SIZ, ra)
	move	t0, a0
	PTR_L	t2, L_CPU(MIPS_CURLWP)
	INT_L	a0, MTX_IPL(t0)
	INT_L	ta1, CPU_INFO_CPL(t2)		# get current cpl

	/*
	 * If the current IPL is less than the mutex's IPL, we need to raise
	 * our IPL to the mutex'es 
	 */
	sltu	a3, ta1, a0			# a3 = ta1 < a0
	beqz	a3, 1f
	 nop

	/*
	 * We need to raise our IPL.
	 * call splraise (only uses a0-a3, v0-v1, and ra)
	 */
	move	t3, ra
	jal	_C_LABEL(splraise)
	 nop
	move	ra, t3

	/*
	 * If this is the first lock of the mutex, store the previous IPL
	 * for exit.
	 */
1:
	INT_L	ta2, CPU_INFO_MTX_COUNT(t2)
	INT_ADDU ta3, ta2, -1
	INT_S	ta2, CPU_INFO_MTX_COUNT(t2)

	bnez	ta2, 2f
	 nop
	INT_S	t1, CPU_INFO_MTX_OLDSPL(t2)
2:
#if defined(DIAGNOSTIC)
	INT_L	t3, MTX_LOCK(t0)
	li	t1, 1
	bnez	t3, 3f
	 nop
	j	ra
	 INT_S	t1, MTX_LOCK(t0)
3:
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
	addiu	t0, _ucas_32_ras_start - _atomic_cas_start
	beq	t2, t0, 1f
	 nop
#ifndef LOCKDEBUG
	addiu	t0, _mutex_enter_start - _ucas_32_ras_start
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

/*
 * int ucas_32(volatile uint32_t *ptr, uint32_t old, uint32_t new, uint32_t *ret)
 */
LEAF(ucas_32)
	PTR_L	v1, L_ADDR(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(ucaserr)
	PTR_S	v0, U_PCB_ONFAULT(v1)
	bltz	a0, _C_LABEL(ucaserr)
	 nop
	b	_C_LABEL(_ucas_32_ras)
	 move	v0, zero
END(ucas_32)

STRONG_ALIAS(ucas_int, ucas_32)
STRONG_ALIAS(ucas_uint, ucas_32)
STRONG_ALIAS(ucas_long, ucas_32)
STRONG_ALIAS(ucas_ulong, ucas_32)
STRONG_ALIAS(ucas_ptr, ucas_32)
#endif	/* MIPS_HAS_LLSC == 0 */

/*
 *
 */
LEAF(ucaserr)
	PTR_S	zero, U_PCB_ONFAULT(v1)		# reset fault handler
	j	ra
	 li	v0, EFAULT			# return EFAULT on error
END(ucaserr)

#ifndef LOCKDEBUG

/*
 * void	mutex_spin_exit(kmutex_t *mtx);
 */
LEAF(mutex_spin_exit)
	PTR_L	t2, L_CPU(MIPS_CURLWP)
#if defined(DIAGNOSTIC)
	INT_L	t0, MTX_LOCK(a0)
	SYNC
	beqz	t0, 2f
	 nop
	INT_S	zero, MTX_LOCK(a0)
#elif defined(MULTIPROCESSOR)
	SYNC
	INT_S	zero, MTX_LOCK(a0)
#endif

	/*
	 * We need to grab this before the mutex count is incremented
	 * because if we get an interrupt, it may see the count as zero
	 * and overwrite the oldspl value with a bogus value.
	 */
#ifdef PARANOIA
	INT_L	a2, MTX_IPL(a0)
#endif
	INT_L	a0, CPU_INFO_MTX_OLDSPL(t2)

	/*
	 * Increment the mutex count
	 */
	INT_L	t0, CPU_INFO_MTX_COUNT(t2)
	INT_ADDU t0, t0, 1
	INT_S	t0, CPU_INFO_MTX_COUNT(t2)

	/*
	 * If the IPL doesn't change, nothing to do
	 */
	INT_L	a1, CPU_INFO_CPL(t2)

#ifdef PARANOIA
	sltu	v0, a1, a2		# v0 = cpl < mtx_ipl
	sltu	v1, a1, a0		# v1 = cpl < oldspl
	sll	v0, 1
	or	v0, v1
12:	bnez	v0, 12b			# loop forever if either is true
	 nop
#endif /* PARANOIA */

	beq	a0, a1, 1f		# if oldspl == cpl
	 nop				#   no reason to drop ipl

	bltz	t0, 1f			# there are still holders
	 nop				# so don't drop IPL

	/*
	 * Mutex count is zero so we need to restore the old IPL
	 */
	j	 _C_LABEL(splx)
	 nop
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
