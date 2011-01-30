/*	$NetBSD: printquota.c,v 1.1.2.5 2011/01/30 20:54:22 bouyer Exp $ */

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
__RCSID("$NetBSD: printquota.c,v 1.1.2.5 2011/01/30 20:54:22 bouyer Exp $");
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

#include <printquota.h>

/*
 * convert 64bit value to a printable string
 */
const char *
intprt(uint64_t val, u_int flags, int hflag, int space)
{
#define NBUFS	3
	static char bufs[NBUFS][21];
	char *buf;
	static int i = 0;

	buf = bufs[i++];
	if (i == NBUFS)
		i = 0;
#undef NBUFS
	if (val == UQUAD_MAX)
		return ((u_int)space > strlen("unlimited")) ? "unlimited" : "-";

	if (flags & HN_B)
		val = dbtob(val);
	
	if (hflag) {
		humanize_number(buf, space + 1, val, "", HN_AUTOSCALE, flags);
		return buf;
	}
	if (flags & HN_B) {
		/* traditionnal display: blocks are in kilobytes */
		val = val / 1024;
	}
	snprintf(buf, space + 1, "%" PRIu64, val);
	return buf;
}

/*
 * Calculate the grace period and return a printable string for it.
 */
const char *
timeprt(time_t now, time_t seconds, int space)
{
#define MINUTE	60
#define HOUR	(MINUTE * 60)
#define DAY	(HOUR * 24)
#define WEEK	(DAY * 7)

	static char buf[20], *append;
	int i, remain = space + 1;

	if (now > seconds)
		return ("none");
	seconds -= now;

	append = &buf[0];
	if ((seconds / WEEK) > 0) {
		i = snprintf(append, remain, "%" PRId64 "W", (seconds / WEEK));
		append += i;
		remain -=i;
		seconds = seconds % WEEK;
	}
	if (remain < 3 || seconds == 0)
		return (buf);
	if ((seconds / DAY) > 0) {
		i = snprintf(append, remain, "%" PRId64 "D", (seconds / DAY));
		append += i;
		remain -=i;
		seconds = seconds % DAY;
	}
	if (remain < 4 || seconds == 0)
		return (buf);
	if ((seconds / HOUR) > 0) {
		i = snprintf(append, remain, "%" PRId64 "H", (seconds / HOUR));
		append += i;
		remain -=i;
		seconds = seconds % HOUR;
	}
	if (remain < 4 || seconds == 0)
		return (buf);
	if ((seconds / MINUTE) > 0) {
		i = snprintf(append, remain, "%" PRId64 "M",
		    (seconds / MINUTE));
		append += i;
		remain -=i;
		seconds = seconds % MINUTE;
	}
	if (remain < 4 || seconds == 0)
		return (buf);
	i = snprintf(append, remain, "%" PRId64 "S", seconds);
	return (buf);
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
