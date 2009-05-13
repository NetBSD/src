/*	$NetBSD: assorted.c,v 1.15.40.1 2009/05/13 19:18:05 jym Exp $	*/

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
#if 0
static char sccsid[] = "@(#)assorted.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: assorted.c,v 1.15.40.1 2009/05/13 19:18:05 jym Exp $");
#endif
#endif /* not lint */

#include <stdlib.h>
#include <err.h>
#include "extern.h"

static void strike(struct ship *, struct ship *);

void
table(struct ship *from, struct ship *on,
      int rig, int shot, int hittable, int roll)
{
	int hhits = 0, chits = 0, ghits = 0, rhits = 0;
	int Ghit = 0, Hhit = 0, Rhit = 0, Chit = 0;
	int guns, car, pc, hull;
	int crew[3];
	int n;
	int rigg[4];
	const char *message;
	const struct Tables *tp;

	pc = on->file->pcrew;
	hull = on->specs->hull;
	crew[0] = on->specs->crew1;
	crew[1] = on->specs->crew2;
	crew[2] = on->specs->crew3;
	rigg[0] = on->specs->rig1;
	rigg[1] = on->specs->rig2;
	rigg[2] = on->specs->rig3;
	rigg[3] = on->specs->rig4;
	if (shot == L_GRAPE) {
		Chit = chits = hittable;
	} else {
		tp = &(rig ? RigTable : HullTable)[hittable][roll-1];
		Chit = chits = tp->C;
		Rhit = rhits = tp->R;
		Hhit = hhits = tp->H;
		Ghit = ghits = tp->G;
		if (on->file->FS)
			rhits *= 2;
		if (shot == L_CHAIN) {
			Ghit = ghits = 0;
			Hhit = hhits = 0;
		}
	}
	if (on->file->captured != 0) {
		pc -= (chits + 1) / 2;
		chits /= 2;
	}
	for (n = 0; n < 3; n++) {
		if (chits > crew[n]) {
			chits -= crew[n];
			crew[n] = 0;
		} else {
			crew[n] -= chits;
			chits = 0;
		}
	}
	for (n = 0; n < 3; n++) {
		if (rhits > rigg[n]) {
			rhits -= rigg[n];
			rigg[n] = 0;
		} else {
			rigg[n] -= rhits;
			rhits = 0;
		}
	}
	if (rigg[3] != -1 && rhits > rigg[3]) {
		rhits -= rigg[3];
		rigg[3] = 0;
	} else if (rigg[3] != -1) {
		rigg[3] -= rhits;
	}
	if (rig && !rigg[2] && (!rigg[3] || rigg[3] == -1))
		makemsg(on, "dismasted!");
	if (portside(from, on, 0)) {
		guns = on->specs->gunR;
		car = on->specs->carR;
	} else {
		guns = on->specs->gunL;
		car = on->specs->carL;
	}
	if (ghits > car) {
		ghits -= car;
		car = 0;
	} else {
		car -= ghits;
		ghits = 0;
	}
	if (ghits > guns) {
		ghits -= guns;
		guns = 0;
	} else {
		guns -= ghits;
		ghits = 0;
	}
	hull -= ghits;
	if (Ghit) {
		if (portside(from, on, 0)) {
			send_gunr(on, guns, car);
		} else {
			send_gunl(on, guns, car);
		}
	}
	hull -= hhits;
	hull = hull < 0 ? 0 : hull;
	if (on->file->captured != 0 && Chit)
		send_pcrew(on, pc);
	if (Hhit)
		send_hull(on, hull);
	if (Chit)
		send_crew(on, crew[0], crew[1], crew[2]);
	if (Rhit)
		send_rigg(on, rigg[0], rigg[1], rigg[2], rigg[3]);
	switch (shot) {
	case L_ROUND:
		message = "firing round shot on $$";
		break;
	case L_GRAPE:
		message = "firing grape shot on $$";
		break;
	case L_CHAIN:
		message = "firing chain shot on $$";
		break;
	case L_DOUBLE:
		message = "firing double shot on $$";
		break;
	case L_EXPLODE:
		message = "exploding shot on $$";
		break;
	default:
		errx(1, "Unknown shot type %d", shot);

	}
	makesignal(from, message, on);
	if (roll == 6 && rig) {
		switch(Rhit) {
		case 0:
			message = "fore topsail sheets parted";
			break;
		case 1:
			message = "mizzen shrouds parted";
			break;
		case 2:
			message = "main topsail yard shot away";
			break;
		case 4:
			message = "fore topmast and foremast shrouds shot away";
			break;
		case 5:
			message = "mizzen mast and yard shot through";
			break;
		case 6:
			message = "foremast and spritsail yard shattered";
			break;
		case 7:
			message = "main topmast and mizzen mast shattered";
			break;
		default:
			errx(1, "Bad Rhit = %d", Rhit);
		}
		makemsg(on, message);
	} else if (roll == 6) {
		switch (Hhit) {
		case 0:
			message = "anchor cables severed";
			break;
		case 1:
			message = "two anchor stocks shot away";
			break;
		case 2:
			message = "quarterdeck bulwarks damaged";
			break;
		case 3:
			message = "three gun ports shot away";
			break;
		case 4:
			message = "four guns dismounted";
			break;
		case 5:
			message = "rudder cables shot through";
			send_ta(on, 0);
			break;
		case 6:
			message = "shot holes below the water line";
			break;
		default:
			errx(1, "Bad Hhit = %d", Hhit);
		}
		makemsg(on, message);
	}
	/*
	if (Chit > 1 && on->file->readyL & R_INITIAL &&
	    on->file->readyR & R_INITIAL) {
		on->specs->qual--;
		if (on->specs->qual <= 0) {
			makemsg(on, "crew mutinying!");
			on->specs->qual = 5;
			Write(W_CAPTURED, on, on->file->index, 0, 0, 0);
		} else {
			makemsg(on, "crew demoralized");
		}
		Write(W_QUAL, on, on->specs->qual, 0, 0, 0);
	}
	*/
	if (!hull)
		strike(on, from);
}

void
Cleansnag(struct ship *from, struct ship *to, int all, int flag)
{
	if (flag & 1) {
		send_ungrap(from, to->file->index, all);
		send_ungrap(to, from->file->index, all);
	}
	if (flag & 2) {
		send_unfoul(from, to->file->index, all);
		send_unfoul(to, from->file->index, all);
	}
	if (!snagged2(from, to)) {
		if (!snagged(from)) {
			unboard(from, from, 1);		/* defense */
			unboard(from, from, 0);		/* defense */
		} else
			unboard(from, to, 0);		/* offense */
		if (!snagged(to)) {
			unboard(to, to, 1);		/* defense */
			unboard(to, to, 0);		/* defense */
		} else
			unboard(to, from, 0);		/* offense */
	}
}

static void
strike(struct ship *ship, struct ship *from)
{
	int points;

	if (ship->file->struck)
		return;
	send_struck(ship, 1);
	points = ship->specs->pts + from->file->points;
	send_points(from, points);
	unboard(ship, ship, 0);		/* all offense */
	unboard(ship, ship, 1);		/* all defense */
	switch (dieroll()) {
	case 3:
	case 4:		/* ship may sink */
		send_sink(ship, 1);
		break;
	case 5:
	case 6:		/* ship may explode */
		send_explode(ship, 1);
		break;
	}
	send_signal(ship, "striking her colours!");
}
