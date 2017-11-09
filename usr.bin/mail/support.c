/*	$NetBSD: support.c,v 1.25 2017/11/09 20:27:50 christos Exp $	*/

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
static char sccsid[] = "@(#)aux.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: support.c,v 1.25 2017/11/09 20:27:50 christos Exp $");
#endif
#endif /* not lint */

#include <stdarg.h>
#include <util.h>

#include "rcv.h"
#include "extern.h"
#include "mime.h"
#include "thread.h"

/*
 * Mail -- a mail program
 *
 * Auxiliary functions.
 */


/*
 * Return a pointer to a dynamic copy of the argument.
 */
PUBLIC char *
savestr(const char *str)
{
	char *new;
	size_t size = strlen(str) + 1;

	if ((new = salloc(size)) != NULL)
		(void)memmove(new, str, size);
	return new;
}

/*
 * Make a copy of new argument incorporating old one.
 */
static char *
save2str(char *str, char *old)
{
	char *new;
	size_t newsize = strlen(str) + 1;
	size_t oldsize = old ? strlen(old) + 1 : 0;

	if ((new = salloc(newsize + oldsize)) != NULL) {
		if (oldsize) {
			(void)memmove(new, old, oldsize);
			new[oldsize - 1] = ' ';
		}
		(void)memmove(new + oldsize, str, newsize);
	}
	return new;
}

/*
 * Like asprintf(), but with result salloc-ed rather than malloc-ed.
 */
PUBLIC int
sasprintf(char **ret, const char *format, ...)
{
	int rval;
	char *p;
	va_list args;

	va_start(args, format);
	rval = evasprintf(&p, format, args);
	*ret = savestr(p);
	free(p);
	va_end(args);
	return rval;
}

struct set_m_flag_args_s {
	int and_bits;
	int xor_bits;
};
static int
set_m_flag_core(struct message *mp, void *v)
{
	struct set_m_flag_args_s *args;
	args = v;
	mp->m_flag &= args->and_bits;
	mp->m_flag ^= args->xor_bits;
	return 0;
}

PUBLIC struct message *
set_m_flag(int msgnum, int and_bits, int xor_bits)
{
	struct message *mp;
	struct set_m_flag_args_s args;
	mp = get_message(msgnum);
	args.and_bits = and_bits;
	args.xor_bits = xor_bits;
	(void)thread_recursion(mp, set_m_flag_core, &args);
	return mp;
}

/*
 * Touch the named message by setting its MTOUCH flag.
 * Touched messages have the effect of not being sent
 * back to the system mailbox on exit.
 */
PUBLIC void
touch(struct message *mp)
{

	mp->m_flag |= MTOUCH;
	if ((mp->m_flag & MREAD) == 0)
		mp->m_flag |= MREAD|MSTATUS;
}

/*
 * Test to see if the passed file name is a directory.
 * Return true if it is.
 */
PUBLIC int
isdir(const char name[])
{
	struct stat sbuf;

	if (stat(name, &sbuf) < 0)
		return 0;
	return S_ISDIR(sbuf.st_mode);
}

/*
 * Count the number of arguments in the given string raw list.
 */
PUBLIC int
argcount(char **argv)
{
	char **ap;

	for (ap = argv; *ap++ != NULL; /*EMPTY*/)
		continue;
	return (int)(ap - argv - 1);
}

/*
 * Check whether the passed line is a header line of
 * the desired breed.  Return the field body, or NULL.
 */
static char*
ishfield(const char linebuf[], char *colon, const char field[])
{
	char *cp = colon;

	*cp = 0;
	if (strcasecmp(linebuf, field) != 0) {
		*cp = ':';
		return NULL;
	}
	*cp = ':';
	for (cp++; is_WSP(*cp); cp++)
		continue;
	return cp;
}

/*
 * Return the next header field found in the given message.
 * Return >= 0 if something found, < 0 elsewise.
 * "colon" is set to point to the colon in the header.
 * Must deal with \ continuations & other such fraud.
 *
 * WARNING - do not call this outside hfield() or decoding will not
 *           get done.
 */
static int
gethfield(FILE *f, char linebuf[], int rem, char **colon)
{
	char line2[LINESIZE];
	char *cp, *cp2;
	int c;

	for (;;) {
		if (--rem < 0)
			return -1;
		if ((c = readline(f, linebuf, LINESIZE, 0)) <= 0)
			return -1;
		for (cp = linebuf;
		     isprint((unsigned char)*cp) && *cp != ' ' && *cp != ':';
		     cp++)
			continue;
		if (*cp != ':' || cp == linebuf)
			continue;
		/*
		 * I guess we got a headline.
		 * Handle wraparounding
		 */
		*colon = cp;
		cp = linebuf + c;
		for (;;) {
			while (--cp >= linebuf && is_WSP(*cp))
				continue;
			cp++;
			if (rem <= 0)
				break;
			(void)ungetc(c = getc(f), f);
			if (!is_WSP(c))
				break;
			if ((c = readline(f, line2, LINESIZE, 0)) < 0)
				break;
			rem--;
			cp2 = skip_WSP(line2);
			c -= (int)(cp2 - line2);
			if (cp + c >= linebuf + LINESIZE - 2)
				break;
			*cp++ = ' ';
			(void)memmove(cp, cp2, (size_t)c);
			cp += c;
		}
		*cp = 0;
		return rem;
	}
	/* NOTREACHED */
}

/*
 * Return the desired header line from the passed message
 * pointer (or NULL if the desired header field is not available).
 */
PUBLIC char *
hfield(const char field[], const struct message *mp)
{
	FILE *ibuf;
	char linebuf[LINESIZE];
	int lc;
	char *headerfield;
	char *colon, *oldhfield = NULL;
#ifdef MIME_SUPPORT
	int decode;

	decode = value(ENAME_MIME_DECODE_MSG) &&
	    value(ENAME_MIME_DECODE_HDR);
#endif

	ibuf = setinput(mp);
	if ((lc = (int)(mp->m_lines - 1)) < 0)
		return NULL;
	if (readline(ibuf, linebuf, LINESIZE, 0) < 0)
		return NULL;
	while (lc > 0) {
		if ((lc = gethfield(ibuf, linebuf, lc, &colon)) < 0)
			return oldhfield;
#ifdef MIME_SUPPORT
		if ((headerfield = ishfield(linebuf, colon, field)) != NULL) {
			char linebuf2[LINESIZE];
			if (decode)
				headerfield = mime_decode_hfield(linebuf2,
				    sizeof(linebuf2), linebuf, headerfield);
			oldhfield = save2str(headerfield, oldhfield);
		}
#else
		if ((headerfield = ishfield(linebuf, colon, field)) != NULL)
			oldhfield = save2str(headerfield, oldhfield);
#endif
	}
	return oldhfield;
}

/*
 * Copy a string, lowercasing it as we go.
 */
PUBLIC void
istrcpy(char *dest, const char *src)
{

	do {
		*dest++ = tolower((unsigned char)*src);
	} while (*src++ != 0);
}

/*
 * The following code deals with input stacking to do source
 * commands.  All but the current file pointer are saved on
 * the stack.
 */

static	int	ssp;			/* Top of file stack */
static struct sstack {
	FILE	*s_file;		/* File we were in. */
	int	s_cond;			/* Saved state of conditionals */
#ifdef NEW_CONDITIONAL
	struct cond_stack_s *s_cond_stack; /* Saved conditional stack */
#endif
	int	s_loading;		/* Loading .mailrc, etc. */
} sstack[NOFILE];

/*
 * Pushdown current input file and switch to a new one.
 * Set the global flag "sourcing" so that others will realize
 * that they are no longer reading from a tty (in all probability).
 */
PUBLIC int
source(void *v)
{
	char **arglist = v;
	FILE *fi;
	const char *cp;

	if ((cp = expand(*arglist)) == NULL)
		return 1;
	if ((fi = Fopen(cp, "ref")) == NULL) {
		warn("%s", cp);
		return 1;
	}
	if (ssp >= NOFILE - 1) {
		(void)printf("Too much \"sourcing\" going on.\n");
		(void)Fclose(fi);
		return 1;
	}
	sstack[ssp].s_file = input;
	sstack[ssp].s_cond = cond;
#ifdef NEW_CONDITIONAL
	sstack[ssp].s_cond_stack = cond_stack;
#endif
	sstack[ssp].s_loading = loading;
	ssp++;
	loading = 0;
	cond = CNONE;
	input = fi;
	sourcing++;
	return 0;
}

/*
 * Pop the current input back to the previous level.
 * Update the "sourcing" flag as appropriate.
 */
PUBLIC int
unstack(void)
{
	if (ssp <= 0) {
		(void)printf("\"Source\" stack over-pop.\n");
		sourcing = 0;
		return 1;
	}
	(void)Fclose(input);
	if (cond != CNONE || cond_stack != NULL)
		(void)printf("Unmatched \"if\"\n");
	ssp--;
	input = sstack[ssp].s_file;
	cond = sstack[ssp].s_cond;
#ifdef NEW_CONDITIONAL
	cond_stack = sstack[ssp].s_cond_stack;
#endif
	loading = sstack[ssp].s_loading;
	if (ssp == 0)
		sourcing = loading;
	return 0;
}

/*
 * Touch the indicated file.
 * This is nifty for the shell.
 */
PUBLIC void
alter(char *name)
{
	struct stat sb;
	struct timeval tv[2];

	if (stat(name, &sb))
		return;
	(void)gettimeofday(&tv[0], NULL);
	tv[0].tv_sec++;
	TIMESPEC_TO_TIMEVAL(&tv[1], &sb.st_mtimespec);
	(void)utimes(name, tv);
}

/*
 * Examine the passed line buffer and
 * return true if it is all blanks and tabs.
 */
PUBLIC int
blankline(char linebuf[])
{
	char *cp;

	for (cp = linebuf; *cp; cp++)
		if (!is_WSP(*cp))
			return 0;
	return 1;
}

/*
 * Start of a "comment".
 * Ignore it.
 */
static char *
skip_comment(char *cp)
{
	int nesting = 1;

	for (/*EMPTY*/; nesting > 0 && *cp; cp++) {
		switch (*cp) {
		case '\\':
			if (cp[1])
				cp++;
			break;
		case '(':
			nesting++;
			break;
		case ')':
			nesting--;
			break;
		}
	}
	return cp;
}

/*
 * Skin an arpa net address according to the RFC 822 interpretation
 * of "host-phrase."
 */
PUBLIC char *
skin(char *name)
{
	int c;
	char *cp, *cp2;
	char *bufend;
	int gotlt, lastsp;
	char nbuf[LINESIZE];

	if (name == NULL)
		return NULL;
	if (strchr(name, '(') == NULL && strchr(name, '<') == NULL
	    && strchr(name, ' ') == NULL)
		return name;
	gotlt = 0;
	lastsp = 0;
	bufend = nbuf;
	for (cp = name, cp2 = bufend; (c = *cp++) != '\0'; /*EMPTY*/) {
		switch (c) {
		case '(':
			cp = skip_comment(cp);
			lastsp = 0;
			break;

		case '"':
			/*
			 * Start of a "quoted-string".
			 * Copy it in its entirety.
			 */
			while ((c = *cp) != '\0') {
				cp++;
				if (c == '"')
					break;
				if (c != '\\')
					*cp2++ = c;
				else if ((c = *cp) != '\0') {
					*cp2++ = c;
					cp++;
				}
			}
			lastsp = 0;
			break;

		case ' ':
			if (cp[0] == 'a' && cp[1] == 't' && cp[2] == ' ')
				cp += 3, *cp2++ = '@';
			else
			if (cp[0] == '@' && cp[1] == ' ')
				cp += 2, *cp2++ = '@';
			else
				lastsp = 1;
			break;

		case '<':
			cp2 = bufend;
			gotlt++;
			lastsp = 0;
			break;

		case '>':
			if (gotlt) {
				gotlt = 0;
				while ((c = *cp) && c != ',') {
					cp++;
					if (c == '(')
						cp = skip_comment(cp);
					else if (c == '"')
						while ((c = *cp) != '\0') {
							cp++;
							if (c == '"')
								break;
							if (c == '\\' && *cp)
								cp++;
						}
				}
				lastsp = 0;
				break;
			}
			/* FALLTHROUGH */

		default:
			if (lastsp) {
				lastsp = 0;
				*cp2++ = ' ';
			}
			*cp2++ = c;
			if (c == ',' && !gotlt) {
				*cp2++ = ' ';
				for (/*EMPTY*/; *cp == ' '; cp++)
					continue;
				lastsp = 0;
				bufend = cp2;
			}
		}
	}
	*cp2 = 0;

	return savestr(nbuf);
}

/*
 * Fetch the sender's name from the passed message.
 * Reptype can be
 *	0 -- get sender's name for display purposes
 *	1 -- get sender's name for reply
 *	2 -- get sender's name for Reply
 */
static char *
name1(struct message *mp, int reptype)
{
	char namebuf[LINESIZE];
	char linebuf[LINESIZE];
	char *cp, *cp2;
	FILE *ibuf;
	int firstrun = 1;

	if ((cp = hfield("from", mp)) != NULL)
		return cp;
	if (reptype == 0 && (cp = hfield("sender", mp)) != NULL)
		return cp;
	ibuf = setinput(mp);
	namebuf[0] = '\0';
	if (readline(ibuf, linebuf, LINESIZE, 0) < 0)
		return savestr(namebuf);
 newname:
	for (cp = linebuf; *cp && *cp != ' '; cp++)
		continue;
	cp = skip_WSP(cp);
	for (cp2 = &namebuf[strlen(namebuf)];
	     *cp && !is_WSP(*cp) && cp2 < namebuf + LINESIZE - 1;
	     /*EMPTY*/)
		*cp2++ = *cp++;
	*cp2 = '\0';
	if (readline(ibuf, linebuf, LINESIZE, 0) < 0)
		return savestr(namebuf);
	if ((cp = strchr(linebuf, 'F')) == NULL)
		return savestr(namebuf);
	if (strncmp(cp, "From", 4) != 0)
		return savestr(namebuf);
	while ((cp = strchr(cp, 'r')) != NULL) {
		if (strncmp(cp, "remote", 6) == 0) {
			if ((cp = strchr(cp, 'f')) == NULL)
				break;
			if (strncmp(cp, "from", 4) != 0)
				break;
			if ((cp = strchr(cp, ' ')) == NULL)
				break;
			cp++;
			if (firstrun) {
				cp2 = namebuf;
				firstrun = 0;
			} else
				cp2 = strrchr(namebuf, '!') + 1;
			while (*cp && cp2 < namebuf + LINESIZE - 1)
				*cp2++ = *cp++;
			if (cp2 < namebuf + LINESIZE - 1)
				*cp2++ = '!';
			*cp2 = '\0';
			if (cp2 < namebuf + LINESIZE - 1)
				goto newname;
			else
				break;
		}
		cp++;
	}
	return savestr(namebuf);
}

/*
 * Count the occurrences of c in str
 */
static int
charcount(char *str, int c)
{
	char *cp;
	int i;

	for (i = 0, cp = str; *cp; cp++)
		if (*cp == c)
			i++;
	return i;
}

/*
 * Get sender's name from this message.  If the message has
 * a bunch of arpanet stuff in it, we may have to skin the name
 * before returning it.
 */
PUBLIC char *
nameof(struct message *mp, int reptype)
{
	char *cp, *cp2;

	cp = skin(name1(mp, reptype));
	if (reptype != 0 || charcount(cp, '!') < 2)
		return cp;
	cp2 = strrchr(cp, '!');
	cp2--;
	while (cp2 > cp && *cp2 != '!')
		cp2--;
	if (*cp2 == '!')
		return cp2 + 1;
	return cp;
}

/*
 * Copy s1 to s2, return pointer to null in s2.
 */
PUBLIC char *
copy(char *s1, char *s2)
{

	while ((*s2++ = *s1++) != '\0')
		continue;
	return s2 - 1;
}

/*
 * The core routine to check the ignore table for a field.
 * Note: realfield should be lower-cased!
 */
PUBLIC int
member(char *realfield, struct ignoretab *table)
{
	struct ignore *igp;

	for (igp = table->i_head[hash(realfield)]; igp != 0; igp = igp->i_link)
		if (*igp->i_field == *realfield &&
		    equal(igp->i_field, realfield))
			return 1;
	return 0;
}

/*
 * See if the given header field is supposed to be ignored.
 */
PUBLIC int
isign(const char *field, struct ignoretab ignoretabs[2])
{
	char realfld[LINESIZE];

	if (ignoretabs == ignoreall)
		return 1;
	/*
	 * Lower-case the string, so that "Status" and "status"
	 * will hash to the same place.
	 */
	istrcpy(realfld, field);
	if (ignoretabs[1].i_count > 0)
		return !member(realfld, ignoretabs + 1);
	else
		return member(realfld, ignoretabs);
}

PUBLIC void
add_ignore(const char *name, struct ignoretab *tab)
{
	char field[LINESIZE];
	int h;
	struct ignore *igp;

	istrcpy(field, name);
	if (member(field, tab))
		return;
	h = hash(field);
	igp = ecalloc(1, sizeof(struct ignore));
	igp->i_field = ecalloc(strlen(field) + 1, sizeof(char));
	(void)strcpy(igp->i_field, field);
	igp->i_link = tab->i_head[h];
	tab->i_head[h] = igp;
	tab->i_count++;
}

/*
 * Write a file to stdout, skipping comment lines.
 */
PUBLIC void
cathelp(const char *fname)
{
	char *line;
	FILE *f;
	size_t len;

	if ((f = Fopen(fname, "ref")) == NULL) {
		warn("%s", fname);
		return;
	}
	while ((line = fgetln(f, &len)) != NULL) {
		if (*line == '#')
			continue;
		if (fwrite(line, 1, len, stdout) != len)
			break;
	}
	(void)Fclose(f);
	return;
}
