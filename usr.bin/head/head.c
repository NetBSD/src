/*	$NetBSD: head.c,v 1.11.6.1 1999/12/27 18:37:00 wrstuden Exp $	*/

/*
 * Copyright (c) 1980, 1987, 1992, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1987, 1992, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)head.c	8.2 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: head.c,v 1.11.6.1 1999/12/27 18:37:00 wrstuden Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * head - give the first few lines of a stream or of each of a set of files
 *
 * Bill Joy UCB August 24, 1977
 */

void head __P((FILE *, long));
void obsolete __P((char *[]));
void usage __P((void));
int main __P((int, char *[]));

int eval = 0;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch;
	FILE *fp;
	int first;
	long linecnt;
	char *ep;

	(void)setlocale(LC_ALL, "");
	obsolete(argv);
	linecnt = 10;
	while ((ch = getopt(argc, argv, "n:")) != -1)
		switch(ch) {
		case 'n':
			linecnt = strtol(optarg, &ep, 10);
			if ((linecnt == LONG_MIN || linecnt == LONG_MAX) &&
			    errno == ERANGE)
				err(1, "illegal line count -- %s", optarg);
			else if (*ep || linecnt <= 0)
				errx(1, "illegal line count -- %s", optarg);
			break;

		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (*argv)
		for (first = 1; *argv; ++argv) {
			if ((fp = fopen(*argv, "r")) == NULL) {
				warn("%s", *argv);
				eval = 1;
				continue;
			}
			if (argc > 1) {
				(void)printf("%s==> %s <==\n",
				    first ? "" : "\n", *argv);
				first = 0;
			}
			head(fp, linecnt);
			(void)fclose(fp);
		}
	else
		head(stdin, linecnt);
	exit(eval);
}

void
head(fp, cnt)
	FILE *fp;
	long cnt;
{
	int ch;

	while (cnt--)
		while ((ch = getc(fp)) != EOF) {
			if (putchar(ch) == EOF)
				err(1, "stdout");
			if (ch == '\n')
				break;
		}
}

void
obsolete(argv)
	char *argv[];
{
	char *ap;

	while ((ap = *++argv)) {
		/* Return if "--" or not "-[0-9]*". */
		if (ap[0] != '-' || ap[1] == '-' ||
		    !isdigit((unsigned char)ap[1]))
			return;
		if ((ap = malloc(strlen(*argv) + 2)) == NULL)
			err(1, NULL);
		ap[0] = '-';
		ap[1] = 'n';
		(void)strcpy(ap + 2, *argv + 1);
		*argv = ap;
	}
}

void
usage()
{
	extern char *__progname;
	(void)fprintf(stderr, "Usage: %s [-n lines] [file ...]\n", __progname);
	exit(1);
}
