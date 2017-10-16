/*	$NetBSD: npfd.c,v 1.9 2017/10/16 11:18:43 christos Exp $	*/

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
__RCSID("$NetBSD: npfd.c,v 1.9 2017/10/16 11:18:43 christos Exp $");

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <poll.h>
#include <errno.h>
#include <err.h>
#include <syslog.h>
#include <util.h>

#include <net/npf.h>

#include "npfd.h"

static volatile sig_atomic_t hup, stats, done, flush;

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
npfd_event_loop(npfd_log_t *log, int delay)
{
	struct pollfd pfd;
	size_t count = 0;

	pfd.fd = npfd_log_getsock(log);
	pfd.events = POLLHUP | POLLIN;

	while  (!done) {
		if (hup) {
			hup = false;
			npfd_log_file_reopen(log, false);
		}
		if (stats) {
			stats = false;
			npfd_log_stats(log);
		}
		if (flush) {
			flush = false;
			npfd_log_flush(log);
			count = 0;
		}
		switch (poll(&pfd, 1, delay)) {
		case -1:
			if (errno == EINTR)
				continue;
			syslog(LOG_ERR, "poll failed: %m");
			exit(EXIT_FAILURE);
			/*NOTREACHED*/
		case 0:
			npfd_log_flush(log);
			count = 0;
			continue;
		default:
			if (count++ >= 100) {
				npfd_log_flush(log);
				count = 0;
			}
			if (npfd_log(log) <= 0)
				npfd_log_pcap_reopen(log);
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
		done = true;
		break;
	case SIGINFO:
		stats = true;
		break;
	case SIGALRM:
		flush = true;
		break;
	default:
		syslog(LOG_ERR, "Unhandled signal %d", sig);
		break;
	}
}

static __dead void
usage(void)
{
	fprintf(stderr, "Usage: %s [-D] [-d <delay>] [-i <interface>]"
	    " [-f <filename>] [-p <pidfile>] [-s <snaplen>] expression\n",
	    getprogname());
	exit(EXIT_FAILURE);
}

static char *
copyargs(int argc, char **argv)
{
	if (argc == 0)
		return NULL;

	size_t len = 0, p = 0;
	char *buf = NULL;

	for (int i = 0; i < argc; i++) {
		size_t l = strlen(argv[i]);
		if (p + l + 1 >= len)
			buf = erealloc(buf, len = p + l + 1);
		memcpy(buf + p, argv[i], l);
		p += l;
		buf[p++] = i == argc - 1 ? '\0' : ' ';
	}
	return buf;
}

int
main(int argc, char **argv)
{
	bool daemon_off = false;
	int ch;

	int delay = 5 * 1000;
	const char *iface = "npflog0";
	int snaplen = 116;
	char *pidname = NULL;
	char *filename = NULL;

	int fd = npfd_getctl();
	(void)close(fd);

	while ((ch = getopt(argc, argv, "Dd:f:i:p:s:")) != -1) {
		switch (ch) {
		case 'D':
			daemon_off = true;
			break;
		case 'd':
			delay = atoi(optarg) * 1000;
			break;
		case 'f':
			filename = optarg;
			break;
		case 'i':
			iface = optarg;
			break;
		case 'p':
			pidname = optarg;
			break;
		case 's':
			snaplen = atoi(optarg);
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	char *filter = copyargs(argc, argv);

	npfd_log_t *log = npfd_log_create(filename, iface, filter, snaplen);

	if (!daemon_off) {
		if (daemon(0, 0) == -1)
			err(EXIT_FAILURE, "daemon");
		pidfile(pidname);
	}

	openlog(argv[0], LOG_PID | LOG_NDELAY | LOG_CONS, LOG_DAEMON);
	signal(SIGHUP, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGINFO, sighandler);
	signal(SIGQUIT, sighandler);

	npfd_event_loop(log, delay);

	closelog();

	return 0;
}
