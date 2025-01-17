// SPDX-FileCopyrightText: 2010 Paolo Bonzini <pbonzini@redhat.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_ARCH_GENERIC_H
#define _URCU_ARCH_GENERIC_H

/*
 * arch_generic.h: common definitions for multiple architectures.
 */

#include <urcu/compiler.h>
#include <urcu/config.h>
#include <urcu/syscall-compat.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CAA_CACHE_LINE_SIZE
#define CAA_CACHE_LINE_SIZE	64
#endif

#if !defined(cmm_mc) && !defined(cmm_rmc) && !defined(cmm_wmc)
#define CONFIG_HAVE_MEM_COHERENCY
/*
 * Architectures with cache coherency must _not_ define cmm_mc/cmm_rmc/cmm_wmc.
 *
 * For them, cmm_mc/cmm_rmc/cmm_wmc are implemented with a simple
 * compiler barrier; in addition, we provide defaults for cmm_mb (using
 * GCC builtins) as well as cmm_rmb and cmm_wmb (defaulting to cmm_mb).
 */

#ifdef CONFIG_RCU_USE_ATOMIC_BUILTINS

# ifdef CMM_SANITIZE_THREAD
/*
 * This makes TSAN quiet about unsupported thread fence.
 */
static inline void _cmm_thread_fence_wrapper(void)
{
#   if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpragmas"
#    pragma clang diagnostic ignored "-Wunknown-warning-option"
#    pragma clang diagnostic ignored "-Wtsan"
#   elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpragmas"
#    pragma GCC diagnostic ignored "-Wtsan"
#   endif
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
#   if defined(__clang__)
#    pragma clang diagnostic pop
#   elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#   endif
}
# endif	 /* CMM_SANITIZE_THREAD */

# ifndef cmm_smp_mb
#  ifdef CMM_SANITIZE_THREAD
#   define cmm_smp_mb() _cmm_thread_fence_wrapper()
#  else
#   define cmm_smp_mb() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#  endif /* CMM_SANITIZE_THREAD */
# endif /* !cmm_smp_mb */

#endif	/* CONFIG_RCU_USE_ATOMIC_BUILTINS */


/*
 * cmm_mb() expands to __sync_synchronize() instead of __atomic_thread_fence
 * with SEQ_CST because the former "issues a full memory barrier" while the
 * latter "acts as a synchronization fence between threads" which is too weak
 * for what we want, for example with I/O devices.
 *
 * Even though sync_synchronize seems to be an alias for a sequential consistent
 * atomic thread fence on every architecture on GCC and Clang, this assumption
 * might be untrue in future.  Therefore, the definitions above are used to
 * ensure correct behavior in the future.
 *
 * The above defintions are quoted from the GCC manual.
 */
#ifndef cmm_mb
#define cmm_mb()    __sync_synchronize()
#endif

#ifndef cmm_rmb
#define cmm_rmb()	cmm_mb()
#endif

#ifndef cmm_wmb
#define cmm_wmb()	cmm_mb()
#endif

#define cmm_mc()	cmm_barrier()
#define cmm_rmc()	cmm_barrier()
#define cmm_wmc()	cmm_barrier()
#else
/*
 * Architectures without cache coherency need something like the following:
 *
 * #define cmm_mc()	arch_cache_flush()
 * #define cmm_rmc()	arch_cache_flush_read()
 * #define cmm_wmc()	arch_cache_flush_write()
 *
 * Of these, only cmm_mc is mandatory. cmm_rmc and cmm_wmc default to
 * cmm_mc. cmm_mb/cmm_rmb/cmm_wmb use these definitions by default:
 *
 * #define cmm_mb()	cmm_mc()
 * #define cmm_rmb()	cmm_rmc()
 * #define cmm_wmb()	cmm_wmc()
 */

#ifndef cmm_mb
#define cmm_mb()	cmm_mc()
#endif

#ifndef cmm_rmb
#define cmm_rmb()	cmm_rmc()
#endif

#ifndef cmm_wmb
#define cmm_wmb()	cmm_wmc()
#endif

#ifndef cmm_rmc
#define cmm_rmc()	cmm_mc()
#endif

#ifndef cmm_wmc
#define cmm_wmc()	cmm_mc()
#endif
#endif

/* Nop everywhere except on alpha. */
#ifndef cmm_read_barrier_depends
#define cmm_read_barrier_depends()
#endif

#ifdef CONFIG_RCU_SMP
#ifndef cmm_smp_mb
#define cmm_smp_mb()	cmm_mb()
#endif
#ifndef cmm_smp_rmb
#define cmm_smp_rmb()	cmm_rmb()
#endif
#ifndef cmm_smp_wmb
#define cmm_smp_wmb()	cmm_wmb()
#endif
#ifndef cmm_smp_mc
#define cmm_smp_mc()	cmm_mc()
#endif
#ifndef cmm_smp_rmc
#define cmm_smp_rmc()	cmm_rmc()
#endif
#ifndef cmm_smp_wmc
#define cmm_smp_wmc()	cmm_wmc()
#endif
#ifndef cmm_smp_read_barrier_depends
#define cmm_smp_read_barrier_depends()	cmm_read_barrier_depends()
#endif
#else
#ifndef cmm_smp_mb
#define cmm_smp_mb()	cmm_barrier()
#endif
#ifndef cmm_smp_rmb
#define cmm_smp_rmb()	cmm_barrier()
#endif
#ifndef cmm_smp_wmb
#define cmm_smp_wmb()	cmm_barrier()
#endif
#ifndef cmm_smp_mc
#define cmm_smp_mc()	cmm_barrier()
#endif
#ifndef cmm_smp_rmc
#define cmm_smp_rmc()	cmm_barrier()
#endif
#ifndef cmm_smp_wmc
#define cmm_smp_wmc()	cmm_barrier()
#endif
#ifndef cmm_smp_read_barrier_depends
#define cmm_smp_read_barrier_depends()
#endif
#endif

#ifndef caa_cpu_relax
#define caa_cpu_relax()		cmm_barrier()
#endif

#ifndef HAS_CAA_GET_CYCLES
#define HAS_CAA_GET_CYCLES

#if defined(__APPLE__)

#include <mach/mach.h>
#include <mach/clock.h>
#include <mach/mach_time.h>
#include <time.h>
#include <stdint.h>

typedef uint64_t caa_cycles_t;

static inline caa_cycles_t caa_get_cycles (void)
{
	mach_timespec_t ts = { 0, 0 };
	static clock_serv_t clock_service;

	if (caa_unlikely(!clock_service)) {
		if (host_get_clock_service(mach_host_self(),
				SYSTEM_CLOCK, &clock_service))
			return -1ULL;
	}
	if (caa_unlikely(clock_get_time(clock_service, &ts)))
		return -1ULL;
	return ((uint64_t) ts.tv_sec * 1000000000ULL) + ts.tv_nsec;
}

#elif defined(CONFIG_RCU_HAVE_CLOCK_GETTIME)

#include <time.h>
#include <stdint.h>

typedef uint64_t caa_cycles_t;

static inline caa_cycles_t caa_get_cycles (void)
{
	struct timespec ts;

	if (caa_unlikely(clock_gettime(CLOCK_MONOTONIC, &ts)))
		return -1ULL;
	return ((uint64_t) ts.tv_sec * 1000000000ULL) + ts.tv_nsec;
}

#else

#error caa_get_cycles() not implemented for this platform.

#endif

#endif /* HAS_CAA_GET_CYCLES */

#ifdef __cplusplus
}
#endif

#endif /* _URCU_ARCH_GENERIC_H */
