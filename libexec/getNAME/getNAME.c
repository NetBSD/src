/*	$NetBSD: getNAME.c,v 1.23 2004/03/20 20:26:58 christos Exp $	*/

/*-
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#if 0
static char sccsid[] = "@(#)getNAME.c	8.1 (Berkeley) 6/30/93";
#else
__RCSID("$NetBSD: getNAME.c,v 1.23 2004/03/20 20:26:58 christos Exp $");
#endif
#endif /* not lint */

/*
 * Get name sections from manual pages.
 *	-t	for building toc
 *	-i	for building intro entries
 *	-w	for querying type of manual source
 *	-v	verbose
 *	other	apropos database
 */
#include <err.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int tocrc;
static int intro;
static int typeflag;
static int verbose;

#define SLOP 10	/* strlen(" () - ") < 10 */

static char *linebuf = NULL;
static size_t maxlen = 0;


static void doname(char *);
static void dorefname(char *);
static void getfrom(char *);
static void oldman(char *, char *);
static void newman(char *, char *);
static void remcomma(char *, size_t *);
static void remquote(char *, size_t *);
static void fixxref(char *, size_t *);
static void split(char *, char *);
static void usage(void);

int main(int, char *[]);

/* The .SH NAMEs that are allowed. */
static const char *names[] = { "name", "namn", 0 };

int
main(int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "itvw")) != -1)
		switch (ch) {
		case 'i':
			intro = 1;
			break;
		case 't':
			tocrc = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'w':
			typeflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (!*argv)
		usage();

	for (; *argv; ++argv)
		getfrom(*argv);
	return 0;
}

static void
getfrom(char *pathname)
{
	char *name;
	char *line;
	size_t len;

	if (freopen(pathname, "r", stdin) == 0) {
		warn("Cannot open `%s'", pathname);
		return;
	}
	if ((name = strrchr(pathname, '/')) != NULL)
		name++;
	else
		name = pathname;
	for (;;) {
		if ((line = fgetln(stdin, &len)) == NULL) {
			if (typeflag)
				(void)printf("%-60s\tUNKNOWN\n", pathname);
			if (verbose)
				warnx("missing .TH or .Dt section in `%s'",
				    pathname);
			return;
		}
		if (len < 3)
			continue;
		if (line[0] != '.')
			continue;
		if ((line[1] == 'T' && line[2] == 'H') ||
		    (line[1] == 't' && line[2] == 'h')) {
			oldman(pathname, name);
			return;
		}
		if (line[1] == 'D' && line[2] == 't') {
			newman(pathname, name);
			return;
		}
	}
}

static void
oldman(char *pathname, char *name)
{
	char *line, *ext, *s, *newlinebuf;
	size_t len, i, extlen;
	size_t curlen = 0;
	size_t newmaxlen;

	if (typeflag) {
		(void)printf("%-60s\tOLD\n", pathname);
		return;
	}
	for (;;) {
		if ((line = fgetln(stdin, &len)) == NULL) {
			if (verbose)
				warnx("missing .SH section in `%s'", pathname);
			return;
		}
		if (len < 4)
			continue;
		if (line[0] != '.')
			continue;
		if (line[1] == 'S' && line[2] == 'H')
			break;
		if (line[1] == 's' && line[2] == 'h')
			break;
	}

	for (s = &line[3]; s < &line[len] && 
	    (isspace((unsigned char) *s) || *s == '"' || *s == '\''); s++)
		continue;
	if (s == &line[len]) {
		warnx("missing argument to .SH in `%s'", pathname);
		return;
	}
	for (i = 0; names[i]; i++)
		if (strncasecmp(s, names[i], strlen(names[i])) == 0)
			break;
	if (names[i] == NULL) {
		warnx("first .SH section is not \"NAME\" in `%s'", pathname);
		return;
	}

	if (tocrc)
		doname(name);

	for (i = 0;; i++) {
		if ((line = fgetln(stdin, &len)) == NULL)
			break;
		if (line[0] == '.') {
			if (line[1] == '\\' && line[2] == '"')
				continue;	/* [nt]roff comment */
			if (line[1] == 'S' && line[2] == 'H')
				break;
			if (line[1] == 's' && line[2] == 'h')
				break;
			if (line[1] == 'P' && line[2] == 'P')
				break;
		}
		if (line[len - 1] == '\n') {
			line[len - 1] = '\0';
			len--;
		}
		if ((ext = strrchr(name, '.')) != NULL) {
			ext++;
			extlen = strlen(ext);
		}
		else
			extlen = 0;

		if (maxlen + extlen < curlen + len + SLOP) {
			newmaxlen = 2 * (curlen + len) + SLOP + extlen;
			if ((newlinebuf = realloc(linebuf, newmaxlen)) == NULL)
				err(1, NULL);
			linebuf = newlinebuf;
			maxlen = newmaxlen;
		}
		if (i != 0)
			linebuf[curlen++] = ' ';
		(void)memcpy(&linebuf[curlen], line, len);
		curlen += len;
		linebuf[curlen] = '\0';
		
		if(!tocrc && !intro) {
			/* change the \- into (N) - */
			if ((s = strstr(linebuf, "\\-")) != NULL) {
				(void)memmove(s + extlen + 3, s + 1, 
					      curlen - (s + 1 - linebuf));
				curlen--;
				if (extlen) {
					*s++ = '(';
					while (*ext)
						*s++ = *ext++;
					*s++ = ')';
					*s++ = ' ';
					curlen += extlen + 3;
				}
				linebuf[curlen] = '\0';
			}
		}
	}

	if (intro)
		split(linebuf, name);
	else
		(void)printf("%s\n", linebuf);
	return;
}

static void
newman(char *pathname, char *name)
{
	char *line, *ext, *s, *newlinebuf;
	size_t len, i, extlen;
	size_t curlen = 0;
	size_t newmaxlen;

	if (typeflag) {
		(void)printf("%-60s\tNEW\n", pathname);
		return;
	}
	for (;;) {
		if ((line = fgetln(stdin, &len)) == NULL) {
			if (verbose)
				warnx("missing .Sh section in `%s'", pathname);
			return;
		}
		if (line[0] != '.')
			continue;
		if (line[1] == 'S' && line[2] == 'h')
			break;
	}

	for (s = &line[3]; s < &line[len] && isspace((unsigned char) *s); s++)
		continue;
	if (s == &line[len]) {
		warnx("missing argument to .Sh in `%s'", pathname);
		return;
	}
	for (i = 0; names[i]; i++)
		if (strncasecmp(s, names[i], strlen(names[i])) == 0)
			break;
	if (names[i] == NULL) {
		warnx("first .SH section is not \"NAME\" in `%s'", pathname);
		return;
	}

	if (tocrc)
		doname(name);

	for (i = 0;; i++) {
		if ((line = fgetln(stdin, &len)) == NULL)
			break;

		if (line[0] == '.') {
			if (line[1] == '\\' && line[2] == '"')
				continue;	/* [nt]roff comment */
			if (line[1] == 'S' && line[2] == 'h')
				break;
		}

		if (line[len - 1] == '\n') {
			line[len - 1] = '\0';
			len--;
		}

		if ((ext = strrchr(name, '.')) != NULL) {
			ext++;
			extlen = strlen(ext);
		}
		else
			extlen = 0;

		if (maxlen + extlen < curlen + len + SLOP) {
			newmaxlen = 2 * (curlen + len) + SLOP + extlen;
			if ((newlinebuf = realloc(linebuf, newmaxlen)) == NULL)
				err(1, NULL);
			linebuf = newlinebuf;
			maxlen = newmaxlen;
		}

		if (i != 0)
			linebuf[curlen++] = ' ';

		remcomma(line, &len);

		if (line[0] != '.') {
			(void)memcpy(&linebuf[curlen], line, len);
			curlen += len;
		}
		else {
			remquote(line, &len);
			fixxref(line, &len);

			/*
			 * Put section and dash between names and description.
			 */
			if (line[1] == 'N' && line[2] == 'd') {
				if(!tocrc && !intro) {
					if (extlen) {
						linebuf[curlen++] = '(';
						while (*ext)
							linebuf[curlen++] = *ext++;
						linebuf[curlen++] = ')';
						linebuf[curlen++] = ' ';
					}
				}
				linebuf[curlen++] = '-';
				linebuf[curlen++] = ' ';
			}
			/*
			 * Skip over macro names.
			 */
			if (len <= 4) 
				continue;
			(void)memcpy(&linebuf[curlen], &line[4], len - 4);
			curlen += len - 4;
		}
	}
	linebuf[curlen] = '\0';
	if (intro)
		split(linebuf, name);
	else
		(void)printf("%s\n", linebuf);
}

/*
 * convert " ," -> " "
 */
static void
remcomma(char *line, size_t *len)
{
	char *pline = line, *loc;
	size_t plen = *len;

	while ((loc = memchr(pline, ' ', plen)) != NULL) {
		plen -= loc - pline + 1;
		pline = loc;
		if (loc[1] == ',') {
			(void)memcpy(loc, &loc[1], plen);
			(*len)--;
		}
		else
			pline++;
	}
}

/*
 * Get rid of quotes in macros.
 */
static void
remquote(char *line, size_t *len)
{
	char *loc;
	char *pline = &line[4];
	size_t plen = *len - 4;

	if (*len < 4)
		return;

	while ((loc = memchr(pline, '"', plen)) != NULL) {
		plen -= loc - pline + 1;
		pline = loc;
		(void)memcpy(loc, &loc[1], plen);
		(*len)--;
	}
}

/*
 * Handle cross references
 */
static void
fixxref(char *line, size_t *len)
{
	char *loc;
	char *pline = &line[4];
	size_t plen = *len - 4;

	if (*len < 4)
		return;

	if (line[1] == 'X' && line[2] == 'r') {
		if ((loc = memchr(pline, ' ', plen)) != NULL) {
			*loc++ = '(';
			loc++;
			*loc++ = ')';
			*len = loc - line;
		}
	}
}

static void
doname(char *name)
{
	char *dp = name, *ep;

again:
	while (*dp && *dp != '.')
		(void)putchar(*dp++);
	if (*dp)
		for (ep = dp+1; *ep; ep++)
			if (*ep == '.') {
				(void)putchar(*dp++);
				goto again;
			}
	(void)putchar('(');
	if (*dp)
		dp++;
	while (*dp)
		(void)putchar(*dp++);
	(void)putchar(')');
	(void)putchar(' ');
}

static void
split(char *line, char *name)
{
	char *cp, *dp;
	char *sp;
	const char *sep;

	cp = strchr(line, '-');
	if (cp == 0)
		return;
	sp = cp + 1;
	for (--cp; *cp == ' ' || *cp == '\t' || *cp == '\\'; cp--)
		;
	*++cp = '\0';
	while (*sp && (*sp == ' ' || *sp == '\t'))
		sp++;
	for (sep = "", dp = line; dp && *dp; dp = cp, sep = "\n") {
		cp = strchr(dp, ',');
		if (cp) {
			char *tp;

			for (tp = cp - 1; *tp == ' ' || *tp == '\t'; tp--)
				;
			*++tp = '\0';
			for (++cp; *cp == ' ' || *cp == '\t'; cp++)
				;
		}
		(void)printf("%s%s\t", sep, dp);
		dorefname(name);
		(void)printf("\t- %s", sp);
	}
	(void)putchar('\n');
}

static void
dorefname(char *name)
{
	char *dp = name, *ep;

again:
	while (*dp && *dp != '.')
		(void)putchar(*dp++);
	if (*dp)
		for (ep = dp+1; *ep; ep++)
			if (*ep == '.') {
				(void)putchar(*dp++);
				goto again;
			}
	(void)putchar('.');
	if (*dp)
		dp++;
	while (*dp)
		(void)putchar(*dp++);
}

static void
usage(void)
{

	(void)fprintf(stderr, "Usage: %s [-itw] file ...\n", getprogname());
	exit(1);
}
