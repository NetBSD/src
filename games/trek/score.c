/*	$NetBSD: score.c,v 1.10 2009/05/25 00:03:18 dholland Exp $	*/

/*
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
#if 0
static char sccsid[] = "@(#)score.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: score.c,v 1.10 2009/05/25 00:03:18 dholland Exp $");
#endif
#endif /* not lint */

#include <stdarg.h>
#include <stdio.h>
#include "trek.h"
#include "getpar.h"

/*
**  PRINT OUT THE CURRENT SCORE
*/

static void scoreitem(long amount, const char *descfmt, ...)
	__printflike(2, 3);

static void
scoreitem(long amount, const char *descfmt, ...)
{
	va_list ap;
	char buf[128];

	if (amount == 0)
		return;

	va_start(ap, descfmt);
	vsnprintf(buf, sizeof(buf), descfmt, ap);
	va_end(ap);

	printf("%-40s %10ld\n", buf, amount);
}

long
score(void)
{
	int		u;
	int		t;
	long		s;
	double		r;

	printf("\n*** Your score:\n");

	s = t = Param.klingpwr / 4 * (u = Game.killk);
	scoreitem(t, "%d Klingons killed", u);

	r = Now.date - Param.date;
	if (r < 1.0)
		r = 1.0;
	r = Game.killk / r;
	s += (t = 400 * r);
	scoreitem(t, "Kill rate %.2f Klingons/stardate", r);

	r = Now.klings;
	r /= Game.killk + 1;
	s += (t = -400 * r);
	scoreitem(t, "Penalty for %d klingons remaining", Now.klings);

	if (Move.endgame > 0) {
		s += (t = 100 * (u = Game.skill));
		scoreitem(t, "Bonus for winning a %s%s game",
			Skitab[u - 1].abrev, Skitab[u - 1].full);
	}

	if (Game.killed) {
		s -= 500;
		scoreitem(-500, "Penalty for getting killed");
	}

	s += (t = -100 * (u = Game.killb));
	scoreitem(t, "%d starbases killed", u);

	s += (t = -100 * (u = Game.helps));
	scoreitem(t, "%d calls for help", u);

	s += (t = -5 * (u = Game.kills));
	scoreitem(t, "%d stars destroyed", u);

	s += (t = -150 * (u = Game.killinhab));
	scoreitem(t, "%d inhabited starsystems destroyed", u);

	if (Ship.ship != ENTERPRISE) {
		s -= 200;
		scoreitem(-200, "penalty for abandoning ship");
	}

	s += (t = 3 * (u = Game.captives));
	scoreitem(t, "%d Klingons captured", u);

	s += (t = -(u = Game.deaths));
	scoreitem(t, "%d casualties", u);

	printf("\n");
	scoreitem(s, "***  TOTAL");
	return (s);
}
