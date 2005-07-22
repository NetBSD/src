/* $NetBSD: date.c,v 1.42 2005/07/22 14:27:08 peter Exp $ */

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
__COPYRIGHT(
"@(#) Copyright (c) 1985, 1987, 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)date.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: date.c,v 1.42 2005/07/22 14:27:08 peter Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>
#include <util.h>

#include "extern.h"

static time_t tval;
static int aflag, rflag, nflag;
int retval;

static void badformat(void);
static void badtime(void);
static void setthetime(const char *);
static void usage(void);

int
main(int argc, char *argv[])
{
	char buf[1024];
	const char *format;
	int ch;

	setprogname(argv[0]);
	(void)setlocale(LC_ALL, "");

	while ((ch = getopt(argc, argv, "anr:u")) != -1) {
		switch (ch) {
		case 'a':		/* adjust time slowly */
			aflag = 1;
			nflag = 1;
			break;
		case 'n':		/* don't set network */
			nflag = 1;
			break;
		case 'r':		/* user specified seconds */
			rflag = 1;
			tval = strtol(optarg, NULL, 0);
			break;
		case 'u':		/* do everything in UTC */
			(void)putenv("TZ=UTC0");
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (!rflag && time(&tval) == -1)
		err(EXIT_FAILURE, "time");

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

static void
badformat(void)
{
	warnx("illegal time format");
	usage();
}

static void
badtime(void)
{
	errx(EXIT_FAILURE, "illegal time");
	/* NOTREACHED */
}

#define ATOI2(s) ((s) += 2, ((s)[-2] - '0') * 10 + ((s)[-1] - '0'))

static void
setthetime(const char *p)
{
	struct timeval tv;
	time_t new_time;
	struct tm *lt;
	const char *dot, *t;
	int len, yearset;

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

	lt->tm_isdst = -1;			/* Divine correct DST */

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
	case 0:					/* was just .sss */
		if (len != 0)
			break;
		/* FALLTHROUGH */
	default:
		badformat();
	}

	/* convert broken-down time to UTC clock time */
	if ((new_time = mktime(lt)) == -1)
		badtime();

	/* set the time */
	if (nflag || netsettime(new_time)) {
		logwtmp("|", "date", "");
		if (aflag) {
			tv.tv_sec = new_time - tval;
			tv.tv_usec = 0;
			if (adjtime(&tv, NULL))
				err(EXIT_FAILURE, "date: adjtime");
		} else {
			tval = new_time;
			tv.tv_sec = tval;
			tv.tv_usec = 0;
			if (settimeofday(&tv, NULL))
				err(EXIT_FAILURE, "date: settimeofday");
		}
		logwtmp("{", "date", "");
	}

	if ((p = getlogin()) == NULL)
		p = "???";
	syslog(LOG_AUTH | LOG_NOTICE, "date set by %s", p);
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: %s [-u] [-r seconds] [+format]\n", getprogname());
	(void)fprintf(stderr, "       %s [-anu] [[[[[cc]yy]mm]dd]hh]mm[.ss]\n",
	    getprogname());
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}
