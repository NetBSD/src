/*	$NetBSD: main.c,v 1.1.1.1.2.2 1997/11/10 19:23:04 thorpej Exp $	*/

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
#include <curses.h>
#include <unistd.h>

#define MAIN
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

int main(int argc, char **argv);
void usage (void);

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
			strncpy (rels, optarg, SSTRSIZE);
			break;
		case '?':
		default:
			usage();
		}
	

	/* initialize message window */
	win = newwin(22,78,1,1);
       	msg_window(win);

	/* Change X.Y release number into XY */
	{	char *t, *r;
		for (t=r=rels; *t; t++)
			if (*t != '.')
				*r++=*t;
		*r = 0;
	}

	/* Menu processing */
	process_menu (MENU_netbsd);
	
	return 0;
}
	

/* The usage ... */

void
usage(void)
{
	(void)fprintf (stderr, msg_string(MSG_usage));
	exit(1);
}
