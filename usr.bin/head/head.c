/*	$NetBSD: head.c,v 1.18 2004/05/04 10:57:42 mrg Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1980, 1987, 1992, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)head.c	8.2 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: head.c,v 1.18 2004/05/04 10:57:42 mrg Exp $");
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

void head __P((FILE *, long, long));
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
	long bytecnt;
	char *ep;
	int qflag = 0;
	int vflag = 0;

	(void)setlocale(LC_ALL, "");
	obsolete(argv);
	linecnt = 10;
	bytecnt = 0;
	while ((ch = getopt(argc, argv, "c:n:qv")) != -1)
		switch(ch) {
		case 'c':
			errno = 0;
			bytecnt = strtol(optarg, &ep, 10);
			if ((bytecnt == LONG_MIN || bytecnt == LONG_MAX) &&
			    errno == ERANGE)
				err(1, "illegal byte count -- %s", optarg);
			else if (*ep || bytecnt <= 0)
				errx(1, "illegal byte count -- %s", optarg);
			break;

		case 'n':
			errno = 0;
			linecnt = strtol(optarg, &ep, 10);
			if ((linecnt == LONG_MIN || linecnt == LONG_MAX) &&
			    errno == ERANGE)
				err(1, "illegal line count -- %s", optarg);
			else if (*ep || linecnt <= 0)
				errx(1, "illegal line count -- %s", optarg);
			break;

		case 'q':
			qflag = 1;
			vflag = 0;
			break;

		case 'v':
			qflag = 0;
			vflag = 1;
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
			if (vflag || (qflag == 0 && argc > 1)) {
				(void)printf("%s==> %s <==\n",
				    first ? "" : "\n", *argv);
				first = 0;
			}
			head(fp, linecnt, bytecnt);
			(void)fclose(fp);
		}
	else
		head(stdin, linecnt, bytecnt);
	exit(eval);
}

void
head(fp, cnt, bytecnt)
	FILE *fp;
	long cnt;
	long bytecnt;
{
	int ch;

	if (bytecnt)
		cnt = bytecnt;
	while (cnt--)
		while ((ch = getc(fp)) != EOF) {
			if (putchar(ch) == EOF)
				err(1, "stdout");
			if (ch == '\n' || bytecnt)
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

	(void)fprintf(stderr, "usage: %s [-n lines] [file ...]\n",
	    getprogname());
	exit(1);
}
