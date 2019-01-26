/* $NetBSD: sleep.c,v 1.24.42.1 2019/01/26 21:58:12 pgoyette Exp $ */

/*
 * Copyright (c) 1988, 1993, 1994
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
__COPYRIGHT("@(#) Copyright (c) 1988, 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)sleep.c	8.3 (Berkeley) 4/2/94";
#else
__RCSID("$NetBSD: sleep.c,v 1.24.42.1 2019/01/26 21:58:12 pgoyette Exp $");
#endif
#endif /* not lint */

#include <ctype.h>
#include <err.h>
#include <locale.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

__dead static void alarmhandle(int);
__dead static void usage(void);

static void report(const time_t, const time_t, const char *const);

static volatile sig_atomic_t report_requested;
static void
report_request(int signo __unused)
{

	report_requested = 1;
}

int
main(int argc, char *argv[])
{
	char *arg, *temp;
	const char *msg;
	double fval, ival, val;
	struct timespec ntime;
	time_t original;
	int ch, fracflag;
	unsigned delay;

	setprogname(argv[0]);
	(void)setlocale(LC_ALL, "");

	(void)signal(SIGALRM, alarmhandle);

	while ((ch = getopt(argc, argv, "")) != -1)
		switch(ch) {
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	/*
	 * Okay, why not just use atof for everything? Why bother
	 * checking if there is a fraction in use? Because the old
	 * sleep handled the full range of integers, that's why, and a
	 * double can't handle a large long. This is fairly useless
	 * given how large a number a double can hold on most
	 * machines, but now we won't ever have trouble. If you want
	 * 1000000000.9 seconds of sleep, well, that's your
	 * problem. Why use an isdigit() check instead of checking for
	 * a period? Because doing it this way means locales will be
	 * handled transparently by the atof code.
	 *
	 * Since fracflag is set for any non-digit, we also fall
	 * into the floating point conversion path if the input
	 * is hex (the 'x' in 0xA is not a digit).  Then if
	 * strtod() handles hex (on NetBSD it does) so will we.
	 * That path is also taken for scientific notation (1.2e+3)
	 * and when the input is simply nonsense.
	 */
	fracflag = 0;
	arg = *argv;
	for (temp = arg; *temp != '\0'; temp++)
		if (!isdigit((unsigned char)*temp)) {
			ch = *temp;
			fracflag++;
		}

	if (fracflag) {
		/*
		 * If we cannot convert the value using the user's locale
		 * then try again using the C locale, so strtod() can always
		 * parse values like 2.5, even if the user's locale uses
		 * a different decimal radix character (like ',')
		 *
		 * (but only if that is the potential problem)
		 */
		val = strtod(arg, &temp);
		if (*temp != '\0')
			val = strtod_l(arg, &temp, LC_C_LOCALE);
		if (val < 0 || temp == arg || *temp != '\0')
			usage();

		ival = floor(val);
		fval = (1000000000 * (val-ival));
		ntime.tv_sec = ival;
		if ((double)ntime.tv_sec != ival)
			errx(1, "requested delay (%s) out of range", arg);
		ntime.tv_nsec = fval;

		if (ntime.tv_sec == 0 && ntime.tv_nsec == 0)
			return EXIT_SUCCESS;	/* was 0.0 or underflowed */
	} else {
		ntime.tv_sec = strtol(arg, &temp, 10);
		if (ntime.tv_sec < 0 || temp == arg || *temp != '\0')
			usage();

		if (ntime.tv_sec == 0)
			return EXIT_SUCCESS;
		ntime.tv_nsec = 0;
	}

	original = ntime.tv_sec;
	if (ntime.tv_nsec != 0)
		msg = " and a bit";
	else
		msg = "";

	signal(SIGINFO, report_request);

	if (ntime.tv_sec <= 10000) {			/* arbitrary */
		while (nanosleep(&ntime, &ntime) != 0) {
			if (report_requested) {
				report(ntime.tv_sec, original, msg);
				report_requested = 0;
			} else
				err(EXIT_FAILURE, "nanosleep failed");
		}
	} else while (ntime.tv_sec > 0) {
		delay = (unsigned int)ntime.tv_sec;

		if ((time_t)delay != ntime.tv_sec || delay > 30 * 86400)
			delay = 30 * 86400;

		ntime.tv_sec -= delay;
		delay = sleep(delay);
		ntime.tv_sec += delay;

		if (delay != 0 && report_requested) {
			report(ntime.tv_sec, original, "");
			report_requested = 0;
		} else
			break;
	}

	return EXIT_SUCCESS;
	/* NOTREACHED */
}

	/* Reporting does not bother with nanoseconds. */
static void
report(const time_t remain, const time_t original, const char * const msg)
{
	if (remain == 0)
		warnx("In the final moments of the original"
		    " %ld%s second%s", (long)original, msg,
		    original == 1 && *msg == '\0' ? "" : "s");
	else if (remain < 2000)
		warnx("Between %ld and %ld seconds left"
		    " out of the original %g%s",
		    (long)remain, (long)remain + 1, (double)original,
		    msg);
	else if ((original - remain) < 100000 && (original-remain) < original/8)
		warnx("Have waited only %d seconds of the original %g",
			(int)(original - remain), (double)original);
	else
		warnx("Approximately %g seconds left out of the original %g",
			(double)remain, (double)original);
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: %s seconds\n", getprogname());
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}

/* ARGSUSED */
static void
alarmhandle(int i)
{
	_exit(EXIT_SUCCESS);
	/* NOTREACHED */
}
