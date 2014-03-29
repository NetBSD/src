/*	$NetBSD: draw.c,v 1.9 2014/03/29 19:41:10 dholland Exp $	*/
/*
 * Copyright (c) 1983-2003, Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 * + Redistributions of source code must retain the above copyright 
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution.
 * + Neither the name of the University of California, San Francisco nor 
 *   the names of its contributors may be used to endorse or promote 
 *   products derived from this software without specific prior written 
 *   permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: draw.c,v 1.9 2014/03/29 19:41:10 dholland Exp $");
#endif /* not lint */

#include "hunt.h"

static void drawstatus(PLAYER *);
static void see(PLAYER *, int);
static char translate(char);
static int player_sym(PLAYER *, int, int);

void
drawmaze(PLAYER *pp)
{
	int x;
	char *sp;
	int y;
	char *endp;

	clrscr(pp);
	outstr(pp, pp->p_maze[0], WIDTH);
	for (y = 1; y < HEIGHT - 1; y++) {
		endp = &pp->p_maze[y][WIDTH];
		for (x = 0, sp = pp->p_maze[y]; sp < endp; x++, sp++)
			if (*sp != SPACE) {
				cgoto(pp, y, x);
				if (pp->p_x == x && pp->p_y == y)
					outch(pp, translate(*sp));
				else if (isplayer(*sp))
					outch(pp, player_sym(pp, y, x));
				else
					outch(pp, *sp);
			}
	}
	cgoto(pp, HEIGHT - 1, 0);
	outstr(pp, pp->p_maze[HEIGHT - 1], WIDTH);
	drawstatus(pp);
}

/*
 * drawstatus - put up the status lines (this assumes the screen
 *		size is 80x24 with the maze being 64x24)
 */
static void
drawstatus(PLAYER *pp)
{
	int i;
	PLAYER *np;

	cgoto(pp, STAT_AMMO_ROW, STAT_LABEL_COL);
	outstr(pp, "Ammo:", 5);
	(void) snprintf(Buf, sizeof(Buf), "%3d", pp->p_ammo);
	cgoto(pp, STAT_AMMO_ROW, STAT_VALUE_COL);
	outstr(pp, Buf, 3);

	cgoto(pp, STAT_GUN_ROW, STAT_LABEL_COL);
	outstr(pp, "Gun:", 4);
	cgoto(pp, STAT_GUN_ROW, STAT_VALUE_COL);
	outstr(pp, (pp->p_ncshot < MAXNCSHOT) ? " ok" : "   ", 3);

	cgoto(pp, STAT_DAM_ROW, STAT_LABEL_COL);
	outstr(pp, "Damage:", 7);
	(void) snprintf(Buf, sizeof(Buf), "%2d/%2d", pp->p_damage,
		pp->p_damcap);
	cgoto(pp, STAT_DAM_ROW, STAT_VALUE_COL);
	outstr(pp, Buf, 5);

	cgoto(pp, STAT_KILL_ROW, STAT_LABEL_COL);
	outstr(pp, "Kills:", 6);
	(void) snprintf(Buf, sizeof(Buf), "%3d", (pp->p_damcap - MAXDAM) / 2);
	cgoto(pp, STAT_KILL_ROW, STAT_VALUE_COL);
	outstr(pp, Buf, 3);

	cgoto(pp, STAT_PLAY_ROW, STAT_LABEL_COL);
	outstr(pp, "Player:", 7);
	for (i = STAT_PLAY_ROW + 1, np = Player; np < End_player; np++) {
		(void) snprintf(Buf, sizeof(Buf), "%5.2f%c%-10.10s %c",
			np->p_ident->i_score,
			stat_char(np), np->p_ident->i_name,
			np->p_ident->i_team);
		cgoto(pp, i++, STAT_NAME_COL);
		outstr(pp, Buf, STAT_NAME_LEN);
	}

#ifdef MONITOR
	cgoto(pp, STAT_MON_ROW, STAT_LABEL_COL);
	outstr(pp, "Monitor:", 8);
	for (i = STAT_MON_ROW + 1, np = Monitor; np < End_monitor; np++) {
		(void) snprintf(Buf, sizeof(Buf), "%5.5s %-10.10s %c", " ",
			np->p_ident->i_name, np->p_ident->i_team);
		cgoto(pp, i++, STAT_NAME_COL);
		outstr(pp, Buf, STAT_NAME_LEN);
	}
#endif
}

void
look(PLAYER *pp)
{
	int x, y;

	x = pp->p_x;
	y = pp->p_y;

	check(pp, y - 1, x - 1);
	check(pp, y - 1, x    );
	check(pp, y - 1, x + 1);
	check(pp, y    , x - 1);
	check(pp, y    , x    );
	check(pp, y    , x + 1);
	check(pp, y + 1, x - 1);
	check(pp, y + 1, x    );
	check(pp, y + 1, x + 1);

	switch (pp->p_face) {
	  case LEFTS:
		see(pp, LEFTS);
		see(pp, ABOVE);
		see(pp, BELOW);
		break;
	  case RIGHT:
		see(pp, RIGHT);
		see(pp, ABOVE);
		see(pp, BELOW);
		break;
	  case ABOVE:
		see(pp, ABOVE);
		see(pp, LEFTS);
		see(pp, RIGHT);
		break;
	  case BELOW:
		see(pp, BELOW);
		see(pp, LEFTS);
		see(pp, RIGHT);
		break;
#ifdef FLY
	  case FLYER:
		break;
#endif
	}
	cgoto(pp, y, x);
}

static void
see(PLAYER *pp, int face)
{
	char *sp;
	int y, x, i, cnt;

	x = pp->p_x;
	y = pp->p_y;

	switch (face) {
	  case LEFTS:
		sp = &Maze[y][x];
		for (i = 0; See_over[(int)*--sp]; i++)
			continue;

		if (i == 0)
			break;

		cnt = i;
		x = pp->p_x - 1;
		--y;
		while (i--)
			check(pp, y, --x);
		i = cnt;
		x = pp->p_x - 1;
		++y;
		while (i--)
			check(pp, y, --x);
		i = cnt;
		x = pp->p_x - 1;
		++y;
		while (i--)
			check(pp, y, --x);
		break;
	  case RIGHT:
		sp = &Maze[y][++x];
		for (i = 0; See_over[(int)*sp++]; i++)
			continue;

		if (i == 0)
			break;

		cnt = i;
		x = pp->p_x + 1;
		--y;
		while (i--)
			check(pp, y, ++x);
		i = cnt;
		x = pp->p_x + 1;
		++y;
		while (i--)
			check(pp, y, ++x);
		i = cnt;
		x = pp->p_x + 1;
		++y;
		while (i--)
			check(pp, y, ++x);
		break;
	  case ABOVE:
		sp = &Maze[--y][x];
		if (!See_over[(int)*sp])
			break;
		do {
			--y;
			sp -= sizeof Maze[0];
			check(pp, y, x - 1);
			check(pp, y, x    );
			check(pp, y, x + 1);
		} while (See_over[(int)*sp]);
		break;
	  case BELOW:
		sp = &Maze[++y][x];
		if (!See_over[(int)*sp])
			break;
		do {
			y++;
			sp += sizeof Maze[0];
			check(pp, y, x - 1);
			check(pp, y, x    );
			check(pp, y, x + 1);
		} while (See_over[(int)*sp]);
		break;
	}
}

void
check(PLAYER *pp, int y, int x)
{
	int indx;
	int ch;
	PLAYER *rpp;

	indx = y * sizeof Maze[0] + x;
	ch = ((char *) Maze)[indx];
	if (ch != ((char *) pp->p_maze)[indx]) {
		rpp = pp;
		cgoto(rpp, y, x);
		if (x == rpp->p_x && y == rpp->p_y)
			outch(rpp, translate(ch));
		else if (isplayer(ch))
			outch(rpp, player_sym(rpp, y, x));
		else
			outch(rpp, ch);
		((char *) rpp->p_maze)[indx] = ch;
	}
}

/*
 * showstat
 *	Update the status of players
 */
void
showstat(PLAYER *pp)
{
	PLAYER *np;
	int y;
	char c;

	y = STAT_PLAY_ROW + 1 + (pp - Player);
	c = stat_char(pp);
#ifdef MONITOR
	for (np = Monitor; np < End_monitor; np++) {
		cgoto(np, y, STAT_SCAN_COL);
		outch(np, c);
	}
#endif
	for (np = Player; np < End_player; np++) {
		cgoto(np, y, STAT_SCAN_COL);
		outch(np, c);
	}
}

/*
 * drawplayer:
 *	Draw the player on the screen and show him to everyone who's scanning
 *	unless he is cloaked.
 */
void
drawplayer(PLAYER *pp, bool draw)
{
	PLAYER *newp;
	int x, y;

	x = pp->p_x;
	y = pp->p_y;
	Maze[y][x] = draw ? pp->p_face : pp->p_over;

#ifdef MONITOR
	for (newp = Monitor; newp < End_monitor; newp++)
		check(newp, y, x);
#endif

	for (newp = Player; newp < End_player; newp++) {
		if (!draw || newp == pp) {
			check(newp, y, x);
			continue;
		}
		if (newp->p_scan == 0) {
			newp->p_scan--;
			showstat(newp);
		}
		else if (newp->p_scan > 0) {
			if (pp->p_cloak < 0)
				check(newp, y, x);
			newp->p_scan--;
		}
	}
	if (!draw || pp->p_cloak < 0)
		return;
	if (pp->p_cloak-- == 0)
		showstat(pp);
}

void
message(PLAYER *pp, const char *s)
{
	cgoto(pp, HEIGHT, 0);
	outstr(pp, s, strlen(s));
	ce(pp);
}

/*
 * translate:
 *	Turn a character into the right direction character if we are
 *	looking at the current player.
 */
static char
translate(char ch)
{
	switch (ch) {
	  case LEFTS:
		return '<';
	  case RIGHT:
		return '>';
	  case ABOVE:
		return '^';
	  case BELOW:
		return 'v';
	}
	return ch;
}

/*
 * player_sym:
 *	Return the player symbol
 */
static int
player_sym(PLAYER *pp, int y, int x)
{
	PLAYER *npp;

	npp = play_at(y, x);
	if (npp->p_ident->i_team == ' ')
		return Maze[y][x];
#ifdef MONITOR
	if (pp->p_ident->i_team == '*')
		return npp->p_ident->i_team;
#endif
	if (pp->p_ident->i_team != npp->p_ident->i_team)
		return Maze[y][x];
	return pp->p_ident->i_team;
}
