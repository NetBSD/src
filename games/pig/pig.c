/*	$NetBSD: pig.c,v 1.14.6.1 2012/10/30 18:58:27 yamt Exp $	*/

/*-
 * Copyright (c) 1992, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1992, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)pig.c	8.2 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: pig.c,v 1.14.6.1 2012/10/30 18:58:27 yamt Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int, char *[]);
static void pigout(char *, int);
static void usage(void) __dead;

int
main(int argc, char *argv[])
{
	int len;
	int ch;
	char buf[1024];

	while ((ch = getopt(argc, argv, "")) != -1)
		switch(ch) {
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	for (len = 0; (ch = getchar()) != EOF;) {
		if (isalpha(ch)) {
			if ((size_t)len >= sizeof(buf))
				errx(1, "ate too much!");
			buf[len++] = ch;
			continue;
		}
		if (len != 0) {
			pigout(buf, len);
			len = 0;
		}
		(void)putchar(ch);
	}
	exit(0);
}

static void
pigout(char *buf, int len)
{
	int ch, start, i;
	int olen, allupper, firstupper;

	/* See if the word is all upper case */
	allupper = firstupper = isupper((unsigned char)buf[0]);
	for (i = 1; i < len && allupper; i++)
		allupper = allupper && isupper((unsigned char)buf[i]);

	/*
	 * If the word starts with a vowel, append "way".  Don't treat 'y'
	 * as a vowel if it appears first.
	 */
	if (strchr("aeiouAEIOU", buf[0]) != NULL) {
		(void)printf("%.*s%s", len, buf,
		    allupper ? "WAY" : "way");
		return;
	}

	/*
	 * Copy leading consonants to the end of the word.  The unit "qu"
	 * isn't treated as a vowel.
	 */
	if (!allupper)
		buf[0] = tolower((unsigned char)buf[0]);
	for (start = 0, olen = len;
	    !strchr("aeiouyAEIOUY", buf[start]) && start < olen;) {
		ch = buf[len++] = buf[start++];
		if ((ch == 'q' || ch == 'Q') && start < olen &&
		    (buf[start] == 'u' || buf[start] == 'U'))
			buf[len++] = buf[start++];
	}
	if (firstupper)
		buf[start] = toupper((unsigned char)buf[start]);
	(void)printf("%.*s%s", olen, buf + start, allupper ? "AY" : "ay");
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: pig\n");
	exit(1);
}
