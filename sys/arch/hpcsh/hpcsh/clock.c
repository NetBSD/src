/*	$NetBSD: clock.c,v 1.6 2002/02/17 21:01:18 uch Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#include <sh3/cpufunc.h>

#include <machine/shbvar.h>
#include <machine/debug.h>

#include <hpcsh/hpcsh/clockvar.h>

/* RTC */
#define SH3_R64CNT			0xfffffec0
#define SH3_RSECCNT			0xfffffec2
#define SH3_RMINCNT			0xfffffec4
#define SH3_RHRCNT			0xfffffec6
#define SH3_RWKCNT			0xfffffec8
#define SH3_RDAYCNT			0xfffffeca
#define SH3_RMONCNT			0xfffffecc
#define SH3_RYRCNT			0xfffffece
#define SH3_RSECAR			0xfffffed0
#define SH3_RMINAR			0xfffffed2
#define SH3_RHRAR			0xfffffed4
#define SH3_RWKAR			0xfffffed6
#define SH3_RDAYAR			0xfffffed8
#define SH3_RMONAR			0xfffffeda
#define SH3_RCR1			0xfffffedc
#define SH3_RCR2			0xfffffede

#define SH4_R64CNT			0xffc80000
#define SH4_RSECCNT			0xffc80004
#define SH4_RMINCNT			0xffc80008
#define SH4_RHRCNT			0xffc8000c
#define SH4_RWKCNT			0xffc80010
#define SH4_RDAYCNT			0xffc80014
#define SH4_RMONCNT			0xffc80018
#define SH4_RYRCNT			0xffc8001c	/* 16 bit */
#define SH4_RSECAR			0xffc80020
#define SH4_RMINAR			0xffc80024
#define SH4_RHRAR			0xffc80028
#define SH4_RWKAR			0xffc8002c
#define SH4_RDAYAR			0xffc80030
#define SH4_RMONAR			0xffc80034
#define SH4_RCR1			0xffc80038
#define SH4_RCR2			0xffc8003c

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
#endif /* SH3 && SH4 */

#define   SH_RCR1_CF			  0x80
#define   SH_RCR1_CIE			  0x10
#define   SH_RCR1_AIE			  0x08
#define   SH_RCR1_AF			  0x01
#define   SH_RCR2_PEF			  0x80
#define   SH_RCR2_PES2			  0x40
#define   SH_RCR2_PES1			  0x20
#define   SH_RCR2_PES0			  0x10
#define   SH_RCR2_ENABLE		  0x08
#define   SH_RCR2_ADJ			  0x04
#define   SH_RCR2_RESET			  0x02
#define   SH_RCR2_START			  0x01

/* TMU */
#define SH3_TOCR			0xfffffe90
#define SH3_TSTR			0xfffffe92
#define SH3_TCOR0			0xfffffe94
#define SH3_TCNT0			0xfffffe98
#define SH3_TCR0			0xfffffe9c
#define SH3_TCOR1			0xfffffea0
#define SH3_TCNT1			0xfffffea4
#define SH3_TCR1			0xfffffea8
#define SH3_TCOR2			0xfffffeac
#define SH3_TCNT2			0xfffffeb0
#define SH3_TCR2			0xfffffeb4
#define SH3_TCPR2			0xfffffeb8

#define SH4_TOCR			0xffd80000
#define SH4_TSTR			0xffd80004
#define SH4_TCOR0			0xffd80008
#define SH4_TCNT0			0xffd8000c
#define SH4_TCR0			0xffd80010
#define SH4_TCOR1			0xffd80014
#define SH4_TCNT1			0xffd80018
#define SH4_TCR1			0xffd8001c
#define SH4_TCOR2			0xffd80020
#define SH4_TCNT2			0xffd80024
#define SH4_TCR2			0xffd80028
#define SH4_TCPR2			0xffd8002c

#if defined(SH3) && defined(SH4)
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
#endif /* SH3 && SH4 */

#define TOCR_TCOE			  0x01
#define TSTR_STR2			  0x04
#define TSTR_STR1			  0x02
#define TSTR_STR0			  0x01
#define TCR_ICPF			  0x0200
#define TCR_UNF				  0x0100
#define TCR_ICPE1			  0x0080
#define TCR_ICPE0			  0x0040
#define TCR_UNIE			  0x0020
#define TCR_CKEG1			  0x0010
#define TCR_CKEG0			  0x0008
#define TCR_TPSC2			  0x0004
#define TCR_TPSC1			  0x0002
#define TCR_TPSC0			  0x0001
#define TCR_TPSC_P4			  0x0000
#define TCR_TPSC_P16			  0x0001
#define TCR_TPSC_P64			  0x0002
#define TCR_TPSC_P256			  0x0003
#define SH3_TCR_TPSC_RTC		  0x0004
#define SH3_TCR_TPSC_TCLK		  0x0005
#define SH4_TCR_TPSC_P512		  0x0004
#define SH4_TCR_TPSC_RTC		  0x0006
#define SH4_TCR_TPSC_TCLK		  0x0007

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
#define MINYEAR		2001	/* "today" */
#define RTC_CLOCK	16384	/* Hz */

/*
 * hpcsh clock module
 *  + default 64Hz
 *  + use TMU channel 0 as clock interrupt source.
 *  + TMU channel 0 input source is SH internal RTC output. (1.6384kHz)
 */
static void clock_register_init(void);
/* TMU */
/* interrupt handler is timing critical. prepared for each. */
int sh3_clock_intr(void *);
int sh4_clock_intr(void *);
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
#define TMU_START(x)							\
do {									\
	_reg_write_1(SH_(TSTR), _reg_read_1(SH_(TSTR)) & ~TSTR_STR ## x);\
	_reg_write_4(SH_(TCNT ## x), 0xffffffff);			\
	_reg_write_1(SH_(TSTR), _reg_read_1(SH_(TSTR)) | TSTR_STR ## x);\
} while (/*CONSTCOND*/0)
#define TMU_ELAPSED(x)							\
	(0xffffffff - _reg_read_4(SH_(TCNT ## x)))
void
clock_init()
{

	u_int32_t t0, t1;

	clock_register_init();

	/* initialize TMU */
	_reg_write_2(SH_(TCR0), 0);
	_reg_write_2(SH_(TCR1), 0);
	_reg_write_2(SH_(TCR2), 0);

	/* stop all counter */
	_reg_write_1(SH_(TSTR), 0);
	/* set TMU channel 0 source to RTC counter clock (16.384kHz) */
	_reg_write_2(SH_(TCR0), CPU_IS_SH3 ? SH3_TCR_TPSC_RTC : SH4_TCR_TPSC_RTC);

	/*
	 * estimate CPU clock.
	 */
	TMU_START(0);
	DELAY_LOOP(10000000);
	t0 = TMU_ELAPSED(0);
	__cpuclock = (100000000 / t0) * RTC_CLOCK;

	if (CPU_IS_SH4)
		__cpuclock >>= 1;	/* two-issue */

	__cnt_delay = (RTC_CLOCK * 10) / t0;

	/*
	 * estimate PCLOCK
	 */
	/* set TMU channel 1 source to PCLOCK / 4 */
	_reg_write_2(SH_(TCR1), TCR_TPSC_P4);
	TMU_START(0);
	TMU_START(1);
	delay(1000000);
	t0 = TMU_ELAPSED(0);
	t1 = TMU_ELAPSED(1);

	__pclock = (t1 / t0) * RTC_CLOCK * 4;

	/* stop all counter */
	_reg_write_1(SH_(TSTR), 0);

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

	tv->tv_usec += ((__cnt_clock - _reg_read_4(SH_(TCNT0))) * 1000000) /
	    RTC_CLOCK;
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
	_reg_write_1(SH_(TSTR), _reg_read_1(SH_(TSTR)) & ~TSTR_STR1);
	_reg_write_2(SH_(TCR1), TCR_UNIE | 
	    (CPU_IS_SH3 ? SH3_TCR_TPSC_RTC : SH4_TCR_TPSC_RTC));
	__cnt_clock = RTC_CLOCK / hz - 1;
	_reg_write_4(SH_(TCOR1), __cnt_clock);
	_reg_write_4(SH_(TCNT1), __cnt_clock);
	_reg_write_1(SH_(TSTR), _reg_read_1(SH_(TSTR)) | TSTR_STR1);

	shb_intr_establish(TMU1_IRQ, IST_EDGE, IPL_CLOCK,
	    CPU_IS_SH3 ? sh3_clock_intr : sh4_clock_intr, 0);
}

#ifdef SH3
int
sh3_clock_intr(void *arg) /* trap frame */
{
	/* clear underflow status */
	_reg_write_2(SH3_TCR1, _reg_read_2(SH3_TCR1) & ~TCR_UNF);

	__dbg_heart_beat(HEART_BEAT_WHITE);
	hardclock(arg);

	return (1);
}
#endif /* SH3 */
#ifdef SH4
int
sh4_clock_intr(void *arg) /* trap frame */
{
	/* clear underflow status */
	_reg_write_2(SH4_TCR1, _reg_read_2(SH4_TCR1) & ~TCR_UNF);

	__dbg_heart_beat(HEART_BEAT_WHITE);
	hardclock(arg);

	return (1);
}
#endif /* SH4 */

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
	_reg_write_1(SH_(RCR1), 0);
	/* make sure to start RTC */
	_reg_write_1(SH_(RCR2), SH_RCR2_ENABLE | SH_RCR2_START);
}

int
rtc_gettime(struct clock_ymdhms *dt)
{
	int retry = 8;

	/* disable carry interrupt */
	_reg_write_1(SH_(RCR1), _reg_read_1(SH_(RCR1)) & ~SH_RCR1_CIE);

	do {
		u_int8_t r = _reg_read_1(SH_(RCR1));
		r &= ~SH_RCR1_CF;
		r |= SH_RCR1_AF; /* don't clear alarm flag */
		_reg_write_1(SH_(RCR1), r);

		/* read counter */
#define RTCGET(x, y)	dt->dt_ ## x = FROMBCD(_reg_read_1(SH_(R ## y ## CNT)))
		RTCGET(year, YR);
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

	return (0);
}

#define SH3REG(x)	__sh_ ## x = SH3_ ## x
#define SH4REG(x)	__sh_ ## x = SH4_ ## x
static void
clock_register_init()
{
#if defined(SH3) && defined(SH4)
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
#endif /* SH3 && SH4 */
}
#undef SH3REG
#undef SH4REG
