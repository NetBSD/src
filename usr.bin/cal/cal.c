/*	$NetBSD: cal.c,v 1.15 2003/06/05 00:21:20 atatat Exp $	*/

/*
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kim Letkeman.
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
__COPYRIGHT("@(#) Copyright (c) 1989, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)cal.c	8.4 (Berkeley) 4/2/94";
#else
__RCSID("$NetBSD: cal.c,v 1.15 2003/06/05 00:21:20 atatat Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termcap.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>

#define	THURSDAY		4		/* for reformation */
#define	SATURDAY 		6		/* 1 Jan 1 was a Saturday */

#define	FIRST_MISSING_DAY 	639799		/* 3 Sep 1752 */
#define	NUMBER_MISSING_DAYS 	11		/* 11 day correction */

#define	MAXDAYS			42		/* max slots in a month array */
#define	SPACE			-1		/* used in day array */

static int days_in_month[2][13] = {
	{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

int sep1752[MAXDAYS] = {
	SPACE,	SPACE,	1,	2,	14,	15,	16,
	17,	18,	19,	20,	21,	22,	23,
	24,	25,	26,	27,	28,	29,	30,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
}, j_sep1752[MAXDAYS] = {
	SPACE,	SPACE,	245,	246,	258,	259,	260,
	261,	262,	263,	264,	265,	266,	267,
	268,	269,	270,	271,	272,	273,	274,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
}, empty[MAXDAYS] = {
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
};

char *month_names[12] = {
	"January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December",
};

char *day_headings = " S  M Tu  W Th  F  S";
char *j_day_headings = "  S   M  Tu   W  Th   F   S";

/* leap year -- account for gregorian reformation in 1752 */
#define	leap_year(yr) \
	((yr) <= 1752 ? !((yr) % 4) : \
	(!((yr) % 4) && ((yr) % 100)) || !((yr) % 400))

/* number of centuries since 1700, not inclusive */
#define	centuries_since_1700(yr) \
	((yr) > 1700 ? (yr) / 100 - 17 : 0)

/* number of centuries since 1700 whose modulo of 400 is 0 */
#define	quad_centuries_since_1700(yr) \
	((yr) > 1600 ? ((yr) - 1600) / 400 : 0)

/* number of leap years between year 1 and this year, not inclusive */
#define	leap_years_since_year_1(yr) \
	((yr) / 4 - centuries_since_1700(yr) + quad_centuries_since_1700(yr))

int julian;
int hilite;
char *md, *me;

void	init_hilite(void);
int	getnum(const char *);
int	ascii_day(char *, int);
void	center(char *, int, int);
void	day_array(int, int, int *);
int	day_in_week(int, int, int);
int	day_in_year(int, int, int);
void	monthrange(int, int, int, int, int);
int	main(int, char **);
void	trim_trailing_spaces(char *);
void	usage(void);

int
main(int argc, char **argv)
{
	struct tm *local_time;
	time_t now;
	int ch, month, year, yflag;
	int before, after;
	int yearly = 0;

	before = after = 0;
	yflag = year = 0;
	while ((ch = getopt(argc, argv, "A:B:hjy3")) != -1) {
		switch (ch) {
		case 'A':
			after = getnum(optarg);
			break;
		case 'B':
			before = getnum(optarg);
			break;
		case 'h':
			init_hilite();
			break;
		case 'j':
			julian = 1;
			break;
		case 'y':
			yflag = 1;
			break;
		case '3':
			before = after = 1;
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;

	month = 0;
	switch (argc) {
	case 2:
		if ((month = atoi(*argv++)) < 1 || month > 12)
			errx(1, "illegal month value: use 1-12");
		/* FALLTHROUGH */
	case 1:
		if ((year = atoi(*argv)) < 1 || year > 9999)
			errx(1, "illegal year value: use 1-9999");
		break;
	case 0:
		(void)time(&now);
		local_time = localtime(&now);
		year = local_time->tm_year + TM_YEAR_BASE;
		if (!yflag)
			month = local_time->tm_mon + 1;
		break;
	default:
		usage();
	}

	if (!month) {
		/* yearly */
		month = 1;
		before = 0;
		after = 11;
		yearly = 1;
	}

	monthrange(month, year, before, after, yearly);

	exit(0);
}

#define	DAY_LEN		3		/* 3 spaces per day */
#define	J_DAY_LEN	4		/* 4 spaces per day */
#define	WEEK_LEN	20		/* 7 * 3 - one space at the end */
#define	J_WEEK_LEN	27		/* 7 * 4 - one space at the end */
#define	HEAD_SEP	2		/* spaces between day headings */
#define	J_HEAD_SEP	2
#define	MONTH_PER_ROW	3		/* how many monthes in a row */
#define	J_MONTH_PER_ROW	2

void
monthrange(int month, int year, int before, int after, int yearly)
{
	int startmonth, startyear;
	int endmonth, endyear;
	int i, row;
	int days[3][MAXDAYS];
	char lineout[256];
	int inayear;
	int newyear;
	int day_len, week_len, head_sep;
	int month_per_row;
	int skip, r_off, w_off;

	if (julian) {
		day_len = J_DAY_LEN;
		week_len = J_WEEK_LEN;
		head_sep = J_HEAD_SEP;
		month_per_row = J_MONTH_PER_ROW;
	}
	else {
		day_len = DAY_LEN;
		week_len = WEEK_LEN;
		head_sep = HEAD_SEP;
		month_per_row = MONTH_PER_ROW;
	}

	month--;

	startyear = year - (before + 12 - 1 - month) / 12;
	startmonth = 12 - 1 - ((before + 12 - 1 - month) % 12);
	endyear = year + (month + after) / 12;
	endmonth = (month + after) % 12;

	if (startyear < 0 || endyear > 9999) {
		errx(1, "year should be in 1-9999\n");
	}

	year = startyear;
	month = startmonth;
	inayear = newyear = (year != endyear || yearly);
	if (inayear) {
		skip = month % month_per_row;
		month -= skip;
	}
	else {
		skip = 0;
	}

	do {
		if (newyear) {
			(void)snprintf(lineout, sizeof(lineout), "%d", year);
			center(lineout, week_len * month_per_row +
			    head_sep * (month_per_row - 1), 0);
			(void)printf("\n\n");
			newyear = 0;
		}

		for (i = 0; i < skip; i++)
			center("", week_len, head_sep);

		for (; i < month_per_row; i++) {
			int sep;

			if (year == endyear && month + i > endmonth)
				break;

			sep = (i == month_per_row - 1) ? 0 : head_sep;
			day_array(month + i + 1, year, days[i]);
			if (inayear) {
				center(month_names[month + i], week_len, sep);
			}
			else {
				snprintf(lineout, sizeof(lineout), "%s %d",
				    month_names[month + i], year);
				center(lineout, week_len, sep);
			}
		}
		printf("\n");

		for (i = 0; i < skip; i++)
			center("", week_len, head_sep);

		for (; i < month_per_row; i++) {
			int sep;

			if (year == endyear && month + i > endmonth)
				break;

			sep = (i == month_per_row - 1) ? 0 : head_sep;
			printf("%s%*s",
			    (julian) ? j_day_headings : day_headings, sep, "");
		}
		printf("\n");

		memset(lineout, ' ', sizeof(lineout));
		for (row = 0; row < 6; row++) {
			char *p;
			for (i = 0; i < skip; i++) {
				p = lineout + i * (week_len + 2) + w_off;
				memset(p, ' ', week_len);
			}
			w_off = 0;
			for (; i < month_per_row; i++) {
				int col, *dp;

				if (year == endyear && month + i > endmonth)
					break;

				p = lineout + i * (week_len + 2) + w_off;
				dp = &days[i][row * 7];
				for (col = 0; col < 7;
				     col++, p += day_len + r_off) {
					r_off = ascii_day(p, *dp++);
					w_off += r_off;
				}
			}
			*p = '\0';
			trim_trailing_spaces(lineout);
			(void)printf("%s\n", lineout);
		}

		skip = 0;
		month += month_per_row;
		if (month >= 12) {
			month -= 12;
			year++;
			newyear = 1;
		}
	} while (year < endyear || (year == endyear && month <= endmonth));
}

/*
 * day_array --
 *	Fill in an array of 42 integers with a calendar.  Assume for a moment
 *	that you took the (maximum) 6 rows in a calendar and stretched them
 *	out end to end.  You would have 42 numbers or spaces.  This routine
 *	builds that array for any month from Jan. 1 through Dec. 9999.
 */
void
day_array(int month, int year, int *days)
{
	int day, dw, dm;
	time_t t;
	struct tm *tm;

	t = time(NULL);
	tm = localtime(&t);
	tm->tm_year += TM_YEAR_BASE;
	tm->tm_mon++;
	tm->tm_yday++; /* jan 1 is 1 for us, not 0 */

	if (month == 9 && year == 1752) {
		memmove(days,
			julian ? j_sep1752 : sep1752, MAXDAYS * sizeof(int));
		return;
	}
	memmove(days, empty, MAXDAYS * sizeof(int));
	dm = days_in_month[leap_year(year)][month];
	dw = day_in_week(1, month, year);
	day = julian ? day_in_year(1, month, year) : 1;
	while (dm--) {
		if (hilite && year == tm->tm_year &&
		    (julian ? (day == tm->tm_yday) :
		     (month == tm->tm_mon && day == tm->tm_mday)))
			days[dw++] = -1 - day++;
		else
			days[dw++] = day++;
	}
}

/*
 * day_in_year --
 *	return the 1 based day number within the year
 */
int
day_in_year(int day, int month, int year)
{
	int i, leap;

	leap = leap_year(year);
	for (i = 1; i < month; i++)
		day += days_in_month[leap][i];
	return (day);
}

/*
 * day_in_week
 *	return the 0 based day number for any date from 1 Jan. 1 to
 *	31 Dec. 9999.  Assumes the Gregorian reformation eliminates
 *	3 Sep. 1752 through 13 Sep. 1752.  Returns Thursday for all
 *	missing days.
 */
int
day_in_week(int day, int month, int year)
{
	long temp;

	temp = (long)(year - 1) * 365 + leap_years_since_year_1(year - 1)
	    + day_in_year(day, month, year);
	if (temp < FIRST_MISSING_DAY)
		return ((temp - 1 + SATURDAY) % 7);
	if (temp >= (FIRST_MISSING_DAY + NUMBER_MISSING_DAYS))
		return (((temp - 1 + SATURDAY) - NUMBER_MISSING_DAYS) % 7);
	return (THURSDAY);
}

int
ascii_day(char *p, int day)
{
	int display, val, rc;
	char *b;
	static char *aday[] = {
		"",
		" 1", " 2", " 3", " 4", " 5", " 6", " 7",
		" 8", " 9", "10", "11", "12", "13", "14",
		"15", "16", "17", "18", "19", "20", "21",
		"22", "23", "24", "25", "26", "27", "28",
		"29", "30", "31",
	};

	if (day == SPACE) {
		memset(p, ' ', julian ? J_DAY_LEN : DAY_LEN);
		return (0);
	}
	if (day < 0) {
		b = p;
		day = -1 - day;
	} else
		b = NULL;
	if (julian) {
		if ((val = day / 100) != 0) {
			day %= 100;
			*p++ = val + '0';
			display = 1;
		} else {
			*p++ = ' ';
			display = 0;
		}
		val = day / 10;
		if (val || display)
			*p++ = val + '0';
		else
			*p++ = ' ';
		*p++ = day % 10 + '0';
	} else {
		*p++ = aday[day][0];
		*p++ = aday[day][1];
	}

	rc = 0;
	if (b != NULL) {
		char *t, h[64];
		int l;

		l = p - b;
		memcpy(h, b, l);
		p = b;

		if (md != NULL) {
			for (t = md; *t; rc++)
				*p++ = *t++;
			memcpy(p, h, l);
			p += l;
			for (t = me; *t; rc++)
				*p++ = *t++;
		} else {
			for (t = &h[0]; l--; t++) {
				*p++ = *t;
				rc++;
				*p++ = '\b';
				rc++;
				*p++ = *t;
			}
		}

	}

	*p = ' ';
	return (rc);
}

void
trim_trailing_spaces(char *s)
{
	char *p;

	for (p = s; *p; ++p)
		continue;
	while (p > s && isspace((unsigned char)*--p))
		continue;
	if (p > s)
		++p;
	*p = '\0';
}

void
center(char *str, int len, int separate)
{

	len -= strlen(str);
	(void)printf("%*s%s%*s", len / 2, "", str, len / 2 + len % 2, "");
	if (separate)
		(void)printf("%*s", separate, "");
}

int
getnum(const char *p)
{
	long result;
	char *ep;

	errno = 0;
	result = strtoul(p, &ep, 10);
	if (p[0] == '\0' || *ep != '\0')
		goto error;
	if (errno == ERANGE && result == ULONG_MAX)
		goto error;
	if (result > INT_MAX)
		goto error;

	return (int)result;

error:
	errx(1, "bad number: %s", p);
	/*NOTREACHED*/
}

void
init_hilite(void)
{
	static char control[128];
	char cap[1024];
	char *tc;

	hilite++;

	if (!isatty(fileno(stdout)))
		return;

	tc = getenv("TERM");
	if (tc == NULL)
		tc = "dumb";
	if (tgetent(&cap[0], tc) != 1)
		return;

	tc = &control[0];
	if ((md = tgetstr(hilite > 1 ? "mr" : "md", &tc)))
		*tc++ = '\0';
	if ((me = tgetstr("me", &tc)))
		*tc++ = '\0';
	if (me == NULL || md == NULL)
		md = me = NULL;
}

void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: cal [-hjy3] [-B before] [-A after] [[month] year]\n");
	exit(1);
}
