/*	$NetBSD: main.c,v 1.8 1997/12/05 13:38:59 simonb Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
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
 *      This product includes software develooped for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* main sysinst program. */

#include <stdio.h>
#include <signal.h>
#include <curses.h>
#include <unistd.h>

#define MAIN
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

int main(int argc, char **argv);
static void usage(void);
static void inthandler(int);
static void cleanup(void);

static int exit_cleanly = 0;	/* Did we finish nicely? */

int main(int argc, char **argv)
{
	WINDOW *win;
	int ch;

	/* Check for TERM ... */
	if (!getenv("TERM")) {
		(void)fprintf (stderr,
			 "sysinst: environment varible TERM not set.\n");
		exit(1);
	}

	/* argv processing */
	while ((ch  = getopt(argc, argv, "r:")) != -1)
		switch(ch) {
		case 'r':
			/* Release name other than compiled in release. */
			strncpy (rel, optarg, SSTRSIZE);
			break;
		case '?':
		default:
			usage();
		}
	

	/* initialize message window */
	win = newwin(22,78,1,1);
       	msg_window(win);

	/* Watch for SIGINT and clean up */
	(void) signal(SIGINT, inthandler);
	(void) atexit(cleanup);

	/* Menu processing */
	process_menu (MENU_netbsd);
	
	exit_cleanly = 1;
	exit(0);
}
	

/* toplevel menu handler ... */
void toplevel(void)
{
	/* Display banner message in (english, francais, deutche..) */
	msg_display (MSG_hello);

	/* 
	 * Undo any stateful side-effects of previous menu choices.
	 * XXX must be idempotent, since we get run each time the main
	 *     menu is displayed.
	 */
	unwind_mounts();
	/* ... */
}


/* The usage ... */

static void
usage(void)
{
	(void)fprintf (stderr, msg_string(MSG_usage));
	exit(1);
}

/* ARGSUSED */
static void
inthandler(int notused)
{
	/* atexit() wants a void function, so inthandler() just calls cleanup */
	cleanup();
	exit(1);
}

static void
cleanup(void)
{
	endwin();
	unwind_mounts();
	run_prog("/sbin/umount /mnt2 2>/dev/null");
	if (!exit_cleanly)
		fprintf(stderr, "\n\n sysinst terminated.\n");
}
