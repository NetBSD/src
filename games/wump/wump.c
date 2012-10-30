/*	$NetBSD: wump.c,v 1.29.2.1 2012/10/30 18:58:28 yamt Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Dave Taylor, of Intuitive Systems.
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
__COPYRIGHT("@(#) Copyright (c) 1989, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)wump.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: wump.c,v 1.29.2.1 2012/10/30 18:58:28 yamt Exp $");
#endif
#endif /* not lint */

/*
 * A very new version of the age old favorite Hunt-The-Wumpus game that has
 * been a part of the BSD distribution of Unix for longer than us old folk
 * would care to remember.
 */

#include <err.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "pathnames.h"

/* some defines to spec out what our wumpus cave should look like */

#define	MAX_ARROW_SHOT_DISTANCE	6		/* +1 for '0' stopper */
#define	MAX_LINKS_IN_ROOM	25		/* a complex cave */

#define	MAX_ROOMS_IN_CAVE	250
#define	ROOMS_IN_CAVE		20
#define	MIN_ROOMS_IN_CAVE	10

#define	LINKS_IN_ROOM		3
#define	NUMBER_OF_ARROWS	5
#define	PIT_COUNT		3
#define	BAT_COUNT		3

#define	EASY			1		/* levels of play */
#define	HARD			2

/* some macro definitions for cleaner output */

#define	plural(n)	(n == 1 ? "" : "s")

/* simple cave data structure; +1 so we can index from '1' not '0' */
static struct room_record {
	int tunnel[MAX_LINKS_IN_ROOM];
	int has_a_pit, has_a_bat;
} cave[MAX_ROOMS_IN_CAVE+1];

/*
 * global variables so we can keep track of where the player is, how
 * many arrows they still have, where el wumpo is, and so on...
 */
static int player_loc = -1;		/* player location */
static int wumpus_loc = -1;		/* The Bad Guy location */
static int level = EASY;		/* level of play */
static int arrows_left;			/* arrows unshot */

#ifdef DEBUG
static int debug = 0;
#endif

static int pit_num = PIT_COUNT;		/* # pits in cave */
static int bat_num = BAT_COUNT;		/* # bats */
static int room_num = ROOMS_IN_CAVE;	/* # rooms in cave */
static int link_num = LINKS_IN_ROOM;	/* links per room  */
static int arrow_num = NUMBER_OF_ARROWS;/* arrow inventory */

static char answer[20];			/* user input */

int	main(int, char **);
static int bats_nearby(void);
static void cave_init(void);
static void clear_things_in_cave(void);
static void display_room_stats(void);
static int gcd(int, int);
static int getans(const char *);
static void initialize_things_in_cave(void);
static void instructions(void);
static int int_compare(const void *, const void *);
static void jump(int);
static void kill_wump(void);
static int move_to(const char *);
static void move_wump(void);
static void no_arrows(void);
static void pit_kill(void);
static int pit_nearby(void);
static void pit_survive(void);
static int shoot(char *);
static void shoot_self(void);
static int take_action(void);
static void usage(void) __dead;
static void wump_kill(void);
static int wump_nearby(void);

int
main(int argc, char **argv)
{
	int c, e=0;

	/* Revoke setgid privileges */
	setgid(getgid());

#ifdef DEBUG
	while ((c = getopt(argc, argv, "a:b:hp:r:t:d")) != -1)
#else
	while ((c = getopt(argc, argv, "a:b:hp:r:t:")) != -1)
#endif
		switch (c) {
		case 'a':
			arrow_num = atoi(optarg);
			break;
		case 'b':
			bat_num = atoi(optarg);
			break;
#ifdef DEBUG
		case 'd':
			debug = 1;
			break;
#endif
		case 'h':
			level = HARD;
			break;
		case 'p':
			pit_num = atoi(optarg);
			break;
		case 'r':
			room_num = atoi(optarg);
			if (room_num < MIN_ROOMS_IN_CAVE) {
				(void)fprintf(stderr,
	"No self-respecting wumpus would live in such a small cave!\n");
				exit(1);
			}
			if (room_num > MAX_ROOMS_IN_CAVE) {
				(void)fprintf(stderr,
	"Even wumpi can't furnish caves that large!\n");
				exit(1);
			}
			break;
		case 't':
			link_num = atoi(optarg);
			if (link_num < 2) {
				(void)fprintf(stderr,
	"Wumpi like extra doors in their caves!\n");
				exit(1);
			}
			break;
		case '?':
		default:
			usage();
	}

	if (link_num > MAX_LINKS_IN_ROOM ||
	    link_num > room_num - (room_num / 4)) {
		(void)fprintf(stderr,
"Too many tunnels!  The cave collapsed!\n(Fortunately, the wumpus escaped!)\n");
		exit(1);
	}

	if (level == HARD) {
		bat_num += ((random() % (room_num / 2)) + 1);
		pit_num += ((random() % (room_num / 2)) + 1);
	}

	if (bat_num > room_num / 2) {
		(void)fprintf(stderr,
"The wumpus refused to enter the cave, claiming it was too crowded!\n");
		exit(1);
	}

	if (pit_num > room_num / 2) {
		(void)fprintf(stderr,
"The wumpus refused to enter the cave, claiming it was too dangerous!\n");
		exit(1);
	}

	instructions();
	cave_init();

	/* and we're OFF!  da dum, da dum, da dum, da dum... */
	(void)printf(
"\nYou're in a cave with %d rooms and %d tunnels leading from each room.\n\
There are %d bat%s and %d pit%s scattered throughout the cave, and your\n\
quiver holds %d custom super anti-evil Wumpus arrows.  Good luck.\n",
	    room_num, link_num, bat_num, plural(bat_num), pit_num,
	    plural(pit_num), arrow_num);

	for (;;) {
		clear_things_in_cave();
		initialize_things_in_cave();
		arrows_left = arrow_num;
		do {
			display_room_stats();
			(void)printf("Move or shoot? (m-s) ");
			(void)fflush(stdout);
			if (!fgets(answer, sizeof(answer), stdin)) {
				e=2;
				break;
			}
		} while (!(e = take_action()));

		if (e == 2 || !getans("\nCare to play another game? (y-n) "))
			exit(0);
		if (getans("In the same cave? (y-n) ") == 0)
			cave_init();
	}
	/* NOTREACHED */
	return (0);
}

static void
display_room_stats(void)
{
	int i;

	/*
	 * Routine will explain what's going on with the current room, as well
	 * as describe whether there are pits, bats, & wumpi nearby.  It's
	 * all pretty mindless, really.
	 */
	(void)printf(
"\nYou are in room %d of the cave, and have %d arrow%s left.\n",
	    player_loc, arrows_left, plural(arrows_left));

	if (bats_nearby())
		(void)printf("*rustle* *rustle* (must be bats nearby)\n");
	if (pit_nearby())
		(void)printf("*whoosh* (I feel a draft from some pits).\n");
	if (wump_nearby())
		(void)printf("*sniff* (I can smell the evil Wumpus nearby!)\n");

	(void)printf("There are tunnels to rooms %d, ",
	   cave[player_loc].tunnel[0]);

	for (i = 1; i < link_num - 1; i++)
		if (cave[player_loc].tunnel[i] <= room_num)
			(void)printf("%d, ", cave[player_loc].tunnel[i]);
	(void)printf("and %d.\n", cave[player_loc].tunnel[link_num - 1]);
}

static int
take_action(void)
{
	/*
	 * Do the action specified by the player, either 'm'ove, 's'hoot
	 * or something exceptionally bizarre and strange!  Returns 1
	 * iff the player died during this turn, otherwise returns 0.
	 */
	switch (*answer) {
		case 'M':
		case 'm':			/* move */
			return(move_to(answer + 1));
		case 'S':
		case 's':			/* shoot */
			return(shoot(answer + 1));
		case 'Q':
		case 'q':
		case 'x':
			exit(0);
		case '\n':
			return(0);
		}
	if (random() % 15 == 1)
		(void)printf("Que pasa?\n");
	else
		(void)printf("I don't understand!\n");
	return(0);
}

static int
move_to(const char *room_number)
{
	int i, just_moved_by_bats, next_room, tunnel_available;

	/*
	 * This is responsible for moving the player into another room in the
	 * cave as per their directions.  If room_number is a null string,
	 * then we'll prompt the user for the next room to go into.   Once
	 * we've moved into the room, we'll check for things like bats, pits,
	 * and so on.  This routine returns 1 if something occurs that kills
	 * the player and 0 otherwise...
	 */
	tunnel_available = just_moved_by_bats = 0;
	next_room = atoi(room_number);

	/* crap for magic tunnels */
	if (next_room == room_num + 1 &&
	    cave[player_loc].tunnel[link_num-1] != next_room)
		++next_room;

	while (next_room < 1 || next_room > room_num + 1) {
		if (next_room < 0 && next_room != -1)
(void)printf("Sorry, but we're constrained to a semi-Euclidean cave!\n");
		if (next_room > room_num + 1)
(void)printf("What?  The cave surely isn't quite that big!\n");
		if (next_room == room_num + 1 &&
		    cave[player_loc].tunnel[link_num-1] != next_room) {
			(void)printf("What?  The cave isn't that big!\n");
			++next_room;
		}
		(void)printf("To which room do you wish to move? ");
		(void)fflush(stdout);
		if (!fgets(answer, sizeof(answer), stdin))
			return(1);
		next_room = atoi(answer);
	}

	/* now let's see if we can move to that room or not */
	tunnel_available = 0;
	for (i = 0; i < link_num; i++)
		if (cave[player_loc].tunnel[i] == next_room)
			tunnel_available = 1;

	if (!tunnel_available) {
		(void)printf("*Oof!*  (You hit the wall)\n");
		if (random() % 6 == 1) {
(void)printf("Your colorful comments awaken the wumpus!\n");
			move_wump();
			if (wumpus_loc == player_loc) {
				wump_kill();
				return(1);
			}
		}
		return(0);
	}

	/* now let's move into that room and check it out for dangers */
	if (next_room == room_num + 1)
		jump(next_room = (random() % room_num) + 1);

	player_loc = next_room;
	for (;;) {
		if (next_room == wumpus_loc) {		/* uh oh... */
			wump_kill();
			return(1);
		}
		if (cave[next_room].has_a_pit) {
			if (random() % 12 < 2) {
				pit_survive();
				return(0);
			} else {
				pit_kill();
				return(1);
			}
		}

		if (cave[next_room].has_a_bat) {
			(void)printf(
"*flap*  *flap*  *flap*  (humongous bats pick you up and move you%s!)\n",
			    just_moved_by_bats ? " again": "");
			next_room = player_loc = (random() % room_num) + 1;
			just_moved_by_bats = 1;
		}

		else
			break;
	}
	return(0);
}

static int
shoot(char *room_list)
{
	int chance, next, roomcnt;
	int j, arrow_location, lnk, ok;
	char *p;

	/*
	 * Implement shooting arrows.  Arrows are shot by the player indicating
	 * a space-separated list of rooms that the arrow should pass through;
	 * if any of the rooms they specify are not accessible via tunnel from
	 * the room the arrow is in, it will instead fly randomly into another
	 * room.  If the player hits the wumpus, this routine will indicate
	 * such.  If it misses, this routine will *move* the wumpus one room.
	 * If it's the last arrow, the player then dies...  Returns 1 if the
	 * player has won or died, 0 if nothing has happened.
	 */
	arrow_location = player_loc;
	for (roomcnt = 1;; ++roomcnt, room_list = NULL) {
		if (!(p = strtok(room_list, " \t\n"))) {
			if (roomcnt == 1) {
				(void)printf(
			"The arrow falls to the ground at your feet!\n");
				return(0);
			} else
				break;
		}
		if (roomcnt > 5) {
			(void)printf(
"The arrow wavers in its flight and and can go no further!\n");
			break;
		}
		next = atoi(p);
		for (j = 0, ok = 0; j < link_num; j++)
			if (cave[arrow_location].tunnel[j] == next)
				ok = 1;

		if (ok) {
			if (next > room_num) {
				(void)printf(
"A faint gleam tells you the arrow has gone through a magic tunnel!\n");
				arrow_location = (random() % room_num) + 1;
			} else
				arrow_location = next;
		} else {
			lnk = (random() % link_num);
			if (lnk == player_loc)
				(void)printf(
"*thunk*  The arrow can't find a way from %d to %d and flies back into\n\
your room!\n",
				    arrow_location, next);
			else if (cave[arrow_location].tunnel[lnk] > room_num)
				(void)printf(
"*thunk*  The arrow flies randomly into a magic tunnel, thence into\n\
room %d!\n",
				    cave[arrow_location].tunnel[lnk]);
			else
				(void)printf(
"*thunk*  The arrow can't find a way from %d to %d and flies randomly\n\
into room %d!\n",
				    arrow_location, next,
				    cave[arrow_location].tunnel[lnk]);
			arrow_location = cave[arrow_location].tunnel[lnk];
			break;
		}
		chance = random() % 10;
		if (roomcnt == 3 && chance < 2) {
			(void)printf(
"Your bowstring breaks!  *twaaaaaang*\n\
The arrow is weakly shot and can go no further!\n");
			break;
		} else if (roomcnt == 4 && chance < 6) {
			(void)printf(
"The arrow wavers in its flight and and can go no further!\n");
			break;
		}
	}

	/*
	 * now we've gotten into the new room let us see if El Wumpo is
	 * in the same room ... if so we've a HIT and the player WON!
	 */
	if (arrow_location == wumpus_loc) {
		kill_wump();
		return(1);
	}

	if (arrow_location == player_loc) {
		shoot_self();
		return(1);
	}

	if (!--arrows_left) {
		no_arrows();
		return(1);
	}

	{
		/* each time you shoot, it's more likely the wumpus moves */
		static int lastchance = 2;

		if (random() % (level == EASY ? 12 : 9) < (lastchance += 2)) {
			move_wump();
			if (wumpus_loc == player_loc)
				wump_kill();
			lastchance = random() % 3;

		}
	}
	return(0);
}

static int
gcd(int a, int b)
{
	int r;

	r = a % b;
	if (r == 0)
		return (b);
	return (gcd(b, r));
}

static void
cave_init(void)
{
	int i, j, k, lnk;
	int delta;

	/*
	 * This does most of the interesting work in this program actually!
	 * In this routine we'll initialize the Wumpus cave to have all rooms
	 * linking to all others by stepping through our data structure once,
	 * recording all forward links and backwards links too.  The parallel
	 * "linkcount" data structure ensures that no room ends up with more
	 * than three links, regardless of the quality of the random number
	 * generator that we're using.
	 */
	srandom((int)time((time_t *)0));

	/* initialize the cave first off. */
	for (i = 1; i <= room_num; ++i)
		for (j = 0; j < link_num ; ++j)
			cave[i].tunnel[j] = -1;

	/*
	 * Choose a random 'hop' delta for our guaranteed link.
	 * To keep the cave connected, we need the greatest common divisor
	 * of (delta + 1) and room_num to be 1.
	 */
	do {
		delta = (random() % (room_num - 1)) + 1;
	} while (gcd(room_num, delta + 1) != 1);

	for (i = 1; i <= room_num; ++i) {
		lnk = ((i + delta) % room_num) + 1;	/* connection */
		cave[i].tunnel[0] = lnk;		/* forw link */
		cave[lnk].tunnel[1] = i;		/* back link */
	}
	/* now fill in the rest of the cave with random connections */
	for (i = 1; i <= room_num; i++)
		for (j = 2; j < link_num ; j++) {
			if (cave[i].tunnel[j] != -1)
				continue;
try_again:		lnk = (random() % room_num) + 1;
			/* skip duplicates */
			for (k = 0; k < j; k++)
				if (cave[i].tunnel[k] == lnk)
					goto try_again;
			cave[i].tunnel[j] = lnk;
			if (random() % 2 == 1)
				continue;
			for (k = 0; k < link_num; ++k) {
				/* if duplicate, skip it */
				if (cave[lnk].tunnel[k] == i)
					k = link_num;

				/* if open link, use it, force exit */
				if (cave[lnk].tunnel[k] == -1) {
					cave[lnk].tunnel[k] = i;
					k = link_num;
				}
			}
		}
	/*
	 * now that we're done, sort the tunnels in each of the rooms to
	 * make it easier on the intrepid adventurer.
	 */
	for (i = 1; i <= room_num; ++i)
		qsort(cave[i].tunnel, link_num,
		    sizeof(cave[i].tunnel[0]), int_compare);

#ifdef DEBUG
	if (debug)
		for (i = 1; i <= room_num; ++i) {
			(void)printf("<room %d  has tunnels to ", i);
			for (j = 0; j < link_num; ++j)
				(void)printf("%d ", cave[i].tunnel[j]);
			(void)printf(">\n");
		}
#endif
}

static void
clear_things_in_cave(void)
{
	int i;

	/*
	 * remove bats and pits from the current cave in preparation for us
	 * adding new ones via the initialize_things_in_cave() routines.
	 */
	for (i = 1; i <= room_num; ++i)
		cave[i].has_a_bat = cave[i].has_a_pit = 0;
}

static void
initialize_things_in_cave(void)
{
	int i, loc;

	/* place some bats, pits, the wumpus, and the player. */
	for (i = 0; i < bat_num; ++i) {
		do {
			loc = (random() % room_num) + 1;
		} while (cave[loc].has_a_bat);
		cave[loc].has_a_bat = 1;
#ifdef DEBUG
		if (debug)
			(void)printf("<bat in room %d>\n", loc);
#endif
	}

	for (i = 0; i < pit_num; ++i) {
		do {
			loc = (random() % room_num) + 1;
		} while (cave[loc].has_a_pit || cave[loc].has_a_bat);
		cave[loc].has_a_pit = 1;
#ifdef DEBUG
		if (debug)
			(void)printf("<pit in room %d>\n", loc);
#endif
	}

	wumpus_loc = (random() % room_num) + 1;
#ifdef DEBUG
	if (debug)
		(void)printf("<wumpus in room %d>\n", loc);
#endif

	i = 0;
	do {
		player_loc = (random() % room_num) + 1;
		i++;
	} while (player_loc == wumpus_loc || cave[player_loc].has_a_pit ||
	    cave[player_loc].has_a_bat || (level == HARD ?
	        (link_num / room_num < 0.4 ? wump_nearby() : 0) : 0) ||
	    (i > 100 && player_loc != wumpus_loc));
}

static int
getans(const char *prompt)
{
	char buf[20];

	/*
	 * simple routine to ask the yes/no question specified until the user
	 * answers yes or no, then return 1 if they said 'yes' and 0 if they
	 * answered 'no'.
	 */
	for (;;) {
		(void)printf("%s", prompt);
		(void)fflush(stdout);
		if (!fgets(buf, sizeof(buf), stdin))
			return(0);
		if (*buf == 'N' || *buf == 'n')
			return(0);
		if (*buf == 'Y' || *buf == 'y')
			return(1);
		(void)printf(
"I don't understand your answer; please enter 'y' or 'n'!\n");
	}
	/* NOTREACHED */
}

static int
bats_nearby(void)
{ 
	int i;

	/* check for bats in the immediate vicinity */
	for (i = 0; i < link_num; ++i)
		if (cave[cave[player_loc].tunnel[i]].has_a_bat)
			return(1);
	return(0);
}

static int
pit_nearby(void)
{ 
	int i;

	/* check for pits in the immediate vicinity */
	for (i = 0; i < link_num; ++i)
		if (cave[cave[player_loc].tunnel[i]].has_a_pit)
			return(1);
	return(0);
}

static int
wump_nearby(void)
{
	int i, j;

	/* check for a wumpus within TWO caves of where we are */
	for (i = 0; i < link_num; ++i) {
		if (cave[player_loc].tunnel[i] == wumpus_loc)
			return(1);
		for (j = 0; j < link_num; ++j)
			if (cave[cave[player_loc].tunnel[i]].tunnel[j] ==
			    wumpus_loc)
				return(1);
	}
	return(0);
}

static void
move_wump(void)
{
	wumpus_loc = cave[wumpus_loc].tunnel[random() % link_num];
}

static int
int_compare(const void *a, const void *b)
{
	return(*(const int *)a < *(const int *)b ? -1 : 1);
}

static void
instructions(void)
{
	const char *pager;
	pid_t pid;
	int status;
	int fd;

	/*
	 * read the instructions file, if needed, and show the user how to
	 * play this game!
	 */
	if (!getans("Instructions? (y-n) "))
		return;

	if (access(_PATH_WUMPINFO, R_OK)) {
		(void)printf(
"Sorry, but the instruction file seems to have disappeared in a\n\
puff of greasy black smoke! (poof)\n");
		return;
	}

	if (!isatty(STDOUT_FILENO))
		pager = "cat";
	else {
		if (!(pager = getenv("PAGER")) || (*pager == 0))
			pager = _PATH_PAGER;
	}
	switch (pid = fork()) {
	case 0: /* child */
		if ((fd = open(_PATH_WUMPINFO, O_RDONLY)) == -1)
			err(1, "open %s", _PATH_WUMPINFO);
		if (dup2(fd, STDIN_FILENO) == -1)
			err(1, "dup2");
		(void)execl("/bin/sh", "sh", "-c", pager, (char *) NULL);
		err(1, "exec sh -c %s", pager);
	case -1:
		err(1, "fork");
	default:
		(void)waitpid(pid, &status, 0);
		break;
	}
}

static void
usage(void)
{
	(void)fprintf(stderr,
"usage: wump [-h] [-a arrows] [-b bats] [-p pits] [-r rooms] [-t tunnels]\n");
	exit(1);
}

/* messages */

static void
wump_kill(void)
{
	(void)printf(
"*ROAR* *chomp* *snurfle* *chomp*!\n\
Much to the delight of the Wumpus, you walked right into his mouth,\n\
making you one of the easiest dinners he's ever had!  For you, however,\n\
it's a rather unpleasant death.  The only good thing is that it's been\n\
so long since the evil Wumpus cleaned his teeth that you immediately\n\
passed out from the stench!\n");
}

static void
kill_wump(void)
{
	(void)printf(
"*thwock!* *groan* *crash*\n\n\
A horrible roar fills the cave, and you realize, with a smile, that you\n\
have slain the evil Wumpus and won the game!  You don't want to tarry for\n\
long, however, because not only is the Wumpus famous, but the stench of\n\
dead Wumpus is also quite well known, a stench plenty enough to slay the\n\
mightiest adventurer at a single whiff!!\n");
}

static void
no_arrows(void)
{
	(void)printf(
"\nYou turn and look at your quiver, and realize with a sinking feeling\n\
that you've just shot your last arrow (figuratively, too).  Sensing this\n\
with its psychic powers, the evil Wumpus rampages through the cave, finds\n\
you, and with a mighty *ROAR* eats you alive!\n");
}

static void
shoot_self(void)
{
	(void)printf(
"\n*Thwack!*  A sudden piercing feeling informs you that the ricochet\n\
of your wild arrow has resulted in it wedging in your side, causing\n\
extreme agony.  The evil Wumpus, with its psychic powers, realizes this\n\
and immediately rushes to your side, not to help, alas, but to EAT YOU!\n\
(*CHOMP*)\n");
}

static void
jump(int where)
{
	(void)printf(
"\nWith a jaunty step you enter the magic tunnel.  As you do, you\n\
notice that the walls are shimmering and glowing.  Suddenly you feel\n\
a very curious, warm sensation and find yourself in room %d!!\n", where);
}

static void
pit_kill(void)
{
	(void)printf(
"*AAAUUUUGGGGGHHHHHhhhhhhhhhh...*\n\
The whistling sound and updraft as you walked into this room of the\n\
cave apparently wasn't enough to clue you in to the presence of the\n\
bottomless pit.  You have a lot of time to reflect on this error as\n\
you fall many miles to the core of the earth.  Look on the bright side;\n\
you can at least find out if Jules Verne was right...\n");
}

static void
pit_survive(void)
{
	(void)printf(
"Without conscious thought you grab for the side of the cave and manage\n\
to grasp onto a rocky outcrop.  Beneath your feet stretches the limitless\n\
depths of a bottomless pit!  Rock crumbles beneath your feet!\n");
}
