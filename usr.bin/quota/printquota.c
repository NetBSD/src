/*	$NetBSD: printquota.c,v 1.1.2.1 2011/01/21 16:58:06 bouyer Exp $	*/

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
__RCSID("$NetBSD: printquota.c,v 1.1.2.1 2011/01/21 16:58:06 bouyer Exp $");
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

#include <printquota.h>

/*
 * convert 64bit value to a printable string
 */
const char *
intprt(uint64_t val, int flags, int hflag)
{
	static char buf[21];

	if (val == UQUAD_MAX)
		return("-");

	if (flags & HN_B)
		val = dbtob(val);
	
	if (hflag) {
		humanize_number(buf, 6, val, "", HN_AUTOSCALE, flags);
		return buf;
	}
	if (flags & HN_B) {
		/* traditionnal display: blocks are in kilobytes */
		val = val / 1024;
	}
	snprintf(buf, sizeof(buf), "%" PRIu64, val);
	return buf;
}

/*
 * Calculate the grace period and return a printable string for it.
 */
const char *
timeprt(time_t seconds)
{
	time_t hours, minutes;
	static char buf[20];
	static time_t now;

	if (now == 0)
		time(&now);
	if (now > seconds)
		return ("none");
	seconds -= now;
	minutes = (seconds + 30) / 60;
	hours = (minutes + 30) / 60;
	if (hours >= 36) {
		(void)snprintf(buf, sizeof buf, "%ddays",
		    (int)((hours + 12) / 24));
		return (buf);
	}
	if (minutes >= 60) {
		(void)snprintf(buf, sizeof buf, "%2d:%d",
		    (int)(minutes / 60), (int)(minutes % 60));
		return (buf);
	}
	(void)snprintf(buf, sizeof buf, "%2d", (int)minutes);
	return (buf);
}
