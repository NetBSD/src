/*	$NetBSD: teach.c,v 1.21.12.2 2014/08/20 00:00:22 tls Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif				/* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)teach.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: teach.c,v 1.21.12.2 2014/08/20 00:00:22 tls Exp $");
#endif
#endif				/* not lint */

#include "back.h"
#include "tutor.h"

static const char *const contin[] = {
	"",
	0
};

int
main(int argc __unused, char *argv[])
{
	int     i;
	struct move mmstore, *mm;

	/* revoke setgid privileges */
	setgid(getgid());

	signal(SIGINT, getout);
	if (tcgetattr(0, &old) == -1)	/* get old tty mode */
		errexit("teachgammon(gtty)");
	noech = old;
	noech.c_lflag &= ~ECHO;
	raw = noech;
	raw.c_lflag &= ~ICANON;	/* set up modes */
	ospeed = cfgetospeed(&old);	/* for termlib */
	tflag = getcaps(getenv("TERM"));

	/* need this now beceause getarg() may try to load a game */
	mm = &mmstore;
	move_init(mm);
	while (*++argv != 0)
		getarg(mm, &argv);
	if (tflag) {
		noech.c_oflag &= ~(ONLCR | OXTABS);
		raw.c_oflag &= ~(ONLCR | OXTABS);
		clear();
	}
	wrtext(hello);
	wrtext(list);
	i = wrtext(contin);
	if (i == 0)
		i = 2;
	init();
	while (i)
		switch (i) {
		case 1:
			leave();

		case 2:
			if ((i = wrtext(intro1)) != 0)
				break;
			wrboard();
			if ((i = wrtext(intro2)) != 0)
				break;

		case 3:
			if ((i = wrtext(moves)) != 0)
				break;

		case 4:
			if ((i = wrtext(removepiece)) != 0)
				break;

		case 5:
			if ((i = wrtext(hits)) != 0)
				break;

		case 6:
			if ((i = wrtext(endgame)) != 0)
				break;

		case 7:
			if ((i = wrtext(doubl)) != 0)
				break;

		case 8:
			if ((i = wrtext(stragy)) != 0)
				break;

		case 9:
			if ((i = wrtext(prog)) != 0)
				break;

		case 10:
			if ((i = wrtext(lastch)) != 0)
				break;
		}
	tutor(mm);
	/* NOTREACHED */
	return (0);
}

void
leave(void)
{
	if (tflag)
		clear();
	else
		writec('\n');
	fixtty(&old);
	execl(EXEC, "backgammon", "-n", args[0]?args:0, (char *) 0);
	writel("Help! Backgammon program is missing\007!!\n");
	exit(1);
}
