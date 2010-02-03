/*	$NetBSD: hack.terminfo.c,v 1.1 2010/02/03 15:34:38 roy Exp $	*/

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
__RCSID("$NetBSD: hack.terminfo.c,v 1.1 2010/02/03 15:34:38 roy Exp $");
#endif				/* not lint */

#include <string.h>
#include <termios.h>
#include <term.h>
#include <stdlib.h>
#include <unistd.h>
#include "hack.h"
#include "extern.h"
#include "def.flag.h"		/* for flags.nonull */

char           *CD;		/* tested in pri.c: docorner() */
int             CO, LI;		/* used in pri.c and whatis.c */

void
startup(void)
{

	/* Will exit if no suitable term found */
	setupterm(NULL, 0, NULL);
	CO = columns;
	LI = lines;
	if (CO < COLNO || LI < ROWNO + 2)
		setclipped();
	if (clear_screen == NULL)
		error("Hack needs clear_screen.");
	if (over_strike)
		error("Hack can't have over_strike.");
	if (cursor_address == NULL) {
		printf("Playing hack without cursor_address is suspect...");
		getret();
	}
	set_whole_screen();
}

void
startscreen(void)
{
}

void
endscreen(void)
{
}

static int
xputc(int c)
{
	return (fputc(c, stdout));
}

static void
xputs(const char *s)
{
	tputs(s, 1, xputc);
}

static void
cmov(int x, int y)
{
	char *p; 

	p = vtparm(cursor_address, y - 1, x - 1);
	if (p) {
		xputs(p);
		cury = y;
		curx = x;
	}
}

static void
nocmov(int x, int y)
{
	if (cury > y) {
		if (cursor_up) {
			while (cury > y) {	/* Go up. */
				xputs(cursor_up);
				cury--;
			}
		} else if (cursor_address) {
			cmov(x, y);
		} else if (cursor_home) {
			home();
			curs(x, y);
		}		/* else impossible("..."); */
	} else if (cury < y) {
		if (cursor_address) {
			cmov(x, y);
#if 0
		} else if (XD) {
			while (cury < y) {
				xputs(XD);
				cury++;
			}
#endif
		} else {
			while (cury < y) {
				xputc('\n');
				curx = 1;
				cury++;
			}
		}
	}
	if (curx < x) {		/* Go to the right. */
		if (!cursor_right)
			cmov(x, y);
		else		/* bah */
			/* should instead print what is there already */
			while (curx < x) {
				xputs(cursor_right);
				curx++;
			}
	} else if (curx > x) {
		while (curx > x)
			backsp();
	}
}

/*
 * Cursor movements
 *
 * x,y not xchar: perhaps xchar is unsigned and
 * curx-x would be unsigned as well
 */
void
curs(int x, int y)
{

	if (y == cury && x == curx)
		return;
	if (!cursor_right && (curx != x || x <= 3)) { /* Extremely primitive */
		cmov(x, y);	/* bunker!wtm */
		return;
	}
	if (abs(cury - y) <= 3 && abs(curx - x) <= 3)
		nocmov(x, y);
	else if ((x <= 3 && abs(cury - y) <= 3) ||
	    (!cursor_address && x < abs(curx - x)))
	{
		(void) putchar('\r');
		curx = 1;
		nocmov(x, y);
	} else if (!cursor_address) {
		nocmov(x, y);
	} else
		cmov(x, y);
}

void
cl_end(void)
{
	if (clr_eol)
		xputs(clr_eol);
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
clearscreen(void)
{
	xputs(clear_screen);
	curx = cury = 1;
}

void
home(void)
{
	char *out;
	
	if (cursor_home)
		xputs(cursor_home);
	else if ((cursor_address) && (out = vtparm(cursor_address, 0, 0)))
		xputs(out);
	else
		curs(1, 1);	/* using UP ... */
	curx = cury = 1;
}

void
standoutbeg(void)
{
	if (enter_standout_mode && exit_standout_mode && !magic_cookie_glitch)
		xputs(enter_standout_mode);
}

void
standoutend(void)
{
	if (exit_standout_mode && enter_standout_mode && !magic_cookie_glitch)
		xputs(exit_standout_mode);
}

void
backsp(void)
{
	if (cursor_left)
		xputs(cursor_left);
	else
		(void) putchar('\b');
	curx--;
}

void
sound_bell(void)
{
	(void) putchar('\007');	/* curx does not change */
	(void) fflush(stdout);
}

void
delay_output(void)
{
	
	/* delay 50 ms - could also use a 'nap'-system call */
	  /* or the usleep call like this :-) */
	usleep(50000);
}

void
cl_eos(void)
{				/* free after Robert Viduya *//* must only be
				 * called with curx = 1 */

	if (clr_eos)
		xputs(clr_eos);
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
