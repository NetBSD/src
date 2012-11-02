/*	$NetBSD: flock.c,v 1.3 2012/11/02 02:03:18 wiz Exp $	*/

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
__RCSID("$NetBSD: flock.c,v 1.3 2012/11/02 02:03:18 wiz Exp $");

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

struct option flock_longopts[] = {
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

static int verbose = 0;
static sig_atomic_t timeout_expired;

static __dead void usage(void) 
{
	fprintf(stderr, "Usage: %s [-dnosvx] [-w timeout] lockfile|lockdir [-c] "
	    "command\n\t%s [-dnsuvx] [-w timeout] lockfd\n", getprogname(),
	    getprogname());
	exit(EXIT_FAILURE);
}

static __dead void
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

int
main(int argc, char *argv[])
{
	int c;
	int lock = LOCK_EX;
	double timeout = 0;
	int cls = 0;
	int fd = -1;
	int debug = 0;
	char *mcargv[] = {
	    __UNCONST(_PATH_BSHELL), __UNCONST("-c"), NULL, NULL
	};
	char **cmdargv = NULL;

	setprogname(argv[0]);

	while ((c = getopt_long(argc, argv, "+dnosuw:x", flock_longopts, NULL))
	    != -1)
		switch (c) {
		case 'd':
			debug++;
			break;
		case 'x':
			lock = LOCK_EX | (lock & ~LOCK_NB);
			break;
		case 'n':
			lock |= LOCK_NB;
			break;
		case 's':
			lock = LOCK_SH | (lock & ~LOCK_NB);
			break;
		case 'u':
			lock = LOCK_UN | (lock & ~LOCK_NB);
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
			usage();
		}

	argc -= optind;
	argv += optind;

	switch (argc) {
	case 0:
		usage();
	case 1:
		if (cls)
			usage();
		fd = strtol(argv[0], NULL, 0);	// XXX: error checking
		if (debug)
			fprintf(stderr, "descriptor %s lock %s\n",
			    argv[0], lock2name(lock));
		if (lock == LOCK_UN)
			usage();
	default:
		if (strcmp(argv[1], "-c") == 0 ||
		    strcmp(argv[1], "--command") == 0) {
			if (argc == 2)
				usage();
			mcargv[2] = argv[2];
			cmdargv = mcargv;
		} else
			cmdargv = argv + 1;
			
		if ((fd = open(argv[0], O_RDONLY)) == -1) {
			if (errno != ENOENT || 
			    (fd = open(argv[0], O_RDWR|O_CREAT, 0600)) == -1)
				err(EXIT_FAILURE, "Cannot open `%s'", argv[0]);
		}
		if (debug)
			fprintf(stderr, "file %s lock %s command %s ...\n",
			    argv[0], lock2name(lock), cmdargv[0]);
		break;
	}

	if (timeout) {
		signal(SIGALRM, sigalrm);
		alarm((int)timeout);	// XXX: User timer_create()
		if (debug)
			fprintf(stderr, "alarm %d\n", (int)timeout);
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
		alarm(0);

	if (cls)
		(void)close(fd);

	if (cmdargv != NULL) {
		execvp(cmdargv[0], cmdargv);
		err(EXIT_FAILURE, "execvp '%s'", cmdargv[0]);
	}
	return 0;
}
