/*	$NetBSD: lock.h,v 1.29.24.1 2017/12/03 11:36:48 jdolecek Exp $	*/

/*
 * Copyright (c) 2000 Ludd, University of Lule}, Sweden.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _VAX_LOCK_H_
#define _VAX_LOCK_H_

#include <sys/param.h>

#ifdef _KERNEL
#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#include <machine/intr.h>
#endif
#include <machine/cpu.h>
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

static __inline void __cpu_simple_lock_init(__cpu_simple_lock_t *);
static __inline void
__cpu_simple_lock_init(__cpu_simple_lock_t *__alp)
{
#ifdef _HARDKERNEL
	__asm __volatile ("movl %0,%%r1;jsb Sunlock"
		: /* No output */
		: "g"(__alp)
		: "r1","cc","memory");
#else
	__asm __volatile ("bbcci $0,%0,1f;1:"
		: /* No output */
		: "m"(*__alp)
		: "cc");
#endif
}

static __inline int __cpu_simple_lock_try(__cpu_simple_lock_t *);
static __inline int
__cpu_simple_lock_try(__cpu_simple_lock_t *__alp)
{
	int ret;

#ifdef _HARDKERNEL
	__asm __volatile ("movl %1,%%r1;jsb Slocktry;movl %%r0,%0"
		: "=&r"(ret)
		: "g"(__alp)
		: "r0","r1","cc","memory");
#else
	__asm __volatile ("clrl %0;bbssi $0,%1,1f;incl %0;1:"
		: "=&r"(ret)
		: "m"(*__alp)
		: "cc");
#endif

	return ret;
}

static __inline void __cpu_simple_lock(__cpu_simple_lock_t *);
static __inline void
__cpu_simple_lock(__cpu_simple_lock_t *__alp)
{
#if defined(_HARDKERNEL) && defined(MULTIPROCESSOR)
	struct cpu_info * const __ci = curcpu();

	while (__cpu_simple_lock_try(__alp) == 0) {
#define	VAX_LOCK_CHECKS ((1 << IPI_SEND_CNCHAR) | (1 << IPI_DDB))
		if (__ci->ci_ipimsgs & VAX_LOCK_CHECKS) {
			cpu_handle_ipi();
		}
	}
#else /* _HARDKERNEL && MULTIPROCESSOR */
	__asm __volatile ("1:bbssi $0,%0,1b"
		: /* No outputs */
		: "m"(*__alp)
		: "cc");
#endif /* _HARDKERNEL && MULTIPROCESSOR */
}

static __inline void __cpu_simple_unlock(__cpu_simple_lock_t *);
static __inline void
__cpu_simple_unlock(__cpu_simple_lock_t *__alp)
{
#ifdef _HARDKERNEL
	__asm __volatile ("movl %0,%%r1;jsb Sunlock"
		: /* No output */
		: "g"(__alp)
		: "r1","cc","memory");
#else
	__asm __volatile ("bbcci $0,%0,1f;1:"
		: /* No output */
		: "m"(*__alp)
		: "cc");
#endif
}

#if defined(MULTIPROCESSOR)
/*
 * On the Vax, interprocessor interrupts can come in at device priority
 * level or lower. This can cause some problems while waiting for r/w
 * spinlocks from a high'ish priority level: IPIs that come in will not
 * be processed. This can lead to deadlock.
 *
 * This hook allows IPIs to be processed while a spinlock's interlock
 * is released.
 */
#define SPINLOCK_SPIN_HOOK						\
do {									\
	struct cpu_info * const __ci = curcpu();			\
									\
	if (__ci->ci_ipimsgs != 0) {					\
		/* printf("CPU %lu has IPIs pending\n",			\
		    __ci->ci_cpuid); */					\
		cpu_handle_ipi();					\
	}								\
} while (/*CONSTCOND*/0)
#endif /* MULTIPROCESSOR */

static __inline void mb_read(void);
static __inline void
mb_read(void)
{
}

static __inline void mb_write(void);
static __inline void
mb_write(void)
{
}
#endif /* _VAX_LOCK_H_ */
