/*	$NetBSD: caesar.c,v 1.15 2005/05/23 23:02:30 rillig Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Adams.
 *
 * Authors:
 *	Stan King, John Eldridge, based on algorithm suggested by
 *	Bob Morris
 * 29-Sep-82
 *      Roland Illig, 2005
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
__COPYRIGHT("@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)caesar.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: caesar.c,v 1.15 2005/05/23 23:02:30 rillig Exp $");
#endif
#endif /* not lint */

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define NCHARS			(1 << CHAR_BIT)
#define LETTERS			(26)

/*
 * letter frequencies (taken from some unix(tm) documentation)
 * (unix is a trademark of Bell Laboratories)
 */
static const unsigned char upper[LETTERS] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const unsigned char lower[LETTERS] = "abcdefghijklmnopqrstuvwxyz";
static double stdf[LETTERS] = {
	7.97, 1.35, 3.61, 4.78, 12.37, 2.01, 1.46, 4.49, 6.39, 0.04,
	0.42, 3.81, 2.69, 5.92,  6.96, 2.91, 0.08, 6.63, 8.77, 9.68,
	2.62, 0.81, 1.88, 0.23,  2.07, 0.06
};

static unsigned char rottbl[NCHARS];


static void
init_rottbl(int rot)
{
	size_t i;

	rot %= LETTERS;

	for (i = 0; i < NCHARS; i++)
		rottbl[i] = (unsigned char)i;

	for (i = 0; i < LETTERS; i++)
		rottbl[upper[i]] = upper[(i + rot) % LETTERS];

	for (i = 0; i < LETTERS; i++)
		rottbl[lower[i]] = lower[(i + rot) % LETTERS];
}

static void __attribute__((__noreturn__))
printrot(const char *arg)
{
	int ch;
	long rot;
	char *endp;

	errno = 0;
	rot = strtol(arg, &endp, 10);
	if (*endp != '\0') {
		errx(EXIT_FAILURE, "bad rotation value: %s", arg);
		/* NOTREACHED */
	}
	if (errno == ERANGE || rot < 0 || rot > INT_MAX) {
		errx(EXIT_FAILURE, "rotation value out of range: %s", arg);
		/* NOTREACHED */
	}
	init_rottbl((int)rot);

	while ((ch = getchar()) != EOF) {
		if (putchar(rottbl[ch]) == EOF) {
			err(EXIT_FAILURE, "writing to stdout");
			/* NOTREACHED */
		}
	}
	exit(EXIT_SUCCESS);
	/* NOTREACHED */
}

int
main(int argc, char **argv)
{
	ssize_t i, nread, ntotal;
	double dot, winnerdot;
	int try, winner;
	unsigned char inbuf[2048];
	unsigned int obs[NCHARS];

	/* revoke setgid privileges */
	(void)setgid(getgid());

	if (argc > 1) {
		printrot(argv[1]);
		/* NOTREACHED */
	}

	/* adjust frequency table to weight low probs REAL low */
	for (i = 0; i < LETTERS; i++)
		stdf[i] = log(stdf[i]) + log(LETTERS / 100.0);

	/* zero out observation table */
	(void)memset(obs, 0, sizeof(obs));

	for (ntotal = 0; (size_t) ntotal < sizeof(inbuf); ntotal += nread) {
		nread = read(STDIN_FILENO, &inbuf[ntotal],
		             sizeof(inbuf) - ntotal);
		if (nread < 0) {
			err(EXIT_FAILURE, "reading from stdin");
			/* NOTREACHED */
		}
		if (nread == 0)
			break;
	}

	for (i = 0; i < ntotal; i++)
		obs[inbuf[i]]++;

	/*
	 * now "dot" the freqs with the observed letter freqs
	 * and keep track of best fit
	 */
	winner = 0;
	winnerdot = 0.0;
	for (try = 0; try < LETTERS; try++) {
		dot = 0.0;
		for (i = 0; i < LETTERS; i++)
			dot += (obs[upper[i]] + obs[lower[i]])
			     * stdf[(i + try) % LETTERS];

		if (try == 0 || dot > winnerdot) {
			/* got a new winner! */
			winner = try;
			winnerdot = dot;
		}
	}
	init_rottbl(winner);

	while (ntotal > 0) {
		for (i = 0; i < ntotal; i++) {
			if (putchar(rottbl[inbuf[i]]) == EOF) {
				err(EXIT_FAILURE, "writing to stdout");
				/* NOTREACHED */
			}
		}
		if ((ntotal = read(STDIN_FILENO, inbuf, sizeof(inbuf))) < 0) {
			err(EXIT_FAILURE, "reading from stdin");
			/* NOTREACHED */
		}
	}
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}
