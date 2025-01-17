// SPDX-FileCopyrightText: 2017 Michael Jeanson <mjeanson@efficios.com>
//
// SPDX-License-Identifier: MIT

/*
 * Atomic exchange operations for the m68k architecture. Let GCC do it.
 */

#ifndef _URCU_ARCH_UATOMIC_M68K_H
#define _URCU_ARCH_UATOMIC_M68K_H

#include <urcu/compiler.h>
#include <urcu/system.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UATOMIC_HAS_ATOMIC_BYTE
#define UATOMIC_HAS_ATOMIC_SHORT

#ifdef __cplusplus
}
#endif

#include <urcu/uatomic/generic.h>

#endif /* _URCU_ARCH_UATOMIC_M68K_H */
