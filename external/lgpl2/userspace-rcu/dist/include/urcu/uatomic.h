// SPDX-FileCopyrightText: 2020 Michael Jeanson <mjeanson@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_UATOMIC_H
#define _URCU_UATOMIC_H

#include <urcu/arch.h>
#include <urcu/compiler.h>
#include <urcu/config.h>

enum cmm_memorder {
	CMM_RELAXED = 0,
	CMM_CONSUME = 1,
	CMM_ACQUIRE = 2,
	CMM_RELEASE = 3,
	CMM_ACQ_REL = 4,
	CMM_SEQ_CST = 5,
	CMM_SEQ_CST_FENCE = 6,
};

#if defined(_CMM_TOOLCHAIN_SUPPORT_C11_MM)

/*
 * Make sure that CMM_SEQ_CST_FENCE is not equivalent to other memory orders.
 */
urcu_static_assert(CMM_RELAXED == __ATOMIC_RELAXED, "CMM_RELAXED vs __ATOMIC_RELAXED values mismatch", cmm_relaxed_values_mismatch);
urcu_static_assert(CMM_CONSUME == __ATOMIC_CONSUME, "CMM_CONSUME vs __ATOMIC_CONSUME values mismatch", cmm_consume_values_mismatch);
urcu_static_assert(CMM_ACQUIRE == __ATOMIC_ACQUIRE, "CMM_ACQUIRE vs __ATOMIC_ACQUIRE values mismatch", cmm_acquire_values_mismatch);
urcu_static_assert(CMM_RELEASE == __ATOMIC_RELEASE, "CMM_RELEASE vs __ATOMIC_RELEASE values mismatch", cmm_release_values_mismatch);
urcu_static_assert(CMM_ACQ_REL == __ATOMIC_ACQ_REL, "CMM_ACQ_REL vs __ATOMIC_ACQ_REL values mismatch", cmm_acq_rel_values_mismatch);
urcu_static_assert(CMM_SEQ_CST == __ATOMIC_SEQ_CST, "CMM_SEQ_CST vs __ATOMIC_SEQ_CST values mismatch", cmm_seq_cst_values_mismatch);

/*
 * This is not part of the public API. It it used internally to implement the
 * CMM_SEQ_CST_FENCE memory order.
 *
 * NOTE: Using switch here instead of if statement to avoid -Wduplicated-cond
 * warning when memory order is conditionally determined.
 */
static inline void cmm_seq_cst_fence_after_atomic(enum cmm_memorder mo)
{
	switch (mo) {
	case CMM_SEQ_CST_FENCE:
		cmm_smp_mb();
		break;
	default:
		break;
	}
}

#endif

/*
 * This is not part of the public API. It is used internally to convert from the
 * CMM memory model to the C11 memory model.
 */
static inline int cmm_to_c11(int mo)
{
	if (mo == CMM_SEQ_CST_FENCE) {
		return CMM_SEQ_CST;
	}
	return mo;
}

#include <urcu/uatomic/api.h>

#if defined(CONFIG_RCU_USE_ATOMIC_BUILTINS)
#include <urcu/uatomic/builtins.h>
#elif defined(URCU_ARCH_X86)
#include <urcu/uatomic/x86.h>
#elif defined(URCU_ARCH_PPC)
#include <urcu/uatomic/ppc.h>
#elif defined(URCU_ARCH_S390)
#include <urcu/uatomic/s390.h>
#elif defined(URCU_ARCH_SPARC64)
#include <urcu/uatomic/sparc64.h>
#elif defined(URCU_ARCH_ALPHA)
#include <urcu/uatomic/alpha.h>
#elif defined(URCU_ARCH_IA64)
#include <urcu/uatomic/ia64.h>
#elif defined(URCU_ARCH_ARM)
#include <urcu/uatomic/arm.h>
#elif defined(URCU_ARCH_AARCH64)
#include <urcu/uatomic/aarch64.h>
#elif defined(URCU_ARCH_MIPS)
#include <urcu/uatomic/mips.h>
#elif defined(URCU_ARCH_NIOS2)
#include <urcu/uatomic/nios2.h>
#elif defined(URCU_ARCH_TILE)
#include <urcu/uatomic/tile.h>
#elif defined(URCU_ARCH_HPPA)
#include <urcu/uatomic/hppa.h>
#elif defined(URCU_ARCH_M68K)
#include <urcu/uatomic/m68k.h>
#elif defined(URCU_ARCH_RISCV)
#include <urcu/uatomic/riscv.h>
#elif defined(URCU_ARCH_LOONGARCH)
#include <urcu/uatomic/loongarch.h>
#elif defined(URCU_ARCH_SH3)
#include <urcu/uatomic/sh3.h>
#elif defined(URCU_ARCH_VAX)
#include <urcu/uatomic/vax.h>
#else
#error "Cannot build: unrecognized architecture, see <urcu/arch.h>."
#endif

#endif /* _URCU_UATOMIC_H */
