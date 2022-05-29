/*	$NetBSD: bdisp.c,v 1.51 2022/05/29 00:12:11 rillig Exp $	*/

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
__RCSID("$NetBSD: bdisp.c,v 1.51 2022/05/29 00:12:11 rillig Exp $");

#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include "gomoku.h"

#define	SCRNH		24		/* assume 24 lines for the moment */
#define	SCRNW		80		/* assume 80 chars for the moment */

static	int	lastline;
static	const char pcolor[] = "*O.?";

#define	scr_y(by)	(1 + (BSZ - 1) - ((by) - 1))
#define	scr_x(bx)	(3 + 2 * ((bx) - 1))

#define TRANSCRIPT_COL	(3 + (2 * BSZ - 1) + 3 + 3)

/*
 * Initialize screen display.
 */
void
cursinit(void)
{

	if (initscr() == NULL)
		errx(EXIT_FAILURE, "Couldn't initialize screen");

	if (LINES < SCRNH || COLS < SCRNW)
		errx(EXIT_FAILURE, "Screen too small (need %dx%d)",
		    SCRNW, SCRNH);

	keypad(stdscr, true);
	nonl();
	noecho();
	cbreak();
	leaveok(stdscr, false);

	mousemask(BUTTON1_CLICKED, NULL);
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
		mvprintw(scr_y(j), 0, "%2d", j);
		mvprintw(scr_y(j), scr_x(BSZ) + 2, "%d", j);
	}

	bdwho();
	mvaddstr(0, TRANSCRIPT_COL, "  #  black  white");
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

static bool
should_highlight(spot_index s)
{

	if (game.nmoves > 0 && game.moves[game.nmoves - 1] == s)
		return true;
	if (game.winning_spot != 0)
		for (int i = 0; i < 5; i++)
			if (s == game.winning_spot + i * dd[game.winning_dir])
				return true;
	return false;
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
			if (should_highlight(PT(i, j))) {
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

	for (;;) {
		int ch = getch();
		if (allowed == NULL || ch == '\0' ||
		    strchr(allowed, ch) != NULL)
			return ch;
		beep();
		refresh();
	}
}

bool
get_line(char *buf, int size, void (*on_change)(const char *))
{
	char *cp, *end;
	int c;

	c = 0;
	cp = buf;
	end = buf + size - 1;	/* save room for the '\0' */
	while ((c = getchar()) != EOF && c != '\n' && c != '\r') {
		if (!interactive && cp < end) {
			*cp++ = c;
			continue;
		}
		if (!interactive)
			errx(EXIT_FAILURE, "line too long");

		switch (c) {
		case 0x0c:	/* ^L */
			wrefresh(curscr);
			continue;
		case 0x15:	/* ^U */
		case 0x18:	/* ^X */
			for (; cp > buf; cp--)
				addstr("\b \b");
			break;
		case '\b':
		case 0x7f:	/* DEL */
			if (cp == buf)
				continue;
			cp--;
			addstr("\b \b");
			break;
		default:
			if (cp < end) {
				*cp++ = c;
				addch(c);
			} else
				beep();
		}
		if (on_change != NULL) {
			*cp = '\0';
			on_change(buf);
		}
		refresh();
	}
	*cp = '\0';
	return c != EOF;
}

static bool
get_coord_mouse(int *x, int *y)
{
	MEVENT ev;

	if (getmouse(&ev) == OK &&
	    (ev.bstate & (BUTTON1_RELEASED | BUTTON1_CLICKED)) != 0 &&
	    ev.y >= scr_y(BSZ) && ev.y <= scr_y(1) &&
	    ev.x >= scr_x(1) && ev.x <= scr_x(BSZ) &&
	    (ev.x - scr_x(1)) % (scr_x(2) - scr_x(1)) == 0) {
		*x = 1 + (ev.x - scr_x(1)) / (scr_x(2) - scr_x(1));
		*y = 1 + (scr_y(1) - ev.y) / (scr_y(1) - scr_y(2));
		return true;
	}
	return false;
}

/*
 * Ask the user for the coordinate of a move, or return RESIGN or SAVE.
 *
 * Based on Eric S. Raymond's modifications to the battleship (bs) user
 * interface.
 */
int
get_coord(void)
{
	static int x = 1 + (BSZ - 1) / 2;
	static int y = 1 + (BSZ - 1) / 2;

	move(scr_y(y), scr_x(x));
	refresh();
	for (;;) {
		mvprintw(BSZ + 3, 6, "(%c %d) ", letters[x], y);
		move(scr_y(y), scr_x(x));

		int ch = getch();
		switch (ch) {
		case 'k':
		case '8':
		case KEY_UP:
			y++;
			break;
		case 'j':
		case '2':
		case KEY_DOWN:
			y--;
			break;
		case 'h':
		case '4':
		case KEY_LEFT:
			x--;
			break;
		case 'l':
		case '6':
		case KEY_RIGHT:
			x++;
			break;
		case 'y':
		case '7':
		case KEY_A1:
			x--;
			y++;
			break;
		case 'b':
		case '1':
		case KEY_C1:
			x--;
			y--;
			break;
		case 'u':
		case '9':
		case KEY_A3:
			x++;
			y++;
			break;
		case 'n':
		case '3':
		case KEY_C3:
			x++;
			y--;
			break;
		case 'K':
			y += 5;
			break;
		case 'J':
			y -= 5;
			break;
		case 'H':
			x -= 5;
			break;
		case 'L':
			x += 5;
			break;
		case 'Y':
			x -= 5;
			y += 5;
			break;
		case 'B':
			x -= 5;
			y -= 5;
			break;
		case 'U':
			x += 5;
			y += 5;
			break;
		case 'N':
			x += 5;
			y -= 5;
			break;
		case 0x0c:	/* ^L */
			(void)clearok(stdscr, true);
			(void)refresh();
			break;
		case KEY_MOUSE:
			if (get_coord_mouse(&x, &y))
				return PT(x, y);
			beep();
			break;
		case 'Q':
		case 'q':
			return RESIGN;
		case 'S':
		case 's':
			return SAVE;
		case ' ':
		case '\r':
			(void)mvhline(BSZ + 3, 6, ' ', 6);
			return PT(x, y);
		}

		x = 1 + (x + BSZ - 1) % BSZ;
		y = 1 + (y + BSZ - 1) % BSZ;
	}
}
