/*	$NetBSD: move.c,v 1.10.6.1 2012/10/30 18:58:17 yamt Exp $	*/

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
static char sccsid[] = "@(#)move.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: move.c,v 1.10.6.1 2012/10/30 18:58:17 yamt Exp $");
#endif
#endif /* not lint */

#include <assert.h>

#include "back.h"
#include "backlocal.h"

#ifdef DEBUG
FILE   *trace;
static char tests[20];
#endif

struct BOARD {			/* structure of game position */
	int     b_board[26];	/* board position */
	int     b_in[2];	/* men in */
	int     b_off[2];	/* men off */
	int     b_st[4], b_fn[4];	/* moves */

	struct BOARD *b_next;	/* forward queue pointer */
};

static struct BOARD *freeq = 0;
static struct BOARD *checkq = 0;

 /* these variables are values for the candidate move */
static int ch;			/* chance of being hit */
static int op;			/* computer's open men */
static int pt;			/* comp's protected points */
static int em;			/* farthest man back */
static int frc;			/* chance to free comp's men */
static int frp;			/* chance to free pl's men */

 /* these values are the values for the move chosen (so far) */
static int chance;		/* chance of being hit */
static int openmen;		/* computer's open men */
static int points;		/* comp's protected points */
static int endman;		/* farthest man back */
static int barmen;		/* men on bar */
static int menin;		/* men in inner table */
static int menoff;		/* men off board */
static int oldfrc;		/* chance to free comp's men */
static int oldfrp;		/* chance to free pl's men */

static int cp[5];		/* candidate start position */
static int cg[5];		/* candidate finish position */

static int race;		/* game reduced to a race */


static int bcomp(struct BOARD *, struct BOARD *);
static struct BOARD *bsave(struct move *);
static void binsert(struct move *, struct BOARD *);
static void boardcopy(struct move *, struct BOARD *);
static void makefree(struct BOARD *);
static void mvcheck(struct move *, struct BOARD *, struct BOARD *);
static struct BOARD *nextfree(void);
static void trymove(struct move *, int, int);
static void pickmove(struct move *);
static void movcmp(struct move *);
static int movegood(void);


/* zero if first move */
void
move(struct move *mm, int okay)
{
	int     i;		/* index */
	int     l;		/* last man */

	l = 0;
	if (okay) {
		/* see if comp should double */
		if (gvalue < 64 && dlast != cturn && dblgood()) {
			writel(*Colorptr);
			dble();	/* double */
			/* return if declined */
			if (cturn != 1 && cturn != -1)
				return;
		}
		roll(mm);
	}
	race = 0;
	for (i = 0; i < 26; i++) {
		if (board[i] < 0)
			l = i;
	}
	for (i = 0; i < l; i++) {
		if (board[i] > 0)
			break;
	}
	if (i == l)
		race = 1;

	/* print roll */
	if (tflag)
		curmove(cturn == -1 ? 18 : 19, 0);
	writel(*Colorptr);
	writel(" rolls ");
	writec(mm->D0 + '0');
	writec(' ');
	writec(mm->D1 + '0');
	/* make tty interruptable while thinking */
	if (tflag)
		cline();
	fixtty(&noech);

	/* find out how many moves */
	mm->mvlim = movallow(mm);
	if (mm->mvlim == 0) {
		writel(" but cannot use it.\n");
		nexturn();
		fixtty(&raw);
		return;
	}
	/* initialize */
	for (i = 0; i < 4; i++)
		cp[i] = cg[i] = 0;

	/* strategize */
	trymove(mm, 0, 0);
	pickmove(mm);

	/* print move */
	writel(" and moves ");
	for (i = 0; i < mm->mvlim; i++) {
		if (i > 0)
			writec(',');
		wrint(mm->p[i] = cp[i]);
		writec('-');
		wrint(mm->g[i] = cg[i]);
		makmove(mm, i);

		/*
		 * This assertion persuades gcc 4.5 that the loop
		 * doesn't result in signed overflow of i. mvlim
		 * isn't, or at least shouldn't be, changed by makmove
		 * at all.
		 */
		assert(mm->mvlim >= 0 && mm->mvlim <= 5);
	}
	writec('.');

	/* print blots hit */
	if (tflag)
		curmove(20, 0);
	else
		writec('\n');
	for (i = 0; i < mm->mvlim; i++)
		if (mm->h[i])
			wrhit(mm->g[i]);
	/* get ready for next move */
	nexturn();
	if (!okay) {
		buflush();
		sleep(3);
	}
	fixtty(&raw);		/* no more tty interrupt */
}

/* 	mvnum   == number of move (rel zero) */
/* 	swapped == see if swapped also tested */
static void
trymove(struct move *mm, int mvnum, int swapped)
{
	int     pos;		/* position on board */
	int     rval;		/* value of roll */

	/* if recursed through all dice values, compare move */
	if (mvnum == mm->mvlim) {
		binsert(mm, bsave(mm));
		return;
	}
	/* make sure dice in always same order */
	if (mm->d0 == swapped)
		mswap(mm);
	/* choose value for this move */
	rval = mm->dice[mvnum != 0];

	/* find all legitimate moves */
	for (pos = bar; pos != home; pos += cturn) {
		/* fix order of dice */
		if (mm->d0 == swapped)
			mswap(mm);
		/* break if stuck on bar */
		if (board[bar] != 0 && pos != bar)
			break;
		/* on to next if not occupied */
		if (board[pos] * cturn <= 0)
			continue;
		/* set up arrays for move */
		mm->p[mvnum] = pos;
		mm->g[mvnum] = pos + rval * cturn;
		if (mm->g[mvnum] * cturn >= home) {
			if (*offptr < 0)
				break;
			mm->g[mvnum] = home;
		}
		/* try to move */
		if (makmove(mm, mvnum))
			continue;
		else
			trymove(mm, mvnum + 1, 2);
		/* undo move to try another */
		backone(mm, mvnum);
	}

	/* swap dice and try again */
	if ((!swapped) && mm->D0 != mm->D1)
		trymove(mm, 0, 1);
}

static struct BOARD *
bsave(struct move *mm)
{
	int     i;		/* index */
	struct BOARD *now;	/* current position */

	now = nextfree();	/* get free BOARD */

	/* store position */
	for (i = 0; i < 26; i++)
		now->b_board[i] = board[i];
	now->b_in[0] = in[0];
	now->b_in[1] = in[1];
	now->b_off[0] = off[0];
	now->b_off[1] = off[1];
	for (i = 0; i < mm->mvlim; i++) {
		now->b_st[i] = mm->p[i];
		now->b_fn[i] = mm->g[i];
	}
	return (now);
}

/* new == item to insert */
static void
binsert(struct move *mm, struct BOARD *new)
{
	struct BOARD *qp = checkq;	/* queue pointer */
	int     result;		/* comparison result */

	if (qp == 0) {		/* check if queue empty */
		checkq = qp = new;
		qp->b_next = 0;
		return;
	}
	result = bcomp(new, qp);	/* compare to first element */
	if (result < 0) {	/* insert in front */
		new->b_next = qp;
		checkq = new;
		return;
	}
	if (result == 0) {	/* duplicate entry */
		mvcheck(mm, qp, new);
		makefree(new);
		return;
	}
	while (qp->b_next != 0) {/* traverse queue */
		result = bcomp(new, qp->b_next);
		if (result < 0) {	/* found place */
			new->b_next = qp->b_next;
			qp->b_next = new;
			return;
		}
		if (result == 0) {	/* duplicate entry */
			mvcheck(mm, qp->b_next, new);
			makefree(new);
			return;
		}
		qp = qp->b_next;
	}
	/* place at end of queue */
	qp->b_next = new;
	new->b_next = 0;
}

static int
bcomp(struct BOARD *a, struct BOARD *b)
{
	int    *aloc = a->b_board;	/* pointer to board a */
	int    *bloc = b->b_board;	/* pointer to board b */
	int     i;		/* index */
	int     result;		/* comparison result */

	for (i = 0; i < 26; i++) {	/* compare boards */
		result = cturn * (aloc[i] - bloc[i]);
		if (result)
			return (result);	/* found inequality */
	}
	return (0);		/* same position */
}

static void
mvcheck(struct move *mm, struct BOARD *incumbent, struct BOARD *candidate)
{
	int     i;
	int     result;

	for (i = 0; i < mm->mvlim; i++) {
		result = cturn * (candidate->b_st[i] - incumbent->b_st[i]);
		if (result > 0)
			return;
		if (result < 0)
			break;
	}
	if (i == mm->mvlim)
		return;
	for (i = 0; i < mm->mvlim; i++) {
		incumbent->b_st[i] = candidate->b_st[i];
		incumbent->b_fn[i] = candidate->b_fn[i];
	}
}

void
makefree(struct BOARD *dead)
{
	dead->b_next = freeq;	/* add to freeq */
	freeq = dead;
}

static struct BOARD *
nextfree(void)
{
	struct BOARD *new;

	if (freeq == 0) {
		new = (struct BOARD *) calloc(1, sizeof(struct BOARD));
		if (new == 0) {
			writel("\nOut of memory\n");
			getout(0);
		}
	} else {
		new = freeq;
		freeq = freeq->b_next;
	}

	new->b_next = 0;
	return (new);
}

static void
pickmove(struct move *mm)
{
	/* current game position */
	struct BOARD *now = bsave(mm);
	struct BOARD *next;	/* next move */

#ifdef DEBUG
	if (trace == NULL)
		trace = fopen("bgtrace", "w");
	fprintf(trace, "\nRoll:  %d %d%s\n", D0, D1, race ? " (race)" : "");
	fflush(trace);
#endif
	do {			/* compare moves */
		boardcopy(mm, checkq);
		next = checkq->b_next;
		makefree(checkq);
		checkq = next;
		movcmp(mm);
	} while (checkq != 0);

	boardcopy(mm, now);
}

static void
boardcopy(struct move *mm, struct BOARD *s)
{
	int     i;		/* index */

	for (i = 0; i < 26; i++)
		board[i] = s->b_board[i];
	for (i = 0; i < 2; i++) {
		in[i] = s->b_in[i];
		off[i] = s->b_off[i];
	}
	for (i = 0; i < mm->mvlim; i++) {
		mm->p[i] = s->b_st[i];
		mm->g[i] = s->b_fn[i];
	}
}

static void
movcmp(struct move *mm)
{
	int     i;

#ifdef DEBUG
	if (trace == NULL)
		trace = fopen("bgtrace", "w");
#endif

	odds(0, 0, 0);
	if (!race) {
		ch = op = pt = 0;
		for (i = 1; i < 25; i++) {
			if (board[i] == cturn)
				ch = canhit(i, 1);
			op += abs(bar - i);
		}
		for (i = bar + cturn; i != home; i += cturn)
			if (board[i] * cturn > 1)
				pt += abs(bar - i);
		frc = freemen(bar) + trapped(bar, cturn);
		frp = freemen(home) + trapped(home, -cturn);
	}
	for (em = bar; em != home; em += cturn)
		if (board[em] * cturn > 0)
			break;
	em = abs(home - em);
#ifdef DEBUG
	fputs("Board: ", trace);
	for (i = 0; i < 26; i++)
		fprintf(trace, " %d", board[i]);
	if (race)
		fprintf(trace, "\n\tem = %d\n", em);
	else
		fprintf(trace,
		    "\n\tch = %d, pt = %d, em = %d, frc = %d, frp = %d\n",
		    ch, pt, em, frc, frp);
	fputs("\tMove: ", trace);
	for (i = 0; i < mvlim; i++)
		fprintf(trace, " %d-%d", p[i], g[i]);
	fputs("\n", trace);
	fflush(trace);
	strcpy(tests, "");
#endif
	if ((cp[0] == 0 && cg[0] == 0) || movegood()) {
#ifdef DEBUG
		fprintf(trace, "\t[%s] ... wins.\n", tests);
		fflush(trace);
#endif
		for (i = 0; i < mm->mvlim; i++) {
			cp[i] = mm->p[i];
			cg[i] = mm->g[i];
		}
		if (!race) {
			chance = ch;
			openmen = op;
			points = pt;
			endman = em;
			barmen = abs(board[home]);
			oldfrc = frc;
			oldfrp = frp;
		}
		menin = *inptr;
		menoff = *offptr;
	}
#ifdef DEBUG
	else {
		fprintf(trace, "\t[%s] ... loses.\n", tests);
		fflush(trace);
	}
#endif
}

static int
movegood(void)
{
	int     n;

	if (*offptr == 15)
		return (1);
	if (menoff == 15)
		return (0);
	if (race) {
#ifdef DEBUG
		strcat(tests, "o");
#endif
		if (*offptr - menoff)
			return (*offptr > menoff);
#ifdef DEBUG
		strcat(tests, "e");
#endif
		if (endman - em)
			return (endman > em);
#ifdef DEBUG
		strcat(tests, "i");
#endif
		if (menin == 15)
			return (0);
		if (*inptr == 15)
			return (1);
#ifdef DEBUG
		strcat(tests, "i");
#endif
		if (*inptr - menin)
			return (*inptr > menin);
		return (rnum(2));
	} else {
		n = barmen - abs(board[home]);
#ifdef DEBUG
		strcat(tests, "c");
#endif
		if (abs(chance - ch) + 25 * n > rnum(150))
			return (n ? (n < 0) : (ch < chance));
#ifdef DEBUG
		strcat(tests, "o");
#endif
		if (*offptr - menoff)
			return (*offptr > menoff);
#ifdef DEBUG
		strcat(tests, "o");
#endif
		if (abs(openmen - op) > 7 + rnum(12))
			return (openmen > op);
#ifdef DEBUG
		strcat(tests, "b");
#endif
		if (n)
			return (n < 0);
#ifdef DEBUG
		strcat(tests, "e");
#endif
		if (abs(endman - em) > rnum(2))
			return (endman > em);
#ifdef DEBUG
		strcat(tests, "f");
#endif
		if (abs(frc - oldfrc) > rnum(2))
			return (frc < oldfrc);
#ifdef DEBUG
		strcat(tests, "p");
#endif
		if (abs(n = pt - points) > rnum(4))
			return (n > 0);
#ifdef DEBUG
		strcat(tests, "i");
#endif
		if (*inptr - menin)
			return (*inptr > menin);
#ifdef DEBUG
		strcat(tests, "f");
#endif
		if (abs(frp - oldfrp) > rnum(2))
			return (frp > oldfrp);
		return (rnum(2));
	}
}
