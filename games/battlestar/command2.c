/*	$NetBSD: command2.c,v 1.2 2003/08/07 09:37:00 agc Exp $	*/

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
static char sccsid[] = "@(#)com2.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: command2.c,v 1.2 2003/08/07 09:37:00 agc Exp $");
#endif
#endif				/* not lint */

#include "extern.h"

int
wearit()
{				/* synonyms = {sheathe, sheath} */
	int     firstnumber, value;

	firstnumber = wordnumber;
	wordnumber++;
	while (wordnumber <= wordcount && (wordtype[wordnumber] == OBJECT ||
	    wordtype[wordnumber] == NOUNS) && wordvalue[wordnumber] != DOOR) {
		value = wordvalue[wordnumber];
		if (value >= 0 && objsht[value] == NULL)
			break;
		switch (value) {

		case -1:
			puts("Wear what?");
			return (firstnumber);

		default:
			printf("You can't wear %s%s!\n",
			    A_OR_AN_OR_BLANK(value), objsht[value]);
			return (firstnumber);

		case KNIFE:
			/* case SHIRT:	 */
		case ROBE:
		case LEVIS:	/* wearable things */
		case SWORD:
		case MAIL:
		case HELM:
		case SHOES:
		case PAJAMAS:
		case COMPASS:
		case LASER:
		case AMULET:
		case TALISMAN:
		case MEDALION:
		case ROPE:
		case RING:
		case BRACELET:
		case GRENADE:

			if (testbit(inven, value)) {
				clearbit(inven, value);
				setbit(wear, value);
				carrying -= objwt[value];
				encumber -= objcumber[value];
				ourtime++;
				printf("You are now wearing %s%s.\n",
				    A_OR_AN_OR_THE(value), objsht[value]);
			} else
				if (testbit(wear, value))
					printf("You are already wearing the %s.\n",
					    objsht[value]);
				else
					printf("You aren't holding the %s.\n",
					    objsht[value]);
			if (wordnumber < wordcount - 1 &&
			    wordvalue[++wordnumber] == AND)
				wordnumber++;
			else
				return (firstnumber);
		}		/* end switch */
	}			/* end while */
	puts("Don't be ridiculous.");
	return (firstnumber);
}

int
put()
{				/* synonyms = {buckle, strap, tie} */
	if (wordvalue[wordnumber + 1] == ON) {
		wordvalue[++wordnumber] = PUTON;
		wordtype[wordnumber] = VERB;
		return (cypher());
	}
	if (wordvalue[wordnumber + 1] == DOWN) {
		wordvalue[++wordnumber] = DROP;
		wordtype[wordnumber] = VERB;
		return (cypher());
	}
	puts("I don't understand what you want to put.");
	return (-1);

}

int
draw()
{				/* synonyms = {pull, carry} */
	return (take(wear));
}

int
use()
{
	wordnumber++;
	if (wordvalue[wordnumber] == AMULET && testbit(inven, AMULET) &&
	    position != FINAL) {
		puts("The amulet begins to glow.");
		if (testbit(inven, MEDALION)) {
			puts("The medallion comes to life too.");
			if (position == 114) {
				location[position].down = 160;
				whichway(location[position]);
				puts("The waves subside and it is possible to descend to the sea cave now.");
				ourtime++;
				return (-1);
			}
		}
		puts("A light mist falls over your eyes and the sound of purling water trickles in");
		puts("your ears.   When the mist lifts you are standing beside a cool stream.");
		if (position == 229)
			position = 224;
		else
			position = 229;
		ourtime++;
		notes[CANTSEE] = 0;
		return (0);
	} else if (position == FINAL)
		puts("The amulet won't work in here.");
	else if (wordvalue[wordnumber] == COMPASS && testbit(inven, COMPASS))
		printf("Your compass points %s.\n", truedirec(NORTH, '-'));
	else if (wordvalue[wordnumber] == COMPASS)
		puts("You aren't holding the compass.");
	else if (wordvalue[wordnumber] == AMULET)
		puts("You aren't holding the amulet.");
	else
		puts("There is no apparent use.");
	return (-1);
}

void
murder()
{
	int     n;

	for (n = 0; !((n == SWORD || n == KNIFE || n == TWO_HANDED || n == MACE || n == CLEAVER || n == BROAD || n == CHAIN || n == SHOVEL || n == HALBERD) && testbit(inven, n)) && n < NUMOFOBJECTS; n++);
	if (n == NUMOFOBJECTS) {
		if (testbit(inven, LASER)) {
			printf("Your laser should do the trick.\n");
			wordnumber++;
			switch(wordvalue[wordnumber]) {
			case NORMGOD:
			case TIMER:
			case NATIVE:
			case MAN:
				wordvalue[--wordnumber] = SHOOT;
				cypher();
				break;
			case -1:
				puts("Kill what?");
				break;
			default:
				if (wordtype[wordnumber] != OBJECT ||
				    wordvalue[wordnumber] == EVERYTHING)
					puts("You can't kill that!");
				else
					printf("You can't kill %s%s!\n",
					    A_OR_AN_OR_BLANK(wordvalue[wordnumber]),
					    objsht[wordvalue[wordnumber]]);
				break;
			}
		} else
			puts("You don't have suitable weapons to kill.");
	} else {
		printf("Your %s should do the trick.\n", objsht[n]);
		wordnumber++;
		switch (wordvalue[wordnumber]) {

		case NORMGOD:
			if (testbit(location[position].objects, BATHGOD)) {
				puts("The goddess's head slices off.  Her corpse floats in the water.");
				clearbit(location[position].objects, BATHGOD);
				setbit(location[position].objects, DEADGOD);
				power += 5;
				notes[JINXED]++;
			} else
				if (testbit(location[position].objects, NORMGOD)) {
					puts("The goddess pleads but you strike her mercilessly.  Her broken body lies in a\npool of blood.");
					clearbit(location[position].objects, NORMGOD);
					setbit(location[position].objects, DEADGOD);
					power += 5;
					notes[JINXED]++;
					if (wintime)
						live();
				} else
					puts("I don't see her anywhere.");
			break;
		case TIMER:
			if (testbit(location[position].objects, TIMER)) {
				puts("The old man offers no resistance.");
				clearbit(location[position].objects, TIMER);
				setbit(location[position].objects, DEADTIME);
				power++;
				notes[JINXED]++;
			} else
				puts("Who?");
			break;
		case NATIVE:
			if (testbit(location[position].objects, NATIVE)) {
				puts("The girl screams as you cut her body to shreds.  She is dead.");
				clearbit(location[position].objects, NATIVE);
				setbit(location[position].objects, DEADNATIVE);
				power += 5;
				notes[JINXED]++;
			} else
				puts("What girl?");
			break;
		case MAN:
			if (testbit(location[position].objects, MAN)) {
				puts("You strike him to the ground, and he coughs up blood.");
				puts("Your fantasy is over.");
				die();
			}
		case -1:
			puts("Kill what?");
			break;

		default:
			if (wordtype[wordnumber] != OBJECT ||
			    wordvalue[wordnumber] == EVERYTHING)
				puts("You can't kill that!");
			else
				printf("You can't kill the %s!\n",
				    objsht[wordvalue[wordnumber]]);
		}
	}
}

void
ravage()
{
	while (wordtype[++wordnumber] != NOUNS && wordnumber <= wordcount)
		continue;
	if (wordtype[wordnumber] == NOUNS && (testbit(location[position].objects, wordvalue[wordnumber])
	    || (wordvalue[wordnumber] == NORMGOD && testbit(location[position].objects, BATHGOD)))) {
		ourtime++;
		switch (wordvalue[wordnumber]) {
		case NORMGOD:
			puts("You attack the goddess, and she screams as you beat her.  She falls down");
			if (testbit(location[position].objects, BATHGOD))
				puts("crying and tries to cover her nakedness.");
			else
				puts("crying and tries to hold her torn and bloodied dress around her.");
			power += 5;
			pleasure += 8;
			ego -= 10;
			wordnumber--;
			godready = -30000;
			murder();
			win = -30000;
			break;
		case NATIVE:
			puts("The girl tries to run, but you catch her and throw her down.  Her face is");
			puts("bleeding, and she screams as you tear off her clothes.");
			power += 3;
			pleasure += 5;
			ego -= 10;
			wordnumber--;
			murder();
			if (rnd(100) < 50) {
				puts("Her screams have attracted attention.  I think we are surrounded.");
				setbit(location[ahead].objects, WOODSMAN);
				setbit(location[ahead].objects, DEADWOOD);
				setbit(location[ahead].objects, MALLET);
				setbit(location[back].objects, WOODSMAN);
				setbit(location[back].objects, DEADWOOD);
				setbit(location[back].objects, MALLET);
				setbit(location[left].objects, WOODSMAN);
				setbit(location[left].objects, DEADWOOD);
				setbit(location[left].objects, MALLET);
				setbit(location[right].objects, WOODSMAN);
				setbit(location[right].objects, DEADWOOD);
				setbit(location[right].objects, MALLET);
			}
			break;
		default:
			puts("You are perverted.");
		}
	} else
		puts("Who?");
}

int
follow()
{
	if (followfight == ourtime) {
		puts("The Dark Lord leaps away and runs down secret tunnels and corridors.");
		puts("You chase him through the darkness and splash in pools of water.");
		puts("You have cornered him.  His laser sword extends as he steps forward.");
		position = FINAL;
		fight(DARK, 75);
		setbit(location[position].objects, TALISMAN);
		setbit(location[position].objects, AMULET);
		return (0);
	} else
		if (followgod == ourtime) {
			puts("The goddess leads you down a steamy tunnel and into a high, wide chamber.");
			puts("She sits down on a throne.");
			position = 268;
			setbit(location[position].objects, NORMGOD);
			notes[CANTSEE] = 1;
			return (0);
		} else
			puts("There is no one to follow.");
	return (-1);
}
