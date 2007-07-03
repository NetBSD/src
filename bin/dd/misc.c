/*	$NetBSD: misc.c,v 1.20 2007/07/03 05:25:25 lukem Exp $	*/

/*-
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Keith Muller of the University of California, San Diego and Lance
 * Visser of Convex Computer Corporation.
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
#if 0
static char sccsid[] = "@(#)misc.c	8.3 (Berkeley) 4/2/94";
#else
__RCSID("$NetBSD: misc.c,v 1.20 2007/07/03 05:25:25 lukem Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include "dd.h"
#include "extern.h"

#define	tv2mS(tv) ((tv).tv_sec * 1000LL + ((tv).tv_usec + 500) / 1000)

void
summary(void)
{
	char buf[100];
	int64_t mS;
	struct timeval tv;

	if (progress)
		(void)write(STDERR_FILENO, "\n", 1);

	(void)gettimeofday(&tv, NULL);
	mS = tv2mS(tv) - tv2mS(st.start);
	if (mS == 0)
		mS = 1;
	/* Use snprintf(3) so that we don't reenter stdio(3). */
	(void)snprintf(buf, sizeof(buf),
	    "%llu+%llu records in\n%llu+%llu records out\n",
	    (unsigned long long)st.in_full,  (unsigned long long)st.in_part,
	    (unsigned long long)st.out_full, (unsigned long long)st.out_part);
	(void)write(STDERR_FILENO, buf, strlen(buf));
	if (st.swab) {
		(void)snprintf(buf, sizeof(buf), "%llu odd length swab %s\n",
		    (unsigned long long)st.swab,
		    (st.swab == 1) ? "block" : "blocks");
		(void)write(STDERR_FILENO, buf, strlen(buf));
	}
	if (st.trunc) {
		(void)snprintf(buf, sizeof(buf), "%llu truncated %s\n",
		    (unsigned long long)st.trunc,
		    (st.trunc == 1) ? "block" : "blocks");
		(void)write(STDERR_FILENO, buf, strlen(buf));
	}
	if (st.sparse) {
		(void)snprintf(buf, sizeof(buf), "%llu sparse output %s\n",
		    (unsigned long long)st.sparse,
		    (st.sparse == 1) ? "block" : "blocks");
		(void)write(STDERR_FILENO, buf, strlen(buf));
	}
	(void)snprintf(buf, sizeof(buf),
	    "%llu bytes transferred in %lu.%03d secs (%llu bytes/sec)\n",
	    (unsigned long long) st.bytes,
	    (long) (mS / 1000),
	    (int) (mS % 1000),
	    (unsigned long long) (st.bytes * 1000LL / mS));
	(void)write(STDERR_FILENO, buf, strlen(buf));
}

/* ARGSUSED */
void
summaryx(int notused)
{

	summary();
}

/* ARGSUSED */
void
terminate(int signo)
{

	summary();
	(void)signal(signo, SIG_DFL);
	raise(signo);
	/* NOTREACHED */
}
