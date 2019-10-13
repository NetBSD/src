/*	$NetBSD: locate.code.c,v 1.12 2019/10/13 19:39:15 christos Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * James A. Woods.
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
__COPYRIGHT("@(#) Copyright (c) 1989, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)locate.code.c	8.4 (Berkeley) 5/4/95";
#endif
__RCSID("$NetBSD: locate.code.c,v 1.12 2019/10/13 19:39:15 christos Exp $");
#endif /* not lint */

/*
 * PURPOSE:	sorted list compressor (works with a modified 'find'
 *		to encode/decode a filename database)
 *
 * USAGE:	bigram < list > bigrams
 *		process bigrams (see updatedb) > common_bigrams
 *		code common_bigrams < list > squozen_list
 *
 * METHOD:	Uses 'front compression' (see ";login:", Volume 8, Number 1
 *		February/March 1983, p. 8).  Output format is, per line, an
 *		offset differential count byte followed by a partially bigram-
 *		encoded ascii residue.  A bigram is a two-character sequence,
 *		the first 128 most common of which are encoded in one byte.
 *
 * EXAMPLE:	For simple front compression with no bigram encoding,
 *		if the input is...		then the output is...
 *
 *		/usr/src			 0 /usr/src
 *		/usr/src/cmd/aardvark.c		 8 /cmd/aardvark.c
 *		/usr/src/cmd/armadillo.c	14 armadillo.c
 *		/usr/tmp/zoo			 5 tmp/zoo
 *
 *	The codes are:
 *
 *	0-28	likeliest differential counts + offset to make nonnegative
 *	30	switch code for out-of-range count to follow in next word
 *	128-255 bigram codes (128 most common, as determined by 'updatedb')
 *	32-127  single character (printable) ascii residue (ie, literal)
 *
 * SEE ALSO:	updatedb.csh, bigram.c
 *
 * AUTHOR:	James A. Woods, Informatics General Corp.,
 *		NASA Ames Research Center, 10/82
 */

#include <sys/param.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "locate.h"

#define	BGBUFSIZE	(NBG * 2)	/* size of bigram buffer */

static char buf1[MAXPATHLEN] = " ";
static char buf2[MAXPATHLEN];
static char bigrams[BGBUFSIZE + 1] = { 0 };

static int	bgindex(char *);
__dead static void	usage(void);

int
main(int argc, char *argv[])
{
	char *cp, *oldpath, *path;
	int ch, code, count, diffcount, oldcount;

	while ((ch = getopt(argc, argv, "")) != -1)
		switch(ch) {
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	/* First copy bigram array to stdout. */
	strncpy(bigrams, argv[0], BGBUFSIZE);
	bigrams[BGBUFSIZE] = '\0';
	if (fwrite(bigrams, 1, BGBUFSIZE, stdout) != BGBUFSIZE)
		err(1, "stdout");

	oldpath = buf1;
	path = buf2;
	oldcount = 0;
	while (fgets(path, sizeof(buf2), stdin) != NULL) {
		/* Truncate newline. */
		cp = path + strlen(path) - 1;
		if (cp > path && *cp == '\n')
			*cp = '\0';

		/* Squelch characters that would botch the decoding. */
		for (cp = path; *cp != '\0'; cp++) {
			*cp &= PARITY-1;
			if (*cp <= SWITCH)
				*cp = '?';
		}

		/* Skip longest common prefix. */
		for (cp = path; *cp == *oldpath; cp++, oldpath++)
			if (*oldpath == '\0')
				break;
		count = cp - path;
		diffcount = count - oldcount + OFFSET;
		oldcount = count;
		if (diffcount < 0 || diffcount > 2 * OFFSET) {
			if (putchar(SWITCH) == EOF ||
			    putw(diffcount, stdout) == EOF)
				err(1, "stdout");
		} else
			if (putchar(diffcount) == EOF)
				err(1, "stdout");

		while (*cp != '\0') {
			if (*(cp + 1) == '\0') {
				if (putchar(*cp) == EOF)
					err(1, "stdout");
				break;
			}
			if ((code = bgindex(cp)) < 0) {
				if (putchar(*cp++) == EOF ||
				    putchar(*cp++) == EOF)
					err(1, "stdout");
			} else {
				/* Found, so mark byte with parity bit. */
				if (putchar((code / 2) | PARITY) == EOF)
					err(1, "stdout");
				cp += 2;
			}
		}
		if (path == buf1) {		/* swap pointers */
			path = buf2;
			oldpath = buf1;
		} else {
			path = buf1;
			oldpath = buf2;
		}
	}
	/* Non-zero status if there were errors */
	if (fflush(stdout) != 0 || ferror(stdout))
		exit(1);
	exit(0);
}

static int
bgindex(char *bg)			/* Return location of bg in bigrams or -1. */
{
	char bg0, bg1, *p;

	bg0 = bg[0];
	bg1 = bg[1];
	for (p = bigrams; *p != '\0'; p += 2)
		if (p[0] == bg0 && p[1] == bg1)
			break;
	return (*p == '\0' ? -1 : p - bigrams);
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: locate.code common_bigrams < list > squozen_list\n");
	exit(1);
}
