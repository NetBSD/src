/*	$NetBSD: moreobj.c,v 1.11 2008/02/19 06:05:26 dholland Exp $	*/

/*
 * moreobj.c 		Larn is copyrighted 1986 by Noah Morgan.
 * 
 * Routines in this file:
 * 
 * oaltar() othrone() ochest() ofountain()
 */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: moreobj.c,v 1.11 2008/02/19 06:05:26 dholland Exp $");
#endif				/* not lint */
#include <stdlib.h>
#include <unistd.h>
#include "header.h"
#include "extern.h"

static void fch(int, long *);

/*
 *	subroutine to process an altar object
 */
void
oaltar()
{

	lprcat("\nDo you (p) pray  (d) desecrate");
	iopts();
	while (1) {
		while (1)
			switch (ttgetch()) {
			case 'p':
				lprcat(" pray\nDo you (m) give money or (j) just pray? ");
				while (1)
					switch (ttgetch()) {
					case 'j':
						act_just_pray();
						return;

					case 'm':
						act_donation_pray();
						return;

					case '\33':
						return;
					};

			case 'd':
				lprcat(" desecrate");
				act_desecrate_altar();
				return;

			case 'i':
			case '\33':
				ignore();
				act_ignore_altar();
				return;
			};
	}
}

/*
	subroutine to process a throne object
 */
void
othrone(arg)
	int             arg;
{

	lprcat("\nDo you (p) pry off jewels, (s) sit down");
	iopts();
	while (1) {
		while (1)
			switch (ttgetch()) {
			case 'p':
				lprcat(" pry off");
				act_remove_gems(arg);
				return;

			case 's':
				lprcat(" sit down");
				act_sit_throne(arg);
				return;

			case 'i':
			case '\33':
				ignore();
				return;
			};
	}
}

void
odeadthrone()
{
	int    k;

	lprcat("\nDo you (s) sit down");
	iopts();
	while (1) {
		while (1)
			switch (ttgetch()) {
			case 's':
				lprcat(" sit down");
				k = rnd(101);
				if (k < 35) {
					lprcat("\nZaaaappp!  You've been teleported!\n");
					beep();
					oteleport(0);
				} else
					lprcat("\nnothing happens");
				return;

			case 'i':
			case '\33':
				ignore();
				return;
			};
	}
}

/*
	subroutine to process a throne object
 */
void
ochest()
{

	lprcat("\nDo you (t) take it, (o) try to open it");
	iopts();
	while (1) {
		while (1)
			switch (ttgetch()) {
			case 'o':
				lprcat(" open it");
				act_open_chest(playerx, playery);
				return;

			case 't':
				lprcat(" take");
				if (take(OCHEST, iarg[playerx][playery]) == 0)
					item[playerx][playery] = know[playerx][playery] = 0;
				return;

			case 'i':
			case '\33':
				ignore();
				return;
			};
	}
}

/*
	process a fountain object
 */
void
ofountain()
{

	cursors();
	lprcat("\nDo you (d) drink, (w) wash yourself");
	iopts();
	while (1)
		switch (ttgetch()) {
		case 'd':
			lprcat("drink");
			act_drink_fountain();
			return;

		case '\33':
		case 'i':
			ignore();
			return;

		case 'w':
			lprcat("wash yourself");
			act_wash_fountain();
			return;
		}
}

/*
	***
	FCH
	***

	subroutine to process an up/down of a character attribute for ofountain
 */
static void
fch(how, x)
	int             how;
	long           *x;
{
	if (how < 0) {
		lprcat(" went down by one!");
		--(*x);
	} else {
		lprcat(" went up by one!");
		(*x)++;
	}
	bottomline();
}

/*
	a subroutine to raise or lower character levels
	if x > 0 they are raised   if x < 0 they are lowered
 */
void
fntchange(how)
	int             how;
{
	long   j;
	lprc('\n');
	switch (rnd(9)) {
	case 1:
		lprcat("Your strength");
		fch(how, &c[0]);
		break;
	case 2:
		lprcat("Your intelligence");
		fch(how, &c[1]);
		break;
	case 3:
		lprcat("Your wisdom");
		fch(how, &c[2]);
		break;
	case 4:
		lprcat("Your constitution");
		fch(how, &c[3]);
		break;
	case 5:
		lprcat("Your dexterity");
		fch(how, &c[4]);
		break;
	case 6:
		lprcat("Your charm");
		fch(how, &c[5]);
		break;
	case 7:
		j = rnd(level + 1);
		if (how < 0) {
			lprintf("You lose %ld hit point", (long) j);
			if (j > 1)
				lprcat("s!");
			else
				lprc('!');
			losemhp((int) j);
		} else {
			lprintf("You gain %ld hit point", (long) j);
			if (j > 1)
				lprcat("s!");
			else
				lprc('!');
			raisemhp((int) j);
		}
		bottomline();
		break;

	case 8:
		j = rnd(level + 1);
		if (how > 0) {
			lprintf("You just gained %ld spell", (long) j);
			raisemspells((int) j);
			if (j > 1)
				lprcat("s!");
			else
				lprc('!');
		} else {
			lprintf("You just lost %ld spell", (long) j);
			losemspells((int) j);
			if (j > 1)
				lprcat("s!");
			else
				lprc('!');
		}
		bottomline();
		break;

	case 9:
		j = 5 * rnd((level + 1) * (level + 1));
		if (how < 0) {
			lprintf("You just lost %ld experience point", (long) j);
			if (j > 1)
				lprcat("s!");
			else
				lprc('!');
			loseexperience((long) j);
		} else {
			lprintf("You just gained %ld experience point", (long) j);
			if (j > 1)
				lprcat("s!");
			else
				lprc('!');
			raiseexperience((long) j);
		}
		break;
	}
	cursors();
}
