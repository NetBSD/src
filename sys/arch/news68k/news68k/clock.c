/*      $NetBSD: clock.c,v 1.3 2000/01/19 02:52:19 msaitoh Exp $	*/

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
/*
 * news68k/clock.c - from mvme68k/clock.c
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <news68k/news68k/clockreg.h>
#include <news68k/news68k/clockvar.h>

#include <machine/cpu.h>

#if defined(GPROF)
#include <sys/gmon.h>
#endif

static	struct clockreg *RTCbase = NULL;
static	caddr_t NVRAMbase = NULL;
static	int NVRAMsize;
static	void (*cpu_initclocks_hook) __P((int, int));

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
clock_config(dev, clockregs, nvram, nvramsize, initfunc)
	struct device *dev;
	caddr_t clockregs, nvram;
	int nvramsize;
	void (*initfunc) __P((int, int));
{
	extern int delay_divisor;	/* from machdep.c */

	if (RTCbase || NVRAMbase)
		panic("clock_config: too many clocks configured");

	/* Hook up that which we need. */
	RTCbase = (struct clockreg *)clockregs;
	NVRAMbase = nvram;
	NVRAMsize = nvramsize;
	cpu_initclocks_hook = initfunc;

	/* Print info about the clock. */
	printf(": Mostek MK48T0%d, %d bytes of NVRAM\n", (nvramsize / 1024),
	    nvramsize);
	printf("%s: delay_divisor %d\n", dev->dv_xname, delay_divisor);
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

	if (RTCbase == NULL)
		panic("clock not configured");

	if (1000000 % hz) {
		printf("cannot get %d Hz clock; using 100 Hz\n", hz);
		hz = 100;
		tick = 1000000 / hz;
	}

	/* Call the machine-specific initclocks hook. */

	(*cpu_initclocks_hook)(0, 0);
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
 * plus the amount of time since the last clock interrupt (clock.c:clkread).
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
	tvp->tv_usec++;
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
#define	FROMBCD(x)	((((x) >> 4) & 0xf) * 10 + ((x) & 0xf))
#define	TOBCD(x)	(((((x) / 10) % 10) << 4) + ((x) % 10))

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
	year = FROMBCD(year);
	year = year + (year >= 70 ? 1900 : 2000);

	/* simple sanity checks */
	if (mon < 1 || mon > 12 || day < 1 || day > 31)
		return (0);
	days = 0;
	for (yr = 1970; yr < year; yr++)
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
	if (c->year >= 100)
		c->year -= 100;

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
        c->year = TOBCD(c->year);
}

/*
 * Set up the system's time, given a `reasonable' time value.
 */
void
inittodr(base)
        time_t base;
{
        struct clockreg *cl = RTCbase;
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
        cl->cl_csr |= CLK_READ;         /* enable read (stop time) */
        sec = cl->cl_sec;
        min = cl->cl_min;
        hour = cl->cl_hour;
        day = cl->cl_mday;
        mon = cl->cl_month;
        year = cl->cl_year;
        cl->cl_csr &= ~CLK_READ;        /* time wears on */
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
        struct clockreg *cl;
        struct chiptime c;

        if (!time.tv_sec || (cl = RTCbase) == NULL)
                return;
        timetochip(&c);
        cl->cl_csr |= CLK_WRITE;        /* enable write */
        cl->cl_sec = c.sec;
        cl->cl_min = c.min;
        cl->cl_hour = c.hour;
        cl->cl_wday = c.wday;
        cl->cl_mday = c.day;
        cl->cl_month = c.mon;
        cl->cl_year = c.year;
        cl->cl_csr &= ~CLK_WRITE;       /* load them up */
}
