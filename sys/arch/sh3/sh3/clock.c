/*	$NetBSD: clock.c,v 1.3.8.1 1999/12/27 18:33:44 wrstuden Exp $	*/

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

#include "opt_pclock.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <sh3/rtcreg.h>
#include <sh3/tmureg.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/cpufunc.h>
#include <machine/shbvar.h>

void	spinwait __P((int));
void	findcpuspeed __P((void));
int	clockintr __P((void *));
int	gettick __P((void));
void	sysbeepstop __P((void *));
void	sysbeep __P((int, int));
void	rtcinit __P((void));
static int yeartoday __P((int));
int 	hexdectodec __P((int));
int	dectohexdec __P((int));


#define	SECMIN	((unsigned)60)			/* seconds per minute */
#define	SECHOUR	((unsigned)(60*SECMIN))		/* seconds per hour */
#define	SECDAY	((unsigned)(24*SECHOUR))	/* seconds per day */
#define	SECYR	((unsigned)(365*SECDAY))	/* seconds per common year */

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

#if 0
#define USE_RTCCLK
#endif
/*
 * Fill in *tvp with current time with microsecond resolution.
 */
void
microtime(tvp)
	struct timeval *tvp;
{
	int s = splclock();
	static struct timeval lasttime;
#if 0
	u_long ticks = 0;
#endif

	*tvp = time;

#if 0
#ifdef USE_RTCCLK
	/* ticks = (16000 - SHREG_TCNT1)*1000000/16000; */
	ticks = 1000000 - SHREG_TCNT1*1000/16;
#else
	ticks = (PCLOCK/16 - SHREG_TCNT1)/(PCLOCK/16/1000000);
#endif

	tvp->tv_usec += ticks;
	while (tvp->tv_usec > 1000000) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
	}
#endif
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}


int
clockintr(arg)
	void *arg;
{
#if 1
	struct clockframe *frame = arg;		/* not strictly necessary */
#endif

	/* clear timer counter under flow interrupt flag */
#ifdef USE_RTCCLK
        SHREG_TCR1 = 0x0024;
#else
        SHREG_TCR1 = 0x0021;
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
delay(n)
	int n;
{
	unsigned int limit, tick, otick;

	/*
	 * Read the counter first, so that the rest of the setup overhead is
	 * counted.
	 */
#if 1
	n *= 2;
#endif
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

void
sysbeepstop(arg)
	void *arg;
{
#ifdef	TODO
#endif
}

void
sysbeep(pitch, period)
	int pitch, period;
{
#ifdef	TODO
#endif
}

unsigned int delaycount;	/* calibrated loop variable (1 millisecond) */

#define FIRST_GUESS   0x2000

void
findcpuspeed()
{
	int i;
	unsigned int remainder;

#if 0
	/* disable Under Flow int,up rising edge, 1/4 Cys */
	SHREG_TCR0 = 0;
#else
	/* disable Under Flow int,up rising edge, 1/16 Cys */
	SHREG_TCR0 = 0;
#endif

	/* set counter */
	SHREG_TCNT0 = 0xffffffff;

	/* start counter */
	SHREG_TSTR = 0x01;

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
        SHREG_TCR1 = 0x0024;
	SHREG_TCOR1 = 16000 / hz; /* about 1/HZ Sec */
	SHREG_TCNT1 = 16000 / hz; /* about 1/HZ Sec */
#else
        /* enable under flow interrupt, up rising edge, 1/16 Pcyc */
        SHREG_TCR1 = 0x0021;
	SHREG_TCOR1 = PCLOCK / 16 / hz; /* about 1/HZ Sec */
	SHREG_TCNT1 = PCLOCK / 16 / hz; /* about 1/HZ Sec */
#endif

	/* start timer counter 1 */
	SHREG_TSTR |= 0x02;

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


static int month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int
yeartoday(year)
	int year;
{

	return ((year % 4) ? 365 : 366);
}

int
hexdectodec(n)
	int n;
{

	return (((n >> 4) & 0x0f) * 10 + (n & 0x0f));
}

int
dectohexdec(n)
	int n;
{

	return ((u_char)(((n / 10) << 4) & 0xf0) | ((n % 10) & 0x0f));
}

static int timeset;

/*
 * Initialize the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */
void
inittodr(base)
	time_t base;
{
	time_t n;
	int sec, min, hr, dom, mon, yr;
	int i, days = 0;

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

	sec = hexdectodec(SHREG_RSECCNT);
	min = hexdectodec(SHREG_RMINCNT);
	hr = hexdectodec(SHREG_RHRCNT);
	dom = hexdectodec(SHREG_RDAYCNT);
	mon = hexdectodec(SHREG_RMONCNT);
	yr = hexdectodec(SHREG_RYRCNT);
	yr = (yr < 70) ? yr+100 : yr;

	n = sec + 60 * min + 3600 * hr;
	n += (dom - 1) * 3600 * 24;

	if (yeartoday(yr) == 366)
		month[1] = 29;
	for (i = mon - 2; i >= 0; i--)
		days += month[i];
	month[1] = 28;
	for (i = 70; i < yr; i++)
		days += yeartoday(i);
	n += days * 3600 * 24;

	n += rtc_offset * 60;

#ifndef INITTODR_ALWAYS_USE_RTC
	if (base < n - 5*SECYR)
		printf("WARNING: file system time much less than clock time\n");
	else if (base > n + 5*SECYR) {
		printf("WARNING: clock time much less than file system time\n");
		printf("WARNING: using file system time\n");
		goto fstime;
	}
#endif

	timeset = 1;
	time.tv_sec = n;
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
	time_t n;
	int diff, i, j;
	int s;

	/*
	 * We might have been called by boot() due to a crash early
	 * on.  Don't reset the clock chip in this case.
	 */

	if (!timeset)
		return;

	s = splclock();

	diff = rtc_offset * 60;
	n = (time.tv_sec - diff) % (3600 * 24);   /* hrs+mins+secs */

	/* stop RTC */
	SHREG_RCR2 = SHREG_RCR2_RESET|SHREG_RCR2_ENABLE;

	SHREG_RSECCNT = dectohexdec(n % 60);
	n /= 60;
	SHREG_RMINCNT = dectohexdec(n % 60);
	SHREG_RHRCNT = dectohexdec(n / 60);

	n = (time.tv_sec - diff) / (3600 * 24);	/* days */
	SHREG_RWKCNT = (n + 4) % 7;  /* 1/1/70 is Thursday */

	for (j = 1970, i = yeartoday(j); n >= i; j++, i = yeartoday(j))
		n -= i;

	SHREG_RYRCNT = dectohexdec(j - 1900);

	if (i == 366)
		month[1] = 29;
	for (i = 0; n >= month[i]; i++)
		n -= month[i];
	month[1] = 28;
	SHREG_RMONCNT = dectohexdec(++i);
	SHREG_RDAYCNT = dectohexdec(++n);

	/* start RTC */
	SHREG_RCR2 = SHREG_RCR2_RESET|SHREG_RCR2_ENABLE|SHREG_RCR2_START;

	splx(s);
}

void
setstatclockrate(arg)
	int arg;
{
}
