/*	$NetBSD: update.c,v 1.7 1999/06/06 03:39:11 thorpej Exp $	*/

/*-
 * Copyright (c) 1987, 1990, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1987, 1990, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)update.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: update.c,v 1.7 1999/06/06 03:39:11 thorpej Exp $");
#endif
#endif /* not lint */

#include <sys/time.h>

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

int	main __P((int, char **));
void	mysync __P((int));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct itimerval value;
	int ch;
	struct sigaction sa;
	sigset_t set, oset;

	value.it_interval.tv_sec = 30;
	value.it_interval.tv_usec = 0;

	while ((ch = getopt(argc, argv, "")) != -1) {
		switch (ch) {
		default:
		case '?':
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc) {
		value.it_interval.tv_sec = atoi(argv[0]);
		--argc;
		++argv;
	}

	if (argc)
		usage();

	daemon(0, 0);
	pidfile(NULL);

	sa.sa_handler = mysync;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGALRM, &sa, (struct sigaction *)0) < 0)
		err(1, "sigaction");

	value.it_value = value.it_interval;
	if (setitimer(ITIMER_REAL, &value, NULL) < 0)
		err(1, "setitimer");

	sigemptyset(&set);
	sigprocmask(SIG_BLOCK, &set, &oset);
	for (;;)
		sigsuspend(&oset);
	/* NOTREACHED */
}

void
mysync(n)
	int n;
{

	(void)sync();
}

void
usage()
{

	(void)fprintf(stderr, "usage: update [interval]\n");
	exit(1);
}
