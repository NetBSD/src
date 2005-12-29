/*	$NetBSD: clock.c,v 1.1 2005/12/29 15:20:08 tsutsui Exp $	*/

/*-
 * Copyright (c) 2001, 2004, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.1 2005/12/29 15:20:08 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>		/* time */

#include <dev/clock_subr.h>

#include <mips/locore.h>	/* mips3_cp0_count_read */

#include <machine/sbdvar.h>

static todr_chip_handle_t todr_handle;

#define	MINYEAR		2005	/* "today" */

void
cpu_initclocks(void)
{

	KASSERT(platform.initclocks);

	(*platform.initclocks)();
}

void
setstatclockrate(int arg)
{

	/* not yet */
}

/*
 * Common parts of todclock autoconfiguration.
 */
void
todr_attach(todr_chip_handle_t handle)
{

	if (todr_handle)
		panic("todr_attach: too many todclocks configured");

	todr_handle = handle;
}

void
inittodr(time_t base)
{
	int badbase, waszero;

	badbase = 0;
	waszero = (base == 0);

	if (base < 5 * SECYR) {
		/*
		 * If base is 0, assume filesystem time is just unknown
		 * in stead of preposterous. Don't bark.
		 */
		if (base != 0)
			printf("WARNING: preposterous time in file system\n");
		/* not going to use it anyway, if the chip is readable */
		/* 2005/10/01   12:00:00 */
		base = 35*SECYR + 273*SECDAY + SECDAY/2;
		badbase = 1;
	}

	if (todr_gettime(todr_handle, &time) != 0 || time.tv_sec == 0) {
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
		printf("WARNING: clock %s %d days",
		    time.tv_sec < base ? "lost" : "gained", deltat / SECDAY);
	}
	printf(" -- CHECK AND RESET THE DATE!\n");
}

void
resettodr(void)
{

	if (time.tv_sec == 0)
		return;

	if (todr_settime(todr_handle, &time) != 0)
		printf("resettodr: cannot set time in time-of-day clock\n");
}

/*
 * Return the best possible estimate of the time in the timeval to
 * which tv points.
 */
void
microtime(struct timeval *tvp)
{
	int s = splclock();
	static struct timeval lasttime;

	*tvp = time;
	if (platform.readclock)
		tvp->tv_usec += (*platform.readclock)();

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
 *  Wait at least `n' usec.
 */
void
delay(unsigned int n)
{
	uint32_t cur, last, delta, usecs;
	
	last = mips3_cp0_count_read();
	delta = usecs = 0;

	while (n > usecs) {
		cur = mips3_cp0_count_read();

		/* Check to see if the timer has wrapped around. */
		if (cur < last)
			delta += ((curcpu()->ci_cycles_per_hz - last) + cur);
		else
 			delta += (cur - last);

		last = cur;

		if (delta >= curcpu()->ci_divisor_delay) {
			usecs += delta / curcpu()->ci_divisor_delay;
			delta %= curcpu()->ci_divisor_delay;
		}
	}
}
