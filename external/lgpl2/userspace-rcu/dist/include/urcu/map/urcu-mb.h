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

#define rcu_read_lock			urcu_mb_read_lock
#define _rcu_read_lock			_urcu_mb_read_lock
#define rcu_read_unlock			urcu_mb_read_unlock
#define _rcu_read_unlock		_urcu_mb_read_unlock
#define rcu_read_ongoing		urcu_mb_read_ongoing
#define _rcu_read_ongoing		_urcu_mb_read_ongoing
#define rcu_quiescent_state		urcu_mb_quiescent_state
#define _rcu_quiescent_state		_urcu_mb_quiescent_state
#define rcu_thread_offline		urcu_mb_thread_offline
#define rcu_thread_online		urcu_mb_thread_online
#define rcu_register_thread		urcu_mb_register_thread
#define rcu_unregister_thread		urcu_mb_unregister_thread
#define rcu_init			urcu_mb_init
#define rcu_exit			urcu_mb_exit
#define synchronize_rcu			urcu_mb_synchronize_rcu
#define rcu_reader			urcu_mb_reader
#define rcu_gp				urcu_mb_gp

#define get_cpu_call_rcu_data		urcu_mb_get_cpu_call_rcu_data
#define get_call_rcu_thread		urcu_mb_get_call_rcu_thread
#define create_call_rcu_data		urcu_mb_create_call_rcu_data
#define set_cpu_call_rcu_data		urcu_mb_set_cpu_call_rcu_data
#define get_default_call_rcu_data	urcu_mb_get_default_call_rcu_data
#define get_call_rcu_data		urcu_mb_get_call_rcu_data
#define get_thread_call_rcu_data	urcu_mb_get_thread_call_rcu_data
#define set_thread_call_rcu_data	urcu_mb_set_thread_call_rcu_data
#define create_all_cpu_call_rcu_data	urcu_mb_create_all_cpu_call_rcu_data
#define free_all_cpu_call_rcu_data	urcu_mb_free_all_cpu_call_rcu_data
#define call_rcu			urcu_mb_call_rcu
#define call_rcu_data_free		urcu_mb_call_rcu_data_free
#define call_rcu_before_fork		urcu_mb_call_rcu_before_fork
#define call_rcu_after_fork_parent	urcu_mb_call_rcu_after_fork_parent
#define call_rcu_after_fork_child	urcu_mb_call_rcu_after_fork_child
#define rcu_barrier			urcu_mb_barrier

#define defer_rcu			urcu_mb_defer_rcu
#define rcu_defer_register_thread	urcu_mb_defer_register_thread
#define rcu_defer_unregister_thread	urcu_mb_defer_unregister_thread
#define rcu_defer_barrier		urcu_mb_defer_barrier
#define rcu_defer_barrier_thread	urcu_mb_defer_barrier_thread
#define rcu_defer_exit			urcu_mb_defer_exit

#define rcu_flavor			urcu_mb_flavor

#define urcu_register_rculfhash_atfork		\
		urcu_mb_register_rculfhash_atfork
#define urcu_unregister_rculfhash_atfork	\
		urcu_mb_unregister_rculfhash_atfork

#define start_poll_synchronize_rcu	urcu_mb_start_poll_synchronize_rcu
#define poll_state_synchronize_rcu	urcu_mb_poll_state_synchronize_rcu


/* Compat identifiers for prior undocumented multiflavor usage */
#ifndef URCU_NO_COMPAT_IDENTIFIERS

#define rcu_dereference_mb		urcu_mb_dereference
#define rcu_cmpxchg_pointer_mb		urcu_mb_cmpxchg_pointer
#define rcu_xchg_pointer_mb		urcu_mb_xchg_pointer
#define rcu_set_pointer_mb		urcu_mb_set_pointer

#define rcu_mb_before_fork		urcu_mb_before_fork
#define rcu_mb_after_fork_parent	urcu_mb_after_fork_parent
#define rcu_mb_after_fork_child		urcu_mb_after_fork_child

#define rcu_read_lock_mb		urcu_mb_read_lock
#define _rcu_read_lock_mb		_urcu_mb_read_lock
#define rcu_read_unlock_mb		urcu_mb_read_unlock
#define _rcu_read_unlock_mb		_urcu_mb_read_unlock
#define rcu_read_ongoing_mb		urcu_mb_read_ongoing
#define _rcu_read_ongoing_mb		_urcu_mb_read_ongoing
#define rcu_register_thread_mb		urcu_mb_register_thread
#define rcu_unregister_thread_mb	urcu_mb_unregister_thread
#define rcu_init_mb			urcu_mb_init
#define rcu_exit_mb			urcu_mb_exit
#define synchronize_rcu_mb		urcu_mb_synchronize_rcu
#define rcu_reader_mb			urcu_mb_reader
#define rcu_gp_mb			urcu_mb_gp

#define get_cpu_call_rcu_data_mb	urcu_mb_get_cpu_call_rcu_data
#define get_call_rcu_thread_mb		urcu_mb_get_call_rcu_thread
#define create_call_rcu_data_mb		urcu_mb_create_call_rcu_data
#define set_cpu_call_rcu_data_mb	urcu_mb_set_cpu_call_rcu_data
#define get_default_call_rcu_data_mb	urcu_mb_get_default_call_rcu_data
#define get_call_rcu_data_mb		urcu_mb_get_call_rcu_data
#define get_thread_call_rcu_data_mb	urcu_mb_get_thread_call_rcu_data
#define set_thread_call_rcu_data_mb	urcu_mb_set_thread_call_rcu_data
#define create_all_cpu_call_rcu_data_mb	urcu_mb_create_all_cpu_call_rcu_data
#define free_all_cpu_call_rcu_data_mb	urcu_mb_free_all_cpu_call_rcu_data
#define call_rcu_mb			urcu_mb_call_rcu
#define call_rcu_data_free_mb		urcu_mb_call_rcu_data_free
#define call_rcu_before_fork_mb		urcu_mb_call_rcu_before_fork
#define call_rcu_after_fork_parent_mb	urcu_mb_call_rcu_after_fork_parent
#define call_rcu_after_fork_child_mb	urcu_mb_call_rcu_after_fork_child
#define rcu_barrier_mb			urcu_mb_barrier

#define defer_rcu_mb			urcu_mb_defer_rcu
#define rcu_defer_register_thread_mb	urcu_mb_defer_register_thread
#define rcu_defer_unregister_thread_mb	urcu_mb_defer_unregister_thread
#define rcu_defer_barrier_mb		urcu_mb_defer_barrier
#define rcu_defer_barrier_thread_mb	urcu_mb_defer_barrier_thread
#define rcu_defer_exit_mb		urcu_mb_defer_exit

#define rcu_flavor_mb			urcu_mb_flavor

#define urcu_register_rculfhash_atfork_mb	\
		urcu_mb_register_rculfhash_atfork
#define urcu_unregister_rculfhash_atfork_mb	\
		urcu_mb_unregister_rculfhash_atfork

#endif /* URCU_NO_COMPAT_IDENTIFIERS */
