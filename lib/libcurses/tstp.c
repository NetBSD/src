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
/*static char sccsid[] = "from: @(#)tstp.c	5.7 (Berkeley) 8/23/92";*/
static char rcsid[] = "$Id: tstp.c,v 1.4 1993/08/10 02:12:47 mycroft Exp $";
#endif /* not lint */

#include <curses.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

/*
 * tstp --
 *	Handle stop and start signals.
 */
void
tstp(signo)
	int signo;
{
	struct termios save;
	sigset_t set;

	/* Get the current terminal state. */
	if (tcgetattr(STDIN_FILENO, &save))
		return;

	/* Move the cursor to the end of the screen. */
	mvcur(0, COLS - 1, LINES - 1, 0);

	/* End the window. */
	endwin();

	/* Stop ourselves. */
	(void)sigemptyset(&set);
	(void)sigaddset(&set, signo);
	(void)sigprocmask(SIG_UNBLOCK, &set, NULL);
	(void)signal(signo, SIG_DFL);
	(void)kill(0, signo);

	/* Time passes ... */

	/* Reset the signal handler. */
	(void)signal(signo, tstp);

	/* Reset the terminal state. */
	(void)tcsetattr(STDIN_FILENO, TCSADRAIN, &save);

	/* Restart the screen. */
	wrefresh(curscr);
}
