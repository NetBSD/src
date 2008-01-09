/*	$NetBSD: keyboard.c,v 1.23.12.1 2008/01/09 02:01:05 matt Exp $	*/

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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)keyboard.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: keyboard.c,v 1.23.12.1 2008/01/09 02:01:05 matt Exp $");
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>
#include <signal.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "systat.h"
#include "extern.h"

extern sig_atomic_t needsredraw;

void
keyboard(void)
{
	int ch, rch, col;
	char *line;
	int i, linesz;
	static char help[] = "help";
	static char quit[] = "quit";

	ch = 0;	/* XXX gcc */
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
			for (;;) {
				ch = getch();
				if (!needsredraw)
					break;
				redraw();
			}
			if (ch == ERR) {
				display(SIGALRM);
				continue;
			}
			if (ch == KEY_RESIZE) {
				redraw();
				continue;
			}
			ch &= 0177;
			rch = ch;
			if (col == 0) {
				switch(ch) {
				    case '\n':
				    case '\r':
				    case ' ':
					display(0);
					break;
				    case CTRL('l'):
					wrefresh(curscr);
					break;
				    case CTRL('g'):
					status();
					break;
				    case '?':
				    case 'H':
				    case 'h':
					command(help);
					move(CMDLINE, 0);
					break;
				    case 'Q':
				    case 'q':
					command(quit);
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
			if (ch == '\b' || ch == '\?' || ch == erasechar()) {
				if (col > 0)
					col--;
				goto doerase;
			}
			if (ch == CTRL('w') && col > 0) {
				while (--col >= 0 &&
				    isspace((unsigned char)line[col]))
					continue;
				col++;
				while (--col >= 0 &&
				    !isspace((unsigned char)line[col]))
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
			line[i] = tolower((unsigned char)line[i]);
		command(line + 1);
	}
	/* NOTREACHED */
}
