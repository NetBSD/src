/*	$NetBSD: iostat.c,v 1.69 2022/06/18 11:33:13 kre Exp $	*/

/*
 * Copyright (c) 1996 John M. Vinopal
 * All rights reserved.
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
 *      This product includes software developed for the NetBSD Project
 *      by John M. Vinopal.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1986, 1991, 1993
 *      The Regents of the University of California.  All rights reserved.
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
__COPYRIGHT("@(#) Copyright (c) 1986, 1991, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)iostat.c	8.3 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: iostat.c,v 1.69 2022/06/18 11:33:13 kre Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/sched.h>
#include <sys/time.h>

#include <err.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <fnmatch.h>

#include "drvstats.h"

int		hz;
static int	reps, interval;
static int	todo = 0;
static int	defdrives;
static int	winlines = 20;
static int	wincols = 80;

static int *order, ordersize;

static char Line_Marker[] = "________________________________________________";

#define	MAX(a,b)	(((a)>(b))?(a):(b))
#define	MIN(a,b)	(((a)<(b))?(a):(b))

#define	ISSET(x, a)	((x) & (a))
#define	SHOW_CPU	(1<<0)
#define	SHOW_TTY	(1<<1)
#define	SHOW_STATS_1	(1<<2)
#define	SHOW_STATS_2	(1<<3)
#define	SHOW_STATS_X	(1<<4)
#define	SHOW_STATS_Y	(1<<5)
#define	SHOW_UPDATES	(1<<6)
#define	SHOW_TOTALS	(1<<7)
#define	SHOW_NEW_TOTALS	(1<<8)
#define	SUPPRESS_ZERO	(1<<9)

#define	SHOW_STATS_ALL	(SHOW_STATS_1 | SHOW_STATS_2 |	\
			    SHOW_STATS_X | SHOW_STATS_Y)

/*
 * Decide how many screen columns each output statistic is given
 * (these are determined empirically ("looks good to me") and likely
 * will require changes from time to time as technology advances).
 *
 * The odd "+ N" at the end of the summary (total width of stat) definition
 * allows for the gaps between the columns, and is (#data cols - 1).
 * So, tty stats have "in" and "out", 2 columns, so there is 1 extra space,
 * whereas the cpu stats have 5 columns, so 4 extra spaces (etc).
 */
#define	LAYOUT_TTY_IN	4	/* tty input in last interval */
#define	LAYOUT_TTY_TIN	7	/* tty input forever */
#define	LAYOUT_TTY_OUT	5	/* tty output in last interval */
#define	LAYOUT_TTY_TOUT	10	/* tty output forever */
#define	LAYOUT_TTY	(((todo & SHOW_TOTALS)				     \
				? (LAYOUT_TTY_TIN + LAYOUT_TTY_TOUT)	     \
				: (LAYOUT_TTY_IN + LAYOUT_TTY_OUT)) + 1)
#define	LAYOUT_TTY_GAP	0		/* always starts at left margin */

#define	LAYOUT_CPU_USER	2
#define	LAYOUT_CPU_NICE	2
#define	LAYOUT_CPU_SYS	2
#define	LAYOUT_CPU_INT	2
#define	LAYOUT_CPU_IDLE	3
#define	LAYOUT_CPU	(LAYOUT_CPU_USER + LAYOUT_CPU_NICE + LAYOUT_CPU_SYS + \
			    LAYOUT_CPU_INT + LAYOUT_CPU_IDLE + 4)
#define	LAYOUT_CPU_GAP	2

			/* used for:       w/o TOTALS  w TOTALS	*/
#define	LAYOUT_DRIVE_1_XSIZE	5	/*	KB/t	KB/t	*/
#define	LAYOUT_DRIVE_1_RATE	6	/*	t/s		*/
#define	LAYOUT_DRIVE_1_XFER	10	/*		xfr	*/
#define	LAYOUT_DRIVE_1_SPEED	5	/*	MB/s		*/
#define	LAYOUT_DRIVE_1_VOLUME	8	/*		MB	*/
#define	LAYOUT_DRIVE_1_INCR	5	/*		(inc)	*/

#define	LAYOUT_DRIVE_2_XSIZE	7	/*	KB		*/
#define	LAYOUT_DRIVE_2_VOLUME	11	/*		KB	*/
#define	LAYOUT_DRIVE_2_XFR	7	/*	xfr		*/
#define	LAYOUT_DRIVE_2_TXFR	10	/*		xfr	*/
#define	LAYOUT_DRIVE_2_INCR	5	/*		(inc)	*/
#define	LAYOUT_DRIVE_2_TBUSY	9	/*		time	*/
#define	LAYOUT_DRIVE_2_BUSY	5	/*	time		*/

#define	LAYOUT_DRIVE_1	(LAYOUT_DRIVE_1_XSIZE + ((todo & SHOW_TOTALS) ?	       \
			    (LAYOUT_DRIVE_1_XFER + LAYOUT_DRIVE_1_VOLUME +     \
			    ((todo&SHOW_UPDATES)? 2*LAYOUT_DRIVE_1_INCR+2 :0)) \
			  : (LAYOUT_DRIVE_1_RATE + LAYOUT_DRIVE_1_SPEED)) + 3)
#define	LAYOUT_DRIVE_2	(((todo & SHOW_TOTALS) ? (LAYOUT_DRIVE_2_VOLUME +      \
			    LAYOUT_DRIVE_2_TXFR + LAYOUT_DRIVE_2_TBUSY +       \
			    ((todo&SHOW_UPDATES)? 2*LAYOUT_DRIVE_2_INCR+2 : 0))\
			  : (LAYOUT_DRIVE_2_XSIZE + LAYOUT_DRIVE_2_XFR +       \
			     LAYOUT_DRIVE_2_BUSY)) + 3)

#define	LAYOUT_DRIVE_GAP 0	/* Gap included in column, always present */

/* TODO: X & Y stats layouts */

static void cpustats(void);
static double drive_time(double, int);
static void drive_stats(int, double);
static void drive_stats2(int, double);
static void drive_statsx(int, double);
static void drive_statsy(int, double);
static void drive_statsy_io(double, double, double);
static void drive_statsy_q(double, double, double, double, double, double);
static void sig_header(int);
static volatile int do_header;
static void header(int);
__dead static void usage(void);
static void display(int);
static int selectdrives(int, char *[], int);

int
main(int argc, char *argv[])
{
	int ch, hdrcnt, hdroffset, ndrives, lines;
	struct timespec	tv;
	struct ttysize ts;
	long width = -1, height = -1;
	char *ep;

#if 0		/* -i and -u are not currently (sanely) implementable */
	while ((ch = getopt(argc, argv, "Cc:dDH:iITuw:W:xyz")) != -1)
#else
	while ((ch = getopt(argc, argv, "Cc:dDH:ITw:W:xyz")) != -1)
#endif
		switch (ch) {
		case 'c':
			if ((reps = atoi(optarg)) <= 0)
				errx(1, "repetition count <= 0.");
			break;
		case 'C':
			todo |= SHOW_CPU;
			break;
		case 'd':
			todo &= ~SHOW_STATS_ALL;
			todo |= SHOW_STATS_1;
			break;
		case 'D':
			todo &= ~SHOW_STATS_ALL;
			todo |= SHOW_STATS_2;
			break;
		case 'H':
			height = strtol(optarg, &ep, 10);
			if (height < 0 || *ep != '\0')
				errx(1, "bad height (-H) value.");
			height += 2;	/* magic, but needed to be sane */
			break;
#if 0
		case 'i':
			todo |= SHOW_TOTALS | SHOW_NEW_TOTALS;
			break;
#endif
		case 'I':
			todo |= SHOW_TOTALS;
			break;
		case 'T':
			todo |= SHOW_TTY;
			break;
#if 0
		case 'u':
			todo |= SHOW_UPDATES;
			break;
#endif
		case 'w':
			if ((interval = atoi(optarg)) <= 0)
				errx(1, "interval <= 0.");
			break;
		case 'W':
			width = strtol(optarg, &ep, 10);
			if (width < 0 || *ep != '\0')
				errx(1, "bad width (-W) value.");
			break;
		case 'x':
			todo &= ~SHOW_STATS_ALL;
			todo |= SHOW_STATS_X;
			break;
		case 'y':
			todo &= ~SHOW_STATS_ALL;
			todo |= SHOW_STATS_Y;
			break;
		case 'z':
			todo |= SUPPRESS_ZERO;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (!ISSET(todo, SHOW_CPU | SHOW_TTY | SHOW_STATS_ALL))
		todo |= SHOW_CPU | SHOW_TTY | SHOW_STATS_1;
	if (ISSET(todo, SHOW_STATS_X)) {
		todo &= ~(SHOW_CPU | SHOW_TTY | SHOW_STATS_ALL);
		todo |= SHOW_STATS_X;
	}
	if (ISSET(todo, SHOW_STATS_Y)) {
		todo &= ~(SHOW_CPU | SHOW_TTY | SHOW_STATS_ALL | SHOW_TOTALS);
		todo |= SHOW_STATS_Y;
	}

	if (ioctl(STDOUT_FILENO, TIOCGSIZE, &ts) != -1) {
		if (ts.ts_lines)
			winlines = ts.ts_lines;
		if (ts.ts_cols)
			wincols = ts.ts_cols;
	}

	if (height == -1) {
		char *lns = getenv("LINES");

		if (lns == NULL || (height = strtol(lns, &ep, 10)) < 0 ||
		    *ep != '\0')
			height = winlines;
	}
	winlines = height;

	if (width == -1) {
		char *cols = getenv("COLUMNS");

		if (cols == NULL || (width = strtol(cols, &ep, 10)) < 0 ||
		    *ep != '\0')
			width = wincols;
	}
	defdrives = width;
	if (defdrives == 0) {
		defdrives = 5000;	/* anything absurdly big */
	} else {
		if (ISSET(todo, SHOW_CPU))
			defdrives -= LAYOUT_CPU + LAYOUT_CPU_GAP;
		if (ISSET(todo, SHOW_TTY))
			defdrives -= LAYOUT_TTY + LAYOUT_TTY_GAP;
		if (ISSET(todo, SHOW_STATS_2))
			defdrives /= LAYOUT_DRIVE_2 + LAYOUT_DRIVE_GAP;
		else
			defdrives /= LAYOUT_DRIVE_1 + LAYOUT_DRIVE_GAP;
	}

	drvinit(0);
	cpureadstats();
	drvreadstats();
	ordersize = 0;
	ndrives = selectdrives(argc, argv, 1);
	if (ndrives == 0) {
		/* No drives are selected.  No need to show drive stats. */
		todo &= ~SHOW_STATS_ALL;
		if (todo == 0)
			errx(1, "no drives");
	}
	tv.tv_sec = interval;
	tv.tv_nsec = 0;

	/* print a new header on sigcont */
	(void)signal(SIGCONT, sig_header);
	do_header = 1;

	for (hdrcnt = 1;;) {
		if (ISSET(todo, SHOW_STATS_X | SHOW_STATS_Y)) {
			lines = ndrives;
			hdroffset = 3;
		} else {
			lines = 1;
			hdroffset = 4;
		}

		if (do_header || (winlines != 0 && (hdrcnt -= lines) <= 0)) {
			do_header = 0;
			header(ndrives);
			hdrcnt = winlines - hdroffset;
		}

		if (!ISSET(todo, SHOW_TOTALS) || ISSET(todo, SHOW_NEW_TOTALS)) {
			cpuswap();
			drvswap();
			tkswap();
			todo &= ~SHOW_NEW_TOTALS;
		}

		display(ndrives);

		if (reps >= 0 && --reps <= 0)
			break;
		nanosleep(&tv, NULL);
		cpureadstats();
		drvreadstats();

		ndrives = selectdrives(argc, argv, 0);
	}
	exit(0);
}

static void
sig_header(int signo)
{
	do_header = 1;
}

static void
header(int ndrives)
{
	int i;

					/* Main Headers. */
	if (ISSET(todo, SHOW_STATS_X)) {
		if (ISSET(todo, SHOW_TOTALS)) {
			(void)printf(
			    "device  read KB/t    xfr   time     MB  ");
			(void)printf(" write KB/t    xfr   time     MB\n");
		} else {
			(void)printf(
			    "device  read KB/t    r/s   time     MB/s");
			(void)printf(" write KB/t    w/s   time     MB/s\n");
		}
		return;
	}

	if (ISSET(todo, SHOW_STATS_Y)) {
		(void)printf("device  read KB/t    r/s     MB/s write KB/t    w/s     MB/s");
		(void)printf("   wait   actv  wsvc_t  asvc_t  wtime   time");
		(void)printf("\n");
		return;
	}

	if (ISSET(todo, SHOW_TTY))
		(void)printf("%*s", LAYOUT_TTY_GAP + LAYOUT_TTY, "tty");

	if (ISSET(todo, SHOW_STATS_1)) {
		for (i = 0; i < ndrives; i++) {
			char *dname = cur.name[order[i]];
			int dnlen = (int)strlen(dname);

			printf(" ");	/* always a 1 column gap */
			if (dnlen < LAYOUT_DRIVE_1 - 6)
				printf("|%-*.*s ",
				    (LAYOUT_DRIVE_1 - 1 - dnlen - 1) / 2 - 1,
				    (LAYOUT_DRIVE_1 - 1 - dnlen - 1) / 2 - 1,
				    Line_Marker);
			printf("%*.*s", ((dnlen >= LAYOUT_DRIVE_1 - 6) ?
			    MIN(MAX((LAYOUT_DRIVE_1 - dnlen) / 2, 0),
				LAYOUT_DRIVE_1) : 0),
			    LAYOUT_DRIVE_1, dname);
			if (dnlen < LAYOUT_DRIVE_1 - 6)
				printf(" %*.*s|",
				    (LAYOUT_DRIVE_1 - 1 - dnlen - 2) / 2 - 1,
				    (LAYOUT_DRIVE_1 - 1 - dnlen - 2) / 2 - 1,
				    Line_Marker);
		}
	}

	if (ISSET(todo, SHOW_STATS_2)) {
		for (i = 0; i < ndrives; i++) {
			char *dname = cur.name[order[i]];
			int dnlen = (int)strlen(dname);

			printf(" ");	/* always a 1 column gap */
			if (dnlen < LAYOUT_DRIVE_2 - 6)
				printf("|%-*.*s ",
				    (LAYOUT_DRIVE_2 - 1 - dnlen - 1) / 2 - 1,
				    (LAYOUT_DRIVE_2 - 1 - dnlen - 1) / 2 - 1,
				    Line_Marker);
			printf("%*.*s", ((dnlen >= LAYOUT_DRIVE_1 - 6) ?
			    MIN(MAX((LAYOUT_DRIVE_2 - dnlen) / 2, 0),
				LAYOUT_DRIVE_2) : 0),
			    LAYOUT_DRIVE_1, dname);
			if (dnlen < LAYOUT_DRIVE_2 - 6)
				printf(" %*.*s|",
				    (LAYOUT_DRIVE_2 - 1 - dnlen - 2) / 2 - 1,
				    (LAYOUT_DRIVE_2 - 1 - dnlen - 2) / 2 - 1,
				    Line_Marker);
		}
	}

	if (ISSET(todo, SHOW_CPU))
		(void)printf("%*s", LAYOUT_CPU + LAYOUT_CPU_GAP, "CPU");

	printf("\n");

					/* Sub-Headers. */
	if (ISSET(todo, SHOW_TTY)) {
		printf("%*s %*s",
		   ((todo&SHOW_TOTALS)?LAYOUT_TTY_TIN:LAYOUT_TTY_IN), "tin",
		   ((todo&SHOW_TOTALS)?LAYOUT_TTY_TOUT:LAYOUT_TTY_OUT), "tout");
	}

	if (ISSET(todo, SHOW_STATS_1)) {
		for (i = 0; i < ndrives; i++) {
			if (ISSET(todo, SHOW_TOTALS)) {
				(void)printf(" %*s %*s %*s",
				    LAYOUT_DRIVE_1_XFER, "xfr",
				    LAYOUT_DRIVE_1_XSIZE, "KB/t",
				    LAYOUT_DRIVE_1_VOLUME, "MB");
			} else {
				(void)printf(" %*s %*s %*s",
				    LAYOUT_DRIVE_1_RATE, "t/s",
				    LAYOUT_DRIVE_1_XSIZE, "KB/t",
				    LAYOUT_DRIVE_1_SPEED, "MB/s");
			}
		}
	}

	if (ISSET(todo, SHOW_STATS_2)) {
		for (i = 0; i < ndrives; i++) {
			if (ISSET(todo, SHOW_TOTALS)) {
				(void)printf(" %*s %*s %*s",
				    LAYOUT_DRIVE_2_TXFR, "xfr",
				    LAYOUT_DRIVE_2_VOLUME, "KB",
				    LAYOUT_DRIVE_2_TBUSY, "time");
			} else {
				(void)printf(" %*s %*s %*s",
				    LAYOUT_DRIVE_2_XFR, "xfr",
				    LAYOUT_DRIVE_2_XSIZE, "KB",
				    LAYOUT_DRIVE_2_BUSY, "time");
			}
		}
	}

	/* should do this properly, but it is such a simple case... */
	if (ISSET(todo, SHOW_CPU))
		(void)printf("  us ni sy in  id");
	printf("\n");
}

static double
drive_time(double etime, int dn)
{
	if (ISSET(todo, SHOW_TOTALS))
		return etime;

	if (cur.timestamp[dn].tv_sec || cur.timestamp[dn].tv_usec) {
		etime = (double)cur.timestamp[dn].tv_sec +
		    ((double)cur.timestamp[dn].tv_usec / (double)1000000);
	}

	return etime;
}

static void
drive_stats(int ndrives, double etime)
{
	int drive;
	double atime, dtime, mbps;
	int c1, c2, c3;

	if (ISSET(todo, SHOW_TOTALS)) {
		c1 = LAYOUT_DRIVE_1_XFER;
		c2 = LAYOUT_DRIVE_1_XSIZE;
		c3 = LAYOUT_DRIVE_1_VOLUME;	
	} else {
		c1 = LAYOUT_DRIVE_1_RATE;
		c2 = LAYOUT_DRIVE_1_XSIZE;
		c3 = LAYOUT_DRIVE_1_SPEED;
	}

	for (drive = 0; drive < ndrives; ++drive) {
		int dn = order[drive];

		if (!cur.select[dn])	/* should be impossible */
			continue;

		if (todo & SUPPRESS_ZERO) {
			if (cur.rxfer[dn] == 0 &&
			    cur.wxfer[dn] == 0 &&
			    cur.rbytes[dn] == 0 &&
			    cur.wbytes[dn] == 0) {
				printf("%*s", c1 + 1 + c2 + 1 + c3 + 1, "");
				continue;
			}
		}

		dtime = drive_time(etime, dn);

					/* average transfers per second. */
		(void)printf(" %*.0f", c1,
		    (cur.rxfer[dn] + cur.wxfer[dn]) / dtime);

					/* average Kbytes per transfer. */
		if (cur.rxfer[dn] + cur.wxfer[dn])
			mbps = ((cur.rbytes[dn] + cur.wbytes[dn]) /
			    1024.0) / (cur.rxfer[dn] + cur.wxfer[dn]);
		else
			mbps = 0.0;
		(void)printf(" %*.*f", c2,
		    MAX(0, 3 - (int)floor(log10(fmax(1.0, mbps)))), mbps);

					/* time busy in drive activity */
		atime = (double)cur.time[dn].tv_sec +
		    ((double)cur.time[dn].tv_usec / (double)1000000);

					/* Megabytes per second. */
		if (atime != 0.0)
			mbps = (cur.rbytes[dn] + cur.wbytes[dn]) /
			    (double)(1024 * 1024);
		else
			mbps = 0;
		mbps /= dtime;
		(void)printf(" %*.*f", c3,
		    MAX(0, 3 - (int)floor(log10(fmax(1.0, mbps)))), mbps);
	}
}

static void
drive_stats2(int ndrives, double etime)
{
	int drive;
	double atime, dtime;
	int c1, c2, c3;

	if (ISSET(todo, SHOW_TOTALS)) {
		c1 = LAYOUT_DRIVE_2_TXFR;
		c2 = LAYOUT_DRIVE_2_VOLUME;
		c3 = LAYOUT_DRIVE_2_TBUSY;	
	} else {
		c1 = LAYOUT_DRIVE_2_XFR;
		c2 = LAYOUT_DRIVE_2_XSIZE;
		c3 = LAYOUT_DRIVE_2_BUSY;
	}

	for (drive = 0; drive < ndrives; ++drive) {
		int dn = order[drive];

		if (!cur.select[dn])		/* should be impossible */
			continue;

		if (todo & SUPPRESS_ZERO) {
			if (cur.rxfer[dn] == 0 &&
			    cur.wxfer[dn] == 0 &&
			    cur.rbytes[dn] == 0 &&
			    cur.wbytes[dn] == 0) {
				printf("%*s", c1 + 1 + c2 + 1 + c3 + 1, "");
				continue;
			}
		}

		dtime = drive_time(etime, dn);

					/* average transfers per second. */
		(void)printf(" %*.0f", c1,
		    (cur.rxfer[dn] + cur.wxfer[dn]) / dtime);

					/* average kbytes per second. */
		(void)printf(" %*.0f", c2,
		    (cur.rbytes[dn] + cur.wbytes[dn]) / 1024.0 / dtime);

					/* average time busy in dn activity */
		atime = (double)cur.time[dn].tv_sec +
		    ((double)cur.time[dn].tv_usec / (double)1000000);
		(void)printf(" %*.2f", c3, atime / dtime);
	}
}

static void
drive_statsx(int ndrives, double etime)
{
	int dn, drive;
	double atime, dtime, kbps;

	for (drive = 0; drive < ndrives; ++drive) {
		dn = order[drive];

		if (!cur.select[dn])	/* impossible */
			continue;

		(void)printf("%-8.8s", cur.name[dn]);

		if (todo & SUPPRESS_ZERO) {
			if (cur.rbytes[dn] == 0 && cur.rxfer[dn] == 0 &&
			    cur.wbytes[dn] == 0 && cur.wxfer[dn] == 0) {
				printf("\n");
				continue;
			}
		}

		dtime = drive_time(etime, dn);

					/* average read Kbytes per transfer */
		if (cur.rxfer[dn])
			kbps = (cur.rbytes[dn] / 1024.0) / cur.rxfer[dn];
		else
			kbps = 0.0;
		(void)printf(" %8.2f", kbps);

					/* average read transfers
					   (per second) */
		(void)printf(" %6.0f", cur.rxfer[dn] / dtime);

					/* time read busy in drive activity */
		atime = (double)cur.time[dn].tv_sec +
		    ((double)cur.time[dn].tv_usec / (double)1000000);
		(void)printf(" %6.2f", atime / dtime);

					/* average read megabytes
					   (per second) */
		(void)printf(" %8.2f",
		    cur.rbytes[dn] / (1024.0 * 1024) / dtime);


					/* average write Kbytes per transfer */
		if (cur.wxfer[dn])
			kbps = (cur.wbytes[dn] / 1024.0) / cur.wxfer[dn];
		else
			kbps = 0.0;
		(void)printf("   %8.2f", kbps);

					/* average write transfers
					   (per second) */
		(void)printf(" %6.0f", cur.wxfer[dn] / dtime);

					/* time write busy in drive activity */
		atime = (double)cur.time[dn].tv_sec +
		    ((double)cur.time[dn].tv_usec / (double)1000000);
		(void)printf(" %6.2f", atime / dtime);

					/* average write megabytes
					   (per second) */
		(void)printf(" %8.2f\n",
		    cur.wbytes[dn] / (1024.0 * 1024) / dtime);
	}
}

static void
drive_statsy_io(double elapsed, double count, double volume)
{
	double kbps;

	/* average Kbytes per transfer */
	if (count)
		kbps = (volume / 1024.0) / count;
	else
		kbps = 0.0;
	(void)printf(" %8.2f", kbps);

	/* average transfers (per second) */
	(void)printf(" %6.0f", count / elapsed);

	/* average megabytes (per second) */
	(void)printf(" %8.2f", volume / (1024.0 * 1024) / elapsed);
}

static void
drive_statsy_q(double elapsed, double busy, double wait, double busysum, double waitsum, double count)
{
	/* average wait queue length */
	(void)printf(" %6.1f", waitsum / elapsed);

	/* average busy queue length */
	(void)printf(" %6.1f", busysum / elapsed);

	/* average wait time */
	(void)printf(" %7.2f", count > 0 ? waitsum / count * 1000.0 : 0.0);

	/* average service time */
	(void)printf(" %7.2f", count > 0 ? busysum / count * 1000.0 : 0.0);

	/* time waiting for drive activity */
	(void)printf(" %6.2f", wait / elapsed);

	/* time busy in drive activity */
	(void)printf(" %6.2f", busy / elapsed);
}

static void
drive_statsy(int ndrives, double etime)
{
	int drive, dn;
	double atime, await, abusysum, awaitsum, dtime;

	for (drive = 0; drive < ndrives; ++drive) {
		dn = order[drive];
		if (!cur.select[dn])	/* impossible */
			continue;

		(void)printf("%-8.8s", cur.name[dn]);

		if (todo & SUPPRESS_ZERO) {
			if (cur.rbytes[dn] == 0 && cur.rxfer[dn] == 0 &&
			    cur.wbytes[dn] == 0 && cur.wxfer[dn] == 0) {
				printf("\n");
				continue;
			}
		}

		dtime = drive_time(etime, dn);

		atime = (double)cur.time[dn].tv_sec +
		    ((double)cur.time[dn].tv_usec / (double)1000000);
		await = (double)cur.wait[dn].tv_sec +
		    ((double)cur.wait[dn].tv_usec / (double)1000000);
		abusysum = (double)cur.busysum[dn].tv_sec +
		    ((double)cur.busysum[dn].tv_usec / (double)1000000);
		awaitsum = (double)cur.waitsum[dn].tv_sec +
		    ((double)cur.waitsum[dn].tv_usec / (double)1000000);

		drive_statsy_io(dtime, cur.rxfer[dn], cur.rbytes[dn]);
		(void)printf("  ");
		drive_statsy_io(dtime, cur.wxfer[dn], cur.wbytes[dn]);
		drive_statsy_q(dtime, atime, await, abusysum, awaitsum, cur.rxfer[dn]+cur.wxfer[dn]);

		(void)printf("\n");
	}
}

static void
cpustats(void)
{
	int state;
	double ttime;

	static int cwidth[CPUSTATES] = {
		LAYOUT_CPU_USER,
		LAYOUT_CPU_NICE,
		LAYOUT_CPU_SYS,
		LAYOUT_CPU_INT,
		LAYOUT_CPU_IDLE
	};

	ttime = 0;
	for (state = 0; state < CPUSTATES; ++state)
		ttime += cur.cp_time[state];
	if (!ttime)
		ttime = 1.0;

	printf("%*s", LAYOUT_CPU_GAP - 1, "");	/* the 1 is the next space */
	for (state = 0; state < CPUSTATES; ++state) {
		if ((todo & SUPPRESS_ZERO) && cur.cp_time[state] == 0) {
			printf(" %*s", cwidth[state], "");
			continue;
		}
		printf(" %*.0f", cwidth[state],
		    100. * cur.cp_time[state] / ttime);
	}
}

static void
usage(void)
{

	(void)fprintf(stderr, "usage: iostat [-CdDITxyz] [-c count] "
	    "[-H height] [-W width] [-w wait] [drives]\n");
	exit(1);
}

static void
display(int ndrives)
{
	double	etime;

	/* Sum up the elapsed ticks. */
	etime = cur.cp_etime;

	/*
	 * If we're showing totals only, then don't divide by the
	 * system time.
	 */
	if (ISSET(todo, SHOW_TOTALS))
		etime = 1.0;

	if (ISSET(todo, SHOW_STATS_X)) {
		drive_statsx(ndrives, etime);
		goto out;
	}

	if (ISSET(todo, SHOW_STATS_Y)) {
		drive_statsy(ndrives, etime);
		goto out;
	}

	if (ISSET(todo, SHOW_TTY))
		printf("%*.0f %*.0f",
		    ((todo & SHOW_TOTALS) ? LAYOUT_TTY_TIN : LAYOUT_TTY_IN),
		    cur.tk_nin / etime,
		    ((todo & SHOW_TOTALS) ? LAYOUT_TTY_TOUT : LAYOUT_TTY_OUT),
		    cur.tk_nout / etime);

	if (ISSET(todo, SHOW_STATS_1)) {
		drive_stats(ndrives, etime);
	}


	if (ISSET(todo, SHOW_STATS_2)) {
		drive_stats2(ndrives, etime);
	}


	if (ISSET(todo, SHOW_CPU))
		cpustats();

	(void)printf("\n");

out:
	(void)fflush(stdout);
}

static int
selectdrives(int argc, char *argv[], int first)
{
	int	i, maxdrives, ndrives, tried;

	/*
	 * Choose drives to be displayed.  Priority goes to (in order) drives
	 * supplied as arguments and default drives.  If everything isn't
	 * filled in and there are drives not taken care of, display the first
	 * few that fit.
	 *
	 * The backward compatibility #ifdefs permit the syntax:
	 *	iostat [ drives ] [ interval [ count ] ]
	 */

#define	BACKWARD_COMPATIBILITY
	for (tried = ndrives = 0; *argv; ++argv) {
#ifdef BACKWARD_COMPATIBILITY
		if (isdigit((unsigned char)**argv))
			break;
#endif
		tried++;
		for (i = 0; i < (int)ndrive; i++) {
			if (fnmatch(*argv, cur.name[i], 0))
				continue;
			cur.select[i] = 1;
			if (ordersize <= ndrives) {
				int *new = realloc(order,
				    (ordersize + 8) * sizeof *order);
				if (new == NULL)
					break;
				ordersize += 8;
				order = new;
			}
			order[ndrives++] = i;
		}

	}

	if (ndrives == 0 && tried == 0) {
		/*
		 * Pick up to defdrives (or all if -x is given) drives
		 * if none specified.
		 */
		maxdrives = (ISSET(todo, SHOW_STATS_X | SHOW_STATS_Y) ||
			     (int)ndrive < defdrives)
			? (int)(ndrive) : defdrives;
		ordersize = maxdrives;
		free(order);
		order = calloc(ordersize, sizeof *order);
		if (order == NULL)
			errx(1, "Insufficient memory");
		for (i = 0; i < maxdrives; i++) {
			cur.select[i] = 1;
			order[i] = i;

			++ndrives;
			if (!ISSET(todo, SHOW_STATS_X | SHOW_STATS_Y) &&
			    ndrives == defdrives)
				break;
		}
	}

#ifdef BACKWARD_COMPATIBILITY
	if (first && *argv) {
		interval = atoi(*argv);
		if (*++argv)
			reps = atoi(*argv);
	}
#endif

	if (interval) {
		if (!reps)
			reps = -1;
	} else
		if (reps)
			interval = 1;

	return (ndrives);
}
