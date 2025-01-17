// SPDX-FileCopyrightText: 2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Userspace RCU library - test multiple RCU flavors into one program
 */

#include <stdlib.h>
#include "test_urcu_multiflavor.h"

#include "tap.h"

int main(void)
{
	plan_tests(4);

	ok1(!test_mf_memb());

	ok1(!test_mf_mb());
	ok1(!test_mf_qsbr());
	ok1(!test_mf_bp());

	return exit_status();
}
