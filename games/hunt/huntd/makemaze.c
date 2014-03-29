/*	$NetBSD: makemaze.c,v 1.11 2014/03/29 20:44:20 dholland Exp $	*/
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
__RCSID("$NetBSD: makemaze.c,v 1.11 2014/03/29 20:44:20 dholland Exp $");
#endif /* not lint */

#include "hunt.h"

#define ISCLEAR(y,x)	(Maze[y][x] == SPACE)
#define ODD(n)		((n) & 01)

#if 0

#define NPERM	24
#define NDIR	4

static const int dirs[NPERM][NDIR] = {
	{0,1,2,3},	{3,0,1,2},	{0,2,3,1},	{0,3,2,1},
	{1,0,2,3},	{2,3,0,1},	{0,2,1,3},	{2,3,1,0},
	{1,0,3,2},	{1,2,0,3},	{3,1,2,0},	{2,0,3,1},
	{1,3,0,2},	{0,3,1,2},	{1,3,2,0},	{2,0,1,3},
	{0,1,3,2},	{3,1,0,2},	{2,1,0,3},	{1,2,3,0},
	{2,1,3,0},	{3,0,2,1},	{3,2,0,1},	{3,2,1,0}
};

static const int incr[NDIR][2] = {
	{0, 1}, {1, 0}, {0, -1}, {-1, 0}
};


/*
 * candig:
 *	Is it legal to clear this spot?
 */
static bool
candig(int y, int x)
{
	int i;

	if (ODD(x) && ODD(y))
		return false;		/* can't touch ODD spots */

	if (y < UBOUND || y >= DBOUND)
		return false;		/* Beyond vertical bounds, NO */
	if (x < LBOUND || x >= RBOUND)
		return false;		/* Beyond horizontal bounds, NO */

	if (ISCLEAR(y, x))
		return false;		/* Already clear, NO */

	i = ISCLEAR(y, x + 1);
	i += ISCLEAR(y, x - 1);
	if (i > 1)
		return false;		/* Introduces cycle, NO */
	i += ISCLEAR(y + 1, x);
	if (i > 1)
		return false;		/* Introduces cycle, NO */
	i += ISCLEAR(y - 1, x);
	if (i > 1)
		return false;		/* Introduces cycle, NO */

	return true;			/* OK */
}

static void
dig(int y, int x)
{
	const int *dp;
	const int *ip;
	int ny, nx;
	const int *endp;

	Maze[y][x] = SPACE;			/* Clear this spot */
	dp = dirs[rand_num(NPERM)];
	endp = &dp[NDIR];
	while (dp < endp) {
		ip = &incr[*dp++][0];
		ny = y + *ip++;
		nx = x + *ip;
		if (candig(ny, nx))
			dig(ny, nx);
	}
}
#endif

static void
dig_maze(int x, int y)
{
	int tx, ty;
	int i, j;
	int order[4];
#define MNORTH	0x1
#define MSOUTH	0x2
#define MEAST	0x4
#define MWEST	0x8

	tx = ty = 0;
	Maze[y][x] = SPACE;
	order[0] = MNORTH;
	for (i = 1; i < 4; i++) {
		j = rand_num(i + 1);
		order[i] = order[j];
		order[j] = 0x1 << i;
	}
	for (i = 0; i < 4; i++) {
		switch (order[i]) {
		  case MNORTH:
			tx = x;
			ty = y - 2;
			break;
		  case MSOUTH:
			tx = x;
			ty = y + 2;
			break;
		  case MEAST:
			tx = x + 2;
			ty = y;
			break;
		  case MWEST:
			tx = x - 2;
			ty = y;
			break;
		}
		if (tx < 0 || ty < 0 || tx >= WIDTH || ty >= HEIGHT)
			continue;
		if (Maze[ty][tx] == SPACE)
			continue;
		Maze[(y + ty) / 2][(x + tx) / 2] = SPACE;
		dig_maze(tx, ty);
	}
}

static void
remap(void)
{
	int y, x;
	char *sp;
	int stat;

	for (y = 0; y < HEIGHT; y++)
		for (x = 0; x < WIDTH; x++) {
			sp = &Maze[y][x];
			if (*sp == SPACE)
				continue;
			stat = 0;
			if (y - 1 >= 0 && Maze[y - 1][x] != SPACE)
				stat |= NORTH;
			if (y + 1 < HEIGHT && Maze[y + 1][x] != SPACE)
				stat |= SOUTH;
			if (x + 1 < WIDTH && Maze[y][x + 1] != SPACE)
				stat |= EAST;
			if (x - 1 >= 0 && Maze[y][x - 1] != SPACE)
				stat |= WEST;
			switch (stat) {
			  case WEST | EAST:
			  case EAST:
			  case WEST:
				*sp = WALL1;
				break;
			  case NORTH | SOUTH:
			  case NORTH:
			  case SOUTH:
				*sp = WALL2;
				break;
			  case 0:
#ifdef RANDOM
				*sp = DOOR;
#endif
#ifdef REFLECT
				*sp = rand_num(2) ? WALL4 : WALL5;
#endif
				break;
			  default:
				*sp = WALL3;
				break;
			}
		}
	memcpy(Orig_maze, Maze, sizeof Maze);
}

void
makemaze(void)
{
	char *sp;
	int y, x;

	/*
	 * fill maze with walls
	 */
	sp = &Maze[0][0];
	while (sp < &Maze[HEIGHT - 1][WIDTH])
		*sp++ = DOOR;

	x = rand_num(WIDTH / 2) * 2 + 1;
	y = rand_num(HEIGHT / 2) * 2 + 1;
	dig_maze(x, y);
	remap();
}
