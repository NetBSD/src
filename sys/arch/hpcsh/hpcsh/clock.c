/*	$NetBSD: clock.c,v 1.1 2001/04/23 11:22:19 uch Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <sh3/rtcreg.h>
#include <sh3/tmureg.h>

#include <machine/shbvar.h>

#include <hpcsh/hpcsh/clockvar.h>

#ifndef HZ
#define HZ		64
#endif
#define MINYEAR		2001	/* "today" */
#define RTC_CLOCK	16384	/* Hz */

/*
 * hpcsh clock module
 *  + default 64Hz
 *  + use TMU channel 0 as clock interrupt source.
 *  + TMU channel 0 input source is SH internal RTC output. (1.6384kHz)
 */
/* TMU */
static int clockintr(void *);
/* RTC */
static void rtc_init(void);
static int rtc_gettime(struct clock_ymdhms *);
static int rtc_settime(struct clock_ymdhms *);

static int __todr_inited;
static int __cnt_delay;		/* calibrated loop variable (1 us) */
static u_int32_t __cnt_clock;	/* clock interrupt interval count */
static int __cpuclock;
static int __pclock;

/*
 * [...]
 * add    IF ID EX MA WB
 * nop       IF ID EX MA WB
 * cmp/pl       IF ID EX MA WB -  -
 * nop             IF ID EX MA -  -  WB
 * bt                 IF ID EX .  .  MA WB
 * nop                   IF ID -  -  EX MA WB
 * nop                      IF -  -  ID EX MA WB
 * nop                      -  -  -  IF ID EX MA WB
 * add                                  IF ID EX MA WB
 * nop                                     IF ID EX MA WB
 * cmp/pl                                     IF ID EX MA WB -  -
 * nop                                           IF ID EX MA -  - WB
 * bt                                               IF ID EX .  . MA
 * [...]
 */
#define DELAY_LOOP(x)							\
	__asm__ __volatile__("						\
		mov.l	r0, @-r15;					\
		mov	%0, r0;						\
		nop;nop;nop;nop;					\
	1:	nop;			/* 1 */				\
	 	nop;			/* 2 */				\
		nop;			/* 3 */				\
		add	#-1, r0;	/* 4 */				\
		nop;			/* 5 */				\
		cmp/pl	r0;		/* 6 */				\
		nop;			/* 7 */				\
		bt	1b;		/* 8, 9, 10 */			\
		mov.l	@r15+, r0;					\
	" :: "r"(x))

/*
 * Estimate CPU and Peripheral clock.
 */
void
clock_init()
{
#define TMU_START(x)							\
({									\
	SHREG_TSTR &= ~TSTR_STR ## x;					\
	SHREG_TCNT ## x = 0xffffffff;					\
	SHREG_TSTR |= TSTR_STR ## x;					\
})
#define TMU_ELAPSED(x)							\
	(0xffffffff - SHREG_TCNT ## x)

	u_int32_t t0, t1;

	/* initialize TMU */
	SHREG_TCR0 = 0;
	SHREG_TCR1 = 0;
	SHREG_TCR2 = 0;

	/* stop all counter */
	SHREG_TSTR = 0;
	/* set TMU channel 0 source to RTC counter clock (16.384kHz) */
	SHREG_TCR0 = TCR_TPSC_RTC;

	/*
	 * estimate CPU clock.
	 */
	TMU_START(0);
	DELAY_LOOP(10000000);
	t0 = TMU_ELAPSED(0);
	__cpuclock = (100000000 / t0) * RTC_CLOCK;
	__cnt_delay = (RTC_CLOCK * 10) / t0;

	/*
	 * estimate PCLOCK
	 */
	/* set TMU channel 1 source to PCLOCK / 4 */
	SHREG_TCR1 = TCR_TPSC_P4;
	TMU_START(0);
	TMU_START(1);
	delay(1000000);
	t0 = TMU_ELAPSED(0);
	t1 = TMU_ELAPSED(1);

	__pclock = (t1 / t0) * RTC_CLOCK * 4;

	/* stop all counter */
	SHREG_TSTR = 0;

	/* Initialize RTC */
	rtc_init();
#undef TMU_START
#undef TMU_ELAPSED
}

int
clock_get_cpuclock()
{
	return __cpuclock;
}

int
clock_get_pclock()
{
	return __pclock;
}

/*
 * Return the best possible estimate of the time in the timeval to
 * which tv points.
 */
void
microtime(struct timeval *tv)
{
	static struct timeval lasttime;

	int s = splclock();
	*tv = time;
	splx(s);

	tv->tv_usec += ((__cnt_clock - SHREG_TCNT0) * 1000000) / RTC_CLOCK;
	while (tv->tv_usec >= 1000000) {
		tv->tv_usec -= 1000000;
		tv->tv_sec++;
	}

	if (tv->tv_sec == lasttime.tv_sec &&
	    tv->tv_usec <= lasttime.tv_usec &&
	    (tv->tv_usec = lasttime.tv_usec + 1) >= 1000000) {
		tv->tv_usec -= 1000000;
		tv->tv_sec++;
	}
	lasttime = *tv;
}

/*
 *  Wait at least `n' usec.
 */
void
delay(int n)
{
	DELAY_LOOP(__cnt_delay * n);
}

/*
 * Start the clock interrupt.
 */
void
cpu_initclocks()
{
	/* set global variables. */
	hz = HZ;
	tick = 1000000 / hz;

	/* use TMU channel 0 as clock source. */
	SHREG_TSTR &= ~TSTR_STR1;
	SHREG_TCR1 = TCR_UNIE | TCR_TPSC_RTC;
	__cnt_clock = RTC_CLOCK / hz - 1;
	SHREG_TCOR1 = __cnt_clock;
	SHREG_TCNT1 = __cnt_clock;
	SHREG_TSTR |= TSTR_STR1;

	shb_intr_establish(TMU1_IRQ, IST_EDGE, IPL_CLOCK, clockintr, 0);
}

int
clockintr(void *arg) /* trap frame */
{
	/* clear underflow status */
	SHREG_TCR1 &= ~TCR_UNF;

	hardclock(arg);

	return (1);
}

/*
 * Initialize time of day.
 */
void
inittodr(time_t base)
{
	struct clock_ymdhms dt;
	time_t rtc;
	int s;

	__todr_inited = 1;
	rtc_gettime(&dt);
	rtc = clock_ymdhms_to_secs(&dt);

#ifdef DEBUG
	printf("readclock: %d/%d/%d/%d/%d/%d(%d)\n", dt.dt_year, 
	    dt.dt_mon, dt.dt_day, dt.dt_hour, dt.dt_min, dt.dt_sec,
	    dt.dt_wday);
#endif

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

	return;
}

/*
 * Reset the RTC.
 */
void
resettodr()
{
	struct clock_ymdhms dt;
	int s;

	if (!__todr_inited)
		return;

	s = splclock();
	clock_secs_to_ymdhms(time.tv_sec - rtc_offset * 60, &dt);
	splx(s);

	rtc_settime(&dt);
#ifdef DEBUG
        printf("setclock: %d/%d/%d/%d/%d/%d(%d) rtc_offset %d\n", dt.dt_year,
	   dt.dt_mon, dt.dt_day, dt.dt_hour, dt.dt_min, dt.dt_sec, dt.dt_wday,
	   rtc_offset);
#endif
}

void
setstatclockrate(int newhz)
{
	/* XXX not yet */
}

/*
 * SH3 RTC module ops.
 */
void
rtc_init()
{
	/* reset RTC alarm and interrupt */
	SHREG_RCR1 = 0;
	/* make sure to start RTC */
	SHREG_RCR2 = (SHREG_RCR2_ENABLE | SHREG_RCR2_START);
}

int
rtc_gettime(struct clock_ymdhms *dt)
{
	int retry = 8;

	/* disable carry interrupt */
	SHREG_RCR1 &= ~SHREG_RCR1_CIE;

	do {
		u_int8_t r = SHREG_RCR1;
		r &= ~SHREG_RCR1_CF;
		r |= SHREG_RCR1_AF; /* don't clear alarm flag */
		SHREG_RCR1 = r;

		/* read counter */
#define RTCGET(x, y)	dt->dt_ ## x = FROMBCD(SHREG_R ## y ## CNT)
		RTCGET(year, YR);
		RTCGET(mon, MON);
		RTCGET(wday, WK);
		RTCGET(day, DAY);
		RTCGET(hour, HR);
		RTCGET(min, MIN);
		RTCGET(sec, SEC);
#undef RTCGET		
	} while ((SHREG_RCR1 & SHREG_RCR1_CF) && --retry > 0);

	if (retry == 0) {
		printf("rtc_gettime: couldn't read RTC register.\n");
		memset(dt, sizeof(*dt), 0);
		return (1);
	}

	dt->dt_year = (dt->dt_year % 100) + 1900;
	if (dt->dt_year < 1970)
		dt->dt_year += 100;

	return (0);
}
	
int
rtc_settime(struct clock_ymdhms *dt)
{
	u_int8_t r;

	/* stop clock */
	r = SHREG_RCR2;
	r |= SHREG_RCR2_RESET;
	r &= ~SHREG_RCR2_START;
	SHREG_RCR2 = r;

	/* set time */
#define RTCSET(x, y)	SHREG_R ## x ## CNT = TOBCD(dt->dt_ ## y);
	SHREG_RYRCNT	= TOBCD(dt->dt_year % 100);
	RTCSET(MON, mon);
	RTCSET(WK, wday);
	RTCSET(DAY, day);
	RTCSET(HR, hour);
	RTCSET(MIN, min);
	RTCSET(SEC, sec);
#undef RTCSET
	/* start clock */
	SHREG_RCR2 = (r | SHREG_RCR2_START);

	return (0);
}
