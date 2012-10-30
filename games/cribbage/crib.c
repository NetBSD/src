/*	$NetBSD: crib.c,v 1.24.6.1 2012/10/30 18:58:19 yamt Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)crib.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: crib.c,v 1.24.6.1 2012/10/30 18:58:19 yamt Exp $");
#endif
#endif /* not lint */

#include <curses.h>
#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "deck.h"
#include "cribbage.h"
#include "cribcur.h"
#include "pathnames.h"

static void makeboard(void);
static void gamescore(void);
static void game(void);
static int playhand(BOOLEAN);
static int deal(BOOLEAN);
static void discard(BOOLEAN);
static int cut(BOOLEAN, int);
static void prcrib(BOOLEAN, BOOLEAN);
static int peg(BOOLEAN);
static void prtable(int);
static int score(BOOLEAN);

int
main(int argc, char *argv[])
{
	BOOLEAN playing;
	FILE *f;
	int ch;
	int fd;
	int flags;

	f = fopen(_PATH_LOG, "a");
	if (f == NULL)
		warn("fopen %s", _PATH_LOG);
	if (f != NULL && fileno(f) < 3)
		exit(1);

	/* Revoke setgid privileges */
	setgid(getgid());

	/* Set close-on-exec flag on log file */
	if (f != NULL) {
		fd = fileno(f);
		flags = fcntl(fd, F_GETFD);
		if (flags < 0)
			err(1, "fcntl F_GETFD");
		flags |= FD_CLOEXEC;
		if (fcntl(fd, F_SETFD, flags) == -1)
			err(1, "fcntl F_SETFD");
	}

	while ((ch = getopt(argc, argv, "eqr")) != -1)
		switch (ch) {
		case 'e':
			explain = TRUE;
			break;
		case 'q':
			quiet = TRUE;
			break;
		case 'r':
			rflag = TRUE;
			break;
		case '?':
		default:
			(void) fprintf(stderr, "usage: cribbage [-eqr]\n");
			exit(1);
		}

	if (!initscr())
		errx(0, "couldn't initialize screen");
	(void)signal(SIGINT, receive_intr);
	cbreak();
	noecho();

	Playwin = subwin(stdscr, PLAY_Y, PLAY_X, 0, 0);
	Tablewin = subwin(stdscr, TABLE_Y, TABLE_X, 0, PLAY_X);
	Compwin = subwin(stdscr, COMP_Y, COMP_X, 0, TABLE_X + PLAY_X);
	Msgwin = subwin(stdscr, MSG_Y, MSG_X, Y_MSG_START, SCORE_X + 1);
	leaveok(Playwin, TRUE);
	leaveok(Tablewin, TRUE);
	leaveok(Compwin, TRUE);
	clearok(stdscr, FALSE);

	if (!quiet) {
		msg("Do you need instructions for cribbage? ");
		if (getuchar() == 'Y') {
			endwin();
			clear();
			mvcur(0, COLS - 1, LINES - 1, 0);
			fflush(stdout);
			instructions();
			cbreak();
			noecho();
			clear();
			refresh();
			msg("For cribbage rules, use \"man cribbage\"");
		}
	}
	playing = TRUE;
	do {
		wclrtobot(Msgwin);
		msg(quiet ? "L or S? " : "Long (to 121) or Short (to 61)? ");
		if (glimit == SGAME)
			glimit = (getuchar() == 'L' ? LGAME : SGAME);
		else
			glimit = (getuchar() == 'S' ? SGAME : LGAME);
		game();
		msg("Another game? ");
		playing = (getuchar() == 'Y');
	} while (playing);

	if (f != NULL) {
		(void)fprintf(f, "%s: won %5.5d, lost %5.5d\n",
		    getlogin(), cgames, pgames);
		(void) fclose(f);
	}
	bye();
	exit(0);
}

/*
 * makeboard:
 *	Print out the initial board on the screen
 */
static void
makeboard(void)
{
	mvaddstr(SCORE_Y + 0, SCORE_X,
	    "+---------------------------------------+");
	mvaddstr(SCORE_Y + 1, SCORE_X,
	    "|  Score:   0     YOU                   |");
	mvaddstr(SCORE_Y + 2, SCORE_X,
	    "| *.....:.....:.....:.....:.....:.....  |");
	mvaddstr(SCORE_Y + 3, SCORE_X,
	    "| *.....:.....:.....:.....:.....:.....  |");
	mvaddstr(SCORE_Y + 4, SCORE_X,
	    "|                                       |");
	mvaddstr(SCORE_Y + 5, SCORE_X,
	    "| *.....:.....:.....:.....:.....:.....  |");
	mvaddstr(SCORE_Y + 6, SCORE_X,
	    "| *.....:.....:.....:.....:.....:.....  |");
	mvaddstr(SCORE_Y + 7, SCORE_X,
	    "|  Score:   0      ME                   |");
	mvaddstr(SCORE_Y + 8, SCORE_X,
	    "+---------------------------------------+");
	gamescore();
}

/*
 * gamescore:
 *	Print out the current game score
 */
static void
gamescore(void)
{
	if (pgames || cgames) {
		mvprintw(SCORE_Y + 1, SCORE_X + 28, "Games: %3d", pgames);
		mvprintw(SCORE_Y + 7, SCORE_X + 28, "Games: %3d", cgames);
	}
	Lastscore[0] = -1;
	Lastscore[1] = -1;
}

/*
 * game:
 *	Play one game up to glimit points.  Actually, we only ASK the
 *	player what card to turn.  We do a random one, anyway.
 */
static void
game(void)
{
	int i, j;
	BOOLEAN flag;
	BOOLEAN compcrib;

	compcrib = FALSE;
	makedeck(deck);
	shuffle(deck);
	if (gamecount == 0) {
		flag = TRUE;
		do {
			if (!rflag) {			/* player cuts deck */
				msg(quiet ? "Cut for crib? " :
			    "Cut to see whose crib it is -- low card wins? ");
				get_line();
			}
			i = (rand() >> 4) % CARDS;	/* random cut */
			do {	/* comp cuts deck */
				j = (rand() >> 4) % CARDS;
			} while (j == i);
			addmsg(quiet ? "You cut " : "You cut the ");
			msgcard(deck[i], FALSE);
			endmsg();
			addmsg(quiet ? "I cut " : "I cut the ");
			msgcard(deck[j], FALSE);
			endmsg();
			flag = (deck[i].rank == deck[j].rank);
			if (flag) {
				msg(quiet ? "We tied..." :
				    "We tied and have to try again...");
				shuffle(deck);
				continue;
			} else
				compcrib = (deck[i].rank > deck[j].rank);
		} while (flag);
		do_wait();
		clear();
		makeboard();
		refresh();
	} else {
		makeboard();
		refresh();
		werase(Tablewin);
		wrefresh(Tablewin);
		werase(Compwin);
		wrefresh(Compwin);
		msg("Loser (%s) gets first crib", (iwon ? "you" : "me"));
		compcrib = !iwon;
	}

	pscore = cscore = 0;
	flag = TRUE;
	do {
		shuffle(deck);
		flag = !playhand(compcrib);
		compcrib = !compcrib;
	} while (flag);
	++gamecount;
	if (cscore < pscore) {
		if (glimit - cscore > 60) {
			msg("YOU DOUBLE SKUNKED ME!");
			pgames += 4;
		} else
			if (glimit - cscore > 30) {
				msg("YOU SKUNKED ME!");
				pgames += 2;
			} else {
				msg("YOU WON!");
				++pgames;
			}
		iwon = FALSE;
	} else {
		if (glimit - pscore > 60) {
			msg("I DOUBLE SKUNKED YOU!");
			cgames += 4;
		} else
			if (glimit - pscore > 30) {
				msg("I SKUNKED YOU!");
				cgames += 2;
			} else {
				msg("I WON!");
				++cgames;
			}
		iwon = TRUE;
	}
	gamescore();
}

/*
 * playhand:
 *	Do up one hand of the game
 */
static int
playhand(BOOLEAN mycrib)
{
	int deckpos;

	werase(Compwin);
	wrefresh(Compwin);
	werase(Tablewin);
	wrefresh(Tablewin);

	knownum = 0;
	deckpos = deal(mycrib);
	sorthand(chand, FULLHAND);
	sorthand(phand, FULLHAND);
	makeknown(chand, FULLHAND);
	prhand(phand, FULLHAND, Playwin, FALSE);
	discard(mycrib);
	if (cut(mycrib, deckpos))
		return TRUE;
	if (peg(mycrib))
		return TRUE;
	werase(Tablewin);
	wrefresh(Tablewin);
	if (score(mycrib))
		return TRUE;
	return FALSE;
}

/*
 * deal cards to both players from deck
 */
static int
deal(BOOLEAN mycrib)
{
	int i, j;

	for (i = j = 0; i < FULLHAND; i++) {
		if (mycrib) {
			phand[i] = deck[j++];
			chand[i] = deck[j++];
		} else {
			chand[i] = deck[j++];
			phand[i] = deck[j++];
		}
	}
	return (j);
}

/*
 * discard:
 *	Handle players discarding into the crib...
 * Note: we call cdiscard() after prining first message so player doesn't wait
 */
static void
discard(BOOLEAN mycrib)
{
	const char *prompt;
	CARD crd;

	prcrib(mycrib, TRUE);
	prompt = (quiet ? "Discard --> " : "Discard a card --> ");
	cdiscard(mycrib);	/* puts best discard at end */
	crd = phand[infrom(phand, FULLHAND, prompt)];
	cremove(crd, phand, FULLHAND);
	prhand(phand, FULLHAND, Playwin, FALSE);
	crib[0] = crd;

	/* Next four lines same as last four except for cdiscard(). */
	crd = phand[infrom(phand, FULLHAND - 1, prompt)];
	cremove(crd, phand, FULLHAND - 1);
	prhand(phand, FULLHAND, Playwin, FALSE);
	crib[1] = crd;
	crib[2] = chand[4];
	crib[3] = chand[5];
	chand[4].rank = chand[4].suit = chand[5].rank = chand[5].suit = EMPTY;
}

/*
 * cut:
 *	Cut the deck and set turnover.  Actually, we only ASK the
 *	player what card to turn.  We do a random one, anyway.
 */
static int
cut(BOOLEAN mycrib, int  pos)
{
	int i;
	BOOLEAN win;

	win = FALSE;
	if (mycrib) {
		if (!rflag) {	/* random cut */
			msg(quiet ? "Cut the deck? " :
		    "How many cards down do you wish to cut the deck? ");
			get_line();
		}
		i = (rand() >> 4) % (CARDS - pos);
		turnover = deck[i + pos];
		addmsg(quiet ? "You cut " : "You cut the ");
		msgcard(turnover, FALSE);
		endmsg();
		if (turnover.rank == JACK) {
			msg("I get two for his heels");
			win = chkscr(&cscore, 2);
		}
	} else {
		i = (rand() >> 4) % (CARDS - pos) + pos;
		turnover = deck[i];
		addmsg(quiet ? "I cut " : "I cut the ");
		msgcard(turnover, FALSE);
		endmsg();
		if (turnover.rank == JACK) {
			msg("You get two for his heels");
			win = chkscr(&pscore, 2);
		}
	}
	makeknown(&turnover, 1);
	prcrib(mycrib, FALSE);
	return (win);
}

/*
 * prcrib:
 *	Print out the turnover card with crib indicator
 */
static void
prcrib(BOOLEAN mycrib, BOOLEAN blank)
{
	int y, cardx;

	if (mycrib)
		cardx = CRIB_X;
	else
		cardx = 0;

	mvaddstr(CRIB_Y, cardx + 1, "CRIB");
	prcard(stdscr, CRIB_Y + 1, cardx, turnover, blank);

	if (mycrib)
		cardx = 0;
	else
		cardx = CRIB_X;

	for (y = CRIB_Y; y <= CRIB_Y + 5; y++)
		mvaddstr(y, cardx, "       ");
	refresh();
}

/*
 * peg:
 *	Handle all the pegging...
 */
static CARD Table[14];
static unsigned Tcnt;

static int
peg(BOOLEAN mycrib)
{
	static CARD ch[CINHAND], ph[CINHAND];
	int i, j, k;
	int l;
	int cnum, pnum, sum;
	BOOLEAN myturn, mego, ugo, last, played;
	CARD crd;

	played = FALSE;
	cnum = pnum = CINHAND;
	for (i = 0; i < CINHAND; i++) {	/* make copies of hands */
		ch[i] = chand[i];
		ph[i] = phand[i];
	}
	Tcnt = 0;		/* index to table of cards played */
	sum = 0;		/* sum of cards played */
	mego = ugo = FALSE;
	myturn = !mycrib;
	for (;;) {
		last = TRUE;	/* enable last flag */
		prhand(ph, pnum, Playwin, FALSE);
		prhand(ch, cnum, Compwin, TRUE);
		prtable(sum);
		if (myturn) {	/* my tyrn to play */
			if (!anymove(ch, cnum, sum)) {	/* if no card to play */
				if (!mego && cnum) {	/* go for comp? */
					msg("GO");
					mego = TRUE;
				}
							/* can player move? */
				if (anymove(ph, pnum, sum))
					myturn = !myturn;
				else {			/* give him his point */
					msg(quiet ? "You get one" :
					    "You get one point");
					do_wait();
					if (chkscr(&pscore, 1))
						return TRUE;
					sum = 0;
					mego = ugo = FALSE;
					Tcnt = 0;
				}
			} else {
				played = TRUE;
				j = -1;
				k = 0;
							/* maximize score */
				for (i = 0; i < cnum; i++) {
					l = pegscore(ch[i], Table, Tcnt, sum);
					if (l > k) {
						k = l;
						j = i;
					}
				}
				if (j < 0)		/* if nothing scores */
					j = cchose(ch, cnum, sum);
				crd = ch[j];
				cremove(crd, ch, cnum--);
				sum += VAL(crd.rank);
				Table[Tcnt++] = crd;
				if (k > 0) {
					addmsg(quiet ? "I get %d playing " :
					    "I get %d points playing ", k);
					msgcard(crd, FALSE);
					endmsg();
					if (chkscr(&cscore, k))
						return TRUE;
				}
				myturn = !myturn;
			}
		} else {
			if (!anymove(ph, pnum, sum)) {	/* can player move? */
				if (!ugo && pnum) {	/* go for player */
					msg("You have a GO");
					ugo = TRUE;
				}
							/* can computer play? */
				if (anymove(ch, cnum, sum))
					myturn = !myturn;
				else {
					msg(quiet ? "I get one" :
					    "I get one point");
					do_wait();
					if (chkscr(&cscore, 1))
						return TRUE;
					sum = 0;
					mego = ugo = FALSE;
					Tcnt = 0;
				}
			} else {			/* player plays */
				played = FALSE;
				if (pnum == 1) {
					crd = ph[0];
					msg("You play your last card");
				} else
					for (;;) {
						prhand(ph,
						    pnum, Playwin, FALSE);
						crd = ph[infrom(ph,
						    pnum, "Your play: ")];
						if (sum + VAL(crd.rank) <= 31)
							break;
						else
					msg("Total > 31 -- try again");
					}
				makeknown(&crd, 1);
				cremove(crd, ph, pnum--);
				i = pegscore(crd, Table, Tcnt, sum);
				sum += VAL(crd.rank);
				Table[Tcnt++] = crd;
				if (i > 0) {
					msg(quiet ? "You got %d" :
					    "You got %d points", i);
					if (pnum == 0)
						do_wait();
					if (chkscr(&pscore, i))
						return TRUE;
				}
				myturn = !myturn;
			}
		}
		if (sum >= 31) {
			if (!myturn)
				do_wait();
			sum = 0;
			mego = ugo = FALSE;
			Tcnt = 0;
			last = FALSE;			/* disable last flag */
		}
		if (!pnum && !cnum)
			break;				/* both done */
	}
	prhand(ph, pnum, Playwin, FALSE);
	prhand(ch, cnum, Compwin, TRUE);
	prtable(sum);
	if (last) {
		if (played) {
			msg(quiet ? "I get one for last" :
			    "I get one point for last");
			do_wait();
			if (chkscr(&cscore, 1))
				return TRUE;
		} else {
			msg(quiet ? "You get one for last" :
			    "You get one point for last");
			do_wait();
			if (chkscr(&pscore, 1))
				return TRUE;
		}
	}
	return (FALSE);
}

/*
 * prtable:
 *	Print out the table with the current score
 */
static void
prtable(int curscore)
{
	prhand(Table, Tcnt, Tablewin, FALSE);
	mvwprintw(Tablewin, (Tcnt + 2) * 2, Tcnt + 1, "%2d", curscore);
	wrefresh(Tablewin);
}

/*
 * score:
 *	Handle the scoring of the hands
 */
static int
score(BOOLEAN mycrib)
{
	sorthand(crib, CINHAND);
	if (mycrib) {
		if (plyrhand(phand, "hand"))
			return (TRUE);
		if (comphand(chand, "hand"))
			return (TRUE);
		do_wait();
		if (comphand(crib, "crib"))
			return (TRUE);
		do_wait();
	} else {
		if (comphand(chand, "hand"))
			return (TRUE);
		if (plyrhand(phand, "hand"))
			return (TRUE);
		if (plyrhand(crib, "crib"))
			return (TRUE);
	}
	return (FALSE);
}
