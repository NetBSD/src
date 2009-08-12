/*	$NetBSD: command6.c,v 1.5 2009/08/12 05:20:38 dholland Exp $	*/

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
static char sccsid[] = "@(#)com6.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: command6.c,v 1.5 2009/08/12 05:20:38 dholland Exp $");
#endif
#endif				/* not lint */

#include "extern.h"
#include "pathnames.h"

static void post(int);

int
launch(void)
{
	if (testbit(location[position].objects, VIPER) && !notes[CANTLAUNCH]) {
		if (fuel > 4) {
			clearbit(location[position].objects, VIPER);
			position = location[position].up;
			notes[LAUNCHED] = 1;
			ourtime++;
			fuel -= 4;
			printf("You climb into the viper and prepare for ");
			puts("launch.");
			printf("With a touch of your thumb the turbo engines ");
			printf("ignite, thrusting you back into\nyour seat.\n");
			return (1);
		} else
			puts("Not enough fuel to launch.");
	} else
		puts("Can't launch.");
	return (0);
}

int
land(void)
{
	if (notes[LAUNCHED] && testbit(location[position].objects, LAND) &&
	    location[position].down) {
		notes[LAUNCHED] = 0;
		position = location[position].down;
		setbit(location[position].objects, VIPER);
		fuel -= 2;
		ourtime++;
		puts("You are down.");
		return (1);
	} else
		puts("You can't land here.");
	return (0);
}

void
die(void)
{				/* endgame */
	printf("bye.\nYour rating was %s.\n", rate());
	post(' ');
	exit(0);
}

void
diesig(int dummy __unused)
{
	die();
}

void
live(void)
{
	puts("\nYou win!");
	post('!');
	exit(0);
}

static FILE *score_fp;

void
open_score_file(void)
{
	score_fp = fopen(_PATH_SCORE, "a");
	if (score_fp == NULL)
		warn("open %s for append", _PATH_SCORE);
	if (score_fp != NULL && fileno(score_fp) < 3)
		exit(1);
}

static void
post(int ch)
{
	time_t tv;
	char   *date;
	sigset_t isigset, osigset;

	sigemptyset(&isigset);
	sigaddset(&isigset, SIGINT);
	sigprocmask(SIG_BLOCK, &isigset, &osigset);
	tv = time(NULL);
	date = ctime(&tv);
	date[24] = '\0';
	if (score_fp != NULL) {
		fprintf(score_fp, "%s  %8s  %c%20s", date, username, 
		    ch, rate());
		if (wiz)
			fprintf(score_fp, "   wizard\n");
		else
			if (tempwiz)
				fprintf(score_fp, "   WIZARD!\n");
			else
				fprintf(score_fp, "\n");
	}
	sigprocmask(SIG_SETMASK, &osigset, (sigset_t *) 0);
}

const char *
rate(void)
{
	int     score;

	score = max(max(pleasure, power), ego);
	if (score == pleasure) {
		if (score < 5)
			return ("novice");
		else if (score < 20)
			return ("junior voyeur");
		else if (score < 35)
			return ("Don Juan");
		else
			return ("Marquis De Sade");
	} else if (score == power) {
		if (score < 5)
			return ("serf");
		else if (score < 8)
			return ("Samurai");
		else if (score < 13)
			return ("Klingon");
		else if (score < 22)
			return ("Darth Vader");
		else
			return ("Sauron the Great");
	} else {
		if (score < 5)
			return ("Polyanna");
		else if (score < 10)
			return ("philanthropist");
		else if (score < 20)
			return ("Tattoo");
		else
			return ("Mr. Roarke");
	}
}

int
drive(void)
{
	if (testbit(location[position].objects, CAR)) {
		printf("You hop in the car and turn the key.  There is ");
		puts("a perceptible grating noise,");
		puts("and an explosion knocks you unconscious...");
		clearbit(location[position].objects, CAR);
		setbit(location[position].objects, CRASH);
		injuries[5] = injuries[6] = injuries[7] = injuries[8] = 1;
		ourtime += 15;
		zzz();
		return (0);
	} else
		puts("There is nothing to drive here.");
	return (-1);
}

int
ride(void)
{
	if (testbit(location[position].objects, HORSE)) {
		printf("You climb onto the stallion and kick it in the guts.");
		puts("  The stupid steed launches");
		printf("forward through bush and fern.  You are thrown and ");
		puts("the horse gallops off.");
		clearbit(location[position].objects, HORSE);
		while (!(position = rnd(NUMOFROOMS + 1)) || !OUTSIDE || 
		    !beenthere[position] || location[position].flyhere)
			continue;
		setbit(location[position].objects, HORSE);
		if (location[position].north)
			position = location[position].north;
		else if (location[position].south)
			position = location[position].south;
		else if (location[position].east)
			position = location[position].east;
		else
			position = location[position].west;
		return (0);
	} else
		puts("There is no horse here.");
	return (-1);
}

void
light(void)
{				/* synonyms = {strike, smoke} *//* for
				 * matches, cigars */
	if (testbit(inven, MATCHES) && matchcount) {
		puts("Your match splutters to life.");
		ourtime++;
		matchlight = 1;
		matchcount--;
		if (position == 217) {
			printf("The whole bungalow explodes with an ");
			puts("intense blast.");
			die();
		}
	} else
		puts("You're out of matches.");
}

void
dooropen(void)
{				/* synonyms = {open, unlock} */
	wordnumber++;
	if (wordnumber <= wordcount && wordtype[wordnumber] == NOUNS
	    && wordvalue[wordnumber] == DOOR) {
		switch(position) {
		case 189:
		case 231:
			if (location[189].north == 231)
				puts("The door is already open.");
			else
				puts("The door does not budge.");
			break;
		case 30:
			if (location[30].west == 25)
				puts("The door is gone.");
			else
				puts("The door is locked tight.");
			break;
		case 31:
			puts("That's one immovable door.");
			break;
		case 20:
			puts("The door is already ajar.");
			break;
		default:
			puts("What door?");
		}
	} else
		puts("That doesn't open.");
}
