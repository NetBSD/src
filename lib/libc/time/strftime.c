/*	$NetBSD: strftime.c,v 1.51 2022/12/11 17:57:23 christos Exp $	*/

/* Convert a broken-down timestamp to a string.  */

/* Copyright 1989 The Regents of the University of California.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. Neither the name of the University nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
   SUCH DAMAGE.  */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char	elsieid[] = "@(#)strftime.c	7.64";
static char	elsieid[] = "@(#)strftime.c	8.3";
#else
__RCSID("$NetBSD: strftime.c,v 1.51 2022/12/11 17:57:23 christos Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <stddef.h>
#include <assert.h>
#include <locale.h>
#include "setlocale_local.h"

/*
** Based on the UCB version with the copyright notice appearing above.
**
** This is ANSIish only when "multibyte character == plain character".
*/

#include "private.h"

/*
** We don't use these extensions in strftime operation even when
** supported by the local tzcode configuration.  A strictly
** conforming C application may leave them in undefined state.
*/

#ifdef _LIBC
#undef TM_ZONE
#undef TM_GMTOFF
#endif

#include <fcntl.h>
#include <locale.h>
#include <stdio.h>

#ifndef DEPRECATE_TWO_DIGIT_YEARS
# define DEPRECATE_TWO_DIGIT_YEARS false
#endif

#ifdef __weak_alias
__weak_alias(strftime_l, _strftime_l)
__weak_alias(strftime_lz, _strftime_lz)
__weak_alias(strftime_z, _strftime_z)
#endif

#include "sys/localedef.h"
#define _TIME_LOCALE(loc) \
    ((_TimeLocale *)((loc)->part_impl[(size_t)LC_TIME]))
#define c_fmt   d_t_fmt

enum warn { IN_NONE, IN_SOME, IN_THIS, IN_ALL };

static char *	_add(const char *, char *, const char *);
static char *	_conv(int, const char *, char *, const char *, locale_t);
static char *	_fmt(const timezone_t, const char *, const struct tm *, char *,
			const char *, enum warn *, locale_t);
static char *	_yconv(int, int, bool, bool, char *, const char *, locale_t);

#ifndef YEAR_2000_NAME
# define YEAR_2000_NAME "CHECK_STRFTIME_FORMATS_FOR_TWO_DIGIT_YEARS"
#endif /* !defined YEAR_2000_NAME */

#define	IN_NONE	0
#define	IN_SOME	1
#define	IN_THIS	2
#define	IN_ALL	3

#define	PAD_DEFAULT	0
#define	PAD_LESS	1
#define	PAD_SPACE	2
#define	PAD_ZERO	3

static const char fmt_padding[][4][5] = {
	/* DEFAULT,	LESS,	SPACE,	ZERO */
#define	PAD_FMT_MONTHDAY	0
#define	PAD_FMT_HMS		0
#define	PAD_FMT_CENTURY		0
#define	PAD_FMT_SHORTYEAR	0
#define	PAD_FMT_MONTH		0
#define	PAD_FMT_WEEKOFYEAR	0
#define	PAD_FMT_DAYOFMONTH	0
	{ "%02d",	"%d",	"%2d",	"%02d" },
#define	PAD_FMT_SDAYOFMONTH	1
#define	PAD_FMT_SHMS		1
	{ "%2d",	"%d",	"%2d",	"%02d" },
#define	PAD_FMT_DAYOFYEAR	2
	{ "%03d",	"%d",	"%3d",	"%03d" },
#define	PAD_FMT_YEAR		3
	{ "%04d",	"%d",	"%4d",	"%04d" }
};

size_t
strftime_z(const timezone_t sp, char * __restrict s, size_t maxsize,
    const char * __restrict format, const struct tm * __restrict t)
{
	return strftime_lz(sp, s, maxsize, format, t, _current_locale());
}

#if HAVE_STRFTIME_L
size_t
strftime_l(char *s, size_t maxsize, char const *format, struct tm const *t,
	   ATTRIBUTE_MAYBE_UNUSED locale_t locale)
{
  /* Just call strftime, as only the C locale is supported.  */
  return strftime(s, maxsize, format, t);
}
#endif

size_t
strftime_lz(const timezone_t sp, char *const s, const size_t maxsize,
    const char *const format, const struct tm *const t, locale_t loc)
{
	char *	p;
	int saved_errno = errno;
	enum warn warn = IN_NONE;

	p = _fmt(sp, format, t, s, s + maxsize, &warn, loc);
	if (!p) {
		errno = EOVERFLOW;
		return 0;
	}
	if (/*CONSTCOND*/DEPRECATE_TWO_DIGIT_YEARS
	    && warn != IN_NONE && getenv(YEAR_2000_NAME)) {
		(void) fprintf(stderr, "\n");
		(void) fprintf(stderr, "strftime format \"%s\" ", format);
		(void) fprintf(stderr, "yields only two digits of years in ");
		if (warn == IN_SOME)
			(void) fprintf(stderr, "some locales");
		else if (warn == IN_THIS)
			(void) fprintf(stderr, "the current locale");
		else	(void) fprintf(stderr, "all locales");
		(void) fprintf(stderr, "\n");
	}
	if (p == s + maxsize) {
		errno = ERANGE;
		return 0;
	}
	*p = '\0';
	errno = saved_errno;
	return p - s;
}

static char *
_fmt(const timezone_t sp, const char *format, const struct tm *t, char *pt,
     const char *ptlim, enum warn *warnp, locale_t loc)
{
	int Ealternative, Oalternative, PadIndex;
	_TimeLocale *tptr = _TIME_LOCALE(loc);
	
	for ( ; *format; ++format) {
		if (*format == '%') {
			Ealternative = 0;
			Oalternative = 0;
			PadIndex = PAD_DEFAULT;
label:
			switch (*++format) {
			case '\0':
				--format;
				break;
			case 'A':
				pt = _add((t->tm_wday < 0 ||
					t->tm_wday >= DAYSPERWEEK) ?
					"?" : tptr->day[t->tm_wday],
					pt, ptlim);
				continue;
			case 'a':
				pt = _add((t->tm_wday < 0 ||
					t->tm_wday >= DAYSPERWEEK) ?
					"?" : tptr->abday[t->tm_wday],
					pt, ptlim);
				continue;
			case 'B':
				pt = _add((t->tm_mon < 0 ||
					t->tm_mon >= MONSPERYEAR) ?
					"?" :
					/* no alt_month in _TimeLocale */
					(Oalternative ? tptr->mon/*alt_month*/:
					tptr->mon)[t->tm_mon],
					pt, ptlim);
				continue;
			case 'b':
			case 'h':
				pt = _add((t->tm_mon < 0 ||
					t->tm_mon >= MONSPERYEAR) ?
					"?" : tptr->abmon[t->tm_mon],
					pt, ptlim);
				continue;
			case 'C':
				/*
				** %C used to do a...
				**	_fmt("%a %b %e %X %Y", t);
				** ...whereas now POSIX 1003.2 calls for
				** something completely different.
				** (ado, 1993-05-24)
				*/
				pt = _yconv(t->tm_year, TM_YEAR_BASE,
					    true, false, pt, ptlim, loc);
				continue;
			case 'c':
				{
				enum warn warn2 = IN_SOME;

				pt = _fmt(sp, tptr->c_fmt, t, pt,
				    ptlim, &warn2, loc);
				if (warn2 == IN_ALL)
					warn2 = IN_THIS;
				if (warn2 > *warnp)
					*warnp = warn2;
				}
				continue;
			case 'D':
				pt = _fmt(sp, "%m/%d/%y", t, pt, ptlim, warnp,
				    loc);
				continue;
			case 'd':
				pt = _conv(t->tm_mday, 
				    fmt_padding[PAD_FMT_DAYOFMONTH][PadIndex],
				    pt, ptlim, loc);
				continue;
			case 'E':
				if (Ealternative || Oalternative)
					break;
				Ealternative++;
				goto label;
			case 'O':
				/*
				** Locale modifiers of C99 and later.
				** The sequences
				**	%Ec %EC %Ex %EX %Ey %EY
				**	%Od %oe %OH %OI %Om %OM
				**	%OS %Ou %OU %OV %Ow %OW %Oy
				** are supposed to provide alternative
				** representations.
				*/
				if (Ealternative || Oalternative)
					break;
				Oalternative++;
				goto label;
			case 'e':
				pt = _conv(t->tm_mday, 
				    fmt_padding[PAD_FMT_SDAYOFMONTH][PadIndex],
				    pt, ptlim, loc);
				continue;
			case 'F':
				pt = _fmt(sp, "%Y-%m-%d", t, pt, ptlim, warnp,
				    loc);
				continue;
			case 'H':
				pt = _conv(t->tm_hour,
				    fmt_padding[PAD_FMT_HMS][PadIndex],
				    pt, ptlim, loc);
				continue;
			case 'I':
				pt = _conv((t->tm_hour % 12) ?
				    (t->tm_hour % 12) : 12,
				    fmt_padding[PAD_FMT_HMS][PadIndex],
				    pt, ptlim, loc);
				continue;
			case 'j':
				pt = _conv(t->tm_yday + 1, 
				    fmt_padding[PAD_FMT_DAYOFYEAR][PadIndex],
				    pt, ptlim, loc);
				continue;
			case 'k':
				/*
				** This used to be...
				**	_conv(t->tm_hour % 12 ?
				**		t->tm_hour % 12 : 12, 2, ' ');
				** ...and has been changed to the below to
				** match SunOS 4.1.1 and Arnold Robbins'
				** strftime version 3.0. That is, "%k" and
				** "%l" have been swapped.
				** (ado, 1993-05-24)
				*/
				pt = _conv(t->tm_hour, 
				    fmt_padding[PAD_FMT_SHMS][PadIndex],
				    pt, ptlim, loc);
				continue;
#ifdef KITCHEN_SINK
			case 'K':
				/*
				** After all this time, still unclaimed!
				*/
				pt = _add("kitchen sink", pt, ptlim);
				continue;
#endif /* defined KITCHEN_SINK */
			case 'l':
				/*
				** This used to be...
				**	_conv(t->tm_hour, 2, ' ');
				** ...and has been changed to the below to
				** match SunOS 4.1.1 and Arnold Robbin's
				** strftime version 3.0. That is, "%k" and
				** "%l" have been swapped.
				** (ado, 1993-05-24)
				*/
				pt = _conv((t->tm_hour % 12) ?
					(t->tm_hour % 12) : 12,
					fmt_padding[PAD_FMT_SHMS][PadIndex],
					pt, ptlim, loc);
				continue;
			case 'M':
				pt = _conv(t->tm_min,
				    fmt_padding[PAD_FMT_HMS][PadIndex],
				    pt, ptlim, loc);
				continue;
			case 'm':
				pt = _conv(t->tm_mon + 1, 
				    fmt_padding[PAD_FMT_MONTH][PadIndex],
				    pt, ptlim, loc);
				continue;
			case 'n':
				pt = _add("\n", pt, ptlim);
				continue;
			case 'p':
				pt = _add((t->tm_hour >= (HOURSPERDAY / 2)) ?
					tptr->am_pm[1] :
					tptr->am_pm[0],
					pt, ptlim);
				continue;
			case 'R':
				pt = _fmt(sp, "%H:%M", t, pt, ptlim, warnp,
				    loc);
				continue;
			case 'r':
				pt = _fmt(sp, tptr->t_fmt_ampm, t,
				    pt, ptlim, warnp, loc);
				continue;
			case 'S':
				pt = _conv(t->tm_sec, 
				    fmt_padding[PAD_FMT_HMS][PadIndex],
				    pt, ptlim, loc);
				continue;
			case 's':
				{
					struct tm	tm;
					char		buf[INT_STRLEN_MAXIMUM(
								time_t) + 1];
					time_t		mkt;

					tm.tm_sec = t->tm_sec;
					tm.tm_min = t->tm_min;
					tm.tm_hour = t->tm_hour;
					tm.tm_mday = t->tm_mday;
					tm.tm_mon = t->tm_mon;
					tm.tm_year = t->tm_year;
					tm.tm_isdst = t->tm_isdst;
#if defined TM_GMTOFF && ! UNINIT_TRAP
					tm.TM_GMTOFF = t->TM_GMTOFF;
#endif
					mkt = mktime_z(sp, &tm);
					/* If mktime fails, %s expands to the
					   value of (time_t) -1 as a failure
					   marker; this is better in practice
					   than strftime failing.  */
					/* CONSTCOND */
					if (TYPE_SIGNED(time_t)) {
						intmax_t n = mkt;
						(void)snprintf(buf, sizeof(buf),
						    "%"PRIdMAX, n);
					} else {
						uintmax_t n = mkt;
						(void)snprintf(buf, sizeof(buf),
						    "%"PRIuMAX, n);
					}
					pt = _add(buf, pt, ptlim);
				}
				continue;
			case 'T':
				pt = _fmt(sp, "%H:%M:%S", t, pt, ptlim, warnp,
				    loc);
				continue;
			case 't':
				pt = _add("\t", pt, ptlim);
				continue;
			case 'U':
				pt = _conv((t->tm_yday + DAYSPERWEEK -
				    t->tm_wday) / DAYSPERWEEK,
				    fmt_padding[PAD_FMT_WEEKOFYEAR][PadIndex],
				    pt, ptlim, loc);
				continue;
			case 'u':
				/*
				** From Arnold Robbins' strftime version 3.0:
				** "ISO 8601: Weekday as a decimal number
				** [1 (Monday) - 7]"
				** (ado, 1993-05-24)
				*/
				pt = _conv((t->tm_wday == 0) ?
					DAYSPERWEEK : t->tm_wday,
					"%d", pt, ptlim, loc);
				continue;
			case 'V':	/* ISO 8601 week number */
			case 'G':	/* ISO 8601 year (four digits) */
			case 'g':	/* ISO 8601 year (two digits) */
/*
** From Arnold Robbins' strftime version 3.0: "the week number of the
** year (the first Monday as the first day of week 1) as a decimal number
** (01-53)."
** (ado, 1993-05-24)
**
** From <https://www.cl.cam.ac.uk/~mgk25/iso-time.html> by Markus Kuhn:
** "Week 01 of a year is per definition the first week which has the
** Thursday in this year, which is equivalent to the week which contains
** the fourth day of January. In other words, the first week of a new year
** is the week which has the majority of its days in the new year. Week 01
** might also contain days from the previous year and the week before week
** 01 of a year is the last week (52 or 53) of the previous year even if
** it contains days from the new year. A week starts with Monday (day 1)
** and ends with Sunday (day 7). For example, the first week of the year
** 1997 lasts from 1996-12-30 to 1997-01-05..."
** (ado, 1996-01-02)
*/
				{
					int	year;
					int	base;
					int	yday;
					int	wday;
					int	w;

					year = t->tm_year;
					base = TM_YEAR_BASE;
					yday = t->tm_yday;
					wday = t->tm_wday;
					for ( ; ; ) {
						int	len;
						int	bot;
						int	top;

						len = isleap_sum(year, base) ?
							DAYSPERLYEAR :
							DAYSPERNYEAR;
						/*
						** What yday (-3 ... 3) does
						** the ISO year begin on?
						*/
						bot = ((yday + 11 - wday) %
							DAYSPERWEEK) - 3;
						/*
						** What yday does the NEXT
						** ISO year begin on?
						*/
						top = bot -
							(len % DAYSPERWEEK);
						if (top < -3)
							top += DAYSPERWEEK;
						top += len;
						if (yday >= top) {
							++base;
							w = 1;
							break;
						}
						if (yday >= bot) {
							w = 1 + ((yday - bot) /
								DAYSPERWEEK);
							break;
						}
						--base;
						yday += isleap_sum(year, base) ?
							DAYSPERLYEAR :
							DAYSPERNYEAR;
					}
#ifdef XPG4_1994_04_09
					if ((w == 52 &&
						t->tm_mon == TM_JANUARY) ||
						(w == 1 &&
						t->tm_mon == TM_DECEMBER))
							w = 53;
#endif /* defined XPG4_1994_04_09 */
					if (*format == 'V')
						pt = _conv(w, 
						    fmt_padding[
						    PAD_FMT_WEEKOFYEAR][
						    PadIndex], pt, ptlim, loc);
					else if (*format == 'g') {
						*warnp = IN_ALL;
						pt = _yconv(year, base,
							false, true,
							pt, ptlim, loc);
					} else	pt = _yconv(year, base,
							true, true,
							pt, ptlim, loc);
				}
				continue;
			case 'v':
				/*
				** From Arnold Robbins' strftime version 3.0:
				** "date as dd-bbb-YYYY"
				** (ado, 1993-05-24)
				*/
				pt = _fmt(sp, "%e-%b-%Y", t, pt, ptlim, warnp,
				    loc);
				continue;
			case 'W':
				pt = _conv((t->tm_yday + DAYSPERWEEK -
				    (t->tm_wday ?
				    (t->tm_wday - 1) :
				    (DAYSPERWEEK - 1))) / DAYSPERWEEK,
				    fmt_padding[PAD_FMT_WEEKOFYEAR][PadIndex],
				    pt, ptlim, loc);
				continue;
			case 'w':
				pt = _conv(t->tm_wday, "%d", pt, ptlim, loc);
				continue;
			case 'X':
				pt = _fmt(sp, tptr->t_fmt, t, pt,
				    ptlim, warnp, loc);
				continue;
			case 'x':
				{
				enum warn warn2 = IN_SOME;

				pt = _fmt(sp, tptr->d_fmt, t, pt,
				    ptlim, &warn2, loc);
				if (warn2 == IN_ALL)
					warn2 = IN_THIS;
				if (warn2 > *warnp)
					*warnp = warn2;
				}
				continue;
			case 'y':
				*warnp = IN_ALL;
				pt = _yconv(t->tm_year, TM_YEAR_BASE,
					false, true,
					pt, ptlim, loc);
				continue;
			case 'Y':
				pt = _yconv(t->tm_year, TM_YEAR_BASE,
					true, true,
					pt, ptlim, loc);
				continue;
			case 'Z':
#ifdef TM_ZONE
				pt = _add(t->TM_ZONE, pt, ptlim);
#elif HAVE_TZNAME
				if (t->tm_isdst >= 0) {
					int oerrno = errno, dst = t->tm_isdst;
					const char *z =
					    tzgetname(sp, dst);
					if (z == NULL)
						z = tzgetname(sp, !dst);
					if (z != NULL)
						pt = _add(z, pt, ptlim);
					errno = oerrno;
				}
#endif
				/*
				** C99 and later say that %Z must be
				** replaced by the empty string if the
				** time zone abbreviation is not
				** determinable.
				*/
				continue;
			case 'z':
#if defined TM_GMTOFF || USG_COMPAT || ALTZONE
				{
				long		diff;
				char const *	sign;
				bool negative;

				if (t->tm_isdst < 0)
					continue;
# ifdef TM_GMTOFF
				diff = (int)t->TM_GMTOFF;
# else 
				/*
				** C99 and later say that the UT offset must
				** be computed by looking only at
				** tm_isdst. This requirement is
				** incorrect, since it means the code
				** must rely on magic (in this case
				** altzone and timezone), and the
				** magic might not have the correct
				** offset. Doing things correctly is
				** tricky and requires disobeying the standard;
				** see GNU C strftime for details.
				** For now, punt and conform to the
				** standard, even though it's incorrect.
				**
				** C99 and later say that %z must be replaced by
				** the empty string if the time zone is not
				** determinable, so output nothing if the
				** appropriate variables are not available.
				*/
#  ifndef STD_INSPIRED
				if (t->tm_isdst == 0)
#   if USG_COMPAT
					diff = -timezone;
#   else
					continue;
#   endif
				else
#   if ALTZONE
					diff = -altzone;
#   else
					continue;
#   endif
#  else
				{
					struct tm tmp;
					time_t lct, gct;

					/*
					** Get calendar time from t
					** being treated as local.
					*/
					tmp = *t; /* mktime discards const */
					lct = mktime_z(sp, &tmp);

					if (lct == (time_t)-1)
						continue;

					/*
					** Get calendar time from t
					** being treated as GMT.
					**/
					tmp = *t; /* mktime discards const */
					gct = timegm(&tmp);

					if (gct == (time_t)-1)
						continue;

					/* LINTED difference will fit int */
					diff = (intmax_t)gct - (intmax_t)lct;
				}
#  endif
# endif
				negative = diff < 0;
				if (diff == 0) {
# ifdef TM_ZONE
				  negative = t->TM_ZONE[0] == '-';
# else
				  negative = t->tm_isdst < 0;
#  if HAVE_TZNAME
				  if (tzname[t->tm_isdst != 0][0] == '-')
				    negative = true;
#  endif
# endif
				}
				if (negative) {
					sign = "-";
					diff = -diff;
				} else	sign = "+";
				pt = _add(sign, pt, ptlim);
				diff /= SECSPERMIN;
				diff = (diff / MINSPERHOUR) * 100 +
					(diff % MINSPERHOUR);
				_DIAGASSERT(__type_fit(int, diff));
				pt = _conv((int)diff,
				    fmt_padding[PAD_FMT_YEAR][PadIndex],
				    pt, ptlim, loc);
				}
#endif
				continue;
			case '+':
#ifdef notyet
				/* XXX: no date_fmt in _TimeLocale */
				pt = _fmt(sp, tptr->date_fmt, t,
				    pt, ptlim, warnp, loc);
#else
				pt = _fmt(sp, "%a %b %e %H:%M:%S %Z %Y", t,
				    pt, ptlim, warnp, loc);
#endif
				continue;
			case '-':
				if (PadIndex != PAD_DEFAULT)
					break;
				PadIndex = PAD_LESS;
				goto label;
			case '_':
				if (PadIndex != PAD_DEFAULT)
					break;
				PadIndex = PAD_SPACE;
				goto label;
			case '0':
				if (PadIndex != PAD_DEFAULT)
					break;
				PadIndex = PAD_ZERO;
				goto label;
			case '%':
			/*
			** X311J/88-090 (4.12.3.5): if conversion char is
			** undefined, behavior is undefined. Print out the
			** character itself as printf(3) also does.
			*/
			default:
				break;
			}
		}
		if (pt == ptlim)
			break;
		*pt++ = *format;
	}
	return pt;
}

size_t
strftime(char *s, size_t maxsize, const char *format, const struct tm *t)
{
	size_t r;
	
	rwlock_wrlock(&__lcl_lock);
	tzset_unlocked();
	r = strftime_z(__lclptr, s, maxsize, format, t);
	rwlock_unlock(&__lcl_lock);

	return r;
}

size_t
strftime_l(char * __restrict s, size_t maxsize, const char * __restrict format,
    const struct tm * __restrict t, locale_t loc)
{
	size_t r;

	rwlock_wrlock(&__lcl_lock);
	tzset_unlocked();
	r = strftime_lz(__lclptr, s, maxsize, format, t, loc);
	rwlock_unlock(&__lcl_lock);

	return r;
}

static char *
_conv(int n, const char *format, char *pt, const char *ptlim, locale_t loc)
{
	char	buf[INT_STRLEN_MAXIMUM(int) + 1];

	(void) snprintf_l(buf, sizeof(buf), loc, format, n);
	return _add(buf, pt, ptlim);
}

static char *
_add(const char *str, char *pt, const char *ptlim)
{
	while (pt < ptlim && (*pt = *str++) != '\0')
		++pt;
	return pt;
}

/*
** POSIX and the C Standard are unclear or inconsistent about
** what %C and %y do if the year is negative or exceeds 9999.
** Use the convention that %C concatenated with %y yields the
** same output as %Y, and that %Y contains at least 4 bytes,
** with more only if necessary.
*/

static char *
_yconv(int a, int b, bool convert_top, bool convert_yy,
    char *pt, const char * ptlim, locale_t loc)
{
	int	lead;
	int	trail;

	int DIVISOR = 100;
	trail = a % DIVISOR + b % DIVISOR;
	lead = a / DIVISOR + b / DIVISOR + trail / DIVISOR;
	trail %= DIVISOR;
	if (trail < 0 && lead > 0) {
		trail += DIVISOR;
		--lead;
	} else if (lead < 0 && trail > 0) {
		trail -= DIVISOR;
		++lead;
	}
	if (convert_top) {
		if (lead == 0 && trail < 0)
			pt = _add("-0", pt, ptlim);
		else	pt = _conv(lead, "%02d", pt, ptlim, loc);
	}
	if (convert_yy)
		pt = _conv(((trail < 0) ? -trail : trail), "%02d", pt, ptlim,
		    loc);
	return pt;
}
