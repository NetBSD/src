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
 *	$Id: clock.c,v 1.2 1994/06/18 12:09:43 paulus Exp $
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <da30/da30/clockreg.h>
#include <da30/da30/iio.h>

#include <machine/psl.h>
#include <machine/cpu.h>

#if defined(GPROF)
#include <sys/gmon.h>
#endif

/*
 * Machine-dependent clock routines.
 *
 * The DA30 has a DP8570 timer/clock chip.  We use the 1/100 second
 * periodic interrupt for the hardclock interrupts.  The DP8570
 * has two timers, of which we could use one for statclock interrupts,
 * but that isn't implemented yet.  It also maintains the current
 * time of day in BCD in a set of registers.
 */

static int month_days[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
struct rtc_tm *gmt_to_rtc();
static int tobin(), tobcd();
volatile struct rtc *RTCbase = NULL;
static int rtcinited = 0;
int clockdiv;
int clockcnt;

/*
 * Connect up with autoconfiguration stuff,
 * mainly to get the address.
 */
void clockattach __P((struct device *, struct device *, void *));
int  clockmatch __P((struct device *, struct cfdata *, void *));

struct cfdriver clockcd = {
    NULL, "clock", clockmatch, clockattach, DV_DULL, sizeof(struct device), 0
};

int
clockmatch(parent, cf, args)
    struct device *parent;
    struct cfdata *cf;
    void *args;
{
    return RTCbase == NULL && !badbaddr((caddr_t) IIO_CFLOC_ADDR(cf));
}

void
clockattach(parent, self, args)
    struct device *parent, *self;
    void *args;
{
    iio_print(self->dv_cfdata);

    RTCbase = (volatile struct rtc *) IIO_CFLOC_ADDR(self->dv_cfdata);
}

/*
 * Set up real-time clock; we don't have a statistics clock at
 * present.
 */
cpu_initclocks()
{
    register volatile struct rtc *rtc = RTCbase;

    if( !rtcinited )
	init_rtc();
    if( rtc != NULL ){
	if (hz == 0 || 100 % hz) {
	    printf("%d Hz clock not available; using 100 Hz\n", hz);
	    hz = 100;
	}
	clockdiv = 100 / hz;
	clockcnt = 0;
	rtc->msr = REG_SEL;
	rtc->ictrl0 |= PER_10MIL;  /* enable 1/100 second interrupt */
    } else
	printf("WARNING: cannot enable periodic interrupts\n");
    stathz = 0;
}

void
setstatclockrate(newhz)
    int newhz;
{
}

void
statintr(fp)
    struct clockframe *fp;
{
}

/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  We do this by returning the current time
 * plus the amount of time since the last clock interrupt (clock.c:clkread).
 *
 * Check that this time is no less than any previously-reported time,
 * which could happen around the time of a clock adjustment.  Just for fun,
 * we guarantee that the time will be greater than the value obtained by a
 * previous call.
 */
microtime(tvp)
	register struct timeval *tvp;
{
	int s = splhigh();
	static struct timeval lasttime;

	*tvp = time;
	tvp->tv_usec += clkread();
	while (tvp->tv_usec > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}

/*
 * Returns number of usec since last recorded clock "tick"
 * (i.e. clock interrupt).
 */
clkread()
{
	register volatile struct rtc *rtc = RTCbase;

	return 0;
}

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
	if (RTCbase != NULL && (!rtc_to_gmt(&timbuf) || timbuf < base) ){
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
	register volatile struct rtc *rtc = RTCbase;
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
	register volatile struct rtc *rtc = RTCbase;

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
	register volatile struct rtc *rtc = RTCbase;
	int year, month, day, hour, min, sec;

	if( RTCbase == NULL )
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
