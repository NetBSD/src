/*	$NetBSD: misc.c,v 1.9 2003/05/08 13:03:49 wiz Exp $	*/

/*
 * misc.c  Phantasia miscellaneous support routines
 */

#include "include.h"


void
movelevel()
{
	const struct charstats *statptr; /* for pointing into Stattable */
	double  new;		/* new level */
	double  inc;		/* increment between new and old levels */

	Changed = TRUE;

	if (Player.p_type == C_EXPER)
		/* roll a type to use for increment */
		statptr = &Stattable[(int) ROLL(C_MAGIC, C_HALFLING - C_MAGIC + 1)];
	else
		statptr = Statptr;

	new = explevel(Player.p_experience);
	inc = new - Player.p_level;
	Player.p_level = new;

	/* add increments to statistics */
	Player.p_strength += statptr->c_strength.increase * inc;
	Player.p_mana += statptr->c_mana.increase * inc;
	Player.p_brains += statptr->c_brains.increase * inc;
	Player.p_magiclvl += statptr->c_magiclvl.increase * inc;
	Player.p_maxenergy += statptr->c_energy.increase * inc;

	/* rest to maximum upon reaching new level */
	Player.p_energy = Player.p_maxenergy + Player.p_shield;

	if (Player.p_crowns > 0 && Player.p_level >= 1000.0)
		/* no longer able to be king -- turn crowns into cash */
	{
		Player.p_gold += ((double) Player.p_crowns) * 5000.0;
		Player.p_crowns = 0;
	}
	if (Player.p_level >= 3000.0 && Player.p_specialtype < SC_COUNCIL)
		/* make a member of the council */
	{
		mvaddstr(6, 0, "You have made it to the Council of the Wise.\n");
		addstr("Good Luck on your search for the Holy Grail.\n");

		Player.p_specialtype = SC_COUNCIL;

		/* no rings for council and above */
		Player.p_ring.ring_type = R_NONE;
		Player.p_ring.ring_duration = 0;

		Player.p_lives = 3;	/* three extra lives */
	}
	if (Player.p_level > 9999.0 && Player.p_specialtype != SC_VALAR)
		death("Old age");
}

const char   *
descrlocation(playerp, shortflag)
	struct player *playerp;
	bool    shortflag;
{
	double  circle;		/* corresponding circle for coordinates */
	int     quadrant;	/* quandrant of grid */
	const char   *label;	/* pointer to place name */
	static const char *const nametable[4][4] =	/* names of places */
	{
		{"Anorien", "Ithilien", "Rohan", "Lorien"},
		{"Gondor", "Mordor", "Dunland", "Rovanion"},
		{"South Gondor", "Khand", "Eriador", "The Iron Hills"},
		{"Far Harad", "Near Harad", "The Northern Waste", "Rhun"}
	};

	if (playerp->p_specialtype == SC_VALAR)
		return (" is in Valhala");
	else
		if ((circle = CIRCLE(playerp->p_x, playerp->p_y)) >= 1000.0) {
			if (MAX(fabs(playerp->p_x), fabs(playerp->p_y)) > D_BEYOND)
				label = "The Point of No Return";
			else
				label = "The Ashen Mountains";
		} else
			if (circle >= 55)
				label = "Morannon";
			else
				if (circle >= 35)
					label = "Kennaquahair";
				else
					if (circle >= 20)
						label = "The Dead Marshes";
					else
						if (circle >= 9)
							label = "The Outer Waste";
						else
							if (circle >= 5)
								label = "The Moors Adventurous";
							else {
								if (playerp->p_x == 0.0 && playerp->p_y == 0.0)
									label = "The Lord's Chamber";
								else {
									/* this
									 * 
									 * expr
									 * essi
									 * on
									 * is
									 * spli
									 * t
									 * to
									 * prev
									 * ent
									 * comp
									 * iler
									 * 
									 * loop
									 * 
									 * with
									 * 
									 * some
									 * 
									 * comp
									 * iler
									 * s */
									quadrant = ((playerp->p_x > 0.0) ? 1 : 0);
									quadrant += ((playerp->p_y >= 0.0) ? 2 : 0);
									label = nametable[((int) circle) - 1][quadrant];
								}
							}

	if (shortflag)
		sprintf(Databuf, "%.29s", label);
	else
		sprintf(Databuf, " is in %s  (%.0f,%.0f)", label, playerp->p_x, playerp->p_y);

	return (Databuf);
}

void
tradingpost()
{
	double  numitems;	/* number of items to purchase */
	double  cost;		/* cost of purchase */
	double  blessingcost;	/* cost of blessing */
	int     ch;		/* input */
	int     size;		/* size of the trading post */
	int     loop;		/* loop counter */
	int     cheat = 0;	/* number of times player has tried to cheat */
	bool    dishonest = FALSE;	/* set when merchant is dishonest */

	Player.p_status = S_TRADING;
	writerecord(&Player, Fileloc);

	clear();
	addstr("You are at a trading post. All purchases must be made with gold.");

	size = sqrt(fabs(Player.p_x / 100)) + 1;
	size = MIN(7, size);

	/* set up cost of blessing */
	blessingcost = 1000.0 * (Player.p_level + 5.0);

	/* print Menu */
	move(7, 0);
	for (loop = 0; loop < size; ++loop)
		/* print Menu */
	{
		if (loop == 6)
			cost = blessingcost;
		else
			cost = Menu[loop].cost;
		printw("(%d) %-12s: %6.0f\n", loop + 1, Menu[loop].item, cost);
	}

	mvprintw(5, 0, "L:Leave  P:Purchase  S:Sell Gems ? ");

	for (;;) {
		adjuststats();	/* truncate any bad values */

		/* print some important statistics */
		mvprintw(1, 0, "Gold:   %9.0f  Gems:  %9.0f  Level:   %6.0f  Charms: %6d\n",
		    Player.p_gold, Player.p_gems, Player.p_level, Player.p_charms);
		printw("Shield: %9.0f  Sword: %9.0f  Quicksilver:%3.0f  Blessed: %s\n",
		    Player.p_shield, Player.p_sword, Player.p_quksilver,
		    (Player.p_blessing ? " True" : "False"));
		printw("Brains: %9.0f  Mana:  %9.0f", Player.p_brains, Player.p_mana);

		move(5, 36);
		ch = getanswer("LPS", FALSE);
		move(15, 0);
		clrtobot();
		switch (ch) {
		case 'L':	/* leave */
		case '\n':
			altercoordinates(0.0, 0.0, A_NEAR);
			return;

		case 'P':	/* make purchase */
			mvaddstr(15, 0, "What what would you like to buy ? ");
			ch = getanswer(" 1234567", FALSE);
			move(15, 0);
			clrtoeol();

			if (ch - '0' > size)
				addstr("Sorry, this merchant doesn't have that.");
			else
				switch (ch) {
				case '1':
					printw("Mana is one per %.0f gold piece.  How many do you want (%.0f max) ? ",
					    Menu[0].cost, floor(Player.p_gold / Menu[0].cost));
					cost = (numitems = floor(infloat())) * Menu[0].cost;

					if (cost > Player.p_gold || numitems < 0)
						++cheat;
					else {
						cheat = 0;
						Player.p_gold -= cost;
						if (drandom() < 0.02)
							dishonest = TRUE;
						else
							Player.p_mana += numitems;
					}
					break;

				case '2':
					printw("Shields are %.0f per +1.  How many do you want (%.0f max) ? ",
					    Menu[1].cost, floor(Player.p_gold / Menu[1].cost));
					cost = (numitems = floor(infloat())) * Menu[1].cost;

					if (numitems == 0.0)
						break;
					else
						if (cost > Player.p_gold || numitems < 0)
							++cheat;
						else
							if (numitems < Player.p_shield)
								NOBETTER();
							else {
								cheat = 0;
								Player.p_gold -= cost;
								if (drandom() < 0.02)
									dishonest = TRUE;
								else
									Player.p_shield = numitems;
							}
					break;

				case '3':
					printw("A book costs %.0f gp.  How many do you want (%.0f max) ? ",
					    Menu[2].cost, floor(Player.p_gold / Menu[2].cost));
					cost = (numitems = floor(infloat())) * Menu[2].cost;

					if (cost > Player.p_gold || numitems < 0)
						++cheat;
					else {
						cheat = 0;
						Player.p_gold -= cost;
						if (drandom() < 0.02)
							dishonest = TRUE;
						else
							if (drandom() * numitems > Player.p_level / 10.0
							    && numitems != 1) {
								printw("\nYou blew your mind!\n");
								Player.p_brains /= 5;
							} else {
								Player.p_brains += floor(numitems) * ROLL(20, 8);
							}
					}
					break;

				case '4':
					printw("Swords are %.0f gp per +1.  How many + do you want (%.0f max) ? ",
					    Menu[3].cost, floor(Player.p_gold / Menu[3].cost));
					cost = (numitems = floor(infloat())) * Menu[3].cost;

					if (numitems == 0.0)
						break;
					else
						if (cost > Player.p_gold || numitems < 0)
							++cheat;
						else
							if (numitems < Player.p_sword)
								NOBETTER();
							else {
								cheat = 0;
								Player.p_gold -= cost;
								if (drandom() < 0.02)
									dishonest = TRUE;
								else
									Player.p_sword = numitems;
							}
					break;

				case '5':
					printw("A charm costs %.0f gp.  How many do you want (%.0f max) ? ",
					    Menu[4].cost, floor(Player.p_gold / Menu[4].cost));
					cost = (numitems = floor(infloat())) * Menu[4].cost;

					if (cost > Player.p_gold || numitems < 0)
						++cheat;
					else {
						cheat = 0;
						Player.p_gold -= cost;
						if (drandom() < 0.02)
							dishonest = TRUE;
						else
							Player.p_charms += numitems;
					}
					break;

				case '6':
					printw("Quicksilver is %.0f gp per +1.  How many + do you want (%.0f max) ? ",
					    Menu[5].cost, floor(Player.p_gold / Menu[5].cost));
					cost = (numitems = floor(infloat())) * Menu[5].cost;

					if (numitems == 0.0)
						break;
					else
						if (cost > Player.p_gold || numitems < 0)
							++cheat;
						else
							if (numitems < Player.p_quksilver)
								NOBETTER();
							else {
								cheat = 0;
								Player.p_gold -= cost;
								if (drandom() < 0.02)
									dishonest = TRUE;
								else
									Player.p_quksilver = numitems;
							}
					break;

				case '7':
					if (Player.p_blessing) {
						addstr("You already have a blessing.");
						break;
					}
					printw("A blessing requires a %.0f gp donation.  Still want one ? ", blessingcost);
					ch = getanswer("NY", FALSE);

					if (ch == 'Y') {
						if (Player.p_gold < blessingcost)
							++cheat;
						else {
							cheat = 0;
							Player.p_gold -= blessingcost;
							if (drandom() < 0.02)
								dishonest = TRUE;
							else
								Player.p_blessing = TRUE;
						}
					}
					break;
				}
			break;

		case 'S':	/* sell gems */
			mvprintw(15, 0, "A gem is worth %.0f gp.  How many do you want to sell (%.0f max) ? ",
			    (double) N_GEMVALUE, Player.p_gems);
			numitems = floor(infloat());

			if (numitems > Player.p_gems || numitems < 0)
				++cheat;
			else {
				cheat = 0;
				Player.p_gems -= numitems;
				Player.p_gold += numitems * N_GEMVALUE;
			}
		}

		if (cheat == 1)
			mvaddstr(17, 0, "Come on, merchants aren't stupid.  Stop cheating.\n");
		else
			if (cheat == 2) {
				mvaddstr(17, 0, "You had your chance.  This merchant happens to be\n");
				printw("a %.0f level magic user, and you made %s mad!\n",
				    ROLL(Circle * 20.0, 40.0), (drandom() < 0.5) ? "him" : "her");
				altercoordinates(0.0, 0.0, A_FAR);
				Player.p_energy /= 2.0;
				++Player.p_sin;
				more(23);
				return;
			} else
				if (dishonest) {
					mvaddstr(17, 0, "The merchant stole your money!");
					refresh();
					altercoordinates(Player.p_x - Player.p_x / 10.0,
					    Player.p_y - Player.p_y / 10.0, A_SPECIFIC);
					sleep(2);
					return;
				}
	}
}

void
displaystats()
{
	mvprintw(0, 0, "%s%s\n", Player.p_name, descrlocation(&Player, FALSE));
	mvprintw(1, 0, "Level :%7.0f   Energy  :%9.0f(%9.0f)  Mana :%9.0f  Users:%3d\n",
	    Player.p_level, Player.p_energy, Player.p_maxenergy + Player.p_shield,
	    Player.p_mana, Users);
	mvprintw(2, 0, "Quick :%3.0f(%3.0f)  Strength:%9.0f(%9.0f)  Gold :%9.0f  %s\n",
	    Player.p_speed, Player.p_quickness + Player.p_quksilver, Player.p_might,
	    Player.p_strength + Player.p_sword, Player.p_gold, descrstatus(&Player));
}

void
allstatslist()
{
	static const char *const flags[] = /* to print value of some bools */
	{
		"False",
		" True"
	};

	mvprintw(8, 0, "Type: %s\n", descrtype(&Player, FALSE));

	mvprintw(10, 0, "Experience: %9.0f", Player.p_experience);
	mvprintw(11, 0, "Brains    : %9.0f", Player.p_brains);
	mvprintw(12, 0, "Magic Lvl : %9.0f", Player.p_magiclvl);
	mvprintw(13, 0, "Sin       : %9.5f", Player.p_sin);
	mvprintw(14, 0, "Poison    : %9.5f", Player.p_poison);
	mvprintw(15, 0, "Gems      : %9.0f", Player.p_gems);
	mvprintw(16, 0, "Age       : %9ld", Player.p_age);
	mvprintw(10, 40, "Holy Water: %9d", Player.p_holywater);
	mvprintw(11, 40, "Amulets   : %9d", Player.p_amulets);
	mvprintw(12, 40, "Charms    : %9d", Player.p_charms);
	mvprintw(13, 40, "Crowns    : %9d", Player.p_crowns);
	mvprintw(14, 40, "Shield    : %9.0f", Player.p_shield);
	mvprintw(15, 40, "Sword     : %9.0f", Player.p_sword);
	mvprintw(16, 40, "Quickslver: %9.0f", Player.p_quksilver);

	mvprintw(18, 0, "Blessing: %s   Ring: %s   Virgin: %s   Palantir: %s",
	    flags[(int)Player.p_blessing],
	    flags[Player.p_ring.ring_type != R_NONE],
	    flags[(int)Player.p_virgin],
	    flags[(int)Player.p_palantir]);
}

const char   *
descrtype(playerp, shortflag)
	struct player *playerp;
	bool    shortflag;
{
	int     type;		/* for caluculating result subscript */
	static const char *const results[] =/* description table */
	{
		" Magic User", " MU",
		" Fighter", " F ",
		" Elf", " E ",
		" Dwarf", " D ",
		" Halfling", " H ",
		" Experimento", " EX",
		" Super", " S ",
		" King", " K ",
		" Council of Wise", " CW",
		" Ex-Valar", " EV",
		" Valar", " V ",
		" ? ", " ? "
	};

	type = playerp->p_type;

	switch (playerp->p_specialtype) {
	case SC_NONE:
		type = playerp->p_type;
		break;

	case SC_KING:
		type = 7;
		break;

	case SC_COUNCIL:
		type = 8;
		break;

	case SC_EXVALAR:
		type = 9;
		break;

	case SC_VALAR:
		type = 10;
		break;
	}

	type *= 2;		/* calculate offset */

	if (type > 20)
		/* error */
		type = 22;

	if (shortflag)
		/* use short descriptions */
		++type;

	if (playerp->p_crowns > 0) {
		strcpy(Databuf, results[type]);
		Databuf[0] = '*';
		return (Databuf);
	} else
		return (results[type]);
}

long
findname(name, playerp)
	const char   *name;
	struct player *playerp;
{
	long    loc = 0;	/* location in the file */

	fseek(Playersfp, 0L, SEEK_SET);
	while (fread((char *) playerp, SZ_PLAYERSTRUCT, 1, Playersfp) == 1) {
		if (strcmp(playerp->p_name, name) == 0) {
			if (playerp->p_status != S_NOTUSED || Wizard)
				/* found it */
				return (loc);
		}
		loc += SZ_PLAYERSTRUCT;
	}

	return (-1);
}

long
allocrecord()
{
	long    loc = 0L;	/* location in file */

	fseek(Playersfp, 0L, SEEK_SET);
	while (fread((char *) &Other, SZ_PLAYERSTRUCT, 1, Playersfp) == 1) {
		if (Other.p_status == S_NOTUSED)
			/* found an empty record */
			return (loc);
		else
			loc += SZ_PLAYERSTRUCT;
	}

	/* make a new record */
	initplayer(&Other);
	Player.p_status = S_OFF;
	writerecord(&Other, loc);

	return (loc);
}

void
freerecord(playerp, loc)
	struct player *playerp;
	long    loc;
{
	playerp->p_name[0] = CH_MARKDELETE;
	playerp->p_status = S_NOTUSED;
	writerecord(playerp, loc);
}

void
leavegame()
{

	if (Player.p_level < 1.0)
		/* delete character */
		freerecord(&Player, Fileloc);
	else {
		Player.p_status = S_OFF;
		writerecord(&Player, Fileloc);
	}

	cleanup(TRUE);
	/* NOTREACHED */
}

void
death(how)
	const char   *how;
{
	FILE   *fp;		/* for updating various files */
	int     ch;		/* input */
	static const char *const deathmesg[] =
	/* add more messages here, if desired */
	{
		"You have been wounded beyond repair.  ",
		"You have been disemboweled.  ",
		"You've been mashed, mauled, and spit upon.  (You're dead.)\n",
		"You died!  ",
		"You're a complete failure -- you've died!!\n",
		"You have been dealt a fatal blow!  "
	};

	clear();

	if (strcmp(how, "Stupidity") != 0) {
		if (Player.p_level > 9999.0)
			/* old age */
			addstr("Characters must be retired upon reaching level 10000.  Sorry.");
		else
			if (Player.p_lives > 0)
				/* extra lives */
			{
				addstr("You should be more cautious.  You've been killed.\n");
				printw("You only have %d more chance(s).\n", --Player.p_lives);
				more(3);
				Player.p_energy = Player.p_maxenergy;
				return;
			} else
				if (Player.p_specialtype == SC_VALAR) {
					addstr("You had your chances, but Valar aren't totally\n");
					addstr("immortal.  You are now left to wither and die . . .\n");
					more(3);
					Player.p_brains = Player.p_level / 25.0;
					Player.p_energy = Player.p_maxenergy /= 5.0;
					Player.p_quksilver = Player.p_sword = 0.0;
					Player.p_specialtype = SC_COUNCIL;
					return;
				} else
					if (Player.p_ring.ring_inuse &&
					    (Player.p_ring.ring_type == R_DLREG || Player.p_ring.ring_type == R_NAZREG))
						/* good ring in use - saved
						 * from death */
					{
						mvaddstr(4, 0, "Your ring saved you from death!\n");
						refresh();
						Player.p_ring.ring_type = R_NONE;
						Player.p_energy = Player.p_maxenergy / 12.0 + 1.0;
						if (Player.p_crowns > 0)
							--Player.p_crowns;
						return;
					} else
						if (Player.p_ring.ring_type == R_BAD
						    || Player.p_ring.ring_type == R_SPOILED)
							/* bad ring in
							 * possession; name
							 * idiot after player */
						{
							mvaddstr(4, 0,
							    "Your ring has taken control of you and turned you into a monster!\n");
							fseek(Monstfp, 13L * SZ_MONSTERSTRUCT, SEEK_SET);
							fread((char *) &Curmonster, SZ_MONSTERSTRUCT, 1, Monstfp);
							strcpy(Curmonster.m_name, Player.p_name);
							fseek(Monstfp, 13L * SZ_MONSTERSTRUCT, SEEK_SET);
							fwrite((char *) &Curmonster, SZ_MONSTERSTRUCT, 1, Monstfp);
							fflush(Monstfp);
						}
	}
	enterscore();		/* update score board */

	/* put info in last dead file */
	fp = fopen(_PATH_LASTDEAD, "w");
	fprintf(fp, "%s (%s, run by %s, level %.0f, killed by %s)",
	    Player.p_name, descrtype(&Player, TRUE),
	    Player.p_login, Player.p_level, how);
	fclose(fp);

	/* let other players know */
	fp = fopen(_PATH_MESS, "w");
	fprintf(fp, "%s was killed by %s.", Player.p_name, how);
	fclose(fp);

	freerecord(&Player, Fileloc);

	clear();
	move(10, 0);
	addstr(deathmesg[(int) ROLL(0.0, (double) sizeof(deathmesg) / sizeof(char *))]);
	addstr("Care to give it another try ? ");
	ch = getanswer("NY", FALSE);

	if (ch == 'Y') {
		cleanup(FALSE);
		execl(_PATH_GAMEPROG, "phantasia", "-s",
		    (Wizard ? "-S" : (char *) NULL), 0);
		exit(0);
		/* NOTREACHED */
	}
	cleanup(TRUE);
	/* NOTREACHED */
}

void
writerecord(playerp, place)
	struct player *playerp;
	long    place;
{
	fseek(Playersfp, place, SEEK_SET);
	fwrite((char *) playerp, SZ_PLAYERSTRUCT, 1, Playersfp);
	fflush(Playersfp);
}

double
explevel(experience)
	double  experience;
{
	if (experience < 1.1e7)
		return (floor(pow((experience / 1000.0), 0.4875)));
	else
		return (floor(pow((experience / 1250.0), 0.4865)));
}

void
truncstring(string)
	char   *string;
{
	int     length;		/* length of string */

	length = strlen(string);
	while (string[--length] == ' ')
		string[length] = '\0';
}

void
altercoordinates(xnew, ynew, operation)
	double  xnew;
	double  ynew;
	int     operation;
{
	switch (operation) {
	case A_FORCED:		/* move with no checks */
		break;

	case A_NEAR:		/* pick random coordinates near */
		xnew = Player.p_x + ROLL(1.0, 5.0);
		ynew = Player.p_y - ROLL(1.0, 5.0);
		/* fall through for check */

	case A_SPECIFIC:	/* just move player */
		if (Beyond && fabs(xnew) < D_BEYOND && fabs(ynew) < D_BEYOND)
			/*
			 * cannot move back from point of no return
			 * pick the largest coordinate to remain unchanged
			 */
		{
			if (fabs(xnew) > fabs(ynew))
				xnew = SGN(Player.p_x) * MAX(fabs(Player.p_x), D_BEYOND);
			else
				ynew = SGN(Player.p_y) * MAX(fabs(Player.p_y), D_BEYOND);
		}
		break;

	case A_FAR:		/* pick random coordinates far */
		xnew = Player.p_x + SGN(Player.p_x) * ROLL(50 * Circle, 250 * Circle);
		ynew = Player.p_y + SGN(Player.p_y) * ROLL(50 * Circle, 250 * Circle);
		break;
	}

	/* now set location flags and adjust coordinates */
	Circle = CIRCLE(Player.p_x = floor(xnew), Player.p_y = floor(ynew));

	/* set up flags based upon location */
	Throne = Marsh = Beyond = FALSE;

	if (Player.p_x == 0.0 && Player.p_y == 0.0)
		Throne = TRUE;
	else
		if (Circle < 35 && Circle >= 20)
			Marsh = TRUE;
		else
			if (MAX(fabs(Player.p_x), fabs(Player.p_y)) >= D_BEYOND)
				Beyond = TRUE;

	Changed = TRUE;
}

void
readrecord(playerp, loc)
	struct player *playerp;
	long    loc;
{
	fseek(Playersfp, loc, SEEK_SET);
	fread((char *) playerp, SZ_PLAYERSTRUCT, 1, Playersfp);
}

void
adjuststats()
{
	double  dtemp;		/* for temporary calculations */

	if (explevel(Player.p_experience) > Player.p_level)
		/* move one or more levels */
	{
		movelevel();
		if (Player.p_level > 5.0)
			Timeout = TRUE;
	}
	if (Player.p_specialtype == SC_VALAR)
		/* valar */
		Circle = Player.p_level / 5.0;

	/* calculate effective quickness */
	dtemp = ((Player.p_gold + Player.p_gems / 2.0) - 1000.0) / Statptr->c_goldtote
	    - Player.p_level;
	dtemp = MAX(0.0, dtemp);/* gold slows player down */
	Player.p_speed = Player.p_quickness + Player.p_quksilver - dtemp;

	/* calculate effective strength */
	if (Player.p_poison > 0.0)
		/* poison makes player weaker */
	{
		dtemp = 1.0 - Player.p_poison * Statptr->c_weakness / 800.0;
		dtemp = MAX(0.1, dtemp);
	} else
		dtemp = 1.0;
	Player.p_might = dtemp * Player.p_strength + Player.p_sword;

	/* insure that important things are within limits */
	Player.p_quksilver = MIN(99.0, Player.p_quksilver);
	Player.p_mana = MIN(Player.p_mana,
	    Player.p_level * Statptr->c_maxmana + 1000.0);
	Player.p_brains = MIN(Player.p_brains,
	    Player.p_level * Statptr->c_maxbrains + 200.0);
	Player.p_charms = MIN(Player.p_charms, Player.p_level + 10.0);

	/*
         * some implementations have problems with floating point compare
         * we work around it with this stuff
         */
	Player.p_gold = floor(Player.p_gold) + 0.1;
	Player.p_gems = floor(Player.p_gems) + 0.1;
	Player.p_mana = floor(Player.p_mana) + 0.1;

	if (Player.p_ring.ring_type != R_NONE)
		/* do ring things */
	{
		/* rest to max */
		Player.p_energy = Player.p_maxenergy + Player.p_shield;

		if (Player.p_ring.ring_duration <= 0)
			/* clean up expired rings */
			switch (Player.p_ring.ring_type) {
			case R_BAD:	/* ring drives player crazy */
				Player.p_ring.ring_type = R_SPOILED;
				Player.p_ring.ring_duration = (short) ROLL(10.0, 25.0);
				break;

			case R_NAZREG:	/* ring disappears */
				Player.p_ring.ring_type = R_NONE;
				break;

			case R_SPOILED:	/* ring kills player */
				death("A cursed ring");
				break;

			case R_DLREG:	/* this ring doesn't expire */
				Player.p_ring.ring_duration = 0;
				break;
			}
	}
	if (Player.p_age / N_AGE > Player.p_degenerated)
		/* age player slightly */
	{
		++Player.p_degenerated;
		if (Player.p_quickness > 23.0)
			Player.p_quickness *= 0.99;
		Player.p_strength *= 0.97;
		Player.p_brains *= 0.95;
		Player.p_magiclvl *= 0.97;
		Player.p_maxenergy *= 0.95;
		Player.p_quksilver *= 0.95;
		Player.p_sword *= 0.93;
		Player.p_shield *= 0.93;
	}
}

void
initplayer(playerp)
	struct player *playerp;
{
	playerp->p_experience =
	    playerp->p_level =
	    playerp->p_strength =
	    playerp->p_sword =
	    playerp->p_might =
	    playerp->p_energy =
	    playerp->p_maxenergy =
	    playerp->p_shield =
	    playerp->p_quickness =
	    playerp->p_quksilver =
	    playerp->p_speed =
	    playerp->p_magiclvl =
	    playerp->p_mana =
	    playerp->p_brains =
	    playerp->p_poison =
	    playerp->p_gems =
	    playerp->p_sin =
	    playerp->p_1scratch =
	    playerp->p_2scratch = 0.0;

	playerp->p_gold = ROLL(50.0, 75.0) + 0.1;	/* give some gold */

	playerp->p_x = ROLL(-125.0, 251.0);
	playerp->p_y = ROLL(-125.0, 251.0);	/* give random x, y */

	/* clear ring */
	playerp->p_ring.ring_type = R_NONE;
	playerp->p_ring.ring_duration = 0;
	playerp->p_ring.ring_inuse = FALSE;

	playerp->p_age = 0L;

	playerp->p_degenerated = 1;	/* don't degenerate initially */

	playerp->p_type = C_FIGHTER;	/* default */
	playerp->p_specialtype = SC_NONE;
	playerp->p_lives =
	    playerp->p_crowns =
	    playerp->p_charms =
	    playerp->p_amulets =
	    playerp->p_holywater =
	    playerp->p_lastused = 0;
	playerp->p_status = S_NOTUSED;
	playerp->p_tampered = T_OFF;
	playerp->p_istat = I_OFF;

	playerp->p_palantir =
	    playerp->p_blessing =
	    playerp->p_virgin =
	    playerp->p_blindness = FALSE;

	playerp->p_name[0] =
	    playerp->p_password[0] =
	    playerp->p_login[0] = '\0';
}

void
readmessage()
{
	move(3, 0);
	clrtoeol();
	fseek(Messagefp, 0L, SEEK_SET);
	if (fgets(Databuf, SZ_DATABUF, Messagefp) != NULL)
		addstr(Databuf);
}

void
error(whichfile)
	const char   *whichfile;
{
	int     (*funcp)(const char *,...);

	if (Windows) {
		funcp = printw;
		clear();
	} else
		funcp = printf;

	(*funcp) ("An unrecoverable error has occurred reading %s.  (errno = %d)\n", whichfile, errno);
	(*funcp) ("Please run 'setup' to determine the problem.\n");
	cleanup(TRUE);
	/* NOTREACHED */
}

double
distance(x1, x2, y1, y2)
	double  x1, x2, y1, y2;
{
	double  deltax, deltay;

	deltax = x1 - x2;
	deltay = y1 - y2;
	return (sqrt(deltax * deltax + deltay * deltay));
}

void
ill_sig(whichsig)
	int     whichsig;
{
	clear();
	if (!(whichsig == SIGINT || whichsig == SIGQUIT))
		printw("Error: caught signal # %d.\n", whichsig);
	cleanup(TRUE);
	/* NOTREACHED */
}

const char *
descrstatus(playerp)
	struct player *playerp;
{
	switch (playerp->p_status) {
	case S_PLAYING:
		if (playerp->p_energy < 0.2 * (playerp->p_maxenergy + playerp->p_shield))
			return ("Low Energy");
		else
			if (playerp->p_blindness)
				return ("Blind");
			else
				return ("In game");

	case S_CLOAKED:
		return ("Cloaked");

	case S_INBATTLE:
		return ("In Battle");

	case S_MONSTER:
		return ("Encounter");

	case S_TRADING:
		return ("Trading");

	case S_OFF:
		return ("Off");

	case S_HUNGUP:
		return ("Hung up");

	default:
		return ("");
	}
}

double
drandom()
{
	if (sizeof(int) != 2)
		/* use only low bits */
		return ((double) (random() & 0x7fff) / 32768.0);
	else
		return ((double) random() / 32768.0);
}

void
collecttaxes(gold, gems)
	double  gold;
	double  gems;
{
	FILE   *fp;		/* to update Goldfile */
	double  dtemp;		/* for temporary calculations */
	double  taxes;		/* tax liability */

	/* add to cache */
	Player.p_gold += gold;
	Player.p_gems += gems;

	/* calculate tax liability */
	taxes = N_TAXAMOUNT / 100.0 * (N_GEMVALUE * gems + gold);

	if (Player.p_gold < taxes)
		/* not enough gold to pay taxes, must convert some gems to
		 * gold */
	{
		dtemp = floor(taxes / N_GEMVALUE + 1.0);	/* number of gems to
								 * convert */

		if (Player.p_gems >= dtemp)
			/* player has enough to convert */
		{
			Player.p_gems -= dtemp;
			Player.p_gold += dtemp * N_GEMVALUE;
		} else
			/* take everything; this should never happen */
		{
			Player.p_gold += Player.p_gems * N_GEMVALUE;
			Player.p_gems = 0.0;
			taxes = Player.p_gold;
		}
	}
	Player.p_gold -= taxes;

	if ((fp = fopen(_PATH_GOLD, "r+")) != NULL)
		/* update taxes */
	{
		dtemp = 0.0;
		fread((char *) &dtemp, sizeof(double), 1, fp);
		dtemp += floor(taxes);
		fseek(fp, 0L, SEEK_SET);
		fwrite((char *) &dtemp, sizeof(double), 1, fp);
		fclose(fp);
	}
}
