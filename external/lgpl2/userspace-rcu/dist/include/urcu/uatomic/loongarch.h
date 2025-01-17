// SPDX-FileCopyrightText: 2021 Wang Jing <wangjing@loongson.cn>
//
// SPDX-License-Identifier: MIT

#ifndef _URCU_UATOMIC_ARCH_LOONGARCH_H
#define _URCU_UATOMIC_ARCH_LOONGARCH_H

/*
 * Atomic exchange operations for the LoongArch architecture. Let GCC do it.
 */

#include <urcu/compiler.h>
#include <urcu/system.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * LoongArch implements byte and short atomics with LL/SC instructions,
 * which retry if the cache line is modified concurrently between LL and
 * SC.
 */
#define UATOMIC_HAS_ATOMIC_BYTE
#define UATOMIC_HAS_ATOMIC_SHORT

#ifdef __cplusplus
}
#endif

#include <urcu/uatomic/generic.h>

#endif /* _URCU_UATOMIC_ARCH_LOONGARCH_H */
