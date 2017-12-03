/*	$NetBSD: lock.h,v 1.31.44.1 2017/12/03 11:36:43 jdolecek Exp $ */

/*-
 * Copyright (c) 1998, 1999, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg and Andrew Doran.
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

#ifndef _MACHINE_LOCK_H
#define _MACHINE_LOCK_H

/*
 * Machine dependent spin lock operations.
 */

#if __SIMPLELOCK_UNLOCKED != 0
#error __SIMPLELOCK_UNLOCKED must be 0 for this implementation
#endif

/* XXX So we can expose this to userland. */
#ifdef __lint__
#define __ldstub(__addr)	(__addr)
#else /* !__lint__ */
static __inline int __ldstub(__cpu_simple_lock_t *addr);
static __inline int __ldstub(__cpu_simple_lock_t *addr)
{
	int v;

	__asm volatile("ldstub [%1],%0"
	    : "=&r" (v)
	    : "r" (addr)
	    : "memory");

	return v;
}
#endif /* __lint__ */

static __inline void __cpu_simple_lock_init(__cpu_simple_lock_t *)
	__attribute__((__unused__));
static __inline int __cpu_simple_lock_try(__cpu_simple_lock_t *)
	__attribute__((__unused__));
static __inline void __cpu_simple_unlock(__cpu_simple_lock_t *)
	__attribute__((__unused__));
#ifndef __CPU_SIMPLE_LOCK_NOINLINE
static __inline void __cpu_simple_lock(__cpu_simple_lock_t *)
	__attribute__((__unused__));
#else
extern void __cpu_simple_lock(__cpu_simple_lock_t *);
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

#ifndef __CPU_SIMPLE_LOCK_NOINLINE
static __inline void
__cpu_simple_lock(__cpu_simple_lock_t *alp)
{

	/*
	 * If someone else holds the lock use simple reads until it
	 * is released, then retry the atomic operation. This reduces
	 * memory bus contention because the cache-coherency logic
	 * does not have to broadcast invalidates on the lock while
	 * we spin on it.
	 */
	while (__ldstub(alp) != __SIMPLELOCK_UNLOCKED) {
		while (*alp != __SIMPLELOCK_UNLOCKED)
			/* spin */ ;
	}
}
#endif /* __CPU_SIMPLE_LOCK_NOINLINE */

static __inline int
__cpu_simple_lock_try(__cpu_simple_lock_t *alp)
{

	return (__ldstub(alp) == __SIMPLELOCK_UNLOCKED);
}

static __inline void
__cpu_simple_unlock(__cpu_simple_lock_t *alp)
{

	/*
	 * Insert compiler barrier to prevent instruction re-ordering
	 * around the lock release.
	 */
	__insn_barrier();
	*alp = __SIMPLELOCK_UNLOCKED;
}

#if defined(__sparc_v9__)
static __inline void
mb_read(void)
{
	__asm __volatile("membar #LoadLoad" : : : "memory");
}

static __inline void
mb_write(void)
{
	__asm __volatile("" : : : "memory");
}

static __inline void
mb_memory(void)
{
	__asm __volatile("membar #MemIssue" : : : "memory");
}
#else	/* __sparc_v9__ */
static __inline void
mb_read(void)
{
	static volatile int junk;
	__asm volatile("st %%g0,[%0]"
	    :
	    : "r" (&junk)
	    : "memory");
}

static __inline void
mb_write(void)
{
	__insn_barrier();
}

static __inline void
mb_memory(void)
{
	mb_read();
}
#endif	/* __sparc_v9__ */

#endif /* _MACHINE_LOCK_H */
