/*
 * Copyright (c) 1993 Paul Mackerras.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, provided that the above copyright
 * notice appears in all copies.  This software is provided without any
 * warranty, express or implied.  The author makes no representations
 * about the suitability of this software for any purpose.
 *
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	from: Utah Hdr: clock.c 1.18 91/01/21
 *	from: @(#)clock.c	7.6 (Berkeley) 5/7/91
 *	$Id: clock.c,v 1.1 1994/02/22 23:49:28 paulus Exp $
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <da30/da30/clockreg.h>
#include <da30/da30/iio.h>

#include <machine/psl.h>
#include <machine/cpu.h>

#if defined(GPROF) && defined(PROFTIMER)
#include <sys/gprof.h>
#endif

static int month_days[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
struct rtc_tm *gmt_to_rtc();
static int tobin(), tobcd();
volatile struct rtc *rtcaddr = NULL;
static int rtcinited = 0;

/*
 * Connect up with autoconfiguration stuff,
 * mainly to get the address.
 */
void clockattach __P((struct device *, struct device *, void *));
int  clockmatch __P((struct device *, struct cfdata *, void *));

struct cfdriver clockcd = {
    NULL, "clock", clockmatch, clockattach, DV_TTY, sizeof(struct device), 0
};

int
clockmatch(parent, cf, args)
    struct device *parent;
    struct cfdata *cf;
    void *args;
{
    return rtcaddr == NULL && !badbaddr((caddr_t) IIO_CFLOC_ADDR(cf));
}

void
clockattach(parent, self, args)
    struct device *parent, *self;
    void *args;
{
    iio_print(self->dv_cfdata);

    rtcaddr = (volatile struct rtc *) IIO_CFLOC_ADDR(self->dv_cfdata);
}

/*
 * Machine-dependent clock routines.
 *
 * Startrtclock restarts the real-time clock, which provides
 * hardclock interrupts to kern_clock.c, but doesn't enable
 * interrupts.  We use the 1/100 second periodic interrupt of
 * the DP8570 for this.
 *
 * Enablertclock enables clock interrupts.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.
 *
 * Resettodr restores the time of day hardware after a time change.
 */

/*
 * Start the real-time clock.
 */
startrtclock()
{
	if( !rtcinited )
		init_rtc();
}

/*
 * Enable periodic interrupts from the RTC.
 */
enablertclock()
{
	register volatile struct rtc *rtc = rtcaddr;

	if( (rtc = rtcaddr) != NULL ){
		rtc->msr = REG_SEL;
		rtc->ictrl0 |= PER_10MIL;  /* enable 1/100 second interrupt */
	} else
		printf("enablertclock: warning: cannot enable periodic interrupts\n");
}

/*
 * Returns number of usec since last recorded clock "tick"
 * (i.e. clock interrupt).
 */
clkread()
{
	register volatile struct rtc *rtc = rtcaddr;

	return 0;
}

#ifdef PROFTIMER
#error proftimer stuff not implemented yet
/*
 * This code allows the kernel to use one of the extra timers on
 * the clock chip for profiling, instead of the regular system timer.
 * The advantage of this is that the profiling timer can be turned up to
 * a higher interrupt rate, giving finer resolution timing. The profclock
 * routine is called from the lev6intr in locore, and is a specialized
 * routine that calls addupc. The overhead then is far less than if
 * hardclock/softclock was called. Further, the context switch code in
 * locore has been changed to turn the profile clock on/off when switching
 * into/out of a process that is profiling (startprofclock/stopprofclock).
 * This reduces the impact of the profiling clock on other users, and might
 * possibly increase the accuracy of the profiling. 
 */
int  profint   = PRF_INTERVAL;	/* Clock ticks between interrupts */
int  profscale = 0;		/* Scale factor from sys clock to prof clock */
char profon    = 0;		/* Is profiling clock on? */

/* profon values - do not change, locore.s assumes these values */
#define PRF_NONE	0x00
#define	PRF_USER	0x01
#define	PRF_KERNEL	0x80

initprofclock()
{
	/*
	 * The profile interrupt interval must be an even divisor
	 * of the CLK_INTERVAL so that scaling from a system clock
	 * tick to a profile clock tick is possible using integer math.
	 */
	if (profint > CLK_INTERVAL || (CLK_INTERVAL % profint) != 0)
		profint = CLK_INTERVAL;
	profscale = CLK_INTERVAL / profint;
}

startprofclock()
{
	register struct clkreg *clk = (struct clkreg *)clkstd[0];

	clk->clk_msb3 = (profint-1) >> 8 & 0xFF;
	clk->clk_lsb3 = (profint-1) & 0xFF;

	clk->clk_cr2 = CLK_CR3;
	clk->clk_cr3 = CLK_IENAB;
}

stopprofclock()
{
	register struct clkreg *clk = (struct clkreg *)clkstd[0];

	clk->clk_cr2 = CLK_CR3;
	clk->clk_cr3 = 0;
}

#ifdef GPROF
/*
 * profclock() is expanded in line in lev6intr() unless profiling kernel.
 * Assumes it is called with clock interrupts blocked.
 */
profclock(pc, ps)
	caddr_t pc;
	int ps;
{
	/*
	 * Came from user mode.
	 * If this process is being profiled record the tick.
	 */
	if (USERMODE(ps)) {
		if (p->p_stats.p_prof.pr_scale)
			addupc(pc, &curproc->p_stats.p_prof, 1);
	}
	/*
	 * Came from kernel (supervisor) mode.
	 * If we are profiling the kernel, record the tick.
	 */
	else if (profiling < 2) {
		register int s = pc - s_lowpc;

		if (s < s_textsize)
			kcount[s / (HISTFRACTION * sizeof (*kcount))]++;
	}
	/*
	 * Kernel profiling was on but has been disabled.
	 * Mark as no longer profiling kernel and if all profiling done,
	 * disable the clock.
	 */
	if (profiling && (profon & PRF_KERNEL)) {
		profon &= ~PRF_KERNEL;
		if (profon == PRF_NONE)
			stopprofclock();
	}
}
#endif
#endif

/*
 * Initialize the time of day register, based on the time base which is, e.g.
 * from a filesystem.  N.B. we store local time in the RTC (uncorrected for
 * daylight savings).
 */
inittodr(base)
	time_t base;
{
	u_long timbuf = base;	/* assume no battery clock exists */

	if (!rtcinited)
		init_rtc();

	/*
	 * rtc_to_gmt converts and stores the gmt in timbuf.
	 * If an error is detected in rtc_to_gmt, or if the filesystem
	 * time is more recent than the gmt time in the clock,
	 * then use the filesystem time and warn the user.
 	 */
	if (rtcaddr != NULL && (!rtc_to_gmt(&timbuf) || timbuf < base) ){
		printf("WARNING: bad date in battery-backed clock\n");
		timbuf = base;
	}
	if (base < 18*SECYR) {
		printf("WARNING: preposterous time in file system");
		timbuf = 18*SECYR + 30*SECDAY + SECDAY/2;
		printf(" -- CHECK AND RESET THE DATE!\n");
	}
	
	/* Forget about usecs for now */
	time.tv_sec = timbuf;
}

resettodr()
{
	register volatile struct rtc *rtc = rtcaddr;
	register struct rtc_tm *tm;

	if( rtc == NULL )
		return;
	tm = gmt_to_rtc(time.tv_sec);

	rtc->msr = REG_SEL;
	rtc->rtm &= ~(START | LEAP_CTR);	/* stop clock */
	rtc->hunths = 0;
	rtc->secs = tobcd(tm->tm_sec);
	rtc->mins = tobcd(tm->tm_min);
	rtc->hours = tobcd(tm->tm_hour);
	rtc->days = tobcd(tm->tm_mday);
	rtc->months = tobcd(tm->tm_mon);
	rtc->years = tobcd(tm->tm_year - 60);
	rtc->jul_lo = tobcd(tm->tm_yday % 100);
	rtc->jul_hi = tm->tm_yday / 100;
	rtc->dayweek = tm->tm_wday;
	rtc->rtm |= tm->tm_year & LEAP_CTR;
	rtc->rtm |= START;			/* start the clock again */
}

init_rtc()
{
	register volatile struct rtc *rtc = rtcaddr;

	rtcinited = 1;
	if (rtc == NULL) {
		printf("WARNING: no battery-backed timer/clock\n");
		return;
	}

	rtc->msr = 0;
	rtc->t0ctrl = 0;
	rtc->t1ctrl = 0;
	rtc->irout = 0;
	if( (rtc->pfr & OSC_FAIL) != 0 ){
		rtc->pfr = 0;		/* clear single-supply & test mode */
		rtc->msr = REG_SEL;
		rtc->rtm = XTAL_32768;	/* clock stopped at present */
		printf("WARNING: Battery-backed clock has stopped\n");
	}
	rtc->msr = REG_SEL;
	rtc->rtm |= START;
	rtc->ictrl0 = 0;		/* disable all ints & clear flags */
	rtc->ictrl1 = 0;
	rtc->msr = T1_INT | T0_INT | ALARM_INT | PERIOD_INT;

	/* enable power fail interrupt so we can check for low battery */
	rtc->irout = PFAIL_ROUTE;	/* send pfail int to MFO pin */
	rtc->msr = REG_SEL;
	rtc->ictrl1 = PFAIL_INTEN;
	rtc->msr = 0;
	DELAY(10000);
	if( (rtc->irout & LOW_BATTERY) != 0 )
		printf("WARNING: Clock battery voltage low\n");
	rtc->irout = 0;
	rtc->msr = REG_SEL;
	rtc->ictrl1 = 0;

}

struct rtc_tm *
gmt_to_rtc(tim)
	long tim;
{
	register int i;
	register long hms, day;
	static struct rtc_tm rt;

	tim -= tz.tz_minuteswest * 60;		/* convert to local time */
	day = tim / SECDAY;
	hms = tim % SECDAY;

	/* Hours, minutes, seconds are easy */
	rt.tm_hour = hms / 3600;
	rt.tm_min  = (hms % 3600) / 60;
	rt.tm_sec  = (hms % 3600) % 60;

	/* Number of years in days */
	rt.tm_wday = (day + START_DAY) % 7 + 1;
	for (i = STARTOFTIME - 1900; day >= days_in_year(i); i++)
	  	day -= days_in_year(i);
	rt.tm_year = i;
	rt.tm_yday = day + 1;
	
	/* Number of months in days left */
	if (leapyear(rt.tm_year))
		days_in_month(FEBRUARY) = 29;
	for (i = 1; day >= days_in_month(i); i++)
		day -= days_in_month(i);
	days_in_month(FEBRUARY) = 28;
	rt.tm_mon = i;

	/* Days are what is left over (+1) from all that. */
	rt.tm_mday = day + 1;  
	
	return(&rt);
}

rtc_to_gmt(timbuf)
	u_long *timbuf;
{
	register int i;
	register u_long tmp;
	register volatile struct rtc *rtc = rtcaddr;
	int year, month, day, hour, min, sec;

	if( rtcaddr == NULL )
		return 0;
	rtc->msr = REG_SEL;
	if( (rtc->rtm & START) == 0 )
		return 0;		/* clock stopped - i.e. not set */

	i = rtc->pfr;			/* clear seconds flag */
	do {
		sec = tobin(rtc->secs);
		min = tobin(rtc->mins);
		hour = tobin(rtc->hours);
		day = tobin(rtc->days);
		month = tobin(rtc->months);
		year = tobin(rtc->years) + 1960;
	} while( (rtc->pfr & PER_SECS) != 0 );

	range_test(hour, 0, 23);
	range_test(day, 1, 31);
	range_test(month, 1, 12);
	range_test(year, STARTOFTIME, 2059);

	tmp = 0;

	for (i = STARTOFTIME; i < year; i++)
		tmp += days_in_year(i);
	if (leapyear(year) && month > FEBRUARY)
		tmp++;

	for (i = 1; i < month; i++)
	  	tmp += days_in_month(i);

	tmp += (day - 1);
	tmp = ((tmp * 24 + hour) * 60 + min) * 60 + sec;

	*timbuf = tmp + tz.tz_minuteswest * 60;	/* convert from local time */
	return(1);
}

static int
tobcd(int x)
{
	return x + (x / 10) * 6;
}

static int
tobin(int x)
{
	return x - (x >> 4) * 6;
}
