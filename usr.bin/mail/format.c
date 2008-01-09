/*	$NetBSD: format.c,v 1.7.4.2 2008/01/09 02:00:46 matt Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Anon Ymous.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef __lint__
__RCSID("$NetBSD: format.c,v 1.7.4.2 2008/01/09 02:00:46 matt Exp $");
#endif /* not __lint__ */

#include <time.h>
#include <stdio.h>
#include <util.h>

#include "def.h"
#include "extern.h"
#include "format.h"
#include "glob.h"
#include "thread.h"

#undef DEBUG
#ifdef DEBUG
#define DPRINTF(a) printf a
#else
#define DPRINTF(a)
#endif

static void
check_bufsize(char **buf, size_t *bufsize, char **p, size_t cnt)
{
	char *q;
	if (*p + cnt < *buf + *bufsize)
		return;
	*bufsize *= 2;
	q = erealloc(*buf, *bufsize);
	*p = q + (*p - *buf);
	*buf = q;
}

static const char *
sfmtoff(const char **fmtbeg, const char *fmtch, off_t off)
{
	char *newfmt;	/* pointer to new format string */
	size_t len;	/* space for "lld" including '\0' */
	char *p;

	len = fmtch - *fmtbeg + sizeof(PRId64);
	newfmt = salloc(len);
	(void)strlcpy(newfmt, *fmtbeg, len - sizeof(PRId64) + 1);
	(void)strlcat(newfmt, PRId64, len);
	*fmtbeg = fmtch + 1;
	(void)sasprintf(&p, newfmt, off);
	return p;
}

static const char *
sfmtint(const char **fmtbeg, const char *fmtch, int num)
{
	char *newfmt;
	size_t len;
	char *p;

	len = fmtch - *fmtbeg + 2;	/* space for 'd' and '\0' */
	newfmt = salloc(len);
	(void)strlcpy(newfmt, *fmtbeg, len);
	newfmt[len-2] = 'd';		/* convert to printf format */
	*fmtbeg = fmtch + 1;
	(void)sasprintf(&p, newfmt, num);
	return p;
}

static const char *
sfmtstr(const char **fmtbeg, const char *fmtch, const char *str)
{
	char *newfmt;
	size_t len;
	char *p;

	len = fmtch - *fmtbeg + 2;	/* space for 's' and '\0' */
	newfmt = salloc(len);
	(void)strlcpy(newfmt, *fmtbeg, len);
	newfmt[len-2] = 's';		/* convert to printf format */
	*fmtbeg = fmtch + 1;
	(void)sasprintf(&p, newfmt, str ? str : "");
	return p;
}

#ifdef THREAD_SUPPORT
static char*
sfmtdepth(char *str, int depth)
{
	char *p;
	if (*str == '\0') {
		(void)sasprintf(&p, "%d", depth);
		return p;
	}
	p = __UNCONST("");
	for (/*EMPTY*/; depth > 0; depth--)
		(void)sasprintf(&p, "%s%s", p, str);
	return p;
}
#endif

static const char *
sfmtfield(const char **fmtbeg, const char *fmtch, struct message *mp)
{
	char *q;
	q = strchr(fmtch + 1, '?');
	if (q) {
		size_t len;
		char *p;
		const char *str;
		int skin_it;
#ifdef THREAD_SUPPORT
		int depth;
#endif
		if (mp == NULL) {
			*fmtbeg = q + 1;
			return NULL;
		}
#ifdef THREAD_SUPPORT
		depth = mp->m_depth;
#endif
		skin_it = 0;
		switch (fmtch[1]) { /* check the '?' modifier */
#ifdef THREAD_SUPPORT
		case '&':	/* use the relative depth */
			depth -= thread_depth();
			/* FALLTHROUGH */
		case '*':	/* use the absolute depth */
			len = q - fmtch - 1;
			p = salloc(len);
			(void)strlcpy(p, fmtch + 2, len);
			p = sfmtdepth(p, depth);
			break;
#endif
		case '-':
			skin_it = 1;
			/* FALLTHROUGH */
		default:
			len = q - fmtch - skin_it;
			p = salloc(len);
			(void)strlcpy(p, fmtch + skin_it + 1, len);
			p = hfield(p, mp);
			if (skin_it)
				p = skin(p);
			break;
		}
		str = sfmtstr(fmtbeg, fmtch, p);
		*fmtbeg = q + 1;
		return str;
	}
	return NULL;
}

struct flags_s {
	int f_and;
	int f_or;
	int f_new;	/* some message in the thread is new */
	int f_unread;	/* some message in the thread is unread */
};

static void
get_and_or_flags(struct message *mp, struct flags_s *flags)
{
	for (/*EMPTY*/; mp; mp = mp->m_flink) {
		flags->f_and &= mp->m_flag;
		flags->f_or  |= mp->m_flag;
		flags->f_new    |= (mp->m_flag & (MREAD|MNEW)) == MNEW;
		flags->f_unread |= (mp->m_flag & (MREAD|MNEW)) == 0;
		get_and_or_flags(mp->m_clink, flags);
	}
}

static const char *
sfmtflag(const char **fmtbeg, const char *fmtch, struct message *mp)
{
	char disp[2];
	struct flags_s flags;
	int is_thread;

	if (mp == NULL)
		return NULL;

	is_thread = mp->m_clink != NULL;
	disp[0] = is_thread ? '+' : ' ';
	disp[1] = '\0';

	flags.f_and = mp->m_flag;
	flags.f_or = mp->m_flag;
	flags.f_new = 0;
	flags.f_unread = 0;
#ifdef THREAD_SUPPORT
	if (thread_hidden())
		get_and_or_flags(mp->m_clink, &flags);
#endif

	if (flags.f_or & MTAGGED)
		disp[0] = 't';
	if (flags.f_and & MTAGGED)
		disp[0] = 'T';

	if (flags.f_or & MMODIFY)
		disp[0] = 'e';
	if (flags.f_and & MMODIFY)
		disp[0] = 'E';

	if (flags.f_or & MSAVED)
		disp[0] = '&';
	if (flags.f_and & MSAVED)
		disp[0] = '*';

	if (flags.f_or & MPRESERVE)
		disp[0] = 'p';
	if (flags.f_and & MPRESERVE)
		disp[0] = 'P';

	if (flags.f_unread)
		disp[0] = 'u';
	if ((flags.f_or & (MREAD|MNEW)) == 0)
		disp[0] = 'U';

	if (flags.f_new)
		disp[0] = 'n';
	if ((flags.f_and & (MREAD|MNEW)) == MNEW)
		disp[0] = 'N';

	if (flags.f_or & MBOX)
		disp[0] = 'm';
	if (flags.f_and & MBOX)
		disp[0] = 'M';

	return sfmtstr(fmtbeg, fmtch, disp);
}

static const char *
login_name(const char *addr)
{
	char *p;
	p = strchr(addr, '@');
	if (p) {
		char *q;
		size_t len;
		len = p - addr + 1;
		q = salloc(len);
		(void)strlcpy(q, addr, len);
		return q;
	}
	return addr;
}

/*
 * A simple routine to get around a lint warning.
 */
static inline const char *
skip_fmt(const char **src, const char *p)
{
	*src = p;
	return NULL;
}

static const char *
subformat(const char **src, struct message *mp, const char *addr,
    const char *user, const char *subj, int tm_isdst)
{
#if 0
/* XXX - lint doesn't like this, hence skip_fmt(). */
#define MP(a)	mp ? a : (*src = (p + 1), NULL)
#else
#define MP(a)	mp ? a : skip_fmt(src, p + 1);
#endif
	const char *p;

	p = *src;
	if (p[1] == '%') {
		*src += 2;
		return "%%";
	}
	for (p = *src; *p && !isalpha((unsigned char)*p) && *p != '?'; p++)
		continue;

	switch (*p) {
	/*
	 * Our format extensions to strftime(3)
	 */
	case '?':
		return sfmtfield(src, p, mp);
	case 'J':
		return MP(sfmtint(src, p, (int)(mp->m_lines - mp->m_blines)));
	case 'K':
 		return MP(sfmtint(src, p, (int)mp->m_blines));
	case 'L':
 		return MP(sfmtint(src, p, (int)mp->m_lines));
	case 'N':
		return sfmtstr(src, p, user);
	case 'O':
 		return MP(sfmtoff(src, p, mp->m_size));
	case 'P':
 		return MP(sfmtstr(src, p, mp == dot ? ">" : " "));
	case 'Q':
 		return MP(sfmtflag(src, p, mp));
	case 'f':
		return sfmtstr(src, p, addr);
	case 'i':
 		return sfmtint(src, p, get_msgnum(mp));	/* '0' if mp == NULL */
	case 'n':
		return sfmtstr(src, p, login_name(addr));
	case 'q':
		return sfmtstr(src, p, subj);
	case 't':
		return sfmtint(src, p, get_msgCount());

	/*
	 * strftime(3) special cases:
	 *
	 * When 'tm_isdst' was not determined (i.e., < 0), a C99
	 * compliant strftime(3) will output an empty string for the
	 * "%Z" and "%z" formats.  This messes up alignment so we
	 * handle these ourselves.
	 */
	case 'Z':
		if (tm_isdst < 0) {
			*src = p + 1;
			return "???";	/* XXX - not ideal */
		}
		return NULL;
	case 'z':
		if (tm_isdst < 0) {
			*src = p + 1;
			return "-0000";	/* consistent with RFC 2822 */
		}
		return NULL;

	/* everything else is handled by strftime(3) */
	default:
		return NULL;
	}
#undef MP
}

static const char *
snarf_comment(char **buf, char *bufend, const char *string)
{
	const char *p;
	char *q;
	char *qend;
	int clevel;

	q    = buf ? *buf : NULL;
	qend = buf ? bufend : NULL;

	clevel = 1;
	for (p = string + 1; *p != '\0'; p++) {
		DPRINTF(("snarf_comment: %s\n", p));
		if (*p == '(') {
			clevel++;
			continue;
		}
		if (*p == ')') {
			if (--clevel == 0)
				break;
			continue;
		}
		if (*p == '\\' && p[1] != 0)
			p++;

		if (q < qend)
			*q++ = *p;
	}
	if (buf) {
		*q = '\0';
		DPRINTF(("snarf_comment: terminating: %s\n", *buf));
		*buf = q;
	}
	if (*p == '\0')
		p--;
	return p;
}

static const char *
snarf_quote(char **buf, char *bufend, const char *string)
{
	const char *p;
	char *q;
	char *qend;

	q    = buf ? *buf : NULL;
	qend = buf ? bufend : NULL;

	for (p = string + 1; *p != '\0' && *p != '"'; p++) {
		DPRINTF(("snarf_quote: %s\n", p));
		if (*p == '\\' && p[1] != '\0')
			p++;

		if (q < qend)
			*q++ = *p;
	}
	if (buf) {
		*q = '\0';
		DPRINTF(("snarf_quote: terminating: %s\n", *buf));
		*buf = q;
	}
	if (*p == '\0')
		p--;
	return p;
}

/*
 * Grab the comments, separating each by a space.
 */
static char *
get_comments(char *name)
{
	char nbuf[LINESIZE];
	const char *p;
	char *qend;
	char *q;
	char *lastq;

	if (name == NULL)
		return NULL;

	p = name;
	q = nbuf;
	lastq = nbuf;
	qend = nbuf + sizeof(nbuf) - 1;
	for (p = skip_WSP(name); *p != '\0'; p++) {
		DPRINTF(("get_comments: %s\n", p));
		switch (*p) {
		case '"': /* quoted-string ... skip it! */
			p = snarf_quote(NULL, NULL, p);
			break;

		case '(':
			p = snarf_comment(&q, qend, p);
			lastq = q;
			if (q < qend) /* separate comments by space */
				*q++ = ' ';
			break;

		default:
			break;
		}
	}
	*lastq = '\0';
	return savestr(nbuf);
}

/*
 * Convert a possible obs_zone (see RFC 2822, sec 4.3) to a valid
 * gmtoff string.
 */
static const char *
convert_obs_zone(const char *obs_zone)
{
	static const struct obs_zone_tbl_s {
		const char *zone;
		const char *gmtoff;
	} obs_zone_tbl[] = {
		{"UT",	"+0000"},
		{"GMT",	"+0000"},
		{"EST",	"-0500"},
		{"EDT",	"-0400"},
		{"CST",	"-0600"},
		{"CDT",	"-0500"},
		{"MST",	"-0700"},
		{"MDT",	"-0600"},
		{"PST",	"-0800"},
		{"PDT",	"-0700"},
		{NULL,	NULL},
	};
	const struct obs_zone_tbl_s *zp;

	if (obs_zone[0] == '+' || obs_zone[0] == '-')
		return obs_zone;

	if (obs_zone[1] == 0) { /* possible military zones */
		/* be explicit here - avoid C extensions and ctype(3) */
		switch((unsigned char)obs_zone[0]) {
		case 'A': case 'B': case 'C': case 'D':	case 'E':
		case 'F': case 'G': case 'H': case 'I':
		case 'K': case 'L': case 'M': case 'N': case 'O':
		case 'P': case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X': case 'Y':
		case 'Z':
		case 'a': case 'b': case 'c': case 'd':	case 'e':
		case 'f': case 'g': case 'h': case 'i':
		case 'k': case 'l': case 'm': case 'n': case 'o':
		case 'p': case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x': case 'y':
		case 'z':
			return "-0000";	/* See RFC 2822, sec 4.3 */
		default:
			return obs_zone;
		}
	}
	for (zp = obs_zone_tbl; zp->zone; zp++) {
		if (strcmp(obs_zone, zp->zone) == 0)
			return zp->gmtoff;
	}
	return obs_zone;
}

/*
 * Parse the 'Date:" field into a tm structure and return the gmtoff
 * string or NULL on error.
 */
static const char *
date_to_tm(char *date, struct tm *tm)
{
	/****************************************************************
	 * The header 'date-time' syntax - From RFC 2822 sec 3.3 and 4.3:
	 *
	 * date-time       =       [ day-of-week "," ] date FWS time [CFWS]
	 * day-of-week     =       ([FWS] day-name) / obs-day-of-week
	 * day-name        =       "Mon" / "Tue" / "Wed" / "Thu" /
	 *                         "Fri" / "Sat" / "Sun"
	 * date            =       day month year
	 * year            =       4*DIGIT / obs-year
	 * month           =       (FWS month-name FWS) / obs-month
	 * month-name      =       "Jan" / "Feb" / "Mar" / "Apr" /
	 *                         "May" / "Jun" / "Jul" / "Aug" /
	 *                         "Sep" / "Oct" / "Nov" / "Dec"
	 * day             =       ([FWS] 1*2DIGIT) / obs-day
	 * time            =       time-of-day FWS zone
	 * time-of-day     =       hour ":" minute [ ":" second ]
	 * hour            =       2DIGIT / obs-hour
	 * minute          =       2DIGIT / obs-minute
	 * second          =       2DIGIT / obs-second
	 * zone            =       (( "+" / "-" ) 4DIGIT) / obs-zone
	 *
	 * obs-day-of-week =       [CFWS] day-name [CFWS]
	 * obs-year        =       [CFWS] 2*DIGIT [CFWS]
	 * obs-month       =       CFWS month-name CFWS
	 * obs-day         =       [CFWS] 1*2DIGIT [CFWS]
	 * obs-hour        =       [CFWS] 2DIGIT [CFWS]
	 * obs-minute      =       [CFWS] 2DIGIT [CFWS]
	 * obs-second      =       [CFWS] 2DIGIT [CFWS]
	 ****************************************************************/
	/*
	 * For example, a typical date might look like:
	 *
	 * Date: Mon,  1 Oct 2007 05:38:10 +0000 (UTC)
	 */
	char *tail;
	char *p;
	struct tm tmp_tm;
	/*
	 * NOTE: Rather than depend on strptime(3) modifying only
	 * those fields specified in its format string, we use tmp_tm
	 * and copy the appropriate result to tm.  This is not
	 * required with the NetBSD strptime(3) implementation.
	 */

	/* Check for an optional 'day-of-week' */
	if ((tail = strptime(date, " %a,", &tmp_tm)) == NULL) {
		tail = date;
		tm->tm_wday = tmp_tm.tm_wday;
	}

	/* Get the required 'day' and 'month' */
	if ((tail = strptime(tail, " %d %b", &tmp_tm)) == NULL)
		return NULL;

	tm->tm_mday = tmp_tm.tm_mday;
	tm->tm_mon  = tmp_tm.tm_mon;

	/* Check for 'obs-year' (2 digits) before 'year' (4 digits) */
	/* XXX - Portable?  This depends on strptime not scanning off
	 * trailing whitespace unless specified in the format string.
	 */
	if ((p = strptime(tail, " %y", &tmp_tm)) != NULL && is_WSP(*p))
		tail = p;
	else if ((tail = strptime(tail, " %Y", &tmp_tm)) == NULL)
		return NULL;

	tm->tm_year = tmp_tm.tm_year;

	/* Get the required 'hour' and 'minute' */
	if ((tail = strptime(tail, " %H:%M", &tmp_tm)) == NULL)
		return NULL;

	tm->tm_hour = tmp_tm.tm_hour;
	tm->tm_min  = tmp_tm.tm_min;

	/* Check for an optional 'seconds' field */
	if ((p = strptime(tail, ":%S", &tmp_tm)) != NULL) {
		tail = p;
		tm->tm_sec = tmp_tm.tm_sec;
	}

	tail = skip_WSP(tail);

	/*
	 * The timezone name is frequently in a comment following the
	 * zone offset.
	 *
	 * XXX - this will get overwritten later by timegm(3).
	 */
	if ((p = strchr(tail, '(')) != NULL)
		tm->tm_zone = get_comments(p);
	else
		tm->tm_zone = NULL;

	/* what remains should be the gmtoff string */
	tail = skin(tail);
	return convert_obs_zone(tail);
}

/*
 * Parse the headline string into a tm structure.  Returns a pointer
 * to first non-whitespace after the date or NULL on error.
 *
 * XXX - This needs to be consistent with isdate().
 */
static char *
hl_date_to_tm(const char *buf, struct tm *tm)
{
	/****************************************************************
	 * The BSD and System V headline date formats differ
	 * and each have an optional timezone field between
	 * the time and date (see head.c).  Unfortunately,
	 * strptime(3) doesn't know about timezone fields, so
	 * we have to handle it ourselves.
	 *
	 * char ctype[]        = "Aaa Aaa O0 00:00:00 0000";
	 * char tmztype[]      = "Aaa Aaa O0 00:00:00 AAA 0000";
	 * char SysV_ctype[]   = "Aaa Aaa O0 00:00 0000";
	 * char SysV_tmztype[] = "Aaa Aaa O0 00:00 AAA 0000";
	 ****************************************************************/
	char *tail;
	char *p;
	char zone[4];
	struct tm tmp_tm; /* see comment in date_to_tm() */
	int len;

	zone[0] = '\0';
	if ((tail = strptime(buf, " %a %b %d %H:%M", &tmp_tm)) == NULL)
		return NULL;

	tm->tm_wday = tmp_tm.tm_wday;
	tm->tm_mday = tmp_tm.tm_mday;
	tm->tm_mon  = tmp_tm.tm_mon;
	tm->tm_hour = tmp_tm.tm_hour;
	tm->tm_min  = tmp_tm.tm_min;

	/* Check for an optional 'seconds' field */
	if ((p = strptime(tail, ":%S", &tmp_tm)) != NULL) {
		tail = p;
		tm->tm_sec = tmp_tm.tm_sec;
	}

	/* Grab an optional timezone name */
	/*
	 * XXX - Is the zone name always 3 characters as in isdate()?
	 */
	if (sscanf(tail, " %3[A-Z] %n", zone, &len) == 1) {
		if (zone[0])
			tm->tm_zone = savestr(zone);
		tail += len;
	}

	/* Grab the required year field */
	tail = strptime(tail, " %Y ", &tmp_tm);
	tm->tm_year = tmp_tm.tm_year; /* save this even if it failed */

	return tail;
}

/*
 * Get the date and time info from the "Date:" line, parse it into a
 * tm structure as much as possible.
 *
 * Note: We return the gmtoff as a string as "-0000" has special
 * meaning.  See RFC 2822, sec 3.3.
 */
PUBLIC void
dateof(struct tm *tm, struct message *mp, int use_hl_date)
{
	static int tzinit = 0;
	char *date = NULL;
	const char *gmtoff;

	(void)memset(tm, 0, sizeof(*tm));

	/* Make sure the time zone info is initialized. */
	if (!tzinit) {
		tzinit = 1;
		tzset();
	}
	if (mp == NULL) {	/* use local time */
		time_t now;
		(void)time(&now);
		(void)localtime_r(&now, tm);
		return;
	}

	/*
	 * See RFC 2822 sec 3.3 for date-time format used in
	 * the "Date:" field.
	 *
	 * NOTE: The range for the time is 00:00 to 23:60 (to allow
	 * for a leep second), but I have seen this violated making
	 * strptime() fail, e.g.,
	 *
	 *   Date: Tue, 24 Oct 2006 24:07:58 +0400
	 *
	 * In this case we (silently) fall back to the headline time
	 * which was written locally when the message was received.
	 * Of course, this is not the same time as in the Date field.
	 */
	if (use_hl_date == 0 &&
	    (date = hfield("date", mp)) != NULL &&
	    (gmtoff = date_to_tm(date, tm)) != NULL) {
		int hour;
		int min;
		char sign[2];
		struct tm save_tm;

		/*
		 * Scan the gmtoff and use it to convert the time to a
		 * local time.
		 *
		 * Note: "-0000" means no valid zone info.  See
		 *       RFC 2822, sec 3.3.
		 *
		 * XXX - This is painful!  Is there a better way?
		 */

		tm->tm_isdst = -1;	/* let timegm(3) determine tm_isdst */
		save_tm = *tm;		/* use this if we fail */

		if (strcmp(gmtoff, "-0000") != 0 &&
		    sscanf(gmtoff, " %1[+-]%2d%2d ", sign, &hour, &min) == 3) {
			time_t otime;

			if (sign[0] == '-') {
				tm->tm_hour += hour;
				tm->tm_min += min;
			}
			else {
				tm->tm_hour -= hour;
				tm->tm_min -= min;
			}
			if ((otime = timegm(tm)) == (time_t)-1 ||
			    localtime_r(&otime, tm) == NULL) {
				if (debug)
					warnx("cannot convert date: \"%s\"", date);
				*tm = save_tm;
			}
		}
		else {	/* Unable to do the conversion to local time. */
			*tm = save_tm;
		     /* tm->tm_isdst = -1; */ /* Set above */
			tm->tm_gmtoff = 0;
			tm->tm_zone = NULL;
		}
	}
	else {
		struct headline hl;
		char headline[LINESIZE];
		char pbuf[LINESIZE];

		if (debug && use_hl_date == 0)
			warnx("invalid date: \"%s\"", date ? date : "<null>");

		/*
		 * The headline is written locally so failures here
		 * should be seen (i.e., not conditional on 'debug').
		 */
		tm->tm_isdst = -1; /* let mktime(3) determine tm_isdst */
		headline[0] = '\0';
		(void)mail_readline(setinput(mp), headline, sizeof(headline));
		parse(headline, &hl, pbuf);
		if (hl.l_date == NULL)
			warnx("invalid headline: `%s'", headline);

		else if (hl_date_to_tm(hl.l_date, tm) == NULL ||
		    mktime(tm) == -1)
			warnx("invalid headline date: `%s'", hl.l_date);
	}
}

/*
 * Get the sender's address for display.  Let nameof() do this.
 */
static const char *
addrof(struct message *mp)
{
	if (mp == NULL)
		return NULL;

	return nameof(mp, 0);
}

/************************************************************************
 * The 'address' syntax - from RFC 2822:
 *
 * specials        =       "(" / ")" /     ; Special characters used in
 *                         "<" / ">" /     ;  other parts of the syntax
 *                         "[" / "]" /
 *                         ":" / ";" /
 *                         "@" / "\" /
 *                         "," / "." /
 *                         DQUOTE
 * qtext           =       NO-WS-CTL /     ; Non white space controls
 *                         %d33 /          ; The rest of the US-ASCII
 *                         %d35-91 /       ;  characters not including "\"
 *                         %d93-126        ;  or the quote character
 * qcontent        =       qtext / quoted-pair
 * quoted-string   =       [CFWS]
 *                         DQUOTE *([FWS] qcontent) [FWS] DQUOTE
 *                         [CFWS]
 * atext           =       ALPHA / DIGIT / ; Any character except controls,
 *                         "!" / "#" /     ;  SP, and specials.
 *                         "$" / "%" /     ;  Used for atoms
 *                         "&" / "'" /
 *                         "*" / "+" /
 *                         "-" / "/" /
 *                         "=" / "?" /
 *                         "^" / "_" /
 *                         "`" / "{" /
 *                         "|" / "}" /
 *                         "~"
 * atom            =       [CFWS] 1*atext [CFWS]
 * word            =       atom / quoted-string
 * phrase          =       1*word / obs-phrase
 * display-name    =       phrase
 * dtext           =       NO-WS-CTL /     ; Non white space controls
 *                         %d33-90 /       ; The rest of the US-ASCII
 *                         %d94-126        ;  characters not including "[",
 *                                         ;  "]", or "\"
 * dcontent        =       dtext / quoted-pair
 * domain-literal  =       [CFWS] "[" *([FWS] dcontent) [FWS] "]" [CFWS]
 * domain          =       dot-atom / domain-literal / obs-domain
 * local-part      =       dot-atom / quoted-string / obs-local-part
 * addr-spec       =       local-part "@" domain
 * angle-addr      =       [CFWS] "<" addr-spec ">" [CFWS] / obs-angle-addr
 * name-addr       =       [display-name] angle-addr
 * mailbox         =       name-addr / addr-spec
 * mailbox-list    =       (mailbox *("," mailbox)) / obs-mbox-list
 * group           =       display-name ":" [mailbox-list / CFWS] ";"
 *                         [CFWS]
 * address         =       mailbox / group
 ************************************************************************/
static char *
get_display_name(char *name)
{
	char nbuf[LINESIZE];
	const char *p;
	char *q;
	char *qend;
	char *lastq;
	int quoted;

	if (name == NULL)
		return NULL;

	q = nbuf;
	lastq = nbuf;
	qend = nbuf + sizeof(nbuf) - 1;	/* reserve space for '\0' */
	quoted = 0;
	for (p = skip_WSP(name); *p != '\0'; p++) {
		DPRINTF(("get_display_name: %s\n", p));
		switch (*p) {
		case '"':	/* quoted-string */
			q = nbuf;
			p = snarf_quote(&q, qend, p);
			if (!quoted)
				lastq = q;
			quoted = 1;
			break;

		case ':':	/* group */
		case '<':	/* angle-address */
			if (lastq == nbuf)
				return NULL;
			*lastq = '\0';	/* NULL termination */
			return savestr(nbuf);

		case '(':	/* comment - skip it! */
			p = snarf_comment(NULL, NULL, p);
			break;

		default:
			if (!quoted && q < qend) {
				*q++ = *p;
				if (!is_WSP(*p)
				    /* && !is_specials((unsigned char)*p) */ )
					lastq = q;
			}
			break;
		}
	}
	return NULL;	/* no group or angle-address */
}

/*
 * See RFC 2822 sec 3.4 and 3.6.2.
 */
static const char *
userof(struct message *mp)
{
	char *sender;
	char *dispname;

	if (mp == NULL)
		return NULL;

	if ((sender = hfield("from", mp)) != NULL ||
	    (sender = hfield("sender", mp)) != NULL)
		/*
		 * Try to get the display-name.  If one doesn't exist,
		 * then the best we can hope for is that the user's
		 * name is in the comments.
		 */
		if ((dispname = get_display_name(sender)) != NULL ||
		    (dispname = get_comments(sender)) != NULL)
			return dispname;
	return NULL;
}

/*
 * Grab the subject line.
 */
static const char *
subjof(struct message *mp)
{
	const char *subj;

	if (mp == NULL)
		return NULL;

	if ((subj = hfield("subject", mp)) == NULL)
		subj = hfield("subj", mp);
	return subj;
}

/*
 * Protect a string against strftime() conversion.
 */
static const char*
protect(const char *str)
{
	char *p, *q;
	size_t size;

	if (str == NULL || (size = strlen(str)) == 0)
		return str;

	p = salloc(2 * size + 1);
	for (q = p; *str; str++) {
		*q = *str;
		if (*q++ == '%')
			*q++ = '%';
	}
	*q = '\0';
	return p;
}

static char *
preformat(struct tm *tm, const char *oldfmt, struct message *mp, int use_hl_date)
{
	const char *subj;
	const char *addr;
	const char *user;
	const char *p;
	char *q;
	char *newfmt;
	size_t fmtsize;

	if (mp != NULL && (mp->m_flag & MDELETED) != 0)
		mp = NULL; /* deleted mail shouldn't show up! */

	subj = protect(subjof(mp));
	addr = protect(addrof(mp));
	user = protect(userof(mp));
	dateof(tm, mp, use_hl_date);
	fmtsize = LINESIZE;
	newfmt = ecalloc(1, fmtsize); /* so we can realloc() in check_bufsize() */
	q = newfmt;
	p = oldfmt;
	while (*p) {
		if (*p == '%') {
			const char *fp;
			fp = subformat(&p, mp, addr, user, subj, tm->tm_isdst);
			if (fp) {
				size_t len;
				len = strlen(fp);
				check_bufsize(&newfmt, &fmtsize, &q, len);
				(void)strcpy(q, fp);
				q += len;
				continue;
			}
		}
		check_bufsize(&newfmt, &fmtsize, &q, 1);
		*q++ = *p++;
	}
	*q = '\0';

	return newfmt;
}

/*
 * If a format string begins with the USE_HL_DATE string, smsgprintf
 * will use the headerline date rather than trying to extract the date
 * from the Date field.
 *
 * Note: If a 'valid' date cannot be extracted from the Date field,
 * then the headline date is used.
 */
#define USE_HL_DATE "%??"

PUBLIC char *
smsgprintf(const char *fmtstr, struct message *mp)
{
	struct tm tm;
	int use_hl_date;
	char *newfmt;
	char *buf;
	size_t bufsize;

	if (strncmp(fmtstr, USE_HL_DATE, sizeof(USE_HL_DATE) - 1) != 0)
		use_hl_date = 0;
	else {
		use_hl_date = 1;
		fmtstr += sizeof(USE_HL_DATE) - 1;
	}
	bufsize = LINESIZE;
	buf = salloc(bufsize);
	newfmt = preformat(&tm, fmtstr, mp, use_hl_date);
	(void)strftime(buf, bufsize, newfmt, &tm);
	free(newfmt);	/* preformat() uses malloc()/realloc() */
	return buf;
}


PUBLIC void
fmsgprintf(FILE *fo, const char *fmtstr, struct message *mp)
{
	char *buf;

	buf = smsgprintf(fmtstr, mp);
	(void)fprintf(fo, "%s\n", buf);	/* XXX - add the newline here? */
}
