/*	$NetBSD: sh5_clock.c,v 1.7 2003/08/07 16:29:32 agc Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sh5_clock.c,v 1.7 2003/08/07 16:29:32 agc Exp $");

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
static todr_chip_handle_t todr_handle;


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

void
todr_attach(todr_chip_handle_t todr)
{

	if (todr_handle)
		panic("todr_attach: rtc already configured");

	todr_handle = todr;
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

/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  We do this by returning the current time
 * plus the amount of time, in uSec, since the last clock interrupt
 * (clock_args->ca_microtime()) was handled.
 *
 * Check that this time is no less than any previously-reported time,
 * which could happen around the time of a clock adjustment.  Just for fun,
 * we guarantee that the time will be greater than the value obtained by a
 * previous call.
 */

void
microtime(struct timeval *tvp)
{
	int s = splhigh();
	static struct timeval lasttime;

	*tvp = time;
	tvp->tv_usec += (*clock_args->ca_microtime)(clock_args->ca_arg);
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}

/*
 * Set up the system's time, given a `reasonable' time value.
 */
void
inittodr(time_t base)
{
        int badbase = 0, waszero = (base == 0);

	if (todr_handle == NULL)
		panic("inittodr: todr not configured");

        if (base < 5 * SECYR) {
                /*
                 * If base is 0, assume filesystem time is just unknown
                 * in stead of preposterous. Don't bark.
                 */
                if (base != 0)
                        printf("WARNING: preposterous time in file system\n");
                /* not going to use it anyway, if the chip is readable */
                base = 21*SECYR + 186*SECDAY + SECDAY/2;
                badbase = 1;
        }

        if (todr_gettime(todr_handle, (struct timeval *)&time) != 0 ||
            time.tv_sec == 0) {
badrtc:
                printf("WARNING: bad date in battery clock");
                /*
                 * Believe the time in the file system for lack of
                 * anything better, resetting the clock.
                 */
                time.tv_sec = base;
                if (!badbase)
                        resettodr();
        } else {
                int deltat = time.tv_sec - base;

                if (deltat < 0)
                        deltat = -deltat;
                if (waszero || deltat < 2 * SECDAY)
                        return;
		if (deltat > 50 * SECDAY)
			goto badrtc;

                printf("WARNING: clock %s %d days",
                    time.tv_sec < base ? "lost" : "gained", deltat / SECDAY);
        }
        printf(" -- CHECK AND RESET THE DATE!\n");
}

/*
 * Reset the clock based on the current time.
 * Used when the current clock is preposterous, when the time is changed,
 * and when rebooting.  Do nothing if the time is not yet known, e.g.,
 * when crashing during autoconfig.
 */
void
resettodr(void)
{

        if (!time.tv_sec)
                return;

        if (todr_settime(todr_handle, (struct timeval *)&time) != 0)
                printf("resettodr: failed to set time\n");
}
