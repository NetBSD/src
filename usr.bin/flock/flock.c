/*	$NetBSD: flock.c,v 1.7 2013/02/07 13:57:40 tron Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *    from this software without specific prior written permission.
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
__RCSID("$NetBSD: flock.c,v 1.7 2013/02/07 13:57:40 tron Exp $");

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <paths.h>
#include <time.h>

static struct option flock_longopts[] = {
	{ "debug",		no_argument,		0, 'd' },
	{ "help",		no_argument,		0, 'h' },
	{ "nonblock",		no_argument,		0, 'n' },
	{ "nb",			no_argument,		0, 'n' },
	{ "close",		no_argument,		0, 'o' },
	{ "shared",		no_argument,		0, 's' },
	{ "exclusive",		no_argument,		0, 'x' },
	{ "unlock",		no_argument,		0, 'u' },
	{ "verbose",		no_argument,		0, 'v' },
	{ "command",		required_argument,	0, 'c' },
	{ "wait",		required_argument,	0, 'w' },
	{ "timeout",		required_argument,	0, 'w' },
	{ NULL,			0,			0, 0   },
};

static sig_atomic_t timeout_expired;

static __dead void
usage(const char *fmt, ...) 
{
	if (fmt) {
		va_list ap;
		va_start(ap, fmt);
		fprintf(stderr, "%s: ", getprogname());
		vfprintf(stderr, fmt, ap);
		fputc('\n', stderr);
		va_end(ap);
	}

	fprintf(stderr, "Usage: %s [-dnosvx] [-w timeout] lockfile|lockdir "
	    "[-c command]|command ...\n\t%s [-dnsuvx] [-w timeout] lockfd\n",
	    getprogname(), getprogname());
	exit(EXIT_FAILURE);
}

static void
sigalrm(int sig)
{
	timeout_expired++;
}

static const char *
lock2name(int l)
{
	static char buf[1024];
	int nb = l & LOCK_NB;

	l &= ~LOCK_NB;
	if (nb)
		strlcpy(buf, "LOCK_NB|", sizeof(buf));
	else
		buf[0] = '\0';

	switch (l) {
	case LOCK_SH:
		strlcat(buf, "LOCK_SH", sizeof(buf));
		return buf;
	case LOCK_EX:
		strlcat(buf, "LOCK_EX", sizeof(buf));
		return buf;
	case LOCK_UN:
		strlcat(buf, "LOCK_UN", sizeof(buf));
		return buf;
	default:
		snprintf(buf, sizeof(buf), "*%d*", l | nb);
		return buf;
	}
}

static char
lockchar(int l)
{
	switch (l & ~LOCK_NB) {
	case LOCK_SH:
		return 's';
	case LOCK_EX:
		return 'x';
	case LOCK_UN:
		return 'u';
	default:
		return '*';
	}
}

static char *
cmdline(char **av)
{
	char *v = NULL;
	while (*av)
		if (v) {
			if (asprintf(&v, "%s %s", v, *av++) < 0)
				err(EXIT_FAILURE, "malloc");
		} else {
			if ((v = strdup(*av++)) == NULL)
				err(EXIT_FAILURE, "strdup");
		}
	return v;
}

int
main(int argc, char *argv[])
{
	int c;
	int lock = LOCK_EX;
	double timeout = 0;
	int cls = 0;
	int fd = -1;
	int debug = 0;
	int verbose = 0;
	char *mcargv[] = {
	    __UNCONST(_PATH_BSHELL), __UNCONST("-c"), NULL, NULL
	};
	char **cmdargv = NULL, *v;
	timer_t tm;

	setprogname(argv[0]);

	while ((c = getopt_long(argc, argv, "+dnosuvw:x", flock_longopts, NULL))
	    != -1)
		switch (c) {
		case 'd':
			debug++;
			break;
		case 'x':
			if (lock & ~LOCK_NB)
				goto badlock;
			lock |= LOCK_EX;
			break;
		case 'n':
			lock |= LOCK_NB;
			break;
		case 's':
			if (lock & ~LOCK_NB)
				goto badlock;
			lock |= LOCK_SH;
			break;
		case 'u':
			if (lock & ~LOCK_NB)
				goto badlock;
			lock |= LOCK_UN;
			break;
		case 'w':
			timeout = strtod(optarg, NULL);
			break;
		case 'v':
			verbose = 1;
			break;
		case 'o':
			cls = 1;
			break;
		default:
			usage("Invalid option '%c'", c);
		badlock:
			usage("-%c can't be used with -%c", c, lockchar(lock));
		}

	argc -= optind;
	argv += optind;

	switch (argc) {
	case 0:
		usage("Missing lock file argument");
	case 1:
		if (cls)
			usage("Close is valid only for descriptors");
		fd = strtol(argv[0], NULL, 0);	// XXX: error checking
		if (debug) {
			fprintf(stderr, "descriptor %s lock %s\n",
			    argv[0], lock2name(lock));
		}
		break;

	default:
		if ((lock & LOCK_NB) == LOCK_UN)
			usage("Unlock is only valid for descriptors");
		if (strcmp(argv[1], "-c") == 0 ||
		    strcmp(argv[1], "--command") == 0) {
			if (argc == 2)
				usage("Missing argument to %s", strcmp(argv[1],
				    "-c") == 0 ? "-c" : "--command");
			mcargv[2] = argv[2];
			cmdargv = mcargv;
		} else
			cmdargv = argv + 1;
			
		if ((fd = open(argv[0], O_RDONLY)) == -1) {
			if (errno != ENOENT || 
			    (fd = open(argv[0], O_RDWR|O_CREAT, 0600)) == -1)
				err(EXIT_FAILURE, "Cannot open `%s'", argv[0]);
		}
		if (debug) {
			fprintf(stderr, "file %s lock %s command %s ...\n",
			    argv[0], lock2name(lock), v = cmdline(cmdargv));
			free(v);
		}
		break;
	}

	if (timeout) {
		struct sigevent ev;
		struct itimerspec it;
		struct sigaction sa;

		timespecclear(&it.it_interval);
		it.it_value.tv_sec = timeout;
		it.it_value.tv_nsec = (timeout - it.it_value.tv_sec) *
			1000000000;

		memset(&ev, 0, sizeof(ev));
		ev.sigev_notify = SIGEV_SIGNAL;
		ev.sigev_signo = SIGALRM;

		if (timer_create(CLOCK_REALTIME, &ev, &tm) == -1)
			err(EXIT_FAILURE, "timer_create");

		if (timer_settime(tm, TIMER_RELTIME, &it, NULL) == -1)
			err(EXIT_FAILURE, "timer_settime");

		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = sigalrm;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		if (sigaction(SIGALRM, &sa, NULL) == -1)
			err(EXIT_FAILURE, "sigaction");

		if (debug)
			fprintf(stderr, "alarm %g\n", timeout);
	}

	while (flock(fd, lock) == -1) {
		if (errno == EINTR && timeout_expired == 0)
			continue;
		if (verbose)
			err(EXIT_FAILURE, "flock(%d, %s)", fd, lock2name(lock));
		else
			return EXIT_FAILURE;
	}

	if (timeout)
		timer_delete(tm);

	if (cls)
		(void)close(fd);

	if (cmdargv != NULL) {
		execvp(cmdargv[0], cmdargv);
		err(EXIT_FAILURE, "execvp '%s'", v = cmdline(cmdargv));
		free(v);
	}
	return 0;
}
