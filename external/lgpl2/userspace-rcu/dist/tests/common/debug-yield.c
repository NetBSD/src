// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
// SPDX-FileCopyrightText: 2009 Paul E. McKenney, IBM Corporation.
//
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * Userspace RCU library tests - Debugging code
 *
 * IBM's contributions to this file may be relicensed under LGPLv2 or later.
 */

#include <urcu/tls-compat.h>
#include "debug-yield.h"

unsigned int rcu_yield_active;

DEFINE_URCU_TLS(unsigned int, rcu_rand_yield);
