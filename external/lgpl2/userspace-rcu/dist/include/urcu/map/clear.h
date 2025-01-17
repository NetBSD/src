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
 * #undef _LGPL_SOURCE
 * #include <urcu.h>
 *
 * IBM's contributions to this file may be relicensed under LGPLv2 or later.
 */

#undef rcu_read_lock
#undef _rcu_read_lock
#undef rcu_read_unlock
#undef _rcu_read_unlock
#undef rcu_read_ongoing
#undef _rcu_read_ongoing
#undef rcu_quiescent_state
#undef _rcu_quiescent_state
#undef rcu_thread_offline
#undef rcu_thread_online
#undef rcu_register_thread
#undef rcu_unregister_thread
#undef rcu_init
#undef rcu_exit
#undef synchronize_rcu
#undef rcu_reader
#undef rcu_gp

#undef get_cpu_call_rcu_data
#undef get_call_rcu_thread
#undef create_call_rcu_data
#undef set_cpu_call_rcu_data
#undef get_default_call_rcu_data
#undef get_call_rcu_data
#undef get_thread_call_rcu_data
#undef set_thread_call_rcu_data
#undef create_all_cpu_call_rcu_data
#undef free_all_cpu_call_rcu_data
#undef call_rcu
#undef call_rcu_data_free
#undef call_rcu_before_fork
#undef call_rcu_after_fork_parent
#undef call_rcu_after_fork_child
#undef rcu_barrier

#undef defer_rcu
#undef rcu_defer_register_thread
#undef rcu_defer_unregister_thread
#undef rcu_defer_barrier

#undef rcu_defer_barrier_thread
#undef rcu_defer_exit

#undef rcu_flavor

#undef urcu_register_rculfhash_atfork
#undef urcu_unregister_rculfhash_atfork

#undef start_poll_synchronize_rcu
#undef poll_state_synchronize_rcu
