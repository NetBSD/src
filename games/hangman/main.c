/*	$NetBSD: main.c,v 1.11 2003/08/07 09:37:22 agc Exp $	*/

/*
 * Copyright (c) 1983, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)main.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: main.c,v 1.11 2003/08/07 09:37:22 agc Exp $");
#endif
#endif /* not lint */

#include	<err.h>
#include	"hangman.h"

/*
 * This game written by Ken Arnold.
 */
int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch;

	/* Revoke setgid privileges */
	setgid(getgid());

	while ((ch = getopt(argc, argv, "d:m:")) != -1) {
		switch (ch) {
		case 'd':
			Dict_name = optarg;
			break;
		case 'm':
			Minlen = atoi(optarg);
			if (Minlen < 2)
				errx(1, "minimum word length too short");
			break;
		case '?':
		default:
			(void)fprintf(stderr, "usage: hangman [-d wordlist] [-m minlen]\n");
			exit(1);
		}
	}

	initscr();
	signal(SIGINT, die);
	setup();
	for (;;) {
		Wordnum++;
		playgame();
		Average = (Average * (Wordnum - 1) + Errors) / Wordnum;
	}
	/* NOTREACHED */
}
/*
 * die:
 *	Die properly.
 */
void
die(dummy)
	int dummy __attribute__((__unused__));
{
	mvcur(0, COLS - 1, LINES - 1, 0);
	endwin();
	putchar('\n');
	exit(0);
}
