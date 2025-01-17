// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_MB_H
#define _URCU_MB_H

/*
 * Userspace RCU header
 *
 * LGPL-compatible code should include this header with :
 *
 * #define _LGPL_SOURCE
 * #include <urcu.h>
 *
 * IBM's contributions to this file may be relicensed under LGPLv2 or later.
 */

#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

/*
 * See urcu/pointer.h and urcu/static/pointer.h for pointer
 * publication headers.
 */
#include <urcu/pointer.h>
#include <urcu/urcu-poll.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <urcu/map/urcu-mb.h>

/*
 * Important !
 *
 * Each thread containing read-side critical sections must be registered
 * with rcu_register_thread_mb() before calling rcu_read_lock_mb().
 * rcu_unregister_thread_mb() should be called before the thread exits.
 */

#ifdef _LGPL_SOURCE

#include <urcu/static/urcu-mb.h>

/*
 * Mappings for static use of the userspace RCU library.
 * Should only be used in LGPL-compatible code.
 */

/*
 * rcu_read_lock()
 * rcu_read_unlock()
 *
 * Mark the beginning and end of a read-side critical section.
 * DON'T FORGET TO USE RCU_REGISTER/UNREGISTER_THREAD() FOR EACH THREAD WITH
 * READ-SIDE CRITICAL SECTION.
 */
#define urcu_mb_read_lock		_urcu_mb_read_lock
#define urcu_mb_read_unlock		_urcu_mb_read_unlock
#define urcu_mb_read_ongoing		_urcu_mb_read_ongoing

#else /* !_LGPL_SOURCE */

/*
 * library wrappers to be used by non-LGPL compatible source code.
 * See LGPL-only urcu/static/pointer.h for documentation.
 */

extern void urcu_mb_read_lock(void);
extern void urcu_mb_read_unlock(void);
extern int urcu_mb_read_ongoing(void);

#endif /* !_LGPL_SOURCE */

extern void urcu_mb_synchronize_rcu(void);

/*
 * RCU grace period polling API.
 */
extern struct urcu_gp_poll_state urcu_mb_start_poll_synchronize_rcu(void);
extern bool urcu_mb_poll_state_synchronize_rcu(struct urcu_gp_poll_state state);

/*
 * Reader thread registration.
 */
extern void urcu_mb_register_thread(void);
extern void urcu_mb_unregister_thread(void);

/*
 * Explicit rcu initialization, for "early" use within library constructors.
 */
extern void urcu_mb_init(void);

/*
 * Q.S. reporting are no-ops for these URCU flavors.
 */
static inline void urcu_mb_quiescent_state(void)
{
}

static inline void urcu_mb_thread_offline(void)
{
}

static inline void urcu_mb_thread_online(void)
{
}

#ifdef __cplusplus
}
#endif

#include <urcu/call-rcu.h>
#include <urcu/defer.h>
#include <urcu/flavor.h>

#ifndef URCU_API_MAP
#include <urcu/map/clear.h>
#endif

#endif /* _URCU_MB_H */
