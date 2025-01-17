// SPDX-FileCopyrightText: 2021 Michael Jeanson <mjeanson@efficios.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdio.h>
#include <urcu/arch.h>

#include "tap.h"

#define NR_TESTS 1


/*
 * This is only to make sure the static inline caa_get_cycles() in the public
 * headers builds properly.
 */
static
void test_caa_get_cycles(void) {
	caa_cycles_t cycles = 0;


	cycles = caa_get_cycles();

	ok(cycles != 0, "caa_get_cycles works");
}

int main(void)
{
	plan_tests(NR_TESTS);

	test_caa_get_cycles();

	return exit_status();
}
