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

#define rcu_read_lock			urcu_qsbr_read_lock
#define _rcu_read_lock			_urcu_qsbr_read_lock
#define rcu_read_unlock			urcu_qsbr_read_unlock
#define _rcu_read_unlock		_urcu_qsbr_read_unlock
#define rcu_read_ongoing		urcu_qsbr_read_ongoing
#define _rcu_read_ongoing		_urcu_qsbr_read_ongoing
#define rcu_quiescent_state		urcu_qsbr_quiescent_state
#define _rcu_quiescent_state		_urcu_qsbr_quiescent_state
#define rcu_thread_offline		urcu_qsbr_thread_offline
#define rcu_thread_online		urcu_qsbr_thread_online
#define rcu_register_thread		urcu_qsbr_register_thread
#define rcu_unregister_thread		urcu_qsbr_unregister_thread
#define rcu_exit			urcu_qsbr_exit
#define synchronize_rcu			urcu_qsbr_synchronize_rcu
#define rcu_reader			urcu_qsbr_reader
#define rcu_gp				urcu_qsbr_gp

#define get_cpu_call_rcu_data		urcu_qsbr_get_cpu_call_rcu_data
#define get_call_rcu_thread		urcu_qsbr_get_call_rcu_thread
#define create_call_rcu_data		urcu_qsbr_create_call_rcu_data
#define set_cpu_call_rcu_data		urcu_qsbr_set_cpu_call_rcu_data
#define get_default_call_rcu_data	urcu_qsbr_get_default_call_rcu_data
#define get_call_rcu_data		urcu_qsbr_get_call_rcu_data
#define get_thread_call_rcu_data	urcu_qsbr_get_thread_call_rcu_data
#define set_thread_call_rcu_data	urcu_qsbr_set_thread_call_rcu_data
#define create_all_cpu_call_rcu_data	urcu_qsbr_create_all_cpu_call_rcu_data
#define free_all_cpu_call_rcu_data	urcu_qsbr_free_all_cpu_call_rcu_data
#define call_rcu			urcu_qsbr_call_rcu
#define call_rcu_data_free		urcu_qsbr_call_rcu_data_free
#define call_rcu_before_fork		urcu_qsbr_call_rcu_before_fork
#define call_rcu_after_fork_parent	urcu_qsbr_call_rcu_after_fork_parent
#define call_rcu_after_fork_child	urcu_qsbr_call_rcu_after_fork_child
#define rcu_barrier			urcu_qsbr_barrier

#define defer_rcu			urcu_qsbr_defer_rcu
#define rcu_defer_register_thread	urcu_qsbr_defer_register_thread
#define rcu_defer_unregister_thread	urcu_qsbr_defer_unregister_thread
#define	rcu_defer_barrier		urcu_qsbr_defer_barrier
#define rcu_defer_barrier_thread	urcu_qsbr_defer_barrier_thread
#define rcu_defer_exit			urcu_qsbr_defer_exit

#define rcu_flavor			urcu_qsbr_flavor

#define urcu_register_rculfhash_atfork		\
		urcu_qsbr_register_rculfhash_atfork
#define urcu_unregister_rculfhash_atfork	\
		urcu_qsbr_unregister_rculfhash_atfork

#define start_poll_synchronize_rcu	urcu_qsbr_start_poll_synchronize_rcu
#define poll_state_synchronize_rcu	urcu_qsbr_poll_state_synchronize_rcu


/* Compat identifiers for prior undocumented multiflavor usage */
#ifndef URCU_NO_COMPAT_IDENTIFIERS

#define rcu_dereference_qsbr		urcu_qsbr_dereference
#define rcu_cmpxchg_pointer_qsbr	urcu_qsbr_cmpxchg_pointer
#define rcu_xchg_pointer_qsbr		urcu_qsbr_xchg_pointer
#define rcu_set_pointer_qsbr		urcu_qsbr_set_pointer

#define rcu_qsbr_before_fork		urcu_qsbr_before_fork
#define rcu_qsbr_after_fork_parent	urcu_qsbr_after_fork_parent
#define rcu_qsbr_after_fork_child	urcu_qsbr_after_fork_child

#define rcu_read_lock_qsbr		urcu_qsbr_read_lock
#define _rcu_read_lock_qsbr		_urcu_qsbr_read_lock
#define rcu_read_unlock_qsbr		urcu_qsbr_read_unlock
#define _rcu_read_unlock_qsbr		_urcu_qsbr_read_unlock
#define rcu_read_ongoing_qsbr		urcu_qsbr_read_ongoing
#define _rcu_read_ongoing_qsbr		_urcu_qsbr_read_ongoing
#define rcu_register_thread_qsbr	urcu_qsbr_register_thread
#define rcu_unregister_thread_qsbr	urcu_qsbr_unregister_thread
#define rcu_init_qsbr			urcu_qsbr_init
#define rcu_exit_qsbr			urcu_qsbr_exit
#define synchronize_rcu_qsbr		urcu_qsbr_synchronize_rcu
#define rcu_reader_qsbr			urcu_qsbr_reader
#define rcu_gp_qsbr			urcu_qsbr_gp

#define get_cpu_call_rcu_data_qsbr	urcu_qsbr_get_cpu_call_rcu_data
#define get_call_rcu_thread_qsbr	urcu_qsbr_get_call_rcu_thread
#define create_call_rcu_data_qsbr	urcu_qsbr_create_call_rcu_data
#define set_cpu_call_rcu_data_qsbr	urcu_qsbr_set_cpu_call_rcu_data
#define get_default_call_rcu_data_qsbr	urcu_qsbr_get_default_call_rcu_data
#define get_call_rcu_data_qsbr		urcu_qsbr_get_call_rcu_data
#define get_thread_call_rcu_data_qsbr	urcu_qsbr_get_thread_call_rcu_data
#define set_thread_call_rcu_data_qsbr	urcu_qsbr_set_thread_call_rcu_data
#define create_all_cpu_call_rcu_data_qsbr	\
		urcu_qsbr_create_all_cpu_call_rcu_data
#define free_all_cpu_call_rcu_data_qsbr	urcu_qsbr_free_all_cpu_call_rcu_data
#define call_rcu_qsbr			urcu_qsbr_call_rcu
#define call_rcu_data_free_qsbr		urcu_qsbr_call_rcu_data_free
#define call_rcu_before_fork_qsbr	urcu_qsbr_call_rcu_before_fork
#define call_rcu_after_fork_parent_qsbr	urcu_qsbr_call_rcu_after_fork_parent
#define call_rcu_after_fork_child_qsbr	urcu_qsbr_call_rcu_after_fork_child
#define rcu_barrier_qsbr		urcu_qsbr_barrier

#define defer_rcu_qsbr			urcu_qsbr_defer_rcu
#define rcu_defer_register_thread_qsbr	urcu_qsbr_defer_register_thread
#define rcu_defer_unregister_thread_qsbr	\
		urcu_qsbr_defer_unregister_thread
#define rcu_defer_barrier_qsbr		urcu_qsbr_defer_barrier
#define rcu_defer_barrier_thread_qsbr	urcu_qsbr_defer_barrier_thread
#define rcu_defer_exit_qsbr		urcu_qsbr_defer_exit

#define rcu_flavor_qsbr			urcu_qsbr_flavor

#define urcu_register_rculfhash_atfork_qsbr	\
		urcu_qsbr_register_rculfhash_atfork
#define urcu_unregister_rculfhash_atfork_qsbr	\
		urcu_qsbr_unregister_rculfhash_atfork

#endif /* URCU_NO_COMPAT_IDENTIFIERS */
