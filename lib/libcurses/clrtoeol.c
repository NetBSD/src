/*
 * Copyright (c) 1981 Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#ifndef lint
/*static char sccsid[] = "from: @(#)clrtoeol.c	5.5 (Berkeley) 8/23/92";*/
static char rcsid[] = "$Id: clrtoeol.c,v 1.4 1993/08/07 05:48:43 mycroft Exp $";
#endif	/* not lint */

#include <curses.h>

/*
 * wclrtoeol --
 *	Clear up to the end of line.
 */
int
wclrtoeol(win)
	register WINDOW *win;
{
	register int minx, x, y;
	register char *end, *maxx, *sp;

	y = win->_cury;
	x = win->_curx;
	end = &win->_y[y][win->_maxx];
	minx = _NOCHANGE;
	maxx = &win->_y[y][x];
	for (sp = maxx; sp < end; sp++)
		if (*sp != ' ') {
			maxx = sp;
			if (minx == _NOCHANGE)
				minx = sp - win->_y[y];
			*sp = ' ';
		}
#ifdef DEBUG
	__TRACE("CLRTOEOL: minx = %d, maxx = %d, firstch = %d, lastch = %d\n",
	    minx, maxx - win->_y[y], win->_firstch[y], win->_lastch[y]);
#endif
	/* Update firstch and lastch for the line. */
	return (touchline(win, y, win->_curx, win->_maxx - 1));
}
