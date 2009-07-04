/*	$NetBSD: shots.c,v 1.8 2009/07/04 01:01:18 dholland Exp $	*/
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
__RCSID("$NetBSD: shots.c,v 1.8 2009/07/04 01:01:18 dholland Exp $");
#endif /* not lint */

# include	<err.h>
# include	<signal.h>
# include	<stdlib.h>
# include	"hunt.h"

# define	PLUS_DELTA(x, max)	if (x < max) x++; else x--
# define	MINUS_DELTA(x, min)	if (x > min) x--; else x++

static	void	chkshot(BULLET *, BULLET *);
static	void	chkslime(BULLET *, BULLET *);
static	void	explshot(BULLET *, int, int);
static	void	find_under(BULLET *, BULLET *);
static	int	iswall(int, int);
static	void	mark_boot(BULLET *);
static	void	mark_player(BULLET *);
#ifdef DRONE
static	void	move_drone(BULLET *);
#endif
static	void	move_flyer(PLAYER *);
static	int	move_normal_shot(BULLET *);
static	void	move_slime(BULLET *, int, BULLET *);
static	void	save_bullet(BULLET *);
static	void	zapshot(BULLET *, BULLET *);

/*
 * moveshots:
 *	Move the shots already in the air, taking explosions into account
 */
void
moveshots()
{
	BULLET	*bp, *next;
	PLAYER	*pp;
	int	x, y;
	BULLET	*blist;

	rollexpl();
	if (Bullets == NULL)
		goto ret;

	/*
	 * First we move through the bullet list BULSPD times, looking
	 * for things we may have run into.  If we do run into
	 * something, we set up the explosion and disappear, checking
	 * for damage to any player who got in the way.
	 */

	blist = Bullets;
	Bullets = NULL;
	for (bp = blist; bp != NULL; bp = next) {
		next = bp->b_next;
		x = bp->b_x;
		y = bp->b_y;
		Maze[y][x] = bp->b_over;
		for (pp = Player; pp < End_player; pp++)
			check(pp, y, x);
# ifdef MONITOR
		for (pp = Monitor; pp < End_monitor; pp++)
			check(pp, y, x);
# endif

		switch (bp->b_type) {
		  case SHOT:
		  case GRENADE:
		  case SATCHEL:
		  case BOMB:
			if (move_normal_shot(bp)) {
				bp->b_next = Bullets;
				Bullets = bp;
			}
			break;
# ifdef OOZE
		  case SLIME:
			if (bp->b_expl || move_normal_shot(bp)) {
				bp->b_next = Bullets;
				Bullets = bp;
			}
			break;
# endif
# ifdef DRONE
		  case DSHOT:
			if (move_drone(bp)) {
				bp->b_next = Bullets;
				Bullets = bp;
			}
			break;
# endif
		  default:
			bp->b_next = Bullets;
			Bullets = bp;
			break;
		}
	}

	blist = Bullets;
	Bullets = NULL;
	for (bp = blist; bp != NULL; bp = next) {
		next = bp->b_next;
		if (!bp->b_expl) {
			save_bullet(bp);
# ifdef MONITOR
			for (pp = Monitor; pp < End_monitor; pp++)
				check(pp, bp->b_y, bp->b_x);
# endif
# ifdef DRONE
			if (bp->b_type == DSHOT)
				for (pp = Player; pp < End_player; pp++)
					if (pp->p_scan >= 0)
						check(pp, bp->b_y, bp->b_x);
# endif
			continue;
		}

		chkshot(bp, next);
		free(bp);
	}

	for (pp = Player; pp < End_player; pp++)
		Maze[pp->p_y][pp->p_x] = pp->p_face;

ret:
# ifdef BOOTS
	for (pp = Boot; pp < &Boot[NBOOTS]; pp++)
		if (pp->p_flying >= 0)
			move_flyer(pp);
# endif
	for (pp = Player; pp < End_player; pp++) {
# ifdef FLY
		if (pp->p_flying >= 0)
			move_flyer(pp);
# endif
		sendcom(pp, REFRESH);	/* Flush out the explosions */
		look(pp);
		sendcom(pp, REFRESH);
	}
# ifdef MONITOR
	for (pp = Monitor; pp < End_monitor; pp++)
		sendcom(pp, REFRESH);
# endif

	return;
}

/*
 * move_normal_shot:
 *	Move a normal shot along its trajectory
 */
static int
move_normal_shot(bp)
	BULLET	*bp;
{
	int	i, x, y;
	PLAYER	*pp;

	for (i = 0; i < BULSPD; i++) {
		if (bp->b_expl)
			break;

		x = bp->b_x;
		y = bp->b_y;

		switch (bp->b_face) {
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

		switch (Maze[y][x]) {
		  case SHOT:
			if (rand_num(100) < 5) {
				zapshot(Bullets, bp);
				zapshot(bp->b_next, bp);
			}
			break;
		  case GRENADE:
			if (rand_num(100) < 10) {
				zapshot(Bullets, bp);
				zapshot(bp->b_next, bp);
			}
			break;
# ifdef	REFLECT
		  case WALL4:	/* reflecting walls */
			switch (bp->b_face) {
			  case LEFTS:
				bp->b_face = BELOW;
				break;
			  case RIGHT:
				bp->b_face = ABOVE;
				break;
			  case ABOVE:
				bp->b_face = RIGHT;
				break;
			  case BELOW:
				bp->b_face = LEFTS;
				break;
			}
			Maze[y][x] = WALL5;
# ifdef MONITOR
			for (pp = Monitor; pp < End_monitor; pp++)
				check(pp, y, x);
# endif
			break;
		  case WALL5:
			switch (bp->b_face) {
			  case LEFTS:
				bp->b_face = ABOVE;
				break;
			  case RIGHT:
				bp->b_face = BELOW;
				break;
			  case ABOVE:
				bp->b_face = LEFTS;
				break;
			  case BELOW:
				bp->b_face = RIGHT;
				break;
			}
			Maze[y][x] = WALL4;
# ifdef MONITOR
			for (pp = Monitor; pp < End_monitor; pp++)
				check(pp, y, x);
# endif
			break;
# endif
# ifdef RANDOM
		  case DOOR:
			switch (rand_num(4)) {
			  case 0:
				bp->b_face = ABOVE;
				break;
			  case 1:
				bp->b_face = BELOW;
				break;
			  case 2:
				bp->b_face = LEFTS;
				break;
			  case 3:
				bp->b_face = RIGHT;
				break;
			}
			break;
# endif
# ifdef FLY
		  case FLYER:
			pp = play_at(y, x);
			message(pp, "Zing!");
			break;
# endif
		  case LEFTS:
		  case RIGHT:
		  case BELOW:
		  case ABOVE:
			/*
			 * give the person a chance to catch a
			 * grenade if s/he is facing it
			 */
			pp = play_at(y, x);
			pp->p_ident->i_shot += bp->b_charge;
			if (opposite(bp->b_face, Maze[y][x])) {
			    if (rand_num(100) < 10) {
				if (bp->b_owner != NULL)
					message(bp->b_owner,
					    "Your charge was absorbed!");
				if (bp->b_score != NULL)
					bp->b_score->i_robbed += bp->b_charge;
				pp->p_ammo += bp->b_charge;
				if (pp->p_damage + bp->b_size * MINDAM
				    > pp->p_damcap)
					pp->p_ident->i_saved++;
				message(pp, "Absorbed charge (good shield!)");
				pp->p_ident->i_absorbed += bp->b_charge;
				free(bp);
				(void) snprintf(Buf, sizeof(Buf),
						"%3d", pp->p_ammo);
				cgoto(pp, STAT_AMMO_ROW, STAT_VALUE_COL);
				outstr(pp, Buf, 3);
				return FALSE;
			    }
			    pp->p_ident->i_faced += bp->b_charge;
			}
			/*
			 * Small chance that the bullet just misses the
			 * person.  If so, the bullet just goes on its
			 * merry way without exploding.
			 */
			if (rand_num(100) < 5) {
				pp->p_ident->i_ducked += bp->b_charge;
				if (pp->p_damage + bp->b_size * MINDAM
				    > pp->p_damcap)
					pp->p_ident->i_saved++;
				if (bp->b_score != NULL)
					bp->b_score->i_missed += bp->b_charge;
				message(pp, "Zing!");
				if (bp->b_owner == NULL)
					break;
				message(bp->b_owner, bp->b_score &&
					((bp->b_score->i_missed & 0x7) == 0x7) ?
					"My!  What a bad shot you are!" :
					"Missed him");
				break;
			}
			/*
			 * The shot hit that sucker!  Blow it up.
			 */
			/* FALLTHROUGH */
# ifndef RANDOM
		  case DOOR:
# endif
		  case WALL1:
		  case WALL2:
		  case WALL3:
			bp->b_expl = TRUE;
			break;
		}

		bp->b_x = x;
		bp->b_y = y;
	}
	return TRUE;
}

# ifdef	DRONE
/*
 * move_drone:
 *	Move the drone to the next square
 */
static void
move_drone(bp)
	BULLET	*bp;
{
	int	mask, count;
	int	n, dir;
	PLAYER	*pp;

	/*
	 * See if we can give someone a blast
	 */
	if (isplayer(Maze[bp->b_y][bp->b_x - 1])) {
		dir = WEST;
		goto drone_move;
	}
	if (isplayer(Maze[bp->b_y - 1][bp->b_x])) {
		dir = NORTH;
		goto drone_move;
	}
	if (isplayer(Maze[bp->b_y + 1][bp->b_x])) {
		dir = SOUTH;
		goto drone_move;
	}
	if (isplayer(Maze[bp->b_y][bp->b_x + 1])) {
		dir = EAST;
		goto drone_move;
	}

	/*
	 * Find out what directions are clear
	 */
	mask = count = 0;
	if (!iswall(bp->b_y, bp->b_x - 1))
		mask |= WEST, count++;
	if (!iswall(bp->b_y - 1, bp->b_x))
		mask |= NORTH, count++;
	if (!iswall(bp->b_y + 1, bp->b_x))
		mask |= SOUTH, count++;
	if (!iswall(bp->b_y, bp->b_x + 1))
		mask |= EAST, count++;

	/*
	 * All blocked up, just you wait
	 */
	if (count == 0)
		return TRUE;

	/*
	 * Only one way to go.
	 */
	if (count == 1) {
		dir = mask;
		goto drone_move;
	}

	/*
	 * Get rid of the direction that we came from
	 */
	switch (bp->b_face) {
	  case LEFTS:
		if (mask & EAST)
			mask &= ~EAST, count--;
		break;
	  case RIGHT:
		if (mask & WEST)
			mask &= ~WEST, count--;
		break;
	  case ABOVE:
		if (mask & SOUTH)
			mask &= ~SOUTH, count--;
		break;
	  case BELOW:
		if (mask & NORTH)
			mask &= ~NORTH, count--;
		break;
	}

	/*
	 * Pick one of the remaining directions
	 */
	n = rand_num(count);
	if (n >= 0 && mask & NORTH)
		dir = NORTH, n--;
	if (n >= 0 && mask & SOUTH)
		dir = SOUTH, n--;
	if (n >= 0 && mask & EAST)
		dir = EAST, n--;
	if (n >= 0 && mask & WEST)
		dir = WEST, n--;

	/*
	 * Now that we know the direction of movement,
	 * just update the position of the drone
	 */
drone_move:
	switch (dir) {
	  case WEST:
		bp->b_x--;
		bp->b_face = LEFTS;
		break;
	  case EAST:
		bp->b_x++;
		bp->b_face = RIGHT;
		break;
	  case NORTH:
		bp->b_y--;
		bp->b_face = ABOVE;
		break;
	  case SOUTH:
		bp->b_y++;
		bp->b_face = BELOW;
		break;
	}
	switch (Maze[bp->b_y][bp->b_x]) {
	  case LEFTS:
	  case RIGHT:
	  case BELOW:
	  case ABOVE:
		/*
		 * give the person a chance to catch a
		 * drone if s/he is facing it
		 */
		if (rand_num(100) < 1 &&
		opposite(bp->b_face, Maze[bp->b_y][bp->b_x])) {
			pp = play_at(bp->b_y, bp->b_x);
			pp->p_ammo += bp->b_charge;
			message(pp, "**** Absorbed drone ****");
			free(bp);
			(void) snprintf(Buf, sizeof(buf), "%3d", pp->p_ammo);
			cgoto(pp, STAT_AMMO_ROW, STAT_VALUE_COL);
			outstr(pp, Buf, 3);
			return FALSE;
		}
		bp->b_expl = TRUE;
		break;
	}
	return TRUE;
}
# endif

/*
 * save_bullet:
 *	Put this bullet back onto the bullet list
 */
static void
save_bullet(bp)
	BULLET	*bp;
{
	bp->b_over = Maze[bp->b_y][bp->b_x];
	switch (bp->b_over) {
	  case SHOT:
	  case GRENADE:
	  case SATCHEL:
	  case BOMB:
# ifdef OOZE
	  case SLIME:
# ifdef VOLCANO
	  case LAVA:
# endif
# endif
# ifdef DRONE
	  case DSHOT:
# endif
		find_under(Bullets, bp);
		break;
	}

	switch (bp->b_over) {
	  case LEFTS:
	  case RIGHT:
	  case ABOVE:
	  case BELOW:
# ifdef FLY
	  case FLYER:
# endif
		mark_player(bp);
		break;
# ifdef BOOTS
	  case BOOT:
	  case BOOT_PAIR:
		mark_boot(bp);
# endif
		
	  default:
		Maze[bp->b_y][bp->b_x] = bp->b_type;
		break;
	}

	bp->b_next = Bullets;
	Bullets = bp;
}

/*
 * move_flyer:
 *	Update the position of a player in flight
 */
static void
move_flyer(pp)
	PLAYER	*pp;
{
	int	x, y;

	if (pp->p_undershot) {
		fixshots(pp->p_y, pp->p_x, pp->p_over);
		pp->p_undershot = FALSE;
	}
	Maze[pp->p_y][pp->p_x] = pp->p_over;
	x = pp->p_x + pp->p_flyx;
	y = pp->p_y + pp->p_flyy;
	if (x < 1) {
		x = 1 - x;
		pp->p_flyx = -pp->p_flyx;
	}
	else if (x > WIDTH - 2) {
		x = (WIDTH - 2) - (x - (WIDTH - 2));
		pp->p_flyx = -pp->p_flyx;
	}
	if (y < 1) {
		y = 1 - y;
		pp->p_flyy = -pp->p_flyy;
	}
	else if (y > HEIGHT - 2) {
		y = (HEIGHT - 2) - (y - (HEIGHT - 2));
		pp->p_flyy = -pp->p_flyy;
	}
again:
	switch (Maze[y][x]) {
	  default:
		switch (rand_num(4)) {
		  case 0:
			PLUS_DELTA(x, WIDTH - 2);
			break;
		  case 1:
			MINUS_DELTA(x, 1);
			break;
		  case 2:
			PLUS_DELTA(y, HEIGHT - 2);
			break;
		  case 3:
			MINUS_DELTA(y, 1);
			break;
		}
		goto again;
	  case WALL1:
	  case WALL2:
	  case WALL3:
# ifdef	REFLECT
	  case WALL4:
	  case WALL5:
# endif
# ifdef	RANDOM
	  case DOOR:
# endif
		if (pp->p_flying == 0)
			pp->p_flying++;
		break;
	  case SPACE:
		break;
	}
	pp->p_y = y;
	pp->p_x = x;
	if (pp->p_flying-- == 0) {
# ifdef BOOTS
		if (pp->p_face != BOOT && pp->p_face != BOOT_PAIR) {
# endif
			checkdam(pp, (PLAYER *) NULL, (IDENT *) NULL,
				rand_num(pp->p_damage / 5), FALL);
			pp->p_face = rand_dir();
			showstat(pp);
# ifdef BOOTS
		}
		else {
			if (Maze[y][x] == BOOT)
				pp->p_face = BOOT_PAIR;
			Maze[y][x] = SPACE;
		}
# endif
	}
	pp->p_over = Maze[y][x];
	Maze[y][x] = pp->p_face;
	showexpl(y, x, pp->p_face);
}

/*
 * chkshot
 *	Handle explosions
 */
static void
chkshot(bp, next)
	BULLET	*bp;
	BULLET	*next;
{
	int	y, x;
	int	dy, dx, absdy;
	int	delta, damage;
	char	expl;
	PLAYER	*pp;

	delta = 0;
	switch (bp->b_type) {
	  case SHOT:
	  case MINE:
	  case GRENADE:
	  case GMINE:
	  case SATCHEL:
	  case BOMB:
		delta = bp->b_size - 1;
		break;
# ifdef	OOZE
	  case SLIME:
# ifdef VOLCANO
	  case LAVA:
# endif
		chkslime(bp, next);
		return;
# endif
# ifdef DRONE
	  case DSHOT:
		bp->b_type = SLIME;
		chkslime(bp, next);
		return;
# endif
	}
	for (y = bp->b_y - delta; y <= bp->b_y + delta; y++) {
		if (y < 0 || y >= HEIGHT)
			continue;
		dy = y - bp->b_y;
		absdy = (dy < 0) ? -dy : dy;
		for (x = bp->b_x - delta; x <= bp->b_x + delta; x++) {
			if (x < 0 || x >= WIDTH)
				continue;
			dx = x - bp->b_x;
			if (dx == 0)
				expl = (dy == 0) ? '*' : '|';
			else if (dy == 0)
				expl = '-';
			else if (dx == dy)
				expl = '\\';
			else if (dx == -dy)
				expl = '/';
			else
				expl = '*';
			showexpl(y, x, expl);
			switch (Maze[y][x]) {
			  case LEFTS:
			  case RIGHT:
			  case ABOVE:
			  case BELOW:
# ifdef FLY
			  case FLYER:
# endif
				if (dx < 0)
					dx = -dx;
				if (absdy > dx)
					damage = bp->b_size - absdy;
				else
					damage = bp->b_size - dx;
				pp = play_at(y, x);
				checkdam(pp, bp->b_owner, bp->b_score,
					damage * MINDAM, bp->b_type);
				break;
			  case GMINE:
			  case MINE:
				add_shot((Maze[y][x] == GMINE) ?
					GRENADE : SHOT,
					y, x, LEFTS,
					(Maze[y][x] == GMINE) ?
					GRENREQ : BULREQ,
					(PLAYER *) NULL, TRUE, SPACE);
				Maze[y][x] = SPACE;
				break;
			}
		}
	}
}

# ifdef	OOZE
/*
 * chkslime:
 *	handle slime shot exploding
 */
static void
chkslime(bp, next)
	BULLET	*bp;
	BULLET	*next;
{
	BULLET	*nbp;

	switch (Maze[bp->b_y][bp->b_x]) {
	  case WALL1:
	  case WALL2:
	  case WALL3:
# ifdef	REFLECT
	  case WALL4:
	  case WALL5:
# endif
# ifdef	RANDOM
	  case DOOR:
# endif
		switch (bp->b_face) {
		  case LEFTS:
			bp->b_x++;
			break;
		  case RIGHT:
			bp->b_x--;
			break;
		  case ABOVE:
			bp->b_y++;
			break;
		  case BELOW:
			bp->b_y--;
			break;
		}
		break;
	}
	nbp = malloc(sizeof(*nbp));
	*nbp = *bp;
# ifdef VOLCANO
	move_slime(nbp, nbp->b_type == SLIME ? SLIMESPEED : LAVASPEED, next);
# else
	move_slime(nbp, SLIMESPEED, next);
# endif
}

/*
 * move_slime:
 *	move the given slime shot speed times and add it back if
 *	it hasn't fizzled yet
 */
void
move_slime(bp, speed, next)
	BULLET	*bp;
	int	speed;
	BULLET	*next;
{
	int	i, j, dirmask, count;
	PLAYER	*pp;
	BULLET	*nbp;

	if (speed == 0) {
		if (bp->b_charge <= 0)
			free(bp);
		else
			save_bullet(bp);
		return;
	}

# ifdef VOLCANO
	showexpl(bp->b_y, bp->b_x, bp->b_type == LAVA ? LAVA : '*');
# else
	showexpl(bp->b_y, bp->b_x, '*');
# endif
	switch (Maze[bp->b_y][bp->b_x]) {
	  case LEFTS:
	  case RIGHT:
	  case ABOVE:
	  case BELOW:
# ifdef FLY
	  case FLYER:
# endif
		pp = play_at(bp->b_y, bp->b_x);
		message(pp, "You've been slimed.");
		checkdam(pp, bp->b_owner, bp->b_score, MINDAM, bp->b_type);
		break;
	  case SHOT:
	  case GRENADE:
	  case SATCHEL:
	  case BOMB:
# ifdef DRONE
	  case DSHOT:
# endif
		explshot(next, bp->b_y, bp->b_x);
		explshot(Bullets, bp->b_y, bp->b_x);
		break;
	}

	if (--bp->b_charge <= 0) {
		free(bp);
		return;
	}

	dirmask = 0;
	count = 0;
	switch (bp->b_face) {
	  case LEFTS:
		if (!iswall(bp->b_y, bp->b_x - 1))
			dirmask |= WEST, count++;
		if (!iswall(bp->b_y - 1, bp->b_x))
			dirmask |= NORTH, count++;
		if (!iswall(bp->b_y + 1, bp->b_x))
			dirmask |= SOUTH, count++;
		if (dirmask == 0)
			if (!iswall(bp->b_y, bp->b_x + 1))
				dirmask |= EAST, count++;
		break;
	  case RIGHT:
		if (!iswall(bp->b_y, bp->b_x + 1))
			dirmask |= EAST, count++;
		if (!iswall(bp->b_y - 1, bp->b_x))
			dirmask |= NORTH, count++;
		if (!iswall(bp->b_y + 1, bp->b_x))
			dirmask |= SOUTH, count++;
		if (dirmask == 0)
			if (!iswall(bp->b_y, bp->b_x - 1))
				dirmask |= WEST, count++;
		break;
	  case ABOVE:
		if (!iswall(bp->b_y - 1, bp->b_x))
			dirmask |= NORTH, count++;
		if (!iswall(bp->b_y, bp->b_x - 1))
			dirmask |= WEST, count++;
		if (!iswall(bp->b_y, bp->b_x + 1))
			dirmask |= EAST, count++;
		if (dirmask == 0)
			if (!iswall(bp->b_y + 1, bp->b_x))
				dirmask |= SOUTH, count++;
		break;
	  case BELOW:
		if (!iswall(bp->b_y + 1, bp->b_x))
			dirmask |= SOUTH, count++;
		if (!iswall(bp->b_y, bp->b_x - 1))
			dirmask |= WEST, count++;
		if (!iswall(bp->b_y, bp->b_x + 1))
			dirmask |= EAST, count++;
		if (dirmask == 0)
			if (!iswall(bp->b_y - 1, bp->b_x))
				dirmask |= NORTH, count++;
		break;
	}
	if (count == 0) {
		/*
		 * No place to go.  Just sit here for a while and wait
		 * for adjacent squares to clear out.
		 */
		save_bullet(bp);
		return;
	}
	if (bp->b_charge < count) {
		/* Only bp->b_charge paths may be taken */
		while (count > bp->b_charge) {
			if (dirmask & WEST)
				dirmask &= ~WEST;
			else if (dirmask & EAST)
				dirmask &= ~EAST;
			else if (dirmask & NORTH)
				dirmask &= ~NORTH;
			else if (dirmask & SOUTH)
				dirmask &= ~SOUTH;
			count--;
		}
	}

	i = bp->b_charge / count;
	j = bp->b_charge % count;
	if (dirmask & WEST) {
		count--;
		nbp = create_shot(bp->b_type, bp->b_y, bp->b_x - 1, LEFTS,
			i, bp->b_size, bp->b_owner, bp->b_score, TRUE, SPACE);
		move_slime(nbp, speed - 1, next);
	}
	if (dirmask & EAST) {
		count--;
		nbp = create_shot(bp->b_type, bp->b_y, bp->b_x + 1, RIGHT,
			(count < j) ? i + 1 : i, bp->b_size, bp->b_owner,
			bp->b_score, TRUE, SPACE);
		move_slime(nbp, speed - 1, next);
	}
	if (dirmask & NORTH) {
		count--;
		nbp = create_shot(bp->b_type, bp->b_y - 1, bp->b_x, ABOVE,
			(count < j) ? i + 1 : i, bp->b_size, bp->b_owner,
			bp->b_score, TRUE, SPACE);
		move_slime(nbp, speed - 1, next);
	}
	if (dirmask & SOUTH) {
		count--;
		nbp = create_shot(bp->b_type, bp->b_y + 1, bp->b_x, BELOW,
			(count < j) ? i + 1 : i, bp->b_size, bp->b_owner,
			bp->b_score, TRUE, SPACE);
		move_slime(nbp, speed - 1, next);
	}

	free(bp);
}

/*
 * iswall:
 *	returns whether the given location is a wall
 */
static int
iswall(y, x)
	int	y, x;
{
	if (y < 0 || x < 0 || y >= HEIGHT || x >= WIDTH)
		return TRUE;
	switch (Maze[y][x]) {
	  case WALL1:
	  case WALL2:
	  case WALL3:
# ifdef	REFLECT
	  case WALL4:
	  case WALL5:
# endif
# ifdef	RANDOM
	  case DOOR:
# endif
# ifdef OOZE
	  case SLIME:
# ifdef VOLCANO
	  case LAVA:
# endif
# endif
		return TRUE;
	}
	return FALSE;
}
# endif

/*
 * zapshot:
 *	Take a shot out of the air.
 */
static void
zapshot(blist, obp)
	BULLET	*blist, *obp;
{
	BULLET	*bp;
	FLAG	explode;

	explode = FALSE;
	for (bp = blist; bp != NULL; bp = bp->b_next) {
		if (bp->b_x != obp->b_x || bp->b_y != obp->b_y)
			continue;
		if (bp->b_face == obp->b_face)
			continue;
		explode = TRUE;
		break;
	}
	if (!explode)
		return;
	explshot(blist, obp->b_y, obp->b_x);
}

/*
 * explshot -
 *	Make all shots at this location blow up
 */
void
explshot(blist, y, x)
	BULLET	*blist;
	int	y, x;
{
	BULLET	*bp;

	for (bp = blist; bp != NULL; bp = bp->b_next)
		if (bp->b_x == x && bp->b_y == y) {
			bp->b_expl = TRUE;
			if (bp->b_owner != NULL)
				message(bp->b_owner, "Shot intercepted");
		}
}

/*
 * play_at:
 *	Return a pointer to the player at the given location
 */
PLAYER *
play_at(y, x)
	int	y, x;
{
	PLAYER	*pp;

	for (pp = Player; pp < End_player; pp++)
		if (pp->p_x == x && pp->p_y == y)
			return pp;
	errx(1, "driver: couldn't find player at (%d,%d)", x, y);
	/* NOTREACHED */
}

/*
 * opposite:
 *	Return TRUE if the bullet direction faces the opposite direction
 *	of the player in the maze
 */
int
opposite(face, dir)
	int	face;
	char	dir;
{
	switch (face) {
	  case LEFTS:
		return (dir == RIGHT);
	  case RIGHT:
		return (dir == LEFTS);
	  case ABOVE:
		return (dir == BELOW);
	  case BELOW:
		return (dir == ABOVE);
	  default:
		return FALSE;
	}
}

/*
 * is_bullet:
 *	Is there a bullet at the given coordinates?  If so, return
 *	a pointer to the bullet, otherwise return NULL
 */
BULLET *
is_bullet(y, x)
	int	y, x;
{
	BULLET	*bp;

	for (bp = Bullets; bp != NULL; bp = bp->b_next)
		if (bp->b_y == y && bp->b_x == x)
			return bp;
	return NULL;
}

/*
 * fixshots:
 *	change the underlying character of the shots at a location
 *	to the given character.
 */
void
fixshots(y, x, over)
	int	y, x;
	char	over;
{
	BULLET	*bp;

	for (bp = Bullets; bp != NULL; bp = bp->b_next)
		if (bp->b_y == y && bp->b_x == x)
			bp->b_over = over;
}

/*
 * find_under:
 *	find the underlying character for a bullet when it lands
 *	on another bullet.
 */
static void
find_under(blist, bp)
	BULLET	*blist, *bp;
{
	BULLET	*nbp;

	for (nbp = blist; nbp != NULL; nbp = nbp->b_next)
		if (bp->b_y == nbp->b_y && bp->b_x == nbp->b_x) {
			bp->b_over = nbp->b_over;
			break;
		}
}

/*
 * mark_player:
 *	mark a player as under a shot
 */
static void
mark_player(bp)
	BULLET	*bp;
{
	PLAYER	*pp;

	for (pp = Player; pp < End_player; pp++)
		if (pp->p_y == bp->b_y && pp->p_x == bp->b_x) {
			pp->p_undershot = TRUE;
			break;
		}
}

# ifdef BOOTS
/*
 * mark_boot:
 *	mark a boot as under a shot
 */
static void
mark_boot(bp)
	BULLET	*bp;
{
	PLAYER	*pp;

	for (pp = Boot; pp < &Boot[NBOOTS]; pp++)
		if (pp->p_y == bp->b_y && pp->p_x == bp->b_x) {
			pp->p_undershot = TRUE;
			break;
		}
}
# endif
