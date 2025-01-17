// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * Userspace RCU header -- name mapping to allow multiple flavors to be
 * used in the same executable.
 *
 * LGPL-compatible code should include this header with :
 *
 * #define _LGPL_SOURCE
 * #include <urcu.h>
 *
 * IBM's contributions to this file may be relicensed under LGPLv2 or later.
 */

/* Mapping macros to allow multiple flavors in a single binary. */

#define rcu_read_lock			urcu_bp_read_lock
#define _rcu_read_lock			_urcu_bp_read_lock
#define rcu_read_unlock			urcu_bp_read_unlock
#define _rcu_read_unlock		_urcu_bp_read_unlock
#define rcu_read_ongoing		urcu_bp_read_ongoing
#define _rcu_read_ongoing		_urcu_bp_read_ongoing
#define rcu_quiescent_state		urcu_bp_quiescent_state
#define _rcu_quiescent_state		_urcu_bp_quiescent_state
#define rcu_thread_offline		urcu_bp_thread_offline
#define rcu_thread_online		urcu_bp_thread_online
#define rcu_register_thread		urcu_bp_register_thread
#define rcu_unregister_thread		urcu_bp_unregister_thread
#define rcu_init			urcu_bp_init
#define rcu_exit			urcu_bp_exit
#define synchronize_rcu			urcu_bp_synchronize_rcu
#define rcu_reader			urcu_bp_reader
#define rcu_gp				urcu_bp_gp

#define get_cpu_call_rcu_data		urcu_bp_get_cpu_call_rcu_data
#define get_call_rcu_thread		urcu_bp_get_call_rcu_thread
#define create_call_rcu_data		urcu_bp_create_call_rcu_data
#define set_cpu_call_rcu_data		urcu_bp_set_cpu_call_rcu_data
#define get_default_call_rcu_data	urcu_bp_get_default_call_rcu_data
#define get_call_rcu_data		urcu_bp_get_call_rcu_data
#define get_thread_call_rcu_data	urcu_bp_get_thread_call_rcu_data
#define set_thread_call_rcu_data	urcu_bp_set_thread_call_rcu_data
#define create_all_cpu_call_rcu_data	urcu_bp_create_all_cpu_call_rcu_data
#define free_all_cpu_call_rcu_data	urcu_bp_free_all_cpu_call_rcu_data
#define call_rcu			urcu_bp_call_rcu
#define call_rcu_data_free		urcu_bp_call_rcu_data_free
#define call_rcu_before_fork		urcu_bp_call_rcu_before_fork
#define call_rcu_after_fork_parent	urcu_bp_call_rcu_after_fork_parent
#define call_rcu_after_fork_child	urcu_bp_call_rcu_after_fork_child
#define rcu_barrier			urcu_bp_barrier

#define defer_rcu			urcu_bp_defer_rcu
#define rcu_defer_register_thread	urcu_bp_defer_register_thread
#define rcu_defer_unregister_thread	urcu_bp_defer_unregister_thread
#define rcu_defer_barrier		urcu_bp_defer_barrier
#define rcu_defer_barrier_thread	urcu_bp_defer_barrier_thread
#define rcu_defer_exit			urcu_bp_defer_exit

#define rcu_flavor			urcu_bp_flavor

#define rcu_yield_active		urcu_bp_yield_active
#define rcu_rand_yield			urcu_bp_rand_yield

#define urcu_register_rculfhash_atfork		\
		urcu_bp_register_rculfhash_atfork
#define urcu_unregister_rculfhash_atfork	\
		urcu_bp_unregister_rculfhash_atfork

#define start_poll_synchronize_rcu	urcu_bp_start_poll_synchronize_rcu
#define poll_state_synchronize_rcu	urcu_bp_poll_state_synchronize_rcu


/* Compat identifiers for prior undocumented multiflavor usage */
#ifndef URCU_NO_COMPAT_IDENTIFIERS

#define rcu_dereference_bp		urcu_bp_dereference
#define rcu_cmpxchg_pointer_bp		urcu_bp_cmpxchg_pointer
#define rcu_xchg_pointer_bp		urcu_bp_xchg_pointer
#define rcu_set_pointer_bp		urcu_bp_set_pointer

#define rcu_bp_before_fork		urcu_bp_before_fork
#define rcu_bp_after_fork_parent	urcu_bp_after_fork_parent
#define rcu_bp_after_fork_child		urcu_bp_after_fork_child

#define rcu_read_lock_bp		urcu_bp_read_lock
#define _rcu_read_lock_bp		_urcu_bp_read_lock
#define rcu_read_unlock_bp		urcu_bp_read_unlock
#define _rcu_read_unlock_bp		_urcu_bp_read_unlock
#define rcu_read_ongoing_bp		urcu_bp_read_ongoing
#define _rcu_read_ongoing_bp		_urcu_bp_read_ongoing
#define rcu_register_thread_bp		urcu_bp_register_thread
#define rcu_unregister_thread_bp	urcu_bp_unregister_thread
#define rcu_init_bp			urcu_bp_init
#define rcu_exit_bp			urcu_bp_exit
#define synchronize_rcu_bp		urcu_bp_synchronize_rcu
#define rcu_reader_bp			urcu_bp_reader
#define rcu_gp_bp			urcu_bp_gp

#define get_cpu_call_rcu_data_bp	urcu_bp_get_cpu_call_rcu_data
#define get_call_rcu_thread_bp		urcu_bp_get_call_rcu_thread
#define create_call_rcu_data_bp		urcu_bp_create_call_rcu_data
#define set_cpu_call_rcu_data_bp	urcu_bp_set_cpu_call_rcu_data
#define get_default_call_rcu_data_bp	urcu_bp_get_default_call_rcu_data
#define get_call_rcu_data_bp		urcu_bp_get_call_rcu_data
#define get_thread_call_rcu_data_bp	urcu_bp_get_thread_call_rcu_data
#define set_thread_call_rcu_data_bp	urcu_bp_set_thread_call_rcu_data
#define create_all_cpu_call_rcu_data_bp	urcu_bp_create_all_cpu_call_rcu_data
#define free_all_cpu_call_rcu_data_bp	urcu_bp_free_all_cpu_call_rcu_data
#define call_rcu_bp			urcu_bp_call_rcu
#define call_rcu_data_free_bp		urcu_bp_call_rcu_data_free
#define call_rcu_before_fork_bp		urcu_bp_call_rcu_before_fork
#define call_rcu_after_fork_parent_bp	urcu_bp_call_rcu_after_fork_parent
#define call_rcu_after_fork_child_bp	urcu_bp_call_rcu_after_fork_child
#define rcu_barrier_bp			urcu_bp_barrier

#define defer_rcu_bp			urcu_bp_defer_rcu
#define rcu_defer_register_thread_bp	urcu_bp_defer_register_thread
#define rcu_defer_unregister_thread_bp	urcu_bp_defer_unregister_thread
#define rcu_defer_barrier_bp		urcu_bp_defer_barrier
#define rcu_defer_barrier_thread_bp	urcu_bp_defer_barrier_thread
#define rcu_defer_exit_bp		urcu_bp_defer_exit

#define rcu_flavor_bp			urcu_bp_flavor

#define rcu_yield_active_bp		urcu_bp_yield_active
#define rcu_rand_yield_bp		urcu_bp_rand_yield

#define urcu_register_rculfhash_atfork_bp	\
	urcu_bp_register_rculfhash_atfork
#define urcu_unregister_rculfhash_atfork_bp	\
	urcu_bp_unregister_rculfhash_atfork

#endif /* URCU_NO_COMPAT_IDENTIFIERS */
