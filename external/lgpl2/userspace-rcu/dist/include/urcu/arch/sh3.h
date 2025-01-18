//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_ARCH_SH3_H
#define _URCU_ARCH_SH3_H

/*
 * arch/m68k.h: definitions for m68k architecture
 */

#include <urcu/compiler.h>
#include <urcu/config.h>
#include <urcu/syscall-compat.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <sys/time.h>

/*
 * On Linux, define the membarrier system call number if not yet available in
 * the system headers.
 */
#if (defined(__linux__) && !defined(__NR_membarrier))
#define __NR_membarrier		378
#endif

#define cmm_mb()        __asm__ __volatile__("synco")
#define cmm_rmb()       cmm_mb()
#define cmm_wmb()       cmm_mb()


#ifdef __cplusplus
}
#endif

#include <urcu/arch/generic.h>

#endif /* _URCU_ARCH_SH3_H */
