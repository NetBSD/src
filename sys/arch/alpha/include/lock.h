/* $NetBSD: lock.h,v 1.28 2009/11/25 14:28:50 rmind Exp $ */

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
__SIMPLELOCK_LOCKED_P(__cpu_simple_lock_t *__ptr)
{
	return *__ptr == __SIMPLELOCK_LOCKED;
}

static __inline int
__SIMPLELOCK_UNLOCKED_P(__cpu_simple_lock_t *__ptr)
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

	__asm volatile(
		"# BEGIN __cpu_simple_lock_init\n"
		"	stl	$31, %0		\n"
		"	mb			\n"
		"	# END __cpu_simple_lock_init"
		: "=m" (*alp));
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
 * level.  This can cause some problems while waiting for r/w spinlocks
 * from a high'ish priority level: IPIs that come in will not be processed.
 * This can lead to deadlock.
 *
 * This hook allows IPIs to be processed while a spinlock's interlock
 * is released.
 */
#define	SPINLOCK_SPIN_HOOK						\
do {									\
	struct cpu_info *__ci = curcpu();				\
	int __s;							\
									\
	if (__ci->ci_ipis != 0) {					\
		/* printf("CPU %lu has IPIs pending\n",			\
		    __ci->ci_cpuid); */					\
		__s = splhigh();						\
		alpha_ipi_process(__ci, NULL);				\
		splx(__s);						\
	}								\
} while (0)
#define	SPINLOCK_BACKOFF_HOOK	(void)nullop((void *)0)
#endif /* MULTIPROCESSOR */

static __inline void
mb_read(void)
{
	__asm __volatile("mb" : : : "memory");
}

static __inline void
mb_write(void)
{
	/* XXX wmb */
	__asm __volatile("mb" : : : "memory");
}

static __inline void
mb_memory(void)
{
	__asm __volatile("mb" : : : "memory");
}

#endif /* _ALPHA_LOCK_H_ */
