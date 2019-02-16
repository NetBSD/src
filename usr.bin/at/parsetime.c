/*	$NetBSD: parsetime.c,v 1.20 2019/02/16 17:56:57 kre Exp $	*/

/*
 * parsetime.c - parse time for at(1)
 * Copyright (C) 1993, 1994  Thomas Koenig
 *
 * modifications for english-language times
 * Copyright (C) 1993  David Parsons
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author(s) may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  at [NOW] PLUS NUMBER MINUTES|HOURS|DAYS|WEEKS
 *     /NUMBER [DOT NUMBER] [AM|PM]\ /[MONTH NUMBER [NUMBER]]             \
 *     |NOON                       | |[TOMORROW]                          |
 *     |MIDNIGHT                   | |[DAY OF WEEK]                       |
 *     \TEATIME                    / |NUMBER [SLASH NUMBER [SLASH NUMBER]]|
 *                                   \PLUS NUMBER MINUTES|HOURS|DAYS|WEEKS/
 */

/* System Headers */

#include <sys/types.h>
#include <err.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>

/* Local headers */

#include "at.h"
#include "panic.h"
#include "parsetime.h"
#include "stime.h"

/* Structures and unions */

typedef enum { /* symbols */
	MIDNIGHT, NOON, TEATIME,
	PM, AM, TOMORROW, TODAY, NOW,
	MINUTES, HOURS, DAYS, WEEKS, MONTHS, YEARS,
	NUMBER, PLUS, DOT, SLASH, ID, JUNK,
	JAN, FEB, MAR, APR, MAY, JUN,
	JUL, AUG, SEP, OCT, NOV, DEC,
	SUN, MON, TUE, WED, THU, FRI, SAT,
	TOKEOF	/* EOF marker */
} tokid_t;

/*
 * parse translation table - table driven parsers can be your FRIEND!
 */
static const struct {
	const char *name;	/* token name */
	tokid_t value;		/* token id */
	bool plural;		/* is this plural? */
} Specials[] = {
	{"midnight", MIDNIGHT, false},	/* 00:00:00 of today or tomorrow */
	{"noon", NOON, false},		/* 12:00:00 of today or tomorrow */
	{"teatime", TEATIME, false},	/* 16:00:00 of today or tomorrow */
	{"am", AM, false},		/* morning times for 0-12 clock */
	{"pm", PM, false},		/* evening times for 0-12 clock */
	{"tomorrow", TOMORROW, false},	/* execute 24 hours from time */
	{"today", TODAY, false},		/* execute today - don't advance time */
	{"now", NOW, false},		/* opt prefix for PLUS */

	{"minute", MINUTES, false},	/* minutes multiplier */
	{"min", MINUTES, false},
	{"m", MINUTES, false},
	{"minutes", MINUTES, true},	/* (pluralized) */
	{"hour", HOURS, false},		/* hours ... */
	{"hr", HOURS, false},		/* abbreviated */
	{"h", HOURS, false},
	{"hours", HOURS, true},		/* (pluralized) */
	{"day", DAYS, false},		/* days ... */
	{"d", DAYS, false},
	{"days", DAYS, true},		/* (pluralized) */
	{"week", WEEKS, false},		/* week ... */
	{"w", WEEKS, false},
	{"weeks", WEEKS, true},		/* (pluralized) */
	{ "month", MONTHS, 0 },		/* month ... */
	{ "months", MONTHS, 1 },	/* (pluralized) */
	{ "year", YEARS, 0 },		/* year ... */
	{ "years", YEARS, 1 },		/* (pluralized) */
	{"jan", JAN, false},
	{"feb", FEB, false},
	{"mar", MAR, false},
	{"apr", APR, false},
	{"may", MAY, false},
	{"jun", JUN, false},
	{"jul", JUL, false},
	{"aug", AUG, false},
	{"sep", SEP, false},
	{"oct", OCT, false},
	{"nov", NOV, false},
	{"dec", DEC, false},
	{"january", JAN, false},
	{"february", FEB, false},
	{"march", MAR, false},
	{"april", APR, false},
	{"may", MAY, false},
	{"june", JUN, false},
	{"july", JUL, false},
	{"august", AUG, false},
	{"september", SEP, false},
	{"october", OCT, false},
	{"november", NOV, false},
	{"december", DEC, false},
	{"sunday", SUN, false},
	{"sun", SUN, false},
	{"monday", MON, false},
	{"mon", MON, false},
	{"tuesday", TUE, false},
	{"tue", TUE, false},
	{"wednesday", WED, false},
	{"wed", WED, false},
	{"thursday", THU, false},
	{"thu", THU, false},
	{"friday", FRI, false},
	{"fri", FRI, false},
	{"saturday", SAT, false},
	{"sat", SAT, false}
};

/* File scope variables */

static char **scp;	/* scanner - pointer at arglist */
static char scc;	/* scanner - count of remaining arguments */
static char *sct;	/* scanner - next char pointer in current argument */
static bool need;	/* scanner - need to advance to next argument */

static char *sc_token;	/* scanner - token buffer */
static size_t sc_len;   /* scanner - length of token buffer */
static tokid_t sc_tokid;/* scanner - token id */
static bool sc_tokplur;	/* scanner - is token plural? */

#ifndef lint
#if 0
static char rcsid[] = "$OpenBSD: parsetime.c,v 1.4 1997/03/01 23:40:10 millert Exp $";
#else
__RCSID("$NetBSD: parsetime.c,v 1.20 2019/02/16 17:56:57 kre Exp $");
#endif
#endif

/* Local functions */
static void	assign_date(struct tm *, int, int, int);
static void	expect(tokid_t);
static void	init_scanner(int, char **);
static void	month(struct tm *);
static tokid_t	parse_token(char *);
static void	plonk(tokid_t) __dead;
static void	plus(struct tm *);
static void	tod(struct tm *);
static tokid_t	token(void);

/*
 * parse a token, checking if it's something special to us
 */
static tokid_t
parse_token(char *arg)
{
	size_t i;

	for (i=0; i < __arraycount(Specials); i++) {
		if (strcasecmp(Specials[i].name, arg) == 0) {
			sc_tokplur = Specials[i].plural;
		    	return sc_tokid = Specials[i].value;
		}
	}

	/* not special - must be some random id */
	return ID;
}

/*
 * init_scanner() sets up the scanner to eat arguments
 */
static void
init_scanner(int argc, char **argv)
{

	scp = argv;
	scc = argc;
	need = true;
	sc_len = 1;
	while (argc-- > 0)
		sc_len += strlen(*argv++);

	if ((sc_token = malloc(sc_len)) == NULL)
		panic("Insufficient virtual memory");
}

/*
 * token() fetches a token from the input stream
 */
static tokid_t
token(void)
{
	int idx;

	for(;;) {
		(void)memset(sc_token, 0, sc_len);
		sc_tokid = TOKEOF;
		sc_tokplur = false;
		idx = 0;

		/*
		 * if we need to read another argument, walk along the
		 * argument list; when we fall off the arglist, we'll
		 * just return TOKEOF forever
		 */
		if (need) {
			if (scc < 1)
				return sc_tokid;
			sct = *scp;
			scp++;
			scc--;
			need = false;
		}
		/*
		 * eat whitespace now - if we walk off the end of the argument,
		 * we'll continue, which puts us up at the top of the while loop
		 * to fetch the next argument in
		 */
		while (isspace((unsigned char)*sct))
			++sct;
		if (!*sct) {
			need = true;
			continue;
		}

		/*
		 * preserve the first character of the new token
		 */
		sc_token[0] = *sct++;

		/*
		 * then see what it is
		 */
		if (isdigit((unsigned char)sc_token[0])) {
			while (isdigit((unsigned char)*sct))
				sc_token[++idx] = *sct++;
			sc_token[++idx] = 0;
			return sc_tokid = NUMBER;
		} else if (isalpha((unsigned char)sc_token[0])) {
			while (isalpha((unsigned char)*sct))
				sc_token[++idx] = *sct++;
			sc_token[++idx] = 0;
			return parse_token(sc_token);
		}
		else if (sc_token[0] == ':' || sc_token[0] == '.')
			return sc_tokid = DOT;
		else if (sc_token[0] == '+')
			return sc_tokid = PLUS;
		else if (sc_token[0] == '/')
			return sc_tokid = SLASH;
		else
			return sc_tokid = JUNK;
	}
}

/*
 * plonk() gives an appropriate error message if a token is incorrect
 */
__dead
static void
plonk(tokid_t tok)
{

	panic(tok == TOKEOF ? "incomplete time" : "garbled time");
}

/*
 * expect() gets a token and dies most horribly if it's not the token we want
 */
static void
expect(tokid_t desired)
{

	if (token() != desired)
		plonk(sc_tokid);	/* and we die here... */
}

/*
 * plus() parses a now + time
 *
 *  at [NOW] PLUS NUMBER [MINUTES|HOURS|DAYS|WEEKS|MONTHS|YEARS]
 *
 */
static void
plus(struct tm *tm)
{
	int delay;
	int expectplur;

	expect(NUMBER);

	delay = atoi(sc_token);
	expectplur = delay != 1;

	switch (token()) {
	case YEARS:
		tm->tm_year += delay;
		break;
	case MONTHS:
		tm->tm_mon += delay;
		break;
	case WEEKS:
		delay *= 7;
		/*FALLTHROUGH*/
	case DAYS:
		tm->tm_mday += delay;
		break;
	case HOURS:
		tm->tm_hour += delay;
		break;
	case MINUTES:
		tm->tm_min += delay;
		break;
	default:
		plonk(sc_tokid);
		break;
	}

	if (expectplur != sc_tokplur)
		warnx("pluralization is wrong");

	tm->tm_isdst = -1;
	if (mktime(tm) == -1)
		plonk(sc_tokid);
}

/*
 * tod() computes the time of day
 *     [NUMBER [DOT NUMBER] [AM|PM]]
 */
static void
tod(struct tm *tm)
{
	int hour, minute;
	size_t tlen;

	minute = 0;
	hour = atoi(sc_token);
	tlen = strlen(sc_token);

	/*
	 * first pick out the time of day - if it's 4 digits, we assume
	 * a HHMM time, otherwise it's HH DOT MM time
	 */
	if (token() == DOT) {
		expect(NUMBER);
		minute = atoi(sc_token);
		(void)token();
	} else if (tlen == 4) {
		minute = hour % 100;
		hour = hour / 100;
	}

	if (minute > 59)
		panic("garbled time");

	/*
	 * check if an AM or PM specifier was given
	 */
	if (sc_tokid == AM || sc_tokid == PM) {
		if (hour > 12)
			panic("garbled time");

		if (sc_tokid == PM) {
			if (hour != 12)	/* 12:xx PM is 12:xx, not 24:xx */
				hour += 12;
		} else {
			if (hour == 12)	/* 12:xx AM is 00:xx, not 12:xx */
				hour = 0;
		}
		(void)token();
	} else if (hour > 23)
		panic("garbled time");

	/*
	 * if we specify an absolute time, we don't want to bump the day even
	 * if we've gone past that time - but if we're specifying a time plus
	 * a relative offset, it's okay to bump things
	 */
	if ((sc_tokid == TOKEOF || sc_tokid == PLUS) && (tm->tm_hour > hour ||
	    (tm->tm_hour == hour && tm->tm_min > minute))) {
		tm->tm_mday++;
		tm->tm_wday++;
	}

	tm->tm_hour = hour;
	tm->tm_min = minute;
}

/*
 * assign_date() assigns a date, wrapping to next year if needed.
 * Accept years in 4-digit, 2-digit, or current year (-1).
 */
static void
assign_date(struct tm *tm, int mday, int mon, int year)
{

	if (year > 99) {	/* four digit year */
		if (year >= TM_YEAR_BASE)
			tm->tm_year = year - TM_YEAR_BASE;
		else
			panic("garbled time");
	}
	else if (year >= 0) {	/* two digit year */
		tm->tm_year = conv_2dig_year(year) - TM_YEAR_BASE;
	}
	else if (year == -1) {	/* year not given (use default in tm) */
		/* allow for 1 year range from current date */
		if (tm->tm_mon > mon ||
		    (tm->tm_mon == mon && tm->tm_mday > mday))
			tm->tm_year++;
	}
	else
		panic("invalid year");

	tm->tm_mday = mday;
	tm->tm_mon = mon;
}

/*
 * month() picks apart a month specification
 *
 *  /[<month> NUMBER [NUMBER]]           \
 *  |[TOMORROW]                          |
 *  |[DAY OF WEEK]                       |
 *  |NUMBER [SLASH NUMBER [SLASH NUMBER]]|
 *  \PLUS NUMBER MINUTES|HOURS|DAYS|WEEKS/
 */
static void
month(struct tm *tm)
{
	int year;
	int mday, wday, mon;
	size_t tlen;

	year = -1;
	mday = 0;
	switch (sc_tokid) {
	case PLUS:
		plus(tm);
		break;

	case TOMORROW:
		/* do something tomorrow */
		tm->tm_mday++;
		tm->tm_wday++;
		/*FALLTHROUGH*/
	case TODAY:
		/* force ourselves to stay in today - no further processing */
		(void)token();
		break;

	case JAN: case FEB: case MAR: case APR: case MAY: case JUN:
	case JUL: case AUG: case SEP: case OCT: case NOV: case DEC:
		/*
		 * do month mday [year]
		 */
		mon = sc_tokid - JAN;
		expect(NUMBER);
		mday = atoi(sc_token);
		if (token() == NUMBER) {
			year = atoi(sc_token);
			(void)token();
		}
		assign_date(tm, mday, mon, year);
		break;

	case SUN: case MON: case TUE:
	case WED: case THU: case FRI:
	case SAT:
		/* do a particular day of the week */
		wday = sc_tokid - SUN;

		mday = tm->tm_mday;

		/* if this day is < today, then roll to next week */
		if (wday < tm->tm_wday)
			mday += 7 - (tm->tm_wday - wday);
		else
			mday += (wday - tm->tm_wday);

		tm->tm_wday = wday;

		assign_date(tm, mday, tm->tm_mon, tm->tm_year + TM_YEAR_BASE);
		break;

	case NUMBER:
		/*
		 * get numeric MMDDYY, mm/dd/yy, or dd.mm.yy
		 */
		tlen = strlen(sc_token);
		mon = atoi(sc_token);
		(void)token();

		if (sc_tokid == SLASH || sc_tokid == DOT) {
			tokid_t sep;

			sep = sc_tokid;
			expect(NUMBER);
			mday = atoi(sc_token);
			if (token() == sep) {
				expect(NUMBER);
				year = atoi(sc_token);
				(void)token();
			}

			/*
			 * flip months and days for european timing
			 */
			if (sep == DOT) {
				int x = mday;
				mday = mon;
				mon = x;
			}
		} else if (tlen == 6 || tlen == 8) {
			if (tlen == 8) {
				year = (mon % 10000) - 1900;
				mon /= 10000;
			} else {
				year = mon % 100;
				mon /= 100;
			}
			mday = mon % 100;
			mon /= 100;
		} else
			panic("garbled time");

		mon--;
		if (mon < 0 || mon > 11 || mday < 1 || mday > 31)
			panic("garbled time");

		assign_date(tm, mday, mon, year);
		break;
	default:
		/* plonk(sc_tokid); */	/* XXX - die here? */
		break;
	}
}


/* Global functions */

time_t
parsetime(int argc, char **argv)
{
	/*
	 * Do the argument parsing, die if necessary, and return the
	 * time the job should be run.
	 */
	time_t nowtimer, runtimer;
	struct tm nowtime, runtime;
	int hr = 0; /* this MUST be initialized to zero for
	               midnight/noon/teatime */

	nowtimer = time(NULL);
	nowtime = *localtime(&nowtimer);

	runtime = nowtime;
	runtime.tm_sec = 0;

	if (argc <= optind)
		usage();

	init_scanner(argc - optind, argv + optind);

	switch (token()) {
	case NOW:
		if (scc < 1)
			return nowtimer;

		/* now is optional prefix for PLUS tree */
		expect(PLUS);
		/*FALLTHROUGH*/
	case PLUS:
		plus(&runtime);
		break;

	case NUMBER:
		tod(&runtime);
		month(&runtime);
		break;

		/*
		 * evil coding for TEATIME|NOON|MIDNIGHT - we've initialised
		 * hr to zero up above, then fall into this case in such a
		 * way so we add +12 +4 hours to it for teatime, +12 hours
		 * to it for noon, and nothing at all for midnight, then
		 * set our runtime to that hour before leaping into the
		 * month scanner
		 */
	case TEATIME:
		hr += 4;
		/*FALLTHROUGH*/
	case NOON:
		hr += 12;
		/*FALLTHROUGH*/
	case MIDNIGHT:
		if (runtime.tm_hour >= hr) {
			runtime.tm_mday++;
			runtime.tm_wday++;
		}
		runtime.tm_hour = hr;
		runtime.tm_min = 0;
		(void)token();
		/*FALLTHROUGH*/	/* fall through to month setting */
	default:
		month(&runtime);
		break;
	}
	expect(TOKEOF);

	/*
	 * adjust for daylight savings time
	 */
	runtime.tm_isdst = -1;
	runtimer = mktime(&runtime);

	if (runtimer == (time_t)-1)
		panic("Invalid time");

	if (nowtimer > runtimer)
		panic("Trying to travel back in time");

	return runtimer;
}
