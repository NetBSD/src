/*	$NetBSD: pom.c,v 1.13 2003/08/07 09:37:32 agc Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software posted to USENET.
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
static char sccsid[] = "@(#)pom.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: pom.c,v 1.13 2003/08/07 09:37:32 agc Exp $");
#endif
#endif /* not lint */

/*
 * Phase of the Moon.  Calculates the current phase of the moon.
 * Based on routines from `Practical Astronomy with Your Calculator',
 * by Duffett-Smith.  Comments give the section from the book that
 * particular piece of code was adapted from.
 *
 * -- Keith E. Brandt  VIII 1984
 *
 * Updated to the Third Edition of Duffett-Smith's book, Paul Janzen, IX 1998
 *
 */

#include <ctype.h>
#include <err.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#ifndef PI
#define	PI	  3.14159265358979323846
#endif

/*
 * The EPOCH in the third edition of the book is 1990 Jan 0.0 TDT.
 * In this program, we do not bother to correct for the differences
 * between UTC (as shown by the UNIX clock) and TDT.  (TDT = TAI + 32.184s;
 * TAI-UTC = 32s in Jan 1999.)
 */
#define EPOCH_MINUS_1970	(20 * 365 + 5 - 1) /* 20 years, 5 leaps, back 1 day to Jan 0 */
#define	EPSILONg  279.403303	/* solar ecliptic long at EPOCH */
#define	RHOg	  282.768422	/* solar ecliptic long of perigee at EPOCH */
#define	ECCEN	  0.016713	/* solar orbit eccentricity */
#define	lzero	  318.351648	/* lunar mean long at EPOCH */
#define	Pzero	  36.340410	/* lunar mean long of perigee at EPOCH */
#define	Nzero	  318.510107	/* lunar mean long of node at EPOCH */

void	adj360 __P((double *));
double	dtor __P((double));
int	main __P((int, char *[]));
double	potm __P((double));
time_t	parsetime __P((char *));
void	badformat __P((void)) __attribute__((__noreturn__));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	time_t tmpt, now;
	double days, today, tomorrow;
	char buf[1024];

	if (time(&now) == (time_t)-1)
		err(1, "time");
	if (argc > 1) {
		tmpt = parsetime(argv[1]);
		strftime(buf, sizeof(buf), "%a %Y %b %e %H:%M:%S (%Z)",
			localtime(&tmpt));
		printf("%s:  ", buf);
	} else {
		tmpt = now;
	}
	days = (tmpt - EPOCH_MINUS_1970 * 86400) / 86400.0;
	today = potm(days) + .5;
	if (tmpt < now)
		(void)printf("The Moon was ");
	else if (tmpt == now)
		(void)printf("The Moon is ");
	else
		(void)printf("The Moon will be ");
	if ((int)today == 100)
		(void)printf("Full\n");
	else if (!(int)today)
		(void)printf("New\n");
	else {
		tomorrow = potm(days + 1);
		if ((int)today == 50)
			(void)printf("%s\n", tomorrow > today ?
			    "at the First Quarter" : "at the Last Quarter");
			/* today is 0.5 too big, but it doesn't matter here
			 * since the phase is changing fast enough
			 */
		else {
			today -= 0.5;		/* Now it might matter */
			(void)printf("%s ", tomorrow > today ?
			    "Waxing" : "Waning");
			if (today > 50)
				(void)printf("Gibbous (%1.0f%% of Full)\n",
				    today);
			else if (today < 50)
				(void)printf("Crescent (%1.0f%% of Full)\n",
				    today);
		}
	}
	exit(0);
}

/*
 * potm --
 *	return phase of the moon
 */
double
potm(days)
	double days;
{
	double N, Msol, Ec, LambdaSol, l, Mm, Ev, Ac, A3, Mmprime;
	double A4, lprime, V, ldprime, D, Nm;

	N = 360 * days / 365.242191;				/* sec 46 #3 */
	adj360(&N);
	Msol = N + EPSILONg - RHOg;				/* sec 46 #4 */
	adj360(&Msol);
	Ec = 360 / PI * ECCEN * sin(dtor(Msol));		/* sec 46 #5 */
	LambdaSol = N + Ec + EPSILONg;				/* sec 46 #6 */
	adj360(&LambdaSol);
	l = 13.1763966 * days + lzero;				/* sec 65 #4 */
	adj360(&l);
	Mm = l - (0.1114041 * days) - Pzero;			/* sec 65 #5 */
	adj360(&Mm);
	Nm = Nzero - (0.0529539 * days);			/* sec 65 #6 */
	adj360(&Nm);
	Ev = 1.2739 * sin(dtor(2*(l - LambdaSol) - Mm));	/* sec 65 #7 */
	Ac = 0.1858 * sin(dtor(Msol));				/* sec 65 #8 */
	A3 = 0.37 * sin(dtor(Msol));
	Mmprime = Mm + Ev - Ac - A3;				/* sec 65 #9 */
	Ec = 6.2886 * sin(dtor(Mmprime));			/* sec 65 #10 */
	A4 = 0.214 * sin(dtor(2 * Mmprime));			/* sec 65 #11 */
	lprime = l + Ev + Ec - Ac + A4;				/* sec 65 #12 */
	V = 0.6583 * sin(dtor(2 * (lprime - LambdaSol)));	/* sec 65 #13 */
	ldprime = lprime + V;					/* sec 65 #14 */
	D = ldprime - LambdaSol;				/* sec 67 #2 */
	return(50.0 * (1 - cos(dtor(D))));			/* sec 67 #3 */
}

/*
 * dtor --
 *	convert degrees to radians
 */
double
dtor(deg)
	double deg;
{
	return(deg * PI / 180);
}

/*
 * adj360 --
 *	adjust value so 0 <= deg <= 360
 */
void
adj360(deg)
	double *deg;
{
	for (;;)
		if (*deg < 0)
			*deg += 360;
		else if (*deg > 360)
			*deg -= 360;
		else
			break;
}

#define	ATOI2(ar)	((ar)[0] - '0') * 10 + ((ar)[1] - '0'); (ar) += 2;
time_t
parsetime(p)
	char *p;
{
	struct tm *lt;
	int bigyear;
	int yearset = 0;
	time_t tval;
	unsigned char *t;
	
	for (t = p; *t; ++t) {
		if (isdigit(*t))
			continue;
		badformat();
	}

	tval = time(NULL);
	lt = localtime(&tval);
	lt->tm_sec = 0;
	lt->tm_min = 0;

	switch (strlen(p)) {
	case 10:				/* yyyy */
		bigyear = ATOI2(p);
		lt->tm_year = bigyear * 100 - 1900;
		yearset = 1;
		/* FALLTHROUGH */
	case 8:					/* yy */
		if (yearset) {
			lt->tm_year += ATOI2(p);
		} else {
			lt->tm_year = ATOI2(p);
			if (lt->tm_year < 69)		/* hack for 2000 */
				lt->tm_year += 100;
		}
		/* FALLTHROUGH */
	case 6:					/* mm */
		lt->tm_mon = ATOI2(p);
		if ((lt->tm_mon > 12) || !lt->tm_mon)
			badformat();
		--lt->tm_mon;			/* time struct is 0 - 11 */
		/* FALLTHROUGH */
	case 4:					/* dd */
		lt->tm_mday = ATOI2(p);
		if ((lt->tm_mday > 31) || !lt->tm_mday)
			badformat();
		/* FALLTHROUGH */
	case 2:					/* HH */
		lt->tm_hour = ATOI2(p);
		if (lt->tm_hour > 23)
			badformat();
		break;
	default:
		badformat();
	}
	/* The calling code needs a valid tm_ydays and this is the easiest
	 * way to get one */
	if ((tval = mktime(lt)) == -1)
		errx(1, "specified date is outside allowed range");
	return (tval);
}

void
badformat()
{
	warnx("illegal time format");
	(void)fprintf(stderr, "usage: pom [[[[[cc]yy]mm]dd]HH]\n");
	exit(1);
}
