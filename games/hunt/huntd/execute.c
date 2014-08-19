/*	$NetBSD: execute.c,v 1.10.8.1 2014/08/20 00:00:23 tls Exp $	*/
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
__RCSID("$NetBSD: execute.c,v 1.10.8.1 2014/08/20 00:00:23 tls Exp $");
#endif /* not lint */

#include <stdlib.h>
#include "hunt.h"

static void cloak(PLAYER *);
static void turn_player(PLAYER *, int);
static void fire(PLAYER *, int);
static void fire_slime(PLAYER *, int);
static void move_player(PLAYER *, int);
static void pickup(PLAYER *, int, int, int, int);
static void scan(PLAYER *);


#ifdef MONITOR
/*
 * mon_execute:
 *	Execute a single monitor command
 */
void
mon_execute(PLAYER *pp)
{
	char ch;

	ch = pp->p_cbuf[pp->p_ncount++];
	switch (ch) {
	  case CTRL('L'):
		sendcom(pp, REDRAW);
		break;
	  case 'q':
		(void) strcpy(pp->p_death, "| Quit |");
		break;
	}
}
#endif

/*
 * execute:
 *	Execute a single command
 */
void
execute(PLAYER *pp)
{
	char ch;

	ch = pp->p_cbuf[pp->p_ncount++];

#ifdef FLY
	if (pp->p_flying >= 0) {
		switch (ch) {
		  case CTRL('L'):
			sendcom(pp, REDRAW);
			break;
		  case 'q':
			(void) strcpy(pp->p_death, "| Quit |");
			break;
		}
		return;
	}
#endif

	switch (ch) {
	  case CTRL('L'):
		sendcom(pp, REDRAW);
		break;
	  case 'h':
		move_player(pp, LEFTS);
		break;
	  case 'H':
		turn_player(pp, LEFTS);
		break;
	  case 'j':
		move_player(pp, BELOW);
		break;
	  case 'J':
		turn_player(pp, BELOW);
		break;
	  case 'k':
		move_player(pp, ABOVE);
		break;
	  case 'K':
		turn_player(pp, ABOVE);
		break;
	  case 'l':
		move_player(pp, RIGHT);
		break;
	  case 'L':
		turn_player(pp, RIGHT);
		break;
	  case 'f':
	  case '1':
		fire(pp, 0);		/* SHOT */
		break;
	  case 'g':
	  case '2':
		fire(pp, 1);		/* GRENADE */
		break;
	  case 'F':
	  case '3':
		fire(pp, 2);		/* SATCHEL */
		break;
	  case 'G':
	  case '4':
		fire(pp, 3);		/* 7x7 BOMB */
		break;
	  case '5':
		fire(pp, 4);		/* 9x9 BOMB */
		break;
	  case '6':
		fire(pp, 5);		/* 11x11 BOMB */
		break;
	  case '7':
		fire(pp, 6);		/* 13x13 BOMB */
		break;
	  case '8':
		fire(pp, 7);		/* 15x15 BOMB */
		break;
	  case '9':
		fire(pp, 8);		/* 17x17 BOMB */
		break;
	  case '0':
		fire(pp, 9);		/* 19x19 BOMB */
		break;
	  case '@':
		fire(pp, 10);		/* 21x21 BOMB */
		break;
#ifdef OOZE
	  case 'o':
		fire_slime(pp, 0);	/* SLIME */
		break;
	  case 'O':
		fire_slime(pp, 1);	/* SSLIME */
		break;
	  case 'p':
		fire_slime(pp, 2);
		break;
	  case 'P':
		fire_slime(pp, 3);
		break;
#endif
	  case 's':
		scan(pp);
		break;
	  case 'c':
		cloak(pp);
		break;
	  case 'q':
		(void) strcpy(pp->p_death, "| Quit |");
		break;
	}
}

/*
 * move_player:
 *	Execute a move in the given direction
 */
static void
move_player(PLAYER *pp, int dir)
{
	PLAYER *newp;
	int x, y;
	bool moved;
	BULLET *bp;

	y = pp->p_y;
	x = pp->p_x;

	switch (dir) {
	  case LEFTS:
		x--;
		break;
	  case RIGHT:
		x++;
		break;
	  case ABOVE:
		y--;
		break;
	  case BELOW:
		y++;
		break;
	}

	moved = false;
	switch (Maze[y][x]) {
	  case SPACE:
#ifdef RANDOM
	  case DOOR:
#endif
		moved = true;
		break;
	  case WALL1:
	  case WALL2:
	  case WALL3:
#ifdef REFLECT
	  case WALL4:
	  case WALL5:
#endif
		break;
	  case MINE:
	  case GMINE:
		if (dir == pp->p_face)
			pickup(pp, y, x, 2, Maze[y][x]);
		else if (opposite(dir, pp->p_face))
			pickup(pp, y, x, 95, Maze[y][x]);
		else
			pickup(pp, y, x, 50, Maze[y][x]);
		Maze[y][x] = SPACE;
		moved = true;
		break;
	  case SHOT:
	  case GRENADE:
	  case SATCHEL:
	  case BOMB:
#ifdef OOZE
	  case SLIME:
#endif
#ifdef DRONE
	  case DSHOT:
#endif
		bp = is_bullet(y, x);
		if (bp != NULL)
			bp->b_expl = true;
		Maze[y][x] = SPACE;
		moved = true;
		break;
	  case LEFTS:
	  case RIGHT:
	  case ABOVE:
	  case BELOW:
		if (dir != pp->p_face)
			sendcom(pp, BELL);
		else {
			newp = play_at(y, x);
			checkdam(newp, pp, pp->p_ident, STABDAM, KNIFE);
		}
		break;
#ifdef FLY
	  case FLYER:
		newp = play_at(y, x);
		message(newp, "Oooh, there's a short guy waving at you!");
		message(pp, "You couldn't quite reach him!");
		break;
#endif
#ifdef BOOTS
	  case BOOT:
	  case BOOT_PAIR:
		if (Maze[y][x] == BOOT)
			pp->p_nboots++;
		else
			pp->p_nboots += 2;
		for (newp = Boot; newp < &Boot[NBOOTS]; newp++) {
			if (newp->p_flying < 0)
				continue;
			if (newp->p_y == y && newp->p_x == x) {
				newp->p_flying = -1;
				if (newp->p_undershot)
					fixshots(y, x, newp->p_over);
			}
		}
		if (pp->p_nboots == 2)
			message(pp, "Wow!  A pair of boots!");
		else
			message(pp, "You can hobble around on one boot.");
		Maze[y][x] = SPACE;
		moved = true;
		break;
#endif
	}
	if (moved) {
		if (pp->p_ncshot > 0)
			if (--pp->p_ncshot == MAXNCSHOT) {
				cgoto(pp, STAT_GUN_ROW, STAT_VALUE_COL);
				outstr(pp, " ok", 3);
			}
		if (pp->p_undershot) {
			fixshots(pp->p_y, pp->p_x, pp->p_over);
			pp->p_undershot = false;
		}
		drawplayer(pp, false);
		pp->p_over = Maze[y][x];
		pp->p_y = y;
		pp->p_x = x;
		drawplayer(pp, true);
	}
}

/*
 * turn_player:
 *	Change the direction the player is facing
 */
static void
turn_player(PLAYER *pp, int dir)
{
	if (pp->p_face != dir) {
		pp->p_face = dir;
		drawplayer(pp, true);
	}
}

/*
 * fire:
 *	Fire a shot of the given type in the given direction
 */
static void
fire(PLAYER *pp, int req_index)
{
	if (pp == NULL)
		return;
#ifdef DEBUG
	if (req_index < 0 || req_index >= MAXBOMB)
		message(pp, "What you do?");
#endif
	while (req_index >= 0 && pp->p_ammo < shot_req[req_index])
		req_index--;
	if (req_index < 0) {
		message(pp, "Not enough charges.");
		return;
	}
	if (pp->p_ncshot > MAXNCSHOT)
		return;
	if (pp->p_ncshot++ == MAXNCSHOT) {
		cgoto(pp, STAT_GUN_ROW, STAT_VALUE_COL);
		outstr(pp, "   ", 3);
	}
	pp->p_ammo -= shot_req[req_index];
	(void) snprintf(Buf, sizeof(Buf), "%3d", pp->p_ammo);
	cgoto(pp, STAT_AMMO_ROW, STAT_VALUE_COL);
	outstr(pp, Buf, 3);

	add_shot(shot_type[req_index], pp->p_y, pp->p_x, pp->p_face,
		shot_req[req_index], pp, false, pp->p_face);
	pp->p_undershot = true;

	/*
	 * Show the object to everyone
	 */
	showexpl(pp->p_y, pp->p_x, shot_type[req_index]);
	for (pp = Player; pp < End_player; pp++)
		sendcom(pp, REFRESH);
#ifdef MONITOR
	for (pp = Monitor; pp < End_monitor; pp++)
		sendcom(pp, REFRESH);
#endif
}

#ifdef OOZE
/*
 * fire_slime:
 *	Fire a slime shot in the given direction
 */
static void
fire_slime(PLAYER *pp, int req_index)
{
	if (pp == NULL)
		return;
#ifdef DEBUG
	if (req_index < 0 || req_index >= MAXSLIME)
		message(pp, "What you do?");
#endif
	while (req_index >= 0 && pp->p_ammo < slime_req[req_index])
		req_index--;
	if (req_index < 0) {
		message(pp, "Not enough charges.");
		return;
	}
	if (pp->p_ncshot > MAXNCSHOT)
		return;
	if (pp->p_ncshot++ == MAXNCSHOT) {
		cgoto(pp, STAT_GUN_ROW, STAT_VALUE_COL);
		outstr(pp, "   ", 3);
	}
	pp->p_ammo -= slime_req[req_index];
	(void) snprintf(Buf, sizeof(Buf), "%3d", pp->p_ammo);
	cgoto(pp, STAT_AMMO_ROW, STAT_VALUE_COL);
	outstr(pp, Buf, 3);

	add_shot(SLIME, pp->p_y, pp->p_x, pp->p_face,
		slime_req[req_index] * SLIME_FACTOR, pp, false, pp->p_face);
	pp->p_undershot = true;

	/*
	 * Show the object to everyone
	 */
	showexpl(pp->p_y, pp->p_x, SLIME);
	for (pp = Player; pp < End_player; pp++)
		sendcom(pp, REFRESH);
#ifdef MONITOR
	for (pp = Monitor; pp < End_monitor; pp++)
		sendcom(pp, REFRESH);
#endif
}
#endif

/*
 * add_shot:
 *	Create a shot with the given properties
 */
void
add_shot(int type, int y, int x, char face, int charge,
	 PLAYER *owner, int expl, char over)
{
	BULLET *bp;
	int size;

	switch (type) {
	  case SHOT:
	  case MINE:
		size = 1;
		break;
	  case GRENADE:
	  case GMINE:
		size = 2;
		break;
	  case SATCHEL:
		size = 3;
		break;
	  case BOMB:
		for (size = 3; size < MAXBOMB; size++)
			if (shot_req[size] >= charge)
				break;
		size++;
		break;
	  default:
		size = 0;
		break;
	}

	bp = create_shot(type, y, x, face, charge, size, owner,
		(owner == NULL) ? NULL : owner->p_ident, expl, over);
	bp->b_next = Bullets;
	Bullets = bp;
}

BULLET *
create_shot(int type, int y, int x, char face, int charge,
	    int size, PLAYER *owner, IDENT *score, int expl, char over)
{
	BULLET *bp;

	bp = malloc(sizeof(*bp));
	if (bp == NULL) {
		if (owner != NULL)
			message(owner, "Out of memory");
		return NULL;
	}

	bp->b_face = face;
	bp->b_x = x;
	bp->b_y = y;
	bp->b_charge = charge;
	bp->b_owner = owner;
	bp->b_score = score;
	bp->b_type = type;
	bp->b_size = size;
	bp->b_expl = expl;
	bp->b_over = over;
	bp->b_next = NULL;

	return bp;
}

/*
 * cloak:
 *	Turn on or increase length of a cloak
 */
static void
cloak(PLAYER *pp)
{
	if (pp->p_ammo <= 0) {
		message(pp, "No more charges");
		return;
	}
#ifdef BOOTS
	if (pp->p_nboots > 0) {
		message(pp, "Boots are too noisy to cloak!");
		return;
	}
#endif
	(void) snprintf(Buf, sizeof(Buf), "%3d", --pp->p_ammo);
	cgoto(pp, STAT_AMMO_ROW, STAT_VALUE_COL);
	outstr(pp, Buf, 3);

	pp->p_cloak += CLOAKLEN;

	if (pp->p_scan >= 0)
		pp->p_scan = -1;

	showstat(pp);
}

/*
 * scan:
 *	Turn on or increase length of a scan
 */
static void
scan(PLAYER *pp)
{
	if (pp->p_ammo <= 0) {
		message(pp, "No more charges");
		return;
	}
	(void) snprintf(Buf, sizeof(Buf), "%3d", --pp->p_ammo);
	cgoto(pp, STAT_AMMO_ROW, STAT_VALUE_COL);
	outstr(pp, Buf, 3);

	pp->p_scan += SCANLEN;

	if (pp->p_cloak >= 0)
		pp->p_cloak = -1;

	showstat(pp);
}

/*
 * pickup:
 *	check whether the object blew up or whether he picked it up
 */
static void
pickup(PLAYER *pp, int y, int x, int prob, int obj)
{
	int req;

	switch (obj) {
	  case MINE:
		req = BULREQ;
		break;
	  case GMINE:
		req = GRENREQ;
		break;
	  default:
		abort();
	}
	if (rand_num(100) < prob)
		add_shot(obj, y, x, LEFTS, req, NULL, true, pp->p_face);
	else {
		pp->p_ammo += req;
		(void) snprintf(Buf, sizeof(Buf), "%3d", pp->p_ammo);
		cgoto(pp, STAT_AMMO_ROW, STAT_VALUE_COL);
		outstr(pp, Buf, 3);
	}
}
