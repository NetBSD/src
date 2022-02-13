/* $NetBSD: lock.h,v 1.3 2022/02/13 13:42:12 riastradh Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _OR1K_LOCK_H_
#define	_OR1K_LOCK_H_

static __inline int
__SIMPLELOCK_LOCKED_P(const __cpu_simple_lock_t *__ptr)
{
	return *__ptr != __SIMPLELOCK_UNLOCKED;
}

static __inline int
__SIMPLELOCK_UNLOCKED_P(const __cpu_simple_lock_t *__ptr)
{
	return *__ptr == __SIMPLELOCK_UNLOCKED;
}

static __inline void
__cpu_simple_lock_clear(__cpu_simple_lock_t *__ptr)
{
#if 0
	__atomic_clear(__ptr, __ATOMIC_RELAXED);
#else
	*__ptr = __SIMPLELOCK_UNLOCKED;
#endif
}

static __inline void
__cpu_simple_lock_set(__cpu_simple_lock_t *__ptr)
{
#if 0
	(void)__atomic_test_and_set(__ptr, __ATOMIC_RELAXED);
#else
	*__ptr = __SIMPLELOCK_LOCKED;
#endif
}

static __inline void __unused
__cpu_simple_lock_init(__cpu_simple_lock_t *__ptr)
{
#if 0
	__atomic_clear(__ptr, __ATOMIC_RELAXED);
#else
	*__ptr = __SIMPLELOCK_UNLOCKED;
#endif
}

static __inline void __unused
__cpu_simple_lock(__cpu_simple_lock_t *__ptr)
{
#if 0
	while (__atomic_test_and_set(__ptr, __ATOMIC_ACQUIRE)) {
		/* do nothing */
	}
#else
	int tmp;
	/*
	 * No explicit memory barrier needed around ll/sc:
	 *
	 * `In implementations that use a weakly-ordered memory model,
	 *  l.swa nad l.lwa will serve as synchronization points,
	 *  similar to lsync.'
	 *
	 * https://openrisc.io/or1k.html#__RefHeading__341344_552419154
	 */
	__asm volatile(
		"1:"
	"\t"	"l.lwa	%[tmp],0(%[ptr])"
	"\n\t"	"l.sfeqi\t%[tmp],%[unlocked]"
	"\n\t"	"l.bnf	1b"
	"\n\t"	"l.nop"

	"\n\t"	"l.swa	0(%[ptr]),%[newval]"
	"\n\t"	"l.bnf	1b"
	"\n\t"	"l.nop"
	   :	[tmp] "=&r" (tmp)
	   :	[newval] "r" (__SIMPLELOCK_LOCKED),
		[ptr] "r" (__ptr),
		[unlocked] "n" (__SIMPLELOCK_UNLOCKED)
	   :    "cc", "memory");
#endif
}

static __inline int __unused
__cpu_simple_lock_try(__cpu_simple_lock_t *__ptr)
{
#if 0
	return !__atomic_test_and_set(__ptr, __ATOMIC_ACQUIRE);
#else
	int oldval;
	/* No explicit memory barrier needed, as in __cpu_simple_lock.  */
	__asm volatile(
		"1:"
	"\t"	"l.lwa	%[oldval],0(%[ptr])"
	"\n\t"	"l.swa	0(%[ptr]),%[newval]"
	"\n\t"	"l.bnf	1b"
	"\n\t"	"l.nop"
	   :	[oldval] "=&r" (oldval)
	   :	[newval] "r" (__SIMPLELOCK_LOCKED),
		[ptr] "r" (__ptr)
	   :    "cc", "memory");
	return oldval == __SIMPLELOCK_UNLOCKED;
#endif
}

static __inline void __unused
__cpu_simple_unlock(__cpu_simple_lock_t *__ptr)
{
#if 0
	__atomic_clear(__ptr, __ATOMIC_RELEASE);
#else
	__asm volatile("l.msync" ::: "");
	*__ptr = __SIMPLELOCK_UNLOCKED;
#endif
}

#endif /* _OR1K_LOCK_H_ */
