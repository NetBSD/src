/* $NetBSD: date.c,v 1.63.2.2 2024/10/14 17:44:57 martin Exp $ */

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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT(
"@(#) Copyright (c) 1985, 1987, 1988, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)date.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: date.c,v 1.63.2.2 2024/10/14 17:44:57 martin Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>
#include <util.h>
#if !HAVE_NBTOOL_CONFIG_H
#include <utmpx.h>
#endif

#include "extern.h"

static time_t tval;
static int Rflag, aflag, jflag, rflag, nflag;

__dead static void badcanotime(const char *, const char *, size_t);
static void setthetime(const char *);
__dead static void usage(void);

#if HAVE_NBTOOL_CONFIG_H
static int parse_iso_datetime(time_t *, const char *);
#else
static char *fmt;
#endif

#if !defined(isleap)
# define isleap(y)   (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))
#endif

int
main(int argc, char *argv[])
{
	char *buf;
	size_t bufsiz;
	const char *format;
	int ch;
	long long val;
	struct tm *tm;

	setprogname(argv[0]);
	(void)setlocale(LC_ALL, "");

	while ((ch = getopt(argc, argv, "ad:f:jnRr:u")) != -1) {
		switch (ch) {
		case 'a':		/* adjust time slowly */
			aflag = 1;
			nflag = 1;
			break;
		case 'd':
			rflag = 1;
#ifdef HAVE_NBTOOL_CONFIG_H
			if (parse_iso_datetime(&tval, optarg))
				break;
			errx(EXIT_FAILURE,
			    "-d only supports ISO format in the tool version");
			break;
#else
			errno = 0;
			tval = parsedate(optarg, NULL, NULL);
			if (tval == -1 && errno != 0) {
				errx(EXIT_FAILURE,
				    "%s: Unrecognized date format", optarg);
			}
			break;
		case 'f':
			fmt = optarg;
			break;
#endif
		case 'j':		/* don't set time */
			jflag = 1;
			break;
		case 'n':		/* don't set network */
			nflag = 1;
			break;
		case 'R':		/* RFC-5322 email format */
			Rflag = 1;
			break;
		case 'r':		/* user specified seconds */
			if (optarg[0] == '\0') {
				errx(EXIT_FAILURE, "<empty>: Invalid number");
			}
			errno = 0;
			val = strtoll(optarg, &buf, 0);
			if (errno) {
				err(EXIT_FAILURE, "%s", optarg);
			}
			if (optarg[0] == '\0' || *buf != '\0') {
				errx(EXIT_FAILURE,
				    "%s: Invalid number", optarg);
			}
			rflag = 1;
			tval = (time_t)val;
			break;
		case 'u':		/* do everything in UTC */
			(void)setenv("TZ", "UTC0", 1);
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (!rflag && time(&tval) == -1)
		err(EXIT_FAILURE, "time");


	/* allow the operands in any order */
	if (*argv && **argv == '+') {
		format = *argv;
		++argv;
	} else if (Rflag) {
		(void)setlocale(LC_TIME, "C");
		format = "+%a, %-e %b %Y %H:%M:%S %z";
	} else
		format = "+%a %b %e %H:%M:%S %Z %Y";

	if (*argv) {
		setthetime(*argv);
		++argv;
#ifndef HAVE_NBTOOL_CONFIG_H
	} else if (fmt) {
		usage();
#endif
	}

	if (*argv && **argv == '+')
		format = *argv;

	if ((buf = malloc(bufsiz = 1024)) == NULL)
		goto bad;

	if ((tm = localtime(&tval)) == NULL)
		err(EXIT_FAILURE, "%lld: localtime", (long long)tval);

	while (strftime(buf, bufsiz, format, tm) == 0)
		if ((buf = realloc(buf, bufsiz <<= 1)) == NULL)
			goto bad;

	(void)printf("%s\n", buf + 1);
	free(buf);
	return 0;
bad:
	err(EXIT_FAILURE, "Cannot allocate format buffer");
}

static void
badcanotime(const char *msg, const char *val, size_t where)
{
	warnx("%s in canonical time", msg);
	warnx("%s", val);
	warnx("%*s", (int)where + 1, "^");
	usage();
}

#define ATOI2(s) ((s) += 2, ((s)[-2] - '0') * 10 + ((s)[-1] - '0'))

#if HAVE_NBTOOL_CONFIG_H

inline static int
digitstring(const char *s, int len)
{
	while (--len > 0) {
		if (!isdigit(*(unsigned char *)s))
			return 0;
		s++;
	}
	return 1;
}

static int
parse_iso_datetime(time_t * res, const char * string)
{
	struct tm tm;
	time_t t;

	memset(&tm, 0, sizeof tm);

	if (!digitstring(string, 4))
		return 0;
	tm.tm_year = ATOI2(string) * 100;
	tm.tm_year += ATOI2(string);
	tm.tm_year -= 1900;

	if (*string == '-')
		string++;

	if (!digitstring(string, 2))
		return 0;

	tm.tm_mon = ATOI2(string);
	if (tm.tm_mon < 1 || tm.tm_mon > 12)
		return 0;
	tm.tm_mon--;

	if (*string == '-')
		string++;

	if (!digitstring(string, 2))
		return 0;

	tm.tm_mday = ATOI2(string);
	if (tm.tm_mday < 1)
		return 0;
	switch (tm.tm_mon) {
	case 0: case 2: case 4: case 6: case 7: case 9: case 11:
		if (tm.tm_mday > 31)
			return 0;
		break;
	case 3: case 5: case 8: case 10:
		if (tm.tm_mday > 30)
			return 0;
		break;
	case 1:
		if (tm.tm_mday > 28 + isleap(tm.tm_year + 1900))
			return 0;
		break;
	default:
		abort();
	}

	do {
		if (*string == '\0')
			break;
		if (*string == 'T' || *string == 't' || *string == ' ' ||
		    *string == '-')
			string++;

		if (!digitstring(string, 2))
			return 0;
		tm.tm_hour = ATOI2(string);
		if (tm.tm_hour > 23)
			return 0;

		if (*string == '\0')
			break;
		if (*string == ':')
			string++;

		if (!digitstring(string, 2))
			return 0;
		tm.tm_min = ATOI2(string);
		if (tm.tm_min >= 60)
			return 0;

		if (*string == '\0')
			break;
		if (*string == ':')
			string++;

		if (!digitstring(string, 2))
			return 0;
		tm.tm_sec = ATOI2(string);
		if (tm.tm_sec >= 60)
			return 0;
	} while (0);

	if (*string != '\0')
		return 0;

	tm.tm_isdst = -1;
	tm.tm_wday = -1;

	t = mktime(&tm);
	if (tm.tm_wday == -1)
		return 0;

	*res = t;
	return 1;
}

#endif	/*NBTOOL*/

static void
setthetime(const char *p)
{
	struct timeval tv;
	time_t new_time;
	struct tm *lt;
	const char *dot, *t, *op;
	size_t len;
	int yearset;

	if ((lt = localtime(&tval)) == NULL)
		err(EXIT_FAILURE, "%lld: localtime", (long long)tval);

	lt->tm_isdst = -1;			/* Divine correct DST */

#ifndef HAVE_NBTOOL_CONFIG_H
	if (fmt) {
		t = strptime(p, fmt, lt);
		if (t == NULL) {
			warnx("Failed conversion of ``%s''"
			    " using format ``%s''\n", p, fmt);
		} else if (*t != '\0')
			warnx("Ignoring %zu extraneous"
				" characters in date string (%s)",
				strlen(t), t);
		goto setit;
	}
#endif
	for (t = p, dot = NULL; *t; ++t) {
		if (*t == '.') {
			if (dot == NULL) {
				dot = t;
			} else {
				badcanotime("Unexpected dot", p, t - p);
			}
		} else if (!isdigit((unsigned char)*t)) {
			badcanotime("Expected digit", p, t - p);
		}
	}


	if (dot != NULL) {			/* .ss */
		len = strlen(dot);
		if (len > 3) {
			badcanotime("Unexpected digit after seconds field",
				    p, strlen(p) - 1);
		} else if (len < 3) {
			badcanotime("Expected digit in seconds field",
				    p, strlen(p));
		}
		++dot;
		lt->tm_sec = ATOI2(dot);
		if (lt->tm_sec > 61)
			badcanotime("Seconds out of range", p, strlen(p) - 1);
	} else {
		len = 0;
		lt->tm_sec = 0;
	}

	op = p;
	yearset = 0;
	switch (strlen(p) - len) {
	case 12:				/* cc */
		lt->tm_year = ATOI2(p) * 100 - TM_YEAR_BASE;
		if (lt->tm_year < 0)
			badcanotime("Year before 1900", op, p - op + 1);
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
		if (lt->tm_mon > 12 || lt->tm_mon == 0)
			badcanotime("Month out of range", op, p - op - 1);
		--lt->tm_mon;			/* time struct is 0 - 11 */
		/* FALLTHROUGH */
	case 6:					/* dd */
		lt->tm_mday = ATOI2(p);
		switch (lt->tm_mon) {
		case 0:
		case 2:
		case 4:
		case 6:
		case 7:
		case 9:
		case 11:
			if (lt->tm_mday > 31 || lt->tm_mday == 0)
				badcanotime("Day out of range (max 31)",
					    op, p - op - 1);
			break;
		case 3:
		case 5:
		case 8:
		case 10:
			if (lt->tm_mday > 30 || lt->tm_mday == 0)
				badcanotime("Day out of range (max 30)",
					    op, p - op - 1);
			break;
		case 1:
			if (isleap(lt->tm_year + TM_YEAR_BASE)) {
				if (lt->tm_mday > 29 || lt->tm_mday == 0) {
					badcanotime("Day out of range "
						    "(max 29)",
						    op, p - op - 1);
				}
			} else {
				if (lt->tm_mday > 28 || lt->tm_mday == 0) {
					badcanotime("Day out of range "
						    "(max 28)",
						    op, p - op - 1);
				}
			}
			break;
		default:
			/*
			 * If the month was given, it's already been
			 * checked.  If a bad value came back from
			 * localtime, something's badly broken.
			 * (make this an assertion?)
			 */
			errx(EXIT_FAILURE, "localtime gave invalid month %d",
			    lt->tm_mon);
		}
		/* FALLTHROUGH */
	case 4:					/* hh */
		lt->tm_hour = ATOI2(p);
		if (lt->tm_hour > 23)
			badcanotime("Hour out of range", op, p - op - 1);
		/* FALLTHROUGH */
	case 2:					/* mm */
		lt->tm_min = ATOI2(p);
		if (lt->tm_min > 59)
			badcanotime("Minute out of range", op, p - op - 1);
		break;
	case 0:					/* was just .sss */
		if (len != 0)
			break;
		/* FALLTHROUGH */
	default:
	    if (strlen(p) - len > 12) {
		    badcanotime("Too many digits", p, 12);
	    } else {
		    badcanotime("Not enough digits", p, strlen(p) - len);
	    }
	}
setit:
	/* convert broken-down time to UTC clock time */
	if ((new_time = mktime(lt)) == -1) {
		/* Can this actually happen? */
		err(EXIT_FAILURE, "mktime");
	}

	/* if jflag is set, don't actually change the time, just return */
	if (jflag) {
		tval = new_time;
		return;
	}

	/* set the time */
#ifndef HAVE_NBTOOL_CONFIG_H
	struct utmpx utx;
	memset(&utx, 0, sizeof(utx));
	utx.ut_type = OLD_TIME;
	(void)gettimeofday(&utx.ut_tv, NULL);
	pututxline(&utx);

	if (nflag || netsettime(new_time)) {
		logwtmp("|", "date", "");
		if (aflag) {
			tv.tv_sec = new_time - tval;
			tv.tv_usec = 0;
			if (adjtime(&tv, NULL))
				err(EXIT_FAILURE, "adjtime");
		} else {
			tval = new_time;
			tv.tv_sec = tval;
			tv.tv_usec = 0;
			if (settimeofday(&tv, NULL))
				err(EXIT_FAILURE, "settimeofday");
		}
		logwtmp("{", "date", "");
	}
	utx.ut_type = NEW_TIME;
	(void)gettimeofday(&utx.ut_tv, NULL);
	pututxline(&utx);

	if ((p = getlogin()) == NULL)
		p = "???";
	syslog(LOG_AUTH | LOG_NOTICE, "date set by %s", p);
#else
	errx(EXIT_FAILURE, "Can't set the time in the tools version");
#endif
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "Usage: %s [-ajnRu] [-d date] [-r seconds] [+format]",
	    getprogname());
	(void)fprintf(stderr, " [[[[[[CC]yy]mm]dd]HH]MM[.SS]]\n");
	(void)fprintf(stderr,
	    "       %s [-ajnRu] -f input_format new_date [+format]\n",
	    getprogname());
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}
