/*	$NetBSD: pl_7.c,v 1.16 2001/01/01 21:57:38 jwise Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__RCSID("$NetBSD: pl_7.c,v 1.16 2001/01/01 21:57:38 jwise Exp $");
#endif
#endif /* not lint */

#include <sys/ttydefaults.h>
#include "player.h"
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <stdlib.h>
#include <unistd.h>


/*
 * Display interface
 */

static char sc_hasprompt;
static const char *sc_prompt;
static const char *sc_buf;
static int sc_line;

WINDOW *view_w;
WINDOW *slot_w;
WINDOW *scroll_w;
WINDOW *stat_w;
WINDOW *turn_w;

char done_curses;
char loaded, fired, changed, repaired;
char dont_adjust;
int viewrow, viewcol;
char movebuf[sizeof SHIP(0)->file->movebuf];
int player;
struct ship *ms;		/* memorial structure, &cc->ship[player] */
struct File *mf;		/* ms->file */
struct shipspecs *mc;		/* ms->specs */

void
initscreen(void)
{
	if (!SCREENTEST()) {
		printf("Can't sail on this terminal.\n");
		exit(1);
	}
	/* initscr() already done in SCREENTEST() */
	view_w = newwin(VIEW_Y, VIEW_X, VIEW_T, VIEW_L);
	slot_w = newwin(SLOT_Y, SLOT_X, SLOT_T, SLOT_L);
	scroll_w = newwin(SCROLL_Y, SCROLL_X, SCROLL_T, SCROLL_L);
	stat_w = newwin(STAT_Y, STAT_X, STAT_T, STAT_L);
	turn_w = newwin(TURN_Y, TURN_X, TURN_T, TURN_L);
	done_curses++;
	leaveok(view_w, 1);
	leaveok(slot_w, 1);
	leaveok(stat_w, 1);
	leaveok(turn_w, 1);
	noecho();
	crmode();
}

void
cleanupscreen(void)
{
	/* alarm already turned off */
	if (done_curses) {
		wmove(scroll_w, SCROLL_Y - 1, 0);
		wclrtoeol(scroll_w);
		draw_screen();
		endwin();
	}
}

/*ARGSUSED*/
void
newturn(int n __attribute__((__unused__)))
{
	repaired = loaded = fired = changed = 0;
	movebuf[0] = '\0';

	alarm(0);
	if (mf->readyL & R_LOADING) {
		if (mf->readyL & R_DOUBLE)
			mf->readyL = R_LOADING;
		else
			mf->readyL = R_LOADED;
	}
	if (mf->readyR & R_LOADING) {
		if (mf->readyR & R_DOUBLE)
			mf->readyR = R_LOADING;
		else
			mf->readyR = R_LOADED;
	}
	if (!hasdriver)
		Write(W_DDEAD, SHIP(0), 0, 0, 0, 0);

	if (sc_hasprompt) {
		wmove(scroll_w, sc_line, 0);
		wclrtoeol(scroll_w);
	}
	if (Sync() < 0)
		leave(LEAVE_SYNC);
	if (!hasdriver)
		leave(LEAVE_DRIVER);
	if (sc_hasprompt)
		wprintw(scroll_w, "%s%s", sc_prompt, sc_buf);

	if (turn % 50 == 0)
		Write(W_ALIVE, SHIP(0), 0, 0, 0, 0);
	if (mf->FS && (!mc->rig1 || windspeed == 6))
		Write(W_FS, ms, 0, 0, 0, 0);
	if (mf->FS == 1)
		Write(W_FS, ms, 2, 0, 0, 0);

	if (mf->struck)
		leave(LEAVE_QUIT);
	if (mf->captured != 0)
		leave(LEAVE_CAPTURED);
	if (windspeed == 7)
		leave(LEAVE_HURRICAN);

	adjustview();
	draw_screen();

	signal(SIGALRM, newturn);
	alarm(7);
}

/*VARARGS2*/
void
Signal(const char *fmt, struct ship *ship, ...)
{
	va_list ap;
	char format[BUFSIZ];

	va_start(ap, ship);
	if (!done_curses)
		return;
	if (*fmt == '\7')
		putchar(*fmt++);
	fmtship(format, sizeof(format), fmt, ship);
	vwprintw(scroll_w, format, ap);
	va_end(ap);
	Scroll();
}

/*VARARGS2*/
void
Msg(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (!done_curses)
		return;
	if (*fmt == '\7')
		putchar(*fmt++);
	vwprintw(scroll_w, fmt, ap);
	va_end(ap);
	Scroll();
}

void
Scroll(void)
{
	if (++sc_line >= SCROLL_Y)
		sc_line = 0;
	wmove(scroll_w, sc_line, 0);
	wclrtoeol(scroll_w);
}

void
prompt(p, ship)
	const char *p;
	struct ship *ship;
{
	static char buf[BUFSIZ];

	fmtship(buf, sizeof(buf), p, ship);
	sc_prompt = buf;
	sc_buf = "";
	sc_hasprompt = 1;
	waddstr(scroll_w, buf);
}

void
endprompt(int flag)
{
	sc_hasprompt = 0;
	if (flag)
		Scroll();
}

int
sgetch(const char *p, struct ship *ship, int flag)
{
	int c;
	prompt(p, ship);
	blockalarm();
	wrefresh(scroll_w);
	unblockalarm();
	while ((c = wgetch(scroll_w)) == EOF)
		;
	if (flag && c >= ' ' && c < 0x7f)
		waddch(scroll_w, c);
	endprompt(flag);
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
		wrefresh(scroll_w);
		unblockalarm();
		while ((c = wgetch(scroll_w)) == EOF)
			;
		switch (c) {
		case '\n':
		case '\r':
			endprompt(1);
			return;
		case '\b':
			if (p > buf) {
				waddstr(scroll_w, "\b \b");
				p--;
			}
			break;
		default:
			if (c >= ' ' && c < 0x7f && p < buf + n - 1) {
				*p++ = c;
				waddch(scroll_w, c);
			} else
				putchar('\a');
		}
	}
}

void
draw_screen(void)
{
	draw_view();
	draw_turn();
	draw_stat();
	draw_slot();
	wrefresh(scroll_w);		/* move the cursor */
}

void
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

void
draw_turn(void)
{
	wmove(turn_w, 0, 0);
	wprintw(turn_w, "%cTurn %d", dont_adjust?'*':'-', turn);
	wrefresh(turn_w);
}

void
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
	if (!boarding(ms, 0)) {
		mvwaddstr(slot_w, 0, 0, "   ");
		mvwaddstr(slot_w, 1, 0, "   ");
	} else
		mvwaddstr(slot_w, 1, 0, "OBP");
	if (!boarding(ms, 1)) {
		mvwaddstr(slot_w, 2, 0, "   ");
		mvwaddstr(slot_w, 3, 0, "   ");
	} else
		mvwaddstr(slot_w, 3, 0, "DBP");

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

	clear();
	werase(view_w);
	werase(slot_w);
	werase(scroll_w);
	werase(stat_w);
	werase(turn_w);

	sc_line = 0;

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

#define WSaIM "Wooden Ships & Iron Men"
	wmove(view_w, 2, (VIEW_X - sizeof WSaIM - 1) / 2);
	waddstr(view_w, WSaIM);
	wmove(view_w, 4, (VIEW_X - strlen(cc->name)) / 2);
	waddstr(view_w, cc->name);
	wrefresh(view_w);

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

void
adjustview(void)
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
