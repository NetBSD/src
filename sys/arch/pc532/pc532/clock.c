/*	$NetBSD: clock.c,v 1.6.2.1 1994/10/19 19:07:14 cgd Exp $
*/

/*-
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
 *
 */

/*
 * Primitive clock interrupt routines.
 *
 * Improved by Phil Budne ... 10/17/94.
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <machine/icu.h>

void spinwait __P((int));

/* Get access to the time variable. */
extern struct timeval time;
extern struct timezone tz;

/* Conversion data */
static const short daymon[12] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

/* Defines to make code more readable. */

#define FROMBCD(B)	(((B)>>4) * 10 + ((B)&0xf))
#define TOBCD(D)	((((D)/10)<<4) + (D)%10)

#define DAYMON(M,L)	(daymon[(M)] + ((L) && (M) > 0))

#define SECDAY		(24*60*60)
#define SECMON(M,L)	(DAYMON(M,L)*SECDAY)
#define SECYR		(365*SECDAY)
#define LEAPYEAR(Y)	(((Y) & 3) == 0)


startrtclock()
{
  int s;
  int x;
  int timer = (ICU_CLK_HZ) / hz;

  /* Write the timer values to the ICU. */
  WR_ADR (unsigned short, ICU_ADR + HCSV, timer);
  WR_ADR (unsigned short, ICU_ADR + HCCV, timer);

}


/* convert years to seconds since Jan 1, 1970 */
static unsigned long
ytos(y)
int y;
{
	int i;
	unsigned long ret;

	ret = 0;
	y -= 70;
	for(i=0;i<y;i++) {
	    ret += SECYR;
	    if (LEAPYEAR(i))
		ret += SECDAY;
	}
	return ret;
}


/*
 *  Access to the real time clock.
 * 
 */

extern int have_rtc;

void
inittodr(base)
    time_t base;
{
    unsigned char buffer[8];
    unsigned int year, month, dom, hour, min, sec, csec;

    if (!have_rtc) {
	time.tv_sec = base;
	return;
    }

    /* Read rtc and convert to seconds since Jan 1, 1970. */

    rw_rtc( buffer, 0);		/* Read the rtc. */

    /* convert to decimal */
    year = FROMBCD(buffer[7]);
    /* XXX apply offset, or tweak for 21st century? */

    month = FROMBCD(buffer[6]);
    dom = FROMBCD(buffer[5]);
    /* buffer[4] is dow! */
    hour = FROMBCD(buffer[3]);
    min = FROMBCD(buffer[2]);
    sec = FROMBCD(buffer[1]);
    csec = FROMBCD(buffer[0]);

    /* Check to see if it was really the rtc by checking for bad date info. */
    if (sec > 59 || min > 59 || hour > 23 || dom > 31 || month > 12) {
	printf("inittodr: No clock found\n");
	have_rtc = 0;
	time.tv_sec = base;
	return;
    }

    month--;			/* make zero based */
    dom--;			/* make zero based */

#ifdef TEST
    printf("ytos %d secmon %d dom %d hour %d, min %d\n",
	   ytos(year),		/* years */
	   SECMON(month, LEAPYEAR(year)), /* months */
	   dom * SECDAY,	/* days of month */
	   hour*60*60,		/* hours */
	   min*60);		/* minutes */

#endif

    sec += ytos(year)		/* years */
	+ SECMON(month, LEAPYEAR(year)) /* months */
	    + dom * SECDAY	/* days of month */
		+ hour*60*60	/* hours */
		    + min*60;	/* minutes */

    /* apply local offset */
    sec += tz.tz_minuteswest * 60;
    if (tz.tz_dsttime)
	sec -= 60 * 60;

    if (sec < base)
	printf ("WARNING: clock is earlier than last shutdown time.\n");
    time.tv_sec = sec;

    if (csec > 99)
	csec = 0;		/* ignore if bogus */
    time.tv_usec = csec * 10000;
}


/*
 * Reset clock chip to current system time
 * Phil Budne 10/17/94
 */
resettodr()
{
    int year, leap, mon, dom, hour, min, sec, csec;
    struct timeval tv;
    unsigned char buffer[8];
    unsigned long t, t2;

    tv = time;			/* XXX do under spl? */

    t = tv.tv_sec;

    /* XXX apply 1-day tweak? */

    /* apply local offset */
    sec -= tz.tz_minuteswest * 60;
    if (tz.tz_dsttime)
	sec += 60 * 60;

    year = 70;
    for (;;) {
	t2 = SECYR;
	if (LEAPYEAR(year))
	    t2 += SECDAY;
	if (t < t2)
	    break;
	year++;
	t -= t2;
    }

    leap = LEAPYEAR(year);
    mon = 0;
    for (;;) {
	mon++;
	if (t < SECMON(mon, leap))
	    break;
    }
    mon--;			/* get zero based */
    t -= SECMON(mon, leap);
    mon++;			/* make one based */

    dom = t / SECDAY;
    t %= SECDAY;

    hour = t / (60*60);
    t %= 60*60;

    min = t / 60;
    sec = t % 60;
    csec = tv.tv_usec / 10000;

    dom++;			/* make one-based */

#ifdef TEST
    printf("resettodr: %d/%d/%d %d:%02d:%02d.%02d\n",
	   mon, dom, year + 1900, hour, min, sec, csec );
#endif

    buffer[0] = TOBCD(csec);
    buffer[1] = TOBCD(sec);
    buffer[2] = TOBCD(min);
    buffer[3] = TOBCD(hour);
    buffer[4] = 0;		/* XXX DOW! */
    buffer[5] = TOBCD(dom);
    buffer[6] = TOBCD(mon);
    buffer[7] = TOBCD(year);    /* XXX remove offset, or mod by 100? */

    rw_rtc(buffer,1);		/* reset chip!! */
}

/*
 * Wire clock interrupt in.
 */
enablertclock()
{
  /* Set the clock interrupt enable (CICTL) */
  WR_ADR (unsigned char, ICU_ADR +CICTL, 0x30);
  PL_zero |= SPL_CLK | SPL_SOFTCLK | SPL_NET | SPL_IMP;
}

/*
 * Delay for some number of milliseconds.
 */
void
spinwait(int millisecs)
{
	DELAY(5000 * millisecs);
}

DELAY(n)
{ volatile int N = (n); while (--N > 0); }


/* new functions ... */
int
cpu_initclocks()
{
  startrtclock();
  enablertclock();
}

int
setstatclockrate()
{printf ("setstatclockrate\n");}

