/*      $NetBSD: blabel.c,v 1.4 2001/01/09 19:01:57 garbled Exp $       */

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Copyright (c) 2000 Tim Rightnour <garbled@netbsd.org>
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/ioctl.h>
#include <limits.h>
#include <curses.h>
#include <cdk/cdk.h>

#include "sushi.h"
#include "handlers.h"
#include "blabel.h"

extern nl_catd catalog;
extern struct winsize ws;
extern char *lang_id;
extern CDKSCREEN *cdkscreen;
extern char **searchpaths;

WINDOW *labelwin;

/* if type=1, only show basic.
   2 = show additional for form manipulation
*/
void
bottom_help(int type)
{
	char *label[10];
	int i, j;

	label[0] = catgets(catalog, 2, 1, "F1=Help");
	label[1] = catgets(catalog, 2, 2, "F2=Refresh");
	label[2] = catgets(catalog, 2, 3, "F3=Cancel");
	label[3] = catgets(catalog, 2, 4, "F4=List");
	label[4] = catgets(catalog, 2, 5, "F5=Reset");
	label[5] = catgets(catalog, 2, 6, "F6=Command");
	label[6] = catgets(catalog, 2, 7, "F7=Edit");
	label[7] = catgets(catalog, 2, 8, "F8=Image");
	label[8] = catgets(catalog, 2, 9, "F9=Shell");
	label[9] = catgets(catalog, 2, 10, "F10=Exit");

	labelwin = subwin(stdscr, 2, ws.ws_col, ws.ws_row-2, 0);
	wattron(labelwin, A_BOLD);

	for (i=0,j=0; i < 10; i+=2, j+=17)
		if (i < 3 || i > 6 || type == 2) {
			mvwaddstr(labelwin, 0, j, label[i]);
			wrefresh(labelwin);
		}
	for (i=1,j=0; i < 10; i+=2, j+=17)
		if (i < 3 || i > 6 || type == 2) {
			mvwaddstr(labelwin, 1, j, label[i]);
			wrefresh(labelwin);
		}

	wattroff(labelwin, A_BOLD);
}

/*ARGSUSED*/
static void
wrap_help(EObjectType cdktype, void *object, void *clientdata, chtype key)
{
	char *p;
	int rc, i;

	if (strcmp((char *)clientdata, "sushi_topmenu") == 0) {
		for (i = 0, rc = -2, p = searchpaths[i]; p != NULL && rc == -2;
		    i++) {
			p = searchpaths[i];   
			rc = simple_lang_handler(p, HELPFILE, handle_help);
		}
		if (rc != -2)
			return;
	} else {
		rc = simple_lang_handler((char *)clientdata, HELPFILE,
		    handle_help);
		if (rc != -2)
			return;
	}
	/* now do something meaningful */
}

/*ARGSUSED*/
static void
do_cancel(EObjectType cdktype, void *object, void *clientdata, chtype key)
{
	CDKSCROLL *scroll = (CDKSCROLL *)object;
	scroll->exitType = vESCAPE_HIT;
}

/*ARGSUSED*/
static void
do_refresh(EObjectType cdktype, void *object, void *clientdata, chtype key)
{
	eraseCDKScreen (cdkscreen);
	refreshCDKScreen (cdkscreen);
}

/*ARGSUSED*/
static void
do_exit(EObjectType cdktype, void *object, void *clientdata, chtype key)
{
	destroyCDKScreen(cdkscreen);
	endCDK();
	exit(EXIT_SUCCESS);
}

/*ARGSUSED*/
void
do_scrdump(EObjectType cdktype, void *object, void *clientdata, chtype key)
{
	char *dumpimg;
	int origx, origy, x, y;
	FILE *file;

	getyx(stdscr, origy, origx);

	dumpimg = malloc(ws.ws_row * ws.ws_col);
	if (dumpimg == NULL)
		bailout("malloc: %s", strerror(errno));

	for (y=0; y < ws.ws_row; y++)
		for (x=0; x < ws.ws_col; x++)
			dumpimg[ws.ws_col*y+x] =
			    mvwinch(curscr, y, x) & A_CHARTEXT;

	file = fopen(DUMPFILE, "w");
	for (y=0; y < ws.ws_row; y++) {
		for (x=0; x < ws.ws_col; x++)
			fprintf(file, "%c", dumpimg[ws.ws_col*y+x]);
		fprintf(file, "\n");
	}
	fclose(file);
}

/*ARGSUSED*/
static void
do_shell(EObjectType cdktype, void *object, void *clientdata, chtype key)
{
	eraseCDKScreen (cdkscreen);
	wclear(stdscr);
	wrefresh(stdscr);
	savetty();
	reset_shell_mode();
	if (getenv("SHELL") != NULL)
		system(getenv("SHELL"));
	else
		system("/bin/sh");
	resetty();
	wclear(stdscr);
	wrefresh(stdscr);
	bottom_help(1);
	eraseCDKScreen (cdkscreen);
	refreshCDKScreen (cdkscreen);
}

void
bind_menu(CDKSCROLL *scroll, char *basedir)
{
	bindCDKObject(vSCROLL, scroll, KEY_F1, wrap_help, basedir);
	bindCDKObject(vSCROLL, scroll, KEY_F2, do_refresh, NULL);
	bindCDKObject(vSCROLL, scroll, KEY_F3, do_cancel, NULL);
	bindCDKObject(vSCROLL, scroll, KEY_F8, do_scrdump, NULL);
	bindCDKObject(vSCROLL, scroll, KEY_F9, do_shell, NULL);
	bindCDKObject(vSCROLL, scroll, KEY_F10, do_exit, NULL);
}
