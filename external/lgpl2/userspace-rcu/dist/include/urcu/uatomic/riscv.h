// SPDX-FileCopyrightText: 2018 Michael Jeanson <mjeanson@efficios.com>
//
// SPDX-License-Identifier: MIT

/*
 * Atomic exchange operations for the RISC-V architecture.
 *
 * Let the compiler do it.
 */

#ifndef _URCU_ARCH_UATOMIC_RISCV_H
#define _URCU_ARCH_UATOMIC_RISCV_H

#include <urcu/compiler.h>
#include <urcu/system.h>

/*
 * See <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104831> for details.
 *
 * The following GCC patches are required to have a working Userspace RCU on
 * the RISC-V architecture. The were introduced in GCC 14 and backported to
 * 13.3.0.
 *
 *  - <https://gcc.gnu.org/git/?p=gcc.git;a=commit;h=dcd7b2f5f7233a04c8b14b362d0befa76e9654c0>
 *  - <https://gcc.gnu.org/git/?p=gcc.git;a=commit;h=4990cf84c460f064d6281d0813f20b0ef20c7448>
 *  - <https://gcc.gnu.org/git/?p=gcc.git;a=commit;h=d199d2e56da2379004e7e0457150409c0c99d3e6>
 */
#ifdef URCU_GCC_VERSION
# if (URCU_GCC_VERSION < 130300) && !defined(__NetBSD__)
#  error "Implementations of some atomic operations of GCC < 13.3.0 for RISC-V are insufficient for sequential consistency. For this reason Userspace RCU is currently marked as 'broken' for RISC-V on these GCC versions."
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define UATOMIC_HAS_ATOMIC_BYTE
#define UATOMIC_HAS_ATOMIC_SHORT

#ifdef __cplusplus
}
#endif

#include <urcu/uatomic/generic.h>

#endif /* _URCU_ARCH_UATOMIC_RISCV_H */
