/*	$NetBSD: unexpand.c,v 1.11 2003/07/14 09:24:00 itojun Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)unexpand.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: unexpand.c,v 1.11 2003/07/14 09:24:00 itojun Exp $");
#endif /* not lint */

/*
 * unexpand - put tabs into a file replacing blanks
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

char	genbuf[BUFSIZ];
char	linebuf[BUFSIZ];

int	main(int, char **);
void	tabify(int, uint);

int
main(int argc, char **argv)
{
	int all, c;
	uint tabsize;
	ulong l;
	char *ep;

	setprogname(argv[0]);

	all = 0;
	tabsize = 8;
	while ((c = getopt(argc, argv, "at:")) != -1) {
		switch (c) {
		case 'a':
			all++;
			break;
		case 't':
			errno = 0;
			l = strtoul(optarg, &ep, 0);
			/*
			 * If every input char is a tab, the line length
			 * must not exceed maxuint.
			 */
			tabsize = (int)l * BUFSIZ;
			tabsize /= BUFSIZ;
			if (*ep != 0 || errno != 0 || (ulong)tabsize != l)
				errx(EXIT_FAILURE, "Invalid tabstop \"%s\"",
				    optarg);
			break;
		case '?':
		default:
			fprintf(stderr, "usage: %s [-a] [-t tabstop] [file ...]\n",
				getprogname());
			exit(EXIT_FAILURE);
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	do {
		if (argc > 0) {
			if (freopen(argv[0], "r", stdin) == NULL) {
				perror(argv[0]);
				exit(EXIT_FAILURE);
			}
			argc--, argv++;
		}
		while (fgets(genbuf, BUFSIZ, stdin) != NULL) {
			tabify(all, tabsize);
			printf("%s", linebuf);
		}
	} while (argc > 0);
	exit(EXIT_SUCCESS);
	/* NOTREACHED */
}

void
tabify(int all, uint tabsize)
{
	char *cp, *dp;
	uint dcol;
	uint ocol;
	uint tcol;

	ocol = 0;
	dcol = 0;
	cp = genbuf, dp = linebuf;
	for (;;) {
		switch (*cp) {

		case ' ':
			dcol++;
			break;

		case '\t':
			dcol = (dcol + tabsize) / tabsize * tabsize;
			break;

		default:
			if (dcol > ocol + 1) {
				tcol = (ocol + tabsize) / tabsize * tabsize;
				while (tcol <= dcol) {
					*dp++ = '\t';
					ocol = tcol;
					tcol += tabsize;
				}
			}
			while (ocol < dcol) {
				*dp++ = ' ';
				ocol++;
			}
			if (*cp == 0 || all == 0) {
				strlcpy(dp, cp,
				    sizeof(linebuf) - (dp - linebuf));
				return;
			}
			*dp++ = *cp;
			dcol = ++ocol;
		}
		cp++;
	}
}
