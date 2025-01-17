/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2022 Michael Jeanson <mjeanson@efficios.com>
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef __linux__

#include "compat-smp.h"

int main(int argc, char *argv[])
{
	int ret;

	if( argc < 2 ) {
		fprintf(stderr, "Missing argument.\n");
		return EXIT_FAILURE;
	}

	ret = _get_max_cpuid_from_sysfs(argv[1]);

	printf("%d\n", ret);

	if (ret >= 0)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}

#else

int main(void)
{
	return EXIT_SUCCESS;
}
#endif
