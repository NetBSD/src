/*	$NetBSD: clock.c,v 1.2 2003/07/15 02:54:37 lukem Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.2 2003/07/15 02:54:37 lukem Exp $");

#include "debug_playstation2.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>		/* time */

#include <mips/locore.h>

#include <dev/clock_subr.h>
#include <machine/bootinfo.h>

#include <playstation2/ee/timervar.h>

#define MINYEAR		2001	/* "today" */

static void get_bootinfo_tod(struct clock_ymdhms *);

void
cpu_initclocks()
{

	hz = 100;

	/* Install clock interrupt */
	timer_clock_init();
}

void
inittodr(time_t base)
{
	struct clock_ymdhms dt;
	time_t rtc;
	int s;
	
	get_bootinfo_tod(&dt);
	
	rtc = clock_ymdhms_to_secs(&dt);

	if (rtc < base ||
	    dt.dt_year < MINYEAR || dt.dt_year > 2037 ||
	    dt.dt_mon < 1 || dt.dt_mon > 12 ||
	    dt.dt_wday > 6 ||
	    dt.dt_day < 1 || dt.dt_day > 31 ||
	    dt.dt_hour > 23 || dt.dt_min > 59 || dt.dt_sec > 59) {
		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the RTC.
		 */
		s = splclock();
		time.tv_sec = base;
		time.tv_usec = 0;
		splx(s);
		printf("WARNING: preposterous clock chip time\n");
		resettodr();
		printf(" -- CHECK AND RESET THE DATE!\n");
		return;
	}

	s = splclock();
	time.tv_sec = rtc + rtc_offset * 60;
	time.tv_usec = 0;
	splx(s);
}

void
resettodr()
{
	/* NetBSD kernel can't access PS2 RTC module. nothing to do */
}

void
setstatclockrate(int arg)
{
	/* not yet */
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

	if (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
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
 *  Wait at least `n' usec. (max 15 sec)
 *  PS2 R5900 CPU clock is 294.912Mhz = (1 << 15) * 9 * 1000
 */
void
delay(unsigned usec)
{
	u_int32_t r0, r1, r2;
	u_int64_t n;
	int overlap;

	r0 = mips3_cp0_count_read();
	n = (((u_int64_t)usec * 294912) / 1000) + (u_int64_t)r0;

	overlap = n  > 0xffffffff;
	r2 = (u_int32_t)(overlap ? 0xffffffff : n);

	do {
		r1 = mips3_cp0_count_read();
	} while (r1 < r2 && r1 > r0);

	if (overlap) {
		r0 = r1;
		r2 = (u_int32_t)(n - 0xffffffff);
		do {
			r1 = mips3_cp0_count_read();
		} while (r1 < r2 && r1 > r0);
	}
}

static void
get_bootinfo_tod(struct clock_ymdhms *dt)
{
	time_t utc;
	struct bootinfo_rtc *rtc = 
	    (void *)MIPS_PHYS_TO_KSEG1(BOOTINFO_BLOCK_BASE + BOOTINFO_RTC);

	/* PS2 RTC is JST */
	dt->dt_year = FROMBCD(rtc->year) + 2000;
	dt->dt_mon = FROMBCD(rtc->mon);
	dt->dt_day = FROMBCD(rtc->day);
	dt->dt_hour = FROMBCD(rtc->hour);
	dt->dt_min = FROMBCD(rtc->min);
	dt->dt_sec = FROMBCD(rtc->sec);

	/* convert to UTC */
	utc = clock_ymdhms_to_secs(dt) - 9*60*60;
	clock_secs_to_ymdhms(utc, dt);
#ifdef DEBUG
        printf("bootinfo: %d/%d/%d/%d/%d/%d rtc_offset %d\n", dt->dt_year,
	    dt->dt_mon, dt->dt_day, dt->dt_hour, dt->dt_min, dt->dt_sec,
	    rtc_offset);
#endif
}
