/*	$NetBSD: printquota.c,v 1.6 2011/03/24 17:05:46 bouyer Exp $ */

/*
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
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
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1980, 1990, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)quota.c	8.4 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: printquota.c,v 1.6 2011/03/24 17:05:46 bouyer Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/types.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <inttypes.h>

#include "printquota.h"

/*
 * convert 64bit value to a printable string
 */
const char *
intprt(char *buf, size_t len, uint64_t val, int flags, int hflag)
{
	if (val == UQUAD_MAX)
		return (len > sizeof("unlimited")) ? "unlimited" : "-";

	if (flags & HN_B)
		val = dbtob(val);
	
	if (hflag) {
		(void)humanize_number(buf, len, (int64_t)val, "", HN_AUTOSCALE,
		    flags);
		return buf;
	}
	if (flags & HN_B) {
		/* traditionnal display: blocks are in kilobytes */
		val = val / 1024;
	}
	(void)snprintf(buf, len, "%" PRId64, val);
	return buf;
}

/*
 * Calculate the grace period and return a user-friendly string for it.
 */
#define MINUTE	60
#define HOUR	(MINUTE * 60)
#define DAY	(HOUR * 24)
#define WEEK	(DAY * 7)
#define MONTH	(DAY * 30)
#define YEAR	(DAY * 355)

const char *
timeprt(char *buf, size_t len, time_t now, time_t seconds)
{
	time_t years, months, weeks, days, hours, minutes;

	if (now > seconds)
		return "none";

	seconds -= now;

	minutes = (seconds + MINUTE / 2) / MINUTE;
	hours = (seconds + HOUR / 2) / HOUR;
	days = (seconds + DAY / 2) / DAY;
	years = (seconds + YEAR / 2) / YEAR;
	months = (seconds + MONTH / 2) / MONTH;
	weeks = (seconds + WEEK / 2) / WEEK;

	if (years >= 2) {
		(void)snprintf(buf, len, "%" PRId64 "years", years);
		return buf;
	}
	if (weeks > 9) {
		(void)snprintf(buf, len, "%" PRId64 "months", months);
		return buf;
	}
	if (days > 9) {
		(void)snprintf(buf, len, "%" PRId64 "weeks", weeks);
		return buf;
	}
	if (hours > 36) {
		(void)snprintf(buf, len, "%" PRId64 "days", days);
		return buf;
	}
	if (minutes > 60) {
		(void)snprintf(buf, len, "%2d:%d",
		    (int)(minutes / 60), (int)(minutes % 60));
		return buf;
	}
	(void)snprintf(buf, len, "%2d", (int)minutes);
	return buf;
}

/*
 * Calculate the grace period and return a precise string for it,
 * either in seconds or in format xWyDzHtMuS
 */
const char *
timepprt(char *buf, size_t len, time_t seconds, int hflag)
{
	ssize_t i = 0;

	if (hflag == 0) {
		(void)snprintf(buf, len, "%" PRId64, seconds);
		return buf;
	}

	if ((seconds / WEEK) > 0) {
		i += snprintf(buf + i, len - i, "%" PRId64 "W", seconds / WEEK);
		seconds = seconds % WEEK;
	}

	if (len - i < 3 || seconds == 0)
		return buf;

	if ((seconds / DAY) > 0) {
		i += snprintf(buf + i, len - i, "%" PRId64 "D", seconds / DAY);
		seconds = seconds % DAY;
	}

	if (len - i < 4 || seconds == 0)
		return buf;

	if ((seconds / HOUR) > 0) {
		i += snprintf(buf + i, len - i, "%" PRId64 "H", seconds / HOUR);
		seconds = seconds % HOUR;
	}

	if (len - i < 4 || seconds == 0)
		return buf;

	if ((seconds / MINUTE) > 0) {
		i += snprintf(buf + i , len - i, "%" PRId64 "M", 
		    seconds / MINUTE);
		seconds = seconds % MINUTE;
	}

	if (len - i < 4 || seconds == 0)
		return buf;

	(void)snprintf(buf + i, len - i, "%" PRId64 "S", seconds);
	return buf;
}

/*
 * convert a string of the form xWyDzHtMuS, or plain decimal, to 
 * a time in seconds
 */
int
timeprd(const char *str, time_t *valp)
{
	char buf[20];
	char *cur, *next, *end;
	time_t val= 0;

	strncpy(buf, str, sizeof(buf));
	next = buf;
	cur = strsep(&next, "Ww");
	if (next != NULL) {
		val = strtoumax(cur, &end, 10) * WEEK;	
		if (end[0] != '\0')
			return EINVAL;
	} else
		next = cur;
	cur = strsep(&next, "Dd");
	if (next != NULL) {
		val += strtoumax(cur, &end, 10) * DAY;	
		if (end[0] != '\0')
			return EINVAL;
	} else
		next = cur;
	cur = strsep(&next, "Hh");
	if (next != NULL) {
		val += strtoumax(cur, &end, 10) * HOUR;	
		if (end[0] != '\0')
			return EINVAL;
	} else
		next = cur;
	cur = strsep(&next, "Mm");
	if (next != NULL) {
		val += strtoumax(cur, &end, 10) * MINUTE;	
		if (end[0] != '\0')
			return EINVAL;
	} else
		next = cur;
	cur = strsep(&next, "Ss");
	val += strtoumax(cur, &end, 10);
	if (end[0] != '\0')
		return EINVAL;
	*valp = val;
	return 0;
}

/*
 * convert a string to a uint64 value
 */
int
intrd(char *str, uint64_t *val, u_int flags)
{
	char *last = &str[strlen(str) - 1];
	int ret;

	if (*last >= '0' && *last <= '9') {
		/* no unit provided, use default */
		errno = 0;
		*val = strtoumax(str, NULL, 10);
		if (flags & HN_B) {
			/* in kb, convert to disk blocks */
			*val = btodb(*val * 1024);
		}
		
		return errno;
	}
	if (strcmp(str, "-") == 0 || strcmp(str, "unlimited") == 0) {
		*val = UQUAD_MAX;
		return 0;
	}
	if (flags & HN_B) {
		if (*last == 'B' || *last == 'b')
			*last = '\0';
	}
	ret = dehumanize_number(str, (int64_t *)val);
	if (flags & HN_B)
		*val = btodb(*val);
	return ret;
}

int
alldigits(const char *s)
{
	unsigned char c;

	c = *s++;
	do {
		if (!isdigit(c))
			return 0;
	} while ((c = *s++) != 0);
	return 1;
}
