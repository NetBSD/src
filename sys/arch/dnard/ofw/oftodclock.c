/*	$NetBSD: oftodclock.c,v 1.1 2001/05/09 16:08:44 matt Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/* 
 *   Real time clock (RTC) functions using OFW /rtc device:
 *      inittodr()
 *      resettodr()
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <dev/ofw/openfirm.h>

/* The OFW RTC interface, straight from wmb:

selftest  ( -- error? )
	Designed to be called from the "test" user interface command, e.g.
		test /rtc
	which displays a message if the test fails.
	Basically all this does is to check the battery.

check-battery  ( -- error? )
	Called from an open instance; returns non-zero if the battery
	is dead.  Also displays a message to that effect, which it probably
	should not do.

set-time  ( s m h d m y -- )
get-time  ( -- s m h d m y )
	Called from an open instance.
	seconds: 0-59
	minutes: 0-59
	hours: 0-23
	day: 1-31
	month: 1-12
	year: e.g. 1997
*/

#define OFRTC_SEC 0
#define OFRTC_MIN 1
#define OFRTC_HR  2
#define OFRTC_DOM 3
#define OFRTC_MON 4
#define OFRTC_YR  5

static int month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define	SECMIN	((unsigned)60)			/* seconds per minute */
#define	SECHOUR	((unsigned)(60*SECMIN))		/* seconds per hour */
#define	SECDAY	((unsigned)(24*SECHOUR))	/* seconds per day */
#define	SECYR	((unsigned)(365*SECDAY))	/* seconds per common year */

static int
yeartoday(year)
	int year;
{

	return ((year % 4) ? 365 : 366);
}

static int timeset = 0;

static void
setthetime(thetime, warning)
	time_t thetime;
	int warning;
{

	timeset = 1;
	time.tv_sec = thetime;
	time.tv_usec = 0;

	if (warning)
		printf("WARNING: CHECK AND RESET THE DATE!\n");
}

static int ofrtc_phandle;
static int ofrtc_ihandle = 0;
static int ofrtcinited = 0;

static void
ofrtcinit(void)
{
	char buf[256];
	int  l;
	int  chosen;

	if (ofrtcinited) return;

	if ((ofrtc_phandle = OF_finddevice("/rtc")) == -1)
		panic("OFW RTC: no package");

	if ((l = OF_getprop(ofrtc_phandle, "device_type", buf, sizeof buf - 1)) < 0)
		panic("OFW RTC: no device type");

	if ((l >= sizeof buf) || strcmp(buf, "rtc"))
		panic("OFW RTC: bad device type");

	if ((chosen = OF_finddevice("/chosen")) == -1 ||
 	    OF_getprop(chosen, "clock", &ofrtc_ihandle, sizeof(int)) < 0) {
		ofrtc_ihandle = 0;
		return;
	}

	ofrtc_ihandle = of_decode_int((unsigned char *)&ofrtc_ihandle);

	ofrtcinited = 1;
}

static int
ofrtcstatus(void)
{
	char status[256];
	int  l;

	if ((ofrtc_ihandle == 0) || (l = OF_getprop(ofrtc_phandle, "status", 
	    status, sizeof status - 1)) < 0) {
		printf("OFW RTC: old firmware does not support RTC\n");
		return 0;
	}

	status[sizeof status - 1] = 0; /* always null terminate */

	if (strcmp(status, "okay")) { /* something is wrong */
		printf("RTC: %s\n", status);
		return 0;
	}

	return 1; /* all systems are go */
}

/*
 * Initialize the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */
void
inittodr(base)
        time_t base;
{
	time_t n;
	int i, days = 0;
	int date[6];
	int yr;

	/*
	 * We mostly ignore the suggested time and go for the RTC clock time
	 * stored in the CMOS RAM.  If the time can't be obtained from the
	 * CMOS, or if the time obtained from the CMOS is 5 or more years
	 * less than the suggested time, we used the suggested time.  (In
	 * the latter case, it's likely that the CMOS battery has died.)
	 */

	if (base < 25*SECYR) {	/* if before 1995, something's odd... */
		printf("WARNING: preposterous time in file system\n");
		/* read the system clock anyway */
		/* happy b-day sarina */
		base = 26*SECYR + 24*SECDAY + 18 * SECHOUR + 58 * SECMIN;
	}

	ofrtcinit();

	if (!ofrtcstatus()) {
		setthetime(base, 1);
		return;
	}

	if (OF_call_method("get-time", ofrtc_ihandle, 0, 6,
	    date, date + 1, date + 2, date + 3, date + 4, date + 5)) {
		printf("OFW RTC: get-time failed\n");
		setthetime(base, 1);
		return;
	}
	  
	n  = date[OFRTC_SEC];
	n += date[OFRTC_MIN]       * SECMIN;
	n += date[OFRTC_HR]        * SECHOUR;
	n += (date[OFRTC_DOM] - 1) * SECDAY;

	yr = date[OFRTC_YR];

	if (yeartoday(yr) == 366)
		month[1] = 29;
	for (i = date[OFRTC_MON] - 2; i >= 0; i--)
		days += month[i];
	month[1] = 28;
	for (i = 1970; i < yr; i++)
		days += yeartoday(i);
	n += days * 3600 * 24;

	n += rtc_offset * 60;

	if (base < n - 5*SECYR)
		printf("WARNING: file system time much less than clock time\n");

	else if (base > n + 5*SECYR) {
		printf("WARNING: clock time much less than file system time\n");
		printf("WARNING: using file system time\n");
		setthetime(base, 1);
		return;
	}

	setthetime(n, 0);
}

/*
 * Reset the clock.
 */
void
resettodr(void)
{
	time_t n;
	int diff, i, j;
	int sec, min, hr, dom, mon, yr;

	/* old version of the firmware? */
	if (ofrtc_ihandle == 0) return;

	/*
	 * We might have been called by boot() due to a crash early
	 * on.  Don't reset the clock chip in this case.
	 */
	if (!timeset)
		return;

	diff = rtc_offset * 60;
	n = (time.tv_sec - diff) % (3600 * 24);   /* hrs+mins+secs */
	sec = n % 60;
	n /= 60;
	min = n % 60;
	hr  = n / 60;

	n = (time.tv_sec - diff) / (3600 * 24);	/* days */

	for (j = 1970, i = yeartoday(j); n >= i; j++, i = yeartoday(j))
		n -= i;

	yr = j;

	if (i == 366)
		month[1] = 29;
	for (i = 0; n >= month[i]; i++)
		n -= month[i];
	month[1] = 28;
	mon = ++i;
	dom = ++n;

	if (OF_call_method("set-time", ofrtc_ihandle, 6, 0,
	     sec, min, hr, dom, mon, yr))
		printf("OFW RTC: set-time failed\n");
}
