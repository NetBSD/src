/*	$NetBSD: iostat.c,v 1.66.2.1 2018/04/16 02:00:10 pgoyette Exp $	*/

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
__RCSID("$NetBSD: iostat.c,v 1.66.2.1 2018/04/16 02:00:10 pgoyette Exp $");
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

#define	MAX(a,b)	(((a)>(b))?(a):(b))

#define	ISSET(x, a)	((x) & (a))
#define	SHOW_CPU	(1<<0)
#define	SHOW_TTY	(1<<1)
#define	SHOW_STATS_1	(1<<2)
#define	SHOW_STATS_2	(1<<3)
#define	SHOW_STATS_X	(1<<4)
#define	SHOW_STATS_Y	(1<<5)
#define	SHOW_TOTALS	(1<<7)
#define	SHOW_STATS_ALL	(SHOW_STATS_1 | SHOW_STATS_2 | SHOW_STATS_X | SHOW_STATS_Y)

static void cpustats(void);
static double drive_time(double, int);
static void drive_stats(double);
static void drive_stats2(double);
static void drive_statsx(double);
static void drive_statsy(double);
static void drive_statsy_io(double, double, double);
static void drive_statsy_q(double, double, double, double, double, double);
static void sig_header(int);
static volatile int do_header;
static void header(void);
__dead static void usage(void);
static void display(void);
static int selectdrives(int, char *[]);

int
main(int argc, char *argv[])
{
	int ch, hdrcnt, hdroffset, ndrives, lines;
	struct timespec	tv;
	struct ttysize ts;

	while ((ch = getopt(argc, argv, "Cc:dDITw:xy")) != -1)
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
		case 'I':
			todo |= SHOW_TOTALS;
			break;
		case 'T':
			todo |= SHOW_TTY;
			break;
		case 'w':
			if ((interval = atoi(optarg)) <= 0)
				errx(1, "interval <= 0.");
			break;
		case 'x':
			todo &= ~SHOW_STATS_ALL;
			todo |= SHOW_STATS_X;
			break;
		case 'y':
			todo &= ~SHOW_STATS_ALL;
			todo |= SHOW_STATS_Y;
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

	defdrives = wincols;
	if (ISSET(todo, SHOW_CPU))
		defdrives -= 16;	/* XXX magic number */
	if (ISSET(todo, SHOW_TTY))
		defdrives -= 10;	/* XXX magic number */
	defdrives /= 18;		/* XXX magic number */

	drvinit(0);
	cpureadstats();
	drvreadstats();
	ndrives = selectdrives(argc, argv);
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

	for (hdrcnt = 1;;) {
		if (ISSET(todo, SHOW_STATS_X | SHOW_STATS_Y)) {
			lines = ndrives;
			hdroffset = 3;
		} else {
			lines = 1;
			hdroffset = 4;
		}

		if (do_header || (hdrcnt -= lines) <= 0) {
			do_header = 0;
			header();
			hdrcnt = winlines - hdroffset;
		}

		if (!ISSET(todo, SHOW_TOTALS)) {
			cpuswap();
			drvswap();
			tkswap();
		}

		display();

		if (reps >= 0 && --reps <= 0)
			break;
		nanosleep(&tv, NULL);
		cpureadstats();
		drvreadstats();

		ndrives = selectdrives(argc, argv);
	}
	exit(0);
}

static void
sig_header(int signo)
{
	do_header = 1;
}

static void
header(void)
{
	size_t i;

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
		(void)printf("      tty");

	if (ISSET(todo, SHOW_STATS_1)) {
		for (i = 0; i < ndrive; i++)
			if (cur.select[i])
				(void)printf("        %9.9s ", cur.name[i]);
	}

	if (ISSET(todo, SHOW_STATS_2)) {
		for (i = 0; i < ndrive; i++)
			if (cur.select[i])
				(void)printf("        %9.9s ", cur.name[i]);
	}

	if (ISSET(todo, SHOW_CPU))
		(void)printf("            CPU");

	printf("\n");

					/* Sub-Headers. */
	if (ISSET(todo, SHOW_TTY))
		printf(" tin  tout");

	if (ISSET(todo, SHOW_STATS_1)) {
		for (i = 0; i < ndrive; i++)
			if (cur.select[i]) {
				if (ISSET(todo, SHOW_TOTALS))
					(void)printf("  KB/t  xfr  MB   ");
				else
					(void)printf("  KB/t  t/s  MB/s ");
			}
	}

	if (ISSET(todo, SHOW_STATS_2)) {
		for (i = 0; i < ndrive; i++)
			if (cur.select[i])
				(void)printf("    KB   xfr time ");
	}

	if (ISSET(todo, SHOW_CPU))
		(void)printf(" us ni sy in id");
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
drive_stats(double etime)
{
	size_t dn;
	double atime, dtime, mbps;

	for (dn = 0; dn < ndrive; ++dn) {
		if (!cur.select[dn])
			continue;

		dtime = drive_time(etime, dn);

					/* average Kbytes per transfer. */
		if (cur.rxfer[dn] + cur.wxfer[dn])
			mbps = ((cur.rbytes[dn] + cur.wbytes[dn]) /
			    1024.0) / (cur.rxfer[dn] + cur.wxfer[dn]);
		else
			mbps = 0.0;
		(void)printf(" %5.*f",
		    MAX(0, 3 - (int)floor(log10(fmax(1.0, mbps)))), mbps);

					/* average transfers per second. */
		(void)printf(" %4.0f",
		    (cur.rxfer[dn] + cur.wxfer[dn]) / dtime);

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
		(void)printf(" %5.*f ",
		    MAX(0, 3 - (int)floor(log10(fmax(1.0, mbps)))), mbps);
	}
}

static void
drive_stats2(double etime)
{
	size_t dn;
	double atime, dtime;

	for (dn = 0; dn < ndrive; ++dn) {
		if (!cur.select[dn])
			continue;

		dtime = drive_time(etime, dn);

					/* average kbytes per second. */
		(void)printf(" %5.0f",
		    (cur.rbytes[dn] + cur.wbytes[dn]) / 1024.0 / dtime);

					/* average transfers per second. */
		(void)printf(" %5.0f",
		    (cur.rxfer[dn] + cur.wxfer[dn]) / dtime);

					/* average time busy in drive activity */
		atime = (double)cur.time[dn].tv_sec +
		    ((double)cur.time[dn].tv_usec / (double)1000000);
		(void)printf(" %4.2f ", atime / dtime);
	}
}

static void
drive_statsx(double etime)
{
	size_t dn;
	double atime, dtime, kbps;

	for (dn = 0; dn < ndrive; ++dn) {
		if (!cur.select[dn])
			continue;

		dtime = drive_time(etime, dn);

		(void)printf("%-8.8s", cur.name[dn]);

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
drive_statsy(double etime)
{
	size_t dn;
	double atime, await, abusysum, awaitsum, dtime;

	for (dn = 0; dn < ndrive; ++dn) {
		if (!cur.select[dn])
			continue;

		dtime = drive_time(etime, dn);

		(void)printf("%-8.8s", cur.name[dn]);

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

	ttime = 0;
	for (state = 0; state < CPUSTATES; ++state)
		ttime += cur.cp_time[state];
	if (!ttime)
		ttime = 1.0;
			/* States are generally never 100% and can use %3.0f. */
	for (state = 0; state < CPUSTATES; ++state)
		printf(" %2.0f", 100. * cur.cp_time[state] / ttime);
}

static void
usage(void)
{

	(void)fprintf(stderr, "usage: iostat [-CdDITxy] [-c count] "
	    "[-w wait] [drives]\n");
	exit(1);
}

static void
display(void)
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
		drive_statsx(etime);
		goto out;
	}

	if (ISSET(todo, SHOW_STATS_Y)) {
		drive_statsy(etime);
		goto out;
	}

	if (ISSET(todo, SHOW_TTY))
		printf("%4.0f %5.0f", cur.tk_nin / etime, cur.tk_nout / etime);

	if (ISSET(todo, SHOW_STATS_1)) {
		drive_stats(etime);
	}


	if (ISSET(todo, SHOW_STATS_2)) {
		drive_stats2(etime);
	}


	if (ISSET(todo, SHOW_CPU))
		cpustats();

	(void)printf("\n");

out:
	(void)fflush(stdout);
}

static int
selectdrives(int argc, char *argv[])
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
			++ndrives;
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
		for (i = 0; i < maxdrives; i++) {
			cur.select[i] = 1;

			++ndrives;
			if (!ISSET(todo, SHOW_STATS_X | SHOW_STATS_Y) &&
			    ndrives == defdrives)
				break;
		}
	}

#ifdef BACKWARD_COMPATIBILITY
	if (*argv) {
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
