/*	$NetBSD: graphics.c,v 1.16.12.1 2014/08/20 00:00:21 tls Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ed James.
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

/*
 * Copyright (c) 1987 by Ed James, UC Berkeley.  All rights reserved.
 *
 * Copy permission is hereby granted provided that this notice is
 * retained on all partial or complete copies.
 *
 * For more info on this and all of my stuff, mail edjames@berkeley.edu.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)graphics.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: graphics.c,v 1.16.12.1 2014/08/20 00:00:21 tls Exp $");
#endif
#endif /* not lint */

#include "include.h"

#define C_TOPBOTTOM		'-'
#define C_LEFTRIGHT		'|'
#define C_AIRPORT		'='
#define C_LINE			'+'
#define C_BACKROUND		'.'
#define C_BEACON		'*'
#define C_CREDIT		'*'

static void draw_line(WINDOW *, int, int, int, int, const char *);

static WINDOW *radar, *cleanradar, *credit, *input, *planes;

int
getAChar(void)
{
	int c;

	errno = 0;
	while ((c = getchar()) == EOF && errno == EINTR) {
		errno = 0;
		clearerr(stdin);
	}
	return(c);
}

void
erase_all(void)
{
	PLANE	*pp;

	for (pp = air.head; pp != NULL; pp = pp->next) {
		(void)wmove(cleanradar, pp->ypos, pp->xpos * 2);
		(void)wmove(radar, pp->ypos, pp->xpos * 2);
		(void)waddch(radar, winch(cleanradar));
		(void)wmove(cleanradar, pp->ypos, pp->xpos * 2 + 1);
		(void)wmove(radar, pp->ypos, pp->xpos * 2 + 1);
		(void)waddch(radar, winch(cleanradar));
	}
}

void
draw_all(void)
{
	PLANE	*pp;

	for (pp = air.head; pp != NULL; pp = pp->next) {
		if (pp->status == S_MARKED)
			(void)wstandout(radar);
		(void)wmove(radar, pp->ypos, pp->xpos * 2);
		(void)waddch(radar, name(pp));
		(void)waddch(radar, '0' + pp->altitude);
		if (pp->status == S_MARKED)
			(void)wstandend(radar);
	}
	(void)wrefresh(radar);
	(void)planewin();
	(void)wrefresh(input);		/* return cursor */
	(void)fflush(stdout);
}

void
init_gr(void)
{
	static char	buffer[BUFSIZ];

	if (!initscr())
		errx(0, "couldn't initialize screen");
	setbuf(stdout, buffer);
	input = newwin(INPUT_LINES, COLS - PLANE_COLS, LINES - INPUT_LINES, 0);
	credit = newwin(INPUT_LINES, PLANE_COLS, LINES - INPUT_LINES, 
		COLS - PLANE_COLS);
	planes = newwin(LINES - INPUT_LINES, PLANE_COLS, 0, COLS - PLANE_COLS);
}

void
setup_screen(const C_SCREEN *scp)
{
	int	i, j;
	unsigned iu;
	char	str[3];
	const char *airstr;

	str[2] = '\0';

	if (radar != NULL)
		(void)delwin(radar);
	radar = newwin(scp->height, scp->width * 2, 0, 0);

	if (cleanradar != NULL)
		(void)delwin(cleanradar);
	cleanradar = newwin(scp->height, scp->width * 2, 0, 0);

	/* minus one here to prevent a scroll */
	for (i = 0; i < PLANE_COLS - 1; i++) {
		(void)wmove(credit, 0, i);
		(void)waddch(credit, C_CREDIT);
		(void)wmove(credit, INPUT_LINES - 1, i);
		(void)waddch(credit, C_CREDIT);
	}
	(void)wmove(credit, INPUT_LINES / 2, 1);
	(void)waddstr(credit, AUTHOR_STR);

	for (i = 1; i < scp->height - 1; i++) {
		for (j = 1; j < scp->width - 1; j++) {
			(void)wmove(radar, i, j * 2);
			(void)waddch(radar, C_BACKROUND);
		}
	}

	/*
	 * Draw the lines first, since people like to draw lines
	 * through beacons and exit points.
	 */
	str[0] = C_LINE;
	for (iu = 0; iu < scp->num_lines; iu++) {
		str[1] = ' ';
		draw_line(radar, scp->line[iu].p1.x, scp->line[iu].p1.y,
			scp->line[iu].p2.x, scp->line[iu].p2.y, str);
	}

	str[0] = C_TOPBOTTOM;
	str[1] = C_TOPBOTTOM;
	(void)wmove(radar, 0, 0);
	for (i = 0; i < scp->width - 1; i++)
		(void)waddstr(radar, str);
	(void)waddch(radar, C_TOPBOTTOM);

	str[0] = C_TOPBOTTOM;
	str[1] = C_TOPBOTTOM;
	(void)wmove(radar, scp->height - 1, 0);
	for (i = 0; i < scp->width - 1; i++)
		(void)waddstr(radar, str);
	(void)waddch(radar, C_TOPBOTTOM);

	for (i = 1; i < scp->height - 1; i++) {
		(void)wmove(radar, i, 0);
		(void)waddch(radar, C_LEFTRIGHT);
		(void)wmove(radar, i, (scp->width - 1) * 2);
		(void)waddch(radar, C_LEFTRIGHT);
	}

	str[0] = C_BEACON;
	for (iu = 0; iu < scp->num_beacons; iu++) {
		str[1] = '0' + iu;
		(void)wmove(radar, scp->beacon[iu].y, scp->beacon[iu].x * 2);
		(void)waddstr(radar, str);
	}

	for (iu = 0; iu < scp->num_exits; iu++) {
		(void)wmove(radar, scp->exit[iu].y, scp->exit[iu].x * 2);
		(void)waddch(radar, '0' + iu);
	}

	airstr = "^?>?v?<?";
	for (iu = 0; iu < scp->num_airports; iu++) {
		str[0] = airstr[scp->airport[iu].dir];
		str[1] = '0' + iu;
		(void)wmove(radar, scp->airport[iu].y, scp->airport[iu].x * 2);
		(void)waddstr(radar, str);
	}
	
	(void)overwrite(radar, cleanradar);
	(void)wrefresh(radar);
	(void)wrefresh(credit);
	(void)fflush(stdout);
}

static void
draw_line(WINDOW *w, int x, int y, int lx, int ly, const char *s)
{
	int	dx, dy;

	dx = SGN(lx - x);
	dy = SGN(ly - y);
	for (;;) {
		(void)wmove(w, y, x * 2);
		(void)waddstr(w, s);
		if (x == lx && y == ly)
			break;
		x += dx;
		y += dy;
	}
}

void
ioclrtoeol(int pos)
{
	(void)wmove(input, 0, pos);
	(void)wclrtoeol(input);
	(void)wrefresh(input);
	(void)fflush(stdout);
}

void
iomove(int pos)
{
	(void)wmove(input, 0, pos);
	(void)wrefresh(input);
	(void)fflush(stdout);
}

void
ioaddstr(int pos, const char *str)
{
	(void)wmove(input, 0, pos);
	(void)waddstr(input, str);
	(void)wrefresh(input);
	(void)fflush(stdout);
}

void
ioclrtobot(void)
{
	(void)wclrtobot(input);
	(void)wrefresh(input);
	(void)fflush(stdout);
}

void
ioerror(int pos, int len, const char *str)
{
	int	i;

	(void)wmove(input, 1, pos);
	for (i = 0; i < len; i++)
		(void)waddch(input, '^');
	(void)wmove(input, 2, 0);
	(void)waddstr(input, str);
	(void)wrefresh(input);
	(void)fflush(stdout);
}

/* ARGSUSED */
void
quit(int dummy __unused)
{
	int			c, y, x;
#ifdef BSD
	struct itimerval	itv;
#endif

	getyx(input, y, x);
	(void)wmove(input, 2, 0);
	(void)waddstr(input, "Really quit? (y/n) ");
	(void)wclrtobot(input);
	(void)wrefresh(input);
	(void)fflush(stdout);

	c = getchar();
	if (c == EOF || c == 'y') {
		/* disable timer */
#ifdef BSD
		itv.it_value.tv_sec = 0;
		itv.it_value.tv_usec = 0;
		(void)setitimer(ITIMER_REAL, &itv, NULL);
#endif
#ifdef SYSV
		alarm(0);
#endif
		(void)fflush(stdout);
		(void)clear();
		(void)refresh();
		(void)endwin();
		(void)log_score(0);
		exit(0);
	}
	(void)wmove(input, 2, 0);
	(void)wclrtobot(input);
	(void)wmove(input, y, x);
	(void)wrefresh(input);
	(void)fflush(stdout);
}

void
planewin(void)
{
	PLANE	*pp;
	int	warning = 0;

#ifdef BSD
	(void)wclear(planes);
#endif

	(void)wmove(planes, 0,0);

#ifdef SYSV
	wclrtobot(planes);
#endif
	(void)wprintw(planes, "Time: %-4d Safe: %d", clck, safe_planes);
	(void)wmove(planes, 2, 0);

	(void)waddstr(planes, "pl dt  comm");
	for (pp = air.head; pp != NULL; pp = pp->next) {
		if (waddch(planes, '\n') == ERR) {
			warning++;
			break;
		}
		(void)waddstr(planes, command(pp));
	}
	(void)waddch(planes, '\n');
	for (pp = ground.head; pp != NULL; pp = pp->next) {
		if (waddch(planes, '\n') == ERR) {
			warning++;
			break;
		}
		(void)waddstr(planes, command(pp));
	}
	if (warning) {
		(void)wmove(planes, LINES - INPUT_LINES - 1, 0);
		(void)waddstr(planes, "---- more ----");
		(void)wclrtoeol(planes);
	}
	(void)wrefresh(planes);
	(void)fflush(stdout);
}

void
loser(const PLANE *p, const char *s)
{
	int			c;
#ifdef BSD
	struct itimerval	itv;
#endif

	/* disable timer */
#ifdef BSD
	itv.it_value.tv_sec = 0;
	itv.it_value.tv_usec = 0;
	(void)setitimer(ITIMER_REAL, &itv, NULL);
#endif
#ifdef SYSV
	alarm(0);
#endif

	(void)wmove(input, 0, 0);
	(void)wclrtobot(input);
	/* p may be NULL if we ran out of memory */
	if (p == NULL)
		(void)wprintw(input, "%s\n\nHit space for top players list...",
		    s);
	else {
		(void)wprintw(input, "Plane '%c' %s\n\n", name(p), s);
		(void)wprintw(input, "Hit space for top players list...");
	}
	(void)wrefresh(input);
	(void)fflush(stdout);
	while ((c = getchar()) != EOF && c != ' ')
		;
	(void)clear();	/* move to top of screen */
	(void)refresh();
	(void)endwin();
	(void)log_score(0);
	exit(0);
}

void
redraw(void)
{
	(void)clear();
	(void)refresh();

	(void)touchwin(radar);
	(void)wrefresh(radar);
	(void)touchwin(planes);
	(void)wrefresh(planes);
	(void)touchwin(credit);
	(void)wrefresh(credit);

	/* refresh input last to get cursor in right place */
	(void)touchwin(input);
	(void)wrefresh(input);
	(void)fflush(stdout);
}

void
done_screen(void)
{
	(void)clear();
	(void)refresh();
	(void)endwin();	  /* clean up curses */
}
