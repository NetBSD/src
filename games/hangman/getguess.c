/*	$NetBSD: getguess.c,v 1.8.54.1 2012/10/30 18:58:23 yamt Exp $	*/

/*
 * Copyright (c) 1983, 1993
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
static char sccsid[] = "@(#)getguess.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: getguess.c,v 1.8.54.1 2012/10/30 18:58:23 yamt Exp $");
#endif
#endif /* not lint */

#include <sys/ttydefaults.h>
#include "hangman.h"

/*
 * getguess:
 *	Get another guess
 */
void
getguess(void)
{
	int i;
	int ch;
	bool correct;

	leaveok(stdscr, FALSE);
	for (;;) {
		move(PROMPTY, PROMPTX + sizeof "Guess: ");
		refresh();
		ch = readch();
		if (isalpha(ch)) {
			if (isupper(ch))
				ch = tolower(ch);
			if (Guessed[ch - 'a'])
				mvprintw(MESGY, MESGX, "Already guessed '%c'",
				    ch);
			else
				break;
		} else
			if (ch == CTRL('D'))
				die(0);
			else
				mvprintw(MESGY, MESGX,
				    "Not a valid guess: '%s'", unctrl(ch));
	}
	leaveok(stdscr, TRUE);
	move(MESGY, MESGX);
	clrtoeol();

	Guessed[ch - 'a'] = true;
	correct = false;
	for (i = 0; Word[i] != '\0'; i++)
		if (Word[i] == ch) {
			Known[i] = ch;
			correct = true;
		}
	if (!correct)
		Errors++;
}
/*
 * readch;
 *	Read a character from the input
 */
int
readch(void)
{
	int cnt;
	char ch;

	cnt = 0;
	for (;;) {
		if (read(0, &ch, sizeof ch) <= 0) {
			if (++cnt > 100)
				die(0);
		} else
			if (ch == CTRL('L')) {
				wrefresh(curscr);
			} else
				return ch;
	}
}
