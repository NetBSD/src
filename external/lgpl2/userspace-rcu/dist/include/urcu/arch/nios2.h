// SPDX-FileCopyrightText: 2016 Marek Vasut <marex@denx.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_ARCH_NIOS2_H
#define _URCU_ARCH_NIOS2_H

/*
 * arch_nios2.h: trivial definitions for the NIOS2 architecture.
 */

#include <urcu/compiler.h>
#include <urcu/config.h>
#include <urcu/syscall-compat.h>

#ifdef __cplusplus
extern "C" {
#endif

#define cmm_mb()	cmm_barrier()

#ifdef __cplusplus
}
#endif

#include <urcu/arch/generic.h>

#endif /* _URCU_ARCH_NIOS2_H */
