// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef URCU_TESTS_DEBUG_YIELD_H
#define URCU_TESTS_DEBUG_YIELD_H

/*
 * Userspace RCU library tests - Debugging header
 *
 * IBM's contributions to this file may be relicensed under LGPLv2 or later.
 */

#include <sched.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include <compat-rand.h>

#define RCU_YIELD_READ 	(1 << 0)
#define RCU_YIELD_WRITE	(1 << 1)

#define MAX_SLEEP 50

extern unsigned int rcu_yield_active;
extern DECLARE_URCU_TLS(unsigned int, rcu_rand_yield);

#ifdef DEBUG_YIELD
static inline void rcu_debug_yield_read(void)
{
	if (rcu_yield_active & RCU_YIELD_READ)
		if (rand_r(&URCU_TLS(rcu_rand_yield)) & 0x1)
			usleep(rand_r(&URCU_TLS(rcu_rand_yield)) % MAX_SLEEP);
}

static inline void rcu_debug_yield_write(void)
{
	if (rcu_yield_active & RCU_YIELD_WRITE)
		if (rand_r(&URCU_TLS(rcu_rand_yield)) & 0x1)
			usleep(rand_r(&URCU_TLS(rcu_rand_yield)) % MAX_SLEEP);
}

static inline void rcu_debug_yield_enable(unsigned int flags)
{
	rcu_yield_active |= flags;
}

static inline void rcu_debug_yield_disable(unsigned int flags)
{
	rcu_yield_active &= ~flags;
}

static inline void rcu_debug_yield_init(void)
{
	URCU_TLS(rcu_rand_yield) = time(NULL) ^ (unsigned long) pthread_self();
}
#else /* DEBUG_YIELD */
static inline void rcu_debug_yield_read(void)
{
}

static inline void rcu_debug_yield_write(void)
{
}

static inline void rcu_debug_yield_enable(
		unsigned int flags __attribute__((unused)))
{
}

static inline void rcu_debug_yield_disable(
		unsigned int flags __attribute__((unused)))
{
}

static inline void rcu_debug_yield_init(void)
{
}
#endif

#endif /* URCU_TESTS_DEBUG_YIELD_H */
