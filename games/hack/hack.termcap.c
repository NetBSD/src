/*	$NetBSD: hack.termcap.c,v 1.8.2.1 1999/12/27 18:29:02 wrstuden Exp $	*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: hack.termcap.c,v 1.8.2.1 1999/12/27 18:29:02 wrstuden Exp $");
#endif				/* not lint */

#include <string.h>
#include <termios.h>
#include <termcap.h>
#include <stdlib.h>
#include "hack.h"
#include "extern.h"
#include "def.flag.h"		/* for flags.nonull */

static char     tbuf[512];
static char    *HO, *CL, *CE, *UP, *CM, *ND, *XD, *BC, *SO, *SE, *TI, *TE;
static char    *VS, *VE;
static int      SG;
static char     PC = '\0';
char           *CD;		/* tested in pri.c: docorner() */
int             CO, LI;		/* used in pri.c and whatis.c */

void
startup()
{
	char           *term;
	char           *tptr;
	char           *tbufptr, *pc;

	tptr = (char *) alloc(1024);

	tbufptr = tbuf;
	if (!(term = getenv("TERM")))
		error("Can't get TERM.");
	if (!strncmp(term, "5620", 4))
		flags.nonull = 1;	/* this should be a termcap flag */
	if (tgetent(tptr, term) < 1)
		error("Unknown terminal type: %s.", term);
	if ((pc = tgetstr("pc", &tbufptr)) != NULL)
		PC = *pc;
	if (!(BC = tgetstr("bc", &tbufptr))) {
		if (!tgetflag("bs"))
			error("Terminal must backspace.");
		BC = tbufptr;
		tbufptr += 2;
		*BC = '\b';
	}
	HO = tgetstr("ho", &tbufptr);
	CO = tgetnum("co");
	LI = tgetnum("li");
	if (CO < COLNO || LI < ROWNO + 2)
		setclipped();
	if (!(CL = tgetstr("cl", &tbufptr)))
		error("Hack needs CL.");
	ND = tgetstr("nd", &tbufptr);
	if (tgetflag("os"))
		error("Hack can't have OS.");
	CE = tgetstr("ce", &tbufptr);
	UP = tgetstr("up", &tbufptr);
	/*
	 * It seems that xd is no longer supported, and we should use a
	 * linefeed instead; unfortunately this requires resetting CRMOD, and
	 * many output routines will have to be modified slightly. Let's
	 * leave that till the next release.
	 */
	XD = tgetstr("xd", &tbufptr);
	/* not: 		XD = tgetstr("do", &tbufptr); */
	if (!(CM = tgetstr("cm", &tbufptr))) {
		if (!UP && !HO)
			error("Hack needs CM or UP or HO.");
		printf("Playing hack on terminals without cm is suspect...\n");
		getret();
	}
	SO = tgetstr("so", &tbufptr);
	SE = tgetstr("se", &tbufptr);
	SG = tgetnum("sg");	/* -1: not fnd; else # of spaces left by so */
	if (!SO || !SE || (SG > 0))
		SO = SE = 0;
	CD = tgetstr("cd", &tbufptr);
	set_whole_screen();	/* uses LI and CD */
	if (tbufptr - tbuf > sizeof(tbuf))
		error("TERMCAP entry too big...\n");
	free(tptr);
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
	xputs(tgoto(CM, x - 1, y - 1));
	cury = y;
	curx = x;
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
	if (HO)
		xputs(HO);
	else if (CM)
		xputs(tgoto(CM, 0, 0));
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
	/*
	 * BUG: if the padding character is visible, as it is on the 5620
	 * then this looks terrible.
	 */
	if (!flags.nonull)
		tputs("50", 1, xputc);

	/* cbosgd!cbcephus!pds for SYS V R2 */
	/* is this terminfo, or what? */
	/* tputs("$<50>", 1, xputc); */

	else if (ospeed > 0)
		if (CM) {
			/*
			 * delay by sending cm(here) an appropriate number of
			 * times
			 */
			int             cmlen = strlen(tgoto(CM, curx - 1, cury - 1));
			int             i = (ospeed + (100 * cmlen)) / (200 * cmlen);

			while (i > 0) {
				cmov(curx, cury);
			}
		}
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
