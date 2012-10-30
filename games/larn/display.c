/*	$NetBSD: display.c,v 1.9.6.1 2012/10/30 18:58:24 yamt Exp $	*/

/* display.c		Larn is copyrighted 1986 by Noah Morgan. */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: display.c,v 1.9.6.1 2012/10/30 18:58:24 yamt Exp $");
#endif /* not lint */

#include "header.h"
#include "extern.h"

#define makecode(_a,_b,_c) (((_a)<<16) + ((_b)<<8) + (_c))

static void bot_hpx(void);
static void bot_spellx(void);
static void botside(void);
static void botsub(int, const char *);
static void seepage(void);

static int      minx, maxx, miny, maxy, k, m;
static char     bot1f = 0, bot2f = 0, bot3f = 0;
static char always = 0;
/*
	bottomline()

	now for the bottom line of the display
 */
void
bottomline(void)
{
	recalc();
	bot1f = 1;
}

void
bottomhp(void)
{
	bot2f = 1;
}

void
bottomspell(void)
{
	bot3f = 1;
}

void
bottomdo(void)
{
	if (bot1f) {
		bot3f = bot1f = bot2f = 0;
		bot_linex();
		return;
	}
	if (bot2f) {
		bot2f = 0;
		bot_hpx();
	}
	if (bot3f) {
		bot3f = 0;
		bot_spellx();
	}
}

void
bot_linex(void)
{
	int    i;
	if (cbak[SPELLS] <= -50 || (always)) {
		cursor(1, 18);
		if (c[SPELLMAX] > 99)
			lprintf("Spells:%3ld(%3ld)", (long) c[SPELLS], (long) c[SPELLMAX]);
		else
			lprintf("Spells:%3ld(%2ld) ", (long) c[SPELLS], (long) c[SPELLMAX]);
		lprintf(" AC: %-3ld  WC: %-3ld  Level", (long) c[AC], (long) c[WCLASS]);
		if (c[LEVEL] > 99)
			lprintf("%3ld", (long) c[LEVEL]);
		else
			lprintf(" %-2ld", (long) c[LEVEL]);
		lprintf(" Exp: %-9ld %s\n", (long) c[EXPERIENCE], class[c[LEVEL] - 1]);
		lprintf("HP: %3ld(%3ld) STR=%-2ld INT=%-2ld ",
			(long) c[HP], (long) c[HPMAX], (long) (c[STRENGTH] + c[STREXTRA]), (long) c[INTELLIGENCE]);
		lprintf("WIS=%-2ld CON=%-2ld DEX=%-2ld CHA=%-2ld LV:",
			(long) c[WISDOM], (long) c[CONSTITUTION], (long) c[DEXTERITY], (long) c[CHARISMA]);

		if ((level == 0) || (wizard))
			c[TELEFLAG] = 0;
		if (c[TELEFLAG])
			lprcat(" ?");
		else
			lprcat(levelname[level]);
		lprintf("  Gold: %-6ld", (long) c[GOLD]);
		always = 1;
		botside();
		c[TMP] = c[STRENGTH] + c[STREXTRA];
		for (i = 0; i < 100; i++)
			cbak[i] = c[i];
		return;
	}
	botsub(makecode(SPELLS, 8, 18), "%3ld");
	if (c[SPELLMAX] > 99)
		botsub(makecode(SPELLMAX, 12, 18), "%3ld)");
	else
		botsub(makecode(SPELLMAX, 12, 18), "%2ld) ");
	botsub(makecode(HP, 5, 19), "%3ld");
	botsub(makecode(HPMAX, 9, 19), "%3ld");
	botsub(makecode(AC, 21, 18), "%-3ld");
	botsub(makecode(WCLASS, 30, 18), "%-3ld");
	botsub(makecode(EXPERIENCE, 49, 18), "%-9ld");
	if (c[LEVEL] != cbak[LEVEL]) {
		cursor(59, 18);
		lprcat(class[c[LEVEL] - 1]);
	}
	if (c[LEVEL] > 99)
		botsub(makecode(LEVEL, 40, 18), "%3ld");
	else
		botsub(makecode(LEVEL, 40, 18), " %-2ld");
	c[TMP] = c[STRENGTH] + c[STREXTRA];
	botsub(makecode(TMP, 18, 19), "%-2ld");
	botsub(makecode(INTELLIGENCE, 25, 19), "%-2ld");
	botsub(makecode(WISDOM, 32, 19), "%-2ld");
	botsub(makecode(CONSTITUTION, 39, 19), "%-2ld");
	botsub(makecode(DEXTERITY, 46, 19), "%-2ld");
	botsub(makecode(CHARISMA, 53, 19), "%-2ld");
	if ((level != cbak[CAVELEVEL]) || (c[TELEFLAG] != cbak[TELEFLAG])) {
		if ((level == 0) || (wizard))
			c[TELEFLAG] = 0;
		cbak[TELEFLAG] = c[TELEFLAG];
		cbak[CAVELEVEL] = level;
		cursor(59, 19);
		if (c[TELEFLAG])
			lprcat(" ?");
		else
			lprcat(levelname[level]);
	}
	botsub(makecode(GOLD, 69, 19), "%-6ld");
	botside();
}

/*
	special subroutine to update only the gold number on the bottomlines
	called from ogold()
 */
void
bottomgold(void)
{
	botsub(makecode(GOLD, 69, 19), "%-6ld");
	/* botsub(GOLD,"%-6ld",69,19); */
}

/*
	special routine to update hp and level fields on bottom lines
	called in monster.c hitplayer() and spattack()
 */
static void
bot_hpx(void)
{
	if (c[EXPERIENCE] != cbak[EXPERIENCE]) {
		recalc();
		bot_linex();
	} else
		botsub(makecode(HP, 5, 19), "%3ld");
}

/*
	special routine to update number of spells called from regen()
 */
static void
bot_spellx(void)
{
	botsub(makecode(SPELLS, 9, 18), "%2ld");
}

/*
	common subroutine for a more economical bottomline()
 */
static struct bot_side_def {
	int             typ;
	const char     *string;
}
                bot_data[] =
{
	{ STEALTH, "stealth"},
	{ UNDEADPRO, "undead pro" },
	{ SPIRITPRO, "spirit pro" },
	{ CHARMCOUNT, "Charm"},
	{ TIMESTOP, "Time Stop" },
	{ HOLDMONST, "Hold Monst" },
	{ GIANTSTR, "Giant Str"},
	{ FIRERESISTANCE, "Fire Resit" },
	{ DEXCOUNT, "Dexterity" },
	{ STRCOUNT, "Strength"},
	{ SCAREMONST, "Scare" },
	{ HASTESELF, "Haste Self" },
	{ CANCELLATION, "Cancel"},
	{ INVISIBILITY, "Invisible" },
	{ ALTPRO, "Protect 3" },
	{ PROTECTIONTIME, "Protect 2"},
	{ WTW, "Wall-Walk" }
};

static void
botside(void)
{
	int    i, idx;
	for (i = 0; i < 17; i++) {
		idx = bot_data[i].typ;
		if ((always) || (c[idx] != cbak[idx])) {
			if ((always) || (cbak[idx] == 0)) {
				if (c[idx]) {
					cursor(70, i + 1);
					lprcat(bot_data[i].string);
				}
			} else if (c[idx] == 0) {
				cursor(70, i + 1);
				lprcat("          ");
			}
			cbak[idx] = c[idx];
		}
	}
	always = 0;
}

static void
botsub(int idx, const char *str)
{
	int    x, y;
	y = idx & 0xff;
	x = (idx >> 8) & 0xff;
	idx >>= 16;
	if (c[idx] != cbak[idx]) {
		cbak[idx] = c[idx];
		cursor(x, y);
		lprintf(str, (long) c[idx]);
	}
}

/*
 *	subroutine to draw only a section of the screen
 *	only the top section of the screen is updated.
 *	If entire lines are being drawn, then they will be cleared first.
 */
/* for limited screen drawing */
static int d_xmin = 0, d_xmax = MAXX, d_ymin = 0, d_ymax = MAXY;

void
draws(int xmin, int xmax, int ymin, int ymax)
{
	int    i, idx;
	if (xmin == 0 && xmax == MAXX) {	/* clear section of screen as
						 * needed */
		if (ymin == 0)
			cl_up(79, ymax);
		else
			for (i = ymin; i < ymin; i++)
				cl_line(1, i + 1);
		xmin = -1;
	}
	d_xmin = xmin;
	d_xmax = xmax;
	d_ymin = ymin;
	d_ymax = ymax;		/* for limited screen drawing */
	drawscreen();
	if (xmin <= 0 && xmax == MAXX) {	/* draw stuff on right side
						 * of screen as needed */
		for (i = ymin; i < ymax; i++) {
			idx = bot_data[i].typ;
			if (c[idx]) {
				cursor(70, i + 1);
				lprcat(bot_data[i].string);
			}
			cbak[idx] = c[idx];
		}
	}
}

/*
	drawscreen()

	subroutine to redraw the whole screen as the player knows it
 */
u_char            screen[MAXX][MAXY];	/* template for the screen */
static u_char d_flag;
void
drawscreen(void)
{
	int    i, j, kk;
	int             lastx, lasty;	/* variables used to optimize the
					 * object printing */
	if (d_xmin == 0 && d_xmax == MAXX && d_ymin == 0 && d_ymax == MAXY) {
		d_flag = 1;
		clear();	/* clear the screen */
	} else {
		d_flag = 0;
		cursor(1, 1);
	}
	if (d_xmin < 0)
		d_xmin = 0;	/* d_xmin=-1 means display all without
				 * bottomline */

	for (i = d_ymin; i < d_ymax; i++)
		for (j = d_xmin; j < d_xmax; j++)
			if (know[j][i] == 0)
				screen[j][i] = ' ';
			else if ((kk = mitem[j][i]) != 0)
				screen[j][i] = monstnamelist[kk];
			else if ((kk = item[j][i]) == OWALL)
				screen[j][i] = '#';
			else
				screen[j][i] = ' ';

	for (i = d_ymin; i < d_ymax; i++) {
		j = d_xmin;
		while ((screen[j][i] == ' ') && (j < d_xmax))
			j++;
		/* was m=0 */
		if (j >= d_xmax)
			m = d_xmin;	/* don't search backwards if blank
					 * line */
		else {		/* search backwards for end of line */
			m = d_xmax - 1;
			while ((screen[m][i] == ' ') && (m > d_xmin))
				--m;
			if (j <= m)
				cursor(j + 1, i + 1);
			else
				continue;
		}
		while (j <= m) {
			if (j <= m - 3) {
				for (kk = j; kk <= j + 3; kk++)
					if (screen[kk][i] != ' ')
						kk = 1000;
				if (kk < 1000) {
					while (screen[j][i] == ' ' && j <= m)
						j++;
					cursor(j + 1, i + 1);
				}
			}
			lprc(screen[j++][i]);
		}
	}
	setbold();		/* print out only bold objects now */

	for (lastx = lasty = 127, i = d_ymin; i < d_ymax; i++)
		for (j = d_xmin; j < d_xmax; j++) {
			if ((kk = item[j][i]) != 0)
				if (kk != OWALL)
					if ((know[j][i]) && (mitem[j][i] == 0))
						if (objnamelist[kk] != ' ') {
							if (lasty != i + 1 || lastx != j)
								cursor(lastx = j + 1, lasty = i + 1);
							else
								lastx++;
							lprc(objnamelist[kk]);
						}
		}

	resetbold();
	if (d_flag) {
		always = 1;
		botside();
		always = 1;
		bot_linex();
	}
	oldx = 99;
	d_xmin = 0, d_xmax = MAXX, d_ymin = 0, d_ymax = MAXY;	/* for limited screen
								 * drawing */
}


/*
	showcell(x,y)

	subroutine to display a cell location on the screen
 */
void
showcell(int x, int y)
{
	int    i, j, kk, mm;
	if (c[BLINDCOUNT])
		return;		/* see nothing if blind		 */
	if (c[AWARENESS]) {
		minx = x - 3;
		maxx = x + 3;
		miny = y - 3;
		maxy = y + 3;
	} else {
		minx = x - 1;
		maxx = x + 1;
		miny = y - 1;
		maxy = y + 1;
	}

	if (minx < 0)
		minx = 0;
	if (maxx > MAXX - 1)
		maxx = MAXX - 1;
	if (miny < 0)
		miny = 0;
	if (maxy > MAXY - 1)
		maxy = MAXY - 1;

	for (j = miny; j <= maxy; j++)
		for (mm = minx; mm <= maxx; mm++)
			if (know[mm][j] == 0) {
				cursor(mm + 1, j + 1);
				x = maxx;
				while (know[x][j])
					--x;
				for (i = mm; i <= x; i++) {
					if ((kk = mitem[i][j]) != 0)
						lprc(monstnamelist[kk]);
					else
						switch (kk = item[i][j]) {
						case OWALL:
						case 0:
						case OIVTELETRAP:
						case OTRAPARROWIV:
						case OIVDARTRAP:
						case OIVTRAPDOOR:
							lprc(objnamelist[kk]);
							break;

						default:
							setbold();
							lprc(objnamelist[kk]);
							resetbold();
						};
					know[i][j] = 1;
				}
				mm = maxx;
			}
}

/*
	this routine shows only the spot that is given it.  the spaces around
	these coordinated are not shown
	used in godirect() in monster.c for missile weapons display
 */
void
show1cell(int x, int y)
{
	if (c[BLINDCOUNT])
		return;		/* see nothing if blind		 */
	cursor(x + 1, y + 1);
	if ((k = mitem[x][y]) != 0)
		lprc(monstnamelist[k]);
	else
		switch (k = item[x][y]) {
		case OWALL:
		case 0:
		case OIVTELETRAP:
		case OTRAPARROWIV:
		case OIVDARTRAP:
		case OIVTRAPDOOR:
			lprc(objnamelist[k]);
			break;

		default:
			setbold();
			lprc(objnamelist[k]);
			resetbold();
		};
	know[x][y] |= 1;	/* we end up knowing about it */
}

/*
	showplayer()

	subroutine to show where the player is on the screen
	cursor values start from 1 up
 */
void
showplayer(void)
{
	cursor(playerx + 1, playery + 1);
	oldx = playerx;
	oldy = playery;
}

/*
	moveplayer(dir)

	subroutine to move the player from one room to another
	returns 0 if can't move in that direction or hit a monster or on an object
	else returns 1
	nomove is set to 1 to stop the next move (inadvertent monsters hitting
	players when walking into walls) if player walks off screen or into wall
 */
short           diroffx[] = {0, 0, 1, 0, -1, 1, -1, 1, -1};
short           diroffy[] = {0, 1, 0, -1, 0, -1, -1, 1, 1};
int
moveplayer(int dir)
				/* from = present room #  direction =
				 * [1-north] [2-east] [3-south] [4-west]
				 * [5-northeast] [6-northwest] [7-southeast]
				 * [8-southwest] if direction=0, don't
				 * move--just show where he is */
{
	int    kk, mm, i, j;
	if (c[CONFUSE])
		if (c[LEVEL] < rnd(30))
			dir = rund(9);	/* if confused any dir */
	kk = playerx + diroffx[dir];
	mm = playery + diroffy[dir];
	if (kk < 0 || kk >= MAXX || mm < 0 || mm >= MAXY) {
		nomove = 1;
		return (yrepcount = 0);
	}
	i = item[kk][mm];
	j = mitem[kk][mm];
	if (i == OWALL && c[WTW] == 0) {
		nomove = 1;
		return (yrepcount = 0);
	}			/* hit a wall	 */
	if (kk == 33 && mm == MAXY - 1 && level == 1) {
		newcavelevel(0);
		for (kk = 0; kk < MAXX; kk++)
			for (mm = 0; mm < MAXY; mm++)
				if (item[kk][mm] == OENTRANCE) {
					playerx = kk;
					playery = mm;
					positionplayer();
					drawscreen();
					return (0);
				}
	}
	if (j > 0) {
		hitmonster(kk, mm);
		return (yrepcount = 0);
	}			/* hit a monster */
	lastpx = playerx;
	lastpy = playery;
	playerx = kk;
	playery = mm;
	if (i && i != OTRAPARROWIV && i != OIVTELETRAP && i != OIVDARTRAP && i != OIVTRAPDOOR)
		return (yrepcount = 0);
	else
		return (1);
}


/*
 *	function to show what magic items have been discovered thus far
 *	enter with -1 for just spells, anything else will give scrolls & potions
 */
static int      lincount, count;
void
seemagic(int arg)
{
	int    i, number = 0;
	count = lincount = 0;
	nosignal = 1;

	if (arg == -1) {	/* if display spells while casting one */
		for (number = i = 0; i < SPNUM; i++)
			if (spelknow[i])
				number++;
		number = (number + 2) / 3 + 4;	/* # lines needed to display */
		cl_up(79, number);
		cursor(1, 1);
	} else {
		resetscroll();
		clear();
	}

	lprcat("The magic spells you have discovered thus far:\n\n");
	for (i = 0; i < SPNUM; i++)
		if (spelknow[i]) {
			lprintf("%s %-20s ", spelcode[i], spelname[i]);
			seepage();
		}
	if (arg == -1) {
		seepage();
		more();
		nosignal = 0;
		draws(0, MAXX, 0, number);
		return;
	}
	lincount += 3;
	if (count != 0) {
		count = 2;
		seepage();
	}
	lprcat("\nThe magic scrolls you have found to date are:\n\n");
	count = 0;
	for (i = 0; i < MAXSCROLL; i++)
		if (scrollname[i][0])
			if (scrollname[i][1] != ' ') {
				lprintf("%-26s", &scrollname[i][1]);
				seepage();
			}
	lincount += 3;
	if (count != 0) {
		count = 2;
		seepage();
	}
	lprcat("\nThe magic potions you have found to date are:\n\n");
	count = 0;
	for (i = 0; i < MAXPOTION; i++)
		if (potionname[i][0])
			if (potionname[i][1] != ' ') {
				lprintf("%-26s", &potionname[i][1]);
				seepage();
			}
	if (lincount != 0)
		more();
	nosignal = 0;
	setscroll();
	drawscreen();
}

/*
 *	subroutine to paginate the seemagic function
 */
static void
seepage(void)
{
	if (++count == 3) {
		lincount++;
		count = 0;
		lprc('\n');
		if (lincount > 17) {
			lincount = 0;
			more();
			clear();
		}
	}
}
