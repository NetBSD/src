/*	$NetBSD: clock.c,v 1.4 2014/11/17 02:15:49 christos Exp $	*/

/*
 * This is a slightly modified version of mvme68k's standalone clock.c.
 * As there was no attribution/copyright header on that file, there's
 * not going to be one for this file.
 */

#include <sys/types.h>
#include <dev/clock_subr.h>

#include "stand.h"
#include "net.h"
#include "libsa.h"
#include "bugsyscalls.h"

#define FROMBCD(x)      (int)((((unsigned int)(x)) >> 4) * 10 +\
			      (((unsigned int)(x)) & 0xf))

#define YEAR0		68

/*
 * This code is defunct after 2068.
 * Will Unix still be here then??
 */
const short dayyr[12] =
{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

static u_long
chiptotime(int sec, int min, int hour, int day, int mon, int year)
{
	int days, yr;

	sec = FROMBCD(sec);
	min = FROMBCD(min);
	hour = FROMBCD(hour);
	day = FROMBCD(day);
	mon = FROMBCD(mon);
	year = FROMBCD(year) + YEAR0;
	if (year < 70)
		year = 70;

	/* simple sanity checks */
	if (year < 70 || mon < 1 || mon > 12 || day < 1 || day > 31)
		return (0);
	days = 0;
	for (yr = 70; yr < year; yr++)
		days += days_per_year(yr);
	days += dayyr[mon - 1] + day - 1;
	if (is_leap_year(yr) && mon > 2)
		days++;
	/* now have days since Jan 1, 1970; the rest is easy... */
 	return days * SECS_PER_DAY + hour * SECS_PER_HOUR
	    + min * SECS_PER_MINUTE + sec;
}

satime_t
getsecs(void)
{
	struct bug_rtc_rd rr;

	bugsys_rtc_rd(&rr);

	return (chiptotime(rr.rr_second, rr.rr_minute, rr.rr_hour,
	    rr.rr_dayofmonth, rr.rr_month, rr.rr_year));
}
