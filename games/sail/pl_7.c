/*	$NetBSD: pl_7.c,v 1.38 2009/08/12 09:05:08 dholland Exp $	*/

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
static char sccsid[] = "@(#)pl_7.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: pl_7.c,v 1.38 2009/08/12 09:05:08 dholland Exp $");
#endif
#endif /* not lint */

#include <curses.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "array.h"
#include "extern.h"
#include "player.h"
#include "display.h"

/*
 * Use values above KEY_MAX for custom keycodes. (blymn@ says this is ok)
 */
#define KEY_ESC(ch) (KEY_MAX+10+ch)


/*
 * Display interface
 */

static void draw_view(void);
static void draw_turn(void);
static void draw_stat(void);
static void draw_slot(void);
static void draw_board(void);

static struct stringarray *sc_lines;
static unsigned sc_scrollup;
static bool sc_hasprompt;
static bool sc_hideprompt;
static const char *sc_prompt;
static const char *sc_buf;

static WINDOW *view_w;
static WINDOW *turn_w;
static WINDOW *stat_w;
static WINDOW *slot_w;
static WINDOW *scroll_w;

static bool obp[3];
static bool dbp[3];

int done_curses;
int loaded, fired, changed, repaired;
int dont_adjust;
static int viewrow, viewcol;
char movebuf[sizeof SHIP(0)->file->movebuf];
int player;
struct ship *ms;		/* memorial structure, &cc->ship[player] */
struct File *mf;		/* ms->file */
struct shipspecs *mc;		/* ms->specs */

////////////////////////////////////////////////////////////
// overall initialization

static
void
define_esc_key(int ch)
{
	char seq[3] = { '\x1b', ch, 0 };

	define_key(seq, KEY_ESC(ch));
}

void
initscreen(void)
{
	int ch;

	sc_lines = stringarray_create();
	if (sc_lines == NULL) {
		err(1, "malloc");
	}

	if (signal(SIGTSTP, SIG_DFL) == SIG_ERR) {
		err(1, "signal(SIGTSTP)");
	}

	if (initscr() == NULL) {
		errx(1, "Can't sail on this terminal.");
	}
	if (STAT_R >= COLS || SCROLL_Y <= 0) {
		errx(1, "Window/terminal not large enough.");
	}

	view_w = newwin(VIEW_Y, VIEW_X, VIEW_T, VIEW_L);
	slot_w = newwin(SLOT_Y, SLOT_X, SLOT_T, SLOT_L);
	scroll_w = newwin(SCROLL_Y, SCROLL_X, SCROLL_T, SCROLL_L);
	stat_w = newwin(STAT_Y, STAT_X, STAT_T, STAT_L);
	turn_w = newwin(TURN_Y, TURN_X, TURN_T, TURN_L);

	if (view_w == NULL ||
	    slot_w == NULL ||
	    scroll_w == NULL ||
	    stat_w == NULL ||
	    turn_w == NULL) {
		endwin();
		errx(1, "Curses initialization failed.");
	}

	leaveok(view_w, 1);
	leaveok(slot_w, 1);
	leaveok(stat_w, 1);
	leaveok(turn_w, 1);
	noecho();
	cbreak();

	/*
	 * Define esc-x keys
	 */
	for (ch = 0; ch < 127; ch++) {
		define_esc_key(ch);
	}

	done_curses++;
}

void
cleanupscreen(void)
{
	/* alarm already turned off */
	if (done_curses) {
		wmove(scroll_w, SCROLL_Y - 1, 0);
		wclrtoeol(scroll_w);
		display_redraw();
		endwin();
	}
}

////////////////////////////////////////////////////////////
// scrolling message area

static void
scrollarea_add(const char *text)
{
	char *copy;
	int errsave;

	copy = strdup(text);
	if (copy == NULL) {
		goto nomem;
	}
	if (stringarray_add(sc_lines, copy, NULL)) {
		goto nomem;
	}
	return;

nomem:
	/*
	 * XXX this should use leave(), but that won't
	 * currently work right.
	 */
	errsave = errno;
#if 0
	leave(LEAVE_MALLOC);
#else
	cleanupscreen();
	sync_close(!hasdriver);
	errno = errsave;
	err(1, "malloc");
#endif
}

static void
draw_scroll(void)
{
	unsigned total_lines;
	unsigned visible_lines;
	unsigned index_of_top;
	unsigned index_of_y;
	unsigned y;
	unsigned cursorx;

	werase(scroll_w);

	/* XXX: SCROLL_Y and whatnot should be unsigned too */
	visible_lines = SCROLL_Y - 1;

	total_lines = stringarray_num(sc_lines);
	if (total_lines > visible_lines) {
		index_of_top = total_lines - visible_lines;
	} else {
		index_of_top = 0;
	}
	if (index_of_top < sc_scrollup) {
		index_of_top = 0;
	} else {
		index_of_top -= sc_scrollup;
	}

	for (y = 0; y < visible_lines; y++) {
		index_of_y = index_of_top + y;
		if (index_of_y >= total_lines) {
			break;
		}
		wmove(scroll_w, y, 0);
		waddstr(scroll_w, stringarray_get(sc_lines, index_of_y));
	}
	if (sc_hasprompt && !sc_hideprompt) {
		wmove(scroll_w, SCROLL_Y-1, 0);
		waddstr(scroll_w, sc_prompt);
		waddstr(scroll_w, sc_buf);
		cursorx = strlen(sc_prompt) + strlen(sc_buf);
		wmove(scroll_w, SCROLL_Y-1, cursorx);
	}
	else {
		wmove(scroll_w, SCROLL_Y-1, 0);
	}
}

/*VARARGS2*/
void
Signal(const char *fmt, struct ship *ship, ...)
{
	va_list ap;
	char format[BUFSIZ];
	char buf[BUFSIZ];

	if (!done_curses)
		return;
	va_start(ap, ship);
	if (*fmt == '\a') {
		beep();
		fmt++;
	}
	fmtship(format, sizeof(format), fmt, ship);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	scrollarea_add(buf);
}

/*VARARGS2*/
void
Msg(const char *fmt, ...)
{
	va_list ap;
	char buf[BUFSIZ];

	if (!done_curses)
		return;
	va_start(ap, fmt);
	if (*fmt == '\a') {
		beep();
		fmt++;
	}
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	scrollarea_add(buf);
}

static void
prompt(const char *p, struct ship *ship)
{
	static char buf[BUFSIZ];

	fmtship(buf, sizeof(buf), p, ship);
	sc_prompt = buf;
	sc_buf = "";
	sc_hasprompt = true;
}

static void
endprompt(void)
{
	sc_prompt = NULL;
	sc_buf = NULL;
	sc_hasprompt = false;
}

/*
 * Next two functions called from newturn() to poke display. Shouldn't
 * exist... XXX
 */

void
display_hide_prompt(void)
{
	sc_hideprompt = true;
	draw_scroll();
	wrefresh(scroll_w);
}

void
display_reshow_prompt(void)
{
	sc_hideprompt = false;
	draw_scroll();
	wrefresh(scroll_w);
}


int
sgetch(const char *p, struct ship *ship, int flag)
{
	int c;
	char input[2];

	prompt(p, ship);
	input[0] = '\0';
	input[1] = '\0';
	sc_buf = input;
	blockalarm();
	draw_scroll();
	wrefresh(scroll_w);
	fflush(stdout);
	unblockalarm();
	while ((c = wgetch(scroll_w)) == EOF)
		;
	if (flag && c >= ' ' && c < 0x7f) {
		blockalarm();
		input[0] = c;
		draw_scroll();
		wrefresh(scroll_w);
		fflush(stdout);
		unblockalarm();
	}
	endprompt();
	return c;
}

void
sgetstr(const char *pr, char *buf, int n)
{
	int c;
	char *p = buf;

	prompt(pr, (struct ship *)0);
	sc_buf = buf;
	for (;;) {
		*p = 0;
		blockalarm();
		draw_scroll();
		wrefresh(scroll_w);
		fflush(stdout);
		unblockalarm();
		while ((c = wgetch(scroll_w)) == EOF)
			;
		switch (c) {
		case '\n':
		case '\r':
			endprompt();
			return;
		case '\b':
			if (p > buf) {
				/*waddstr(scroll_w, "\b \b");*/
				p--;
			}
			break;
		default:
			if (c >= ' ' && c < 0x7f && p < buf + n - 1) {
				*p++ = c;
				/*waddch(scroll_w, c);*/
			} else
				beep();
		}
	}
}

////////////////////////////////////////////////////////////
// drawing of other panes

void
display_force_full_redraw(void)
{
	clear();
}

void
display_redraw(void)
{
	draw_board();
	draw_view();
	draw_turn();
	draw_stat();
	draw_slot();
	draw_scroll();
	/* move the cursor */
	wrefresh(scroll_w);
	/* paranoia */
	fflush(stdout);
}

static void
draw_view(void)
{
	struct ship *sp;

	werase(view_w);
	foreachship(sp) {
		if (sp->file->dir
		    && sp->file->row > viewrow
		    && sp->file->row < viewrow + VIEW_Y
		    && sp->file->col > viewcol
		    && sp->file->col < viewcol + VIEW_X) {
			wmove(view_w, sp->file->row - viewrow,
				sp->file->col - viewcol);
			waddch(view_w, colours(sp));
			wmove(view_w,
				sternrow(sp) - viewrow,
				sterncol(sp) - viewcol);
			waddch(view_w, sterncolour(sp));
		}
	}
	wrefresh(view_w);
}

static void
draw_turn(void)
{
	wmove(turn_w, 0, 0);
	wprintw(turn_w, "%cTurn %d", dont_adjust?'*':'-', turn);
	wrefresh(turn_w);
}

static void
draw_stat(void)
{
	wmove(stat_w, STAT_1, 0);
	wprintw(stat_w, "Points  %3d\n", mf->points);
	wprintw(stat_w, "Fouls    %2d\n", fouled(ms));
	wprintw(stat_w, "Grapples %2d\n", grappled(ms));

	wmove(stat_w, STAT_2, 0);
	wprintw(stat_w, "    0 %c(%c)\n",
		maxmove(ms, winddir + 3, -1) + '0',
		maxmove(ms, winddir + 3, 1) + '0');
	waddstr(stat_w, "   \\|/\n");
	wprintw(stat_w, "   -^-%c(%c)\n",
		maxmove(ms, winddir + 2, -1) + '0',
		maxmove(ms, winddir + 2, 1) + '0');
	waddstr(stat_w, "   /|\\\n");
	wprintw(stat_w, "    | %c(%c)\n",
		maxmove(ms, winddir + 1, -1) + '0',
		maxmove(ms, winddir + 1, 1) + '0');
	wprintw(stat_w, "   %c(%c)\n",
		maxmove(ms, winddir, -1) + '0',
		maxmove(ms, winddir, 1) + '0');

	wmove(stat_w, STAT_3, 0);
	wprintw(stat_w, "Load  %c%c %c%c\n",
		loadname[mf->loadL], readyname(mf->readyL),
		loadname[mf->loadR], readyname(mf->readyR));
	wprintw(stat_w, "Hull %2d\n", mc->hull);
	wprintw(stat_w, "Crew %2d %2d %2d\n",
		mc->crew1, mc->crew2, mc->crew3);
	wprintw(stat_w, "Guns %2d %2d\n", mc->gunL, mc->gunR);
	wprintw(stat_w, "Carr %2d %2d\n", mc->carL, mc->carR);
	wprintw(stat_w, "Rigg %d %d %d ", mc->rig1, mc->rig2, mc->rig3);
	if (mc->rig4 < 0)
		waddch(stat_w, '-');
	else
		wprintw(stat_w, "%d", mc->rig4);
	wrefresh(stat_w);
}

void
draw_slot(void)
{
	int i;

	if (!boarding(ms, 0)) {
		mvwaddstr(slot_w, 0, 0, "   ");
		mvwaddstr(slot_w, 1, 0, "   ");
	} else {
		wmove(slot_w, 0, 0);
		for (i = 0; i < 3; i++) {
			waddch(slot_w, obp[i] ? '1'+i : ' ');
		}
		mvwaddstr(slot_w, 1, 0, "OBP");
	}
	if (!boarding(ms, 1)) {
		mvwaddstr(slot_w, 2, 0, "   ");
		mvwaddstr(slot_w, 3, 0, "   ");
	} else {
		wmove(slot_w, 2, 0);
		for (i = 0; i < 3; i++) {
			waddch(slot_w, dbp[i] ? '1'+i : ' ');
		}
		mvwaddstr(slot_w, 3, 0, "DBP");
	}

	wmove(slot_w, SLOT_Y-4, 0);
	if (mf->RH)
		wprintw(slot_w, "%dRH", mf->RH);
	else
		waddstr(slot_w, "   ");
	wmove(slot_w, SLOT_Y-3, 0);
	if (mf->RG)
		wprintw(slot_w, "%dRG", mf->RG);
	else
		waddstr(slot_w, "   ");
	wmove(slot_w, SLOT_Y-2, 0);
	if (mf->RR)
		wprintw(slot_w, "%dRR", mf->RR);
	else
		waddstr(slot_w, "   ");

#define Y	(SLOT_Y/2)
	wmove(slot_w, 7, 1);
	wprintw(slot_w,"%d", windspeed);
	mvwaddch(slot_w, Y, 0, ' ');
	mvwaddch(slot_w, Y, 2, ' ');
	mvwaddch(slot_w, Y-1, 0, ' ');
	mvwaddch(slot_w, Y-1, 1, ' ');
	mvwaddch(slot_w, Y-1, 2, ' ');
	mvwaddch(slot_w, Y+1, 0, ' ');
	mvwaddch(slot_w, Y+1, 1, ' ');
	mvwaddch(slot_w, Y+1, 2, ' ');
	wmove(slot_w, Y - dr[winddir], 1 - dc[winddir]);
	switch (winddir) {
	case 1:
	case 5:
		waddch(slot_w, '|');
		break;
	case 2:
	case 6:
		waddch(slot_w, '/');
		break;
	case 3:
	case 7:
		waddch(slot_w, '-');
		break;
	case 4:
	case 8:
		waddch(slot_w, '\\');
		break;
	}
	mvwaddch(slot_w, Y + dr[winddir], 1 + dc[winddir], '+');
	wrefresh(slot_w);
}

void
draw_board(void)
{
	int n;

	erase();
	werase(view_w);
	werase(slot_w);
	werase(scroll_w);
	werase(stat_w);
	werase(turn_w);

	move(BOX_T, BOX_L);
	for (n = 0; n < BOX_X; n++)
		addch('-');
	move(BOX_B, BOX_L);
	for (n = 0; n < BOX_X; n++)
		addch('-');
	for (n = BOX_T+1; n < BOX_B; n++) {
		mvaddch(n, BOX_L, '|');
		mvaddch(n, BOX_R, '|');
	}
	mvaddch(BOX_T, BOX_L, '+');
	mvaddch(BOX_T, BOX_R, '+');
	mvaddch(BOX_B, BOX_L, '+');
	mvaddch(BOX_B, BOX_R, '+');
	refresh();

#if 0
#define WSaIM "Wooden Ships & Iron Men"
	wmove(view_w, 2, (VIEW_X - sizeof WSaIM - 1) / 2);
	waddstr(view_w, WSaIM);
	wmove(view_w, 4, (VIEW_X - strlen(cc->name)) / 2);
	waddstr(view_w, cc->name);
	wrefresh(view_w);
#endif

	move(LINE_T, LINE_L);
	printw("Class %d %s (%d guns) '%s' (%c%c)",
		mc->class,
		classname[mc->class],
		mc->guns,
		ms->shipname,
		colours(ms),
		sterncolour(ms));
	refresh();
}

void
display_set_obp(int which, bool show)
{
	obp[which] = show;
}

void
display_set_dbp(int which, bool show)
{
	dbp[which] = show;
}

////////////////////////////////////////////////////////////
// external actions on the display

void
display_scroll_pageup(void)
{
	unsigned total_lines, visible_lines, limit;
	unsigned pagesize = SCROLL_Y - 2;

	total_lines = stringarray_num(sc_lines);
	visible_lines = SCROLL_Y - 1;
	limit = total_lines - visible_lines;

	sc_scrollup += pagesize;
	if (sc_scrollup > limit) {
		sc_scrollup = limit;
	}
}

void
display_scroll_pagedown(void)
{
	unsigned pagesize = SCROLL_Y - 2;

	if (sc_scrollup < pagesize) {
		sc_scrollup = 0;
	} else {
		sc_scrollup -= pagesize;
	}
}

void
centerview(void)
{
	viewrow = mf->row - VIEW_Y / 2;
	viewcol = mf->col - VIEW_X / 2;
}

void
upview(void)
{
	viewrow -= VIEW_Y / 3;
}

void
downview(void)
{
	viewrow += VIEW_Y / 3;
}

void
leftview(void)
{
	viewcol -= VIEW_X / 5;
}

void
rightview(void)
{
	viewcol += VIEW_X / 5;
}

/* Called from newturn()... rename? */
void
display_adjust_view(void)
{
	if (dont_adjust)
		return;
	if (mf->row < viewrow + VIEW_Y/4)
		viewrow = mf->row - (VIEW_Y - VIEW_Y/4);
	else if (mf->row > viewrow + (VIEW_Y - VIEW_Y/4))
		viewrow = mf->row - VIEW_Y/4;
	if (mf->col < viewcol + VIEW_X/8)
		viewcol = mf->col - (VIEW_X - VIEW_X/8);
	else if (mf->col > viewcol + (VIEW_X - VIEW_X/8))
		viewcol = mf->col - VIEW_X/8;
}
