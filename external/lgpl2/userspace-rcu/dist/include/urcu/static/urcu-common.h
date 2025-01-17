// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_COMMON_STATIC_H
#define _URCU_COMMON_STATIC_H

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

#include <urcu/annotate.h>
#include <urcu/config.h>
#include <urcu/compiler.h>
#include <urcu/arch.h>
#include <urcu/system.h>
#include <urcu/uatomic.h>
#include <urcu/list.h>
#include <urcu/futex.h>
#include <urcu/tls-compat.h>

#ifdef __cplusplus
extern "C" {
#endif

enum urcu_state {
	URCU_READER_ACTIVE_CURRENT,
	URCU_READER_ACTIVE_OLD,
	URCU_READER_INACTIVE,
};

/*
 * The trick here is that URCU_GP_CTR_PHASE must be a multiple of 8 so
 * we can use a full 8-bits, 16-bits or 32-bits bitmask for the lower
 * order bits.
 */
#define URCU_GP_COUNT		(1UL << 0)
/* Use the amount of bits equal to half of the architecture long size */
#define URCU_GP_CTR_PHASE	(1UL << (sizeof(unsigned long) << 2))
#define URCU_GP_CTR_NEST_MASK	(URCU_GP_CTR_PHASE - 1)

struct urcu_gp {
	/*
	 * Global grace period counter.
	 * Contains the current URCU_GP_CTR_PHASE.
	 * Also has a URCU_GP_COUNT of 1, to accelerate the reader fast path.
	 * Written to only by writer with mutex taken.
	 * Read by both writer and readers.
	 */
	unsigned long ctr;

	int32_t futex;
} __attribute__((aligned(CAA_CACHE_LINE_SIZE)));

struct urcu_reader {
	/* Data used by both reader and synchronize_rcu() */
	unsigned long ctr;
	char need_mb;
	/* Data used for registry */
	struct cds_list_head node __attribute__((aligned(CAA_CACHE_LINE_SIZE)));
	pthread_t tid;
	/* Reader registered flag, for internal checks. */
	unsigned int registered:1;
};

/*
 * Wake-up waiting synchronize_rcu(). Called from many concurrent threads.
 */
static inline void urcu_common_wake_up_gp(struct urcu_gp *gp)
{
	if (caa_unlikely(uatomic_read(&gp->futex) == -1)) {
		uatomic_set(&gp->futex, 0);
		/*
		 * Ignoring return value until we can make this function
		 * return something (because urcu_die() is not publicly
		 * exposed).
		 */
		(void) futex_async(&gp->futex, FUTEX_WAKE, 1,
				NULL, NULL, 0);
	}
}

static inline enum urcu_state urcu_common_reader_state(struct urcu_gp *gp,
						unsigned long *ctr,
						cmm_annotate_t *group)
{
	unsigned long v;

	/*
	 * Make sure both tests below are done on the same version of *value
	 * to insure consistency.
	 */
	v = uatomic_load(ctr, CMM_RELAXED);
	cmm_annotate_group_mem_acquire(group, ctr);

	if (!(v & URCU_GP_CTR_NEST_MASK))
		return URCU_READER_INACTIVE;
	if (!((v ^ gp->ctr) & URCU_GP_CTR_PHASE))
		return URCU_READER_ACTIVE_CURRENT;
	return URCU_READER_ACTIVE_OLD;
}

#ifdef __cplusplus
}
#endif

#endif /* _URCU_COMMON_STATIC_H */
