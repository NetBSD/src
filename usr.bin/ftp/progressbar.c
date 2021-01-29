/*	$NetBSD: progressbar.c,v 1.23.2.1 2021/01/29 20:58:19 martin Exp $	*/

/*-
 * Copyright (c) 1997-2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: progressbar.c,v 1.23.2.1 2021/01/29 20:58:19 martin Exp $");
#endif /* not lint */

/*
 * FTP User Program -- Misc support routines
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>

#include "progressbar.h"

#if !defined(NO_PROGRESS)
/*
 * return non-zero if we're the current foreground process
 */
int
foregroundproc(void)
{
	static pid_t pgrp = -1;

	if (pgrp == -1)
		pgrp = getpgrp();

	return (tcgetpgrp(fileno(ttyout)) == pgrp);
}
#endif	/* !defined(NO_PROGRESS) */


static void updateprogressmeter(int);

/*
 * SIGALRM handler to update the progress meter
 */
static void
updateprogressmeter(int dummy)
{
	int oerrno = errno;

	progressmeter(0);
	errno = oerrno;
}

/*
 * List of order of magnitude suffixes, per IEC 60027-2.
 */
#if !defined(NO_PROGRESS) || !defined(STANDALONE_PROGRESS)
static const char * const suffixes[] = {
	"",	/* 2^0  (byte) */
	"KiB",	/* 2^10 Kibibyte */
	"MiB",	/* 2^20 Mebibyte */
	"GiB",	/* 2^30 Gibibyte */
	"TiB",	/* 2^40 Tebibyte */
	"PiB",	/* 2^50 Pebibyte */
	"EiB",	/* 2^60 Exbibyte */
#if 0
		/* The following are not necessary for signed 64-bit off_t */
	"ZiB",	/* 2^70 Zebibyte */
	"YiB",	/* 2^80 Yobibyte */
#endif
};
#define NSUFFIXES	(int)(sizeof(suffixes) / sizeof(suffixes[0]))
#endif

/*
 * Display a transfer progress bar if progress is non-zero.
 * SIGALRM is hijacked for use by this function.
 * - Before the transfer, set filesize to size of file (or -1 if unknown),
 *   and call with flag = -1. This starts the once per second timer,
 *   and a call to updateprogressmeter() upon SIGALRM.
 * - During the transfer, updateprogressmeter will call progressmeter
 *   with flag = 0
 * - After the transfer, call with flag = 1
 */
static struct timeval start;
static struct timeval lastupdate;

#define	BUFLEFT	(sizeof(buf) - len)

void
progressmeter(int flag)
{
	static off_t lastsize;
	off_t cursize;
	struct timeval now, wait;
#ifndef NO_PROGRESS
	struct timeval td;
	off_t abbrevsize, bytespersec;
	double elapsed;
	int ratio, i, remaining, barlength;

			/*
			 * Work variables for progress bar.
			 *
			 * XXX:	if the format of the progress bar changes
			 *	(especially the number of characters in the
			 *	`static' portion of it), be sure to update
			 *	these appropriately.
			 */
#endif
#if !defined(NO_PROGRESS) || !defined(STANDALONE_PROGRESS)
	size_t		len;
	char		buf[256];	/* workspace for progress bar */
#endif
#ifndef NO_PROGRESS
#define	BAROVERHEAD	45		/* non `*' portion of progress bar */
					/*
					 * stars should contain at least
					 * sizeof(buf) - BAROVERHEAD entries
					 */
	static const char	stars[] =
"*****************************************************************************"
"*****************************************************************************"
"*****************************************************************************";

#endif

	if (flag == -1) {
		(void)gettimeofday(&start, NULL);
		lastupdate = start;
		lastsize = restart_point;
	}

	(void)gettimeofday(&now, NULL);
	cursize = bytes + restart_point;
	timersub(&now, &lastupdate, &wait);
	if (cursize > lastsize) {
		lastupdate = now;
		lastsize = cursize;
		wait.tv_sec = 0;
	} else {
#ifndef STANDALONE_PROGRESS
		if (quit_time > 0 && wait.tv_sec > quit_time) {
			len = snprintf(buf, sizeof(buf), "\r\n%s: "
			    "transfer aborted because stalled for %lu sec.\r\n",
			    getprogname(), (unsigned long)wait.tv_sec);
			(void)write(fileno(ttyout), buf, len);
			alarmtimer(0);
			(void)xsignal(SIGALRM, SIG_DFL);
			siglongjmp(toplevel, 1);
		}
#endif	/* !STANDALONE_PROGRESS */
	}
	/*
	 * Always set the handler even if we are not the foreground process.
	 */
#ifdef STANDALONE_PROGRESS
	if (progress) {
#else
	if (quit_time > 0 || progress) {
#endif /* !STANDALONE_PROGRESS */
		if (flag == -1) {
			(void)xsignal(SIGALRM, updateprogressmeter);
			alarmtimer(1);		/* set alarm timer for 1 Hz */
		} else if (flag == 1) {
			alarmtimer(0);
			(void)xsignal(SIGALRM, SIG_DFL);
		}
	}
#ifndef NO_PROGRESS
	if (!progress)
		return;
	len = 0;

	/*
	 * print progress bar only if we are foreground process.
	 */
	if (! foregroundproc())
		return;

	len += snprintf(buf + len, BUFLEFT, "\r");
	if (prefix)
	  len += snprintf(buf + len, BUFLEFT, "%s", prefix);
	if (filesize > 0) {
		ratio = (int)((double)cursize * 100.0 / (double)filesize);
		ratio = MAX(ratio, 0);
		ratio = MIN(ratio, 100);
		len += snprintf(buf + len, BUFLEFT, "%3d%% ", ratio);

			/*
			 * calculate the length of the `*' bar, ensuring that
			 * the number of stars won't exceed the buffer size
			 */
		barlength = MIN((int)(sizeof(buf) - 1), ttywidth) - BAROVERHEAD;
		if (prefix)
			barlength -= (int)strlen(prefix);
		if (barlength > 0) {
			i = barlength * ratio / 100;
			len += snprintf(buf + len, BUFLEFT,
			    "|%.*s%*s|", i, stars, (int)(barlength - i), "");
		}
	}

	abbrevsize = cursize;
	for (i = 0; abbrevsize >= 100000 && i < NSUFFIXES; i++)
		abbrevsize >>= 10;
	if (i == NSUFFIXES)
		i--;
	len += snprintf(buf + len, BUFLEFT, " " LLFP("5") " %-3s ",
	    (LLT)abbrevsize,
	    suffixes[i]);

	timersub(&now, &start, &td);
	elapsed = td.tv_sec + (td.tv_usec / 1000000.0);

	bytespersec = 0;
	if (bytes > 0) {
		bytespersec = bytes;
		if (elapsed > 0.0)
			bytespersec /= elapsed;
	}
	for (i = 1; bytespersec >= 1024000 && i < NSUFFIXES; i++)
		bytespersec >>= 10;
	len += snprintf(buf + len, BUFLEFT,
	    " " LLFP("3") ".%02d %.2sB/s ",
	    (LLT)(bytespersec / 1024),
	    (int)((bytespersec % 1024) * 100 / 1024),
	    suffixes[i]);

	if (filesize > 0) {
		if (bytes <= 0 || elapsed <= 0.0 || cursize > filesize) {
			len += snprintf(buf + len, BUFLEFT, "   --:-- ETA");
		} else if (wait.tv_sec >= STALLTIME) {
			len += snprintf(buf + len, BUFLEFT, " - stalled -");
		} else {
			remaining = (int)
			    ((filesize - restart_point) / (bytes / elapsed) -
			    elapsed);
			if (remaining >= 100 * SECSPERHOUR)
				len += snprintf(buf + len, BUFLEFT,
				    "   --:-- ETA");
			else {
				i = remaining / SECSPERHOUR;
				if (i)
					len += snprintf(buf + len, BUFLEFT,
					    "%2d:", i);
				else
					len += snprintf(buf + len, BUFLEFT,
					    "   ");
				i = remaining % SECSPERHOUR;
				len += snprintf(buf + len, BUFLEFT,
				    "%02d:%02d ETA", i / 60, i % 60);
			}
		}
	}
	if (flag == 1)
		len += snprintf(buf + len, BUFLEFT, "\n");
	(void)write(fileno(ttyout), buf, len);

#endif	/* !NO_PROGRESS */
}

#ifndef STANDALONE_PROGRESS
/*
 * Display transfer statistics.
 * Requires start to be initialised by progressmeter(-1),
 * direction to be defined by xfer routines, and filesize and bytes
 * to be updated by xfer routines
 * If siginfo is nonzero, an ETA is displayed, and the output goes to stderr
 * instead of ttyout.
 */
void
ptransfer(int siginfo)
{
	struct timeval now, td, wait;
	double elapsed;
	off_t bytespersec;
	int remaining, hh, i;
	size_t len;

	char buf[256];		/* Work variable for transfer status. */

	if (!verbose && !progress && !siginfo)
		return;

	(void)gettimeofday(&now, NULL);
	timersub(&now, &start, &td);
	elapsed = td.tv_sec + (td.tv_usec / 1000000.0);
	bytespersec = 0;
	if (bytes > 0) {
		bytespersec = bytes;
		if (elapsed > 0.0)
			bytespersec /= elapsed;
	}
	len = 0;
	len += snprintf(buf + len, BUFLEFT, LLF " byte%s %s in ",
	    (LLT)bytes, bytes == 1 ? "" : "s", direction);
	remaining = (int)elapsed;
	if (remaining > SECSPERDAY) {
		int days;

		days = remaining / SECSPERDAY;
		remaining %= SECSPERDAY;
		len += snprintf(buf + len, BUFLEFT,
		    "%d day%s ", days, days == 1 ? "" : "s");
	}
	hh = remaining / SECSPERHOUR;
	remaining %= SECSPERHOUR;
	if (hh)
		len += snprintf(buf + len, BUFLEFT, "%2d:", hh);
	len += snprintf(buf + len, BUFLEFT,
	    "%02d:%02d ", remaining / 60, remaining % 60);

	for (i = 1; bytespersec >= 1024000 && i < NSUFFIXES; i++)
		bytespersec >>= 10;
	if (i == NSUFFIXES)
		i--;
	len += snprintf(buf + len, BUFLEFT, "(" LLF ".%02d %.2sB/s)",
	    (LLT)(bytespersec / 1024),
	    (int)((bytespersec % 1024) * 100 / 1024),
	    suffixes[i]);

	if (siginfo && bytes > 0 && elapsed > 0.0 && filesize >= 0
	    && bytes + restart_point <= filesize) {
		remaining = (int)((filesize - restart_point) /
				  (bytes / elapsed) - elapsed);
		hh = remaining / SECSPERHOUR;
		remaining %= SECSPERHOUR;
		len += snprintf(buf + len, BUFLEFT, "  ETA: ");
		if (hh)
			len += snprintf(buf + len, BUFLEFT, "%2d:", hh);
		len += snprintf(buf + len, BUFLEFT, "%02d:%02d",
		    remaining / 60, remaining % 60);
		timersub(&now, &lastupdate, &wait);
		if (wait.tv_sec >= STALLTIME)
			len += snprintf(buf + len, BUFLEFT, "  (stalled)");
	}
	len += snprintf(buf + len, BUFLEFT, "\n");
	(void)write(siginfo ? STDERR_FILENO : fileno(ttyout), buf, len);
}

/*
 * SIG{INFO,QUIT} handler to print transfer stats if a transfer is in progress
 */
void
psummary(int notused)
{
	int oerrno = errno;

	if (bytes > 0) {
		if (fromatty)
			write(fileno(ttyout), "\n", 1);
		ptransfer(1);
	}
	errno = oerrno;
}
#endif	/* !STANDALONE_PROGRESS */


/*
 * Set the SIGALRM interval timer for wait seconds, 0 to disable.
 */
void
alarmtimer(int wait)
{
	struct itimerval itv;

	itv.it_value.tv_sec = wait;
	itv.it_value.tv_usec = 0;
	itv.it_interval = itv.it_value;
	setitimer(ITIMER_REAL, &itv, NULL);
}

/*
 * Install a non-restartable POSIX signal handler.
 */
sigfunc
xsignal(int sig, sigfunc func)
{
	struct sigaction act, oact;
	act.sa_handler = func;

	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
#if defined(SA_INTERRUPT)		/* SunOS 4.x */
	act.sa_flags = SA_INTERRUPT;
#endif
	if (sigaction(sig, &act, &oact) < 0)
		return (SIG_ERR);
	return (oact.sa_handler);
}
