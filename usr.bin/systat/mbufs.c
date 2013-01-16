/*	$NetBSD: mbufs.c,v 1.15.6.1 2013/01/16 05:34:08 yamt Exp $	*/

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
static char sccsid[] = "@(#)mbufs.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: mbufs.c,v 1.15.6.1 2013/01/16 05:34:08 yamt Exp $");
#endif /* not lint */

#include <sys/param.h>
#define MBUFTYPES
#include <sys/mbuf.h>
#include <sys/sysctl.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "systat.h"
#include "extern.h"

static struct mbstat *mb;

#define	NNAMES	__arraycount(mbuftypes)

WINDOW *
openmbufs(void)
{

	return (subwin(stdscr, -1, 0, 5, 0));
}

void
closembufs(WINDOW *w)
{

	if (w == NULL)
		return;
	wclear(w);
	wrefresh(w);
	delwin(w);
}

void
labelmbufs(void)
{

	wmove(wnd, 0, 0); wclrtoeol(wnd);
	mvwaddstr(wnd, 0, 10,
	    "/0   /5   /10  /15  /20  /25  /30  /35  /40  /45  /50  /55  /60");
}

void
showmbufs(void)
{
	int i, j, max, idx;
	char buf[10];

	if (mb == 0)
		return;
	for (j = 1; j <= getmaxy(wnd); j++) {
		max = 0, idx = -1; 
		for (i = 0; i < getmaxy(wnd); i++)
			if (mb->m_mtypes[i] > max) {
				max = mb->m_mtypes[i];
				idx = i;
			}
		if (max == 0)
			break;
		if (j >= (int)NNAMES)
			mvwprintw(wnd, j, 0, "%10d", idx);
		else
			mvwprintw(wnd, j, 0, "%-10.10s", &mbuftypes[idx][2]);
		wmove(wnd, j, 10);
		if (max > 60) {
			snprintf(buf, sizeof buf, " %5d", max);
			max = 60;
			while (max--)
				waddch(wnd, 'X');
			waddstr(wnd, buf);
		} else {
			wclrtoeol(wnd);
			whline(wnd, 'X', max);
		}
		mb->m_mtypes[idx] = 0;
	}
	wmove(wnd, j, 0);
	wclrtobot(wnd);
}

int
initmbufs(void)
{
	if (mb == 0)
		mb = calloc(1, sizeof (*mb));
	return(1);
}

void
fetchmbufs(void)
{
	size_t len = sizeof(*mb);
	if (sysctlbyname("kern.mbuf.stats", mb, &len, NULL, 0))
		error("error getting \"kern.mbuf.stats\": %s", strerror(errno));
}
