/*	$NetBSD: lock_stubs_llsc.S,v 1.2.30.2 2015/09/22 12:05:47 skrll Exp $	*/

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

#include "assym.h"

#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR)
#define	FULL
#endif

#if defined(MULTIPROCESSOR)
#if defined(__OCTEON__)
#define	SYNC		syncw
#define	BDSYNC		syncw
#else
#define	SYNC		sync
#define	BDSYNC		sync
#endif
#else
#define	SYNC		/* nothing */
#define	BDSYNC		nop
#endif /* MIPS_HAS_LLSC != 0 && defined(MULTIPROCESSOR) */


/*
 * Set ISA level for the assembler.
 * XXX Clean up with a macro?  Same code fragment is in mipsX_subr.S too.
 * XXX Key off build abi instead of processor type?
 */
#if defined(MIPS3)
	.set	mips3
#endif

#if defined(MIPS32)
	.set	mips32
#endif

#if defined(MIPS64)
	.set	mips64
#endif

	.set	noreorder
	.set	noat

/*
 * unsigned long atomic_cas_ulong_llsc(volatile unsigned long *val,
 *     unsigned long old, unsigned long new);
 */
STATIC_LEAF(llsc_atomic_cas_ulong)
1:
	LONG_LL	t0, (a0)
	bne	t0, a1, 2f
	 move t1, a2
	LONG_SC	t1, (a0)
	beqz	t1, 1b
	 nop
	SYNC
	j	ra
	 move	v0, a1
2:
	j	ra
	 move	v0, t0
END(llsc_atomic_cas_ulong)

/*
 * unsigned int _atomic_cas_uint_llsc(volatile unsigned int *val,
 *    unsigned int old, unsigned int new);
 */
STATIC_LEAF(llsc_atomic_cas_uint)
1:
	INT_LL	t0, (a0)
	bne	t0, a1, 2f
	 move	t1, a2
	INT_SC	t1, (a0)
	beqz	t1, 1b
	 nop
	SYNC
	j	ra
	 move	v0, a1
2:
	j	ra
	 move	v0, t0
END(llsc_atomic_cas_uint)

/*
 * int llsc_ucas_uint(volatile unsigned int *ptr, unsigned int old,
 *	unsigned int new, unsigned int *ret)
 */
STATIC_LEAF(llsc_ucas_uint)
	.set at
	PTR_LA	v0, _C_LABEL(llsc_ucaserr)
	.set noat
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_S	v0, PCB_ONFAULT(v1)
	bltz	a0, _C_LABEL(llsc_ucaserr)
	 nop
	move	v0, zero

1:	INT_LL	t0, 0(a0)
	 nop
	bne	t0, a1, 2f
	 move	t1, a2
	INT_SC	t1, 0(a0)
	beqz	t1, 1b
	 nop
	SYNC

2:	PTR_S	zero, PCB_ONFAULT(v1)
	j	ra
	 INT_S	t0, 0(a3)
END(llsc_ucas_uint)

/*
 * int ucas_ulong(volatile unsigned long *ptr, unsigned long old,
 *	unsigned long new, unsigned long *ret)
 */
STATIC_LEAF(llsc_ucas_ulong)
	.set at
	PTR_LA	v0, _C_LABEL(llsc_ucaserr)
	.set noat
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_S	v0, PCB_ONFAULT(v1)
	bltz	a0, _C_LABEL(llsc_ucaserr)
	 nop
	move	v0, zero

1:	LONG_LL	t0, 0(a0)
	 nop
	bne	t0, a1, 2f
	 move	t1, a2
	LONG_SC	t1, 0(a0)
	beqz	t1, 1b
	 nop
	SYNC

2:	PTR_S	zero, PCB_ONFAULT(v1)
	j	ra
	 LONG_S	t0, 0(a3)
END(llsc_ucas_ulong)

STATIC_LEAF_NOPROFILE(llsc_ucaserr)
	PTR_S	zero, PCB_ONFAULT(v1)		# reset fault handler
	j	ra
	 li	v0, EFAULT			# return EFAULT on error
END(llsc_ucaserr)

#ifndef LOCKDEBUG

/*
 * void	mutex_enter(kmutex_t *mtx);
 */
STATIC_LEAF(llsc_mutex_enter)
	PTR_LL	t0, MTX_OWNER(a0)
	nop
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
END(llsc_mutex_enter)

/*
 * void	mutex_exit(kmutex_t *mtx);
 */
STATIC_LEAF(llsc_mutex_exit)
	PTR_LL	t0, MTX_OWNER(a0)
	SYNC
1:
	bne	t0, MIPS_CURLWP, 2f
	 move	t2, zero
	PTR_SC	t2, MTX_OWNER(a0)
	beqz	t2, 1b
	 PTR_LL	t0, MTX_OWNER(a0)
	j	ra
	 BDSYNC
2:
	j	_C_LABEL(mutex_vector_exit)
	 nop
END(llsc_mutex_exit)

/*
 * void	mutex_spin_enter(kmutex_t *mtx);
 */
STATIC_NESTED(llsc_mutex_spin_enter, CALLFRAME_SIZ, ra)
	move	t0, a0
	PTR_L	t2, L_CPU(MIPS_CURLWP)
	INT_L	a0, MTX_IPL(t0)
#ifdef PARANOIA
	INT_L	ta1, CPU_INFO_CPL(t2)
#endif

	/*
	 * We need to raise our IPL.  But it means calling another routine
	 * but it's written to have little overhead.  call splraise
	 * (only uses a0-a3 and v0-v1)
	 */
	move	t3, ra			# need to save ra
	jal	_C_LABEL(splraise)
	 nop
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
	nop
	INT_ADDU ta3, ta2, -1
	INT_S	ta3, CPU_INFO_MTX_COUNT(t2)
	bltz	ta2, 2f
	 nop
	INT_S	v0, CPU_INFO_MTX_OLDSPL(t2)	/* returned by splraise */
2:
#ifdef PARANOIA
	INT_L	ta1, CPU_INFO_MTX_OLDSPL(t2)
	INT_L	ta2, CPU_INFO_CPL(t2)	# get updated CPL
	nop
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
	j	_C_LABEL(mutex_spin_retry)
	 move	a0, t0
#else
	j	ra
	 nop
#endif /* FULL */
END(llsc_mutex_spin_enter)

/*
 * void	mutex_spin_exit(kmutex_t *mtx);
 */
LEAF(llsc_mutex_spin_exit)
	PTR_L	t2, L_CPU(MIPS_CURLWP)
#if defined(DIAGNOSTIC)
	INT_L	t0, MTX_LOCK(a0)
	nop
	beqz	t0, 2f
	 nop
	INT_S	zero, MTX_LOCK(a0)
#endif

	/*
	 * We need to grab this before the mutex count is incremented
	 * because if we get an interrupt, it may see the count as zero
	 * and overwrite the oldspl value with a bogus value.
	 */
#ifdef PARANOIA
	INT_L	a2, MTX_IPL(a0)
	nop
#endif
	INT_L	a0, CPU_INFO_MTX_OLDSPL(t2)

	/*
	 * Increment the mutex count
	 */
	INT_L	t0, CPU_INFO_MTX_COUNT(t2)
	nop
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
#ifdef PARANOIA
	sltiu	v0, a0, IPL_HIGH+1
13:	beqz	v0, 13b			# loop forever if ipl > IPL_HIGH
	 nop
#endif
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
END(llsc_mutex_spin_exit)
#endif	/* !LOCKDEBUG */

	.rdata
EXPORT(mips_llsc_locore_atomicvec)
	PTR_WORD 	llsc_atomic_cas_uint
	PTR_WORD 	llsc_atomic_cas_ulong
	PTR_WORD	llsc_ucas_uint
	PTR_WORD	llsc_ucas_ulong
#ifdef LOCKDEBUG
	PTR_WORD	mutex_vector_enter
	PTR_WORD	mutex_vector_exit
	PTR_WORD	mutex_vector_enter
	PTR_WORD	mutex_vector_exit
#else
	PTR_WORD	llsc_mutex_enter
	PTR_WORD	llsc_mutex_exit
	PTR_WORD	llsc_mutex_spin_enter
	PTR_WORD	llsc_mutex_spin_exit
#endif	/* !LOCKDEBUG */
