/*	$NetBSD: apropos.c,v 1.17 2002/03/08 20:23:10 jdolecek Exp $	*/

/*
 * Copyright (c) 1987, 1993, 1994
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
__COPYRIGHT("@(#) Copyright (c) 1987, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)apropos.c	8.8 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: apropos.c,v 1.17 2002/03/08 20:23:10 jdolecek Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/queue.h>

#include <ctype.h>
#include <err.h>
#include <glob.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <man/config.h>
#include <man/pathnames.h>

static int *found, foundman;

#define	MAXLINELEN	8192		/* max line handled */

int main __P((int, char **));
void apropos __P((char **, char *, int));
void lowstr __P((char *, char *));
int match __P((char *, char *));
void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	ENTRY *ep;
	TAG *tp;
	int ch, rv;
	char *conffile, **p, *p_augment, *p_path;
	glob_t pg;

	conffile = NULL;
	p_augment = p_path = NULL;
	while ((ch = getopt(argc, argv, "C:M:m:P:")) != -1)
		switch (ch) {
		case 'C':
			conffile = optarg;
			break;
		case 'M':
		case 'P':		/* backward compatible */
			p_path = optarg;
			break;
		case 'm':
			p_augment = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

	if (argc < 1)
		usage();

	if ((found = malloc((u_int)argc * sizeof(int))) == NULL)
		err(1, "malloc");
	memset(found, 0, argc * sizeof(int));

	for (p = argv; *p; ++p)			/* convert to lower-case */
		lowstr(*p, *p);

	if (p_augment)
		apropos(argv, p_augment, 1);
	if (p_path || (p_path = getenv("MANPATH")))
		apropos(argv, p_path, 1);
	else {
		config(conffile);
		ep = (tp = getlist("_whatdb")) == NULL ?
			NULL : TAILQ_FIRST(&tp->list);
		for (; ep != NULL; ep = TAILQ_NEXT(ep, q)) {
			if ((rv = glob(ep->s, GLOB_BRACE | GLOB_NOSORT, NULL,
			    &pg)) != 0) {
				if (rv == GLOB_NOMATCH)
					continue;
				else
					err(EXIT_FAILURE, "glob"); 
			}
			if (pg.gl_pathc)
				for (p = pg.gl_pathv; *p; p++)
					apropos(argv, *p, 0);
			globfree(&pg);
		}
	}

	if (!foundman)
		errx(1, "no %s file found", _PATH_WHATIS);

	rv = 1;
	for (p = argv; *p; ++p)
		if (found[p - argv])
			rv = 0;
		else
			(void)printf("%s: nothing appropriate\n", *p);
	exit(rv);
}

void
apropos(argv, path, buildpath)
	char **argv, *path;
	int buildpath;
{
	char *end, *name, **p;
	char buf[MAXLINELEN + 1], wbuf[MAXLINELEN + 1];
	char hold[MAXPATHLEN + 1];

	for (name = path; name; name = end) {	/* through name list */
		if ((end = strchr(name, ':')))
			*end++ = '\0';

		if (buildpath) {
			(void)sprintf(hold, "%s/%s", name, _PATH_WHATIS);
			name = hold;
		}

		if (!freopen(name, "r", stdin))
			continue;

		foundman = 1;

		/* for each file found */
		while (fgets(buf, sizeof(buf), stdin)) {
			if (!strchr(buf, '\n')) {
				warnx("%s: line too long", name);
				continue;
			}
			lowstr(buf, wbuf);
			for (p = argv; *p; ++p)
				if (match(wbuf, *p)) {
					(void)printf("%s", buf);
					found[p - argv] = 1;

					/* only print line once */
					while (*++p)
						if (match(wbuf, *p))
							found[p - argv] = 1;
					break;
				}
		}
	}
}

/*
 * match --
 *	match anywhere the string appears
 */
int
match(bp, str)
	char *bp, *str;
{
	int len;
	char test;

	if (!*bp)
		return (0);
	/* backward compatible: everything matches empty string */
	if (!*str)
		return (1);
	for (test = *str++, len = strlen(str); *bp;)
		if (test == *bp++ && !strncmp(bp, str, len))
			return (1);
	return (0);
}

/*
 * lowstr --
 *	convert a string to lower case
 */
void
lowstr(from, to)
	char *from, *to;
{
	char ch;

	while ((ch = *from++) && ch != '\n')
		*to++ = isupper((unsigned char)ch) ? tolower(ch) : ch;
	*to = '\0';
}

/*
 * usage --
 *	print usage message and die
 */
void
usage()
{

	(void)fprintf(stderr,
	    "Usage: %s [-C file] [-M path] [-m path] keyword ...\n",
	    getprogname());
	exit(1);
}
