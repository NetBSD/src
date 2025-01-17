// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_QSBR_H
#define _URCU_QSBR_H

/*
 * Userspace RCU QSBR header.
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

#include <urcu/config.h>

/*
 * See urcu/pointer.h and urcu/static/pointer.h for pointer
 * publication headers.
 */
#include <urcu/pointer.h>
#include <urcu/urcu-poll.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <urcu/map/urcu-qsbr.h>

#ifdef RCU_DEBUG	/* For backward compatibility */
#define DEBUG_RCU
#endif

/*
 * Important !
 *
 * Each thread containing read-side critical sections must be registered
 * with rcu_register_thread() before calling rcu_read_lock().
 * rcu_unregister_thread() should be called before the thread exits.
 */

#ifdef _LGPL_SOURCE

#include <urcu/static/urcu-qsbr.h>

/*
 * Mappings for static use of the userspace RCU library.
 * Should only be used in LGPL-compatible code.
 */

/*
 * rcu_read_lock()
 * rcu_read_unlock()
 *
 * Mark the beginning and end of a read-side critical section.
 * DON'T FORGET TO USE rcu_register_thread/rcu_unregister_thread()
 * FOR EACH THREAD WITH READ-SIDE CRITICAL SECTION.
 */
#define urcu_qsbr_read_lock		_urcu_qsbr_read_lock
#define urcu_qsbr_read_unlock		_urcu_qsbr_read_unlock
#define urcu_qsbr_read_ongoing		_urcu_qsbr_read_ongoing

#define urcu_qsbr_quiescent_state	_urcu_qsbr_quiescent_state
#define urcu_qsbr_thread_offline	_urcu_qsbr_thread_offline
#define urcu_qsbr_thread_online		_urcu_qsbr_thread_online

#else /* !_LGPL_SOURCE */

/*
 * library wrappers to be used by non-LGPL compatible source code.
 */

/*
 * QSBR read lock/unlock are guaranteed to be no-ops. Therefore, we expose them
 * in the LGPL header for any code to use. However, the debug version is not
 * nops and may contain sanity checks. To activate it, applications must be
 * recompiled with -DDEBUG_RCU (even non-LGPL/GPL applications), or
 * compiled against a urcu/config.h that has CONFIG_RCU_DEBUG defined.
 * This is the best trade-off between license/performance/code
 * triviality and library debugging & tracing features we could come up
 * with.
 */

#if (!defined(BUILD_QSBR_LIB) && !defined(DEBUG_RCU) && !defined(CONFIG_RCU_DEBUG))

static inline void urcu_qsbr_read_lock(void)
{
}

static inline void urcu_qsbr_read_unlock(void)
{
}

#else /* #if (!defined(BUILD_QSBR_LIB) && !defined(DEBUG_RCU) && !defined(CONFIG_RCU_DEBUG)) */

extern void urcu_qsbr_read_lock(void);
extern void urcu_qsbr_read_unlock(void);

#endif /* #else #if (!defined(BUILD_QSBR_LIB) && !defined(DEBUG_RCU) && !defined(CONFIG_RCU_DEBUG)) */

extern int urcu_qsbr_read_ongoing(void);
extern void urcu_qsbr_quiescent_state(void);
extern void urcu_qsbr_thread_offline(void);
extern void urcu_qsbr_thread_online(void);

#endif /* !_LGPL_SOURCE */

extern void urcu_qsbr_synchronize_rcu(void);

/*
 * RCU grace period polling API.
 */
extern struct urcu_gp_poll_state urcu_qsbr_start_poll_synchronize_rcu(void);
extern bool urcu_qsbr_poll_state_synchronize_rcu(struct urcu_gp_poll_state state);

/*
 * Reader thread registration.
 */
extern void urcu_qsbr_register_thread(void);
extern void urcu_qsbr_unregister_thread(void);

#ifdef __cplusplus
}
#endif

#include <urcu/call-rcu.h>
#include <urcu/defer.h>
#include <urcu/flavor.h>

#ifndef URCU_API_MAP
#include <urcu/map/clear.h>
#endif

#endif /* _URCU_QSBR_H */
