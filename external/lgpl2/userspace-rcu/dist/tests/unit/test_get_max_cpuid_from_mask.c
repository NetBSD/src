/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2022 Michael Jeanson <mjeanson@efficios.com>
 */

#include <string.h>

#include "tap.h"

#include "compat-smp.h"

#ifdef __linux__
struct parse_test_data {
	const char *buf;
	int expected;
};

static struct parse_test_data parse_test_data[] = {
	{ "", -1 },
	{ "abc", -1 },
	{ ",,,", -1 },
	{ "--", -1 },
	{ ",", -1 },
	{ "-", -1 },
	{ "2147483647", -1 },
	{ "18446744073709551615", -1 },
	{ "0-2147483647", -1 },
	{ "0-18446744073709551615", -1 },
	{ "0", 0 },
	{ "1", 1 },
	{ "0-1", 1 },
	{ "1-3", 3 },
	{ "0,2", 2 },
	{ "1,2", 2 },
	{ "0,4-6,127", 127 },
	{ "0-4095", 4095 },

	{ "\n", -1 },
	{ "abc\n", -1 },
	{ ",,,\n", -1 },
	{ "--\n", -1 },
	{ ",\n", -1 },
	{ "-\n", -1 },
	{ "2147483647\n", -1 },
	{ "18446744073709551615\n", -1 },
	{ "0-2147483647\n", -1 },
	{ "0-18446744073709551615\n", -1 },
	{ "0\n", 0 },
	{ "1\n", 1 },
	{ "0-1\n", 1 },
	{ "1-3\n", 3 },
	{ "0,2\n", 2 },
	{ "1,2\n", 2 },
	{ "0,4-6,127\n", 127 },
	{ "0-4095\n", 4095 },
};

static int parse_test_data_len = sizeof(parse_test_data) / sizeof(parse_test_data[0]);

int main(void)
{
	int ret, i;

	plan_tests(parse_test_data_len);

	diag("Testing smp helpers");

	for (i = 0; i < parse_test_data_len; i++) {
		ret = get_max_cpuid_from_mask(parse_test_data[i].buf,
				strlen(parse_test_data[i].buf));
		ok(ret == parse_test_data[i].expected,
			"get_max_cpuid_from_mask '%s', expected: '%d', result: '%d'",
			parse_test_data[i].buf, parse_test_data[i].expected, ret);
	}

	return exit_status();
}

#else

int main(void)
{
	plan_skip_all("Linux specific tests.");

	return exit_status();
}
#endif
