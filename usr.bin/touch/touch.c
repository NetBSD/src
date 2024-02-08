/*	$NetBSD: touch.c,v 1.37 2024/02/08 02:53:53 kre Exp $	*/

/*
 * Copyright (c) 1993
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
__COPYRIGHT("@(#) Copyright (c) 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)touch.c	8.2 (Berkeley) 4/28/95";
#endif
__RCSID("$NetBSD: touch.c,v 1.37 2024/02/08 02:53:53 kre Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>
#include <util.h>
#include <getopt.h>

static void	stime_arg0(const char *, struct timespec *);
static void	stime_arg1(char *, struct timespec *);
static void	stime_arg2(const char *, int, struct timespec *);
static void	stime_file(const char *, struct timespec *);
static int	stime_posix(const char *, struct timespec *);
__dead static void	usage(void);

struct option touch_longopts[] = {
	{ "date",		required_argument,	0,
						'd' },
	{ "reference",		required_argument,	0,
						'r' },
	{ NULL,			0,			0,
						0 },
};

#define	YEAR_BOUNDARY		69
#define	LOW_YEAR_CENTURY	2000	/* for 2 digit years < YEAR_BOUNDARY */
#define	HIGH_YEAR_CENTURY	1900	/* for 2 digit years >=  "  */

#define	NO_TIME		((time_t)-1)	/* time_t might be unsigned */

int
main(int argc, char *argv[])
{
	struct stat sb;
	struct timespec ts[2];
	int aflag, cflag, hflag, mflag, ch, fd, len, rval, timeset;
	char *p;
	int (*change_file_times)(const char *, const struct timespec *);
	int (*get_file_status)(const char *, struct stat *);

	setlocale(LC_ALL, "");

	aflag = cflag = hflag = mflag = timeset = 0;
	if (clock_gettime(CLOCK_REALTIME, &ts[0]))
		err(1, "clock_gettime");

	while ((ch = getopt_long(argc, argv, "acd:fhmr:t:", touch_longopts,
	    NULL)) != -1)
		switch (ch) {
		case 'a':
			aflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'd':
			timeset = 1;
			if (!stime_posix(optarg, ts))
				stime_arg0(optarg, ts);
			break;
		case 'f':
			break;
		case 'h':
			hflag = 1;
			break;
		case 'm':
			mflag = 1;
			break;
		case 'r':
			timeset = 1;
			stime_file(optarg, ts);
			break;
		case 't':
			timeset = 1;
			stime_arg1(optarg, ts);
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/* Default is both -a and -m. */
	if (aflag == 0 && mflag == 0)
		aflag = mflag = 1;

	if (hflag) {
		cflag = 1;		/* Don't create new file */
		change_file_times = lutimens;
		get_file_status = lstat;
	} else {
		change_file_times = utimens;
		get_file_status = stat;
	}

	/*
	 * If no -r or -t flag, at least two operands, the first of which
	 * is an 8 or 10 digit number, use the obsolete time specification.
	 */
	if (!timeset && argc > 1) {
		(void)strtol(argv[0], &p, 10);
		len = p - argv[0];
		if (*p == '\0' && (len == 8 || len == 10)) {
			timeset = 1;
			stime_arg2(*argv++, len == 10, ts);
		}
	}

	/* Otherwise use the current time of day. */
	if (!timeset)
		ts[1] = ts[0];

	if (*argv == NULL)
		usage();

	for (rval = EXIT_SUCCESS; *argv; ++argv) {
		/* See if the file exists. */
		if ((*get_file_status)(*argv, &sb)) {
			if (!cflag) {
				/* Create the file. */
				fd = open(*argv,
				    O_WRONLY | O_CREAT, DEFFILEMODE);
				if (fd == -1 || fstat(fd, &sb) || close(fd)) {
					rval = EXIT_FAILURE;
					warn("%s", *argv);
					continue;
				}

				/* If using the current time, we're done. */
				if (!timeset)
					continue;
			} else
				continue;
		}
		if (!aflag)
			ts[0] = sb.st_atimespec;
		if (!mflag)
			ts[1] = sb.st_mtimespec;

		/* Try utimes(2). */
		if (!(*change_file_times)(*argv, ts))
			continue;

		/* If the user specified a time, nothing else we can do. */
		if (timeset) {
			rval = EXIT_FAILURE;
			warn("%s", *argv);
		}

		/*
		 * System V and POSIX 1003.1 require that a NULL argument
		 * set the access/modification times to the current time.
		 * The permission checks are different, too, in that the
		 * ability to write the file is sufficient.  Take a shot.
		 */
		 if (!(*change_file_times)(*argv, NULL))
			continue;

		rval = EXIT_FAILURE;
		warn("%s", *argv);
	}
	exit(rval);
}

#define	ATOI2(s)	((s) += 2, ((s)[-2] - '0') * 10 + ((s)[-1] - '0'))

static void
stime_arg0(const char *arg, struct timespec *tsp)
{
	tsp[1].tv_sec = tsp[0].tv_sec = parsedate(arg, NULL, NULL);
	if (tsp[0].tv_sec == NO_TIME)
		errx(EXIT_FAILURE, "Could not parse `%s'", arg);
	tsp[0].tv_nsec = tsp[1].tv_nsec = 0;
}

static void
stime_arg1(char *arg, struct timespec *tsp)
{
	struct tm *t;
	time_t tmptime;
	int yearset;
	char *p;
	char *initarg = arg;

					/* Start with the current time. */
	tmptime = tsp[0].tv_sec;
	if ((t = localtime(&tmptime)) == NULL)
		err(EXIT_FAILURE, "localtime");
					/* [[CC]YY]MMDDhhmm[.ss] */
	if ((p = strchr(arg, '.')) == NULL)
		t->tm_sec = 0;		/* Seconds defaults to 0. */
	else {
		if (strlen(p + 1) != 2)
			goto terr;
		*p++ = '\0';
		t->tm_sec = ATOI2(p);
	}
		
	yearset = 0;
	switch (strlen(arg)) {
	case 12:			/* CCYYMMDDhhmm */
		t->tm_year = ATOI2(arg) * 100 - TM_YEAR_BASE;
		yearset = 1;
		/* FALLTHROUGH */
	case 10:			/* YYMMDDhhmm */
		if (yearset) {
			t->tm_year += ATOI2(arg);
		} else {
			yearset = ATOI2(arg);
			if (yearset < YEAR_BOUNDARY)
				t->tm_year = yearset +
				    LOW_YEAR_CENTURY - TM_YEAR_BASE;
			else
				t->tm_year = yearset +
				    HIGH_YEAR_CENTURY - TM_YEAR_BASE;
		}
		/* FALLTHROUGH */
	case 8:				/* MMDDhhmm */
		t->tm_mon = ATOI2(arg);
		--t->tm_mon;		/* Convert from 01-12 to 00-11 */
		/* FALLTHROUGH */
	case 6:
		t->tm_mday = ATOI2(arg);
		/* FALLTHROUGH */
	case 4:
		t->tm_hour = ATOI2(arg);
		/* FALLTHROUGH */
	case 2:
		t->tm_min = ATOI2(arg);
		break;
	default:
		goto terr;
	}

	t->tm_isdst = -1;		/* Figure out DST. */
	tsp[0].tv_sec = tsp[1].tv_sec = mktime(t);
	if (tsp[0].tv_sec == NO_TIME)
 terr:		errx(EXIT_FAILURE, "out of range or bad time specification:\n"
		    "\t'%s' should be [[CC]YY]MMDDhhmm[.ss]", initarg);

	tsp[0].tv_nsec = tsp[1].tv_nsec = 0;
}

static void
stime_arg2(const char *arg, int year, struct timespec *tsp)
{
	struct tm *t;
	time_t tmptime;
					/* Start with the current time. */
	tmptime = tsp[0].tv_sec;
	if ((t = localtime(&tmptime)) == NULL)
		err(EXIT_FAILURE, "localtime");

	t->tm_mon = ATOI2(arg);		/* MMDDhhmm[yy] */
	--t->tm_mon;			/* Convert from 01-12 to 00-11 */
	t->tm_mday = ATOI2(arg);
	t->tm_hour = ATOI2(arg);
	t->tm_min = ATOI2(arg);
	if (year) {
		year = ATOI2(arg);
		if (year < YEAR_BOUNDARY)
			t->tm_year = year + LOW_YEAR_CENTURY - TM_YEAR_BASE;
		else
			t->tm_year = year + HIGH_YEAR_CENTURY - TM_YEAR_BASE;
	}
	t->tm_sec = 0;

	t->tm_isdst = -1;		/* Figure out DST. */
	tsp[0].tv_sec = tsp[1].tv_sec = mktime(t);
	if (tsp[0].tv_sec == NO_TIME)
		errx(EXIT_FAILURE,
		    "out of range or bad time specification: MMDDhhmm[YY]");

	tsp[0].tv_nsec = tsp[1].tv_nsec = 0;
}

static void
stime_file(const char *fname, struct timespec *tsp)
{
	struct stat sb;

	if (stat(fname, &sb))
		err(1, "%s", fname);
	tsp[0] = sb.st_atimespec;
	tsp[1] = sb.st_mtimespec;
}

static int
stime_posix(const char *arg, struct timespec *tsp)
{
	struct tm tm;
	const char *p;
	char *ep;
	int utc = 0;
	long val;

#define	isdigch(c)	(isdigit((int)((unsigned char)(c))))

	if ((p = strchr(arg, '-')) == NULL)
		return 0;
	if (p - arg < 4)	/* at least 4 year digits required */
		return 0;

	if (!isdigch(arg[0]))	/* and the first must be a digit! */
		return 0;

	errno = 0;
	val = strtol(arg, &ep, 10);		/* YYYY */
	if (val < 0 || val > INT_MAX)
		return 0;
	if (*ep != '-')
		return 0;
	tm.tm_year = (int)val - 1900;

	p = ep + 1;

	if (!isdigch(*p))
		return 0;
	val = strtol(p, &ep, 10);		/* MM */
	if (val < 1 || val > 12)
		return 0;
	if (*ep != '-' || ep != p + 2)
		return 0;
	tm.tm_mon = (int)val - 1;

	p = ep + 1;

	if (!isdigch(*p))
		return 0;
	val = strtol(p, &ep, 10);		/* DD */
	if (val < 1 || val > 31)
		return 0;
	if ((*ep != 'T' && *ep != ' ') || ep != p + 2)
		return 0;
	tm.tm_mday = (int)val;

	p = ep + 1;

	if (!isdigch(*p))
		return 0;
	val = strtol(p, &ep, 10);		/* hh */
	if (val < 0 || val > 23)
		return 0;
	if (*ep != ':' || ep != p + 2)
		return 0;
	tm.tm_hour = (int)val;

	p = ep + 1;

	if (!isdigch(*p))
		return 0;
	val = strtol(p, &ep, 10);		/* mm */
	if (val < 0 || val > 59)
		return 0;
	if (*ep != ':' || ep != p + 2)
		return 0;
	tm.tm_min = (int)val;

	p = ep + 1;

	if (!isdigch(*p))
		return 0;
	val = strtol(p, &ep, 10);		/* ss (or in POSIX, SS) */
	if (val < 0 || val > 60)
		return 0;
	if ((*ep != '.' && *ep != ',' && *ep != 'Z' && *ep != '\0') ||
	      ep != p + 2)
		return 0;
	tm.tm_sec = (int)val;

	if (*ep == ',' || *ep == '.') {
		double frac;
		ptrdiff_t fdigs;

		p = ep + 1;
		if (!isdigch(*p))
			return 0;
		val = strtol(p, &ep, 10);
		if (val < 0)
			return 0;
		if (ep == p)	/* at least 1 digit required */
			return 0;
		if (*ep != 'Z' && *ep != '\0')
			return 0;

		if (errno != 0)
			return 0;

		fdigs = ep - p;
		if (fdigs > 15) {
			/* avoid being absurd */
			/* don't want to risk 10^fdigs being INF */
			if (val == 0)
				fdigs = 1;
			else while (fdigs > 15) {
				val = (val + 5) / 10;
				fdigs--;
			}
		}

		frac = pow(10.0, (double)fdigs);

		tsp[0].tv_nsec = tsp[1].tv_nsec =
			(long)round(((double)val / frac) * 1000000000.0);
	} else
		tsp[0].tv_nsec = tsp[1].tv_nsec = 0;

	if (*ep == 'Z') {
		if (ep[1] != '\0')
			return 0;
		utc = 1;
	}

	if (errno != 0)
		return 0;

	tm.tm_isdst = -1;
	if (utc)
		tsp[0].tv_sec = tsp[1].tv_sec = timegm(&tm);
	else
		tsp[0].tv_sec = tsp[1].tv_sec = mktime(&tm);

	if (errno != 0 && tsp[1].tv_sec == NO_TIME)
		return 0;

	return 1;
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "Usage: %s [-acfhm] [-d|--date datetime] [-r|--reference file]"
	    " [-t time] file ...\n", getprogname());
	exit(EXIT_FAILURE);
}
