/*	$NetBSD: command7.c,v 1.4 2018/02/04 08:48:05 mrg Exp $	*/

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
static char sccsid[] = "@(#)com7.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: command7.c,v 1.4 2018/02/04 08:48:05 mrg Exp $");
#endif
#endif				/* not lint */

#include "extern.h"

int
fight(int enemy, int strength)
{
	int     lifeline = 0;
	int     hurt;
	char    auxbuf[LINELENGTH];
	char   *next;
	int     i;
	int     exhaustion;

	exhaustion = 0;
fighton:
	ourtime++;
	snooze -= 5;
	if (snooze > ourtime)
		exhaustion = CYCLE / (snooze - ourtime);
	else {
		puts("You collapse exhausted, and he pulverizes your skull.");
		die();
	}
	if (snooze - ourtime < 20)
		puts("You look tired! I hope you're able to fight.");
	next = getcom(auxbuf, LINELENGTH, "<fight!>-: ", 0);
	for (i = 0; next && i < 10; i++)
		next = getword(next, words[i], -1);
	parse();
	switch (wordvalue[wordnumber]) {

	case KILL:
	case SMITE:
		if (testbit(inven, TWO_HANDED))
			hurt = rnd(70) - 2 * card(injuries, NUMOFINJURIES) - 
			    ucard(wear) - exhaustion;
		else if (testbit(inven, SWORD) || testbit(inven, BROAD))
			hurt = rnd(50) % (WEIGHT - carrying) - 
			    card(injuries, NUMOFINJURIES) - encumber - 
			    exhaustion;
		else if (testbit(inven, KNIFE) || testbit(inven, MALLET) || 
		    testbit(inven, CHAIN) || testbit(inven, MACE) || 
		    testbit(inven, HALBERD))
			hurt = rnd(15) - card(injuries, NUMOFINJURIES) - 
			    exhaustion;
		else
			hurt = rnd(7) - encumber;
		if (hurt < 5)
			switch (rnd(3)) {

			case 0:
				puts("You swung wide and missed.");
				break;
			case 1:
				puts("He checked your blow. CLASH! CLANG!");
				break;
			case 2:
				printf("His filthy tunic hangs by one less ");
				puts("thread.");
				break;
			}
		else if (hurt < 10) {
			switch (rnd(3)) {
			case 0:
				puts("He's bleeding.");
				break;
			case 1:
				puts("A trickle of blood runs down his face.");
				break;
			case 2:
				printf("A huge purple bruise is forming on ");
				puts("the side of his face.");
				break;
			}
			lifeline++;
		} else if (hurt < 20) {
			switch (rnd(3)) {
			case 0:
				puts("He staggers back quavering.");
				break;
			case 1:
				printf("He jumps back with his hand over ");
				puts("the wound.");
				break;
			case 2:
				printf("His shirt falls open with a swath ");
				puts("across the chest.");
				break;
			}
			lifeline += 5;
		} else if (hurt < 30) {
			switch (rnd(3)) {
			case 0:
				printf("A bloody gash opens up on his %s ", 
				    (rnd(2) ? "left" : "right"));
				printf("side.\n");
				break;
			case 1:
				printf("The steel bites home and scrapes ");
				puts("along his ribs.");
				break;
			case 2:
				printf("You pierce him, and his breath ");
				puts("hisses through clenched teeth.");
				break;
			}
			lifeline += 10;
		} else if (hurt < 40) {
			switch (rnd(3)) {
			case 0:
				puts("You smite him to the ground.");
				if (strength - lifeline > 20) {
					printf("But in a flurry of steel he ");
					puts("regains his feet!");
				}
				break;
			case 1:
				printf("The force of your blow sends him to ");
				puts("his knees.");
				puts("His arm swings lifeless at his side.");
				break;
			case 2:
				printf("Clutching his blood drenched shirt, ");
				puts("he collapses stunned.");
				break;
			}
			lifeline += 20;
		} else {
			switch (rnd(3)) {
			case 0:
				printf("His ribs crack under your powerful ");
				puts("swing, flooding his lungs with blood.");
				break;
			case 1:
				printf("You shatter his upheld arm in a ");
				printf("spray of blood.  The blade ");
				puts("continues deep");
				printf("into his back, severing the ");
				puts("spinal cord.");
				lifeline += 25;
				break;
			case 2:
				printf("With a mighty lunge the steel ");
				printf("slides in, and gasping, he falls ");
				puts("to the ground.");
				lifeline += 25;
				break;
			}
			lifeline += 30;
		}
		break;

	case BACK:
		if (enemy == DARK && lifeline > strength * 0.33) {
			printf("He throws you back against the rock and ");
			puts("pummels your face.");
			if (testbit(inven, AMULET) || testbit(wear, AMULET)) {
				printf("Lifting the amulet from you, ");
				if (testbit(inven, MEDALION) || 
				    testbit(wear, MEDALION)) {
					printf("his power grows and the ");
					printf("walls of\nthe earth ");
					printf("tremble.\n");
					printf("When he touches the ");
					printf("medallion, your chest ");
					printf("explodes and the foundations ");
					printf("of the\nearth collapse.\n");
					printf("The planet is consumed by ");
					puts("darkness.");
					die();
				}
				if (testbit(inven, AMULET)) {
					clearbit(inven, AMULET);
					carrying -= objwt[AMULET];
					encumber -= objcumber[AMULET];
				} else
					clearbit(wear, AMULET);
				puts("he flees down the dark caverns.");
				clearbit(location[position].objects, DARK);
				injuries[SKULL] = 1;
				followfight = ourtime;
				return (0);
			} else {
				puts("I'm afraid you have been killed.");
				die();
			}
		} else {
			printf("You escape stunned and disoriented from ");
			puts("the fight.");
			printf("A victorious bellow echoes from the ");
			puts("battlescene.");
			if (back && position != back)
				moveplayer(back, BACK);
			else if (ahead && position != ahead)
				moveplayer(ahead, AHEAD);
			else if (left && position != left)
				moveplayer(left, LEFT);
			else if (right && position != right)
				moveplayer(right, RIGHT);
			else
				moveplayer(location[position].down, AHEAD);
			return (0);
		}

	case SHOOT:
		if (testbit(inven, LASER)) {
			if (strength - lifeline <= 50) {
				printf("The %s took a direct hit!\n", 
				    objsht[enemy]);
				lifeline += 50;
			} else {
				printf("With his bare hand he deflects the ");
				printf("laser blast and whips the pistol ");
				puts("from you!");
				clearbit(inven, LASER);
				setbit(location[position].objects, LASER);
				carrying -= objwt[LASER];
				encumber -= objcumber[LASER];
			}
		} else
			puts("Unfortunately, you don't have a blaster handy.");
		break;

	case DROP:
	case DRAW:
		cypher();
		ourtime--;
		break;

	default:
		puts("You don't have a chance; he is too quick.");
		break;

	}
	if (lifeline >= strength) {
		printf("You have killed the %s.\n", objsht[enemy]);
		if (enemy == ELF || enemy == DARK) {
			printf("A watery black smoke consumes his body and ");
			puts("then vanishes with a peal of thunder!");
		}
		clearbit(location[position].objects, enemy);
		power += 2;
		notes[JINXED]++;
		return (0);
	}
	puts("He attacks...");
	/* Some embellishments. */
	hurt = rnd(NUMOFINJURIES) - (testbit(inven, SHIELD) != 0) - 
	    (testbit(wear, MAIL) != 0) - (testbit(wear, HELM) != 0);
	hurt += (testbit(wear, AMULET) != 0) + 
	    (testbit(wear, MEDALION) != 0) + (testbit(wear, TALISMAN) != 0);
	hurt = hurt < 0 ? 0 : hurt;
	hurt = hurt >= NUMOFINJURIES ? NUMOFINJURIES - 1 : hurt;
	if (!injuries[hurt]) {
		injuries[hurt] = 1;
		printf("I'm afraid you have suffered %s.\n", ouch[hurt]);
	} else
		puts("You emerge unscathed.");
	if (injuries[SKULL] && injuries[INCISE] && injuries[NECK]) {
		puts("I'm afraid you have suffered fatal injuries.");
		die();
	}
	goto fighton;
}
