/*      $NetBSD: clock.c,v 1.15 2001/05/31 18:46:09 scw Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *      @(#)clock.c     8.1 (Berkeley) 6/11/93
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/psl.h>
#include <machine/bus.h>

#include <mvme68k/mvme68k/clockreg.h>
#include <mvme68k/mvme68k/clockvar.h>

#if defined(GPROF)
#include <sys/gmon.h>
#endif

static	struct clock_attach_args *clock_args;

struct	evcnt clock_profcnt;
struct	evcnt clock_statcnt;

/*
 * Statistics clock interval and variance, in usec.  Variance must be a
 * power of two.  Since this gives us an even number, not an odd number,
 * we discard one case and compensate.  That is, a variance of 1024 would 
 * give us offsets in [0..1023].  Instead, we take offsets in [1..1023].
 * This is symmetric about the point 512, or statvar/2, and thus averages
 * to that value (assuming uniform random numbers).
 */
/* XXX fix comment to match value */
int	clock_statvar = 8192;
int	clock_statmin;		/* statclock interval - (1/2 * variance) */

/*
 * autoconf
 */

struct chiptime {
        int     sec;
        int     min;
        int     hour;
        int     wday;
        int     day;
        int     mon;
        int     year;
};

static void timetochip __P((struct chiptime *));
static long chiptotime __P((int, int, int, int, int, int));

/*
 * Common parts of clock autoconfiguration.
 */
void
clock_config(dev, ca)
	struct device *dev;
	struct clock_attach_args *ca;
{
	extern int delay_divisor;	/* from machdep.c */

	/* Hook up that which we need. */
	clock_args = ca;

	/* Print info about the clock. */
	printf(": delay_divisor %d\n", delay_divisor);
}

/*
 * Set up the real-time and statistics clocks.  Leave stathz 0 only
 * if no alternative timer is available.
 *
 * The frequencies of these clocks must be an even number of microseconds.
 */
void
cpu_initclocks()
{
	int statint, minint;

	if (clock_args == NULL)
		panic("clock not configured");

	if (1000000 % hz) {
		printf("cannot get %d Hz clock; using 100 Hz\n", hz);
		hz = 100;
		tick = 1000000 / hz;
	}
	if (stathz == 0)
		stathz = hz;
	if (1000000 % stathz) {
		printf("cannot get %d Hz statclock; using 100 Hz\n", stathz);
		stathz = 100;
	}
	profhz = stathz;	/* always */ 

	statint = 1000000 / stathz;
	minint = statint / 2 + 100;
	while (clock_statvar > minint)
		clock_statvar >>= 1;

	clock_statmin = statint - (clock_statvar >> 1);

	/* Call the machine-specific initclocks hook. */
	(*clock_args->ca_initfunc)(clock_args->ca_arg, tick, statint);
}

void
setstatclockrate(newhz)
	int newhz;
{

	/* XXX should we do something here? XXX */
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
microtime(tvp)
	struct timeval *tvp;
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
 * BCD to decimal and decimal to BCD.
 */
#define	FROMBCD(x)	(((x) >> 4) * 10 + ((x) & 0xf))
#define	TOBCD(x)	(((x) / 10 * 16) + ((x) % 10))

#define	SECDAY		(24 * 60 * 60)
#define	SECYR		(SECDAY * 365)
#define	LEAPYEAR(y)	(((y) & 3) == 0)

/*
 * This code is defunct after 2068.
 * Will Unix still be here then??
 */
const short dayyr[12] =
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

static long
chiptotime(sec, min, hour, day, mon, year)
	int sec, min, hour, day, mon, year;
{
	int days, yr;

	sec = FROMBCD(sec);
	min = FROMBCD(min);
	hour = FROMBCD(hour);
	day = FROMBCD(day);
	mon = FROMBCD(mon);
	year = FROMBCD(year) + YEAR0;

	/* simple sanity checks */
	if (year < 70 || mon < 1 || mon > 12 || day < 1 || day > 31)
		return (0);
	days = 0;
	for (yr = 70; yr < year; yr++)
		days += LEAPYEAR(yr) ? 366 : 365;
	days += dayyr[mon - 1] + day - 1;
	if (LEAPYEAR(yr) && mon > 2)
		days++;
	/* now have days since Jan 1, 1970; the rest is easy... */
	return (days * SECDAY + hour * 3600 + min * 60 + sec);
}

static void
timetochip(c)
        struct chiptime *c;
{
        int t, t2, t3, now = time.tv_sec;

        /* compute the year */
        t2 = now / SECDAY;
        t3 = (t2 + 2) % 7;      /* day of week */
        c->wday = TOBCD(t3 + 1);

        t = 69;
        while (t2 >= 0) {       /* whittle off years */
                t3 = t2;
                t++;
                t2 -= LEAPYEAR(t) ? 366 : 365;
        }
        c->year = t;

        /* t3 = month + day; separate */
        t = LEAPYEAR(t);
        for (t2 = 1; t2 < 12; t2++)
                if (t3 < dayyr[t2] + (t && t2 > 1))
                        break;

        /* t2 is month */
        c->mon = t2;
        c->day = t3 - dayyr[t2 - 1] + 1;
        if (t && t2 > 2)
                c->day--;

        /* the rest is easy */
        t = now % SECDAY;
        c->hour = t / 3600;
        t %= 3600;
        c->min = t / 60;
        c->sec = t % 60;

        c->sec = TOBCD(c->sec);
        c->min = TOBCD(c->min);
        c->hour = TOBCD(c->hour);
        c->day = TOBCD(c->day);
        c->mon = TOBCD(c->mon);
        c->year = TOBCD(c->year - YEAR0);
}


#define	rtc_read(r)	bus_space_read_1(clock_args->ca_bust, \
					 clock_args->ca_bush, (r))
#define	rtc_write(r,v)	bus_space_write_1(clock_args->ca_bust, \
					  clock_args->ca_bush, (r), (v))

/*
 * Set up the system's time, given a `reasonable' time value.
 */
void
inittodr(base)
        time_t base;
{
        int sec, min, hour, day, mon, year;
        int badbase = 0, waszero = base == 0;

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

        /* enable read (stop time) */
	rtc_write(MK48TREG_CSR, rtc_read(MK48TREG_CSR) | CLK_READ);

        sec = rtc_read(MK48TREG_SEC);
        min = rtc_read(MK48TREG_MIN);
        hour = rtc_read(MK48TREG_HOUR);
        day = rtc_read(MK48TREG_MDAY);
        mon = rtc_read(MK48TREG_MONTH);
        year = rtc_read(MK48TREG_YEAR);

        /* time wears on */
	rtc_write(MK48TREG_CSR, rtc_read(MK48TREG_CSR) & ~CLK_READ);

        if ((time.tv_sec = chiptotime(sec, min, hour, day, mon, year)) == 0) {
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


/*
 * Reset the clock based on the current time.
 * Used when the current clock is preposterous, when the time is changed,
 * and when rebooting.  Do nothing if the time is not yet known, e.g.,
 * when crashing during autoconfig.
 */
void
resettodr()
{
        struct chiptime c;

        if (!time.tv_sec)
                return;

        timetochip(&c);

        /* enable write */
	rtc_write(MK48TREG_CSR, rtc_read(MK48TREG_CSR) | CLK_WRITE);

        rtc_write(MK48TREG_SEC, c.sec);
        rtc_write(MK48TREG_MIN, c.min);
        rtc_write(MK48TREG_HOUR, c.hour);
        rtc_write(MK48TREG_WDAY, c.wday);
        rtc_write(MK48TREG_MDAY, c.day);
        rtc_write(MK48TREG_MONTH, c.mon);
        rtc_write(MK48TREG_YEAR, c.year);

        /* load them up */
	rtc_write(MK48TREG_CSR, rtc_read(MK48TREG_CSR) & ~CLK_WRITE);
}
