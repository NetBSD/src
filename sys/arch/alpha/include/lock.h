/* $NetBSD: lock.h,v 1.32 2022/02/12 17:17:53 riastradh Exp $ */

/*-
 * Copyright (c) 1998, 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*
 * Machine-dependent spin lock operations.
 */

#ifndef _ALPHA_LOCK_H_
#define	_ALPHA_LOCK_H_

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#endif

static __inline int
__SIMPLELOCK_LOCKED_P(const __cpu_simple_lock_t *__ptr)
{
	return *__ptr == __SIMPLELOCK_LOCKED;
}

static __inline int
__SIMPLELOCK_UNLOCKED_P(const __cpu_simple_lock_t *__ptr)
{
	return *__ptr == __SIMPLELOCK_UNLOCKED;
}

static __inline void
__cpu_simple_lock_clear(__cpu_simple_lock_t *__ptr)
{
	*__ptr = __SIMPLELOCK_UNLOCKED;
}

static __inline void
__cpu_simple_lock_set(__cpu_simple_lock_t *__ptr)
{
	*__ptr = __SIMPLELOCK_LOCKED;
}

static __inline void
__cpu_simple_lock_init(__cpu_simple_lock_t *alp)
{

	*alp = __SIMPLELOCK_UNLOCKED;
}

static __inline void
__cpu_simple_lock(__cpu_simple_lock_t *alp)
{
	unsigned long t0;

	/*
	 * Note, if we detect that the lock is held when
	 * we do the initial load-locked, we spin using
	 * a non-locked load to save the coherency logic
	 * some work.
	 */

	__asm volatile(
		"# BEGIN __cpu_simple_lock\n"
		"1:	ldl_l	%0, %3		\n"
		"	bne	%0, 2f		\n"
		"	bis	$31, %2, %0	\n"
		"	stl_c	%0, %1		\n"
		"	beq	%0, 3f		\n"
		"	mb			\n"
		"	br	4f		\n"
		"2:	ldl	%0, %3		\n"
		"	beq	%0, 1b		\n"
		"	br	2b		\n"
		"3:	br	1b		\n"
		"4:				\n"
		"	# END __cpu_simple_lock\n"
		: "=&r" (t0), "=m" (*alp)
		: "i" (__SIMPLELOCK_LOCKED), "m" (*alp)
		: "memory");
}

static __inline int
__cpu_simple_lock_try(__cpu_simple_lock_t *alp)
{
	unsigned long t0, v0;

	__asm volatile(
		"# BEGIN __cpu_simple_lock_try\n"
		"1:	ldl_l	%0, %4		\n"
		"	bne	%0, 2f		\n"
		"	bis	$31, %3, %0	\n"
		"	stl_c	%0, %2		\n"
		"	beq	%0, 3f		\n"
		"	mb			\n"
		"	bis	$31, 1, %1	\n"
		"	br	4f		\n"
		"2:	bis	$31, $31, %1	\n"
		"	br	4f		\n"
		"3:	br	1b		\n"
		"4:				\n"
		"	# END __cpu_simple_lock_try"
		: "=&r" (t0), "=r" (v0), "=m" (*alp)
		: "i" (__SIMPLELOCK_LOCKED), "m" (*alp)
		: "memory");

	return (v0 != 0);
}

static __inline void
__cpu_simple_unlock(__cpu_simple_lock_t *alp)
{

	__asm volatile(
		"# BEGIN __cpu_simple_unlock\n"
		"	mb			\n"
		"	stl	$31, %0		\n"
		"	# END __cpu_simple_unlock"
		: "=m" (*alp));
}

#if defined(MULTIPROCESSOR)
/*
 * On the Alpha, interprocessor interrupts come in at device priority
 * level (ALPHA_PSL_IPL_CLOCK).  This can cause some problems while
 * waiting for spin locks from a high'ish priority level (like spin
 * mutexes used by the scheduler): IPIs that come in will not be
 * processed. This can lead to deadlock.
 *
 * This hook allows IPIs to be processed while spinning.  Note we only
 * do the special thing if IPIs are blocked (current IPL >= IPL_CLOCK).
 * IPIs will be processed in the normal fashion otherwise, and checking
 * this way ensures that preemption is disabled (i.e. curcpu() is stable).
 */
#define	SPINLOCK_SPIN_HOOK						\
do {									\
	unsigned long _ipl_ = alpha_pal_rdps() & ALPHA_PSL_IPL_MASK;	\
									\
	if (_ipl_ >= ALPHA_PSL_IPL_CLOCK) {				\
		struct cpu_info *__ci = curcpu();			\
		if (atomic_load_relaxed(&__ci->ci_ipis) != 0) {		\
			alpha_ipi_process(__ci, NULL);			\
		}							\
	}								\
} while (/*CONSTCOND*/0)
#define	SPINLOCK_BACKOFF_HOOK	(void)nullop((void *)0)
#endif /* MULTIPROCESSOR */

#endif /* _ALPHA_LOCK_H_ */
