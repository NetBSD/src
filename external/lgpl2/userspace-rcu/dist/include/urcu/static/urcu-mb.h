// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_MB_STATIC_H
#define _URCU_MB_STATIC_H

/*
 * Userspace RCU header.
 *
 * TO BE INCLUDED ONLY IN CODE THAT IS TO BE RECOMPILED ON EACH LIBURCU
 * RELEASE. See urcu.h for linking dynamically with the userspace rcu library.
 *
 * IBM's contributions to this file may be relicensed under LGPLv2 or later.
 */

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

#include <urcu/debug.h>
#include <urcu/config.h>
#include <urcu/compiler.h>
#include <urcu/arch.h>
#include <urcu/system.h>
#include <urcu/uatomic.h>
#include <urcu/list.h>
#include <urcu/futex.h>
#include <urcu/tls-compat.h>
#include <urcu/static/urcu-common.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This code section can only be included in LGPL 2.1 compatible source code.
 * See below for the function call wrappers which can be used in code meant to
 * be only linked with the Userspace RCU library. This comes with a small
 * performance degradation on the read-side due to the added function calls.
 * This is required to permit relinking with newer versions of the library.
 */

extern struct urcu_gp urcu_mb_gp;

extern DECLARE_URCU_TLS(struct urcu_reader, urcu_mb_reader);

/*
 * Helper for _urcu_mb_read_lock().  The format of urcu_mb_gp.ctr (as well as
 * the per-thread rcu_reader.ctr) has the lower-order bits containing a count of
 * _urcu_mb_read_lock() nesting, and a single high-order URCU_BP_GP_CTR_PHASE bit
 * that contains either zero or one.  The cmm_smp_mb() ensures that the accesses in
 * _urcu_mb_read_lock() happen before the subsequent read-side critical section.
 */
static inline void _urcu_mb_read_lock_update(unsigned long tmp)
{
	if (caa_likely(!(tmp & URCU_GP_CTR_NEST_MASK))) {
		_CMM_STORE_SHARED(URCU_TLS(urcu_mb_reader).ctr, _CMM_LOAD_SHARED(urcu_mb_gp.ctr));
		cmm_smp_mb();
	} else
		_CMM_STORE_SHARED(URCU_TLS(urcu_mb_reader).ctr, tmp + URCU_GP_COUNT);
}

/*
 * Enter an RCU read-side critical section.
 *
 * The first cmm_barrier() call ensures that the compiler does not reorder
 * the body of _urcu_mb_read_lock() with a mutex.
 *
 * This function and its helper are both less than 10 lines long.  The
 * intent is that this function meets the 10-line criterion in LGPL,
 * allowing this function to be invoked directly from non-LGPL code.
 */
static inline void _urcu_mb_read_lock(void)
{
	unsigned long tmp;

	urcu_assert_debug(URCU_TLS(urcu_mb_reader).registered);
	cmm_barrier();
	tmp = URCU_TLS(urcu_mb_reader).ctr;
	urcu_assert_debug((tmp & URCU_GP_CTR_NEST_MASK) != URCU_GP_CTR_NEST_MASK);
	_urcu_mb_read_lock_update(tmp);
}

/*
 * This is a helper function for _urcu_mb_read_unlock().
 *
 * The first cmm_smp_mb() call ensures that the critical section is
 * seen to precede the store to rcu_reader.ctr.
 * The second cmm_smp_mb() call ensures that we write to rcu_reader.ctr
 * before reading the update-side futex.
 */
static inline void _urcu_mb_read_unlock_update_and_wakeup(unsigned long tmp)
{
	unsigned long *ctr = &URCU_TLS(urcu_mb_reader).ctr;

	if (caa_likely((tmp & URCU_GP_CTR_NEST_MASK) == URCU_GP_COUNT)) {
		uatomic_store(ctr, tmp - URCU_GP_COUNT, CMM_SEQ_CST);
		urcu_common_wake_up_gp(&urcu_mb_gp);
	} else {
		uatomic_store(ctr, tmp - URCU_GP_COUNT, CMM_RELAXED);
	}
}

/*
 * Exit an RCU read-side critical section.  Both this function and its
 * helper are smaller than 10 lines of code, and are intended to be
 * usable by non-LGPL code, as called out in LGPL.
 */
static inline void _urcu_mb_read_unlock(void)
{
	unsigned long tmp;

	urcu_assert_debug(URCU_TLS(urcu_mb_reader).registered);
	tmp = URCU_TLS(urcu_mb_reader).ctr;
	urcu_assert_debug(tmp & URCU_GP_CTR_NEST_MASK);
	_urcu_mb_read_unlock_update_and_wakeup(tmp);
	cmm_barrier();	/* Ensure the compiler does not reorder us with mutex */
}

/*
 * Returns whether within a RCU read-side critical section.
 *
 * This function is less than 10 lines long.  The intent is that this
 * function meets the 10-line criterion for LGPL, allowing this function
 * to be invoked directly from non-LGPL code.
 */
static inline int _urcu_mb_read_ongoing(void)
{
	return URCU_TLS(urcu_mb_reader).ctr & URCU_GP_CTR_NEST_MASK;
}

#ifdef __cplusplus
}
#endif

#endif /* _URCU_MB_STATIC_H */
