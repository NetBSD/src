/*	$NetBSD: iostat.c,v 1.37.38.2 2019/01/30 13:46:25 martin Exp $	*/

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
static char sccsid[] = "@(#)iostat.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: iostat.c,v 1.37.38.2 2019/01/30 13:46:25 martin Exp $");
#endif /* not lint */

#include <sys/param.h>

#include <string.h>

#include "systat.h"
#include "extern.h"
#include "drvstats.h"

static  int linesperregion;
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

	return (subwin(stdscr, -1, 0, 5, 0));
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

	drvinit(1);
	cpureadstats();
	drvreadstats();
	return(1);
}

void
fetchiostat(void)
{

	cpureadstats();

	if (ndrive != 0)
		drvreadstats();
}

#define	INSET	14

void
labeliostat(void)
{
	int row;

	if (ndrive == 0) {
		error("No drives defined.");
		return;
	}
	row = 0;
	wmove(wnd, row, 0); wclrtobot(wnd);
	mvwaddstr(wnd, row++, INSET,
	    "/0   /10  /20  /30  /40  /50  /60  /70  /80  /90  /100");
	mvwaddstr(wnd, row++, 0, "    CPU  user|");
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
	int col, regions;
	size_t i, ndrives;

#define COLWIDTH	(9 + secs * 5 + 1 + read_write * 9 + 1)
#define DRIVESPERLINE	((getmaxx(wnd) + 1) / COLWIDTH)
	for (ndrives = 0, i = 0; i < ndrive; i++)
		if (cur.select[i])
			ndrives++;

	regions = howmany(ndrives, DRIVESPERLINE);
	/*
	 * Deduct -regions for blank line after each scrolling region.
	 */
	linesperregion = (getmaxy(wnd) - row - regions + 1) / regions;
	/*
	 * Minimum region contains space for two
	 * label lines and one line of statistics.
	 */
	if (linesperregion < 3)
		linesperregion = 3;
	col = 0;
	for (i = 0; i < ndrive; i++)
		if (cur.select[i]) {
			if (col + COLWIDTH - 1 > getmaxx(wnd)) {
				col = 0, row += linesperregion + 1;
				if (row > getmaxy(wnd) - (linesperregion))
					break;
			}

			mvwprintw(wnd, row, col + 5, "%s", cur.name[i]);

			if (read_write)
				mvwprintw(wnd, row, col + 11 + secs * 5,
				    "(write)");
			mvwprintw(wnd, row + 1, col, " kBps %s",
				read_write ? "r/s" : "tps");
			if (secs)
				waddstr(wnd, "  sec");
			if (read_write)
				waddstr(wnd, "  kBps w/s");
			col += COLWIDTH;
		}
	if (col)
		row += linesperregion + 1;
	return (row);
}

static int
barlabels(int row)
{
	size_t i;

	mvwaddstr(wnd, row++, INSET,
	    "/0   /10  /20  /30  /40  /50  /60  /70  /80  /90  /100");
	linesperregion = 2 + secs + (read_write ? 2 : 0);
	for (i = 0; i < ndrive; i++) {
		if (cur.select[i]) {
			if (row > getmaxy(wnd) - linesperregion)
				break;
			mvwprintw(wnd, row++, 0, "%7.7s  kBps|",
			    cur.name[i]);
			mvwaddstr(wnd, row++, 0, "          tps|");
			if (read_write) {
				mvwprintw(wnd, row++, 0, " (write) kBps|");
				mvwaddstr(wnd, row++, 0, "          tps|");
			}
			if (secs)
				mvwaddstr(wnd, row++, 0, "         msec|");
		}
	}

	return (row);
}

void
showiostat(void)
{
	int row, col;
	size_t i;

	if (ndrive == 0)
		return;
	cpuswap();
	drvswap();

	etime = cur.cp_etime;
	row = 1;

	/*
	 * Interrupt CPU state not calculated yet.
	 */
	for (i = 0; i < CPUSTATES; i++)
		stat1(row++, i);
	if (!numbers) {
		row += 2;
		for (i = 0; i < ndrive; i++)
			if (cur.select[i]) {
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
	for (i = 0; i < ndrive; i++)
		if (cur.select[i]) {
			if (col + COLWIDTH - 1 > getmaxx(wnd)) {
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
	double atime, dtime, rwords, wwords;
	uint64_t rxfer;

	/* elapsed time for disk stats */
	dtime = etime;
	if (cur.timestamp[dn].tv_sec || cur.timestamp[dn].tv_usec)
		dtime = (double)cur.timestamp[dn].tv_sec +
			((double)cur.timestamp[dn].tv_usec / (double)1000000);

	/* time busy in disk activity */
	atime = (double)cur.time[dn].tv_sec +
		((double)cur.time[dn].tv_usec / (double)1000000);

	/* # of k transferred */
	rwords = cur.rbytes[dn] / 1024.0;
	wwords = cur.wbytes[dn] / 1024.0;
	rxfer = cur.rxfer[dn];
	if (!read_write) {
		rwords += wwords;
		rxfer += cur.wxfer[dn];
	}
	if (numbers) {
		mvwprintw(wnd, row, col, "%5.0f%4.0f",
		    rwords / dtime, rxfer / dtime);
		if (secs)
			wprintw(wnd, "%5.1f", atime / dtime);
		if (read_write)
			wprintw(wnd, " %5.0f%4.0f",
			    wwords / dtime, cur.wxfer[dn] / dtime);
		return (row);
	}

	wmove(wnd, row++, col);
	histogram(rwords / dtime, 50, 0.5);
	wmove(wnd, row++, col);
	histogram(rxfer / dtime, 50, 0.5);
	if (read_write) {
		wmove(wnd, row++, col);
		histogram(wwords / dtime, 50, 0.5);
		wmove(wnd, row++, col);
		histogram(cur.wxfer[dn] / dtime, 50, 0.5);
	}

	if (secs) {
		wmove(wnd, row++, col);
		atime *= 1000;	/* In milliseconds */
		histogram(atime / dtime, 50, 0.5);
	}
	return (row);
}

static void
stat1(int row, int o)
{
	size_t i;
	double total_time;

	total_time = 0;
	for (i = 0; i < CPUSTATES; i++)
		total_time += cur.cp_time[i];
	if (total_time == 0.0)
		total_time = 1.0;
	wmove(wnd, row, INSET);
#define CPUSCALE	0.5
	histogram(100.0 * cur.cp_time[o] / total_time, 50, CPUSCALE);
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
	read_write ^= 1;
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
