// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_H
#define _URCU_H

/*
 * Userspace RCU header
 *
 * LGPL-compatible code should include this header with :
 *
 * #define _LGPL_SOURCE
 * #include <urcu.h>
 *
 * IBM's contributions to this file may be relicensed under LGPLv2 or later.
 */

#if !defined(RCU_MEMBARRIER) && !defined(RCU_MB)
#define RCU_MEMBARRIER
#endif

#ifdef RCU_MEMBARRIER
#include <urcu/urcu-memb.h>
#elif defined(RCU_MB)
#include <urcu/urcu-mb.h>
#else
#error "Unknown urcu flavor"
#endif

#endif /* _URCU_H */
