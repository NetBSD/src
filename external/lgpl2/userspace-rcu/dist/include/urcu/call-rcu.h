// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_CALL_RCU_H
#define _URCU_CALL_RCU_H

/*
 * Userspace RCU header - batch memory reclamation with kernel API
 *
 * This header is meant to be included indirectly through a liburcu
 * flavor header.
 */

#include <stdlib.h>
#include <pthread.h>

#include <urcu/wfcqueue.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Note that struct call_rcu_data is opaque to callers. */

struct call_rcu_data;

/* Flag values. */

#define URCU_CALL_RCU_RT	(1U << 0)
#define URCU_CALL_RCU_RUNNING	(1U << 1)
#define URCU_CALL_RCU_STOP	(1U << 2)
#define URCU_CALL_RCU_STOPPED	(1U << 3)
#define URCU_CALL_RCU_PAUSE	(1U << 4)
#define URCU_CALL_RCU_PAUSED	(1U << 5)

/*
 * The rcu_head data structure is placed in the structure to be freed
 * via call_rcu().
 */

struct rcu_head {
	struct cds_wfcq_node next;
	void (*func)(struct rcu_head *head);
};

/*
 * Exported functions
 *
 * Important: see rcu-api.md in userspace-rcu documentation for
 * call_rcu family of functions usage detail, including the surrounding
 * RCU usage required when using these primitives.
 */

void call_rcu(struct rcu_head *head,
	      void (*func)(struct rcu_head *head));

struct call_rcu_data *create_call_rcu_data(unsigned long flags,
					   int cpu_affinity);
void call_rcu_data_free(struct call_rcu_data *crdp);

struct call_rcu_data *get_default_call_rcu_data(void);
struct call_rcu_data *get_cpu_call_rcu_data(int cpu);
struct call_rcu_data *get_thread_call_rcu_data(void);
struct call_rcu_data *get_call_rcu_data(void);
pthread_t get_call_rcu_thread(struct call_rcu_data *crdp);

void set_thread_call_rcu_data(struct call_rcu_data *crdp);
int set_cpu_call_rcu_data(int cpu, struct call_rcu_data *crdp);

int create_all_cpu_call_rcu_data(unsigned long flags);
void free_all_cpu_call_rcu_data(void);

void call_rcu_before_fork(void);
void call_rcu_after_fork_parent(void);
void call_rcu_after_fork_child(void);

void rcu_barrier(void);

#ifdef __cplusplus
}
#endif

#endif /* _URCU_CALL_RCU_H */
