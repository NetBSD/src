/*	$NetBSD: sh5_clock.c,v 1.10.2.1 2006/11/18 21:29:31 ad Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1992, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)clock.c     8.1 (Berkeley) 6/11/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sh5_clock.c,v 1.10.2.1 2006/11/18 21:29:31 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/cpu.h>

#include <sh5/sh5/clockvar.h>

/*
 * Statistics clock interval and variance, in ticks.  Variance must be a
 * power of two.  Since this gives us an even number.
 * We discard one case and compensate.  That is, a variance of 8192 would
 * give us offsets in [0..8191].  Instead, we take offsets in [1..8191].
 * This is symmetric about the point 4096, or statvar/2, and thus averages
 * to that value (assuming uniform random numbers).
 */
static u_int clock_statvar = 8192;
static u_int clock_statmin;	/* statclock interval - (1/2 * variance) */

static struct evcnt clock_hardcnt;
static struct evcnt clock_statcnt;

static struct clock_attach_args *clock_args;

/*
 * Common parts of clock autoconfiguration
 */
void
clock_config(struct device *dev, struct clock_attach_args *ca, struct evcnt *ev)
{

	clock_args = ca;

	evcnt_attach_dynamic(&clock_hardcnt, EVCNT_TYPE_INTR, ev,
	    dev->dv_xname, "hardcnt");

	if (clock_args->ca_has_stat_clock)
		evcnt_attach_dynamic(&clock_statcnt, EVCNT_TYPE_INTR, ev,
		    dev->dv_xname, "statcnt");
}

/*
 * Set up the real-time and statistics clocks.
 */
void
cpu_initclocks(void)
{

	if (clock_args == NULL)
		panic("cpu_initclocks: clock not configured");

#if 0
	if ((clock_args->ca_rate % hz) != 0)
		panic("cpu_initclocks: Impossible clock rate: %dHz", hz);
#endif

	if (clock_args->ca_has_stat_clock) {
		u_int minint, statint;

		if (stathz == 0)
			stathz = hz;
#if 0
		else
		if ((clock_args->ca_rate % stathz) != 0)
			panic("cpu_initclocks: Impossible statclock rate: %dHz",
			    hz);
#endif
		profhz = stathz;

		statint = clock_args->ca_rate / stathz;
		minint = statint / 2 + 100;
		while (clock_statvar > minint)
			clock_statvar >>= 1;
		clock_statmin = statint - (clock_statvar >> 1);
		(*clock_args->ca_start)(clock_args->ca_arg, CLK_STATCLOCK,
		    statint);
	}

	(*clock_args->ca_start)(clock_args->ca_arg, CLK_HARDCLOCK, tick);
}

/*ARGSUSED*/
void
setstatclockrate(int newhz)
{

	/* Nothing to do. Stat clock already randomised in clock_statint() */
}

void
clock_hardint(struct clockframe *cf)
{

	hardclock(cf);

	clock_hardcnt.ev_count++;
}

void
clock_statint(struct clockframe *cf)
{
	u_int newint;
	int r;

	statclock(cf);

	clock_statcnt.ev_count++;

	/* Generate a new randomly-distributed clock period. */
	do {
		r = random() & (clock_statvar - 1);
	} while (r == 0);
	newint = clock_statmin + r;

	/*
	 * Load the next clock period into the latch.
	 * It'll be used for the _next_ statclock reload.
	 */
	(*clock_args->ca_start)(clock_args->ca_arg, CLK_STATCLOCK, newint);
}
