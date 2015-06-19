/*	$NetBSD: input.c,v 1.29 2015/06/19 06:02:31 dholland Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ed James.
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

/*
 * Copyright (c) 1987 by Ed James, UC Berkeley.  All rights reserved.
 *
 * Copy permission is hereby granted provided that this notice is
 * retained on all partial or complete copies.
 *
 * For more info on this and all of my stuff, mail edjames@berkeley.edu.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)input.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: input.c,v 1.29 2015/06/19 06:02:31 dholland Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>

#include "pathnames.h"
#include "def.h"
#include "struct.h"
#include "extern.h"
#include "tunable.h"

static void rezero(void);
static void noise(void);
static int gettoken(void);
static const char *setplane(int);
static const char *turn(int);
static const char *circle(int);
static const char *left(int);
static const char *right(int);
static const char *Left(int);
static const char *Right(int);
static const char *delayb(int);
static const char *beacon(int);
static const char *ex_it(int);
static const char *airport(int);
static const char *climb(int);
static const char *descend(int);
static const char *setalt(int);
static const char *setrelalt(int);
static const char *benum(int);
static const char *to_dir(int);
static const char *rel_dir(int);
static const char *mark(int);
static const char *unmark(int);
static const char *ignore(int);



#define MAXRULES	6
#define MAXDEPTH	15

#define RETTOKEN	'\n'
#define REDRAWTOKEN	'\014'	/* CTRL(L) */
#define	SHELLTOKEN	'!'
#define HELPTOKEN	'?'
#define ALPHATOKEN	256
#define NUMTOKEN	257

typedef struct {
	int	token;
	int	to_state;
	const char	*str;
	const char	*(*func)(int);
} RULE;

typedef struct {
	int	num_rules;
	RULE	*rule;
} STATE;

typedef struct {
	char	str[20];
	int	state;
	int	rule;
	int	ch;
	int	pos;
} STACK;

#define T_RULE		stack[level].rule
#define T_STATE		stack[level].state
#define T_STR		stack[level].str
#define T_POS		stack[level].pos
#define	T_CH		stack[level].ch

#define NUMELS(a)	(sizeof (a) / sizeof (*(a)))

#define NUMSTATES	NUMELS(st)

static
RULE	state0[] = {	{ ALPHATOKEN,	1,	"%c:",		setplane},
			{ RETTOKEN,	-1,	"",		NULL	},
			{ HELPTOKEN,	12,	" [a-z]<ret>",	NULL	}},
	state1[] = {	{ 't',		2,	" turn",	turn	},	
			{ 'a',		3,	" altitude:",	NULL	},	
			{ 'c',		4,	" circle",	circle	},
			{ 'm',		7,	" mark",	mark	},
			{ 'u',		7,	" unmark",	unmark	},
			{ 'i',		7,	" ignore",	ignore	},
			{ HELPTOKEN,	12,	" tacmui",	NULL	}},
	state2[] = {	{ 'l',		6,	" left",	left	},	
			{ 'r',		6,	" right",	right	},	
			{ 'L',		4,	" left 90",	Left	},
			{ 'R',		4,	" right 90",	Right	},	
			{ 't',		11,	" towards",	NULL	},
			{ 'w',		4,	" to 0",	to_dir	},
			{ 'e',		4,	" to 45",	to_dir	},
			{ 'd',		4,	" to 90",	to_dir	},
			{ 'c',		4,	" to 135",	to_dir	},
			{ 'x',		4,	" to 180",	to_dir	},
			{ 'z',		4,	" to 225",	to_dir	},
			{ 'a',		4,	" to 270",	to_dir	},
			{ 'q',		4,	" to 315",	to_dir	},
			{ HELPTOKEN,	12,	" lrLRt<dir>",	NULL	}},
	state3[] = {	{ '+',		10,	" climb",	climb	},	
			{ 'c',		10,	" climb",	climb	},	
			{ '-',		10,	" descend",	descend	},	
			{ 'd',		10,	" descend",	descend	},	
			{ NUMTOKEN,	7,	" %c000 feet",	setalt	},
			{ HELPTOKEN,	12,	" +-cd[0-9]",	NULL	}},
	state4[] = {	{ '@',		9,	" at",		NULL	},	
			{ 'a',		9,	" at",		NULL	},	
			{ RETTOKEN,	-1,	"",		NULL	},
			{ HELPTOKEN,	12,	" @a<ret>",	NULL	}},
	state5[] = {	{ NUMTOKEN,	7,	"%c",		delayb	},
			{ HELPTOKEN,	12,	" [0-9]",	NULL	}},
	state6[] = {	{ '@',		9,	" at",		NULL	},
			{ 'a',		9,	" at",		NULL	},
			{ 'w',		4,	" 0",		rel_dir	},
			{ 'e',		4,	" 45",		rel_dir	},
			{ 'd',		4,	" 90",		rel_dir	},
			{ 'c',		4,	" 135",		rel_dir	},
			{ 'x',		4,	" 180",		rel_dir	},
			{ 'z',		4,	" 225",		rel_dir	},
			{ 'a',		4,	" 270",		rel_dir	},
			{ 'q',		4,	" 315",		rel_dir	},
			{ RETTOKEN,	-1,	"",		NULL	},	
			{ HELPTOKEN,	12,	" @a<dir><ret>",NULL	}},
	state7[] = {	{ RETTOKEN,	-1,	"",		NULL	},
			{ HELPTOKEN,	12,	" <ret>",	NULL	}},
	state8[] = {	{ NUMTOKEN,	4,	"%c",		benum	},
			{ HELPTOKEN,	12,	" [0-9]",	NULL	}},
	state9[] = {	{ 'b',		5,	" beacon #",	NULL	},
			{ '*',		5,	" beacon #",	NULL	},
			{ HELPTOKEN,	12,	" b*",		NULL	}},
	state10[] = {	{ NUMTOKEN,	7,	" %c000 ft",	setrelalt},
			{ HELPTOKEN,	12,	" [0-9]",	NULL	}},
	state11[] = {	{ 'b',		8,	" beacon #",	beacon	},	
			{ '*',		8,	" beacon #",	beacon	},
			{ 'e',		8,	" exit #",	ex_it	},
			{ 'a',		8,	" airport #",	airport	},
			{ HELPTOKEN,	12,	" b*ea",	NULL	}},
	state12[] = {	{ -1,		-1,	"",		NULL	}};

#define DEF_STATE(s)	{ NUMELS(s),	(s)	}

static STATE st[] = {
	DEF_STATE(state0), DEF_STATE(state1), DEF_STATE(state2),
	DEF_STATE(state3), DEF_STATE(state4), DEF_STATE(state5),
	DEF_STATE(state6), DEF_STATE(state7), DEF_STATE(state8),
	DEF_STATE(state9), DEF_STATE(state10), DEF_STATE(state11),
	DEF_STATE(state12)
};

static PLANE p;
static STACK stack[MAXDEPTH];
static int level;
static int tval;
static int dir;
static enum places dest_type;
static unsigned dest_no;

static int
pop(void)
{
	if (level == 0)
		return (-1);
	level--;

	ioclrtoeol(T_POS);

	(void)strcpy(T_STR, "");
	T_RULE = -1;
	T_CH = -1;
	return (0);
}

static void
rezero(void)
{
	iomove(0);

	level = 0;
	T_STATE = 0;
	T_RULE = -1;
	T_CH = -1;
	T_POS = 0;
	(void)strcpy(T_STR, "");
}

static void
push(int ruleno, int ch)
{
	int	newstate, newpos;

	assert(level < (MAXDEPTH - 1));
	(void)snprintf(T_STR, sizeof(T_STR),
		st[T_STATE].rule[ruleno].str, tval);
	T_RULE = ruleno;
	T_CH = ch;
	newstate = st[T_STATE].rule[ruleno].to_state;
	newpos = T_POS + strlen(T_STR);

	ioaddstr(T_POS, T_STR);

	if (level == 0)
		ioclrtobot();
	level++;
	T_STATE = newstate;
	T_POS = newpos;
	T_RULE = -1;
	(void)strcpy(T_STR, "");
}

int
getcommand(void)
{
	int	c, i, done;
	const char	*s, *(*func)(int);
	PLANE	*pp;

	rezero();

	do {
		c = gettoken();
		if (c == tty_new.c_cc[VERASE]) {
			if (pop() < 0)
				noise();
		} else if (c == tty_new.c_cc[VKILL]) {
			while (pop() >= 0)
				;
		} else {
			done = 0;
			for (i = 0; i < st[T_STATE].num_rules; i++) {
				if (st[T_STATE].rule[i].token == c ||
				    st[T_STATE].rule[i].token == tval) {
					push(i, (c >= ALPHATOKEN) ? tval : c);
					done = 1;
					break;
				}
			}
			if (!done)
				noise();
		}
	} while (T_STATE != -1);

	if (level == 1)
		return (1);	/* forced update */

	dest_type = T_NODEST;
	
	for (i = 0; i < level; i++) {
		func = st[stack[i].state].rule[stack[i].rule].func;
		if (func != NULL)
			if ((s = (*func)(stack[i].ch)) != NULL) {
				ioerror(stack[i].pos, 
				    (int)strlen(stack[i].str), s);
				return (-1);
			}
	}

	pp = findplane(p.plane_no);
	if (pp->new_altitude != p.new_altitude)
		pp->new_altitude = p.new_altitude;
	else if (pp->status != p.status)
		pp->status = p.status;
	else {
		pp->new_dir = p.new_dir;
		pp->delayd = p.delayd;
		pp->delayd_no = p.delayd_no;
	}
	return (0);
}

static void
noise(void)
{
	(void)putchar('\07');
	(void)fflush(stdout);
}

static int
gettoken(void)
{
	while ((tval = getAChar()) == REDRAWTOKEN || tval == SHELLTOKEN)
	{
		if (tval == SHELLTOKEN)
		{
#ifdef BSD
			struct itimerval	itv;
			itv.it_value.tv_sec = 0;
			itv.it_value.tv_usec = 0;
			(void)setitimer(ITIMER_REAL, &itv, NULL);
#endif
#ifdef SYSV
			int aval;
			aval = alarm(0);
#endif
			if (fork() == 0)	/* child */
			{
				char *shell, *base;

				done_screen();

						 /* run user's favorite shell */
				if ((shell = getenv("SHELL")) != NULL)
				{
					base = strrchr(shell, '/');
					if (base == NULL)
						base = shell;
					else
						base++;
					(void)execl(shell, base, (char *) 0);
				}
				else
					(void)execl(_PATH_BSHELL, "sh", 
					    (char *) 0);

				exit(0);	/* oops */
			}

			(void)wait(0);
			(void)tcsetattr(fileno(stdin), TCSADRAIN, &tty_new);
#ifdef BSD
			itv.it_value.tv_sec = 0;
			itv.it_value.tv_usec = 1;
			itv.it_interval.tv_sec = sp->update_secs;
			itv.it_interval.tv_usec = 0;
			(void)setitimer(ITIMER_REAL, &itv, NULL);
#endif
#ifdef SYSV
			alarm(aval);
#endif
		}
		(void)redraw();
	}

	if (isdigit(tval))
		return (NUMTOKEN);
	else if (isalpha(tval))
		return (ALPHATOKEN);
	else
		return (tval);
}

static const char *
setplane(int c)
{
	PLANE	*pp;

	pp = findplane(number(c));
	if (pp == NULL)
		return ("Unknown Plane");
	(void)memcpy(&p, pp, sizeof (p));
	p.delayd = false;
	return (NULL);
}

/* ARGSUSED */
static const char *
turn(int c __unused)
{
	if (p.altitude == 0)
		return ("Planes at airports may not change direction");
	return (NULL);
}

/* ARGSUSED */
static const char *
circle(int c __unused)
{
	if (p.altitude == 0)
		return ("Planes cannot circle on the ground");
	p.new_dir = MAXDIR;
	return (NULL);
}

/* ARGSUSED */
static const char *
left(int c __unused)
{
	dir = D_LEFT;
	p.new_dir = p.dir - 1;
	if (p.new_dir < 0)
		p.new_dir += MAXDIR;
	return (NULL);
}

/* ARGSUSED */
static const char *
right(int c __unused)
{
	dir = D_RIGHT;
	p.new_dir = p.dir + 1;
	if (p.new_dir >= MAXDIR)
		p.new_dir -= MAXDIR;
	return (NULL);
}

/* ARGSUSED */
static const char *
Left(int c __unused)
{
	p.new_dir = p.dir - 2;
	if (p.new_dir < 0)
		p.new_dir += MAXDIR;
	return (NULL);
}

/* ARGSUSED */
static const char *
Right(int c __unused)
{
	p.new_dir = p.dir + 2;
	if (p.new_dir >= MAXDIR)
		p.new_dir -= MAXDIR;
	return (NULL);
}

static const char *
delayb(int ch)
{
	int	xdiff, ydiff;
	unsigned bn;

	bn = ch -= '0';

	if (bn >= sp->num_beacons)
		return ("Unknown beacon");
	xdiff = sp->beacon[bn].x - p.xpos;
	xdiff = SGN(xdiff);
	ydiff = sp->beacon[bn].y - p.ypos;
	ydiff = SGN(ydiff);
	if (xdiff != displacement[p.dir].dx || ydiff != displacement[p.dir].dy)
		return ("Beacon is not in flight path");
	p.delayd = true;
	p.delayd_no = bn;

	if (dest_type != T_NODEST) {
		switch (dest_type) {
		case T_BEACON:
			xdiff = sp->beacon[dest_no].x - sp->beacon[bn].x;
			ydiff = sp->beacon[dest_no].y - sp->beacon[bn].y;
			break;
		case T_EXIT:
			xdiff = sp->exit[dest_no].x - sp->beacon[bn].x;
			ydiff = sp->exit[dest_no].y - sp->beacon[bn].y;
			break;
		case T_AIRPORT:
			xdiff = sp->airport[dest_no].x - sp->beacon[bn].x;
			ydiff = sp->airport[dest_no].y - sp->beacon[bn].y;
			break;
		default:
			return ("Bad case in delayb!  Get help!");
		}
		if (xdiff == 0 && ydiff == 0)
			return ("Would already be there");
		p.new_dir = DIR_FROM_DXDY(xdiff, ydiff);
		if (p.new_dir == p.dir)
			return ("Already going in that direction");
	}
	return (NULL);
}

/* ARGSUSED */
static const char *
beacon(int c __unused)
{
	dest_type = T_BEACON;
	return (NULL);
}

/* ARGSUSED */
static const char *
ex_it(int c __unused)
{
	dest_type = T_EXIT;
	return (NULL);
}

/* ARGSUSED */
static const char *
airport(int c __unused)
{
	dest_type = T_AIRPORT;
	return (NULL);
}

/* ARGSUSED */
static const char *
climb(int c __unused)
{
	dir = D_UP;
	return (NULL);
}

/* ARGSUSED */
static const char *
descend(int c __unused)
{
	dir = D_DOWN;
	return (NULL);
}

static const char *
setalt(int c)
{
	int newalt = c - '0';
	if ((p.altitude == newalt) && (p.new_altitude == p.altitude))
		return ("Already at that altitude");
	if (p.new_altitude == newalt) {
		return ("Already going to that altitude");
	}
	p.new_altitude = newalt;
	return (NULL);
}

static const char *
setrelalt(int c)
{
	int newalt;

	if (c == 0)
		return ("altitude not changed");

	switch (dir) {
	case D_UP:
		newalt = p.altitude + c - '0';
		break;
	case D_DOWN:
		newalt = p.altitude - (c - '0');
		break;
	default:
		return ("Unknown case in setrelalt!  Get help!");
	}

	if (p.new_altitude == newalt)
		return ("Already going to that altitude");

	p.new_altitude = newalt;

	if (p.new_altitude < 0)
		return ("Altitude would be too low");
	else if (p.new_altitude > 9)
		return ("Altitude would be too high");
	return (NULL);
}

static const char *
benum(int ch)
{
	unsigned n;

	n = ch - '0';
	dest_no = n;

	switch (dest_type) {
	case T_BEACON:
		if (n >= sp->num_beacons)
			return ("Unknown beacon");
		p.new_dir = DIR_FROM_DXDY(sp->beacon[n].x - p.xpos,
			sp->beacon[n].y - p.ypos);
		break;
	case T_EXIT:
		if (n >= sp->num_exits)
			return ("Unknown exit");
		p.new_dir = DIR_FROM_DXDY(sp->exit[n].x - p.xpos,
			sp->exit[n].y - p.ypos);
		break;
	case T_AIRPORT:
		if (n >= sp->num_airports)
			return ("Unknown airport");
		p.new_dir = DIR_FROM_DXDY(sp->airport[n].x - p.xpos,
			sp->airport[n].y - p.ypos);
		break;
	default:
		return ("Unknown case in benum!  Get help!");
	}
	return (NULL);
}

static const char *
to_dir(int c)
{
	p.new_dir = dir_no(c);
	return (NULL);
}

static const char *
rel_dir(int c)
{
	int	angle;

	angle = dir_no(c);
	switch (dir) {
	case D_LEFT:
		p.new_dir = p.dir - angle;
		if (p.new_dir < 0)
			p.new_dir += MAXDIR;
		break;
	case D_RIGHT:
		p.new_dir = p.dir + angle;
		if (p.new_dir >= MAXDIR)
			p.new_dir -= MAXDIR;
		break;
	default:
		return ("Bizarre direction in rel_dir!  Get help!");
	}
	return (NULL);
}

/* ARGSUSED */
static const char *
mark(int c __unused)
{
	if (p.altitude == 0)
		return ("Cannot mark planes on the ground");
	if (p.status == S_MARKED)
		return ("Already marked");
	p.status = S_MARKED;
	return (NULL);
}

/* ARGSUSED */
static const char *
unmark(int c __unused)
{
	if (p.altitude == 0)
		return ("Cannot unmark planes on the ground");
	if (p.status == S_UNMARKED)
		return ("Already unmarked");
	p.status = S_UNMARKED;
	return (NULL);
}

/* ARGSUSED */
static const char *
ignore(int c __unused)
{
	if (p.altitude == 0)
		return ("Cannot ignore planes on the ground");
	if (p.status == S_IGNORED)
		return ("Already ignored");
	p.status = S_IGNORED;
	return (NULL);
}

int
dir_no(int ch)
{
	int	dirno;

	dirno = -1;
	switch (ch) {
	case 'w':	dirno = 0;	break;
	case 'e':	dirno = 1;	break;
	case 'd':	dirno = 2;	break;
	case 'c':	dirno = 3;	break;
	case 'x':	dirno = 4;	break;
	case 'z':	dirno = 5;	break;
	case 'a':	dirno = 6;	break;
	case 'q':	dirno = 7;	break;
	default:
		(void)fprintf(stderr, "bad character in dir_no\n");
		break;
	}
	return (dirno);
}
