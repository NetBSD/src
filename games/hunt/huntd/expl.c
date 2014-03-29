/*	$NetBSD: expl.c,v 1.8 2014/03/29 21:33:41 dholland Exp $	*/
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
__RCSID("$NetBSD: expl.c,v 1.8 2014/03/29 21:33:41 dholland Exp $");
#endif /* not lint */

#include <stdlib.h>
#include "hunt.h"


static EXPL *Expl[EXPLEN];		/* explosion lists */
static EXPL *Last_expl;			/* last explosion on Expl[0] */

static void remove_wall(int, int);


/*
 * showexpl:
 *	Show the explosions as they currently are
 */
void
showexpl(int y, int x, char type)
{
	PLAYER *pp;
	EXPL *ep;

	if (y < 0 || y >= HEIGHT)
		return;
	if (x < 0 || x >= WIDTH)
		return;
	ep = malloc(sizeof(*ep));
	ep->e_y = y;
	ep->e_x = x;
	ep->e_char = type;
	ep->e_next = NULL;
	if (Last_expl == NULL)
		Expl[0] = ep;
	else
		Last_expl->e_next = ep;
	Last_expl = ep;
	for (pp = Player; pp < End_player; pp++) {
		if (pp->p_maze[y][x] == type)
			continue;
		pp->p_maze[y][x] = type;
		cgoto(pp, y, x);
		outch(pp, type);
	}
#ifdef MONITOR
	for (pp = Monitor; pp < End_monitor; pp++) {
		if (pp->p_maze[y][x] == type)
			continue;
		pp->p_maze[y][x] = type;
		cgoto(pp, y, x);
		outch(pp, type);
	}
#endif
	switch (Maze[y][x]) {
	  case WALL1:
	  case WALL2:
	  case WALL3:
#ifdef RANDOM
	  case DOOR:
#endif
#ifdef REFLECT
	  case WALL4:
	  case WALL5:
#endif
		if (y >= UBOUND && y < DBOUND && x >= LBOUND && x < RBOUND)
			remove_wall(y, x);
		break;
	}
}

/*
 * rollexpl:
 *	Roll the explosions over, so the next one in the list is at the
 *	top
 */
void
rollexpl(void)
{
	EXPL *ep;
	PLAYER *pp;
	int y, x;
	char c;
	EXPL *nextep;

	for (ep = Expl[EXPLEN - 1]; ep != NULL; ep = nextep) {
		nextep = ep->e_next;
		y = ep->e_y;
		x = ep->e_x;
		if (y < UBOUND || y >= DBOUND || x < LBOUND || x >= RBOUND)
			c = Maze[y][x];
		else
			c = SPACE;
		for (pp = Player; pp < End_player; pp++)
			if (pp->p_maze[y][x] == ep->e_char) {
				pp->p_maze[y][x] = c;
				cgoto(pp, y, x);
				outch(pp, c);
			}
#ifdef MONITOR
		for (pp = Monitor; pp < End_monitor; pp++)
			check(pp, y, x);
#endif
		free(ep);
	}
	for (x = EXPLEN - 1; x > 0; x--)
		Expl[x] = Expl[x - 1];
	Last_expl = Expl[0] = NULL;
}

/* There's about 700 walls in the initial maze.  So we pick a number
 * that keeps the maze relatively full. */
#define MAXREMOVE	40

static REGEN removed[MAXREMOVE];
static REGEN *rem_index = removed;

/*
 * remove_wall - add a location where the wall was blown away.
 *		 if there is no space left over, put the a wall at
 *		 the location currently pointed at.
 */
static void
remove_wall(int y, int x)
{
	REGEN *r;
#if defined(MONITOR) || defined(FLY)
	PLAYER *pp;
#endif
#ifdef FLY
	char save_char = 0;
#endif

	r = rem_index;
	while (r->r_y != 0) {
#ifdef FLY
		switch (Maze[r->r_y][r->r_x]) {
		  case SPACE:
		  case LEFTS:
		  case RIGHT:
		  case ABOVE:
		  case BELOW:
		  case FLYER:
			save_char = Maze[r->r_y][r->r_x];
			goto found;
		}
#else
		if (Maze[r->r_y][r->r_x] == SPACE)
			break;
#endif
		if (++r >= &removed[MAXREMOVE])
			r = removed;
	}

found:
	if (r->r_y != 0) {
		/* Slot being used, put back this wall */
#ifdef FLY
		if (save_char == SPACE)
			Maze[r->r_y][r->r_x] = Orig_maze[r->r_y][r->r_x];
		else {
			pp = play_at(r->r_y, r->r_x);
			if (pp->p_flying >= 0)
				pp->p_flying += rand_num(10);
			else {
				pp->p_flying = rand_num(20);
				pp->p_flyx = 2 * rand_num(6) - 5;
				pp->p_flyy = 2 * rand_num(6) - 5;
			}
			pp->p_over = Orig_maze[r->r_y][r->r_x];
			pp->p_face = FLYER;
			Maze[r->r_y][r->r_x] = FLYER;
			showexpl(r->r_y, r->r_x, FLYER);
		}
#else
		Maze[r->r_y][r->r_x] = Orig_maze[r->r_y][r->r_x];
#endif
#ifdef RANDOM
		if (rand_num(100) == 0)
			Maze[r->r_y][r->r_x] = DOOR;
#endif
#ifdef REFLECT
		if (rand_num(100) == 0)		/* one percent of the time */
			Maze[r->r_y][r->r_x] = WALL4;
#endif
#ifdef MONITOR
		for (pp = Monitor; pp < End_monitor; pp++)
			check(pp, r->r_y, r->r_x);
#endif
	}

	r->r_y = y;
	r->r_x = x;
	if (++r >= &removed[MAXREMOVE])
		rem_index = removed;
	else
		rem_index = r;

	Maze[y][x] = SPACE;
#ifdef MONITOR
	for (pp = Monitor; pp < End_monitor; pp++)
		check(pp, y, x);
#endif
}

/*
 * clearwalls:
 *	Clear out the walls array
 */
void
clearwalls(void)
{
	REGEN *rp;

	for (rp = removed; rp < &removed[MAXREMOVE]; rp++)
		rp->r_y = 0;
	rem_index = removed;
}
