/*	$NetBSD: pl_2.c,v 1.11.40.1 2009/05/13 19:18:05 jym Exp $	*/

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
static char sccsid[] = "@(#)pl_2.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: pl_2.c,v 1.11.40.1 2009/05/13 19:18:05 jym Exp $");
#endif
#endif /* not lint */

#include <signal.h>
#include <unistd.h>
#include "display.h"
#include "extern.h"
#include "player.h"

/*ARGSUSED*/
void
newturn(int n __unused)
{
	repaired = loaded = fired = changed = 0;
	movebuf[0] = '\0';

	alarm(0);
	if (mf->readyL & R_LOADING) {
		if (mf->readyL & R_DOUBLE)
			mf->readyL = R_LOADING;
		else
			mf->readyL = R_LOADED;
	}
	if (mf->readyR & R_LOADING) {
		if (mf->readyR & R_DOUBLE)
			mf->readyR = R_LOADING;
		else
			mf->readyR = R_LOADED;
	}
	if (!hasdriver)
		send_ddead();

	display_hide_prompt();
	if (Sync() < 0)
		leave(LEAVE_SYNC);
	if (!hasdriver)
		leave(LEAVE_DRIVER);
	display_reshow_prompt();

	if (turn % 50 == 0)
		send_alive();
	if (mf->FS && (!mc->rig1 || windspeed == 6))
		send_fs(ms, 0);
	if (mf->FS == 1)
		send_fs(ms, 2);

	if (mf->struck)
		leave(LEAVE_QUIT);
	if (mf->captured != 0)
		leave(LEAVE_CAPTURED);
	if (windspeed == 7)
		leave(LEAVE_HURRICAN);

	display_adjust_view();
	display_redraw();

	signal(SIGALRM, newturn);
	alarm(7);
}

void
play(void)
{
	struct ship *sp;

	for (;;) {
		blockalarm();
		display_redraw();
		unblockalarm();

		switch (sgetch("~ ", (struct ship *)0, 0)) {
		case 14: /* ^N */
			display_scroll_pagedown();
			break;
		case 16: /* ^P */
			display_scroll_pageup();
			break;
		case 'm':
			acceptmove();
			break;
		case 's':
			acceptsignal();
			break;
		case 'g':
			grapungrap();
			break;
		case 'u':
			unfoulplayer();
			break;
		case 'v':
			Msg("%s", version);
			break;
		case 'b':
			acceptboard();
			break;
		case 'f':
			acceptcombat();
			break;
		case 'l':
			loadplayer();
			break;
		case 'c':
			changesail();
			break;
		case 'r':
			repair();
			break;
		case 'B':
			Msg("'Hands to stations!'");
			unboard(ms, ms, 1);	/* cancel DBP's */
			unboard(ms, ms, 0);	/* cancel offense */
			break;
		case '\f':
			centerview();
			display_force_full_redraw();
			break;
		case 'L':
			mf->loadL = L_EMPTY;
			mf->loadR = L_EMPTY;
			mf->readyL = R_EMPTY;
			mf->readyR = R_EMPTY;
			Msg("Broadsides unloaded");
			break;
		case 'q':
			Msg("Type 'Q' to quit");
			break;
		case 'Q':
			leave(LEAVE_QUIT);
			break;
		case 'I':
			foreachship(sp)
				if (sp != ms)
					eyeball(sp);
			break;
		case 'i':
			if ((sp = closestenemy(ms, 0, 1)) == 0)
				Msg("No more ships left.");
			else
				eyeball(sp);
			break;
		case 'C':
			centerview();
			break;
		case 'U':
			upview();
			break;
		case 'D':
		case 'N':
			downview();
			break;
		case 'H':
			leftview();
			break;
		case 'J':
			rightview();
			break;
		case 'F':
			lookout();
			break;
		case 'S':
			dont_adjust = !dont_adjust;
			break;
		}
	}
}
