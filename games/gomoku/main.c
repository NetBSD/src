/*	$NetBSD: main.c,v 1.29 2022/05/15 22:00:11 rillig Exp $	*/

/*
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
__COPYRIGHT("@(#) Copyright (c) 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)main.c	8.4 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: main.c,v 1.29 2022/05/15 22:00:11 rillig Exp $");
#endif
#endif /* not lint */

#include <curses.h>
#include <err.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "gomoku.h"

#define USER	0		/* get input from standard input */
#define PROGRAM	1		/* get input from program */
#define INPUTF	2		/* get input from a file */

int	interactive = 1;	/* true if interactive */
int	debug;			/* true if debugging */
static int test;		/* both moves come from 1: input, 2: computer */
static char *prog;		/* name of program */
static char user[LOGIN_NAME_MAX]; /* name of player */
static FILE *debugfp;		/* file for debug output */
static FILE *inputfp;		/* file for debug input */

const char	pdir[4]		= "-\\|/";

struct	spotstr	board[BAREA];		/* info for board */
struct	combostr frames[FAREA];		/* storage for all frames */
struct	combostr *sortframes[2];	/* sorted list of non-empty frames */
u_char	overlap[FAREA * FAREA];		/* true if frame [a][b] overlap */
short	intersect[FAREA * FAREA];	/* frame [a][b] intersection */
int	movelog[BSZ * BSZ];		/* log of all the moves */
int	movenum;			/* current move number */
const char	*plyr[2];		/* who's who */

static int readinput(FILE *);
static void misclog(const char *, ...) __printflike(1, 2);
static void quit(void) __dead;
static void quitsig(int) __dead;

int
main(int argc, char **argv)
{
	char buf[128];
	char fname[PATH_MAX];
	char *tmp;
	int color, curmove, i, ch;
	int input[2];

	/* Revoke setgid privileges */
	setgid(getgid());

	tmp = getlogin();
	if (tmp) {
		strlcpy(user, tmp, sizeof(user));
	} else {
		strcpy(user, "you");
	}

	color = curmove = 0;

	prog = strrchr(argv[0], '/');
	if (prog)
		prog++;
	else
		prog = argv[0];

	while ((ch = getopt(argc, argv, "bcdD:u")) != -1) {
		switch (ch) {
		case 'b':	/* background */
			interactive = 0;
			break;
		case 'd':	/* debugging */
			debug++;
			break;
		case 'D':	/* log debug output to file */
			if ((debugfp = fopen(optarg, "w")) == NULL)
				err(1, "%s", optarg);
			break;
		case 'u':	/* testing: user versus user */
			test = 1;
			break;
		case 'c':	/* testing: computer versus computer */
			test = 2;
			break;
		}
	}
	argc -= optind;
	argv += optind;
	if (argc) {
		if ((inputfp = fopen(*argv, "r")) == NULL)
			err(1, "%s", *argv);
	}

	if (!debug)
		srandom(time(0));
	if (interactive)
		cursinit();		/* initialize curses */
again:
	bdinit(board);			/* initialize board contents */

	if (interactive) {
		plyr[BLACK] = plyr[WHITE] = "???";
		bdisp_init();		/* initialize display of board */
#ifdef DEBUG
		signal(SIGINT, whatsup);
#else
		signal(SIGINT, quitsig);
#endif

		if (inputfp == NULL && test == 0) {
			move(BSZ3, 0);
			printw("Black moves first. ");
			ask("(B)lack or (W)hite? ");
			for (;;) {
				ch = get_key(NULL);
				if (ch == 'b' || ch == 'B') {
					color = BLACK;
					break;
				}
				if (ch == 'w' || ch == 'W') {
					color = WHITE;
					break;
				}
				if (ch == 'q' || ch == 'Q') {
					quit();
				}
				beep();
				ask("Please choose (B)lack or (W)hite: ");
			}
			move(BSZ3, 0);
			clrtoeol();
		}
	} else {
		setbuf(stdout, 0);
		get_line(buf, sizeof(buf));
		if (strcmp(buf, "black") == 0)
			color = BLACK;
		else if (strcmp(buf, "white") == 0)
			color = WHITE;
		else {
			panic("Huh?  Expected `black' or `white', got `%s'\n",
			    buf);
		}
	}

	if (inputfp) {
		input[BLACK] = INPUTF;
		input[WHITE] = INPUTF;
	} else {
		switch (test) {
		case 0: /* user versus program */
			input[color] = USER;
			input[!color] = PROGRAM;
			break;

		case 1: /* user versus user */
			input[BLACK] = USER;
			input[WHITE] = USER;
			break;

		case 2: /* program versus program */
			input[BLACK] = PROGRAM;
			input[WHITE] = PROGRAM;
			break;
		}
	}
	if (interactive) {
		plyr[BLACK] = input[BLACK] == USER ? user : prog;
		plyr[WHITE] = input[WHITE] == USER ? user : prog;
		bdwho(1);
	}

	for (color = BLACK; ; color = !color) {
	top:
		switch (input[color]) {
		case INPUTF: /* input comes from a file */
			curmove = readinput(inputfp);
			if (curmove != ILLEGAL)
				break;
			switch (test) {
			case 0: /* user versus program */
				input[color] = USER;
				input[!color] = PROGRAM;
				break;

			case 1: /* user versus user */
				input[BLACK] = USER;
				input[WHITE] = USER;
				break;

			case 2: /* program versus program */
				input[BLACK] = PROGRAM;
				input[WHITE] = PROGRAM;
				break;
			}
			plyr[BLACK] = input[BLACK] == USER ? user : prog;
			plyr[WHITE] = input[WHITE] == USER ? user : prog;
			bdwho(1);
			goto top;

		case USER: /* input comes from standard input */
		getinput:
			if (interactive) {
				ask("Select move, (S)ave or (Q)uit.");
				curmove = get_coord();
				if (curmove == SAVE) {
					FILE *fp;

					ask("Save file name? ");
					(void)get_line(fname, sizeof(fname));
					if ((fp = fopen(fname, "w")) == NULL) {
						misclog("cannot create save file");
						goto getinput;
					}
					for (i = 0; i < movenum - 1; i++)
						fprintf(fp, "%s\n",
						    stoc(movelog[i]));
					fclose(fp);
					goto getinput;
				}
				if (curmove != RESIGN &&
				    board[curmove].s_occ != EMPTY) {
					/*misclog("Illegal move");*/
					beep();
					goto getinput;
				}
			} else {
				if (!get_line(buf, sizeof(buf))) {
					curmove = RESIGN;
					break;
				}
				if (buf[0] == '\0')
					goto getinput;
				curmove = ctos(buf);
			}
			break;

		case PROGRAM: /* input comes from the program */
			if (interactive)
				ask("Thinking...");
			curmove = pickmove(color);
			break;
		}
		if (interactive) {
			misclog("%3d%s%-6s", movenum, color ? "        " : " ",
			    stoc(curmove));
		}
		if ((i = makemove(color, curmove)) != MOVEOK)
			break;
		if (interactive)
			bdisp();
	}
	if (interactive) {
		move(BSZ3, 0);
		switch (i) {
		case WIN:
			if (input[color] == PROGRAM)
				addstr("Ha ha, I won");
			else if (input[0] == USER && input[1] == USER)
				addstr("Well, you won (and lost)");
			else
				addstr("Rats! you won");
			break;
		case TIE:
			addstr("Wow! It's a tie");
			break;
		case ILLEGAL:
			addstr("Illegal move");
			break;
		}
		clrtoeol();
		bdisp();
		if (i != RESIGN) {
		replay:
			ask("Play again? ");
			ch = get_key("YyNnQqSs");
			if (ch == 'Y' || ch == 'y')
				goto again;
			if (ch == 'S') {
				FILE *fp;

				ask("Save file name? ");
				(void)get_line(fname, sizeof(fname));
				if ((fp = fopen(fname, "w")) == NULL) {
					misclog("cannot create save file");
					goto replay;
				}
				for (i = 0; i < movenum - 1; i++)
					fprintf(fp, "%s\n",
					    stoc(movelog[i]));
				fclose(fp);
				goto replay;
			}
		}
	}
	quit();
	/* NOTREACHED */
	return (0);
}

static int
readinput(FILE *fp)
{
	int c;
	char buf[128];
	size_t pos;

	pos = 0;
	while ((c = getc(fp)) != EOF && c != '\n' && pos < sizeof(buf) - 1)
		buf[pos++] = c;
	buf[pos] = '\0';
	return ctos(buf);
}

#ifdef DEBUG
/*
 * Handle strange situations.
 */
void
whatsup(int signum)
{
	int i, n, s1, s2, d1, d2;
	struct spotstr *sp;
	FILE *fp;
	char *str;
	struct elist *ep;
	struct combostr *cbp;
	char input[128];
	char tmp[128];

	if (!interactive)
		quit();
top:
	ask("debug command: ");
	if (!get_line(input, sizeof(input)))
		quit();
	switch (*input) {
	case '\0':
		goto top;
	case 'q':		/* conservative quit */
		quit();
	case 'd':		/* set debug level */
		debug = input[1] - '0';
		debuglog("Debug set to %d", debug);
		sleep(1);
	case 'c':
		break;
	case 'b':		/* back up a move */
		if (movenum > 1) {
			movenum--;
			board[movelog[movenum - 1]].s_occ = EMPTY;
			bdisp();
		}
		goto top;
	case 's':		/* suggest a move */
		i = input[1] == 'b' ? BLACK : WHITE;
		debuglog("suggest %c %s", i == BLACK ? 'B' : 'W',
			stoc(pickmove(i)));
		goto top;
	case 'f':		/* go forward a move */
		board[movelog[movenum - 1]].s_occ = movenum & 1 ? BLACK : WHITE;
		movenum++;
		bdisp();
		goto top;
	case 'l':		/* print move history */
		if (input[1] == '\0') {
			for (i = 0; i < movenum - 1; i++)
				debuglog("%s", stoc(movelog[i]));
			goto top;
		}
		if ((fp = fopen(input + 1, "w")) == NULL)
			goto top;
		for (i = 0; i < movenum - 1; i++) {
			fprintf(fp, "%s", stoc(movelog[i]));
			if (++i < movenum - 1)
				fprintf(fp, " %s\n", stoc(movelog[i]));
			else
				fputc('\n', fp);
		}
		bdump(fp);
		fclose(fp);
		goto top;
	case 'o':
		/* avoid use w/o initialization on invalid input */
		d1 = s1 = 0;

		n = 0;
		for (str = input + 1; *str; str++)
			if (*str == ',') {
				for (d1 = 0; d1 < 4; d1++)
					if (str[-1] == pdir[d1])
						break;
				str[-1] = '\0';
				sp = &board[s1 = ctos(input + 1)];
				n = (sp->s_frame[d1] - frames) * FAREA;
				*str++ = '\0';
				break;
			}
		sp = &board[s2 = ctos(str)];
		while (*str)
			str++;
		for (d2 = 0; d2 < 4; d2++)
			if (str[-1] == pdir[d2])
				break;
		n += sp->s_frame[d2] - frames;
		debuglog("overlap %s%c,%s%c = %x", stoc(s1), pdir[d1],
		    stoc(s2), pdir[d2], overlap[n]);
		goto top;
	case 'p':
		sp = &board[i = ctos(input + 1)];
		debuglog("V %s %x/%d %d %x/%d %d %d %x", stoc(i),
			sp->s_combo[BLACK].s, sp->s_level[BLACK],
			sp->s_nforce[BLACK],
			sp->s_combo[WHITE].s, sp->s_level[WHITE],
			sp->s_nforce[WHITE], sp->s_wval, sp->s_flags);
		debuglog("FB %s %x %x %x %x", stoc(i),
			sp->s_fval[BLACK][0].s, sp->s_fval[BLACK][1].s,
			sp->s_fval[BLACK][2].s, sp->s_fval[BLACK][3].s);
		debuglog("FW %s %x %x %x %x", stoc(i),
			sp->s_fval[WHITE][0].s, sp->s_fval[WHITE][1].s,
			sp->s_fval[WHITE][2].s, sp->s_fval[WHITE][3].s);
		goto top;
	case 'e':	/* e {b|w} [0-9] spot */
		str = input + 1;
		if (*str >= '0' && *str <= '9')
			n = *str++ - '0';
		else
			n = 0;
		sp = &board[i = ctos(str)];
		for (ep = sp->s_empty; ep; ep = ep->e_next) {
			cbp = ep->e_combo;
			if (n) {
				if (cbp->c_nframes > n)
					continue;
				if (cbp->c_nframes != n)
					break;
			}
			printcombo(cbp, tmp, sizeof(tmp));
			debuglog("%s", tmp);
		}
		goto top;
	default:
		debuglog("Options are:");
		debuglog("q    - quit");
		debuglog("c    - continue");
		debuglog("d#   - set debug level to #");
		debuglog("p#   - print values at #");
		goto top;
	}
}
#endif /* DEBUG */

/*
 * Display debug info.
 */
void
debuglog(const char *fmt, ...)
{
	va_list ap;
	char buf[128];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (debugfp)
		fprintf(debugfp, "%s\n", buf);
	if (interactive)
		dislog(buf);
	else
		fprintf(stderr, "%s\n", buf);
}

static void
misclog(const char *fmt, ...)
{
	va_list ap;
	char buf[128];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (debugfp)
		fprintf(debugfp, "%s\n", buf);
	if (interactive)
		dislog(buf);
	else
		printf("%s\n", buf);
}

static void
quit(void)
{
	if (interactive) {
		bdisp();		/* show final board */
		cursfini();
	}
	exit(0);
}

static void
quitsig(int dummy __unused)
{
	quit();
}

/*
 * Die gracefully.
 */
void
panic(const char *fmt, ...)
{
	va_list ap;

	if (interactive) {
		bdisp();
		cursfini();
	}

	fprintf(stderr, "%s: ", prog);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");

	fputs("I resign\n", stdout);
	exit(1);
}
