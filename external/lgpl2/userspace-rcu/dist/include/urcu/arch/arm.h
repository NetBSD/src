// SPDX-FileCopyrightText: 2010 Paul E. McKenney, IBM Corporation.
// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_ARCH_ARM_H
#define _URCU_ARCH_ARM_H

/*
 * arch_arm.h: trivial definitions for the ARM architecture.
 */

#include <urcu/compiler.h>
#include <urcu/config.h>
#include <urcu/syscall-compat.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Using DMB is faster than the builtin __sync_synchronize and this instruction is
 * part of the baseline ARMv7 ISA.
 */
#ifdef URCU_ARCH_ARMV7

/* For backwards compat. */
#define CONFIG_RCU_ARM_HAVE_DMB 1

/*
 * Issues full system DMB operation.
 */
#define cmm_mb()	__asm__ __volatile__ ("dmb sy":::"memory")
#define cmm_rmb()	__asm__ __volatile__ ("dmb sy":::"memory")
#define cmm_wmb()	__asm__ __volatile__ ("dmb sy":::"memory")

/*
 * Issues DMB operation only to the inner shareable domain.
 */
#define cmm_smp_mb()	__asm__ __volatile__ ("dmb ish":::"memory")
#define cmm_smp_rmb()	__asm__ __volatile__ ("dmb ish":::"memory")
#define cmm_smp_wmb()	__asm__ __volatile__ ("dmb ish":::"memory")

#endif /* URCU_ARCH_ARMV7 */

#include <stdlib.h>
#include <sys/time.h>

/*
 * On Linux, define the membarrier system call number if not yet available in
 * the system headers.
 */
#if (defined(__linux__) && !defined(__NR_membarrier))
#define __NR_membarrier		389
#endif

/*
 * Error out for compilers with known bugs.
 */

/*
 * http://gcc.gnu.org/bugzilla/show_bug.cgi?id=58854
 */
#ifdef URCU_GCC_VERSION
# if URCU_GCC_VERSION >= 40800 && URCU_GCC_VERSION <= 40802
#  error Your gcc version produces clobbered frame accesses
# endif
#endif

/*
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=42263
 */
#ifdef URCU_GCC_VERSION
# if URCU_GCC_VERSION >= 40400 && URCU_GCC_VERSION <= 40402
#  error Your gcc version has a non-functional __sync_synchronize()
# endif
#endif

#ifdef __cplusplus
}
#endif

#include <urcu/arch/generic.h>

#endif /* _URCU_ARCH_ARM_H */
