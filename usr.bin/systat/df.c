/*	$NetBSD: df.c,v 1.4 2005/12/24 21:14:50 matt Exp $	*/

/*-
 * Copyright (c) 1980, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2005 
 *	Hubert Feyrer <hubert@feyrer.de>
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
__RCSID("$NetBSD: df.c,v 1.4 2005/12/24 21:14:50 matt Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mount.h>

#include <curses.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "systat.h"
#include "extern.h"

static int nfss;
static struct statvfs *fss;
static const char *nodisplay[] = {"procfs", "kernfs", "ptyfs", "null", NULL };
static int displayall=0;


WINDOW *
opendf(void)
{
	return (subwin(stdscr, -1, 0, 5, 0));
}

void
closedf(WINDOW *w)
{
	if (w == NULL)
		return;
	wclear(w);
	wrefresh(w);
	delwin(w);
}


void
showdf(void)
{
	int i, j, skip;
	char s[MNAMELEN];
	char s2[MNAMELEN];
	float pct;
	int64_t used, bsize, bavail, availblks;
	int y;
	
	y=2; /* at what line to start displaying */
	for (i=0; i<nfss; i++) {
		skip = 0;
		for(j=0; nodisplay[j] != NULL; j++) {
			if (strcmp(nodisplay[j],
				   fss[i].f_fstypename) == 0) {
				skip=1;
				break;
			}
		}

		if (displayall || !skip) {
			wmove(wnd, y, 0);
			wclrtoeol(wnd);
			
			snprintf(s, sizeof(s), "%s", fss[i].f_mntonname);
			mvwaddstr(wnd, y, 0, s);
			
			used = fss[i].f_blocks - fss[i].f_bfree;
			bavail = fss[i].f_bavail;
			availblks = bavail + used;
			bsize = fss[i].f_frsize;
			
			if (availblks == 0) {
				pct = 1.0;    /* full pseudo-disk */
			} else {
				pct = (1.0 * used) / availblks;
			}
			
#define FREELEN 7
			humanize_number(s2, FREELEN, bavail*bsize,
					" |", HN_AUTOSCALE,
					HN_B | HN_NOSPACE | HN_DECIMAL);
			snprintf(s, sizeof(s), "%*s", FREELEN, s2);
			mvwaddstr(wnd, y, 25-FREELEN, s);
#undef FREELEN
			
			mvwhline(wnd, y, 25, 'X', (int)(51*pct));
			
			y++;
		}
	}
	wmove(wnd, y, 0);
	wclrtobot(wnd);
}

int
initdf(void)
{
  	nfss = getmntinfo(&fss, MNT_NOWAIT);
	if (nfss == 0) {
		error("init: getmntinfo error");
		return(0);
	}

	mvwaddstr(wnd, 0, 0, "Disk free %age:");
	return(1);
}

void
fetchdf(void)
{
  	nfss = getmntinfo(&fss, MNT_NOWAIT);
	if (nfss == 0) {
		error("fetch: getmntinfo error");
		return;
	}
}

void
labeldf(void)
{
	wmove(wnd, 0, 0);
	wclrtoeol(wnd);
	mvwaddstr(wnd, 0, 0, "Filesystem");
	mvwaddstr(wnd, 0, 18, "Avail");
	mvwaddstr(wnd, 0, 26, "Capacity");
	mvwaddstr(wnd, 1, 25, "/0%  /10% /20% /30% /40% /50% /60% /70% /80% /90% /100%");
}

void
df_all(char *args)
{
	displayall=1;
}

void
df_some(char *args)
{
	displayall=0;
}

