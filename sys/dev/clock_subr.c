/*	$NetBSD: clock_subr.c,v 1.22 2014/09/07 11:50:23 martin Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: Utah $Hdr: clock.c 1.18 91/01/21$
 *
 *	@(#)clock.c	8.2 (Berkeley) 1/12/94
 */

/*
 * Generic routines to convert between a POSIX date
 * (seconds since 1/1/1970) and yr/mo/day/hr/min/sec
 * Derived from arch/hp300/hp300/clock.c
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif /* HAVE_NBTOOL_CONFIG_H */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock_subr.c,v 1.22 2014/09/07 11:50:23 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#else /* ! _KERNEL */
#include <string.h>
#include <time.h>
#include <errno.h>
#endif /* ! _KERNEL */

#include <dev/clock_subr.h>

static inline int leapyear(uint64_t year);
#define FEBRUARY	2
#define	days_in_year(a) 	(leapyear(a) ? 366 : 365)
#define	days_in_month(a) 	(month_days[(a) - 1])

/* for easier alignment:
 * time from the epoch to 2000 (there were 7 leap years): */
#define	DAYSTO2000	(365*30+7)

/* 4 year intervals include 1 leap year */
#define	DAYS4YEARS	(365*4+1)

/* 100 year intervals include 24 leap years */
#define	DAYS100YEARS	(365*100+24)

/* 400 year intervals include 97 leap years */
#define	DAYS400YEARS	(365*400+97)

static const int month_days[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/*
 * This inline avoids some unnecessary modulo operations
 * as compared with the usual macro:
 *   ( ((year % 4) == 0 &&
 *      (year % 100) != 0) ||
 *     ((year % 400) == 0) )
 * It is otherwise equivalent.
 */
static inline int
leapyear(uint64_t year)
{
	int rv = 0;

	if (year < 1969)
		return EINVAL;

	if ((year & 3) == 0) {
		rv = 1;
		if ((year % 100) == 0) {
			rv = 0;
			if ((year % 400) == 0)
				rv = 1;
		}
	}
	return rv;
}

time_t
clock_ymdhms_to_secs(struct clock_ymdhms *dt)
{
	uint64_t secs, i, year, days;

	year = dt->dt_year;

	/*
	 * Compute days since start of time
	 * First from years, then from months.
	 */
	if (year < POSIX_BASE_YEAR)
		return -1;
	days = 0;
	if (leapyear(year) && dt->dt_mon > FEBRUARY)
		days++;

	if (year < 2000) {
		/* simple way for early years */
		for (i = POSIX_BASE_YEAR; i < year; i++)
			days += days_in_year(i);
	} else {
		/* years are properly aligned */
		days += DAYSTO2000;
		year -= 2000;

		i = year / 400;
		days += i * DAYS400YEARS;
		year -= i * 400;

		i = year / 100;
		days += i * DAYS100YEARS;
		year -= i * 100;

		i = year / 4;
		days += i * DAYS4YEARS;
		year -= i * 4;

		for (i = dt->dt_year-year; i < dt->dt_year; i++)
			days += days_in_year(i);
	}


	/* Months */
	for (i = 1; i < dt->dt_mon; i++)
	  	days += days_in_month(i);
	days += (dt->dt_day - 1);

	/* Add hours, minutes, seconds. */
	secs = (((uint64_t)days
	    * 24 + dt->dt_hour)
	    * 60 + dt->dt_min)
	    * 60 + dt->dt_sec;

	if ((time_t)secs < 0 || secs > __type_max(time_t))
		return -1;
	return secs;
}

int
clock_secs_to_ymdhms(time_t secs, struct clock_ymdhms *dt)
{
	int leap;
	uint64_t i;
	time_t days;
	time_t rsec;	/* remainder seconds */

	if (secs < 0)
		return EINVAL;

	days = secs / SECDAY;
	rsec = secs % SECDAY;

	/* Day of week (Note: 1/1/1970 was a Thursday) */
	dt->dt_wday = (days + 4) % 7;

	if (days >= DAYSTO2000) {
		days -= DAYSTO2000;
		dt->dt_year = 2000;

		i = days / DAYS400YEARS;
		days -= i*DAYS400YEARS;
		dt->dt_year += i*400;

		i = days / DAYS100YEARS;
		days -= i*DAYS100YEARS;
		dt->dt_year += i*100;

		i = days / DAYS4YEARS;
		days -= i*DAYS4YEARS;
		dt->dt_year += i*4;

		for (i = dt->dt_year; days >= days_in_year(i); i++)
			days -= days_in_year(i);
		dt->dt_year = i;
	} else {
		/* Subtract out whole years, counting them in i. */
		for (i = POSIX_BASE_YEAR; days >= days_in_year(i); i++)
			days -= days_in_year(i);
		dt->dt_year = i;
	}

	/* Subtract out whole months, counting them in i. */
	for (leap = 0, i = 1; days >= days_in_month(i)+leap; i++) {
		days -= days_in_month(i)+leap;
		if (i == 1 && leapyear(dt->dt_year))
			leap = 1;
		else
			leap = 0;
	}
	dt->dt_mon = i;

	/* Days are what is left over (+1) from all that. */
	dt->dt_day = days + 1;

	/* Hours, minutes, seconds are easy */
	dt->dt_hour = rsec / 3600;
	rsec = rsec % 3600;
	dt->dt_min  = rsec / 60;
	rsec = rsec % 60;
	dt->dt_sec  = rsec;

	return 0;
}
