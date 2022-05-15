/*	$NetBSD: bdisp.c,v 1.20 2022/05/15 22:00:11 rillig Exp $	*/

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
#if 0
static char sccsid[] = "@(#)bdisp.c	8.2 (Berkeley) 5/3/95";
#else
__RCSID("$NetBSD: bdisp.c,v 1.20 2022/05/15 22:00:11 rillig Exp $");
#endif
#endif /* not lint */

#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include "gomoku.h"

#define	SCRNH		24		/* assume 24 lines for the moment */
#define	SCRNW		80		/* assume 80 chars for the moment */

static	int	lastline;
static	char	pcolor[] = "*O.?";

/*
 * Initialize screen display.
 */
void
cursinit(void)
{

	if (!initscr()) {
		errx(EXIT_FAILURE, "Couldn't initialize screen");
	}
	if ((LINES < SCRNH) || (COLS < SCRNW)) {
		errx(EXIT_FAILURE, "Screen too small (need %d%xd)",
		    SCRNW, SCRNH);
	}
	keypad(stdscr, TRUE);
	nonl();
	noecho();
	cbreak();
	leaveok(stdscr, FALSE);

#if 0 /* no mouse support in netbsd curses yet */
	mousemask(BUTTON1_CLICKED, NULL);
#endif
}

/*
 * Restore screen display.
 */
void
cursfini(void)
{

	move(BSZ4, 0);
	clrtoeol();
	refresh();
	echo();
	endwin();
}

/*
 * Initialize board display.
 */
void
bdisp_init(void)
{
	int i, j;

	/* top border */
	for (i = 1; i < BSZ1; i++) {
		move(0, 2 * i + 1);
		addch(letters[i]);
	}
	/* left and right edges */
	for (j = BSZ1; --j > 0; ) {
		move(20 - j, 0);
		printw("%2d ", j);
		move(20 - j, 2 * BSZ1 + 1);
		printw("%d ", j);
	}
	/* bottom border */
	for (i = 1; i < BSZ1; i++) {
		move(20, 2 * i + 1);
		addch(letters[i]);
	}
	bdwho(0);
	move(0, 47);
	addstr("#  black  white");
	lastline = 0;
	bdisp();
}

/*
 * Update who is playing whom.
 */
void
bdwho(int update)
{
	int i, j;

	move(21, 0);
	printw("                                              ");
	i = strlen(plyr[BLACK]);
	j = strlen(plyr[WHITE]);
	if (i + j <= 20) {
		move(21, 10 - (i + j) / 2);
		printw("BLACK/%s (*) vs. WHITE/%s (O)",
		    plyr[BLACK], plyr[WHITE]);
	} else {
		move(21, 0);
		if (i <= 10) {
			j = 20 - i;
		} else if (j <= 10) {
			i = 20 - j;
		} else {
			i = j = 10;
		}
		printw("BLACK/%.*s (*) vs. WHITE/%.*s (O)",
		    i, plyr[BLACK], j, plyr[WHITE]);
	}
	if (update)
		refresh();
}

/*
 * Update the board display after a move.
 */
void
bdisp(void)
{
	int i, j, c;
	struct spotstr *sp;

	for (j = BSZ1; --j > 0; ) {
		for (i = 1; i < BSZ1; i++) {
			move(BSZ1 - j, 2 * i + 1);
			sp = &board[i + j * BSZ1];
			if (debug > 1 && sp->s_occ == EMPTY) {
				if (sp->s_flags & IFLAGALL)
					c = '+';
				else if (sp->s_flags & CFLAGALL)
					c = '-';
				else
					c = '.';
			} else
				c = pcolor[sp->s_occ];
			addch(c);
		}
	}
	refresh();
}

#ifdef DEBUG
/*
 * Dump board display to a file.
 */
void
bdump(FILE *fp)
{
	int i, j, c;
	struct spotstr *sp;

	/* top border */
	fprintf(fp, "   A B C D E F G H J K L M N O P Q R S T\n");

	for (j = BSZ1; --j > 0; ) {
		/* left edge */
		fprintf(fp, "%2d ", j);
		for (i = 1; i < BSZ1; i++) {
			sp = &board[i + j * BSZ1];
			if (debug > 1 && sp->s_occ == EMPTY) {
				if (sp->s_flags & IFLAGALL)
					c = '+';
				else if (sp->s_flags & CFLAGALL)
					c = '-';
				else
					c = '.';
			} else
				c = pcolor[sp->s_occ];
			putc(c, fp);
			putc(' ', fp);
		}
		/* right edge */
		fprintf(fp, "%d\n", j);
	}

	/* bottom border */
	fprintf(fp, "   A B C D E F G H J K L M N O P Q R S T\n");
}
#endif /* DEBUG */

/*
 * Display a transcript entry
 */
void
dislog(const char *str)
{

	if (++lastline >= SCRNH - 1) {
		/* move 'em up */
		lastline = 1;
	}
	move(lastline, TRANSCRIPT_COL);
	addnstr(str, SCRNW - TRANSCRIPT_COL - 1);
	clrtoeol();
	move(lastline + 1, TRANSCRIPT_COL);
	clrtoeol();
}

/*
 * Display a question.
 */

void
ask(const char *str)
{
	int len = strlen(str);

	move(BSZ4, 0);
	addstr(str);
	clrtoeol();
	move(BSZ4, len);
	refresh();
}

int
get_key(const char *allowed)
{
	int ch;

	for (;;) {
		ch = getch();
		if (allowed != NULL &&
		    ch != '\0' && strchr(allowed, ch) == NULL) {
			beep();
			refresh();
			continue;
		}
		break;
	}
	return ch;
}

int
get_line(char *buf, int size)
{
	char *cp, *end;
	int c;

	c = 0;
	cp = buf;
	end = buf + size - 1;	/* save room for the '\0' */
	while (cp < end && (c = getchar()) != EOF && c != '\n' && c != '\r') {
		*cp++ = c;
		if (interactive) {
			switch (c) {
			case 0x0c: /* ^L */
				wrefresh(curscr);
				cp--;
				continue;
			case 0x15: /* ^U */
			case 0x18: /* ^X */
				while (cp > buf) {
					cp--;
					addch('\b');
				}
				clrtoeol();
				break;
			case '\b':
			case 0x7f: /* DEL */
				if (cp == buf + 1) {
					cp--;
					continue;
				}
				cp -= 2;
				addch('\b');
				c = ' ';
				/* FALLTHROUGH */
			default:
				addch(c);
			}
			refresh();
		}
	}
	*cp = '\0';
	return (c != EOF);
}

/*
 * Decent (n)curses interface for the game, based on Eric S. Raymond's
 * modifications to the battleship (bs) user interface.
 */
int
get_coord(void)
{
	static int curx = BSZ / 2;
	static int cury = BSZ / 2;
	int ny, nx, ch;

	BGOTO(cury, curx);
	refresh();
	nx = curx;
	ny = cury;
	for (;;) {
		mvprintw(BSZ3, (BSZ - 6) / 2, "(%c %d) ",
		    'A' + ((curx > 7) ? (curx + 1) : curx), cury + 1);
		BGOTO(cury, curx);

		ch = getch();
		switch (ch) {
		case 'k':
		case '8':
		case KEY_UP:
			nx = curx;
			ny = cury + 1;
			break;
		case 'j':
		case '2':
		case KEY_DOWN:
			nx = curx;
			ny = BSZ + cury - 1;
			break;
		case 'h':
		case '4':
		case KEY_LEFT:
			nx = BSZ + curx - 1;
			ny = cury;
			break;
		case 'l':
		case '6':
		case KEY_RIGHT:
			nx = curx + 1;
			ny = cury;
			break;
		case 'y':
		case '7':
		case KEY_A1:
			nx = BSZ + curx - 1;
			ny = cury + 1;
			break;
		case 'b':
		case '1':
		case KEY_C1:
			nx = BSZ + curx - 1;
			ny = BSZ + cury - 1;
			break;
		case 'u':
		case '9':
		case KEY_A3:
			nx = curx + 1;
			ny = cury + 1;
			break;
		case 'n':
		case '3':
		case KEY_C3:
			nx = curx + 1;
			ny = BSZ + cury - 1;
			break;
		case 'K':
			nx = curx;
			ny = cury + 5;
			break;
		case 'J':
			nx = curx;
			ny = BSZ + cury - 5;
			break;
		case 'H':
			nx = BSZ + curx - 5;
			ny = cury;
			break;
		case 'L':
			nx = curx + 5;
			ny = cury;
			break;
		case 'Y':
			nx = BSZ + curx - 5;
			ny = cury + 5;
			break;
		case 'B':
			nx = BSZ + curx - 5;
			ny = BSZ + cury - 5;
			break;
		case 'U':
			nx = curx + 5;
			ny = cury + 5;
			break;
		case 'N':
			nx = curx + 5;
			ny = BSZ + cury - 5;
			break;
		case '\f':
			nx = curx;
			ny = cury;
			(void)clearok(stdscr, TRUE);
			(void)refresh();
			break;
#if 0 /* notyet */
		case KEY_MOUSE:
		{
			MEVENT	myevent;

			getmouse(&myevent);
			if (myevent.y >= 1 && myevent.y <= BSZ1 &&
			    myevent.x >= 3 && myevent.x <= (2 * BSZ + 1)) {
				curx = (myevent.x - 3) / 2;
				cury = BSZ - myevent.y;
				return PT(curx,cury);
			} else {
				beep();
			}
		}
		break;
#endif /* 0 */
		case 'Q':
		case 'q':
			return RESIGN;
		case 'S':
		case 's':
			return SAVE;
		case ' ':
		case '\r':
			(void)mvaddstr(BSZ3, (BSZ - 6) / 2, "      ");
			return PT(curx + 1, cury + 1);
		}

		curx = nx % BSZ;
		cury = ny % BSZ;
	}
}
