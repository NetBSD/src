/*	$NetBSD: command5.c,v 1.4.2.1 2014/08/20 00:00:22 tls Exp $	*/

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
static char sccsid[] = "@(#)com5.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: command5.c,v 1.4.2.1 2014/08/20 00:00:22 tls Exp $");
#endif
#endif				/* not lint */

#include "extern.h"

void
kiss(void)
{
	while (wordtype[++wordnumber] != NOUNS && wordnumber <= wordcount)
		continue;
	/* The goddess must be "taken" first if bathing. */
	if (wordtype[wordnumber] == NOUNS && wordvalue[wordnumber] == NORMGOD
	    && testbit(location[position].objects, BATHGOD)) {
		wordvalue[--wordnumber] = TAKE;
		cypher();
		return;
	}
	if (wordtype[wordnumber] == NOUNS) {
		if (testbit(location[position].objects, 
		    wordvalue[wordnumber])) {
			pleasure++;
			printf("Kissed.\n");
			switch (wordvalue[wordnumber]) {
			case NORMGOD:
				switch (godready++) {
				case 0:
					printf("She squirms and avoids your ");
					puts("advances.");
					break;
				case 1:
					puts("She is coming around; she ");
					puts("didn't fight it as much.");
					break;
				case 2:
					puts("She's beginning to like it.");
					break;
				default:
					puts("She's gone limp.");

				}
				break;
			case NATIVE:
				printf("Her lips are warm and her body ");
				printf("robust.  She pulls you down to ");
				puts("the ground.");
				break;
			case TIMER:
				puts("The old man blushes.");
				break;
			case MAN:
				puts("The dwarf punches you in the kneecap.");
				break;
			default:
				pleasure--;
			}
		} else
			puts("I see nothing like that here.");
	} else
		puts("I'd prefer not to.");
}

void
love(void)
{
	int     n;

	while (wordtype[++wordnumber] != NOUNS && wordnumber <= wordcount)
		continue;
	if (wordtype[wordnumber] == NOUNS) {
		if ((testbit(location[position].objects, BATHGOD) ||
		    testbit(location[position].objects, NORMGOD)) &&
		    wordvalue[wordnumber] == NORMGOD) {
			if (loved) {
				printf("Loved.\n");
				return;
			}
			if (godready >= 2) {
				printf("She cuddles up to you, and her mouth ");
				printf("starts to work:\n'That was my ");
				printf("sister's amulet.  The lovely ");
				printf("goddess, Purl, was she.  The Empire\n");
				printf("captured her just after the Darkness ");
				printf("came.  My other sister, Vert, was ");
				printf("killed\nby the Dark Lord himself.  ");
				printf("He took her amulet and warped its ");
				printf("power.\nYour quest was foretold by ");
				printf("my father before he died, but to get ");
				printf("the Dark Lord's\namulet you must use ");
				printf("cunning and skill.  I will leave you ");
				puts("my amulet,");
				printf("which you may use as you wish.  As ");
				printf("for me, I am the last goddess of ");
				printf("the\nwaters.  My father was the ");
				printf("Island King, and the rule is ");
				printf("rightfully mine.'\n\nShe pulls the ");
				puts("throne out into a large bed.");
				power++;
				pleasure += 15;
				ego++;
				if (card(injuries, NUMOFINJURIES)) {
					printf("Her kisses revive you; your ");
					printf("wounds are healed.\n");
					for (n = 0; n < NUMOFINJURIES; n++)
						injuries[n] = 0;
					WEIGHT = MAXWEIGHT;
					CUMBER = MAXCUMBER;
				}
				printf("Goddess:\n");
				if (!loved)
					setbit(location[position].objects, 
					    MEDALION);
				loved = 1;
				ourtime += 10;
				printf("Loved.\n");
				zzz();
				return;
			} else {
				puts("You wish!");
				return;
			}
		}
		if (testbit(location[position].objects, 
		    wordvalue[wordnumber])) {
			if (wordvalue[wordnumber] == NATIVE) {
				printf("The girl is easy prey.  She peels ");
				puts("off her sarong and indulges you.");
				power++;
				pleasure += 5;
				printf("Girl:\n");
				ourtime += 10;
				printf("Loved.\n");
				zzz();
			}
			if (wordvalue[wordnumber] == MAN ||
			    wordvalue[wordnumber] == BODY ||
			    wordvalue[wordnumber] == ELF ||
			    wordvalue[wordnumber] == TIMER)
				puts("Kinky!");
			else
				puts("It doesn't seem to work.");
		} else
			puts("Where's your lover?");
	} else
		puts("It doesn't seem to work.");
}

int
zzz(void)
{
	int     oldtime;
	int     n;
	int zzztime;

	zzztime = (3 * CYCLE) / 4;

	oldtime = ourtime;
	if ((snooze - ourtime) < zzztime) {
		ourtime += zzztime - (snooze - ourtime);
		printf("<zzz>");
		for (n = 0; n < ourtime - oldtime; n++)
			printf(".");
		printf("\n");
		snooze += 3 * (ourtime - oldtime);
		if (notes[LAUNCHED]) {
			fuel -= (ourtime - oldtime);
			if (location[position].down) {
				position = location[position].down;
				crash();
			} else
				notes[LAUNCHED] = 0;
		}
		if (OUTSIDE && rnd(100) < 50) {
			printf("You are awakened abruptly by the sound ");
			puts("of someone nearby.");
			switch (rnd(4)) {
			case 0:
				if (ucard(inven)) {
					n = rnd(NUMOFOBJECTS);
					while (!testbit(inven, n))
						n = rnd(NUMOFOBJECTS);
					clearbit(inven, n);
					if (n != AMULET && n != MEDALION && 
					    n != TALISMAN)
						setbit(
						    location[position].objects,
						    n);
					carrying -= objwt[n];
					encumber -= objcumber[n];
				}
				printf("A fiendish little Elf is stealing ");
				puts("your treasures!");
				fight(ELF, 10);
				break;
			case 1:
				setbit(location[position].objects, DEADWOOD);
				break;
			case 2:
				setbit(location[position].objects, HALBERD);
				break;
			default:
				break;
			}
		}
	} else
		return (0);
	return (1);
}

void
chime(void)
{
	if ((ourtime / CYCLE + 1) % 2 && OUTSIDE)
		switch ((ourtime % CYCLE) / (CYCLE / 7)) {
		case 0:
			puts("It is just after sunrise.");
			break;
		case 1:
			puts("It is early morning.");
			break;
		case 2:
			puts("It is late morning.");
			break;
		case 3:
			puts("It is near noon.");
			break;
		case 4:
			puts("It is early afternoon.");
			break;
		case 5:
			puts("It is late afternoon.");
			break;
		case 6:
			puts("It is near sunset.");
			break;
		}
	else if (OUTSIDE)
		switch ((ourtime % CYCLE) / (CYCLE / 7)) {
		case 0:
			puts("It is just after sunset.");
			break;
		case 1:
			puts("It is early evening.");
			break;
		case 2:
			puts("The evening is getting old.");
			break;
		case 3:
			puts("It is near midnight.");
			break;
		case 4:
			puts("These are the wee hours of the morning.");
			break;
		case 5:
			puts("The night is waning.");
			break;
		case 6:
			puts("It is almost morning.");
			break;
		}
	else
		puts("I can't tell the time in here.");
}

int
give(void)
{
	int     obj = -1, result = -1, person = 0, firstnumber, last1, last2;

	last1 = last2 = 0;
	firstnumber = wordnumber;
	while (wordtype[++wordnumber] != OBJECT && 
	    wordvalue[wordnumber] != AMULET && 
	    wordvalue[wordnumber] != MEDALION && 
	    wordvalue[wordnumber] != TALISMAN && wordnumber <= wordcount)
		continue;
	if (wordnumber <= wordcount) {
		obj = wordvalue[wordnumber];
		if (obj == EVERYTHING)
			wordtype[wordnumber] = -1;
		last1 = wordnumber;
	}
	wordnumber = firstnumber;
	while ((wordtype[++wordnumber] != NOUNS || 
	    wordvalue[wordnumber] == obj) && wordnumber <= wordcount);
	if (wordtype[wordnumber] == NOUNS) {
		person = wordvalue[wordnumber];
		last2 = wordnumber;
	}
	/* Setting wordnumber to last1 - 1 looks wrong if last1 is 0, e.g.,
	 * plain `give'.  However, detecting this case is liable to detect
	 * `give foo' as well, which would give a confusing error.  We
	 * need to make sure the -1 value can cause no problems if it arises.
	 * If in the below we get to the drop("Given") then drop will look
	 * at word 0 for an object to give, and fail, which is OK; then
	 * result will be -1 and we get to the end, where wordnumber gets
	 * set to something more sensible.  If we get to "I don't think
	 * that is possible" then again wordnumber is set to something
	 * sensible.  The wordnumber we leave with still isn't right if
	 * you include words the game doesn't know in your command, but
	 * that's no worse than what other commands than give do in
	 * the same place.  */
	wordnumber = last1 - 1;
	if (person && testbit(location[position].objects, person)) {
		if (person == NORMGOD && godready < 2 && 
		    !(obj == RING || obj == BRACELET))
			puts("The goddess won't look at you.");
		else
			result = drop("Given");
	} else {
		puts("I don't think that is possible.");
		wordnumber = max(last1, last2) + 1;
		return (0);
	}
	if (result != -1 && (testbit(location[position].objects, obj) || 
	    obj == AMULET || obj == MEDALION || obj == TALISMAN)) {
		clearbit(location[position].objects, obj);
		ourtime++;
		ego++;
		switch (person) {
		case NATIVE:
			puts("She accepts it shyly.");
			ego += 2;
			break;
		case NORMGOD:
			if (obj == RING || obj == BRACELET) {
				printf("She takes the charm and puts it on.");
				puts("  A little kiss on the cheek is");
				puts("your reward.");
				ego += 5;
				godready += 3;
			}
			if (obj == AMULET || obj == MEDALION || 
			    obj == TALISMAN) {
				win++;
				ego += 5;
				power -= 5;
				if (win >= 3) {
					printf("The powers of the earth are ");
					printf("now legitimate.  You have ");
					puts("destroyed the Darkness");
					printf("and restored the goddess to ");
					printf("her throne.  The entire ");
					puts("island celebrates with");
					printf("dancing and spring feasts.  ");
					printf("As a measure of her ");
					puts("gratitude, the goddess weds you");
					printf("in the late summer and ");
					printf("crowns you Prince Liverwort, ");
					puts("Lord of Fungus.");
					printf("\nBut, as the year wears on ");
					printf("and autumn comes along, you ");
					puts("become restless and");
					printf("yearn for adventure.  The ");
					printf("goddess, too, realizes that ");
					puts("the marriage can't last.");
					printf("She becomes bored and takes ");
					printf("several more natives as ");
					puts("husbands.  One evening,");
					printf("after having been out ");
					printf("drinking with the girls, she ");
					puts("kicks the throne particularly");
					printf("hard and wakes you up.  (If ");
					printf("you want to win this game, ");
					printf("you're going to have to\n");
					puts("shoot her!)");
					clearbit(location[position].objects, 
					    MEDALION);
					wintime = ourtime;
				}
			}
			break;
		case TIMER:
			if (obj == COINS) {
				printf("He fingers the coins for a moment ");
				printf("and then looks up agape.  `Kind you ");
				puts("are and");
				printf("I mean to repay you as best I can.'  ");
				printf("Grabbing a pencil and cocktail ");
				puts("napkin...\n");
				printf("+-----------------------------------");
				printf("------------------------------------");
				printf("------+\n");
				printf("|				   xxxxxxxx\\				      |\n");
				printf("|				       xxxxx\\	CLIFFS			      |\n");
				printf("|		FOREST			  xxx\\				      |\n");
				printf("|				\\\\	     x\\        	OCEAN		      |\n");
				printf("|				||	       x\\			      |\n");
				printf("|				||  ROAD	x\\			      |\n");
				printf("|				||		x\\			      |\n");
				printf("|		SECRET		||	  .........			      |\n");
				printf("|		 - + -		||	   ........			      |\n");
				printf("|		ENTRANCE	||		...      BEACH		      |\n");
				printf("|				||		...		  E	      |\n");
				printf("|				||		...		  |	      |\n");
				printf("|				//		...	    N <-- + --- S     |\n");
				printf("|		PALM GROVE     //		...		  |	      |\n");
				printf("|			      //		...		  W	      |\n");
				printf("+");
				printf("---------------------------------");
				printf("---------------------------------");
				printf("-----------+\n");
				printf("\n`This map shows a secret entrance ");
				puts("to the catacombs.");
				printf("You will know when you arrive ");
				printf("because I left an old pair of shoes ");
				puts("there.'");
			}
			break;
		}
	}
	wordnumber = max(last1, last2) + 1;
	return (firstnumber);
}
