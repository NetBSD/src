/*	$NetBSD: clock.c,v 1.19 2002/02/12 15:26:48 uch Exp $	*/

/*-
 * Copyright (c) 1993, 1994 Charles Hannum.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)clock.c	7.2 (Berkeley) 5/12/91
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
  Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.

		All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
 * Primitive clock interrupt routines.
 */

#include "wdog.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <sh3/pclock.h>
#include <sh3/rtcreg.h>
#include <sh3/tmureg.h>
#include <sh3/wdogvar.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/cpufunc.h>
#include <machine/shbvar.h>

void	findcpuspeed(void);
int	clockintr(void *);
int	gettick(void);
void	rtcinit(void);

int timer0speed;

/*
 * microtime() makes use of the following globals.  Note that isa_timer_tick
 * may be redundant to the `tick' variable, but is kept here for stability.
 * isa_timer_count is the countdown count for the timer.  timer_msb_table[]
 * and timer_lsb_table[] are used to compute the microsecond increment
 * for time.tv_usec in the follow fashion:
 *
 * time.tv_usec += isa_timer_msb_table[cnt_msb] - isa_timer_lsb_table[cnt_lsb];
 */

void
startrtclock()
{

	findcpuspeed();		/* use the clock (while it's free)
					to find the cpu speed */
}

/*
 * Fill in *tvp with current time with microsecond resolution.
 */
void
microtime(struct timeval *tvp)
{
	int s = splclock();
	long diff;
	static struct timeval lasttime;
	static u_long numerator = 0;
	static u_long denominator = 0;

	/*
	 * 1/hz resolution ``time'' which is counted up at hardclock().
	 */
	*tvp = time;

	/*
	 * diff = (PCLOCK/16/hz - 1 - SHREG_TCNT1) / (PCLOCK/16/hz - 1)
	 *         * 1/hz * 10^6. [usec]
	 */

	if (denominator == 0) {
		numerator = 1000000 / hz;
		denominator = PCLOCK/16/hz - 1;
	}

	diff = (denominator - SHREG_TCNT1) * numerator / denominator;

	/*
	 * add ``diff'' to ``time''
	 */
	tvp->tv_usec += diff;
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
	}

	/*
	 * microtime() always gains 1usec at least.
	 */

	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;

	splx(s);
}

#include <sh3/wdtreg.h>
unsigned int maxwdog;

int
clockintr(void *arg)
{
	struct clockframe *frame = arg;		/* not strictly necessary */

#if (NWDOG > 0)
	unsigned int i;

	i = (unsigned int)SHREG_WTCNT_R;
	if (i > maxwdog)
		maxwdog = i;
	wdog_wr_cnt(0);			/* reset to zero */
#endif

	/* clear timer counter under flow interrupt flag */
#ifdef USE_RTCCLK
        SHREG_TCR1 = TCR_UNIE | TCR_TPSC_RTC;
#else
        SHREG_TCR1 = TCR_UNIE | TCR_TPSC_P16;
#endif

	hardclock(frame);
	return -1;
}

int
gettick()
{
	int counter;
	/* Don't want someone screwing with the counter while we're here. */
	disable_intr();

	counter = SHREG_TCNT0;

	enable_intr();
	return counter;
}

/*
 * Wait "n" microseconds.
 * Relies on timer 1 counting down from (TIMER_FREQ / hz) at TIMER_FREQ Hz.
 * Note: timer had better have been programmed before this is first used!
 * (Note that we use `rate generator' mode, which counts at 1:1; `square
 * wave' mode counts at 2:1).
 */
void
delay(int n)
{
	unsigned int limit, tick, otick;

	/*
	 * Read the counter first, so that the rest of the setup overhead is
	 * counted.
	 */

	n *= timer0speed;

	otick = gettick();
	limit = 0xffffffff;

	while (n > 0) {
		tick = gettick();
		if (tick > otick)
			n -= limit - (tick - otick);
		else
			n -= otick - tick;
		otick = tick;
	}
}

unsigned int delaycount;	/* calibrated loop variable (1 millisecond) */

#define FIRST_GUESS   0x2000

void
findcpuspeed()
{
	int i;
	unsigned int remainder;

	/* using clock = Internal RTC */
	SHREG_TOCR = TOCR_TCOE;

	/* disable Under Flow int,up rising edge, 1/4 Cys */
	SHREG_TCR0 = 0;

	timer0speed = PCLOCK / 1000000 / 4 + 1;

	/* set counter */
	SHREG_TCNT0 = 0xffffffff;

	/* start counter 0 */
	SHREG_TSTR |= TSTR_STR0;

	/* Timer counter is decremented at every 0.5 uSec */
	for (i = FIRST_GUESS; i; i--)
		;

	/* Read the value left in the counter */
	remainder = gettick();

	/* 1 timer tick neary eqyal 0.5 uSec */
	delaycount = (FIRST_GUESS * 2000) / (0xffffffff - remainder);
}

void
cpu_initclocks()
{

#ifdef USE_RTCCLK
        /* enable under flow interrupt, up rising edge, RTCCLK */
	/* RTCCLK == 16kHz */
	SHREG_TCR1 = TCR_UNIE | TCR_TPSC_RTC;
	SHREG_TCOR1 = 16384 / hz - 1; /* about 1/HZ Sec */
	SHREG_TCNT1 = 16384 / hz - 1; /* about 1/HZ Sec */
#else
        /* enable under flow interrupt, up rising edge, 1/16 Pcyc */
	SHREG_TCR1 = TCR_UNIE | TCR_TPSC_P16;
	SHREG_TCOR1 = PCLOCK / 16 / hz - 1; /* about 1/HZ Sec */
	SHREG_TCNT1 = PCLOCK / 16 / hz - 1; /* about 1/HZ Sec */
#endif

	/* start timer counter 1 */
	SHREG_TSTR |= TSTR_STR1;

	(void)shb_intr_establish(TMU1_IRQ, IST_EDGE, IPL_CLOCK, clockintr, 0);
}

void
rtcinit()
{
	static int first_rtcopen_ever = 1;

	if (!first_rtcopen_ever)
		return;
	first_rtcopen_ever = 0;

}

static int timeset;

/*
 * Initialize the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */
void
inittodr(time_t base)
{
	struct clock_ymdhms dt;
	int doreset = 0;

	/*
	 * We mostly ignore the suggested time and go for the RTC clock time
	 * stored in the CMOS RAM.  If the time can't be obtained from the
	 * CMOS, or if the time obtained from the CMOS is 5 or more years
	 * less than the suggested time, we used the suggested time.  (In
	 * the latter case, it's likely that the CMOS battery has died.)
	 */

	if (base < 15*SECYR) {	/* if before 1985, something's odd... */
		printf("WARNING: preposterous time in file system\n");
		/* read the system clock anyway */
		base = 17*SECYR + 186*SECDAY + SECDAY/2;
	}

#ifdef SH4
#define	FROMBCD2(x)	((((x) & 0xf000) >> 12) * 1000 + \
			 (((x) & 0x0f00) >> 8) * 100 + \
			 (((x) & 0x00f0) >> 4) * 10 + ((x) & 0xf))
	dt.dt_year = FROMBCD2(SHREG_RYRCNT);
#else
	dt.dt_year = 1900 + FROMBCD(SHREG_RYRCNT);
#endif
	dt.dt_mon = FROMBCD(SHREG_RMONCNT);
	dt.dt_day = FROMBCD(SHREG_RDAYCNT);
	dt.dt_wday = FROMBCD(SHREG_RWKCNT);
	dt.dt_hour = FROMBCD(SHREG_RHRCNT);
	dt.dt_min = FROMBCD(SHREG_RMINCNT);
	dt.dt_sec = FROMBCD(SHREG_RSECCNT);

#ifdef DEBUG
	printf("readclock: %d/%d/%d/%d/%d/%d(%d)\n", dt.dt_year - 1900,
	       dt.dt_mon, dt.dt_day, dt.dt_hour, dt.dt_min, dt.dt_sec,
	       dt.dt_wday);
#endif

#ifndef SH4
	if (dt.dt_year < 1970)
		dt.dt_year += 100;
#endif

	if (dt.dt_mon < 1 || dt.dt_mon > 12)
		doreset = 1;
	if (dt.dt_day < 1 || dt.dt_day > 31)
		doreset = 1;
	if (dt.dt_hour > 23)
		doreset = 1;
	if (dt.dt_min > 59)
		doreset = 1;
	if (dt.dt_sec > 59)
		doreset = 1;

	if (doreset == 1) {
		printf("WARNING: clock time is invalid.\n");
		printf("WARNING: reset to epoch time!\n");
		time.tv_sec = 0;
	} else
		time.tv_sec = clock_ymdhms_to_secs(&dt) + rtc_offset * 60;

#ifndef INITTODR_ALWAYS_USE_RTC
	if (base < time.tv_sec - 5*SECYR)
		printf("WARNING: file system time much less than clock time\n");
	else if (base > time.tv_sec + 5*SECYR) {
		printf("WARNING: clock time much less than file system time\n");
		printf("WARNING: using file system time\n");
		goto fstime;
	}
#endif

	timeset = 1;
	time.tv_usec = 0;

	return;

#ifndef INITTODR_ALWAYS_USE_RTC
fstime:
	timeset = 1;
	time.tv_sec = base;
	time.tv_usec = 0;
	printf("WARNING: CHECK AND RESET THE DATE!\n");
#endif
}

/*
 * Reset the clock.
 */
void
resettodr()
{
	struct clock_ymdhms dt;
	int s;

	/*
	 * We might have been called by boot() due to a crash early
	 * on.  Don't reset the clock chip in this case.
	 */

	if (!timeset)
		return;

	s = splclock();

	clock_secs_to_ymdhms(time.tv_sec - rtc_offset * 60, &dt);

	/* stop RTC */
	SHREG_RCR2 = SHREG_RCR2_RESET|SHREG_RCR2_ENABLE;

	SHREG_RSECCNT = TOBCD(dt.dt_sec);
	SHREG_RMINCNT = TOBCD(dt.dt_min);
	SHREG_RHRCNT = TOBCD(dt.dt_hour);
	SHREG_RWKCNT = TOBCD(dt.dt_wday);
	SHREG_RDAYCNT = TOBCD(dt.dt_day);
	SHREG_RMONCNT = TOBCD(dt.dt_mon);
#ifdef SH4
#define TOBCD2(x)	((((x) % 10000) / 1000 * 4096) + \
			 (((x) % 1000) / 100 * 256) + \
			 ((((x) % 100) / 10) * 16) + ((x) % 10))
	SHREG_RYRCNT = TOBCD2(dt.dt_year);
#else
	SHREG_RYRCNT = TOBCD(dt.dt_year % 100);
#endif

	/* start RTC */
	SHREG_RCR2 = SHREG_RCR2_RESET|SHREG_RCR2_ENABLE|SHREG_RCR2_START;

	splx(s);

#ifdef DEBUG
        printf("setclock: %d/%d/%d/%d/%d/%d(%d)\n", dt.dt_year % 100,
	       dt.dt_mon, dt.dt_day, dt.dt_hour, dt.dt_min, dt.dt_sec,
	       dt.dt_wday);
#endif
}

void
setstatclockrate(int arg)
{
}
