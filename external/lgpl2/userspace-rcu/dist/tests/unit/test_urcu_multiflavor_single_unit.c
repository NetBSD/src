// SPDX-FileCopyrightText: 2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Userspace RCU library - test multiple RCU flavors into one program
 */

#ifndef DYNAMIC_LINK_TEST
#define _LGPL_SOURCE
#endif

#include <urcu/urcu-mb.h>
#include <urcu/urcu-bp.h>
#include <urcu/urcu-memb.h>
#include <urcu/urcu-qsbr.h>

#include <stdlib.h>
#include "tap.h"

static int test_mf_mb(void)
{
	urcu_mb_register_thread();
	urcu_mb_read_lock();
	urcu_mb_read_unlock();
	urcu_mb_synchronize_rcu();
	urcu_mb_unregister_thread();
	return 0;
}

static int test_mf_bp(void)
{
	urcu_bp_register_thread();
	urcu_bp_read_lock();
	urcu_bp_read_unlock();
	urcu_bp_synchronize_rcu();
	urcu_bp_unregister_thread();
	return 0;
}

static int test_mf_memb(void)
{
	urcu_memb_register_thread();
	urcu_memb_read_lock();
	urcu_memb_read_unlock();
	urcu_memb_synchronize_rcu();
	urcu_memb_unregister_thread();
	return 0;
}

static int test_mf_qsbr(void)
{
	urcu_qsbr_register_thread();
	urcu_qsbr_read_lock();
	urcu_qsbr_read_unlock();
	urcu_qsbr_synchronize_rcu();
	urcu_qsbr_unregister_thread();
	return 0;
}

int main(void)
{
	plan_tests(4);

	ok1(!test_mf_mb());
	ok1(!test_mf_bp());
	ok1(!test_mf_memb());
	ok1(!test_mf_qsbr());

	return exit_status();
}
