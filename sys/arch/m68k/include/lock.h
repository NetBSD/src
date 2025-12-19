/*	$NetBSD: lock.h,v 1.18 2025/12/19 14:57:26 thorpej Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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

#ifndef _M68K_LOCK_H_
#define	_M68K_LOCK_H_

/*
 * For non-kernel or for __HAVE_M68K_BROKEN_RMC, we use _atomic_cas_8()
 * to implement simple locks rather than TAS.
 */
#if defined(__HAVE_M68K_BROKEN_RMC) || !defined(_KERNEL)
#define	__SIMPLELOCK_USE_CAS8
extern uint8_t _atomic_cas_8(volatile uint8_t *, uint8_t, uint8_t);
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
__cpu_simple_lock_init(__cpu_simple_lock_t *alp)
{

	*alp = __SIMPLELOCK_UNLOCKED;
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
__cpu_simple_lock(__cpu_simple_lock_t *alp)
{
#ifdef __SIMPLELOCK_USE_CAS8
	uint8_t __val;

	do {
		__val = _atomic_cas_8(alp, __SIMPLELOCK_UNLOCKED,
		    __SIMPLELOCK_LOCKED);
	} while (__val != __SIMPLELOCK_UNLOCKED);
#else
	__asm volatile(
		"1:	tas	%0	\n"
		"	jne	1b	\n"
		: "=m" (*alp)
		: /* no inputs */
		: "cc", "memory");
#endif
}

static __inline int
__cpu_simple_lock_try(__cpu_simple_lock_t *alp)
{
	int __rv;

#ifdef __SIMPLELOCK_USE_CAS8
	__rv = _atomic_cas_8(alp, __SIMPLELOCK_UNLOCKED, __SIMPLELOCK_LOCKED)
	    == __SIMPLELOCK_UNLOCKED;
#else
	__asm volatile(
		"	moveq	#1, %1	\n"
		"	tas	%0	\n"
		"	jeq	1f	\n"
		"	moveq	#0, %1	\n"
		"1:			\n"
		: "=m" (*alp), "=d" (__rv)
		: /* no inputs */
		: "cc", "memory");
#endif

	return (__rv);
}

static __inline void
__cpu_simple_unlock(__cpu_simple_lock_t *alp)
{

	__insn_barrier();
	*alp = __SIMPLELOCK_UNLOCKED;
}

#endif /* _M68K_LOCK_H_ */
