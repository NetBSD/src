/*	$NetBSD: lock.h,v 1.17 2005/07/20 17:48:17 he Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifdef _KERNEL
#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#include <machine/intr.h>
#endif
#include <machine/cpu.h>
#endif

static __inline void
__cpu_simple_lock_init(__cpu_simple_lock_t *alp)
{
#ifdef _KERNEL
	__asm__ __volatile ("movl %0,%%r1;jsb Sunlock"
		: /* No output */
		: "g"(alp)
		: "r1","cc","memory");
#else
	__asm__ __volatile ("bbcci $0,%0,1f;1:"
		: /* No output */
		: "m"(*alp)
		: "cc");
#endif
}

static __inline int
__cpu_simple_lock_try(__cpu_simple_lock_t *alp)
{
	int ret;

#ifdef _KERNEL
	__asm__ __volatile ("movl %1,%%r1;jsb Slocktry;movl %%r0,%0"
		: "=&r"(ret)
		: "g"(alp)
		: "r0","r1","cc","memory");
#else
	__asm__ __volatile ("clrl %0;bbssi $0,%1,1f;incl %0;1:"
		: "=&r"(ret)
		: "m"(*alp)
		: "cc");
#endif

	return ret;
}

#ifdef _KERNEL
#define	VAX_LOCK_CHECKS ((1 << IPI_SEND_CNCHAR) | (1 << IPI_DDB))
#define	__cpu_simple_lock(alp)						\
do {									\
	struct cpu_info *__ci = curcpu();				\
									\
	while (__cpu_simple_lock_try(alp) == 0) {			\
		int ___s;						\
									\
		if (__ci->ci_ipimsgs & VAX_LOCK_CHECKS) {		\
			___s = splipi();				\
			cpu_handle_ipi();				\
			splx(___s);					\
		}							\
	}								\
} while (0)
#else
static __inline void
__cpu_simple_lock(__cpu_simple_lock_t *alp)
{
	__asm__ __volatile ("1:bbssi $0,%0,1b"
		: /* No outputs */
		: "m"(*alp)
		: "cc");
}
#endif /* _KERNEL */

#if 0
static __inline void
__cpu_simple_lock(__cpu_simple_lock_t *alp)
{
	struct cpu_info *ci = curcpu();

	while (__cpu_simple_lock_try(alp) == 0) {
		int s;

		if (ci->ci_ipimsgs & IPI_SEND_CNCHAR) {
			s = splipi();
			cpu_handle_ipi();
			splx(s);
		}
	}

#if 0
	__asm__ __volatile ("movl %0,%%r1;jsb Slock"
		: /* No output */
		: "g"(alp)
		: "r0","r1","cc","memory");
#endif
#if 0
	__asm__ __volatile ("1:;bbssi $0, %0, 1b"
		: /* No output */
		: "m"(*alp));
#endif
}
#endif

static __inline void
__cpu_simple_unlock(__cpu_simple_lock_t *alp)
{
#ifdef _KERNEL
	__asm__ __volatile ("movl %0,%%r1;jsb Sunlock"
		: /* No output */
		: "g"(alp)
		: "r1","cc","memory");
#else
	__asm__ __volatile ("bbcci $0,%0,1f;1:"
		: /* No output */
		: "m"(*alp)
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
	struct cpu_info *__ci = curcpu();				\
	int ___s;							\
									\
	if (__ci->ci_ipimsgs != 0) {					\
		/* printf("CPU %lu has IPIs pending\n",			\
		    __ci->ci_cpuid); */					\
		___s = splipi();					\
		cpu_handle_ipi();					\
		splx(___s);						\
	}								\
} while (0)
#endif /* MULTIPROCESSOR */
#endif /* _VAX_LOCK_H_ */
