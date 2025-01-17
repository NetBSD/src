// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: MIT

#ifndef _URCU_SYSTEM_H
#define _URCU_SYSTEM_H

/*
 * System definitions.
 */

#include <urcu/config.h>
#include <urcu/compiler.h>
#include <urcu/arch.h>

#ifdef CONFIG_RCU_USE_ATOMIC_BUILTINS

#define CMM_LOAD_SHARED(x)						\
	__atomic_load_n(cmm_cast_volatile(&(x)), __ATOMIC_RELAXED)

#define _CMM_LOAD_SHARED(x) CMM_LOAD_SHARED(x)

#define CMM_STORE_SHARED(x, v)					\
	__extension__						\
	({							\
		__typeof__(v) _v = (v);				\
		__atomic_store_n(cmm_cast_volatile(&(x)), _v,	\
				 __ATOMIC_RELAXED);		\
		_v;						\
	})

#define _CMM_STORE_SHARED(x, v) CMM_STORE_SHARED(x, v)

#else
/*
 * Identify a shared load. A cmm_smp_rmc() or cmm_smp_mc() should come
 * before the load.
 */
#define _CMM_LOAD_SHARED(p)	       CMM_ACCESS_ONCE(p)

/*
 * Load a data from shared memory, doing a cache flush if required.
 */
#define CMM_LOAD_SHARED(p)			\
	__extension__			\
	({				\
		cmm_smp_rmc();		\
		_CMM_LOAD_SHARED(p);	\
	})

/*
 * Identify a shared store. A cmm_smp_wmc() or cmm_smp_mc() should
 * follow the store.
 */
#define _CMM_STORE_SHARED(x, v)	__extension__ ({ CMM_ACCESS_ONCE(x) = (v); })

/*
 * Store v into x, where x is located in shared memory. Performs the
 * required cache flush after writing. Returns v.
 */
#define CMM_STORE_SHARED(x, v)						\
	__extension__							\
	({								\
		__typeof__(x) _v = _CMM_STORE_SHARED(x, v);		\
		cmm_smp_wmc();						\
		_v = _v;	/* Work around clang "unused result" */	\
	})

#endif	/* CONFIG_RCU_USE_ATOMIC_BUILTINS */

#endif /* _URCU_SYSTEM_H */
