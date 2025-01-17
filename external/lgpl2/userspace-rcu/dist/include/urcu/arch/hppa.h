// SPDX-FileCopyrightText: 2014 Helge Deller <deller@gmx.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_ARCH_HPPA_H
#define _URCU_ARCH_HPPA_H

/*
 * arch/hppa.h: definitions for hppa architecture
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
#define __NR_membarrier		343
#endif

#define HAS_CAA_GET_CYCLES
typedef unsigned long caa_cycles_t;

static inline caa_cycles_t caa_get_cycles(void)
{
	caa_cycles_t cycles;

	asm volatile("mfctl 16, %0" : "=r" (cycles));
	return cycles;
}

#ifdef __cplusplus
}
#endif

#include <urcu/arch/generic.h>

#endif /* _URCU_ARCH_HPPA_H */
