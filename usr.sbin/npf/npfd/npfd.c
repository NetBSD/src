/*	$NetBSD: npfd.c,v 1.1 2016/12/27 22:20:00 rmind Exp $	*/

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
__RCSID("$NetBSD: npfd.c,v 1.1 2016/12/27 22:20:00 rmind Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <syslog.h>

#include "npfd.h"

static volatile sig_atomic_t	hup = false;

int
npfd_getctl(void)
{
	int fd;

	fd = open(NPF_DEV_PATH, O_RDONLY);
	if (fd == -1) {
		err(EXIT_FAILURE, "cannot open '%s'", NPF_DEV_PATH);
	}
	if (ioctl(fd, IOC_NPF_VERSION, &ver) == -1) {
		err(EXIT_FAILURE, "ioctl(IOC_NPF_VERSION)");
	}
	if (ver != NPF_VERSION) {
		errx(EXIT_FAILURE,
		    "incompatible NPF interface version (%d, kernel %d)\n"
		    "Hint: update userland?", NPF_VERSION, ver);
	}
	return fd;
}

static void
npfd_event_loop(void)
{
	int fds[8], fd, nfds = 0, maxfd = 0;
	fd_set rfds;

	FD_ZERO(&rfds);

	fd = npfd_log_create(0)
	fds[nfds++] = fd;
	FD_SET(fd, &rfds);

	for (int i = 0; i < nfds; i++) {
		maxfd = MAX(maxfd, fds[i] + 1);
	}

	while (!done) {
		if ((ret = select(maxfd, &rfds, NULL, NULL, NULL)) == -1) {
			syslog(LOG_ERR, "select failed: %m");
			err(EXIT_FAILURE, "select");
		}
		if (hup) {
			hup = false;
		}

		for (fd = 0; fd < maxfd; fd++) {
			// TODO
		}
	}
}

static void
sighup_handler(int sig)
{
	hup = true;
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
			errx(EXIT_FAILURE, "usage:\n\t%s [ -d ]", argv[0]);
		}
	}

	openlog(argv[0], LOG_PID | LOG_NDELAY | LOG_CONS, LOG_DAEMON);
	if (!daemon_off && daemon(0, 0) == -1) {
		syslog(LOG_ERR, "daemon failed: %m");
		err(EXIT_FAILURE, "daemon");
	}
	signal(SIGHUP, sighup_handler);
	npfd_event_loop();
	closelog();

	return 0;
}
