// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_ARCH_SPARC64_H
#define _URCU_ARCH_SPARC64_H

/*
 * arch_sparc64.h: trivial definitions for the Sparc64 architecture.
 */

#include <urcu/compiler.h>
#include <urcu/config.h>
#include <urcu/syscall-compat.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * On Linux, define the membarrier system call number if not yet available in
 * the system headers.
 */
#if (defined(__linux__) && !defined(__NR_membarrier))
#define __NR_membarrier		351
#endif

#define CAA_CACHE_LINE_SIZE	256

#ifdef __sparc_v9__
/*
 * Inspired from the Linux kernel. Workaround Spitfire bug #51.
 */
#define membar_safe(type)			\
__asm__ __volatile__("ba,pt %%xcc, 1f\n\t"	\
		     "membar " type "\n"	\
		     "1:\n"			\
		     : : : "memory")

#define cmm_mb()	membar_safe("#LoadLoad | #LoadStore | #StoreStore | #StoreLoad")
#define cmm_rmb()	membar_safe("#LoadLoad")
#define cmm_wmb()	membar_safe("#StoreStore")
#elif __sparc_v8__
/* from gcc config/sparc/sync.md */
#define cmm_mb()	__asm__ __volatile__("stbar\n\tldstub\t[%%sp-1], %%g0")
#define cmm_rmb()	__asm__ __volatile__("ldstub\t[%%sp-1], %%g0")
#define cmm_wmb()	__asm__ __volatile__("stbar")
#else
#define cmm_mb()
#define cmm_rmb()
#define cmm_wmb()
#endif

#ifdef __cplusplus
}
#endif

#include <urcu/arch/generic.h>

#endif /* _URCU_ARCH_SPARC64_H */
