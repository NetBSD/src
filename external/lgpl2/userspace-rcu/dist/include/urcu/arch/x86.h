// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_ARCH_X86_H
#define _URCU_ARCH_X86_H

/*
 * arch_x86.h: trivial definitions for the x86 architecture.
 */

#include <urcu/compiler.h>
#include <urcu/config.h>
#include <urcu/syscall-compat.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAA_CACHE_LINE_SIZE	128

/*
 * For now, using lock; addl compatibility mode even for i686, because the
 * Pentium III is seen as a i686, but lacks mfence instruction.  Only using
 * fence for x86_64.
 *
 * k1om (__MIC__) is the name for the Intel MIC family (Xeon Phi). It is an
 * x86_64 variant but lacks fence instructions.
 */
#if (defined(URCU_ARCH_AMD64) && !defined(URCU_ARCH_K1OM))

/* For backwards compat */
#define CONFIG_RCU_HAVE_FENCE 1

#define cmm_mb()    __asm__ __volatile__ ("mfence":::"memory")

/*
 * Define cmm_rmb/cmm_wmb to "strict" barriers that may be needed when
 * using SSE or working with I/O areas.  cmm_smp_rmb/cmm_smp_wmb are
 * only compiler barriers, which is enough for general use.
 */
#define cmm_rmb()     __asm__ __volatile__ ("lfence":::"memory")
#define cmm_wmb()     __asm__ __volatile__ ("sfence"::: "memory")
#define cmm_smp_rmb() cmm_barrier()
#define cmm_smp_wmb() cmm_barrier()

#else

/*
 * We leave smp_rmb/smp_wmb as full barriers for processors that do not have
 * fence instructions.
 *
 * An empty cmm_smp_rmb() may not be enough on old PentiumPro multiprocessor
 * systems, due to an erratum.  The Linux kernel says that "Even distro
 * kernels should think twice before enabling this", but for now let's
 * be conservative and leave the full barrier on 32-bit processors.  Also,
 * IDT WinChip supports weak store ordering, and the kernel may enable it
 * under our feet; cmm_smp_wmb() ceases to be a nop for these processors.
 */
#if (CAA_BITS_PER_LONG == 32)
#define cmm_mb()    __asm__ __volatile__ ("lock; addl $0,0(%%esp)":::"memory")
#define cmm_rmb()    __asm__ __volatile__ ("lock; addl $0,0(%%esp)":::"memory")
#define cmm_wmb()    __asm__ __volatile__ ("lock; addl $0,0(%%esp)":::"memory")
#else
#define cmm_mb()    __asm__ __volatile__ ("lock; addl $0,0(%%rsp)":::"memory")
#define cmm_rmb()    __asm__ __volatile__ ("lock; addl $0,0(%%rsp)":::"memory")
#define cmm_wmb()    __asm__ __volatile__ ("lock; addl $0,0(%%rsp)":::"memory")
#endif
#endif

#define caa_cpu_relax()	__asm__ __volatile__ ("rep; nop" : : : "memory")

#define HAS_CAA_GET_CYCLES

#define rdtscll(val)							  \
	do {						  		  \
	     unsigned int __a, __d;					  \
	     __asm__ __volatile__ ("rdtsc" : "=a" (__a), "=d" (__d));	  \
	     (val) = ((unsigned long long)__a)				  \
			| (((unsigned long long)__d) << 32);		  \
	} while(0)

typedef uint64_t caa_cycles_t;

static inline caa_cycles_t caa_get_cycles(void)
{
        caa_cycles_t ret = 0;

        rdtscll(ret);
        return ret;
}

/*
 * On Linux, define the membarrier system call number if not yet available in
 * the system headers.
 */
#if (defined(__linux__) && !defined(__NR_membarrier))
#if (CAA_BITS_PER_LONG == 32)
#define __NR_membarrier		375
#else
#define __NR_membarrier		324
#endif
#endif

#ifdef __cplusplus
}
#endif

#include <urcu/arch/generic.h>

#endif /* _URCU_ARCH_X86_H */
