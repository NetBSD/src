/*	$NetBSD: lock.h,v 1.18.14.1 2017/12/03 11:36:27 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles and Andrew Doran.
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
 * Machine-dependent spin lock operations for MIPS processors.
 *
 * Note: R2000/R3000 doesn't have any atomic update instructions; this
 * will cause problems for user applications using this header.
 */

#ifndef _MIPS_LOCK_H_
#define	_MIPS_LOCK_H_

#include <sys/param.h>

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
	*__ptr = __SIMPLELOCK_UNLOCKED;
}

static __inline void
__cpu_simple_lock_set(__cpu_simple_lock_t *__ptr)
{
	*__ptr = __SIMPLELOCK_LOCKED;
}

#ifndef _HARDKERNEL

static __inline int
__cpu_simple_lock_try(__cpu_simple_lock_t *lp)
{
	unsigned long t0, v0;

	__asm volatile(
		"# -- BEGIN __cpu_simple_lock_try\n"
		"	.set push		\n"
		"	.set mips2		\n"
		"1:	ll	%0, %4		\n"
		"	bnez	%0, 2f		\n"
		"	nop	       # BDslot	\n"
		"	li	%0, %3		\n"
		"	sc	%0, %2		\n"
		"	beqz	%0, 2f		\n"
		"	nop	       # BDslot	\n"
		"	li	%1, 1		\n"
		"	sync			\n"
		"	j	3f		\n"
		"	nop			\n"
		"	nop			\n"
		"2:	li	%1, 0		\n"
		"3:				\n"
		"	.set pop		\n"
		"# -- END __cpu_simple_lock_try	\n"
		: "=r" (t0), "=r" (v0), "+m" (*lp)
		: "i" (__SIMPLELOCK_LOCKED), "m" (*lp));

	return (v0 != 0);
}

#ifdef MIPS1
static __inline void
mb_read(void)
{
	__insn_barrier();
}

static __inline void
mb_write(void)
{
	__insn_barrier();
}

static __inline void
mb_memory(void)
{
	__insn_barrier();
}
#else	/* MIPS1*/
static __inline void
mb_read(void)
{
	__asm volatile(
	    "	.set push\n"
	    "	.set mips2\n"
	    "	sync\n"
	    "	.set pop"
	    ::: "memory"
	);
}

static __inline void
mb_write(void)
{
	mb_read();
}

static __inline void
mb_memory(void)
{
	mb_read();
}
#endif	/* MIPS1 */

#else	/* !_HARDKERNEL */

u_int	_atomic_cas_uint(volatile u_int *, u_int, u_int);
u_long	_atomic_cas_ulong(volatile u_long *, u_long, u_long);
void *	_atomic_cas_ptr(volatile void *, void *, void *);
void	mb_read(void);
void	mb_write(void);
void	mb_memory(void);

static __inline int
__cpu_simple_lock_try(__cpu_simple_lock_t *lp)
{

	return _atomic_cas_uint(lp,
	    __SIMPLELOCK_UNLOCKED, __SIMPLELOCK_LOCKED) ==
	    __SIMPLELOCK_UNLOCKED;
}

#endif	/* _HARDKERNEL */

static __inline void
__cpu_simple_lock_init(__cpu_simple_lock_t *lp)
{

	*lp = __SIMPLELOCK_UNLOCKED;
	mb_memory();
}

static __inline void
__cpu_simple_lock(__cpu_simple_lock_t *lp)
{

	while (!__cpu_simple_lock_try(lp)) {
		while (*lp == __SIMPLELOCK_LOCKED)
			/* spin */;
	}
}

static __inline void
__cpu_simple_unlock(__cpu_simple_lock_t *lp)
{

#ifndef _MIPS_ARCH_OCTEONP
	mb_memory();
#endif
	*lp = __SIMPLELOCK_UNLOCKED;
#ifdef _MIPS_ARCH_OCTEONP
	mb_write();
#endif
}

#endif /* _MIPS_LOCK_H_ */
