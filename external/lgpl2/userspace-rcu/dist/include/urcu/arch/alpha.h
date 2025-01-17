// SPDX-FileCopyrightText: 2010 Paolo Bonzini <pbonzini@redhat.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_ARCH_ALPHA_H
#define _URCU_ARCH_ALPHA_H

/*
 * arch_alpha.h: trivial definitions for the Alpha architecture.
 */

#include <urcu/compiler.h>
#include <urcu/config.h>
#include <urcu/syscall-compat.h>

#ifdef __cplusplus
extern "C" {
#endif

#define cmm_mb()			__asm__ __volatile__ ("mb":::"memory")
#define cmm_wmb()			__asm__ __volatile__ ("wmb":::"memory")
#define cmm_read_barrier_depends()	__asm__ __volatile__ ("mb":::"memory")

/*
 * On Linux, define the membarrier system call number if not yet available in
 * the system headers.
 */
#if (defined(__linux__) && !defined(__NR_membarrier))
#define __NR_membarrier		517
#endif

#ifdef __cplusplus
}
#endif

#include <urcu/arch/generic.h>

#endif /* _URCU_ARCH_ALPHA_H */
