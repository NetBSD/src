// SPDX-FileCopyrightText: 2011 Lai Jiangshan <laijs@cn.fujitsu.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_FLAVOR_H
#define _URCU_FLAVOR_H

/*
 * Userspace RCU header - rcu flavor declarations
 */

#include <urcu/urcu-poll.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rcu_head;

struct urcu_atfork {
	void (*before_fork)(void *priv);
	void (*after_fork_parent)(void *priv);
	void (*after_fork_child)(void *priv);
	void *priv;
};

void urcu_register_rculfhash_atfork(struct urcu_atfork *atfork);
void urcu_unregister_rculfhash_atfork(struct urcu_atfork *atfork);

struct rcu_flavor_struct {
	void (*read_lock)(void);
	void (*read_unlock)(void);
	int (*read_ongoing)(void);
	void (*read_quiescent_state)(void);
	void (*update_call_rcu)(struct rcu_head *head,
				void (*func)(struct rcu_head *head));
	void (*update_synchronize_rcu)(void);
	void (*update_defer_rcu)(void (*fct)(void *p), void *p);

	void (*thread_offline)(void);
	void (*thread_online)(void);
	void (*register_thread)(void);
	void (*unregister_thread)(void);

	void (*barrier)(void);

	void (*register_rculfhash_atfork)(struct urcu_atfork *atfork);
	void (*unregister_rculfhash_atfork)(struct urcu_atfork *atfork);

	struct urcu_gp_poll_state (*update_start_poll_synchronize_rcu)(void);
	bool (*update_poll_state_synchronize_rcu)(struct urcu_gp_poll_state state);
};

#define DEFINE_RCU_FLAVOR(x)				\
const struct rcu_flavor_struct x = {			\
	.read_lock		= rcu_read_lock,	\
	.read_unlock		= rcu_read_unlock,	\
	.read_ongoing		= rcu_read_ongoing,	\
	.read_quiescent_state	= rcu_quiescent_state,	\
	.update_call_rcu	= call_rcu,		\
	.update_synchronize_rcu	= synchronize_rcu,	\
	.update_defer_rcu	= defer_rcu,		\
	.thread_offline		= rcu_thread_offline,	\
	.thread_online		= rcu_thread_online,	\
	.register_thread	= rcu_register_thread,	\
	.unregister_thread	= rcu_unregister_thread,\
	.barrier		= rcu_barrier,		\
	.register_rculfhash_atfork = urcu_register_rculfhash_atfork,	\
	.unregister_rculfhash_atfork = urcu_unregister_rculfhash_atfork,\
	.update_start_poll_synchronize_rcu = start_poll_synchronize_rcu,\
	.update_poll_state_synchronize_rcu = poll_state_synchronize_rcu,\
}

extern const struct rcu_flavor_struct rcu_flavor;

#ifdef __cplusplus
}
#endif

#endif /* _URCU_FLAVOR_H */
