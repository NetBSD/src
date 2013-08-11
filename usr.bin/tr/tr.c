/*	$NetBSD: tr.c,v 1.17 2013/08/11 00:39:22 dholland Exp $	*/

/*
 * Copyright (c) 1988, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1988, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)tr.c	8.2 (Berkeley) 5/4/95";
#endif
__RCSID("$NetBSD: tr.c,v 1.17 2013/08/11 00:39:22 dholland Exp $");
#endif /* not lint */

#include <sys/types.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

static int string1[NCHARS], string2[NCHARS];

static void setup(int *, const char *, int, int);
__dead static void usage(void);

int
main(int argc, char **argv)
{
	int ch, ch2, lastch;
	int cflag, dflag, sflag, isstring2;
	STR *s1, *s2;

	cflag = dflag = sflag = 0;
	while ((ch = getopt(argc, argv, "cds")) != -1)
		switch (ch) {
		case 'c':
			cflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	switch(argc) {
	case 0:
	default:
		usage();
		/* NOTREACHED */
	case 1:
		isstring2 = 0;
		break;
	case 2:
		isstring2 = 1;
		break;
	}

	/*
	 * tr -ds [-c] string1 string2
	 * Delete all characters (or complemented characters) in string1.
	 * Squeeze all characters in string2.
	 */
	if (dflag && sflag) {
		if (!isstring2)
			usage();

		setup(string1, argv[0], 1, cflag);
		setup(string2, argv[1], 2, 0);
		
		for (lastch = OOBCH; (ch = getchar()) != EOF;)
			if (!string1[ch] && (!string2[ch] || lastch != ch)) {
				lastch = ch;
				(void)putchar(ch);
			}
		exit(0);
	}

	/*
	 * tr -d [-c] string1
	 * Delete all characters (or complemented characters) in string1.
	 */
	if (dflag) {
		if (isstring2)
			usage();

		setup(string1, argv[0], 1, cflag);

		while ((ch = getchar()) != EOF)
			if (!string1[ch])
				(void)putchar(ch);
		exit(0);
	}

	/*
	 * tr -s [-c] string1
	 * Squeeze all characters (or complemented characters) in string1.
	 */
	if (sflag && !isstring2) {
		setup(string1, argv[0], 1, cflag);

		for (lastch = OOBCH; (ch = getchar()) != EOF;)
			if (!string1[ch] || lastch != ch) {
				lastch = ch;
				(void)putchar(ch);
			}
		exit(0);
	}

	/*
	 * tr [-cs] string1 string2
	 * Replace all characters (or complemented characters) in string1 with
	 * the character in the same position in string2.  If the -s option is
	 * specified, squeeze all the characters in string2.
	 */
	if (!isstring2)
		usage();

	if (cflag) {
		setup(string1, argv[0], 1, cflag);
		s1 = NULL; /* for safety */
		ch = -1;
	} else {
		s1 = str_create(1, argv[0]);
		for (ch = 0; ch < NCHARS; ch++) {
			string1[ch] = ch;
		}
	}
	s2 = str_create(2, argv[1]);

	if (!next(s2, &ch2))
		errx(1, "empty string2");

	/* If string2 runs out of characters, use the last one specified. */
	while (1) {
		if (cflag) {
			ch++;
			while (ch < NCHARS && string1[ch] == 0) {
				if (string1[ch] == 0) {
					string1[ch] = ch;
				}
				ch++;
			}
			if (ch == NCHARS) {
				break;
			}
		}
		else {
			if (!next(s1, &ch)) {
				break;
			}
		}

		string1[ch] = ch2;
		if (sflag) {
			string2[ch2] = 1;
		}
		(void)next(s2, &ch2);
	}

	if (sflag)
		for (lastch = OOBCH; (ch = getchar()) != EOF;) {
			ch = string1[ch];
			if (!string2[ch] || lastch != ch) {
				lastch = ch;
				(void)putchar(ch);
			}
		}
	else
		while ((ch = getchar()) != EOF)
			(void)putchar(string1[ch]);

	if (s1 != NULL) {
		str_destroy(s1);
	}
	str_destroy(s2);
	exit (0);
}

static void
setup(int *string, const char *arg, int whichstring, int cflag)
{
	int cnt, *p;
	int ch;
	STR *str;

	str = str_create(whichstring, arg);
	memset(string, 0, NCHARS * sizeof(int));
	while (next(str, &ch))
		string[ch] = 1;
	if (cflag)
		for (p = string, cnt = NCHARS; cnt--; ++p)
			*p = !*p;
	str_destroy(str);
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: tr [-cs] string1 string2\n");
	(void)fprintf(stderr, "       tr [-c] -d string1\n");
	(void)fprintf(stderr, "       tr [-c] -s string1\n");
	(void)fprintf(stderr, "       tr [-c] -ds string1 string2\n");
	exit(1);
}
