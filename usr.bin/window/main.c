/*	$NetBSD: main.c,v 1.12 2003/08/07 11:17:27 agc Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Edward Wang at The University of California, Berkeley.
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)main.c	8.2 (Berkeley) 4/2/94";
#else
__RCSID("$NetBSD: main.c,v 1.12 2003/08/07 11:17:27 agc Exp $");
#endif
#endif /* not lint */

#include <err.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defs.h"
#include "window_string.h"
#include "char.h"
#include "local.h"

int	main(int, char **);
void	usage(void);

int
main(int argc, char **argv)
{
	char *p;
	char fflag = 0;
	char dflag = 0;
	char xflag = 0;
	char *cmd = 0;
	char tflag = 0;
	int ch;

	escapec = ESCAPEC;	
	debug = strcmp(getprogname(), "a.out") == 0;
	while ((ch = getopt(argc, argv, "fc:e:tdDx")) != -1) {
		switch (ch) {
		case 'f':
			fflag++;
			break;
		case 'c':
			if (cmd != 0) {
				warnx("Only one -c allowed");
				usage();
			}
			cmd = optarg;
			break;
		case 'e':
			setescape(optarg);
			break;
		case 't':
			tflag++;
			break;
		case 'd':
			dflag++;
			break;
		case 'D':
			debug = !debug;
			break;
		case 'x':
			xflag++;
			break;
		default:
			usage();
		}
	}
	if ((p = getenv("SHELL")) == 0)
		p = _PATH_BSHELL;
	if ((default_shellfile = str_cpy(p)) == 0)
		errx(1, "Out of memory.");
	if ((p = strrchr(default_shellfile, '/')))
		p++;
	else
		p = default_shellfile;
	default_shell[0] = p;
	default_shell[1] = 0;
	default_nline = NLINE;
	default_smooth = 1;
	(void) gettimeofday(&starttime, (struct timezone *)0);
	if (wwinit() < 0)
		errx(1, "%s", wwerror());

#ifdef OLD_TTY
	if (debug)
		wwnewtty.ww_tchars.t_quitc = wwoldtty.ww_tchars.t_quitc;
	if (xflag) {
		wwnewtty.ww_tchars.t_stopc = wwoldtty.ww_tchars.t_stopc;
		wwnewtty.ww_tchars.t_startc = wwoldtty.ww_tchars.t_startc;
	}
#else
	if (debug) {
		wwnewtty.ww_termios.c_cc[VQUIT] =
			wwoldtty.ww_termios.c_cc[VQUIT];
		wwnewtty.ww_termios.c_lflag |= ISIG;
	}
	if (xflag) {
		wwnewtty.ww_termios.c_cc[VSTOP] =
			wwoldtty.ww_termios.c_cc[VSTOP];
		wwnewtty.ww_termios.c_cc[VSTART] =
			wwoldtty.ww_termios.c_cc[VSTART];
		wwnewtty.ww_termios.c_iflag |= IXON;
	}
#endif
	if (debug || xflag)
		(void) wwsettty(0, &wwnewtty);

	if ((cmdwin = wwopen(WWT_INTERNAL, wwbaud > 2400 ? WWO_REVERSE : 0, 1,
			     wwncol, 0, 0, 0)) == 0) {
		wwflush();
		warnx("%s.\r", wwerror());
		goto bad;
	}
	SET(cmdwin->ww_wflags,
	    WWW_MAPNL | WWW_NOINTR | WWW_NOUPDATE | WWW_UNCTRL);
	if ((framewin = wwopen(WWT_INTERNAL, WWO_GLASS|WWO_FRAME, wwnrow,
			       wwncol, 0, 0, 0)) == 0) {
		wwflush();
		warnx("%s.\r", wwerror());
		goto bad;
	}
	wwadd(framewin, &wwhead);
	if ((boxwin = wwopen(WWT_INTERNAL, WWO_GLASS, wwnrow, wwncol, 0, 0, 0))
	    == 0) {
		wwflush();
		warnx("%s.\r", wwerror());
		goto bad;
	}
	fgwin = framewin;

	wwupdate();
	wwflush();
	setvars();

	setterse(tflag);
	setcmd(1);
	if (cmd != 0)
		(void) dolongcmd(cmd, (struct value *)0, 0);
	if (!fflag)
		if (dflag || doconfig() < 0)
			dodefault();
	if (selwin != 0)
		setcmd(0);

	mloop();

bad:
	wwend(1);
	return 0;
}

void
usage(void)
{
	(void) fprintf(stderr,
	    "Usage: %s [-e escape-char] [-c command] [-t] [-f] [-d]\n",
	    getprogname());
	exit(1);
}
