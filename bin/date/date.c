/*	$NetBSD: date.c,v 1.29 2000/01/04 14:15:09 kleink Exp $	*/

/*
 * Copyright (c) 1985, 1987, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT(
"@(#) Copyright (c) 1985, 1987, 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)date.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: date.c,v 1.29 2000/01/04 14:15:09 kleink Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <syslog.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>
#include <util.h>

#include "extern.h"

time_t tval;
int retval, nflag;

int main __P((int, char *[]));
static void setthetime __P((const char *));
static void badformat __P((void));
static void badtime __P((void));
static void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	extern char *optarg;
	int ch, rflag;
	char *format, buf[1024];

	(void)setlocale(LC_ALL, "");

	rflag = 0;
	while ((ch = getopt(argc, argv, "nr:u")) != -1)
		switch((char)ch) {
		case 'n':		/* don't set network */
			nflag = 1;
			break;
		case 'r':		/* user specified seconds */
			rflag = 1;
			tval = atol(optarg);
			break;
		case 'u':		/* do everything in UTC */
			(void)putenv("TZ=UTC0");
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (!rflag && time(&tval) == -1)
		err(1, "time");

	format = "%a %b %e %H:%M:%S %Z %Y";

	/* allow the operands in any order */
	if (*argv && **argv == '+') {
		format = *argv + 1;
		++argv;
	}

	if (*argv) {
		setthetime(*argv);
		++argv;
	}

	if (*argv && **argv == '+')
		format = *argv + 1;

	(void)strftime(buf, sizeof(buf), format, localtime(&tval));
	(void)printf("%s\n", buf);
	exit(retval);
	/* NOTREACHED */
}

#define	ATOI2(s)	((s) += 2, ((s)[-2] - '0') * 10 + ((s)[-1] - '0'))

void
setthetime(p)
	const char *p;
{
	struct tm *lt;
	struct timeval tv;
	const char *dot, *t;
	int yearset, len;

	for (t = p, dot = NULL; *t; ++t) {
		if (isdigit((unsigned char)*t))
			continue;
		if (*t == '.' && dot == NULL) {
			dot = t;
			continue;
		}
		badformat();
	}

	lt = localtime(&tval);

	if (dot != NULL) {			/* .ss */
		len = strlen(dot);
		if (len != 3)
			badformat();
		++dot;
		lt->tm_sec = ATOI2(dot);
	} else {
		len = 0;
		lt->tm_sec = 0;
	}

	yearset = 0;
	switch (strlen(p) - len) {
	case 12:				/* cc */
		lt->tm_year = ATOI2(p) * 100 - TM_YEAR_BASE;
		yearset = 1;
		/* FALLTHROUGH */
	case 10:				/* yy */
		if (yearset) {
			lt->tm_year += ATOI2(p);
		} else {
			yearset = ATOI2(p);
			if (yearset < 69)
				lt->tm_year = yearset + 2000 - TM_YEAR_BASE;
			else
				lt->tm_year = yearset + 1900 - TM_YEAR_BASE;
		}
		/* FALLTHROUGH */
	case 8:					/* mm */
		lt->tm_mon = ATOI2(p);
		--lt->tm_mon;			/* time struct is 0 - 11 */
		/* FALLTHROUGH */
	case 6:					/* dd */
		lt->tm_mday = ATOI2(p);
		/* FALLTHROUGH */
	case 4:					/* hh */
		lt->tm_hour = ATOI2(p);
		/* FALLTHROUGH */
	case 2:					/* mm */
		lt->tm_min = ATOI2(p);
		break;
	default:
		badformat();
	}

	/* convert broken-down time to UTC clock time */
	if ((tval = mktime(lt)) == -1)
		badtime();

	/* set the time */
	if (nflag || netsettime(tval)) {
		logwtmp("|", "date", "");
		tv.tv_sec = tval;
		tv.tv_usec = 0;
		if (settimeofday(&tv, NULL)) {
			perror("date: settimeofday");
			exit(1);
		}
		logwtmp("{", "date", "");
	}

	if ((p = getlogin()) == NULL)
		p = "???";
	syslog(LOG_AUTH | LOG_NOTICE, "date set by %s", p);
}

static void
badformat()
{
	warnx("illegal time format");
	usage();
}

static void
badtime()
{
	errx(1, "illegal time");
}

static void
usage()
{
	(void)fprintf(stderr,
	    "usage: date [-nu] [-r seconds] [+format]\n");
	(void)fprintf(stderr, "       date [[[[[cc]yy]mm]dd]hh]mm[.ss]\n");
	exit(1);
	/* NOTREACHED */
}
