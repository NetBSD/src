// SPDX-FileCopyrightText: 1991-1994 by Xerox Corporation.  All rights reserved.
// SPDX-FileCopyrightText: 1996-1999 by Silicon Graphics.  All rights reserved.
// SPDX-FileCopyrightText: 1999-2004 Hewlett-Packard Development Company, L.P.
// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2010 Paul E. McKenney, IBM Corporation
//
// SPDX-License-Identifier: LicenseRef-Boehm-GC

#ifndef _URCU_ARCH_UATOMIC_GCC_H
#define _URCU_ARCH_UATOMIC_GCC_H

/*
 * Code inspired from libuatomic_ops-1.2, inherited in part from the
 * Boehm-Demers-Weiser conservative garbage collector.
 */

#include <urcu/compiler.h>
#include <urcu/system.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * If your platform doesn't have a full set of atomics, you will need
 * a separate uatomic_arch_*.h file for your architecture.  Otherwise,
 * just rely on the definitions in uatomic/generic.h.
 */
#define UATOMIC_HAS_ATOMIC_BYTE
#define UATOMIC_HAS_ATOMIC_SHORT

#ifdef __cplusplus
}
#endif

#include <urcu/uatomic/generic.h>

#endif /* _URCU_ARCH_UATOMIC_GCC_H */
