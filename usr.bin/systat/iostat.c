/*	$NetBSD: iostat.c,v 1.26 2003/02/01 19:12:30 dsl Exp $	*/

/*
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
static char sccsid[] = "@(#)iostat.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: iostat.c,v 1.26 2003/02/01 19:12:30 dsl Exp $");
#endif /* not lint */

#include <sys/param.h>

#include <string.h>

#include "systat.h"
#include "extern.h"
#include "dkstats.h"

static  int linesperregion;
static  double etime;
static  int numbers = 0;		/* default display bar graphs */
static  int secs = 0;			/* default seconds shown */
static  int read_write = 0;		/* default read/write shown */

static int barlabels(int);
static void histogram(double, int, double);
static int numlabels(int);
static int stats(int, int, int);
static void stat1(int, int);


WINDOW *
openiostat(void)
{

	return (subwin(stdscr, LINES-1-5, 0, 5, 0));
}

void
closeiostat(WINDOW *w)
{

	if (w == NULL)
		return;
	wclear(w);
	wrefresh(w);
	delwin(w);
}

int
initiostat(void)
{

	dkinit(1);
	dkreadstats();
	return(1);
}

void
fetchiostat(void)
{

	if (dk_ndrive == 0)
		return;
	dkreadstats();
}

#define	INSET	14

void
labeliostat(void)
{
	int row;

	if (dk_ndrive == 0) {
		error("No drives defined.");
		return;
	}
	row = 0;
	wmove(wnd, row, 0); wclrtobot(wnd);
	mvwaddstr(wnd, row++, INSET,
	    "/0   /10  /20  /30  /40  /50  /60  /70  /80  /90  /100");
	mvwaddstr(wnd, row++, 0, "    cpu  user|");
	mvwaddstr(wnd, row++, 0, "         nice|");
	mvwaddstr(wnd, row++, 0, "       system|");
	mvwaddstr(wnd, row++, 0, "    interrupt|");
	mvwaddstr(wnd, row++, 0, "         idle|");
	if (numbers)
		row = numlabels(row + 1);
	else
		row = barlabels(row + 1);
}

static int
numlabels(int row)
{
	int i, col, regions, ndrives;

#define COLWIDTH	(read_write ? 24 : 14)
#define DRIVESPERLINE	((getmaxx(wnd) - 1) / COLWIDTH)
	for (ndrives = 0, i = 0; i < dk_ndrive; i++)
		if (cur.dk_select[i])
			ndrives++;
	regions = howmany(ndrives, DRIVESPERLINE);
	/*
	 * Deduct -regions for blank line after each scrolling region.
	 */
	linesperregion = (getmaxy(wnd) - row - regions) / regions;
	/*
	 * Minimum region contains space for two
	 * label lines and one line of statistics.
	 */
	if (linesperregion < 3)
		linesperregion = 3;
	col = 0;
	for (i = 0; i < dk_ndrive; i++)
		if (cur.dk_select[i]) {
			if (col + COLWIDTH > getmaxx(wnd)) {
				col = 0, row += linesperregion + 1;
				if (row > getmaxy(wnd) - (linesperregion + 1))
					break;
			}
			mvwprintw(wnd, row, col + 4, "%s%s",
			    cur.dk_name[i], read_write ? "       (write)" : "");
			if (read_write)
				mvwaddstr(wnd, row + 1, col,
				    "kBps r/s  sec kBps w/s");
			else
				mvwaddstr(wnd, row + 1, col, "kBps tps  sec");
			col += COLWIDTH;
		}
	if (col)
		row += linesperregion + 1;
	return (row);
}

static int
barlabels(int row)
{
	int i;

	mvwaddstr(wnd, row++, INSET,
	    "/0   /10  /20  /30  /40  /50  /60  /70  /80  /90  /100");
	linesperregion = 2 + secs + (read_write ? 2 : 0);
	for (i = 0; i < dk_ndrive; i++)
		if (cur.dk_select[i]) {
			if (row > getmaxy(wnd) - linesperregion)
				break;
			mvwprintw(wnd, row++, 0, "%7.7s  kBps|",
			    cur.dk_name[i]);
			mvwaddstr(wnd, row++, 0, "          tps|");
			if (read_write) {
				mvwprintw(wnd, row++, 0, " (write) kBps|");
				mvwaddstr(wnd, row++, 0, "          tps|");
			}
			if (secs)
				mvwaddstr(wnd, row++, 0, "         msec|");
		}
	return (row);
}

void
showiostat(void)
{
	int i, row, col;

	if (dk_ndrive == 0)
		return;
	dkswap();

	etime = cur.cp_etime;
	row = 1;

	/*
	 * Interrupt CPU state not calculated yet.
	 */
	for (i = 0; i < CPUSTATES; i++)
		stat1(row++, i);
	if (!numbers) {
		row += 2;
		for (i = 0; i < dk_ndrive; i++)
			if (cur.dk_select[i]) {
				if (row > getmaxy(wnd) - linesperregion)
					break;
				row = stats(row, INSET, i);
			}
		return;
	}
	col = 0;
	wmove(wnd, row + linesperregion, 0);
	wdeleteln(wnd);
	wmove(wnd, row + 3, 0);
	winsertln(wnd);
	for (i = 0; i < dk_ndrive; i++)
		if (cur.dk_select[i]) {
			if (col + COLWIDTH > getmaxx(wnd)) {
				col = 0, row += linesperregion + 1;
				if (row > getmaxy(wnd) - (linesperregion + 1))
					break;
				wmove(wnd, row + linesperregion, 0);
				wdeleteln(wnd);
				wmove(wnd, row + 3, 0);
				winsertln(wnd);
			}
			(void) stats(row + 3, col, i);
			col += COLWIDTH;
		}
}

static int
stats(int row, int col, int dn)
{
	double atime, rwords, wwords;

	/* time busy in disk activity */
	atime = (double)cur.dk_time[dn].tv_sec +
		((double)cur.dk_time[dn].tv_usec / (double)1000000);

	/* # of k transferred */
	rwords = cur.dk_rbytes[dn] / 1024.0;
	wwords = cur.dk_wbytes[dn] / 1024.0;
	if (numbers) {
		if (read_write)
			mvwprintw(wnd, row, col, "%4.0f%4.0f%5.1f %3.0f%4.0f",
			    rwords / etime, cur.dk_rxfer[dn] / etime,
			    atime / etime,
			    wwords / etime, cur.dk_wxfer[dn] / etime);
		else
			mvwprintw(wnd, row, col, "%4.0f%4.0f%5.1f",
			    (rwords + wwords) / etime,
			    (cur.dk_rxfer[dn] + cur.dk_wxfer[dn]) / etime,
			    atime / etime);
		return (row);
	}

	if (read_write) {
		wmove(wnd, row++, col);
		histogram(rwords / etime, 50, 0.5);
		wmove(wnd, row++, col);
		histogram(cur.dk_rxfer[dn] / etime, 50, 0.5);
		wmove(wnd, row++, col);
		histogram(wwords / etime, 50, 0.5);
		wmove(wnd, row++, col);
		histogram(cur.dk_wxfer[dn] / etime, 50, 0.5);
	} else {
		wmove(wnd, row++, col);
		histogram((rwords + wwords) / etime, 50, 0.5);
		wmove(wnd, row++, col);
		histogram((cur.dk_rxfer[dn] + cur.dk_wxfer[dn]) / etime, 50, 0.5);
	}

	if (secs) {
		wmove(wnd, row++, col);
		atime *= 1000;	/* In milliseconds */
		histogram(atime / etime, 50, 0.5);
	}
	return (row);
}

static void
stat1(int row, int o)
{
	int i;
	double time;

	time = 0;
	for (i = 0; i < CPUSTATES; i++)
		time += cur.cp_time[i];
	if (time == 0.0)
		time = 1.0;
	wmove(wnd, row, INSET);
#define CPUSCALE	0.5
	histogram(100.0 * cur.cp_time[o] / time, 50, CPUSCALE);
}

static void
histogram(double val, int colwidth, double scale)
{
	int v = (int)(val * scale + 0.5);
	int factor = 1;
	int y, x;

	while (v > colwidth) {
		v = (v + 5) / 10;
		factor *= 10;
	}
	getyx(wnd, y, x);
	wclrtoeol(wnd);
	whline(wnd, 'X', v);
	if (factor != 1)
		mvwprintw(wnd, y, x + colwidth + 1, "* %d ", factor);
}

void
iostat_bars(char *args)
{
	numbers = 0;
	wclear(wnd);
	labeliostat();
	refresh();
}

void
iostat_numbers(char *args)
{
	numbers = 1;
	wclear(wnd);
	labeliostat();
	refresh();
}

void
iostat_secs(char *args)
{
	secs = !secs;
	wclear(wnd);
	labeliostat();
	refresh();
}

void
iostat_rw(char *args)
{
	read_write = 1;
	wclear(wnd);
	labeliostat();
	refresh();
}

void
iostat_all(char *args)
{
	read_write = 0;
	wclear(wnd);
	labeliostat();
	refresh();
}
