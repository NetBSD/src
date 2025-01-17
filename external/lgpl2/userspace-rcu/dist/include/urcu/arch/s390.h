// SPDX-FileCopyrightText: 2009 Novell, Inc.
//
// SPDX-License-Identifier: MIT

#ifndef _URCU_ARCH_S390_H
#define _URCU_ARCH_S390_H

/*
 * Trivial definitions for the S390 architecture based on information from the
 * Principles of Operation "CPU Serialization" (5-91), "BRANCH ON CONDITION"
 * (7-25) and "STORE CLOCK" (7-169).
 *
 * Author: Jan Blunck <jblunck@suse.de>
 */

#include <urcu/compiler.h>
#include <urcu/config.h>
#include <urcu/syscall-compat.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAA_CACHE_LINE_SIZE	128

#define cmm_mb()    __asm__ __volatile__("bcr 15,0" : : : "memory")

#define HAS_CAA_GET_CYCLES

typedef uint64_t caa_cycles_t;

static inline caa_cycles_t caa_get_cycles (void)
{
	caa_cycles_t cycles;

	__asm__ __volatile__("stck %0" : "=m" (cycles) : : "cc", "memory" );

	return cycles;
}

/*
 * On Linux, define the membarrier system call number if not yet available in
 * the system headers.
 */
#if (defined(__linux__) && !defined(__NR_membarrier))
#define __NR_membarrier		356
#endif

#ifdef __cplusplus
}
#endif

#include <urcu/arch/generic.h>

#endif /* _URCU_ARCH_S390_H */
