/*	$NetBSD: pl_7.c,v 1.42 2011/08/26 06:18:18 dholland Exp $	*/

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
__RCSID("$NetBSD: pl_7.c,v 1.42 2011/08/26 06:18:18 dholland Exp $");
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
#include <unistd.h>
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
static bool ingame;
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
#if 0
	for (ch = 0; ch < 127; ch++) {
		if (ch != '[' && ch != 'O') {
			define_esc_key(ch);
		}
	}
#else
	(void)ch;
	(void)define_esc_key;
#endif

	keypad(stdscr, 1);
	keypad(view_w, 1);
	keypad(slot_w, 1);
	keypad(scroll_w, 1);
	keypad(stat_w, 1);
	keypad(turn_w, 1);

	done_curses++;
}

void
cleanupscreen(void)
{
	/* alarm already turned off */
	if (done_curses) {
		if (ingame) {
			wmove(scroll_w, SCROLL_Y - 1, 0);
			wclrtoeol(scroll_w);
			display_redraw();
		} else {
			move(LINES-1, 0);
			clrtoeol();
		}
		endwin();
	}
}

////////////////////////////////////////////////////////////
// curses utility functions

/*
 * fill to eol with spaces
 * (useful with A_REVERSE since cleartoeol() does not clear to reversed)
 */
static void
filltoeol(void)
{
	int x;

	for (x = getcurx(stdscr); x < COLS; x++) {
		addch(' ');
	}
}

/*
 * Add a maybe-selected string.
 *
 * Place strings starting at (Y0, X0); this is string ITEM; CURITEM
 * is the selected one; WIDTH is the total width. STR is the string.
 */
static void
mvaddselstr(int y, int x0, int item, int curitem,
	    size_t width, const char *str)
{
	size_t i, len;

	len = strlen(str);

	move(y, x0);
	if (curitem == item) {
		attron(A_REVERSE);
	}
	addstr(str);
	if (curitem == item) {
		for (i=len; i<width; i++) {
			addch(' ');
		}
		attroff(A_REVERSE);
	}
}

/*
 * Likewise but a printf.
 */
static void __printflike(6, 7)
mvselprintw(int y, int x0, int item, int curitem,
	    size_t width, const char *fmt, ...)
{
	va_list ap;
	size_t x;

	move(y, x0);
	if (curitem == item) {
		attron(A_REVERSE);
	}
	va_start(ap, fmt);
	vwprintw(stdscr, fmt, ap);
	va_end(ap);
	if (curitem == item) {
		for (x = getcurx(stdscr); x < x0 + width; x++) {
			addch(' ');
		}
		attroff(A_REVERSE);
	}
}

/*
 * Move up by 1, scrolling if needed.
 */
static void
up(int *posp, int *scrollp)
{
	if (*posp > 0) {
		(*posp)--;
	}
	if (scrollp != NULL) {
		if (*posp < *scrollp) {
			*scrollp = *posp;
		}
	}
}

/*
 * Move down by 1, scrolling if needed. MAX is the total number of
 * items; VISIBLE is the number that can be visible at once.
 */
static void
down(int *posp, int *scrollp, int max, int visible)
{
	if (max > 0 && *posp < max - 1) {
		(*posp)++;
	}
	if (scrollp != NULL) {
		if (*posp > *scrollp + visible - 1) {
			*scrollp = *posp - visible + 1;
		}
	}
}

/*
 * Complain briefly.
 */
static void __printflike(3, 4)
oops(int y, int x, const char *fmt, ...)
{
	int oy, ox;
	va_list ap;

	oy = getcury(stdscr);
	ox = getcurx(stdscr);
	move(y, x);
	va_start(ap, fmt);
	vwprintw(stdscr, fmt, ap);
	va_end(ap);
	move(oy, ox);
	wrefresh(stdscr);
	sleep(1);
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

////////////////////////////////////////////////////////////
// starting game

static bool shipselected;
static int loadpos;

static int
nextload(int load)
{
	switch (load) {
	    case L_ROUND: return L_GRAPE;
	    case L_GRAPE: return L_CHAIN;
	    case L_CHAIN: return L_DOUBLE;
	    case L_DOUBLE: return L_ROUND;
	}
	return L_ROUND;
}

static int
loadbychar(int ch)
{
	switch (ch) {
	    case 'r': return L_ROUND;
	    case 'g': return L_GRAPE;
	    case 'c': return L_CHAIN;
	    case 'd': return L_DOUBLE;
	}
	return L_ROUND;
}

static const char *
loadstr(int load)
{
	switch (load) {
	    case L_ROUND: return "round";
	    case L_GRAPE: return "grape";
	    case L_CHAIN: return "chain";
	    case L_DOUBLE: return "double";
	}
	return "???";
}

static void
displayshiplist(void)
{
	struct ship *sp;
	int which;

	erase();

	attron(A_BOLD);
	mvaddstr(1, 4, cc->name);
	attroff(A_BOLD);

	which = 0;
	foreachship(sp) {
		mvselprintw(which + 3, 4, which, player, 60,
			    "  %2d:  %-10s %-15s  (%-2d pts)   %s",
			    sp->file->index,
			    countryname[sp->nationality],
			    sp->shipname,
			    sp->specs->pts,
			    saywhat(sp, 1));
		which++;
	}

	if (!shipselected) {
		mvaddstr(15, 4, "Choose your ship");
		move(player + 3, 63);
	} else {
		mvselprintw(15, 4, 0, loadpos, 32,
			    "Initial left broadside: %s", loadstr(mf->loadL));
		mvselprintw(16, 4, 1, loadpos, 32,
			    "Initial right broadside: %s", loadstr(mf->loadR));
		mvselprintw(17, 4, 2, loadpos, 32, "Set sail");
		move(loadpos+15, 35);
	}

	wrefresh(stdscr);
}

static int
pickship(void)
{
	struct File *fp;
	struct ship *sp;
	bool done;
	int ch;

	for (;;) {
		foreachship(sp)
			if (sp->file->captain[0] == 0 && !sp->file->struck
			    && sp->file->captured == 0)
				break;
		if (sp >= ls) {
			return -1;
		}
		player = sp - SHIP(0);
		if (randomize) {
			/* nothing */
		} else {
			done = false;
			while (!done) {
				displayshiplist();

				ch = getch();
				switch (ch) {
				    case 12 /*^L*/:
					clear();
					break;
				    case '\r':
				    case '\n':
					done = true;
					break;
				    case 7 /*^G*/:
				    case 8 /*^H*/:
				    case 27 /*ESC*/:
				    case 127 /*^?*/:
					beep();
					break;
				    case 16 /*^P*/:
				    case KEY_UP:
					up(&player, NULL);
					break;
				    case 14 /*^N*/:
				    case KEY_DOWN:
					down(&player, NULL, cc->vessels,
					     cc->vessels);
					break;
				    default:
					beep();
					break;
				}
			}
		}
		if (player < 0)
			continue;
		if (Sync() < 0)
			leave(LEAVE_SYNC);
		fp = SHIP(player)->file;
		if (fp->captain[0] || fp->struck || fp->captured != 0)
			oops(16, 4, "That ship is taken.");
		else
			break;
	}
	return 0;
}

static void
pickload(void)
{
	bool done;
	int ch;

	mf->loadL = L_ROUND;
	mf->loadR = L_ROUND;

	loadpos = 0;
	done = false;
	while (!done) {
		displayshiplist();

		ch = getch();
		switch (ch) {
		    case 12 /*^L*/:
			clear();
			break;
		    case 'r':
		    case 'g':
		    case 'c':
		    case 'd':
			switch (loadpos) {
			    case 0: mf->loadL = loadbychar(ch); break;
			    case 1: mf->loadR = loadbychar(ch); break;
			    case 2: beep(); break;
			}
			break;
		    case '\r':
		    case '\n':
			switch (loadpos) {
			    case 0: mf->loadL = nextload(mf->loadL); break;
			    case 1: mf->loadR = nextload(mf->loadR); break;
			    case 2: done = true; break;
			}
			break;
		    case 7 /*^G*/:
		    case 8 /*^H*/:
		    case 27 /*ESC*/:
		    case 127 /*^?*/:
			beep();
			break;
		    case 16 /*^P*/:
		    case KEY_UP:
			up(&loadpos, NULL);
			break;
		    case 14 /*^N*/:
		    case KEY_DOWN:
			down(&loadpos, NULL, 3, 3);
			break;
		    default:
			beep();
			break;
		}
	}
	mf->readyR = R_LOADED|R_INITIAL;
	mf->readyL = R_LOADED|R_INITIAL;
}

static void
startgame(void)
{

	ingame = true;
	shipselected = false;

	pl_main_init();

	hasdriver = sync_exists(game);
	if (sync_open() < 0) {
		oops(21, 10, "syncfile: %s", strerror(errno));
		pl_main_uninit();
		ingame = false;
		return;
	}

	if (hasdriver) {
		mvaddstr(21, 10, "Synchronizing with the other players...");
		wrefresh(stdscr);
		fflush(stdout);
		if (Sync() < 0)
			leave(LEAVE_SYNC);
	} else {
		mvaddstr(21, 10, "Starting driver...");
		wrefresh(stdscr);
		fflush(stdout);
		startdriver();
	}

	if (pickship() < 0) {
		oops(21, 10, "All ships taken in that scenario.");
		sync_close(0);
		people = 0;
		pl_main_uninit();
		ingame = false;
		return;
	}
	shipselected = true;

	ms = SHIP(player);
	mf = ms->file;
	mc = ms->specs;

	pickload();

	pl_main();
	ingame = false;
}

////////////////////////////////////////////////////////////
// scenario picker

static int pickerpos;
static int pickerscroll;

static const char *
absdirectionname(int dir)
{
	switch (dir) {
	    case 1: return "South";
	    case 2: return "Southwest";
	    case 3: return "West";
	    case 4: return "Northwest";
	    case 5: return "North";
	    case 6: return "Northeast";
	    case 7: return "East";
	    case 8: return "Southeast";
	}
	return "?";
}

static const char *
windname(int wind)
{
	switch (wind) {
	    case 0: return "calm";
	    case 1: return "light breeze";
	    case 2: return "moderate breeze";
	    case 3: return "fresh breeze";
	    case 4: return "strong breeze";
	    case 5: return "gale";
	    case 6: return "full gale";
	    case 7: return "hurricane";
	}
	return "???";
}

static const char *
nationalityname(int nationality)
{
	switch (nationality) {
	    case N_A: return "a";
	    case N_B: return "b";
	    case N_S: return "s";
	    case N_F: return "f";
	    case N_J: return "j";
	    case N_D: return "d";
	    case N_K: return "k";
	    case N_O: return "o";
	}
	return "?";
}

static void
drawpicker(void)
{
	int y, sc, i;
	struct ship *ship;

	erase();

	mvaddstr(0, 0, "## SHIPS  TITLE");
	for (y=1; y<LINES-11; y++) {
		sc = (y-1) + pickerscroll;
		if (sc < NSCENE) {
			mvselprintw(y, 0, sc, pickerpos, 56,
				    "%-2d %-5d  %s",
				    sc, scene[sc].vessels, scene[sc].name);
		}
	}

	mvprintw(2, 60 + 2, "%s wind",
		 absdirectionname(scene[pickerpos].winddir));
	mvprintw(3, 60 + 2, "(%s)",
		 windname(scene[pickerpos].windspeed));

	for (i=0; i<scene[pickerpos].vessels; i++) {
		ship = &scene[pickerpos].ship[i];
		mvprintw(LINES-10 + i, 0,
			 "(%s) %-16s %3d gun %s (%s crew) (%d pts)",
			 nationalityname(ship->nationality),
			 ship->shipname,
			 ship->specs->guns,
			 shortclassname[ship->specs->class],
			 qualname[ship->specs->qual],
			 ship->specs->pts);
	}

	move(1 + pickerpos - pickerscroll, 55);
	wrefresh(stdscr);
}

static int
pickscenario(int initpos)
{
	int ch;

	pickerpos = initpos;
	if (pickerpos < 0) {
		pickerpos = 0;
	}

	while (1) {
		drawpicker();
		ch = getch();
		switch (ch) {
		    case 12 /*^L*/:
			clear();
			break;
		    case '\r':
		    case '\n':
			return pickerpos;
		    case 7 /*^G*/:
		    case 8 /*^H*/:
		    case 27 /*ESC*/:
		    case 127 /*^?*/:
			return initpos;
		    case 16 /*^P*/:
		    case KEY_UP:
			up(&pickerpos, &pickerscroll);
			break;
		    case 14 /*^N*/:
		    case KEY_DOWN:
			down(&pickerpos, &pickerscroll, NSCENE, LINES-12);
			break;
		    default:
			beep();
			break;
		}
	}
	return pickerpos;
}

////////////////////////////////////////////////////////////
// setup menus

#define MAINITEMS_NUM 5
#define STARTITEMS_NUM 4
#define OPTIONSITEMS_NUM 5

static int mainpos;
static bool connected;

static bool joinactive;
static int joinpos;
static int joinscroll;
static int joinable[NSCENE];
static int numjoinable;

static bool startactive;
static int startpos;
static int startscenario;

static bool optionsactive;
static int optionspos;
static char o_myname[MAXNAMESIZE];
static bool o_randomize;
static bool o_longfmt;
static bool o_nobells;


/*
 * this and sgetstr() should share code
 */
static void
startup_getstr(int y, int x, char *buf, size_t max)
{
	size_t pos = 0;
	int ch;

	for (;;) {
		buf[pos] = 0;
		move(y, x);
		addstr(buf);
		clrtoeol();
		wrefresh(stdscr);
		fflush(stdout);

		ch = getch();
		switch (ch) {
		case '\n':
		case '\r':
			return;
		case '\b':
			if (pos > 0) {
				/*waddstr(scroll_w, "\b \b");*/
				pos--;
			}
			break;
		default:
			if (ch >= ' ' && ch < 0x7f && pos < max - 1) {
				buf[pos++] = ch;
			} else {
				beep();
			}
		}
	}
}

static void
changename(void)
{
	mvaddstr(LINES-2, COLS/2, "Enter your name:");
	startup_getstr(LINES-1, COLS/2, o_myname, sizeof(o_myname));
}

static void
checkforgames(void)
{
	int i;
	int prev;

	if (numjoinable > 0) {
		prev = joinable[joinpos];
	} else {
		prev = 0;
	}

	numjoinable = 0;
	for (i = 0; i < NSCENE; i++) {
		if (!sync_exists(i)) {
			continue;
		}
		if (i < prev) {
			joinpos = numjoinable;
		}
		joinable[numjoinable++] = i;
	}
	if (joinpos > numjoinable) {
		joinpos = (numjoinable > 0) ? numjoinable - 1 : 0;
	}
	if (joinscroll > joinpos) {
		joinscroll = (joinpos > 0) ? joinpos - 1 : 0;
	}
}

static void
drawstartmenus(void)
{
	const int mainy0 = 8;
	const int mainx0 = 12;

	erase();

	mvaddstr(5, 10, "Wooden Ships & Iron Men");

	mvaddselstr(mainy0+0, mainx0, 0, mainpos, 17, "Join a game");
	mvaddselstr(mainy0+1, mainx0, 1, mainpos, 17, "Start a game");
	mvaddselstr(mainy0+2, mainx0, 2, mainpos, 17, "Options");
	mvaddselstr(mainy0+3, mainx0, 3, mainpos, 17, "Show high scores");
	mvaddselstr(mainy0+4, mainx0, 4, mainpos, 17, "Quit");

	mvprintw(15, 10, "Captain %s", myname);
	if (connected) {
		mvaddstr(16, 10, "Connected via scratch files.");
	} else {
		mvaddstr(16, 10, "Not connected.");
	}

	if (joinactive) {
		int y0, leavey = 0, i, sc;

		mvaddstr(0, COLS/2, "## SHIPS  TITLE");
		y0 = 1;
		for (i = 0; i < numjoinable; i++) {
			if (i >= joinscroll && i < joinscroll + LINES-1) {
				move(y0 + i - joinscroll, COLS/2);
				if (i == joinpos) {
					attron(A_REVERSE);
				}
				sc = joinable[i];
				printw("%-2d %-5d  %s",
				       sc, scene[sc].vessels, scene[sc].name);
				if (i == joinpos) {
					filltoeol();
					attroff(A_REVERSE);
					leavey = y0 + i - joinscroll;
				}
			}
		}
		mvaddstr(19, 10, "(Esc to abort)");
		if (numjoinable > 0) {
			mvaddstr(18, 10, "Choose a game to join.");
			move(leavey, COLS-1);
		} else {
			mvaddstr(2, COLS/2, "No games.");
			mvaddstr(18, 10, "Press return to refresh.");
		}

	} else if (startactive) {
		const char *name;

		mvaddstr(18, 10, "Start a new game");
		mvaddstr(19, 10, "(Esc to abort)");
		mvaddstr(2, COLS/2, "New game");

		name = (startscenario < 0) ?
			"not selected" : scene[startscenario].name;

		mvselprintw(4, COLS/2, 0, startpos, COLS/2 - 1,
			    "Scenario: %s", name);
		mvaddselstr(5, COLS/2, 1, startpos, COLS/2 - 1,
			    "Visibility: local");
		mvaddselstr(6, COLS/2, 2, startpos, COLS/2 - 1,
			    "Password: unset");
		mvaddselstr(7, COLS/2, 3, startpos, COLS/2 - 1,
			    "Start game");
		move(4+startpos, COLS - 2);

	} else if (optionsactive) {
		mvaddstr(18, 10, "Adjust options");
		mvaddstr(19, 10, "(Esc to abort)");
		mvaddstr(2, COLS/2, "Adjust options");

		mvselprintw(4, COLS/2, 0, optionspos, COLS/2-1,
			    "Your name: %s", o_myname);
		mvselprintw(5, COLS/2, 1, optionspos, COLS/2-1,
			    "Auto-pick ships: %s", o_randomize ? "ON" : "off");
		mvselprintw(6, COLS/2, 2, optionspos, COLS/2-1,
			    "Usernames in scores: %s",
			    o_longfmt ? "ON" : "off");
		mvselprintw(7, COLS/2, 3, optionspos, COLS/2-1,
			    "Beeping: %s", o_nobells ? "OFF" : "on");
		mvselprintw(8, COLS/2, 4, optionspos, COLS/2-1,
			    "Apply changes");
		move(4+optionspos, COLS - 2);

	} else {
		move(mainy0 + mainpos, mainx0 + 16);
	}

	wrefresh(stdscr);
	fflush(stdout);
}

void
startup(void)
{
	int ch;

	connected = false;
	mainpos = 0;

	joinactive = false;
	joinpos = 0;
	joinscroll = 0;
	numjoinable = 0;

	startactive = false;
	startpos = 0;
	startscenario = -1;

	optionsactive = false;
	optionspos = 0;

	while (1) {
		if (joinactive) {
			checkforgames();
		}
		drawstartmenus();
		ch = getch();
		switch (ch) {
		    case 12 /*^L*/:
			clear();
			break;
		    case '\r':
		    case '\n':
			if (joinactive && numjoinable > 0) {
				game = joinable[joinpos];
				startgame();
				joinactive = false;
			} else if (startactive) {
				switch (startpos) {
				    case 0:
					startscenario = pickscenario(startscenario);
					startpos = 3;
					break;
				    case 1:
				    case 2:
					oops(21, 10, "That doesn't work yet.");
					break;
				    case 3:
					if (startscenario >= 0) {
						game = startscenario;
						/* can't do this here yet */
						/*startdriver();*/
						startgame();
						startactive = false;
						startscenario = -1;
					} else {
						oops(21, 10,
						     "Pick a scenario.");
					}
					break;
				}
			} else if (optionsactive) {
				switch (optionspos) {
				    case 0: changename(); break;
				    case 1: o_randomize = !o_randomize; break;
				    case 2: o_longfmt = !o_longfmt; break;
				    case 3: o_nobells = !o_nobells; break;
				    case 4:
					strlcpy(myname, o_myname,
						sizeof(myname));
					randomize = o_randomize;
					longfmt = o_longfmt;
					nobells = o_nobells;
					optionsactive = false;
					break;
				}
			} else {
				switch (mainpos) {
				    case 0: joinactive = true; break;
				    case 1: startactive = true; break;
				    case 2:
					strlcpy(o_myname, myname,
						sizeof(o_myname));
					o_randomize = randomize;
					o_longfmt = longfmt;
					o_nobells = nobells;
					optionsactive = true;
					break;
				    case 3: lo_curses(); break;
				    case 4: return;
				}
			}
			break;
		    case 7 /*^G*/:
		    case 8 /*^H*/:
		    case 27 /*ESC*/:
		    case 127 /*^?*/:
			if (joinactive) {
				joinactive = false;
			} else if (startactive) {
				startactive = false;
			} else if (optionsactive) {
				optionsactive = false;
			} else {
				/* nothing */
			}
			break;
		    case 16 /*^P*/:
		    case KEY_UP:
			if (joinactive) {
				up(&joinpos, &joinscroll);
			} else if (startactive) {
				up(&startpos, NULL);
			} else if (optionsactive) {
				up(&optionspos, NULL);
			} else {
				up(&mainpos, NULL);
			}
			break;
		    case 14 /*^N*/:
		    case KEY_DOWN:
			if (joinactive) {
				down(&joinpos, &joinscroll,
				     numjoinable, LINES-1);
			} else if (startactive) {
				down(&startpos, NULL,
				     STARTITEMS_NUM, STARTITEMS_NUM);
			} else if (optionsactive) {
				down(&optionspos, NULL,
				     OPTIONSITEMS_NUM, OPTIONSITEMS_NUM);
			} else {
				down(&mainpos, NULL,
				     MAINITEMS_NUM, MAINITEMS_NUM);
			}
			break;
		    default:
			beep();
			break;
		}
	}
}
