/*	$NetBSD: hack.termcap.c,v 1.10 2000/05/20 14:01:42 blymn Exp $	*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: hack.termcap.c,v 1.10 2000/05/20 14:01:42 blymn Exp $");
#endif				/* not lint */

#include <string.h>
#include <termios.h>
#include <termcap.h>
#include <stdlib.h>
#include <unistd.h>
#include "hack.h"
#include "extern.h"
#include "def.flag.h"		/* for flags.nonull */

static char     *tbuf;
static struct tinfo *info;
static char    *HO, *CL, *CE, *UP, *CM, *ND, *XD, *BC, *SO, *SE, *TI, *TE;
static char    *VS, *VE;
static int      SG;
static char     PC = '\0';
static char     BC_char = '\b'; /* if bc is not set use this */
char           *CD;		/* tested in pri.c: docorner() */
int             CO, LI;		/* used in pri.c and whatis.c */

void
startup()
{
	char           *term;
	char           *tbufptr, *pc;

	tbuf = NULL;
	
 	if (!(term = getenv("TERM")))
		error("Can't get TERM.");
	if (!strncmp(term, "5620", 4))
		flags.nonull = 1;	/* this should be a termcap flag */
	if (t_getent(&info, term) < 1)
		error("Unknown terminal type: %s.", term);
	if ((pc = t_agetstr(info, "pc", &tbuf, &tbufptr)) != NULL)
		PC = *pc;
	if (!(BC = t_agetstr(info, "bc", &tbuf, &tbufptr))) {
		if (!t_getflag(info, "bs"))
			error("Terminal must backspace.");
		BC = &BC_char;
	}
	HO = t_agetstr(info, "ho", &tbuf, &tbufptr);
	CO = t_getnum(info, "co");
	LI = t_getnum(info, "li");
	if (CO < COLNO || LI < ROWNO + 2)
		setclipped();
	if (!(CL = t_agetstr(info, "cl", &tbuf, &tbufptr)))
		error("Hack needs CL.");
	ND = t_agetstr(info, "nd", &tbuf, &tbufptr);
	if (t_getflag(info, "os"))
		error("Hack can't have OS.");
	CE = t_agetstr(info, "ce", &tbuf, &tbufptr);
	UP = t_agetstr(info, "up", &tbuf, &tbufptr);
	/*
	 * It seems that xd is no longer supported, and we should use a
	 * linefeed instead; unfortunately this requires resetting CRMOD, and
	 * many output routines will have to be modified slightly. Let's
	 * leave that till the next release.
	 */
	XD = t_agetstr(info, "xd", &tbuf, &tbufptr);
	/* not: 		XD = t_agetstr(info, "do", &tbuf, &tbufptr); */
	if (!(CM = t_agetstr(info, "cm", &tbuf, &tbufptr))) {
		if (!UP && !HO)
			error("Hack needs CM or UP or HO.");
		printf("Playing hack on terminals without cm is suspect...\n");
		getret();
	}
	SO = t_agetstr(info, "so", &tbuf, &tbufptr);
	SE = t_agetstr(info, "se", &tbuf, &tbufptr);
	SG = t_getnum(info, "sg");	/* -1: not fnd; else # of spaces left by so */
	if (!SO || !SE || (SG > 0))
		SO = SE = 0;
	CD = t_agetstr(info, "cd", &tbuf, &tbufptr);
	set_whole_screen();	/* uses LI and CD */
}

void
start_screen()
{
	xputs(TI);
	xputs(VS);
}

void
end_screen()
{
	xputs(VE);
	xputs(TE);
}

/* Cursor movements */
void
curs(x, y)
	int             x, y;	/* not xchar: perhaps xchar is unsigned and
				 * curx-x would be unsigned as well */
{

	if (y == cury && x == curx)
		return;
	if (!ND && (curx != x || x <= 3)) {	/* Extremely primitive */
		cmov(x, y);	/* bunker!wtm */
		return;
	}
	if (abs(cury - y) <= 3 && abs(curx - x) <= 3)
		nocmov(x, y);
	else if ((x <= 3 && abs(cury - y) <= 3) || (!CM && x < abs(curx - x))) {
		(void) putchar('\r');
		curx = 1;
		nocmov(x, y);
	} else if (!CM) {
		nocmov(x, y);
	} else
		cmov(x, y);
}

void
nocmov(x, y)
	int x, y;
{
	if (cury > y) {
		if (UP) {
			while (cury > y) {	/* Go up. */
				xputs(UP);
				cury--;
			}
		} else if (CM) {
			cmov(x, y);
		} else if (HO) {
			home();
			curs(x, y);
		}		/* else impossible("..."); */
	} else if (cury < y) {
		if (XD) {
			while (cury < y) {
				xputs(XD);
				cury++;
			}
		} else if (CM) {
			cmov(x, y);
		} else {
			while (cury < y) {
				xputc('\n');
				curx = 1;
				cury++;
			}
		}
	}
	if (curx < x) {		/* Go to the right. */
		if (!ND)
			cmov(x, y);
		else		/* bah */
			/* should instead print what is there already */
			while (curx < x) {
				xputs(ND);
				curx++;
			}
	} else if (curx > x) {
		while (curx > x) {	/* Go to the left. */
			xputs(BC);
			curx--;
		}
	}
}

void
cmov(x, y)
	int x, y;
{
	char buf[256];

	if (t_goto(info, CM, x - 1, y - 1, buf, 255) >= 0) {
		xputs(buf);
		cury = y;
		curx = x;
	}
}

int
xputc(c)
	char            c;
{
	return (fputc(c, stdout));
}

void
xputs(s)
	char           *s;
{
	tputs(s, 1, xputc);
}

void
cl_end()
{
	if (CE)
		xputs(CE);
	else {			/* no-CE fix - free after Harold Rynes */
		/*
		 * this looks terrible, especially on a slow terminal but is
		 * better than nothing
		 */
		int cx = curx, cy = cury;

		while (curx < COLNO) {
			xputc(' ');
			curx++;
		}
		curs(cx, cy);
	}
}

void
clear_screen()
{
	xputs(CL);
	curx = cury = 1;
}

void
home()
{
	char buf[256];
	
	if (HO)
		xputs(HO);
	else if ((CM) && (t_goto(info, CM, 0, 0, buf, 255) >= 0))
		xputs(buf);
	else
		curs(1, 1);	/* using UP ... */
	curx = cury = 1;
}

void
standoutbeg()
{
	if (SO)
		xputs(SO);
}

void
standoutend()
{
	if (SE)
		xputs(SE);
}

void
backsp()
{
	xputs(BC);
	curx--;
}

void
bell()
{
	(void) putchar('\007');	/* curx does not change */
	(void) fflush(stdout);
}

void
delay_output()
{
	
	/* delay 50 ms - could also use a 'nap'-system call */
	  /* or the usleep call like this :-) */
	usleep(50000);
}

void
cl_eos()
{				/* free after Robert Viduya *//* must only be
				 * called with curx = 1 */

	if (CD)
		xputs(CD);
	else {
		int             cx = curx, cy = cury;
		while (cury <= LI - 2) {
			cl_end();
			xputc('\n');
			curx = 1;
			cury++;
		}
		cl_end();
		curs(cx, cy);
	}
}
