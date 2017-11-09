/*	$NetBSD: list.c,v 1.28 2017/11/09 20:27:50 christos Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
#if 0
static char sccsid[] = "@(#)list.c	8.4 (Berkeley) 5/1/95";
#else
__RCSID("$NetBSD: list.c,v 1.28 2017/11/09 20:27:50 christos Exp $");
#endif
#endif /* not lint */

#include <assert.h>
#include <regex.h>
#include <util.h>

#include "rcv.h"
#include "extern.h"
#include "format.h"
#include "thread.h"
#include "mime.h"

/*
 * Mail -- a mail program
 *
 * Message list handling.
 */

/*
 * Token values returned by the scanner used for argument lists.
 * Also, sizes of scanner-related things.
 */
enum token_e {
	TEOL,			/* End of the command line */
	TNUMBER,		/* A message number or range of numbers */
	TDASH,			/* A simple dash */
	TSTRING,		/* A string (possibly containing '-') */
	TDOT,			/* A "." */
	TUP,			/* An "^" */
	TDOLLAR,		/* A "$" */
	TSTAR,			/* A "*" */
	TOPEN,			/* An '(' */
	TCLOSE,			/* A ')' */
	TPLUS,			/* A '+' */
	TAND,			/* A '&' */
	TOR,			/* A '|' */
	TXOR,			/* A logical '^' */
	TNOT,			/* A '!' */
	TERROR			/* A lexical error */
};

#define	REGDEP		2		/* Maximum regret depth. */
#define	STRINGLEN	1024		/* Maximum length of string token */

static int	lexnumber;		/* Number of TNUMBER from scan() */
static char	lexstring[STRINGLEN];	/* String from TSTRING, scan() */
static int	regretp;		/* Pointer to TOS of regret tokens */
static int	regretstack[REGDEP];	/* Stack of regretted tokens */
static char	*string_stack[REGDEP];	/* Stack of regretted strings */
static int	numberstack[REGDEP];	/* Stack of regretted numbers */

/*
 * Scan out the list of string arguments, shell style
 * for a RAWLIST.
 */
PUBLIC int
getrawlist(const char line[], char **argv, int argc)
{
	char c, *cp2, quotec;
	const char *cp;
	int argn;
	char linebuf[LINESIZE];

	argn = 0;
	cp = line;
	for (;;) {
		cp = skip_WSP(cp);
		if (*cp == '\0')
			break;
		if (argn >= argc - 1) {
			(void)printf(
			"Too many elements in the list; excess discarded.\n");
			break;
		}
		cp2 = linebuf;
		quotec = '\0';
		while ((c = *cp) != '\0') {
			cp++;
			if (quotec != '\0') {
				if (c == quotec)
					quotec = '\0';
				else if (quotec != '\'' && c == '\\')
					switch (c = *cp++) {
					case '\0':
						*cp2++ = '\\';
						cp--;
						break;
					case '0': case '1': case '2': case '3':
					case '4': case '5': case '6': case '7':
						c -= '0';
						if (*cp >= '0' && *cp <= '7')
							c = c * 8 + *cp++ - '0';
						if (*cp >= '0' && *cp <= '7')
							c = c * 8 + *cp++ - '0';
						*cp2++ = c;
						break;
					case 'b':
						*cp2++ = '\b';
						break;
					case 'f':
						*cp2++ = '\f';
						break;
					case 'n':
						*cp2++ = '\n';
						break;
					case 'r':
						*cp2++ = '\r';
						break;
					case 't':
						*cp2++ = '\t';
						break;
					case 'v':
						*cp2++ = '\v';
						break;
					default:
						*cp2++ = c;
					}
				else if (c == '^') {
					c = *cp++;
					if (c == '?')
						*cp2++ = '\177';
					/* null doesn't show up anyway */
					else if ((c >= 'A' && c <= '_') ||
						 (c >= 'a' && c <= 'z'))
						*cp2++ = c & 037;
					else {
						*cp2++ = '^';
						cp--;
					}
				} else
					*cp2++ = c;
			} else if (c == '"' || c == '\'')
				quotec = c;
			else if (is_WSP(c))
				break;
			else
				*cp2++ = c;
		}
		*cp2 = '\0';
		argv[argn++] = savestr(linebuf);
	}
	argv[argn] = NULL;
	return argn;
}

/*
 * Mark all messages that the user wanted from the command
 * line in the message structure.  Return 0 on success, -1
 * on error.
 */

/*
 * Bit values for colon modifiers.
 */
#define	CMBOX		0x001		/* Unread messages */
#define	CMDELETED	0x002		/* Deleted messages */
#define	CMMODIFY	0x004		/* Unread messages */
#define	CMNEW		0x008		/* New messages */
#define	CMOLD		0x010		/* Old messages */
#define	CMPRESERVE	0x020		/* Unread messages */
#define	CMREAD		0x040		/* Read messages */
#define	CMSAVED		0x080		/* Saved messages */
#define	CMTAGGED	0x100		/* Tagged messages */
#define	CMUNREAD	0x200		/* Unread messages */
#define CMNEGATE	0x400		/* Negate the match */
#define CMMASK		0x7ff		/* Mask the valid bits */

/*
 * The following table describes the letters which can follow
 * the colon and gives the corresponding modifier bit.
 */

static const struct coltab {
	char	co_char;		/* What to find past : */
	int	co_bit;			/* Associated modifier bit */
	int	co_mask;		/* m_status bits to mask */
	int	co_equal;		/* ... must equal this */
} coltab[] = {
	{ '!',		CMNEGATE,	0,		0 },
	{ 'd',		CMDELETED,	MDELETED,	MDELETED },
	{ 'e',		CMMODIFY,	MMODIFY,	MMODIFY },
	{ 'm',		CMBOX,		MBOX,		MBOX },
	{ 'n',		CMNEW,		MNEW,		MNEW },
	{ 'o',		CMOLD,		MNEW,		0 },
	{ 'p',		CMPRESERVE,	MPRESERVE,	MPRESERVE },
	{ 'r',		CMREAD,		MREAD,		MREAD },
	{ 's',		CMSAVED,	MSAVED,		MSAVED },
	{ 't',		CMTAGGED,	MTAGGED,	MTAGGED },
	{ 'u',		CMUNREAD,	MREAD|MNEW,	0 },
	{ 0,		0,		0,		0 }
};

static	int	lastcolmod;

static int
ignore_message(int m_flag, int colmod)
{
	int ignore_msg;
	const struct coltab *colp;

	ignore_msg = !(colmod & CMNEGATE);
	colmod &= (~CMNEGATE & CMMASK);

	for (colp = &coltab[0]; colp->co_char; colp++)
		if (colp->co_bit & colmod &&
		    (m_flag & colp->co_mask) == colp->co_equal)
				return !ignore_msg;
	return ignore_msg;
}

/*
 * Turn the character after a colon modifier into a bit
 * value.
 */
static int
evalcol(int col)
{
	const struct coltab *colp;

	if (col == 0)
		return lastcolmod;
	for (colp = &coltab[0]; colp->co_char; colp++)
		if (colp->co_char == col)
			return colp->co_bit;
	return 0;
}

static int
get_colmod(int colmod, char *cp)
{
	if ((cp[0] == '\0') ||
	    (cp[0] == '!' && cp[1] == '\0'))
		colmod |= lastcolmod;

	for (/*EMPTY*/; *cp; cp++) {
		int colresult;
		if ((colresult = evalcol(*cp)) == 0) {
			(void)printf("Unknown colon modifier \"%s\"\n", lexstring);
			return -1;
		}
		if (colresult == CMNEGATE)
			colmod ^= CMNEGATE;
		else
			colmod |= colresult;
	}
	return colmod;
}

static int
syntax_error(const char *msg)
{
	(void)printf("Syntax error: %s\n", msg);
	return -1;
}

/*
 * scan out a single lexical item and return its token number,
 * updating the string pointer passed **p.  Also, store the value
 * of the number or string scanned in lexnumber or lexstring as
 * appropriate.  In any event, store the scanned `thing' in lexstring.
 */
static enum token_e
scan(char **sp)
{
	static const struct lex {
		char	l_char;
		enum token_e l_token;
	} singles[] = {
		{ '$',	TDOLLAR },
		{ '.',	TDOT },
		{ '^',	TUP },
		{ '*',	TSTAR },
		{ '-',	TDASH },
		{ '+',	TPLUS },
		{ '(',	TOPEN },
		{ ')',	TCLOSE },
		{ '&',	TAND },
		{ '|',	TOR },
		{ '!',	TNOT },
		{ 0,	0 }
	};
	const struct lex *lp;
	char *cp, *cp2;
	int c;
	int quotec;

	if (regretp >= 0) {
		(void)strcpy(lexstring, string_stack[regretp]);
		lexnumber = numberstack[regretp];
		return regretstack[regretp--];
	}
	cp = *sp;
	cp2 = lexstring;
	lexstring[0] = '\0';

	/*
	 * strip away leading white space.
	 */
	cp = skip_WSP(cp);

	/*
	 * If no characters remain, we are at end of line,
	 * so report that.
	 */
	if (*cp == '\0') {
		*sp = cp;
		return TEOL;
	}

	/*
	 * If the leading character is a digit, scan
	 * the number and convert it on the fly.
	 * Return TNUMBER when done.
	 */
	c = (unsigned char)*cp++;
	if (isdigit(c)) {
		lexnumber = 0;
		while (isdigit(c)) {
			lexnumber = lexnumber * 10 + c - '0';
			*cp2++ = c;
			c = (unsigned char)*cp++;
		}
		*cp2 = '\0';
		*sp = --cp;
		return TNUMBER;
	}

	/*
	 * Check for single character tokens; return such
	 * if found.
	 */
	for (lp = &singles[0]; lp->l_char != 0; lp++)
		if (c == lp->l_char) {
			lexstring[0] = c;
			lexstring[1] = '\0';
			*sp = cp;
			return lp->l_token;
		}

	/*
	 * We've got a string!  Copy all the characters
	 * of the string into lexstring, until we see
	 * a null, space, or tab.
	 * Respect quoting and quoted pairs.
	 */
	quotec = 0;
	while (c != '\0') {
		if (c == quotec) {
			quotec = 0;
			c = *cp++;
			continue;
		}
		if (quotec) {
			if (c == '\\' && (*cp == quotec || *cp == '\\'))
				c = *cp++;
		}
		else {
			switch (c) {
			case '\'':
			case '"':
				quotec = c;
				c = *cp++;
				continue;
			case ' ':
			case '\t':
				c = '\0';	/* end of token! */
				continue;
			default:
				break;
			}
		}
		if (cp2 - lexstring < STRINGLEN - 1)
			*cp2++ = c;
		c = *cp++;
	}
	if (quotec && c == 0) {
		(void)fprintf(stderr, "Missing %c\n", quotec);
		return TERROR;
	}
	*sp = --cp;
	*cp2 = '\0';
	return TSTRING;
}

/*
 * Unscan the named token by pushing it onto the regret stack.
 */
static void
regret(int token)
{
	if (++regretp >= REGDEP)
		errx(EXIT_FAILURE, "Too many regrets");
	regretstack[regretp] = token;
	lexstring[sizeof(lexstring) - 1] = '\0';
	string_stack[regretp] = savestr(lexstring);
	numberstack[regretp] = lexnumber;
}

/*
 * Reset all the scanner global variables.
 */
static void
scaninit(void)
{
	regretp = -1;
}

#define DELIM " \t,"  /* list of string delimiters */
static int
is_substr(const char *big, const char *little)
{
	const char *cp;
	if ((cp = strstr(big, little)) == NULL)
		return 0;

	return strchr(DELIM, cp[strlen(little)]) != 0 &&
	    (cp == big || strchr(DELIM, cp[-1]) != 0);
}
#undef DELIM


/*
 * Look for (compiled regex) pattern in a line.
 * Returns:
 *	1 if match found.
 *	0 if no match found.
 *	-1 on error
 */
static int
regexcmp(void *pattern, char *line, size_t len)
{
	regmatch_t pmatch[1];
	regmatch_t *pmp;
	int eflags;
	int rval;
	regex_t *preg;

	preg = pattern;

	if (line == NULL)
		return 0;

	if (len == 0) {
		pmp = NULL;
		eflags = 0;
	}
	else {
		pmatch[0].rm_so = 0;
		pmatch[0].rm_eo = line[len - 1] == '\n' ? len - 1 : len;
		pmp = pmatch;
		eflags = REG_STARTEND;
	}

	switch ((rval = regexec(preg, line, 0, pmp, eflags))) {
	case 0:
	case REG_NOMATCH:
		return rval == 0;

	default: {
		char errbuf[LINESIZE];
		(void)regerror(rval, preg, errbuf, sizeof(errbuf));
		(void)printf("regexec failed: '%s': %s\n", line, errbuf);
		return -1;
	}}
}

/*
 * Look for (string) pattern in line.
 * Return 1 if match found.
 */
static int
substrcmp(void *pattern, char *line, size_t len)
{
	char *substr;
	substr = pattern;

	if (line == NULL)
		return 0;

	if (len) {
		if (line[len - 1] == '\n') {
			line[len - 1] = '\0';
		}
		else {
			char *cp;
			cp = salloc(len + 1);
			(void)strlcpy(cp, line, len + 1);
			line = cp;
		}
	}
	return strcasestr(line, substr) != NULL;
}

/*
 * Look for NULL line.  Used to find non-existent fields.
 * Return 1 if match found.
 */
static int
hasfieldcmp(void *pattern __unused, char *line, size_t len __unused)
{
#ifdef __lint__
	pattern = pattern;
	len = len;
#endif
	return line != NULL;
}

static regex_t preg;
/*
 * Determine the compare function and its argument based on the
 * "regex-search" variable.
 */
static int (*
get_cmpfn(void **pattern, char *str)
)(void *, char *, size_t)
{
	char *val;
	int cflags;
	int e;

	if (*str == 0) {
		*pattern = NULL;
		return hasfieldcmp;
	}

	if ((val = value(ENAME_REGEX_SEARCH)) != NULL) {
		cflags = REG_NOSUB;
		val = skip_WSP(val);
		if (*val) {
			if (is_substr(val, "icase"))
				cflags |= REG_ICASE;
			if (is_substr(val, "extended"))
				cflags |= REG_EXTENDED;
			/*
			 * NOTE: regcomp() will fail if "nospec" and
			 * "extended" are used together.
			 */
			if (is_substr(val, "nospec"))
				cflags |= REG_NOSPEC;
		}
		if ((e = regcomp(&preg, str, cflags)) != 0) {
			char errbuf[LINESIZE];
			(void)regerror(e, &preg, errbuf, sizeof(errbuf));
			(void)printf("regcomp failed: '%s': %s\n", str, errbuf);
			return NULL;
		}
		*pattern = &preg;
		return regexcmp;
	}

	*pattern = str;
	return substrcmp;
}

/*
 * Free any memory allocated by get_cmpfn()
 */
static void
free_cmparg(void *pattern)
{
	if (pattern == &preg)
		regfree(&preg);
}

/*
 * Check the message body for the pattern.
 */
static int
matchbody(int (*cmpfn)(void *, char *, size_t),
    void *pattern, struct message *mp, char const *fieldname __unused)
{
	FILE *fp;
	char *line;
	size_t len;
	int gotmatch;

#ifdef __lint__
	fieldname = fieldname;
#endif
	/*
	 * Get a temporary file.
	 */
	{
		char *tempname;
		int fd;

		(void)sasprintf(&tempname, "%s/mail.RbXXXXXXXXXX", tmpdir);
		fp = NULL;
		if ((fd = mkstemp(tempname)) != -1) {
			(void)unlink(tempname);
			if ((fp = Fdopen(fd, "wef+")) == NULL)
				(void)close(fd);
		}
		if (fp == NULL) {
			warn("%s", tempname);
			return -1;
		}
	}

	/*
	 * Pump the (decoded) message body into the temp file.
	 */
	{
#ifdef MIME_SUPPORT
		struct mime_info *mip;
		int retval;

		mip = value(ENAME_MIME_DECODE_MSG) ? mime_decode_open(mp)
		    : NULL;

		retval = mime_sendmessage(mp, fp, ignoreall, NULL, mip);
		mime_decode_close(mip);
		if (retval == -1)
#else
		if (sendmessage(mp, fp, ignoreall, NULL, NULL) == -1)
#endif
		{
			warn("matchbody: mesg=%d", get_msgnum(mp));
			return -1;
		}
	}
	/*
	 * XXX - should we read the entire body into a buffer so we
	 * can search across lines?
	 */
	rewind(fp);
	gotmatch = 0;
	while ((line = fgetln(fp, &len)) != NULL && len > 0) {
		gotmatch = cmpfn(pattern, line, len);
		if (gotmatch)
			break;
	}
	(void)Fclose(fp);

	return gotmatch;
}

/*
 * Check the "To:", "Cc:", and "Bcc" fields for the pattern.
 */
static int
matchto(int (*cmpfn)(void *, char *, size_t),
    void *pattern, struct message *mp, char const *fieldname __unused)
{
	static const char *to_fields[] = { "to", "cc", "bcc", 0 };
	const char **to;
	int gotmatch;

#ifdef __lint__
	fieldname = fieldname;
#endif
	gotmatch = 0;
	for (to = to_fields; *to; to++) {
		char *field;
		field = hfield(*to, mp);
		gotmatch = cmpfn(pattern, field, 0);
		if (gotmatch)
			break;
	}
	return gotmatch;
}

/*
 * Check a field for the pattern.
 */
static int
matchfield(int (*cmpfn)(void *, char *, size_t),
    void *pattern, struct message *mp, char const *fieldname)
{
	char *field;

#ifdef __lint__
	fieldname = fieldname;
#endif
	field = hfield(fieldname, mp);
	return cmpfn(pattern, field, 0);
}

/*
 * Check the headline for the pattern.
 */
static int
matchfrom(int (*cmpfn)(void *, char *, size_t),
    void *pattern, struct message *mp, char const *fieldname __unused)
{
	char headline[LINESIZE];
	char *field;

#ifdef __lint__
	fieldname = fieldname;
#endif
	(void)readline(setinput(mp), headline, (int)sizeof(headline), 0);
	field = savestr(headline);
	if (strncmp(field, "From ", 5) != 0)
		return 1;

	return cmpfn(pattern, field + 5, 0);
}

/*
 * Check the sender for the pattern.
 */
static int
matchsender(int (*cmpfn)(void *, char *, size_t),
    void *pattern, struct message *mp, char const *fieldname __unused)
{
	char *field;

#ifdef __lint__
	fieldname = fieldname;
#endif
	field = nameof(mp, 0);
	return cmpfn(pattern, field, 0);
}

/*
 * Interpret 'str' and check each message (1 thru 'msgCount') for a match.
 * The 'str' has the format: [/[[x]:]y with the following meanings:
 *
 * y	 pattern 'y' is compared against the senders address.
 * /y	 pattern 'y' is compared with the subject field.  If 'y' is empty,
 *       the last search 'str' is used.
 * /:y	 pattern 'y' is compared with the subject field.
 * /x:y	 pattern 'y' is compared with the specified header field 'x' or
 *       the message body if 'x' == "body".
 *
 * The last two forms require "searchheaders" to be defined.
 */
static int
match_string(int *markarray, char *str, int msgCount)
{
	int i;
	int rval;
	int (*matchfn)(int (*)(void *, char *, size_t),
	    void *, struct message *, char const *);
	int (*cmpfn)(void *, char *, size_t);
	void *cmparg;
	char const *fieldname;

	if (*str != '/') {
		matchfn = matchsender;
		fieldname = NULL;
	}
	else {
		static char lastscan[STRINGLEN];
		char *cp;

		str++;
		if (*str == '\0')
			str = lastscan;
		else
			(void)strlcpy(lastscan, str, sizeof(lastscan));

		if (value(ENAME_SEARCHHEADERS) == NULL ||
		    (cp = strchr(str, ':')) == NULL) {
			matchfn = matchfield;
			fieldname = "subject";
		/*	str = str; */
		}
		else {
			static const struct matchtbl_s {
				char const *key;
				size_t len;
				char const *fieldname;
				int (*matchfn)(int (*)(void *, char *, size_t),
				    void *, struct message *, char const *);
			} matchtbl[] = {
				#define	X(a)	a,	sizeof(a) - 1
				#define	X_NULL	NULL,	0
				{ X(":"),	"subject",	matchfield },
				{ X("body:"),	NULL,		matchbody },
				{ X("from:"),	NULL,		matchfrom },
				{ X("to:"),	NULL,		matchto },
				{ X_NULL,	NULL,		matchfield }
				#undef X_NULL
				#undef X
			};
			const struct matchtbl_s *mtp;
			size_t len;
			/*
			 * Check for special cases!
			 * These checks are case sensitive so the true fields
			 * can be grabbed as mentioned in the manpage.
			 */
			cp++;
			len = cp - str;
			for (mtp = matchtbl; mtp->key; mtp++) {
				if (len == mtp->len &&
				    strncmp(str, mtp->key, len) == 0)
					break;
			}
			matchfn = mtp->matchfn;
			if (mtp->key)
				fieldname = mtp->fieldname;
			else {
				char *p;
				p = salloc(len);
				(void)strlcpy(p, str, len);
				fieldname = p;
			}
			str = cp;
		}
	}

	cmpfn = get_cmpfn(&cmparg, str);
	if (cmpfn == NULL)
		return -1;

	rval = 0;
	for (i = 1; i <= msgCount; i++) {
		struct message *mp;
		mp = get_message(i);
		rval = matchfn(cmpfn, cmparg, mp, fieldname);
		if (rval == -1)
			break;
		if (rval)
			markarray[i - 1] = 1;
		rval = 0;
	}

	free_cmparg(cmparg);	/* free any memory allocated by get_cmpfn() */

	return rval;
}


/*
 * Return the message number corresponding to the passed meta character.
 */
static int
metamess(int meta, int f)
{
	int c, m;
	struct message *mp;

	c = meta;
	switch (c) {
	case '^':
		/*
		 * First 'good' message left.
		 */
		for (mp = get_message(1); mp; mp = next_message(mp))
			if ((mp->m_flag & MDELETED) == f)
				return get_msgnum(mp);
		(void)printf("No applicable messages\n");
		return -1;

	case '$':
		/*
		 * Last 'good message left.
		 */
		for (mp = get_message(get_msgCount()); mp; mp = prev_message(mp))
			if ((mp->m_flag & MDELETED) == f)
				return get_msgnum(mp);
		(void)printf("No applicable messages\n");
		return -1;

	case '.':
		/*
		 * Current message.
		 */
		if (dot == NULL) {
			(void)printf("No applicable messages\n");
			return -1;
		}
		m = get_msgnum(dot);
		if ((dot->m_flag & MDELETED) != f) {
			(void)printf("%d: Inappropriate message\n", m);
			return -1;
		}
		return m;

	default:
		(void)printf("Unknown metachar (%c)\n", c);
		return -1;
	}
}

/*
 * Check the passed message number for legality and proper flags.
 * If f is MDELETED, then either kind will do.  Otherwise, the message
 * has to be undeleted.
 */
static int
check(int mesg, int f)
{
	struct message *mp;

	if ((mp = get_message(mesg)) == NULL) {
		(void)printf("%d: Invalid message number\n", mesg);
		return -1;
	}
	if (f != MDELETED && (mp->m_flag & MDELETED) != 0) {
		(void)printf("%d: Inappropriate message\n", mesg);
		return -1;
	}
	return 0;
}


static int
markall_core(int *markarray, char **bufp, int f, int level)
{
	enum token_e tok;
	enum logic_op_e {
		LOP_AND,
		LOP_OR,
		LOP_XOR
	} logic_op;		/* binary logic operation */
	int logic_invert;	/* invert the result */
	int *tmparray;	/* temporarly array with result */
	int msgCount;	/* tmparray length and message count */
	int beg;	/* first value of a range */
	int colmod;	/* the colon-modifier for this group */
	int got_not;	/* for syntax checking of '!' */
	int got_one;	/* we have a message spec, valid or not */
	int got_bin;	/* we have a pending binary operation */
	int i;

	logic_op = LOP_OR;
	logic_invert = 0;
	colmod = 0;

	msgCount = get_msgCount();
	tmparray = csalloc((size_t)msgCount, sizeof(*tmparray));

	beg = 0;
	got_one = 0;
	got_not = 0;
	got_bin = 0;

	while ((tok = scan(bufp)) != TEOL) {
		if (tok == TERROR)
			return -1;

		/*
		 * Do some syntax checking.
		 */
		switch (tok) {
		case TDASH:
		case TPLUS:
		case TDOLLAR:
		case TUP:
		case TDOT:
		case TNUMBER:
			break;

		case TAND:
		case TOR:
		case TXOR:
			if (!got_one)
				return syntax_error("missing left operand");
			/*FALLTHROUGH*/
		default:
			if (beg)
				return syntax_error("end of range missing");
			break;
		}

		/*
		 * The main tok switch.
		 */
		switch (tok) {
			struct message *mp;

		case TERROR:	/* trapped above */
		case TEOL:
			assert(/*CONSTCOND*/0);
			break;

		case TUP:
			if (got_one) {	/* a possible logical xor */
				enum token_e t;
				t = scan(bufp); /* peek ahead */
				regret(t);
				lexstring[0] = '^';  /* restore lexstring */
				lexstring[1] = '\0';
				if (t != TDASH && t != TEOL && t != TCLOSE) {
					/* convert tok to TXOR and put
					 * it back on the stack so we
					 * can handle it consistently */
					tok = TXOR;
					regret(tok);
					continue;
				}
			}
			/* FALLTHROUGH */
		case TDOLLAR:
		case TDOT:
			lexnumber = metamess(lexstring[0], f);
			if (lexnumber == -1)
				return -1;
			/* FALLTHROUGH */
		case TNUMBER:
			if (check(lexnumber, f))
				return -1;
	number:
			got_one = 1;
			if (beg != 0) {
				if (lexnumber < beg) {
					(void)printf("invalid range: %d-%d\n", beg, lexnumber);
					return -1;
				}
				for (i = beg; i <= lexnumber; i++)
					tmparray[i - 1] = 1;

				beg = 0;
				break;
			}
			beg = lexnumber;	/* start of a range */
			tok = scan(bufp);
			if (tok == TDASH) {
				continue;
			}
			else {
				regret(tok);
				tmparray[beg - 1] = 1;
				beg = 0;
			}
			break;

		case TDASH:
			for (mp = prev_message(dot); mp; mp = prev_message(mp)) {
				if ((mp->m_flag & MDELETED) == 0)
					break;
			}
			if (mp == NULL) {
				(void)printf("Referencing before 1\n");
				return -1;
			}
			lexnumber = get_msgnum(mp);
			goto number;

		case TPLUS:
			for (mp = next_message(dot); mp; mp = next_message(mp)) {
				if ((mp->m_flag & MDELETED) == 0)
					break;
			}
			if (mp == NULL) {
				(void)printf("Referencing beyond EOF\n");
				return -1;
			}
			lexnumber = get_msgnum(mp);
			goto number;

		case TSTRING:
			if (lexstring[0] == ':') { /* colon modifier! */
				colmod = get_colmod(colmod, lexstring + 1);
				if (colmod == -1)
					return -1;
				continue;
			}
			got_one = 1;
			if (match_string(tmparray, lexstring, msgCount) == -1)
				return -1;
			break;

		case TSTAR:
			got_one = 1;
			for (i = 1; i <= msgCount; i++)
				tmparray[i - 1] = 1;
			break;


			/**************
			 * Parentheses.
			 */
		case TOPEN:
			if (markall_core(tmparray, bufp, f, level + 1) == -1)
				return -1;
			break;

		case TCLOSE:
			if (level == 0)
				return syntax_error("extra ')'");
			goto done;


			/*********************
			 * Logical operations.
			 */
		case TNOT:
			got_not = 1;
			logic_invert = ! logic_invert;
			continue;

			/*
			 * Binary operations.
			 */
		case TAND:
			if (got_not)
				return syntax_error("'!' precedes '&'");
			got_bin = 1;
			logic_op = LOP_AND;
			continue;

		case TOR:
			if (got_not)
				return syntax_error("'!' precedes '|'");
			got_bin = 1;
			logic_op = LOP_OR;
			continue;

		case TXOR:
			if (got_not)
				return syntax_error("'!' precedes logical '^'");
			got_bin = 1;
			logic_op = LOP_XOR;
			continue;
		}

		/*
		 * Do the logic operations.
		 */
		if (logic_invert)
			for (i = 0; i < msgCount; i++)
				tmparray[i] = ! tmparray[i];

		switch (logic_op) {
		case LOP_AND:
			for (i = 0; i < msgCount; i++)
				markarray[i] &= tmparray[i];
			break;

		case LOP_OR:
			for (i = 0; i < msgCount; i++)
				markarray[i] |= tmparray[i];
			break;

		case LOP_XOR:
			for (i = 0; i < msgCount; i++)
				markarray[i] ^= tmparray[i];
			break;
		}

		/*
		 * Clear the temporary array and reset the logic
		 * operations.
		 */
		for (i = 0; i < msgCount; i++)
			tmparray[i] = 0;

		logic_op = LOP_OR;
		logic_invert = 0;
		got_not = 0;
		got_bin = 0;
	}

	if (beg)
		return syntax_error("end of range missing");

	if (level)
		return syntax_error("missing ')'");

 done:
	if (got_not)
		return syntax_error("trailing '!'");

	if (got_bin)
		return syntax_error("missing right operand");

	if (colmod != 0) {
		/*
		 * If we have colon-modifiers but no messages
		 * specifiec, then assume '*' was given.
		 */
		if (got_one == 0)
			for (i = 1; i <= msgCount; i++)
				markarray[i - 1] = 1;

		for (i = 1; i <= msgCount; i++) {
			struct message *mp;
			if ((mp = get_message(i)) != NULL &&
			    ignore_message(mp->m_flag, colmod))
				markarray[i - 1] = 0;
		}
	}
	return 0;
}

static int
markall(char buf[], int f)
{
	int i;
	int mc;
	int *markarray;
	int msgCount;
	struct message *mp;

	msgCount = get_msgCount();

	/*
	 * Clear all the previous message marks.
	 */
	for (i = 1; i <= msgCount; i++)
		if ((mp = get_message(i)) != NULL)
			mp->m_flag &= ~MMARK;

	buf = skip_WSP(buf);
	if (*buf == '\0')
		return 0;

	scaninit();
	markarray = csalloc((size_t)msgCount, sizeof(*markarray));
	if (markall_core(markarray, &buf, f, 0) == -1)
		return -1;

	/*
	 * Transfer the markarray values to the messages.
	 */
	mc = 0;
	for (i = 1; i <= msgCount; i++) {
		if (markarray[i - 1] &&
		    (mp = get_message(i)) != NULL &&
		    (f == MDELETED || (mp->m_flag & MDELETED) == 0)) {
			mp->m_flag |= MMARK;
			mc++;
		}
	}

	if (mc == 0) {
		(void)printf("No applicable messages.\n");
		return -1;
	}
	return 0;
}

/*
 * Convert the user string of message numbers and
 * store the numbers into vector.
 *
 * Returns the count of messages picked up or -1 on error.
 */
PUBLIC int
getmsglist(char *buf, int *vector, int flags)
{
	int *ip;
	struct message *mp;

	if (get_msgCount() == 0) {
		*vector = 0;
		return 0;
	}
	if (markall(buf, flags) < 0)
		return -1;
	ip = vector;
	for (mp = get_message(1); mp; mp = next_message(mp))
		if (mp->m_flag & MMARK)
			*ip++ = get_msgnum(mp);
	*ip = 0;
	return (int)(ip - vector);
}

/*
 * Find the first message whose flags & m == f  and return
 * its message number.
 */
PUBLIC int
first(int f, int m)
{
	struct message *mp;

	if (get_msgCount() == 0)
		return 0;
	f &= MDELETED;
	m &= MDELETED;
	for (mp = dot; mp; mp = next_message(mp))
		if ((mp->m_flag & m) == f)
			return get_msgnum(mp);
	for (mp = prev_message(dot); mp; mp = prev_message(mp))
		if ((mp->m_flag & m) == f)
			return get_msgnum(mp);
	return 0;
}

/*
 * Show all headers without paging.  (-H flag)
 */
__dead
PUBLIC int
show_headers_and_exit(int flags)
{
	struct message *mp;

	/* We are exiting anyway, so use the default signal handler. */
	if (signal(SIGINT, SIG_DFL) == SIG_IGN)
		(void)signal(SIGINT, SIG_IGN);

	flags &= CMMASK;
	for (mp = get_message(1); mp; mp = next_message(mp))
		if (flags == 0 || !ignore_message(mp->m_flag, flags))
			printhead(get_msgnum(mp));

	exit(0);
	/* NOTREACHED */
}

/*
 * A hack so -H can have an optional modifier as -H[:flags].
 *
 * This depends a bit on the internals of getopt().  In particular,
 * for flags expecting an argument, argv[optind-1] must contain the
 * optarg and optarg must point to a substring of argv[optind-1] not a
 * copy of it.
 */
PUBLIC int
get_Hflag(char **argv)
{
	int flags;

	flags = ~CMMASK;

	if (optarg == NULL)  /* We had an error, just get the flags. */
		return flags;

	if (*optarg != ':' || optarg == argv[optind - 1]) {
		optind--;
		optreset = 1;
		if (optarg != argv[optind]) {
			static char temparg[LINESIZE];
			size_t optlen;
			size_t arglen;
			char *p;

			optlen = strlen(optarg);
			arglen = strlen(argv[optind]);
			p = argv[optind] + arglen - optlen;
			optlen = MIN(optlen, sizeof(temparg) - 1);
			temparg[0] = '-';
			(void)memmove(temparg + 1, p, optlen + 1);
			argv[optind] = temparg;
		}
	}
	else {
		flags = get_colmod(flags, optarg + 1);
	}
	return flags;
}
