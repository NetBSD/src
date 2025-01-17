// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_BP_STATIC_H
#define _URCU_BP_STATIC_H

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

#include <urcu/annotate.h>
#include <urcu/debug.h>
#include <urcu/config.h>
#include <urcu/compiler.h>
#include <urcu/arch.h>
#include <urcu/system.h>
#include <urcu/uatomic.h>
#include <urcu/list.h>
#include <urcu/tls-compat.h>

/*
 * This code section can only be included in LGPL 2.1 compatible source code.
 * See below for the function call wrappers which can be used in code meant to
 * be only linked with the Userspace RCU library. This comes with a small
 * performance degradation on the read-side due to the added function calls.
 * This is required to permit relinking with newer versions of the library.
 */

#ifdef __cplusplus
extern "C" {
#endif

enum urcu_bp_state {
	URCU_BP_READER_ACTIVE_CURRENT,
	URCU_BP_READER_ACTIVE_OLD,
	URCU_BP_READER_INACTIVE,
};

/*
 * The trick here is that URCU_BP_GP_CTR_PHASE must be a multiple of 8 so we can use a
 * full 8-bits, 16-bits or 32-bits bitmask for the lower order bits.
 */
#define URCU_BP_GP_COUNT		(1UL << 0)
/* Use the amount of bits equal to half of the architecture long size */
#define URCU_BP_GP_CTR_PHASE		(1UL << (sizeof(long) << 2))
#define URCU_BP_GP_CTR_NEST_MASK	(URCU_BP_GP_CTR_PHASE - 1)

/*
 * Used internally by _urcu_bp_read_lock.
 */
extern void urcu_bp_register(void);

struct urcu_bp_gp {
	/*
	 * Global grace period counter.
	 * Contains the current URCU_BP_GP_CTR_PHASE.
	 * Also has a URCU_BP_GP_COUNT of 1, to accelerate the reader fast path.
	 * Written to only by writer with mutex taken.
	 * Read by both writer and readers.
	 */
	unsigned long ctr;
} __attribute__((aligned(CAA_CACHE_LINE_SIZE)));

extern struct urcu_bp_gp urcu_bp_gp;

struct urcu_bp_reader {
	/* Data used by both reader and urcu_bp_synchronize_rcu() */
	unsigned long ctr;
	/* Data used for registry */
	struct cds_list_head node __attribute__((aligned(CAA_CACHE_LINE_SIZE)));
	pthread_t tid;
	int alloc;	/* registry entry allocated */
};

/*
 * Bulletproof version keeps a pointer to a registry not part of the TLS.
 * Adds a pointer dereference on the read-side, but won't require to unregister
 * the reader thread.
 */
extern DECLARE_URCU_TLS(struct urcu_bp_reader *, urcu_bp_reader);

#ifdef CONFIG_RCU_FORCE_SYS_MEMBARRIER
#define urcu_bp_has_sys_membarrier	1
#else
extern int urcu_bp_has_sys_membarrier;
#endif

static inline void urcu_bp_smp_mb_slave(void)
{
	if (caa_likely(urcu_bp_has_sys_membarrier))
		cmm_barrier();
	else
		cmm_smp_mb();
}

static inline enum urcu_bp_state urcu_bp_reader_state(unsigned long *ctr,
						cmm_annotate_t *group)
{
	unsigned long v;

	if (ctr == NULL)
		return URCU_BP_READER_INACTIVE;
	/*
	 * Make sure both tests below are done on the same version of *value
	 * to insure consistency.
	 */
	v = uatomic_load(ctr, CMM_RELAXED);
	cmm_annotate_group_mem_acquire(group, ctr);

	if (!(v & URCU_BP_GP_CTR_NEST_MASK))
		return URCU_BP_READER_INACTIVE;
	if (!((v ^ urcu_bp_gp.ctr) & URCU_BP_GP_CTR_PHASE))
		return URCU_BP_READER_ACTIVE_CURRENT;
	return URCU_BP_READER_ACTIVE_OLD;
}

/*
 * Helper for _urcu_bp_read_lock().  The format of urcu_bp_gp.ctr (as well as
 * the per-thread rcu_reader.ctr) has the lower-order bits containing a count of
 * _urcu_bp_read_lock() nesting, and a single high-order URCU_BP_GP_CTR_PHASE bit
 * that contains either zero or one.  The smp_mb_slave() ensures that the accesses in
 * _urcu_bp_read_lock() happen before the subsequent read-side critical section.
 */
static inline void _urcu_bp_read_lock_update(unsigned long tmp)
{
	if (caa_likely(!(tmp & URCU_BP_GP_CTR_NEST_MASK))) {
		_CMM_STORE_SHARED(URCU_TLS(urcu_bp_reader)->ctr, _CMM_LOAD_SHARED(urcu_bp_gp.ctr));
		urcu_bp_smp_mb_slave();
	} else
		_CMM_STORE_SHARED(URCU_TLS(urcu_bp_reader)->ctr, tmp + URCU_BP_GP_COUNT);
}

/*
 * Enter an RCU read-side critical section.
 *
 * The first cmm_barrier() call ensures that the compiler does not reorder
 * the body of _urcu_bp_read_lock() with a mutex.
 *
 * This function and its helper are both less than 10 lines long.  The
 * intent is that this function meets the 10-line criterion in LGPL,
 * allowing this function to be invoked directly from non-LGPL code.
 */
static inline void _urcu_bp_read_lock(void)
{
	unsigned long tmp;

	if (caa_unlikely(!URCU_TLS(urcu_bp_reader)))
		urcu_bp_register(); /* If not yet registered. */
	cmm_barrier();	/* Ensure the compiler does not reorder us with mutex */
	tmp = URCU_TLS(urcu_bp_reader)->ctr;
	urcu_assert_debug((tmp & URCU_BP_GP_CTR_NEST_MASK) != URCU_BP_GP_CTR_NEST_MASK);
	_urcu_bp_read_lock_update(tmp);
}

/*
 * Exit an RCU read-side critical section.  This function is less than
 * 10 lines of code, and is intended to be usable by non-LGPL code, as
 * called out in LGPL.
 */
static inline void _urcu_bp_read_unlock(void)
{
	unsigned long tmp;
	unsigned long *ctr = &URCU_TLS(urcu_bp_reader)->ctr;

	tmp = URCU_TLS(urcu_bp_reader)->ctr;
	urcu_assert_debug(tmp & URCU_BP_GP_CTR_NEST_MASK);
	/* Finish using rcu before decrementing the pointer. */
	urcu_bp_smp_mb_slave();
	cmm_annotate_mem_release(ctr);
	uatomic_store(ctr, tmp - URCU_BP_GP_COUNT, CMM_RELAXED);
	cmm_barrier();	/* Ensure the compiler does not reorder us with mutex */
}

/*
 * Returns whether within a RCU read-side critical section.
 *
 * This function is less than 10 lines long.  The intent is that this
 * function meets the 10-line criterion for LGPL, allowing this function
 * to be invoked directly from non-LGPL code.
 */
static inline int _urcu_bp_read_ongoing(void)
{
	if (caa_unlikely(!URCU_TLS(urcu_bp_reader)))
		urcu_bp_register(); /* If not yet registered. */
	return URCU_TLS(urcu_bp_reader)->ctr & URCU_BP_GP_CTR_NEST_MASK;
}

#ifdef __cplusplus
}
#endif

#endif /* _URCU_BP_STATIC_H */
