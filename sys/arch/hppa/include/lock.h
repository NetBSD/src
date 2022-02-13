/* 	$NetBSD: lock.h,v 1.24 2022/02/13 14:06:51 riastradh Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and Matthew Fredette.
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

#ifndef _HPPA_LOCK_H_
#define	_HPPA_LOCK_H_

#include <sys/stdint.h>

#define HPPA_LDCW_ALIGN	16UL

#define __SIMPLELOCK_ALIGN(p) \
    (volatile unsigned long *)((((uintptr_t)(p) + HPPA_LDCW_ALIGN - 1)) & \
    ~(HPPA_LDCW_ALIGN - 1))

#define __SIMPLELOCK_RAW_LOCKED		0UL
#define __SIMPLELOCK_RAW_UNLOCKED	1UL

static __inline int
__SIMPLELOCK_LOCKED_P(const __cpu_simple_lock_t *__ptr)
{
	return *__SIMPLELOCK_ALIGN(__ptr) == __SIMPLELOCK_RAW_LOCKED;
}

static __inline int
__SIMPLELOCK_UNLOCKED_P(const __cpu_simple_lock_t *__ptr)
{
	return *__SIMPLELOCK_ALIGN(__ptr) == __SIMPLELOCK_RAW_UNLOCKED;
}

static __inline int
__ldcw(volatile unsigned long *__ptr)
{
	int __val;

	__asm volatile("ldcw 0(%1), %0"
	    : "=r" (__val) : "r" (__ptr)
	    : "memory");

	return __val;
}

static __inline void
__sync(void)
{

	__asm volatile("sync\n"
		: /* no outputs */
		: /* no inputs */
		: "memory");
}

static __inline void
__cpu_simple_lock_init(__cpu_simple_lock_t *alp)
{

	alp->csl_lock[0] = alp->csl_lock[1] =
	alp->csl_lock[2] = alp->csl_lock[3] =
	    __SIMPLELOCK_RAW_UNLOCKED;
}

static __inline int
__cpu_simple_lock_try(__cpu_simple_lock_t *alp)
{
	volatile unsigned long *__aptr = __SIMPLELOCK_ALIGN(alp);

	if (__ldcw(__aptr) == __SIMPLELOCK_RAW_LOCKED)
		return 0;

	/*
	 * __cpu_simple_lock_try must be a load-acquire operation, but
	 * HPPA's LDCW does not appear to guarantee load-acquire
	 * semantics, so we have to do LDCW and then an explicit SYNC
	 * to make a load-acquire operation that pairs with a preceding
	 * store-release in __cpu_simple_unlock.
	 */
	__sync();
	return 1;
}

static __inline void
__cpu_simple_lock(__cpu_simple_lock_t *alp)
{
	volatile unsigned long *__aptr = __SIMPLELOCK_ALIGN(alp);

	/*
	 * Note, if we detect that the lock is held when
	 * we do the initial load-clear-word, we spin using
	 * a non-locked load to save the coherency logic
	 * some work.
	 */

	while (!__cpu_simple_lock_try(alp))
		while (*__aptr == __SIMPLELOCK_RAW_LOCKED)
			;
}

static __inline void
__cpu_simple_unlock(__cpu_simple_lock_t *alp)
{
	volatile unsigned long *__aptr = __SIMPLELOCK_ALIGN(alp);

	/*
	 * SYNC and then store makes a store-release that pairs with
	 * the load-acquire in a subsequent __cpu_simple_lock_try.
	 */
	__sync();
	*__aptr = __SIMPLELOCK_RAW_UNLOCKED;
}

static __inline void
__cpu_simple_lock_set(__cpu_simple_lock_t *alp)
{
	volatile unsigned long *__aptr = __SIMPLELOCK_ALIGN(alp);

	*__aptr = __SIMPLELOCK_RAW_LOCKED;
}

static __inline void
__cpu_simple_lock_clear(__cpu_simple_lock_t *alp)
{
	volatile unsigned long *__aptr = __SIMPLELOCK_ALIGN(alp);

	*__aptr = __SIMPLELOCK_RAW_UNLOCKED;
}

#endif /* _HPPA_LOCK_H_ */
