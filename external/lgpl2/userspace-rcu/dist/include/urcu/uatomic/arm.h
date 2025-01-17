// SPDX-FileCopyrightText: 1991-1994 by Xerox Corporation.  All rights reserved.
// SPDX-FileCopyrightText: 1996-1999 by Silicon Graphics.  All rights reserved.
// SPDX-FileCopyrightText: 1999-2004 Hewlett-Packard Development Company, L.P.
// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2010 Paul E. McKenney, IBM Corporation
//
// SPDX-License-Identifier: LicenseRef-Boehm-GC

#ifndef _URCU_ARCH_UATOMIC_ARM_H
#define _URCU_ARCH_UATOMIC_ARM_H

/*
 * Atomics for ARM.  This approach is usable on kernels back to 2.6.15.
 *
 * Code inspired from libuatomic_ops-1.2, inherited in part from the
 * Boehm-Demers-Weiser conservative garbage collector.
 */

#include <urcu/compiler.h>
#include <urcu/system.h>
#include <urcu/arch.h>

#ifdef __cplusplus
extern "C" {
#endif

/* xchg */

/*
 * If the toolchain supports the C11 memory model, then it is safe to implement
 * `uatomic_xchg()' in term of __atomic builtins.  This has the effect of
 * reducing the number of emitted memory barriers except for the
 * CMM_SEQ_CST_FENCE memory order.
 */
#ifdef _CMM_TOOLCHAIN_SUPPORT_C11_MM
#  define uatomic_xchg_mo(addr, v, mo)					\
	__extension__							\
	({								\
		__typeof__((*addr)) _old =				\
			__atomic_exchange_n(cmm_cast_volatile(addr), v,	\
					cmm_to_c11(mo));		\
		cmm_seq_cst_fence_after_atomic(mo);			\
		_old;							\
	})
#else

static inline void _cmm_compat_c11_smp_mb__before_xchg_mo(enum cmm_memorder mo)
{
	switch (mo) {
	case CMM_SEQ_CST_FENCE:
	case CMM_SEQ_CST:
	case CMM_ACQ_REL:
	case CMM_RELEASE:
		cmm_smp_mb();
		break;
	case CMM_ACQUIRE:
	case CMM_CONSUME:
	case CMM_RELAXED:
		break;
	default:
		abort();
	}
}

/*
 * Based on [1], __sync_lock_test_and_set() is not a full barrier, but
 * instead only an acquire barrier. Given that uatomic_xchg() acts as
 * both release and acquire barriers, we therefore need to have our own
 * release barrier before this operation.
 *
 * [1] https://gcc.gnu.org/onlinedocs/gcc-4.1.0/gcc/Atomic-Builtins.html
 */
#  define uatomic_xchg_mo(addr, v, mo)				\
	({							\
		_cmm_compat_c11_smp_mb__before_xchg_mo(mo);	\
		__sync_lock_test_and_set(addr, v);		\
	})
#endif	/* _CMM_TOOLCHAIN_SUPPORT_C11_MM */

#ifdef __cplusplus
}
#endif

#include <urcu/uatomic/generic.h>

#endif /* _URCU_ARCH_UATOMIC_ARM_H */
