/*	$NetBSD: pwait.c,v 1.6 2017/07/30 20:37:35 dholland Exp $	*/

/*-
 * Copyright (c) 2004-2009, Jilles Tjoelker
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 * 2. Redistributions in binary form must reproduce the
 *    above copyright notice, this list of conditions and
 *    the following disclaimer in the documentation and/or
 *    other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifdef __FBSDID
__FBSDID("$FreeBSD: head/bin/pwait/pwait.c 245506 2013-01-16 18:15:25Z delphij $");
#endif
__RCSID("$NetBSD: pwait.c,v 1.6 2017/07/30 20:37:35 dholland Exp $");

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <assert.h>

static __dead void
usage(void)
{

	fprintf(stderr, "Usage: %s [-isv] [-t <timeout>] <pid> ...\n",
	    getprogname());
	exit(EX_USAGE);
}

/*
 * pwait - wait for processes to terminate
 */
int
main(int argc, char *argv[])
{
	int kq;
	struct kevent *e;
	int verbose = 0, childstatus = 0;
	int opt, duplicate, status, immediately = 0;
	size_t nleft, n, i;
	pid_t pid;
	char *s, *end;
	double timeout = 0;
	struct timespec ts, *tsp;

	setprogname(argv[0]);
	while ((opt = getopt(argc, argv, "ist:v")) != -1) {
		switch (opt) {
		case 'i':
			immediately = 1;
			break;
		case 's':
			childstatus = 1;
			break;
		case 't':
			timeout = atof(optarg);
			if (timeout < 0)
				timeout = 0;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	if (timeout != 0) {
		ts.tv_sec = (time_t)timeout;
		timeout -= (double)ts.tv_sec;
		ts.tv_nsec = (long)(timeout * 1000000000L);
		while (ts.tv_nsec < 0) {
			ts.tv_sec--;
			ts.tv_nsec += 1000000000L;
		}
		tsp = &ts;
	} else
		tsp = NULL;

	kq = kqueue();
	if (kq == -1)
		err(EXIT_FAILURE, "kqueue");

	e = malloc((size_t)argc * sizeof(*e));
	if (e == NULL)
		err(EXIT_FAILURE, "malloc");
	nleft = 0;
	for (n = 0; n < (size_t)argc; n++) {
		long pidl;
		s = argv[n];
		if (!strncmp(s, "/proc/", 6)) /* Undocumented Solaris compat */
			s += 6;
		errno = 0;
		pidl = strtol(s, &end, 10);
		if (pidl < 0 || *end != '\0' || errno != 0) {
			warnx("%s: bad process id", s);
			continue;
		}
		pid = (pid_t)pidl;
		duplicate = 0;
		for (i = 0; i < nleft; i++)
			if (e[i].ident == (uintptr_t)pid)
				duplicate = 1;
		if (!duplicate) {
			EV_SET(e + nleft, (uintptr_t)pid, EVFILT_PROC, EV_ADD,
			    NOTE_EXIT, 0, 0);
			if (kevent(kq, e + nleft, 1, NULL, 0, NULL) == -1)
				warn("%jd", (intmax_t)pid);
			else
				nleft++;
		}
	}

	while (nleft > 0) {
		int rv;

		switch (rv = kevent(kq, NULL, 0, e, nleft, tsp)) {
		case 0:
			if (verbose)
				printf("timed out\n");
			if (childstatus)
				return 255;
			return EX_OK;
		case -1:
			err(EXIT_FAILURE, "kevent");
		default:
			n = (size_t)rv;
			break;
		}

		assert(n > 0);
		for (i = 0; i < n; i++) {
			status = (int)e[i].data;
			if (verbose) {
				if (WIFEXITED(status))
					printf("%ld: exited with status %d.\n",
					    (long)e[i].ident,
					    WEXITSTATUS(status));
				else if (WIFSIGNALED(status))
					printf("%ld: killed by signal %d.\n",
					    (long)e[i].ident,
					    WTERMSIG(status));
				else
					printf("%ld: terminated.\n",
					    (long)e[i].ident);
			}
			if (childstatus)
				return status;
		}
		nleft -= n;
		if (immediately)
			break;
	}

	return EX_OK;
}
