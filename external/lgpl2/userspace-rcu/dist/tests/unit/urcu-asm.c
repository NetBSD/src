// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * urcu-asm.c
 *
 * Userspace RCU library - assembly dump of primitives
 */

#include <urcu.h>

void show_read_lock(void)
{
	asm volatile ("/* start */");
	rcu_read_lock();
	asm volatile ("/* end */");
}

void show_read_unlock(void)
{
	asm volatile ("/* start */");
	rcu_read_unlock();
	asm volatile ("/* end */");
}
