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
/*static char sccsid[] = "from: @(#)initscr.c	5.7 (Berkeley) 8/23/92";*/
static char rcsid[] = "$Id: initscr.c,v 1.3 1993/08/07 05:48:55 mycroft Exp $";
#endif	/* not lint */

#include <curses.h>
#include <signal.h>
#include <stdlib.h>

/*
 * initscr --
 *	Initialize the current and standard screen.
 */
WINDOW *
initscr()
{
	register char *sp;

#ifdef DEBUG
	__TRACE("initscr\n");
#endif
	if (My_term) {
		if (setterm(Def_term) == ERR)
			return (NULL);
	} else {
		gettmode();
		if ((sp = getenv("TERM")) == NULL)
			sp = Def_term;
		if (setterm(sp) == ERR)
			return (NULL);
#ifdef DEBUG
		__TRACE("initscr: term = %s\n", sp);
#endif
	}
	tputs(TI, 0, __cputchar);
	tputs(VS, 0, __cputchar);
	(void)signal(SIGTSTP, tstp);
	if (curscr != NULL) {
#ifdef DEBUG
		__TRACE("initscr: curscr = 0%o\n", curscr);
#endif
		delwin(curscr);
	}
#ifdef DEBUG
	__TRACE("initscr: LINES = %d, COLS = %d\n", LINES, COLS);
#endif
	if ((curscr = newwin(LINES, COLS, 0, 0)) == ERR)
		return (NULL);
	clearok(curscr, 1);
	curscr->_flags &= ~_FULLLINE;
	if (stdscr != NULL) {
#ifdef DEBUG
		__TRACE("initscr: stdscr = 0%o\n", stdscr);
#endif
		delwin(stdscr);
	}
	return(stdscr = newwin(LINES, COLS, 0, 0));
}
