/*	$NetBSD: keyboard.c,v 1.12 2000/07/05 11:03:22 ad Exp $	*/

/*-
 * Copyright (c) 1980, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)keyboard.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: keyboard.c,v 1.12 2000/07/05 11:03:22 ad Exp $");
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>
#include <curses.h>
#include <signal.h>
#include <termios.h>
#include <stdlib.h>

#include "systat.h"
#include "extern.h"

int
keyboard(void)
{
	char ch, rch, *line;
	int i, linesz;
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);

	linesz = COLS - 2;		/* XXX does not get updated on SIGWINCH */
	if ((line = malloc(linesz)) == NULL) {
		error("malloc failed");
		die(0);
	}

	for (;;) {
		col = 0;
		move(CMDLINE, 0);

		while (col == 0 || (ch != '\r' && ch != '\n')) {
			refresh();
			ch = getch() & 0177;
			if (ch == 0177 && ferror(stdin)) {
				clearerr(stdin);
				continue;
			}
			rch = ch;
			if (col == 0) {
				switch(ch) {
				    case '\n':
				    case '\r':
				    case ' ':
					display(0);
					break;
				    case CTRL('l'):
					sigprocmask(SIG_BLOCK, &set, NULL);
					wrefresh(curscr);
					sigprocmask(SIG_UNBLOCK, &set, NULL);
					break;
				    case CTRL('g'):
					sigprocmask(SIG_BLOCK, &set, NULL);
					status();
					sigprocmask(SIG_UNBLOCK, &set, NULL);
					break;
				    case '?':
				    case 'H':
				    case 'h':
					command("help");
					move(CMDLINE, 0);
					break;
				    case 'Q':
					command("quit");
					break;
				    case ':':
					move(CMDLINE, 0);
					clrtoeol();
					addch(':');
					col++;
					break;
				}
				continue;
			}
			if (ch == erasechar() && col > 0) {
				if (col == 1)
					continue;
				col--;
				goto doerase;
			}
			if (ch == CTRL('w') && col > 0) {
				while (--col >= 0 && isspace(line[col]));
				col++;
				while (--col >= 0 && !isspace(line[col]))
					if (col == 0)
						break;
				col++;
				goto doerase;
			}
			if (ch == killchar() && col > 0) {
				col = 1;
		doerase:
				move(CMDLINE, col);
				clrtoeol();
				continue;
			}
			if (isprint(rch) || rch == ' ') {
				if (col < linesz) {
					line[col] = rch;
					mvaddch(CMDLINE, col, rch);
					col++;
				}
			}
		}
		line[col] = '\0';
		/* pass commands as lowercase */
		for (i = 1; i < col ; i++)
			line[i] = tolower(line[i]);
		command(line + 1);
	}
	/* NOTREACHED */
}
