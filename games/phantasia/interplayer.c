/*	$NetBSD: interplayer.c,v 1.11 2009/08/12 08:21:41 dholland Exp $	*/

/*
 * interplayer.c - player to player routines for Phantasia
 */

#include "include.h"
#undef bool
#include <curses.h>

static long allocvoid(void);
static void battleplayer(long);
static void myturn(void);
static void tampered(int, double, double);

void
checkbattle(void)
{
	long    foeloc = 0L;	/* location in file of person to fight */

	Users = 0;
	fseek(Playersfp, 0L, SEEK_SET);

	while (fread((char *) &Other, SZ_PLAYERSTRUCT, 1, Playersfp) == 1) {
		if (Other.p_status != S_OFF
		    && Other.p_status != S_NOTUSED
		    && Other.p_status != S_HUNGUP
		    && (Other.p_status != S_CLOAKED || Other.p_specialtype != SC_VALAR))
			/* player is on and not a cloaked valar */
		{
			++Users;

			if (Player.p_x == Other.p_x
			    && Player.p_y == Other.p_y
			/* same coordinates */
			    && foeloc != Fileloc
			/* not self */
			    && Player.p_status == S_PLAYING
			    && (Other.p_status == S_PLAYING || Other.p_status == S_INBATTLE)
			/* both are playing */
			    && Other.p_specialtype != SC_VALAR
			    && Player.p_specialtype != SC_VALAR)
				/* neither is valar */
			{
				battleplayer(foeloc);
				return;
			}
		}
		foeloc += SZ_PLAYERSTRUCT;
	}
}

static void
battleplayer(long foeplace)
{
	double  dtemp;		/* for temporary calculations */
	double  oldhits = 0.0;	/* previous damage inflicted by foe */
	int     loop;		/* for timing out */
	int     ch;		/* input */
	short   oldtampered;	/* old value of foe's p_tampered */

	Lines = 8;
	Luckout = FALSE;
	mvaddstr(4, 0, "Preparing for battle!\n");
	refresh();

#ifdef SYS5
	flushinp();
#endif

	/* set up variables, file, etc. */
	Player.p_status = S_INBATTLE;
	Shield = Player.p_energy;

	/* if p_tampered is not 0, someone else may try to change it (king,
	 * etc.) */
	Player.p_tampered = oldtampered = 1;
	Player.p_1scratch = 0.0;
	Player.p_istat = I_OFF;

	readrecord(&Other, foeplace);
	if (fabs(Player.p_level - Other.p_level) > 20.0)
		/* see if players are greatly mismatched */
	{
		dtemp = (Player.p_level - Other.p_level) / MAX(Player.p_level, Other.p_level);
		if (dtemp < -0.5)
			/* foe outweighs this one */
			Player.p_speed *= 2.0;
	}
	writerecord(&Player, Fileloc);	/* write out all our info */

	if (Player.p_blindness)
		Enemyname = "someone";
	else
		Enemyname = Other.p_name;

	mvprintw(6, 0, "You have encountered %s   Level: %.0f\n", Enemyname, Other.p_level);
	refresh();

	for (loop = 0; Other.p_status != S_INBATTLE && loop < 30; ++loop)
		/* wait for foe to respond */
	{
		readrecord(&Other, foeplace);
		sleep(1);
	}

	if (Other.p_status != S_INBATTLE)
		/* foe did not respond */
	{
		mvprintw(5, 0, "%s is not responding.\n", Enemyname);
		goto LEAVE;
	}
	/* else, we are ready to battle */

	move(4, 0);
	clrtoeol();

	/*
         * determine who is first master
         * if neither player is faster, check level
         * if neither level is greater, battle is not allowed
         * (this should never happen, but we have to handle it)
         */
	if (Player.p_speed > Other.p_speed)
		Foestrikes = FALSE;
	else
		if (Other.p_speed > Player.p_speed)
			Foestrikes = TRUE;
		else
			if (Player.p_level > Other.p_level)
				Foestrikes = FALSE;
			else
				if (Other.p_level > Player.p_level)
					Foestrikes = TRUE;
				else
					/* no one is faster */
				{
					printw("You can't fight %s yet.", Enemyname);
					goto LEAVE;
				}

	for (;;) {
		displaystats();
		readmessage();
		mvprintw(1, 26, "%20.0f", Shield);	/* overprint energy */

		if (!Foestrikes)
			/* take action against foe */
			myturn();
		else
			/* wait for foe to take action */
		{
			mvaddstr(4, 0, "Waiting...\n");
			clrtoeol();
			refresh();

			for (loop = 0; loop < 20; ++loop)
				/* wait for foe to act */
			{
				readrecord(&Other, foeplace);
				if (Other.p_1scratch != oldhits)
					/* p_1scratch changes to indicate
					 * action */
					break;
				else
					/* wait and try again */
				{
					sleep(1);
					addch('.');
					refresh();
				}
			}

			if (Other.p_1scratch == oldhits) {
				/* timeout */
				mvaddstr(22, 0, "Timeout: waiting for response.  Do you want to wait ? ");
				ch = getanswer("NY", FALSE);
				move(22, 0);
				clrtobot();
				if (ch == 'Y')
					continue;
				else
					break;
			} else
				/* foe took action */
			{
				switch (Other.p_istat) {
				case I_RAN:	/* foe ran away */
					mvprintw(Lines++, 0, "%s ran away!", Enemyname);
					break;

				case I_STUCK:	/* foe tried to run, but
						 * couldn't */
					mvprintw(Lines++, 0, "%s tried to run away.", Enemyname);
					break;

				case I_BLEWIT:	/* foe tried to luckout, but
						 * didn't */
					mvprintw(Lines++, 0, "%s tried to luckout!", Enemyname);
					break;

				default:
					dtemp = Other.p_1scratch - oldhits;
					mvprintw(Lines++, 0, "%s hit you %.0f times!", Enemyname, dtemp);
					Shield -= dtemp;
					break;
				}

				oldhits = Other.p_1scratch;	/* keep track of old
								 * hits */

				if (Other.p_tampered != oldtampered)
					/* p_tampered changes to relinquish
					 * turn */
				{
					oldtampered = Other.p_tampered;
					Foestrikes = FALSE;
				}
			}
		}

		/* decide what happens next */
		refresh();
		if (Lines > LINES - 2) {
			more(Lines);
			move(Lines = 8, 0);
			clrtobot();
		}
		if (Other.p_istat == I_KILLED || Shield < 0.0)
			/* we died */
		{
			Shield = -2.0;	/* insure this value is negative */
			break;
		}
		if (Player.p_istat == I_KILLED)
			/* we killed foe; award treasre */
		{
			mvprintw(Lines++, 0, "You killed %s!", Enemyname);
			Player.p_experience += Other.p_experience;
			Player.p_crowns += (Player.p_level < 1000.0) ? Other.p_crowns : 0;
			Player.p_amulets += Other.p_amulets;
			Player.p_charms += Other.p_charms;
			collecttaxes(Other.p_gold, Other.p_gems);
			Player.p_sword = MAX(Player.p_sword, Other.p_sword);
			Player.p_shield = MAX(Player.p_shield, Other.p_shield);
			Player.p_quksilver = MAX(Player.p_quksilver, Other.p_quksilver);
			if (Other.p_virgin && !Player.p_virgin) {
				mvaddstr(Lines++, 0, "You have rescued a virgin.  Will you be honorable ? ");
				if ((ch = getanswer("YN", FALSE)) == 'Y')
					Player.p_virgin = TRUE;
				else {
					++Player.p_sin;
					Player.p_experience += 8000.0;
				}
			}
			sleep(3);	/* give other person time to die */
			break;
		} else
			if (Player.p_istat == I_RAN || Other.p_istat == I_RAN)
				/* either player ran away */
				break;
	}

LEAVE:
	/* clean up things and leave */
	writerecord(&Player, Fileloc);	/* update a final time */
	altercoordinates(0.0, 0.0, A_NEAR);	/* move away from battle site */
	Player.p_energy = Shield;	/* set energy to actual value */
	Player.p_tampered = T_OFF;	/* clear p_tampered */

	more(Lines);		/* pause */

	move(4, 0);
	clrtobot();		/* clear bottom area of screen */

	if (Player.p_energy < 0.0)
		/* we are dead */
		death("Interterminal battle");
}

static void
myturn(void)
{
	double  dtemp;		/* for temporary calculations */
	int     ch;		/* input */

	mvaddstr(7, 0, "1:Fight  2:Run Away!  3:Power Blast  ");
	if (Luckout)
		clrtoeol();
	else
		addstr("4:Luckout  ");

	ch = inputoption();
	move(Lines = 8, 0);
	clrtobot();

	switch (ch) {
	default:		/* fight */
		dtemp = ROLL(2.0, Player.p_might);
HIT:
		mvprintw(Lines++, 0, "You hit %s %.0f times!", Enemyname, dtemp);
		Player.p_sin += 0.5;
		Player.p_1scratch += dtemp;
		Player.p_istat = I_OFF;
		break;

	case '2':		/* run away */
		Player.p_1scratch -= 1.0;	/* change this to indicate
						 * action */
		if (drandom() > 0.25) {
			mvaddstr(Lines++, 0, "You got away!");
			Player.p_istat = I_RAN;
		} else {
			mvprintw(Lines++, 0, "%s is still after you!", Enemyname);
			Player.p_istat = I_STUCK;
		}
		break;

	case '3':		/* power blast */
		dtemp = MIN(Player.p_mana, Player.p_level * 5.0);
		Player.p_mana -= dtemp;
		dtemp *= (drandom() + 0.5) * Player.p_magiclvl * 0.2 + 2.0;
		mvprintw(Lines++, 0, "You blasted %s !", Enemyname);
		goto HIT;

	case '4':		/* luckout */
		if (Luckout || drandom() > 0.1) {
			if (Luckout)
				mvaddstr(Lines++, 0, "You already tried that!");
			else {
				mvaddstr(Lines++, 0, "Not this time . . .");
				Luckout = TRUE;
			}

			Player.p_1scratch -= 1.0;
			Player.p_istat = I_BLEWIT;
		} else {
			mvaddstr(Lines++, 0, "You just lucked out!");
			Player.p_1scratch = Other.p_energy * 1.1;
		}
		break;
	}

	refresh();
	Player.p_1scratch = floor(Player.p_1scratch);	/* clean up any mess */

	if (Player.p_1scratch > Other.p_energy)
		Player.p_istat = I_KILLED;
	else
		if (drandom() * Player.p_speed < drandom() * Other.p_speed)
			/* relinquish control */
		{
			++Player.p_tampered;
			Foestrikes = TRUE;
		}
	writerecord(&Player, Fileloc);	/* let foe know what we did */
}

void
checktampered(void)
{
	long    loc = 0L;	/* location in energy void file */

	/* first check for energy voids */
	fseek(Energyvoidfp, 0L, SEEK_SET);
	while (fread((char *) &Enrgyvoid, SZ_VOIDSTRUCT, 1, Energyvoidfp) == 1)
		if (Enrgyvoid.ev_active
		    && Enrgyvoid.ev_x == Player.p_x
		    && Enrgyvoid.ev_y == Player.p_y)
			/* sitting on one */
		{
			if (loc > 0L)
				/* not the holy grail; inactivate energy void */
			{
				Enrgyvoid.ev_active = FALSE;
				writevoid(&Enrgyvoid, loc);
				tampered(T_NRGVOID, 0.0, 0.0);
			} else
				if (Player.p_status != S_CLOAKED)
					/* holy grail */
					tampered(T_GRAIL, 0.0, 0.0);
			break;
		} else
			loc += SZ_VOIDSTRUCT;

	/* now check for other things */
	readrecord(&Other, Fileloc);
	if (Other.p_tampered != T_OFF)
		tampered(Other.p_tampered, Other.p_1scratch, Other.p_2scratch);
}

static void
tampered(int what, double arg1, double arg2)
{
	long    loc;		/* location in file of other players */

	Changed = TRUE;
	move(4, 0);

	Player.p_tampered = T_OFF;	/* no longer tampered with */

	switch (what) {
	case T_NRGVOID:
		addstr("You've hit an energy void !\n");
		Player.p_mana /= 3.0;
		Player.p_energy /= 2.0;
		Player.p_gold = floor(Player.p_gold / 1.25) + 0.1;
		altercoordinates(0.0, 0.0, A_NEAR);
		break;

	case T_TRANSPORT:
		addstr("The king transported you !  ");
		if (Player.p_charms > 0) {
			addstr("But your charm saved you. . .\n");
			--Player.p_charms;
		} else {
			altercoordinates(0.0, 0.0, A_FAR);
			addch('\n');
		}
		break;

	case T_BESTOW:
		printw("The king has bestowed %.0f gold pieces on you !\n", arg1);
		Player.p_gold += arg1;
		break;

	case T_CURSED:
		addstr("You've been cursed !  ");
		if (Player.p_blessing) {
			addstr("But your blessing saved you. . .\n");
			Player.p_blessing = FALSE;
		} else {
			addch('\n');
			Player.p_poison += 2.0;
			Player.p_energy = 10.0;
			Player.p_maxenergy *= 0.95;
			Player.p_status = S_PLAYING;	/* no longer cloaked */
		}
		break;

	case T_VAPORIZED:
		addstr("You have been vaporized!\n");
		more(7);
		death("Vaporization");
		break;

	case T_MONSTER:
		addstr("The Valar zapped you with a monster!\n");
		more(7);
		encounter((int) arg1);
		return;

	case T_BLESSED:
		addstr("The Valar has blessed you!\n");
		Player.p_energy = (Player.p_maxenergy *= 1.05) + Player.p_shield;
		Player.p_mana += 500.0;
		Player.p_strength += 0.5;
		Player.p_brains += 0.5;
		Player.p_magiclvl += 0.5;
		Player.p_poison = MIN(0.5, Player.p_poison);
		break;

	case T_RELOCATE:
		addstr("You've been relocated. . .\n");
		altercoordinates(arg1, arg2, A_FORCED);
		break;

	case T_HEAL:
		addstr("You've been healed!\n");
		Player.p_poison -= 0.25;
		Player.p_energy = Player.p_maxenergy + Player.p_shield;
		break;

	case T_EXVALAR:
		addstr("You are no longer Valar!\n");
		Player.p_specialtype = SC_COUNCIL;
		break;

	case T_GRAIL:
		addstr("You have found The Holy Grail!!\n");
		if (Player.p_specialtype < SC_COUNCIL)
			/* must be council of wise to behold grail */
		{
			addstr("However, you are not experienced enough to behold it.\n");
			Player.p_sin *= Player.p_sin;
			Player.p_mana += 1000;
		} else
			if (Player.p_specialtype == SC_VALAR
			    || Player.p_specialtype == SC_EXVALAR) {
				addstr("You have made it to the position of Valar once already.\n");
				addstr("The Grail is of no more use to you now.\n");
			} else {
				addstr("It is now time to see if you are worthy to behold it. . .\n");
				refresh();
				sleep(4);

				if (drandom() / 2.0 < Player.p_sin) {
					addstr("You have failed!\n");
					Player.p_strength =
					    Player.p_mana =
					    Player.p_energy =
					    Player.p_maxenergy =
					    Player.p_magiclvl =
					    Player.p_brains =
					    Player.p_experience =
					    Player.p_quickness = 1.0;

					altercoordinates(1.0, 1.0, A_FORCED);
					Player.p_level = 0.0;
				} else {
					addstr("You made to position of Valar!\n");
					Player.p_specialtype = SC_VALAR;
					Player.p_lives = 5;
					fseek(Playersfp, 0L, SEEK_SET);
					loc = 0L;
					while (fread((char *) &Other, SZ_PLAYERSTRUCT, 1, Playersfp) == 1)
						/* search for existing valar */
						if (Other.p_specialtype == SC_VALAR
						    && Other.p_status != S_NOTUSED)
							/* found old valar */
						{
							Other.p_tampered = T_EXVALAR;
							writerecord(&Other, loc);
							break;
						} else
							loc += SZ_PLAYERSTRUCT;
				}
			}

		/* move grail to new location */
		Enrgyvoid.ev_active = TRUE;
		Enrgyvoid.ev_x = ROLL(-1.0e6, 2.0e6);
		Enrgyvoid.ev_y = ROLL(-1.0e6, 2.0e6);
		writevoid(&Enrgyvoid, 0L);
		break;
	}
	refresh();
	sleep(2);
}

void
userlist(phbool ingameflag)
{
	int     numusers = 0;	/* number of users on file */

	if (ingameflag && Player.p_blindness) {
		mvaddstr(8, 0, "You cannot see anyone.\n");
		return;
	}
	fseek(Playersfp, 0L, SEEK_SET);
	mvaddstr(8, 0,
	    "Name                         X         Y    Lvl Type Login    Status\n");

	while (fread((char *) &Other, SZ_PLAYERSTRUCT, 1, Playersfp) == 1) {
		if (Other.p_status == S_NOTUSED
		/* record is unused */
		    || (Other.p_specialtype == SC_VALAR && Other.p_status == S_CLOAKED))
			/* cloaked valar */
		{
			if (!Wizard)
				/* wizard can see everything on file */
				continue;
		}
		++numusers;

		if (ingameflag &&
		/* must be playing for the rest of these conditions */
		    (Player.p_specialtype >= SC_KING
		/* kings and higher can see others */
			|| Other.p_specialtype >= SC_KING
		/* kings and higher can be seen by others */
			|| Circle >= CIRCLE(Other.p_x, Other.p_y)
		/* those nearer the origin can be seen */
			|| Player.p_palantir)
		/* palantir enables one to see others */
		    && (Other.p_status != S_CLOAKED
			|| (Player.p_specialtype == SC_VALAR && Player.p_palantir))
		/* not cloaked; valar can see through cloak with a palantir */
		    && Other.p_specialtype != SC_VALAR)
			/* not a valar */
			/* coordinates should be printed */
			printw("%-20s  %8.0f  %8.0f ",
			    Other.p_name, Other.p_x, Other.p_y);
		else
			/* cannot see player's coordinates */
			printw("%-20s %19.19s ",
			    Other.p_name, descrlocation(&Other, TRUE));

		printw("%6.0f %s  %-9.9s%s\n", Other.p_level, descrtype(&Other, TRUE),
		    Other.p_login, descrstatus(&Other));

		if ((numusers % (LINES - 10)) == 0) {
			more(LINES - 1);
			move(9, 0);
			clrtobot();
		}
	}

	printw("Total players on file = %d\n", numusers);
	refresh();
}

void
throneroom(void)
{
	FILE   *fp;		/* to clear energy voids */
	long    loc = 0L;	/* location of old king in player file */

	if (Player.p_specialtype < SC_KING)
		/* not already king -- assumes crown */
	{
		fseek(Playersfp, 0L, SEEK_SET);
		while (fread((char *) &Other, SZ_PLAYERSTRUCT, 1, Playersfp) == 1)
			if (Other.p_specialtype == SC_KING && Other.p_status != S_NOTUSED)
				/* found old king */
			{
				if (Other.p_status != S_OFF)
					/* old king is playing */
				{
					mvaddstr(4, 0, "The king is playing, so you cannot steal his throne\n");
					altercoordinates(0.0, 0.0, A_NEAR);
					move(6, 0);
					return;
				} else
					/* old king is not playing - remove
					 * him/her */
				{
					Other.p_specialtype = SC_NONE;
					if (Other.p_crowns)
						--Other.p_crowns;
					writerecord(&Other, loc);
					break;
				}
			} else
				loc += SZ_PLAYERSTRUCT;

		/* make player new king */
		Changed = TRUE;
		Player.p_specialtype = SC_KING;
		mvaddstr(4, 0, "You have become king!\n");

		/* let everyone else know */
		fp = fopen(_PATH_MESS, "w");
		fprintf(fp, "All hail the new king!");
		fclose(fp);

		/* clear all energy voids; retain location of holy grail */
		fseek(Energyvoidfp, 0L, SEEK_SET);
		fread((char *) &Enrgyvoid, SZ_VOIDSTRUCT, 1, Energyvoidfp);
		fp = fopen(_PATH_VOID, "w");
		fwrite((char *) &Enrgyvoid, SZ_VOIDSTRUCT, 1, fp);
		fclose(fp);
	}
	mvaddstr(6, 0, "0:Decree  ");
}

void
dotampered(void)
{
	short   tamper;		/* value for tampering with other players */
	const char   *option;		/* pointer to option description */
	double  temp1 = 0.0, temp2 = 0.0;	/* other tampering values */
	int     ch;		/* input */
	long    loc;		/* location in energy void file */
	FILE   *fp;		/* for opening gold file */

	move(6, 0);
	clrtoeol();
	if (Player.p_specialtype < SC_COUNCIL && !Wizard)
		/* king options */
	{
		addstr("1:Transport  2:Curse  3:Energy Void  4:Bestow  5:Collect Taxes  ");

		ch = getanswer(" ", TRUE);
		move(6, 0);
		clrtoeol();
		move(4, 0);
		switch (ch) {
		case '1':	/* transport someone */
			tamper = T_TRANSPORT;
			option = "transport";
			break;

		case '2':	/* curse another */
			tamper = T_CURSED;
			option = "curse";
			break;

		case '3':	/* create energy void */
			if ((loc = allocvoid()) > 20L * (long)SZ_VOIDSTRUCT)
				/* can only have 20 void active at once */
				mvaddstr(5, 0, "Sorry, void creation limit reached.\n");
			else {
				addstr("Enter the X Y coordinates of void ? ");
				getstring(Databuf, SZ_DATABUF);
				sscanf(Databuf, "%lf %lf", &temp1, &temp2);
				Enrgyvoid.ev_x = floor(temp1);
				Enrgyvoid.ev_y = floor(temp2);
				Enrgyvoid.ev_active = TRUE;
				writevoid(&Enrgyvoid, loc);
				mvaddstr(5, 0, "It is done.\n");
			}
			return;

		case '4':	/* bestow gold to subject */
			tamper = T_BESTOW;
			addstr("How much gold to bestow ? ");
			temp1 = infloat();
			if (temp1 > Player.p_gold || temp1 < 0) {
				mvaddstr(5, 0, "You don't have that !\n");
				return;
			}
			/* adjust gold after we are sure it will be given to
			 * someone */
			option = "give gold to";
			break;

		case '5':	/* collect accumulated taxes */
			if ((fp = fopen(_PATH_GOLD, "r+")) != NULL)
				/* collect taxes */
			{
				fread((char *) &temp1, sizeof(double), 1, fp);
				fseek(fp, 0L, SEEK_SET);
				/* clear out value */
				temp2 = 0.0;
				fwrite((char *) &temp2, sizeof(double), 1, fp);
				fclose(fp);
			}
			mvprintw(4, 0, "You have collected %.0f in gold.\n", temp1);
			Player.p_gold += floor(temp1);
			return;

		default:
			return;
		}
		/* end of king options */
	} else
		/* council of wise, valar, wizard options */
	{
		addstr("1:Heal  ");
		if (Player.p_palantir || Wizard)
			addstr("2:Seek Grail  ");
		if (Player.p_specialtype == SC_VALAR || Wizard)
			addstr("3:Throw Monster  4:Relocate  5:Bless  ");
		if (Wizard)
			addstr("6:Vaporize  ");

		ch = getanswer(" ", TRUE);
		if (!Wizard) {
			if (ch > '2' && Player.p_specialtype != SC_VALAR) {
				ILLCMD();
				return;
			}
			if (Player.p_mana < MM_INTERVENE) {
				mvaddstr(5, 0, "No mana left.\n");
				return;
			} else
				Player.p_mana -= MM_INTERVENE;
		}
		switch (ch) {
		case '1':	/* heal another */
			tamper = T_HEAL;
			option = "heal";
			break;

		case '2':	/* seek grail */
			if (Player.p_palantir)
				/* need a palantir to seek */
			{
				fseek(Energyvoidfp, 0L, SEEK_SET);
				fread((char *) &Enrgyvoid, SZ_VOIDSTRUCT, 1, Energyvoidfp);
				temp1 = distance(Player.p_x, Enrgyvoid.ev_x, Player.p_y, Enrgyvoid.ev_y);
				temp1 += ROLL(-temp1 / 10.0, temp1 / 5.0);	/* add some error */
				mvprintw(5, 0, "The palantir says the Grail is about %.0f away.\n", temp1);
			} else
				/* no palantir */
				mvaddstr(5, 0, "You need a palantir to seek the Grail.\n");
			return;

		case '3':	/* lob monster at someone */
			mvaddstr(4, 0, "Which monster [0-99] ? ");
			temp1 = infloat();
			temp1 = MAX(0.0, MIN(99.0, temp1));
			tamper = T_MONSTER;
			option = "throw a monster at";
			break;

		case '4':	/* move another player */
			mvaddstr(4, 0, "New X Y coordinates ? ");
			getstring(Databuf, SZ_DATABUF);
			sscanf(Databuf, "%lf %lf", &temp1, &temp2);
			tamper = T_RELOCATE;
			option = "relocate";
			break;

		case '5':	/* bless a player */
			tamper = T_BLESSED;
			option = "bless";
			break;

		case '6':	/* kill off a player */
			if (Wizard) {
				tamper = T_VAPORIZED;
				option = "vaporize";
				break;
			} else
				return;

		default:
			return;
		}

		/* adjust age after we are sure intervention will be done */
		/* end of valar, etc. options */
	}

	for (;;)
		/* prompt for player to affect */
	{
		mvprintw(4, 0, "Who do you want to %s ? ", option);
		getstring(Databuf, SZ_DATABUF);
		truncstring(Databuf);

		if (Databuf[0] == '\0')
			userlist(TRUE);
		else
			break;
	}

	if (strcmp(Player.p_name, Databuf) != 0)
		/* name other than self */
	{
		if ((loc = findname(Databuf, &Other)) >= 0L) {
			if (Other.p_tampered != T_OFF) {
				mvaddstr(5, 0, "That person has something pending already.\n");
				return;
			} else {
				if (tamper == T_RELOCATE
				    && CIRCLE(temp1, temp2) < CIRCLE(Other.p_x, Other.p_y)
				    && !Wizard)
					mvaddstr(5, 0, "Cannot move someone closer to the Lord's Chamber.\n");
				else {
					if (tamper == T_BESTOW)
						Player.p_gold -= floor(temp1);
					if (!Wizard && (tamper == T_HEAL || tamper == T_MONSTER ||
						tamper == T_RELOCATE || tamper == T_BLESSED))
						Player.p_age += N_AGE;	/* age penalty */
					Other.p_tampered = tamper;
					Other.p_1scratch = floor(temp1);
					Other.p_2scratch = floor(temp2);
					writerecord(&Other, loc);
					mvaddstr(5, 0, "It is done.\n");
				}
				return;
			}
		} else
			/* player not found */
			mvaddstr(5, 0, "There is no one by that name.\n");
	} else
		/* self */
		mvaddstr(5, 0, "You may not do it to yourself!\n");
}

void
writevoid(struct energyvoid *vp, long loc)
{

	fseek(Energyvoidfp, loc, SEEK_SET);
	fwrite((char *) vp, SZ_VOIDSTRUCT, 1, Energyvoidfp);
	fflush(Energyvoidfp);
	fseek(Energyvoidfp, 0L, SEEK_SET);
}

static long
allocvoid(void)
{
	long    loc = 0L;	/* location of new energy void */

	fseek(Energyvoidfp, 0L, SEEK_SET);
	while (fread((char *) &Enrgyvoid, SZ_VOIDSTRUCT, 1, Energyvoidfp) == 1)
		if (Enrgyvoid.ev_active)
			loc += SZ_VOIDSTRUCT;
		else
			break;

	return (loc);
}
