// SPDX-FileCopyrightText: 2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Userspace RCU library - test multiple RCU flavors into one program
 */

#ifndef DYNAMIC_LINK_TEST
#define _LGPL_SOURCE
#endif

#include <urcu-bp.h>
#include "test_urcu_multiflavor.h"

int test_mf_bp(void)
{
	rcu_register_thread();
	rcu_read_lock();
	rcu_read_unlock();
	synchronize_rcu();
	rcu_unregister_thread();
	return 0;
}
