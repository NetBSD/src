/*	$NetBSD: sleep.c,v 1.10 1997/08/04 01:13:10 perry Exp $	*/

/*
 * Copyright (c) 1988, 1993, 1994
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
__COPYRIGHT("@(#) Copyright (c) 1988, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)sleep.c	8.3 (Berkeley) 4/2/94";
#else
__RCSID("$NetBSD: sleep.c,v 1.10 1997/08/04 01:13:10 perry Exp $");
#endif
#endif /* not lint */

/*
 * XXX shouldn't need sys/time.h, but there was an include file bug
 * which may be fixed soon.
 */
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>

void usage __P((void));
void alarmhandle __P((int));
int  main __P((int, char *[]));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char *arg, *temp;
	double val, ival, fval;
	struct timespec ntime;
	int fracflag;
	int ch;

	setlocale(LC_ALL, "");

	(void)signal(SIGALRM, alarmhandle);

	while ((ch = getopt(argc, argv, "")) != EOF)
		switch(ch) {
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	/*
	 * Okay, why not just use atof for everything? Why bother
	 * checking if there is a fraction in use? Because the old
	 * sleep handled the full range of integers, that's why, and a
	 * double can't handle a large long. This is fairly useless
	 * given how large a number a double can hold on most
	 * machines, but now we won't ever have trouble. If you want
	 * 1000000000.9 seconds of sleep, well, that's your
	 * problem. Why use an isdigit() check instead of checking for
	 * a period? Because doing it this way means locales will be
	 * handled transparently by the atof code.
	 */
	fracflag = 0;
	arg = *argv;
	for (temp = arg; *temp != '\0'; temp++)
		if (!isdigit(*temp))
			fracflag++;

	if (fracflag) {
		val = atof(arg);
		if (val <= 0)
			exit(0);
		ival = floor(val);
		fval = (1000000000 * (val-ival));
		ntime.tv_sec = ival;
		ntime.tv_nsec = fval;
	}
	else{
		ntime.tv_sec = atol(arg);
		if (ntime.tv_sec <= 0)
			exit(0);
		ntime.tv_nsec = 0;
	}

	(void)nanosleep(&ntime, NULL);

	exit(0);
}

void
usage()
{

	(void)fprintf(stderr, "usage: sleep seconds\n");
	exit(1);
}

void
alarmhandle(i)
	int i;
{
	_exit(0);
}
