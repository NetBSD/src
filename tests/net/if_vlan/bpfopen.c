/*	$NetBSD: bpfopen.c,v 1.2 2021/08/19 03:27:05 yamaguchi Exp $	*/

/*
 * Copyright (c) 2021 Internet Initiative Japan Inc.
 * All rights reserved.
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
__RCSID("$NetBSD: bpfopen.c,v 1.2 2021/08/19 03:27:05 yamaguchi Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/bpf.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <util.h>

enum {
	ARG_PROG = 0,
	ARG_HOST,
	ARG_IFNAME,
	ARG_NUM
};

enum {
	PFD_BPF = 0,
	PFD_NUM
};

static void	sighandler(int);
static void	log_debug(const char *, ...) __printflike(1, 2);
static int	bpf_open(void);
static void	bpf_close(int);
static void	bpf_read(int);

static sig_atomic_t	 quit;
static bool		 daemonize = false;
static int		 verbose = 0;
static const char	*path_pid = "/var/run/bpfopen.pid";
static const char	*path_bpf = "/dev/bpf";
static const char	*ifname = NULL;

static void
usage(void)
{

	fprintf(stderr, "%s [-vd] [-p pidfile] [-b devbpf ] <ifname>\n"
	    "\t-v: verbose\n"
	    "\t-d: daemon mode\n",
	    getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	int bpfd;
	int ch;

	while ((ch = getopt(argc, argv, "b:dp:v")) != -1) {
		switch (ch) {
		case 'b':
			path_bpf = optarg;
			break;
		case 'd':
			daemonize = true;
			break;
		case 'p':
			path_pid = optarg;
			break;
		case 'v':
			verbose++;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	ifname = argv[0];

	bpfd = bpf_open();
	if (bpfd < 0)
		err(1, "bpf_open");
	log_debug("bpf opened");

	if (daemonize) {
		if (daemon(1, 1) != 0) {
			bpf_close(bpfd);
			err(1, "daemon");
		}
		log_debug("daemonized");

		if (pidfile(path_pid) != 0) {
			bpf_close(bpfd);
			err(1, "pidfile");
		}
	}

	bpf_read(bpfd);
	bpf_close(bpfd);
	if (daemonize)
		pidfile_clean();

	return 0;
}

static void
sighandler(int signo)
{

	quit = 1;
}

static void
log_debug(const char *fmt, ...)
{
	va_list ap;

	if (verbose <= 0)
		return;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");
}

static int
bpf_open(void)
{
	struct ifreq ifr;
	int bpfd;

	bpfd = open(path_bpf, O_RDONLY);
	if (bpfd < 0) {
		log_debug("open: %s", strerror(errno));
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(bpfd, BIOCSETIF, &ifr) != 0) {
		log_debug("BIOCSETIF: %s", strerror(errno));
		goto close_bpfd;
	}

	if (ioctl(bpfd, BIOCPROMISC, NULL) != 0) {
		log_debug("BIOCPROMISC: %s", strerror(errno));
		goto close_bpfd;
	}

	return bpfd;

close_bpfd:
	close(bpfd);

	return -1;
}

static void
bpf_close(int bpfd)
{

	close(bpfd);
}

static void
bpf_read(int bpfd)
{
	struct pollfd pfd[PFD_NUM];
	int nfds;
	char *buf;
	u_int bufsiz;
	ssize_t n;

	if (ioctl(bpfd, BIOCGBLEN, &bufsiz) != 0) {
		bufsiz = BPF_DFLTBUFSIZE;
		log_debug("BIOCGBLEN: %s, use default size %u",
		    strerror(errno), bufsiz);
	}

	bufsiz = MIN(bufsiz, BPF_DFLTBUFSIZE * 4);

	buf = malloc(bufsiz);
	if (buf == NULL) {
		log_debug("malloc: %s", strerror(errno));
		return;
	}

	quit = 0;
	signal(SIGTERM, sighandler);
	signal(SIGQUIT, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGHUP, sighandler);

	log_debug("start reading %s, bufsiz=%u", ifname, bufsiz);

	while (quit == 0) {
		pfd[PFD_BPF].fd = bpfd;
		pfd[PFD_BPF].events = POLLIN;

		nfds = poll(pfd, PFD_NUM, 1 * 1000);
		if (nfds == -1 && errno != EINTR) {
			warn("poll");
			quit = 1;
		}

		if (nfds > 0 && (pfd[PFD_BPF].revents & POLLIN)) {
			/* read & drop */
			memset(buf, 0, bufsiz);
			n = read(pfd[PFD_BPF].fd, buf, bufsiz);
			if (n < 0)
				quit = 1;
		}
	}

	log_debug("finish reading %s", ifname);
	free(buf);
}
