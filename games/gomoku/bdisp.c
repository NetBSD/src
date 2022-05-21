/*	$NetBSD: bdisp.c,v 1.38 2022/05/21 12:08:06 rillig Exp $	*/

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
/*	@(#)bdisp.c	8.2 (Berkeley) 5/3/95	*/
__RCSID("$NetBSD: bdisp.c,v 1.38 2022/05/21 12:08:06 rillig Exp $");

#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include "gomoku.h"

#define	SCRNH		24		/* assume 24 lines for the moment */
#define	SCRNW		80		/* assume 80 chars for the moment */

static	int	lastline;
static	char	pcolor[] = "*O.?";

#define	scr_y(by)	(1 + (BSZ - 1) - ((by) - 1))
#define	scr_x(bx)	(3 + 2 * ((bx) - 1))

/*
 * Initialize screen display.
 */
void
cursinit(void)
{

	if (initscr() == NULL) {
		errx(EXIT_FAILURE, "Couldn't initialize screen");
	}
	if ((LINES < SCRNH) || (COLS < SCRNW)) {
		errx(EXIT_FAILURE, "Screen too small (need %dx%d)",
		    SCRNW, SCRNH);
	}
	keypad(stdscr, true);
	nonl();
	noecho();
	cbreak();
	leaveok(stdscr, false);

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

	move(BSZ + 4, 0);
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

	/* top and bottom borders */
	for (int i = 1; i < BSZ + 1; i++) {
		mvaddch(scr_y(BSZ + 1), scr_x(i), letters[i]);
		mvaddch(scr_y(0), scr_x(i), letters[i]);
	}

	/* left and right edges */
	for (int j = BSZ + 1; --j > 0; ) {
		mvprintw(scr_y(j), 0, "%2d ", j);
		mvprintw(scr_y(j), scr_x(BSZ) + 2, "%d ", j);
	}

	bdwho();
	mvaddstr(0, TRANSCRIPT_COL + 1, "#  black  white");
	lastline = 0;
	bdisp();
}

/*
 * Update who is playing whom.
 */
void
bdwho(void)
{
	int bw = (int)strlen(plyr[BLACK]);
	int ww = (int)strlen(plyr[WHITE]);
	int available = 3 + (1 + scr_x(BSZ) - scr_x(1)) + 3;
	int fixed = (int)sizeof("BLACK/ (*) vs. WHITE/ (O)") - 1;
	int total = fixed + bw + ww;
	int x;

	if (total <= available)
		x = (available - total) / 2;
	else {
		int remaining = available - fixed;
		int half = remaining / 2;

		if (bw <= half)
			ww = remaining - bw;
		else if (ww <= half)
			bw = remaining - ww;
		else
			bw = half, ww = remaining - half;
		x = 0;
	}

	mvhline(BSZ + 2, 0, ' ', available);
	mvprintw(BSZ + 2, x, "BLACK/%.*s (*) vs. WHITE/%.*s (O)",
	    bw, plyr[BLACK], ww, plyr[WHITE]);
}

/*
 * Update the board display after a move.
 */
void
bdisp(void)
{
	int c;
	struct spotstr *sp;

	for (int j = BSZ + 1; --j > 0; ) {
		for (int i = 1; i < BSZ + 1; i++) {
			sp = &board[i + j * (BSZ + 1)];
			if (debug > 1 && sp->s_occ == EMPTY) {
				if ((sp->s_flags & IFLAGALL) != 0)
					c = '+';
				else if ((sp->s_flags & CFLAGALL) != 0)
					c = '-';
				else
					c = '.';
			} else
				c = pcolor[sp->s_occ];

			move(scr_y(j), scr_x(i));
			if (movenum > 1 && movelog[movenum - 2] == PT(i, j)) {
				attron(A_BOLD);
				addch(c);
				attroff(A_BOLD);
			} else
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
	int c;
	struct spotstr *sp;

	/* top border */
	fprintf(fp, "   A B C D E F G H J K L M N O P Q R S T\n");

	for (int j = BSZ + 1; --j > 0; ) {
		/* left edge */
		fprintf(fp, "%2d ", j);
		for (int i = 1; i < BSZ + 1; i++) {
			sp = &board[i + j * (BSZ + 1)];
			if (debug > 1 && sp->s_occ == EMPTY) {
				if ((sp->s_flags & IFLAGALL) != 0)
					c = '+';
				else if ((sp->s_flags & CFLAGALL) != 0)
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
	mvaddnstr(lastline, TRANSCRIPT_COL, str, SCRNW - TRANSCRIPT_COL - 1);
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
	int len = (int)strlen(str);

	mvaddstr(BSZ + 4, 0, str);
	clrtoeol();
	move(BSZ + 4, len);
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

bool
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
				addstr("\b \b");
				break;
			default:
				addch(c);
			}
			refresh();
		}
	}
	*cp = '\0';
	return c != EOF;
}

/*
 * Decent (n)curses interface for the game, based on Eric S. Raymond's
 * modifications to the battleship (bs) user interface.
 */
int
get_coord(void)
{
	/* XXX: These coordinates are 0-based, all others are 1-based. */
	static int curx = BSZ / 2;
	static int cury = BSZ / 2;
	int ny, nx, ch;

	move(scr_y(cury + 1), scr_x(curx + 1));
	refresh();
	nx = curx;
	ny = cury;
	for (;;) {
		mvprintw(BSZ + 3, 6, "(%c %d) ",
		    letters[curx + 1], cury + 1);
		move(scr_y(cury + 1), scr_x(curx + 1));

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
			(void)clearok(stdscr, true);
			(void)refresh();
			break;
#if 0 /* notyet */
		case KEY_MOUSE:
		{
			MEVENT	myevent;

			getmouse(&myevent);
			if (myevent.y >= 1 && myevent.y <= BSZ + 1 &&
			    myevent.x >= 3 && myevent.x <= 2 * BSZ + 1) {
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
			(void)mvaddstr(BSZ + 3, 6, "      ");
			return PT(curx + 1, cury + 1);
		}

		curx = nx % BSZ;
		cury = ny % BSZ;
	}
}
