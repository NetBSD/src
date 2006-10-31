/*	$NetBSD: format.c,v 1.1 2006/10/31 22:36:37 christos Exp $	*/

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
__RCSID("$NetBSD: format.c,v 1.1 2006/10/31 22:36:37 christos Exp $");
#endif /* not __lint__ */

#include <time.h>
#include <stdio.h>
#include <util.h>

#include "def.h"
#include "extern.h"
#include "format.h"
#include "glob.h"


#define DEBUG(a)

static void
check_bufsize(char **buf, size_t *bufsize, char **p, size_t cnt)
{
	char *q;
	if (*p + cnt < *buf + *bufsize)
		return;
	*bufsize *= 2;
	q = realloc(*buf, *bufsize);
	*p = q + (*p - *buf);
	*buf = q;
}

static const char *
sfmtoff(const char **fmtbeg, const char *fmtch, off_t off)
{
	char *newfmt;	/* pointer to new format string */
	size_t len;	/* space for "lld" including '\0' */
	len = fmtch - *fmtbeg + sizeof(PRId64);
	newfmt = salloc(len);
	(void)strlcpy(newfmt, *fmtbeg, len - sizeof(sizeof(PRId64)) + 1);
	(void)strlcat(newfmt, PRId64, len);
	*fmtbeg = fmtch + 1;
	{
		char *p;
		char *q;
		(void)easprintf(&p, newfmt, off);
		q = savestr(p);
		free(p);
		return q;
	}
}

static const char *
sfmtint(const char **fmtbeg, const char *fmtch, int num)
{
	char *newfmt;
	size_t len;

	len = fmtch - *fmtbeg + 2;	/* space for 'd' and '\0' */
	newfmt = salloc(len);
	(void)strlcpy(newfmt, *fmtbeg, len);
	newfmt[len-2] = 'd';		/* convert to printf format */
	
	*fmtbeg = fmtch + 1;
	{
		char *p;
		char *q;
		(void)easprintf(&p, newfmt, num);
		q = savestr(p);
		free(p);
		return q;
	}
}

static const char *
sfmtstr(const char **fmtbeg, const char *fmtch, const char *str)
{
	char *newfmt;
	size_t len;

	len = fmtch - *fmtbeg + 2;	/* space for 's' and '\0' */
	newfmt = salloc(len);
	(void)strlcpy(newfmt, *fmtbeg, len);
	newfmt[len-2] = 's';		/* convert to printf format */
	
	*fmtbeg = fmtch + 1;
	{
		char *p;
		char *q;
		(void)easprintf(&p, newfmt, str ? str : "");
		q = savestr(p);
		free(p);
		return q;
	}
}

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

		skin_it = fmtch[1] == '-' ? 1 : 0;
		len = q - fmtch - skin_it;
		p = salloc(len + 1);
		(void)strlcpy(p, fmtch + skin_it + 1, len);
		str = sfmtstr(fmtbeg, fmtch, hfield(p, mp));
		if (skin_it)
			str = skin(__UNCONST(str));
		*fmtbeg = q + 1;
		return str;
	}
	return NULL;
}

static const char *
sfmtflag(const char **fmtbeg, const char *fmtch, int flags)
{
	char disp[2];
	disp[0] = ' ';
	disp[1] = '\0';
	if (flags & MSAVED)
		disp[0] = '*';
	if (flags & MPRESERVE)
		disp[0] = 'P';
	if ((flags & (MREAD|MNEW)) == MNEW)
		disp[0] = 'N';
	if ((flags & (MREAD|MNEW)) == 0)
		disp[0] = 'U';
	if (flags & MBOX)
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

static const char *
subformat(const char **src, struct message *mp, const char *addr,
    const char *user, const char *subj, const char *gmtoff, const char *zone)
{
#define MP(a)	mp ? a : NULL
	const char *p;

	p = *src;
	if (p[1] == '%') {
		*src += 2;
		return "%%";
	}
	for (p = *src; *p && !isalpha((unsigned char)*p) && *p != '?'; p++)
		continue;

	switch (*p) {
	case '?':
		return MP(sfmtfield(src, p, mp));
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
 		return MP(sfmtflag(src, p, mp->m_flag));
	case 'Z':
		*src = p + 1;
		return zone;

	case 'f':
		return sfmtstr(src, p, addr);
	case 'i':
		if (mp == NULL && (mp = dot) == NULL)
			return NULL;
 		return sfmtint(src, p, (mp - message) + 1);
	case 'n':
		return sfmtstr(src, p, login_name(addr));
	case 'q':
		return sfmtstr(src, p, subj);
	case 't':
		return sfmtint(src, p, msgCount);
	case 'z':
		*src = p + 1;
		return gmtoff;
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
		DEBUG(("snarf_comment: %s\n", p));
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
		DEBUG(("snarf_comment: terminating: %s\n", *buf));
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
		DEBUG(("snarf_quote: %s\n", p));
		if (*p == '\\' && p[1] != '\0')
			p++;

		if (q < qend)
			*q++ = *p;
	}
	if (buf) {
		*q = '\0';
		DEBUG(("snarf_quote: terminating: %s\n", *buf));
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
	char nbuf[BUFSIZ];
	const char *p;
	char *qend;
	char *q;
	char *lastq;

	if (name == NULL)
		return(NULL);

	p = name;
	q = nbuf;
	lastq = nbuf;
	qend = nbuf + sizeof(nbuf) - 1;
	for (p = skip_white(name); *p != '\0'; p++) {
		DEBUG(("get_comments: %s\n", p));
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

static char *
my_strptime(const char *buf, const char *fmtstr, struct tm *tm)
{
	char *tail;
	char zone[4];

	zone[0] = '\0';
	tail = strptime(buf, fmtstr, tm);
	if (tail) {
		int len;
		if (sscanf(tail, " %3[A-Z] %n", zone, &len) == 1) {
			if (zone[0])
				tm->tm_zone = savestr(zone);
			tail += len;
		}
		tail = strptime(tail, " %Y ", tm);
	}
	return tail;
}

/*
 * Get the date and time info from the "Date:" line, parse it into a
 * tm structure as much as possible.
 *
 * Note: We return the gmtoff as a string as "-0000" has special
 * meaning.  See RFC 2822, sec 3.3.
 */
static const char *
dateof(struct tm *tm, struct message *mp, int use_hl_date)
{
	char *tail;
	char *gmtoff;
	const char *date;

	(void)memset(tm, 0, sizeof(*tm));

	if (mp == NULL) {	/* use local time */
		char buf[6];	/* space for "+0000" */
		int hour;
		int min;
		time_t now;
		tzset();
		(void)time(&now);
		(void)localtime_r(&now, tm);
		min = (tm->tm_gmtoff / 60) % 60;
		hour = tm->tm_gmtoff / 3600;
		if (hour > 12)
			hour = 24 - hour;
		(void)snprintf(buf, sizeof(buf), "%+03d%02d", hour, min);
		return savestr(buf);
	}
	gmtoff = NULL; 
	tail = NULL;
	/*
	 * See RFC 2822 sec 3.3 for date-time format used in
	 * the "Date:" field.
	 *
	 * Notes:
	 * 1) The 'day-of-week' and 'second' fields are optional so we
	 *    check 4 possibilities.  This could be optimized.
	 *
	 * 2) The timezone is frequently in a comment following the
	 *    zone offset.
	 *
	 * 3) The range for the time is 00:00 to 23:60 (for a leep
	 *    second), but I have seen this violated (e.g., Date: Tue,
	 *    24 Oct 2006 24:07:58 +0400) making strptime() fail.
	 *    Thus we fall back on the headline time which was written
	 *    locally when the message was received.  Of course, this
	 *    is not the same time as in the Date field.
	 */
	if (use_hl_date == 0 &&
	    (date = hfield("date", mp)) != NULL &&
	    ((tail = strptime(date, " %a, %d %b %Y %T ", tm)) != NULL ||
	     (tail = strptime(date,     " %d %b %Y %T ", tm)) != NULL ||
	     (tail = strptime(date, " %a, %d %b %Y %R ", tm)) != NULL ||
	     (tail = strptime(date,     " %d %b %Y %R ", tm)) != NULL)) {
		char *cp;
		if ((cp = strchr(tail, '(')) != NULL)
			tm->tm_zone = get_comments(cp);
		else
			tm->tm_zone = NULL;
		gmtoff = skin(tail);
	}
	else {
		/*
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
		 */
		struct headline hl;
		char headline[LINESIZE];
		char pbuf[BUFSIZ];
		
		headline[0] = '\0';
		date = headline;
		(void)mail_readline(setinput(mp), headline, LINESIZE);
		parse(headline, &hl, pbuf);
		if (hl.l_date != NULL &&
		    (tail = my_strptime(hl.l_date, " %a %b %d %T ", tm)) == NULL &&
		    (tail = my_strptime(hl.l_date, " %a %b %d %R ", tm)) == NULL) {
			warnx("dateof: cannot determine date: %s", hl.l_date);
		}
	}
	/* tail will be NULL here if the mail file is empty, so don't
	 * check it. */
	
	/* mark the zone and gmtoff info as invalid for strftime. */
	tm->tm_isdst = -1;
	
	return gmtoff;
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
 * The 'address' syntax - from rfc 2822:
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
	char nbuf[BUFSIZ];
	const char *p;
	char *q;
	char *qend;
	char *lastq;
	int quoted;

	if (name == NULL)
		return(NULL);

	q = nbuf;
	lastq = nbuf;
	qend = nbuf + sizeof(nbuf) - 1;	/* reserve space for '\0' */
	quoted = 0;
	for (p = skip_white(name); *p != '\0'; p++) {
		DEBUG(("get_display_name: %s\n", p));
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
			return(savestr(nbuf));

		case '(':	/* comment - skip it! */
			p = snarf_comment(NULL, NULL, p);
			break;

		default:
			if (!quoted && q < qend) {
				*q++ = *p;
				if (!isblank((unsigned char)*p)
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

static char *
preformat(struct tm *tm, const char *oldfmt, struct message *mp, int use_hl_date)
{
	const char *gmtoff;
	const char *zone;
	const char *subj;
	const char *addr;
	const char *user;
	const char *p;
	char *q;
	char *newfmt;
	size_t fmtsize;

	if (mp != NULL && (mp->m_flag & MDELETED) != 0)
		mp = NULL; /* deleted mail shouldn't show up! */

	subj = subjof(mp);
	addr = addrof(mp);
	user = userof(mp);
	gmtoff = dateof(tm, mp, use_hl_date);
	zone = tm->tm_zone;
	fmtsize = LINESIZE;
	newfmt = malloc(fmtsize); /* so we can realloc() in check_bufsize() */
	q = newfmt;
	p = oldfmt;
	while (*p) {
		if (*p == '%') {
			const char *fp;
			fp = subformat(&p, mp, addr, user, subj, gmtoff, zone);
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
