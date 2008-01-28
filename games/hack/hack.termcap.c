/*	$NetBSD: hack.termcap.c,v 1.16 2008/01/28 06:55:42 dholland Exp $	*/

/*
 * Copyright (c) 1985, Stichting Centrum voor Wiskunde en Informatica,
 * Amsterdam
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Stichting Centrum voor Wiskunde en
 * Informatica, nor the names of its contributors may be used to endorse or
 * promote products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982 Jay Fenlason <hack@gnu.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: hack.termcap.c,v 1.16 2008/01/28 06:55:42 dholland Exp $");
#endif				/* not lint */

#include <string.h>
#include <termios.h>
#include <termcap.h>
#include <stdlib.h>
#include <unistd.h>
#include "hack.h"
#include "extern.h"
#include "def.flag.h"		/* for flags.nonull */

static struct tinfo *info;
static const char    *HO, *CL, *CE, *CM, *ND, *XD, *BC_BS, *SO, *SE, *TI, *TE;
static const char    *VS, *VE;
static int      SG;
char           *CD;		/* tested in pri.c: docorner() */
int             CO, LI;		/* used in pri.c and whatis.c */

void
startup()
{
	char           *term;
	
	/* UP, BC, PC already set */
 	if (!(term = getenv("TERM")))
		error("Can't get TERM.");
	if (!strncmp(term, "5620", 4))
		flags.nonull = 1;	/* this should be a termcap flag */
	if (t_getent(&info, term) < 1)
		error("Unknown terminal type: %s.", term);
	BC_BS = t_agetstr(info, "bc");
	if (!BC_BS) {
		if (!t_getflag(info, "bs"))
			error("Terminal must backspace.");
		BC_BS = "\b";
	}
	HO = t_agetstr(info, "ho");
	CO = t_getnum(info, "co");
	LI = t_getnum(info, "li");
	if (CO < COLNO || LI < ROWNO + 2)
		setclipped();
	if (!(CL = t_agetstr(info, "cl")))
		error("Hack needs CL.");
	ND = t_agetstr(info, "nd");
	if (t_getflag(info, "os"))
		error("Hack can't have OS.");
	CE = t_agetstr(info, "ce");
	/*
	 * It seems that xd is no longer supported, and we should use a
	 * linefeed instead; unfortunately this requires resetting CRMOD, and
	 * many output routines will have to be modified slightly. Let's
	 * leave that till the next release.
	 */
	XD = t_agetstr(info, "xd");
	/* not: 		XD = t_agetstr(info, "do"); */
	if (!(CM = t_agetstr(info, "cm"))) {
		if (!UP && !HO)
			error("Hack needs CM or UP or HO.");
		printf("Playing hack on terminals without cm is suspect...\n");
		getret();
	}
	SO = t_agetstr(info, "so");
	SE = t_agetstr(info, "se");
	SG = t_getnum(info, "sg");	/* -1: not fnd; else # of spaces left by so */
	if (!SO || !SE || (SG > 0))
		SO = SE = 0;
	CD = t_agetstr(info, "cd");
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
			xputs(BC_BS);
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
xputc(int c)
{
	return (fputc(c, stdout));
}

void
xputs(const char *s)
{
	tputs(s, 1, xputc);
}

void
cl_end(void)
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
	xputs(BC_BS);
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
