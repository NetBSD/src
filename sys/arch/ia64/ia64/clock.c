/*	$NetBSD: clock.c,v 1.6 2009/07/20 04:41:36 kiyohara Exp $	*/

/*
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 *
 * Author:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Comments about functions from alpha/clock.c */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <machine/clock.h>
#include <machine/cpu.h>
#include <machine/ia64_cpu.h>


uint64_t ia64_clock_reload;


#if !defined(MULTIPROCESSOR)
static unsigned
ia64_get_timecount(struct timecounter* tc)
{

	return ia64_get_itc();
}
#endif

void
pcpu_initclock(void)
{
	struct cpu_info *ci = curcpu();

	ci->ci_clockadj = 0;
	ci->ci_clock = ia64_get_itc();
	ia64_set_itm(ci->ci_clock + ia64_clock_reload);
#if 0
	ia64_set_itv(CLOCK_VECTOR);	/* highest priority class */
#endif
	ia64_srlz_d();
}

/*
 * Start the real-time and statistics clocks. We use cr.itc and cr.itm
 * to implement a 1000hz clock.
 */
void
cpu_initclocks(void)
{
#if !defined(MULTIPROCESSOR)
	static struct timecounter tc =  {
		(timecounter_get_t *)ia64_get_timecount, /* get_timecount */
		0,				/* no poll_pps */
		~0u,				/* counter_mask */
		0,				/* frequency */
		"ia64_timecounter",		/* name */
		100,				/* quality */
	};
#endif

	if (itc_frequency == 0)
		panic("Unknown clock frequency");

	stathz = hz;
	ia64_clock_reload = (itc_frequency + hz / 2) / hz;

#if !defined(MULTIPROCESSOR)
	tc.tc_frequency = itc_frequency;
	tc_init(&tc);
#endif

	pcpu_initclock();
}

void
setstatclockrate(int newhz)
{

	/* nothing to do? */
}
