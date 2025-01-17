// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_POINTER_STATIC_H
#define _URCU_POINTER_STATIC_H

/*
 * Userspace RCU header. Operations on pointers.
 *
 * TO BE INCLUDED ONLY IN CODE THAT IS TO BE RECOMPILED ON EACH LIBURCU
 * RELEASE. See urcu.h for linking dynamically with the userspace rcu library.
 *
 * IBM's contributions to this file may be relicensed under LGPLv2 or later.
 */

#include <urcu/compiler.h>
#include <urcu/arch.h>
#include <urcu/system.h>
#include <urcu/uatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * _rcu_dereference - reads (copy) a RCU-protected pointer to a local variable
 * into a RCU read-side critical section. The pointer can later be safely
 * dereferenced within the critical section.
 *
 * This ensures that the pointer copy is invariant thorough the whole critical
 * section.
 *
 * Inserts memory barriers on architectures that require them (currently only
 * Alpha) and documents which pointers are protected by RCU.
 *
 * With C standards prior to C11/C++11, the compiler memory barrier in
 * CMM_LOAD_SHARED() ensures that value-speculative optimizations (e.g.
 * VSS: Value Speculation Scheduling) does not perform the data read
 * before the pointer read by speculating the value of the pointer.
 * Correct ordering is ensured because the pointer is read as a volatile
 * access. This acts as a global side-effect operation, which forbids
 * reordering of dependent memory operations.
 *
 * With C standards C11/C++11, concerns about dependency-breaking
 * optimizations are taken care of by the "memory_order_consume" atomic
 * load.
 *
 * Use the gcc __atomic_load() rather than C11/C++11 atomic load
 * explicit because the pointer used as input argument is a pointer,
 * not an _Atomic type as required by C11/C++11.
 *
 * By defining URCU_DEREFERENCE_USE_VOLATILE, the user requires use of
 * volatile access to implement rcu_dereference rather than
 * memory_order_consume load from the C11/C++11 standards.
 *
 * CAUTION: If URCU_DEREFERENCE_USE_VOLATILE is defined, pointers comparisons
 * _must_ be done using the `cmm_ptr_eq' static inline helper function to
 * prevent the compiler optimizations from losing addresses dependencies,
 * resulting in the loss of the publication guarantee.
 *
 * This may improve performance on weakly-ordered architectures where
 * the compiler implements memory_order_consume as a
 * memory_order_acquire, which is stricter than required by the
 * standard.
 *
 * Note that using volatile accesses for rcu_dereference may cause
 * LTO to generate incorrectly ordered code starting from C11/C++11.
 *
 * Should match rcu_assign_pointer() or rcu_xchg_pointer().
 *
 * This macro is less than 10 lines long.  The intent is that this macro
 * meets the 10-line criterion in LGPL, allowing this function to be
 * expanded directly in non-LGPL code.
 */
#ifdef URCU_DEREFERENCE_USE_VOLATILE
#  define _rcu_dereference(p)			\
	__extension__				\
	({					\
		cmm_smp_rmc();			\
		CMM_ACCESS_ONCE(p);		\
	})
#else
#  define _rcu_dereference(p)			\
	uatomic_load(&(p), CMM_CONSUME)
#endif

/**
 * Compare pointers A and B, but prevent the compiler from using this knowledge
 * for optimization.
 *
 * This may introduce some overhead. For example, compilers might allocate more
 * register, resulting in register spilling.
 *
 * For example, in the following code, `cmm_ptr_eq' prevents the compiler from
 * re-using the derefence of `a' at the first call of `do_func()' for the second
 * call. If a simple equality comparison was used, the compiler could use the
 * value of `global->x' across two RCU critical regions.
 *
 * struct work {
 * 	long x;
 * }
 *
 * struct work *global;
 *
 * void func(void)
 * {
 * 	struct work *a, *b;
 *
 * 	rcu_read_lock();
 * 	a = rcu_dereference(global);
 * 	if (a)
 * 		do_func(a->x);
 * 	rcu_read_unlock();
 *
 * 	some_stuff();
 *
 * 	rcu_read_lock();
 * 	b = rcu_dereference(global);
 * 	if (b && cmm_ptr_eq(a, b)) // force to use b->x and not a->x
 * 		do_func(b->x);
 * 	rcu_read_unlock();
 * }
 *
 * Consider what could happen if such compiler optimization was done while the
 * following mutator exists. The mutator removes a `work' node that was
 * previously stored in a global location by storing NULL. It then call
 * rcu_synchronize() to ensure no readers can see the node. From there, a new
 * work is created for the node and the node is commited with
 * rcu_assign_pointer(), thus the same pointer is now visible to readers. This
 * fictional mutator is purposely trying to create a ABA problem for the sake of
 * the argument:
 *
 * void mutator(void)
 * {
 * 	struct work *work;
 *
 * 	work = global;
 * 	rcu_assign_pointer(global, NULL);
 * 	rcu_synchronize();
 * 	work->x = new_work();
 * 	rcu_assign_pointer(global, work);
 * }
 *
 * Then, the following execution could happen, where the new work assigned is
 * not executed. In other words, the compiler optimizations can introduce a ABA
 * problem, defeating one of RCU's purposes.
 *
 *   Worker:                               Mutator:
 *     let a, b                              let work = global
 *    let tmp
 *     rcu_read_lock()
 *     a = rcu_dereference(global)
 *     if (a) {
 *       tmp = a->x
 *       do_func(tmp)
 *     }
 *     rcu_read_unlock()
 *     do_stuff() ...
 *       ...                                 rcu_assign_pointer(global, NULL)
 *       ...                                 rcu_synchronize()
 *       ...                                 work->x = new_work()
 *       ...                                 rcu_assign_pointer(global, work)
 *     rcu_read_lock()
 *     b = rcu_dereference(global)
 *     if (b && (a == b))  // BUGGY !!!
 *       do_func(tmp)
 *     rcu_read_unlock()
 *
 * In other words, the publication guarantee is lost here.  The store to
 * `work->x' in the mutator is supposed to be observable by the second
 * rcu_dereference() in the worker, because of the matching
 * rcu_assign_pointer().  But due to the equality comparison, the compiler
 * decides to reuse the prior rcu_dereference().
 */
static inline int cmm_ptr_eq(const void *a, const void *b)
{
	_CMM_OPTIMIZER_HIDE_VAR(a);
	_CMM_OPTIMIZER_HIDE_VAR(b);

	return a == b;
}

/**
 * _rcu_cmpxchg_pointer - same as rcu_assign_pointer, but tests if the pointer
 * is as expected by "old". If succeeds, returns the previous pointer to the
 * data structure, which can be safely freed after waiting for a quiescent state
 * using synchronize_rcu(). If fails (unexpected value), returns old (which
 * should not be freed !).
 *
 * uatomic_cmpxchg() acts as both release and acquire barriers on success.
 *
 * This macro is less than 10 lines long.  The intent is that this macro
 * meets the 10-line criterion in LGPL, allowing this function to be
 * expanded directly in non-LGPL code.
 */
#define _rcu_cmpxchg_pointer(p, old, _new)				\
	__extension__							\
	({								\
		__typeof__(*p) _________pold = (old);			\
		__typeof__(*p) _________pnew = (_new);			\
		uatomic_cmpxchg_mo(p, _________pold, _________pnew,	\
				   CMM_SEQ_CST, CMM_RELAXED);		\
	});

/**
 * _rcu_xchg_pointer - same as rcu_assign_pointer, but returns the previous
 * pointer to the data structure, which can be safely freed after waiting for a
 * quiescent state using synchronize_rcu().
 *
 * uatomic_xchg() acts as both release and acquire barriers.
 *
 * This macro is less than 10 lines long.  The intent is that this macro
 * meets the 10-line criterion in LGPL, allowing this function to be
 * expanded directly in non-LGPL code.
 */
#define _rcu_xchg_pointer(p, v)				\
	__extension__					\
	({						\
		__typeof__(*p) _________pv = (v);	\
		uatomic_xchg_mo(p, _________pv,		\
				CMM_SEQ_CST);		\
	})


#define _rcu_set_pointer(p, v)						\
	do {								\
		__typeof__(*p) _________pv = (v);			\
		uatomic_store(p, _________pv,				\
			__builtin_constant_p(v) && (v) == NULL ?	\
			CMM_RELAXED : CMM_RELEASE);			\
	} while (0)

/**
 * _rcu_assign_pointer - assign (publicize) a pointer to a new data structure
 * meant to be read by RCU read-side critical sections. Returns the assigned
 * value.
 *
 * Documents which pointers will be dereferenced by RCU read-side critical
 * sections and adds the required memory barriers on architectures requiring
 * them. It also makes sure the compiler does not reorder code initializing the
 * data structure before its publication.
 *
 * Should match rcu_dereference().
 *
 * This macro is less than 10 lines long.  The intent is that this macro
 * meets the 10-line criterion in LGPL, allowing this function to be
 * expanded directly in non-LGPL code.
 */
#define _rcu_assign_pointer(p, v)	_rcu_set_pointer(&(p), v)

#ifdef __cplusplus
}
#endif

#endif /* _URCU_POINTER_STATIC_H */
