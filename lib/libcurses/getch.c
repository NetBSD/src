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
/*static char sccsid[] = "from: @(#)getch.c	5.7 (Berkeley) 8/23/92";*/
static char rcsid[] = "$Id: getch.c,v 1.4 1993/08/07 05:48:52 mycroft Exp $";
#endif	/* not lint */

#include <curses.h>

/*
 * wgetch --
 *	Read in a character from the window.
 */
int
wgetch(win)
	register WINDOW *win;
{
	register int inp, weset;

	if (!win->_scroll && (win->_flags & _FULLWIN)
	    && win->_curx == win->_maxx - 1 && win->_cury == win->_maxy - 1)
		return (ERR);
#ifdef DEBUG
	__TRACE("wgetch: __echoit = %d, __rawmode = %d\n",
	    __echoit, __rawmode);
#endif
	if (__echoit && !__rawmode) {
		cbreak();
		weset = 1;
	} else
		weset = 0;

	inp = getchar();
#ifdef DEBUG
	__TRACE("wgetch got '%s'\n", unctrl(inp));
#endif
	if (__echoit) {
		mvwaddch(curscr,
		    win->_cury + win->_begy, win->_curx + win->_begx, inp);
		waddch(win, inp);
	}
	if (weset)
		nocbreak();
	return (inp);
}
