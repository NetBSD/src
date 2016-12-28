/*	$NetBSD: npfd.c,v 1.3 2016/12/28 03:02:54 christos Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
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
__RCSID("$NetBSD: npfd.c,v 1.3 2016/12/28 03:02:54 christos Exp $");

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <poll.h>
#include <errno.h>
#include <err.h>
#include <syslog.h>

#include <net/npf.h>

#include "npfd.h"

static volatile sig_atomic_t hup, stats, done;

static int
npfd_getctl(void)
{
	int fd, ver;

	fd = open(NPF_DEV_PATH, O_RDONLY);
	if (fd == -1) {
		err(EXIT_FAILURE, "cannot open '%s'", NPF_DEV_PATH);
	}
	if (ioctl(fd, IOC_NPF_VERSION, &ver) == -1) {
		err(EXIT_FAILURE, "ioctl(IOC_NPF_VERSION)");
	}
	if (ver != NPF_VERSION) {
		errx(EXIT_FAILURE,
		    "Incompatible NPF interface version (%d, kernel %d)\n"
		    "Hint: update userland?", NPF_VERSION, ver);
	}
	return fd;
}

static void
npfd_event_loop(void)
{
	struct pollfd pfd;
	npfd_log_t *log;

	log = npfd_log_create(0);
	if (log == NULL)
		exit(EXIT_FAILURE);
	pfd.fd = npfd_log_getsock(log);
	pfd.events = POLLHUP | POLLIN;

	while  (!done) {
		if (hup) {
			hup = false;
			npfd_log_reopen(log);
		}
		if (stats) {
			stats = false;
			npfd_log_stats(log);
		}
		switch (poll(&pfd, 1, 1000)) {
		case -1:
			if (errno == EINTR)
				continue;
			syslog(LOG_ERR, "poll failed: %m");
			exit(EXIT_FAILURE);
		case 0:
			continue;
		default:
			npfd_log(log);
		}

	}
	npfd_log_destroy(log);
}

static void
sighandler(int sig)
{
	switch (sig) {
	case SIGHUP:
		hup = true;
		break;
	case SIGTERM:
	case SIGINT:
		hup = true;
		break;
	case SIGINFO:
	case SIGQUIT:
		stats = true;
		break;
	default:
		syslog(LOG_ERR, "Unhandled signal %d", sig);
	}
}

int
main(int argc, char **argv)
{
	bool daemon_off = false;
	int ch;

	while ((ch = getopt(argc, argv, "d")) != -1) {
		switch (ch) {
		case 'd':
			daemon_off = true;
			break;
		default:
			fprintf(stderr, "Usage: %s [-d]\n", getprogname());
			exit(EXIT_FAILURE);
		}
	}
	int fd = npfd_getctl();
	(void)close(fd);

	if (!daemon_off && daemon(0, 0) == -1) {
		err(EXIT_FAILURE, "daemon");
	}
	openlog(argv[0], LOG_PID | LOG_NDELAY | LOG_CONS, LOG_DAEMON);
	signal(SIGHUP, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGINFO, sighandler);
	signal(SIGQUIT, sighandler);
	npfd_event_loop();
	closelog();

	return 0;
}
