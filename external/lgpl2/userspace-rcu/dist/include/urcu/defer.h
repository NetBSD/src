// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_DEFER_H
#define _URCU_DEFER_H

/*
 * Userspace RCU header - deferred execution
 *
 * This header is meant to be included indirectly through a liburcu
 * flavor header.
 */

#include <stdlib.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Note: the defer_rcu() API is currently EXPERIMENTAL. It may change in the
 * future.
 *
 * Important !
 *
 * Each thread queuing memory reclamation must be registered with
 * rcu_defer_register_thread(). rcu_defer_unregister_thread() should be
 * called before the thread exits.
 *
 * *NEVER* use defer_rcu() within a RCU read-side critical section, because this
 * primitive need to call synchronize_rcu() if the thread queue is full.
 */

extern void defer_rcu(void (*fct)(void *p), void *p);

/*
 * Thread registration for reclamation.
 */
extern int rcu_defer_register_thread(void);
extern void rcu_defer_unregister_thread(void);
extern void rcu_defer_barrier(void);
extern void rcu_defer_barrier_thread(void);

#ifdef __cplusplus
}
#endif

#endif /* _URCU_DEFER_H */
