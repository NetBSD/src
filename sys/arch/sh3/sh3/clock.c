/*	$NetBSD: clock.c,v 1.21 2002/02/22 19:44:04 uch Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include "opt_pclock.h"
#include "wdog.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <sh3/cpufunc.h>
#include <sh3/clock.h>
#include <sh3/tmureg.h>
#include <sh3/rtcreg.h>
#include <sh3/wdogvar.h>
#include <sh3/wdtreg.h>

#include <machine/shbvar.h>

/* RTC */
#if defined(SH3) && defined(SH4)
int __sh_R64CNT;
int __sh_RSECCNT;
int __sh_RMINCNT;
int __sh_RHRCNT;
int __sh_RWKCNT;
int __sh_RDAYCNT;
int __sh_RMONCNT;
int __sh_RYRCNT;
int __sh_RSECAR;
int __sh_RMINAR;
int __sh_RHRAR;
int __sh_RWKAR;
int __sh_RDAYAR;
int __sh_RMONAR;
int __sh_RCR1;
int __sh_RCR2;

/* TMU */
int __sh_TOCR;
int __sh_TSTR;
int __sh_TCOR0;
int __sh_TCNT0;
int __sh_TCR0;
int __sh_TCOR1;
int __sh_TCNT1;
int __sh_TCR1;
int __sh_TCOR2;
int __sh_TCNT2;
int __sh_TCR2;
int __sh_TCPR2;

static void clock_register_init(void);
#endif /* SH3 && SH4 */

#if defined(SH3) && defined(SH4)
#define SH_(x)		__sh_ ## x
#elif defined(SH3)
#define SH_(x)		SH3_ ## x
#elif defined(SH4)
#define SH_(x)		SH4_ ## x
#endif

#ifndef HZ
#define HZ		64
#endif
#define MINYEAR		2002	/* "today" */
#define SH_RTC_CLOCK	16384	/* Hz */

/*
 * NetBSD/sh3 clock module
 *  + default 64Hz
 *  + use TMU channel 0 as clock interrupt source.
 *  + If RTC module is active, TMU channel 0 input source is RTC output.
 *    (1.6384kHz)
 */
struct {
	/* Hard clock */
	u_int32_t hz_cnt;	/* clock interrupt interval count */
	u_int32_t cpucycle_1us;	/* calibrated loop variable (1 us) */
	u_int32_t tmuclk;	/* source clock of TMU0 (Hz) */

	/* RTC ops holder. default SH RTC module */
	struct rtc_ops rtc;
	int rtc_initialized;

	u_int32_t pclock;	/* PCLOCK */
	u_int32_t cpuclock;	/* CPU clock */
	int flags;
} sh_clock = {
#ifdef PCLOCK
	.pclock = PCLOCK,
#endif
	.rtc = { 
		/* SH RTC module to default RTC */
		.init	= sh_rtc_init,
		.get	= sh_rtc_get,
		.set	= sh_rtc_set
	}
};

u_int32_t maxwdog;

/* TMU */
/* interrupt handler is timing critical. prepared for each. */
int sh3_clock_intr(void *);
int sh4_clock_intr(void *);

/*
 * Estimate CPU and Peripheral clock.
 */
#define TMU_START(x)							\
do {									\
	_reg_write_1(SH_(TSTR), _reg_read_1(SH_(TSTR)) & ~TSTR_STR##x);	\
	_reg_write_4(SH_(TCNT ## x), 0xffffffff);			\
	_reg_write_1(SH_(TSTR), _reg_read_1(SH_(TSTR)) | TSTR_STR##x);	\
} while (/*CONSTCOND*/0)
#define TMU_ELAPSED(x)							\
	(0xffffffff - _reg_read_4(SH_(TCNT ## x)))
void
sh_clock_init(int flags, struct rtc_ops *rtc)
{
	u_int32_t s, t0, t1 __attribute__((__unused__));

	sh_clock.flags = flags;
	if (rtc != NULL)
		sh_clock.rtc = *rtc;	/* structure copy */

#if defined(SH3) && defined(SH4)
	clock_register_init();
#endif
	/* Initialize TMU */
	_reg_write_2(SH_(TCR0), 0);
	_reg_write_2(SH_(TCR1), 0);
	_reg_write_2(SH_(TCR2), 0);
	/* Reset RTC alarm and interrupt */
	_reg_write_1(SH_(RCR1), 0);

	/* Stop all counter */
	_reg_write_1(SH_(TSTR), 0);

	/*
	 * Estimate CPU clock.
	 */
	if (sh_clock.flags & SH_CLOCK_NORTC) {
		/* Set TMU channel 0 source to PCLOCK / 16 */
		_reg_write_2(SH_(TCR0), TCR_TPSC_P16);
		sh_clock.tmuclk = sh_clock.pclock / 16;
	} else {
		/* Set TMU channel 0 source to RTC counter clock (16.384kHz) */
		_reg_write_2(SH_(TCR0),
		    CPU_IS_SH3 ? SH3_TCR_TPSC_RTC : SH4_TCR_TPSC_RTC);
		sh_clock.tmuclk = SH_RTC_CLOCK;
	}

	s = _cpu_exception_suspend();
	_cpu_spin(1);	/* load function on cache. */
	TMU_START(0);
	_cpu_spin(10000000);
	t0 = TMU_ELAPSED(0);
	_cpu_exception_resume(s);

	sh_clock.cpuclock = ((10000000 * 10) / t0) * sh_clock.tmuclk;
	sh_clock.cpucycle_1us = (sh_clock.tmuclk * 10) / t0;

	if (CPU_IS_SH4)
		sh_clock.cpuclock >>= 1;	/* two-issue */

	/*
	 * Estimate PCLOCK
	 */
	if (sh_clock.pclock == 0) {
		/* set TMU channel 1 source to PCLOCK / 4 */
		_reg_write_2(SH_(TCR1), TCR_TPSC_P4);
		s = _cpu_exception_suspend();
		_cpu_spin(1);	/* load function on cache. */
		TMU_START(0);
		TMU_START(1);
		_cpu_spin(sh_clock.cpucycle_1us * 1000000);	/* 1 sec. */
		t0 = TMU_ELAPSED(0);
		t1 = TMU_ELAPSED(1);
		_cpu_exception_resume(s);
		
		sh_clock.pclock = ((t1 * 4)/ t0) * SH_RTC_CLOCK;
	}

	/* Stop all counter */
	_reg_write_1(SH_(TSTR), 0);

#undef TMU_START
#undef TMU_ELAPSED
}

int
sh_clock_get_cpuclock()
{

	return (sh_clock.cpuclock);
}

int
sh_clock_get_pclock()
{

	return (sh_clock.pclock);
}

void
setstatclockrate(int newhz)
{
	/* XXX not yet */
}

/*
 * Return the best possible estimate of the time in the timeval to
 * which tv points.
 */
void
microtime(struct timeval *tv)
{
	static struct timeval lasttime;
	int s;

	s = splclock();
	*tv = time;
	splx(s);

	tv->tv_usec += ((sh_clock.hz_cnt - _reg_read_4(SH_(TCNT0)))
	    * 1000000) / sh_clock.tmuclk;
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
	
	_cpu_spin(sh_clock.cpucycle_1us * n);
}

/*
 * Start the clock interrupt.
 */
void
cpu_initclocks()
{

	if (sh_clock.pclock == 0)
		panic("No PCLOCK information.");

	/* Set global variables. */
	hz = HZ;
	tick = 1000000 / hz;

	/* 
	 * Use TMU channel 0 as hard clock 
	 */
	_reg_write_1(SH_(TSTR), _reg_read_1(SH_(TSTR)) & ~TSTR_STR0);

	if (sh_clock.flags & SH_CLOCK_NORTC) {
		/* use PCLOCK/16 as TMU0 source */
		_reg_write_2(SH_(TCR0), TCR_UNIE | TCR_TPSC_P16);
	} else {
		/* use RTC clock as TMU0 source */
		_reg_write_2(SH_(TCR0), TCR_UNIE | 
		    (CPU_IS_SH3 ? SH3_TCR_TPSC_RTC : SH4_TCR_TPSC_RTC));
	}
	sh_clock.hz_cnt = sh_clock.tmuclk / hz - 1;

	_reg_write_4(SH_(TCOR0), sh_clock.hz_cnt);
	_reg_write_4(SH_(TCNT0), sh_clock.hz_cnt);

	shb_intr_establish(TMU0_IRQ, IST_EDGE, IPL_CLOCK,
	    CPU_IS_SH3 ? sh3_clock_intr : sh4_clock_intr, 0);
	/* start hardclock */
	_reg_write_1(SH_(TSTR), _reg_read_1(SH_(TSTR)) | TSTR_STR0);

	/*
	 * TMU channel 1, 2 are one shot timer.
	 */
	_reg_write_2(SH_(TCR1), TCR_UNIE | TCR_TPSC_P4);
	_reg_write_4(SH_(TCOR1), 0xffffffff);
	_reg_write_2(SH_(TCR2), TCR_UNIE | TCR_TPSC_P4);
	_reg_write_4(SH_(TCOR2), 0xffffffff);

	/* Make sure to start RTC */
	sh_clock.rtc.init(sh_clock.rtc._cookie);
}

void
inittodr(time_t base)
{
	struct clock_ymdhms dt;
	time_t rtc;
	int s;

	if (!sh_clock.rtc_initialized)
		sh_clock.rtc_initialized = 1;

	sh_clock.rtc.get(sh_clock.rtc._cookie, base, &dt);
	rtc = clock_ymdhms_to_secs(&dt);

#ifdef DEBUG
	printf("inittodr: %d/%d/%d/%d/%d/%d(%d)\n", dt.dt_year, 
	    dt.dt_mon, dt.dt_day, dt.dt_hour, dt.dt_min, dt.dt_sec,
	    dt.dt_wday);
#endif

	
	if (!(sh_clock.flags & SH_CLOCK_NOINITTODR) &&
	    (rtc < base ||
		dt.dt_year < MINYEAR || dt.dt_year > 2037 ||
		dt.dt_mon < 1 || dt.dt_mon > 12 ||
		dt.dt_wday > 6 ||
		dt.dt_day < 1 || dt.dt_day > 31 ||
		dt.dt_hour > 23 || dt.dt_min > 59 || dt.dt_sec > 59)) {
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

void
resettodr()
{
	struct clock_ymdhms dt;
	int s;

	if (!sh_clock.rtc_initialized)
		return;

	s = splclock();
	clock_secs_to_ymdhms(time.tv_sec - rtc_offset * 60, &dt);
	splx(s);

	sh_clock.rtc.set(sh_clock.rtc._cookie, &dt);
#ifdef DEBUG
        printf("%s: %d/%d/%d/%d/%d/%d(%d) rtc_offset %d\n", __FUNCTION__,
	    dt.dt_year, dt.dt_mon, dt.dt_day, dt.dt_hour, dt.dt_min, dt.dt_sec,
	    dt.dt_wday, rtc_offset);
#endif
}

#ifdef SH3
int
sh3_clock_intr(void *arg) /* trap frame */
{
#if (NWDOG > 0)
	u_int32_t i;

	i = (u_int32_t)SHREG_WTCNT_R;
	if (i > maxwdog)
		maxwdog = i;
	wdog_wr_cnt(0);			/* reset to zero */
#endif
	/* clear underflow status */
	_reg_write_2(SH3_TCR0, _reg_read_2(SH3_TCR0) & ~TCR_UNF);

	hardclock(arg);

	return (1);
}
#endif /* SH3 */
#ifdef SH4
int
sh4_clock_intr(void *arg) /* trap frame */
{
#if (NWDOG > 0)
	u_int32_t i;

	i = (u_int32_t)SHREG_WTCNT_R;
	if (i > maxwdog)
		maxwdog = i;
	wdog_wr_cnt(0);			/* reset to zero */
#endif
	/* clear underflow status */
	_reg_write_2(SH4_TCR0, _reg_read_2(SH4_TCR0) & ~TCR_UNF);

	hardclock(arg);

	return (1);
}
#endif /* SH4 */

/*
 * SH3 RTC module ops.
 */

void
sh_rtc_init(void *cookie)
{

	/* Make sure to start RTC */
	_reg_write_1(SH_(RCR2), SH_RCR2_ENABLE | SH_RCR2_START);
}

void
sh_rtc_get(void *cookie, time_t base, struct clock_ymdhms *dt)
{
	int retry = 8;

	/* disable carry interrupt */
	_reg_write_1(SH_(RCR1), _reg_read_1(SH_(RCR1)) & ~SH_RCR1_CIE);

	do {
		u_int8_t r = _reg_read_1(SH_(RCR1));
		r &= ~SH_RCR1_CF;
		r |= SH_RCR1_AF; /* don't clear alarm flag */
		_reg_write_1(SH_(RCR1), r);

		if (CPU_IS_SH3)
			dt->dt_year = FROMBCD(_reg_read_1(SH3_RYRCNT));
		else
			dt->dt_year = FROMBCD(_reg_read_2(SH4_RYRCNT) & 0x00ff);

		/* read counter */
#define RTCGET(x, y)	dt->dt_ ## x = FROMBCD(_reg_read_1(SH_(R ## y ## CNT)))
		RTCGET(mon, MON);
		RTCGET(wday, WK);
		RTCGET(day, DAY);
		RTCGET(hour, HR);
		RTCGET(min, MIN);
		RTCGET(sec, SEC);
#undef RTCGET		
	} while ((_reg_read_1(SH_(RCR1)) & SH_RCR1_CF) && --retry > 0);

	if (retry == 0) {
		printf("rtc_gettime: couldn't read RTC register.\n");
		memset(dt, sizeof(*dt), 0);
		return;
	}

	dt->dt_year = (dt->dt_year % 100) + 1900;
	if (dt->dt_year < 1970)
		dt->dt_year += 100;
}

void
sh_rtc_set(void *cookie, struct clock_ymdhms *dt)
{
	u_int8_t r;

	/* stop clock */
	r = _reg_read_1(SH_(RCR2));
	r |= SH_RCR2_RESET;
	r &= ~SH_RCR2_START;
	_reg_write_1(SH_(RCR2), r);

	/* set time */
	if (CPU_IS_SH3)
		_reg_write_1(SH3_RYRCNT, TOBCD(dt->dt_year % 100));
	else
		_reg_write_2(SH4_RYRCNT, TOBCD(dt->dt_year % 100));
#define RTCSET(x, y)	_reg_write_1(SH_(R ## x ## CNT), TOBCD(dt->dt_ ## y))
	RTCSET(MON, mon);
	RTCSET(WK, wday);
	RTCSET(DAY, day);
	RTCSET(HR, hour);
	RTCSET(MIN, min);
	RTCSET(SEC, sec);
#undef RTCSET
	/* start clock */
	_reg_write_1(SH_(RCR2), r | SH_RCR2_START);
}

#if defined(SH3) && defined(SH4)
#define SH3REG(x)	__sh_ ## x = SH3_ ## x
#define SH4REG(x)	__sh_ ## x = SH4_ ## x
static void
clock_register_init()
{
	if (CPU_IS_SH3) {
		SH3REG(R64CNT);
		SH3REG(RSECCNT);
		SH3REG(RMINCNT);
		SH3REG(RHRCNT);
		SH3REG(RWKCNT);
		SH3REG(RDAYCNT);
		SH3REG(RMONCNT);
		SH3REG(RYRCNT);
		SH3REG(RSECAR);
		SH3REG(RMINAR);
		SH3REG(RHRAR);
		SH3REG(RWKAR);
		SH3REG(RDAYAR);
		SH3REG(RMONAR);
		SH3REG(RCR1);
		SH3REG(RCR2);
		SH3REG(TOCR);
		SH3REG(TSTR);
		SH3REG(TCOR0);
		SH3REG(TCNT0);
		SH3REG(TCR0);
		SH3REG(TCOR1);
		SH3REG(TCNT1);
		SH3REG(TCR1);
		SH3REG(TCOR2);
		SH3REG(TCNT2);
		SH3REG(TCR2);
		SH3REG(TCPR2);
	}

	if (CPU_IS_SH4) {
		SH4REG(R64CNT);
		SH4REG(RSECCNT);
		SH4REG(RMINCNT);
		SH4REG(RHRCNT);
		SH4REG(RWKCNT);
		SH4REG(RDAYCNT);
		SH4REG(RMONCNT);
		SH4REG(RYRCNT);
		SH4REG(RSECAR);
		SH4REG(RMINAR);
		SH4REG(RHRAR);
		SH4REG(RWKAR);
		SH4REG(RDAYAR);
		SH4REG(RMONAR);
		SH4REG(RCR1);
		SH4REG(RCR2);
		SH4REG(TOCR);
		SH4REG(TSTR);
		SH4REG(TCOR0);
		SH4REG(TCNT0);
		SH4REG(TCR0);
		SH4REG(TCOR1);
		SH4REG(TCNT1);
		SH4REG(TCR1);
		SH4REG(TCOR2);
		SH4REG(TCNT2);
		SH4REG(TCR2);
		SH4REG(TCPR2);
	}
}
#undef SH3REG
#undef SH4REG
#endif /* SH3 && SH4 */
