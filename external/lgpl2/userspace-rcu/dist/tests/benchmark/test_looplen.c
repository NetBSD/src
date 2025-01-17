// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Userspace RCU library - test program
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <sched.h>
#include <errno.h>

#include <urcu/arch.h>
#include <urcu/assert.h>

#ifndef DYNAMIC_LINK_TEST
#define _LGPL_SOURCE
#else
#define debug_yield_read()
#endif
#include <urcu.h>

static inline void loop_sleep(unsigned long loops)
{
	while (loops-- != 0)
		caa_cpu_relax();
}

#define LOOPS 1048576
#define TESTS 10

int main(void)
{
	unsigned long i;
	caa_cycles_t time1, time2;
	caa_cycles_t time_tot = 0;
	double cpl;

	for (i = 0; i < TESTS; i++) {
		time1 = caa_get_cycles();
		loop_sleep(LOOPS);
		time2 = caa_get_cycles();
		time_tot += time2 - time1;
	}
	cpl = ((double)time_tot) / (double)TESTS / (double)LOOPS;

	printf("CALIBRATION : %g cycles per loop\n", cpl);
	printf("time_tot = %llu, LOOPS = %d, TESTS = %d\n",
	       (unsigned long long) time_tot, LOOPS, TESTS);

	return 0;
}
