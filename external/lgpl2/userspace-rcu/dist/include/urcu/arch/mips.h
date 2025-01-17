// SPDX-FileCopyrightText: 2010 Paolo Bonzini <pbonzini@redhat.com>
// SPDX-FileCopyrightText: 2012 Ralf Baechle <ralf@linux-mips.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_ARCH_MIPS_H
#define _URCU_ARCH_MIPS_H

/*
 * arch_mips.h: trivial definitions for the MIPS architecture.
 */

#include <urcu/compiler.h>
#include <urcu/config.h>
#include <urcu/syscall-compat.h>

#ifdef __cplusplus
extern "C" {
#endif

#define cmm_mb()			__asm__ __volatile__ (		    \
					"	.set	mips2		\n" \
					"	sync			\n" \
					"	.set	mips0		\n" \
					:::"memory")

#ifdef __cplusplus
}
#endif

#include <urcu/arch/generic.h>

#endif /* _URCU_ARCH_MIPS_H */
