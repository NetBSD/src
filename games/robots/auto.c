/*	$NetBSD: auto.c,v 1.10 2009/07/20 06:00:56 dholland Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *	Automatic move.
 *	intelligent ?
 *	Algo :
 *		IF scrapheaps don't exist THEN
 *			IF not in danger THEN
 *				stay at current position
 *		 	ELSE
 *				move away from the closest robot
 *			FI
 *		ELSE
 *			find closest heap
 *			find closest robot
 *			IF scrapheap is adjacent THEN
 *				move behind the scrapheap
 *			ELSE
 *				take the move that takes you away from the
 *				robots and closest to the heap
 *			FI
 *		FI
 */
#include "robots.h"

#define ABS(a) (((a)>0)?(a):-(a))
#define MIN(a,b) (((a)>(b))?(b):(a))
#define MAX(a,b) (((a)<(b))?(b):(a))

#define CONSDEBUG(a)

static int distance(int, int, int, int);
static int xinc(int);
static int yinc(int);
static const char *find_moves(void);
static COORD *closest_robot(int *);
static COORD *closest_heap(int *);
static char move_towards(int, int);
static char move_away(COORD *);
static char move_between(COORD *, COORD *);
static int between(COORD *, COORD *);

/* distance():
 * return "move" number distance of the two coordinates
 */
static int
distance(int x1, int y1, int x2, int y2)
{
	return MAX(ABS(ABS(x1) - ABS(x2)), ABS(ABS(y1) - ABS(y2)));
}

/* xinc():
 * Return x coordinate moves
 */
static int
xinc(int dir)
{
        switch(dir) {
        case 'b':
        case 'h':
        case 'y':
                return -1;
        case 'l':
        case 'n':
        case 'u':
                return 1;
        case 'j':
        case 'k':
        default:
                return 0;
        }
}

/* yinc():
 * Return y coordinate moves
 */
static int
yinc(int dir)
{
        switch(dir) {
        case 'k':
        case 'u':
        case 'y':
                return -1;
        case 'b':
        case 'j':
        case 'n':
                return 1;
        case 'h':
        case 'l':
        default:
                return 0;
        }
}

/* find_moves():
 * Find possible moves
 */
static const char *
find_moves(void)
{
        int x, y;
        COORD test;
        const char *m;
        char *a;
        static const char moves[] = ".hjklyubn";
        static char ans[sizeof moves];
        a = ans;

        for (m = moves; *m; m++) {
                test.x = My_pos.x + xinc(*m);
                test.y = My_pos.y + yinc(*m);
                move(test.y, test.x);
                switch(winch(stdscr)) {
                case ' ':
                case PLAYER:
                        for (x = test.x - 1; x <= test.x + 1; x++) {
                                for (y = test.y - 1; y <= test.y + 1; y++) {
                                        move(y, x);
                                        if (winch(stdscr) == ROBOT)
						goto bad;
                                }
                        }
                        *a++ = *m;
                }
        bad:;
        }
        *a = 0;
        if (ans[0])
                return ans;
        else
                return "t";
}

/* closest_robot():
 * return the robot closest to us
 * and put in dist its distance
 */
static COORD *
closest_robot(int *dist)
{
	COORD *rob, *end, *minrob = NULL;
	int tdist, mindist;

	mindist = 1000000;
	end = &Robots[MAXROBOTS];
	for (rob = Robots; rob < end; rob++) {
		tdist = distance(My_pos.x, My_pos.y, rob->x, rob->y);
		if (tdist < mindist) {
			minrob = rob;
			mindist = tdist;
		}
	}
	*dist = mindist;
	return minrob;
}

/* closest_heap():
 * return the heap closest to us
 * and put in dist its distance
 */
static COORD *
closest_heap(int *dist)
{
	COORD *hp, *end, *minhp = NULL;
	int mindist, tdist;

	mindist = 1000000;
	end = &Scrap[MAXROBOTS];
	for (hp = Scrap; hp < end; hp++) {
		if (hp->x == 0 && hp->y == 0)
			break;
		tdist = distance(My_pos.x, My_pos.y, hp->x, hp->y);
		if (tdist < mindist) {
			minhp = hp;
			mindist = tdist;
		}
	}
	*dist = mindist;
	return minhp;
}

/* move_towards():
 * move as close to the given direction as possible
 */
static char
move_towards(int dx, int dy)
{
	char ok_moves[10], best_move;
	char *ptr;
	int move_judge, cur_judge, mvx, mvy;

	(void)strcpy(ok_moves, find_moves());
	best_move = ok_moves[0];
	if (best_move != 't') {
		mvx = xinc(best_move);
		mvy = yinc(best_move);
		move_judge = ABS(mvx - dx) + ABS(mvy - dy);
		for (ptr = &ok_moves[1]; *ptr != '\0'; ptr++) {
			mvx = xinc(*ptr);
			mvy = yinc(*ptr);
			cur_judge = ABS(mvx - dx) + ABS(mvy - dy);
			if (cur_judge < move_judge) {
				move_judge = cur_judge;
				best_move = *ptr;
			}
		}
	}
	return best_move;
}

/* move_away():
 * move away form the robot given
 */
static char
move_away(COORD *rob)
{
	int dx, dy;

	dx = sign(My_pos.x - rob->x);
	dy = sign(My_pos.y - rob->y);
	return  move_towards(dx, dy);
}


/* move_between():
 * move the closest heap between us and the closest robot
 */
static char
move_between(COORD *rob, COORD *hp)
{
	int dx, dy;
	float slope, cons;

	/* equation of the line between us and the closest robot */
	if (My_pos.x == rob->x) {
		/*
		 * me and the robot are aligned in x
		 * change my x so I get closer to the heap
		 * and my y far from the robot
		 */
		dx = - sign(My_pos.x - hp->x);
		dy = sign(My_pos.y - rob->y);
		CONSDEBUG(("aligned in x"));
	}
	else if (My_pos.y == rob->y) {
		/*
		 * me and the robot are aligned in y
		 * change my y so I get closer to the heap
		 * and my x far from the robot
		 */
		dx = sign(My_pos.x - rob->x);
		dy = -sign(My_pos.y - hp->y);
		CONSDEBUG(("aligned in y"));
	}
	else {
		CONSDEBUG(("no aligned"));
		slope = (My_pos.y - rob->y) / (My_pos.x - rob->x);
		cons = slope * rob->y;
		if (ABS(My_pos.x - rob->x) > ABS(My_pos.y - rob->y)) {
			/*
			 * we are closest to the robot in x
			 * move away from the robot in x and
			 * close to the scrap in y
			 */
			dx = sign(My_pos.x - rob->x);
			dy = sign(((slope * ((float) hp->x)) + cons) -
				  ((float) hp->y));
		}
		else {
			dx = sign(((slope * ((float) hp->x)) + cons) -
				  ((float) hp->y));
			dy = sign(My_pos.y - rob->y);
		}
	}
	CONSDEBUG(("me (%d,%d) robot(%d,%d) heap(%d,%d) delta(%d,%d)",
		My_pos.x, My_pos.y, rob->x, rob->y, hp->x, hp->y, dx, dy));
	return move_towards(dx, dy);
}

/* between():
 * Return true if the heap is between us and the robot
 */
int
between(COORD *rob, COORD *hp)
{
	/* I = @ */
	if (hp->x > rob->x && My_pos.x < rob->x)
		return 0;
	/* @ = I */
	if (hp->x < rob->x && My_pos.x > rob->x)
		return 0;
	/* @ */
	/* = */
	/* I */
	if (hp->y < rob->y && My_pos.y > rob->y)
		return 0;
	/* I */
	/* = */
	/* @ */
	if (hp->y > rob->y && My_pos.y < rob->y)
		return 0;
	return 1;
}

/* automove():
 * find and do the best move if flag
 * else get the first move;
 */
char
automove(void)
{
#if 0
	return  find_moves()[0];
#else
	COORD *robot_close;
	COORD *heap_close;
	int robot_dist, robot_heap, heap_dist;

	robot_close = closest_robot(&robot_dist);
	if (robot_dist > 1)
		return('.');
	if (!Num_scrap)
		/* no scrap heaps just run away */
		return move_away(robot_close);

	heap_close = closest_heap(&heap_dist);
	robot_heap = distance(robot_close->x, robot_close->y,
	    heap_close->x, heap_close->y);
	if (robot_heap <= heap_dist && !between(robot_close, heap_close)) {
		/*
		 * robot is closest to us from the heap. Run away!
		 */
		return  move_away(robot_close);
	}

	return move_between(robot_close, heap_close);
#endif
}
