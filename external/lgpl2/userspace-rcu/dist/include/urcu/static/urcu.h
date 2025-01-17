// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_STATIC_H
#define _URCU_STATIC_H

/*
 * Userspace RCU header.
 *
 * TO BE INCLUDED ONLY IN CODE THAT IS TO BE RECOMPILED ON EACH LIBURCU
 * RELEASE. See urcu.h for linking dynamically with the userspace rcu library.
 *
 * IBM's contributions to this file may be relicensed under LGPLv2 or later.
 */

/* Default is RCU_MEMBARRIER */
#if !defined(RCU_MEMBARRIER) && !defined(RCU_MB)
#define RCU_MEMBARRIER
#endif

#ifdef RCU_MEMBARRIER
#include <urcu/static/urcu-memb.h>
#endif

#ifdef RCU_MB
#include <urcu/static/urcu-mb.h>
#endif

#endif /* _URCU_STATIC_H */
