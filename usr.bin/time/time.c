/*	$NetBSD: time.c,v 1.11 1999/06/05 19:19:19 kleink Exp $	*/

/*
 * Copyright (c) 1987, 1988, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1987, 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)time.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: time.c,v 1.11 1999/06/05 19:19:19 kleink Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int		main __P((int, char **));
static void	usage __P((void));

int
main(argc, argv)
	int argc;
	char **argv;
{
	int pid;
	int ch, status;
	int lflag, portableflag;
	const char *decpt;
	const struct lconv *lconv;
	struct timeval before, after;
	struct rusage ru;

#ifdef __GNUC__		/* XXX: borken gcc */
	(void)&argv;
#endif

	(void)setlocale(LC_ALL, "");

	lflag = portableflag = 0;
	while ((ch = getopt(argc, argv, "lp")) != -1) {
		switch (ch) {
		case 'p':
			portableflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();

	gettimeofday(&before, (struct timezone *)NULL);
	switch(pid = vfork()) {
	case -1:			/* error */
		perror("vfork");
		exit(EXIT_FAILURE);
		/* NOTREACHED */
	case 0:				/* child */
		/* LINTED will return only on failure */
		execvp(*argv, argv);
		perror(*argv);
		_exit((errno == ENOENT) ? 127 : 126);
		/* NOTREACHED */
	}

	/* parent */
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	while (wait3(&status, 0, &ru) != pid);
	gettimeofday(&after, (struct timezone *)NULL);
	if (!WIFEXITED(status))
		fprintf(stderr, "Command terminated abnormally.\n");
	timersub(&after, &before, &after);

	if ((lconv = localeconv()) == NULL ||
	    (decpt = lconv->decimal_point) == NULL)
		decpt = ".";

	if (portableflag) {
		fprintf (stderr, "real %9ld%s%02ld\n", 
			(long)after.tv_sec, decpt, (long)after.tv_usec/10000);
		fprintf (stderr, "user %9ld%s%02ld\n",
			(long)ru.ru_utime.tv_sec, decpt, (long)ru.ru_utime.tv_usec/10000);
		fprintf (stderr, "sys  %9ld%s%02ld\n",
			(long)ru.ru_stime.tv_sec, decpt, (long)ru.ru_stime.tv_usec/10000);
	} else {

		fprintf(stderr, "%9ld%s%02ld real ", 
			(long)after.tv_sec, decpt, (long)after.tv_usec/10000);
		fprintf(stderr, "%9ld%s%02ld user ",
			(long)ru.ru_utime.tv_sec, decpt, (long)ru.ru_utime.tv_usec/10000);
		fprintf(stderr, "%9ld%s%02ld sys\n",
			(long)ru.ru_stime.tv_sec, decpt, (long)ru.ru_stime.tv_usec/10000);
	}

	if (lflag) {
		int hz = (int)sysconf(_SC_CLK_TCK);
		long ticks;

		ticks = hz * (ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) +
		     hz * (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) / 1000000;

		fprintf(stderr, "%10ld  %s\n",
			ru.ru_maxrss, "maximum resident set size");
		fprintf(stderr, "%10ld  %s\n", ticks ? ru.ru_ixrss / ticks : 0,
			"average shared memory size");
		fprintf(stderr, "%10ld  %s\n", ticks ? ru.ru_idrss / ticks : 0,
			"average unshared data size");
		fprintf(stderr, "%10ld  %s\n", ticks ? ru.ru_isrss / ticks : 0,
			"average unshared stack size");
		fprintf(stderr, "%10ld  %s\n",
			ru.ru_minflt, "page reclaims");
		fprintf(stderr, "%10ld  %s\n",
			ru.ru_majflt, "page faults");
		fprintf(stderr, "%10ld  %s\n",
			ru.ru_nswap, "swaps");
		fprintf(stderr, "%10ld  %s\n",
			ru.ru_inblock, "block input operations");
		fprintf(stderr, "%10ld  %s\n",
			ru.ru_oublock, "block output operations");
		fprintf(stderr, "%10ld  %s\n",
			ru.ru_msgsnd, "messages sent");
		fprintf(stderr, "%10ld  %s\n",
			ru.ru_msgrcv, "messages received");
		fprintf(stderr, "%10ld  %s\n",
			ru.ru_nsignals, "signals received");
		fprintf(stderr, "%10ld  %s\n",
			ru.ru_nvcsw, "voluntary context switches");
		fprintf(stderr, "%10ld  %s\n",
			ru.ru_nivcsw, "involuntary context switches");
	}

	exit(WIFEXITED(status) ? WEXITSTATUS(status) : EXIT_FAILURE);
	/* NOTREACHED */
}

static void
usage()
{

	fprintf(stderr, "usage: time [-lp] utility [argument ...]\n");
	exit(EXIT_FAILURE);
}
