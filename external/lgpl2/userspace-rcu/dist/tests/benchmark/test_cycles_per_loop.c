// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Userspace RCU library - test cycles per loop
 */

#include <urcu/arch.h>
#include <stdio.h>

#define NR_LOOPS 1000000UL

static inline void loop_sleep(unsigned long loops)
{
	while (loops-- != 0)
		caa_cpu_relax();
}

int main()
{
	caa_cycles_t time1, time2;

	time1 = caa_get_cycles();
	loop_sleep(NR_LOOPS);
	time2 = caa_get_cycles();
	printf("CPU clock cycles per loop: %g\n", (time2 - time1) /
						  (double)NR_LOOPS);
	return 0;
}
