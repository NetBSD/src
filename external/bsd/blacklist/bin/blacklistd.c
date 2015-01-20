/*	$NetBSD: blacklistd.c,v 1.4 2015/01/20 00:19:21 christos Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: blacklistd.c,v 1.4 2015/01/20 00:19:21 christos Exp $");

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <util.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <syslog.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <err.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>
#include <time.h>

#include "bl.h"
#include "internal.h"
#include "conf.h"
#include "run.h"
#include "state.h"

static const char *configfile = _PATH_BLCONF;

int debug;
const char *rulename = "blacklistd";
const char *controlprog = _PATH_BLCONTROL;
struct conf *conf;
size_t nconf;

static DB *state;
static const char *dbfile = _PATH_BLSTATE;
static sig_atomic_t rconf = 1;

void (*lfun)(int, const char *, ...) = syslog;

static void
sighup(int n)
{
	rconf++;
}

static __dead void
usage(void)
{
	fprintf(stderr, "Usage: %s -d [-c <config>] [-r <rulename>] "
	    "[-s <sockpath>] [-C <controlprog>] [-D <dbfile>]\n",
	    getprogname());
	exit(EXIT_FAILURE);
}

static const char *
expandm(char *buf, size_t len, const char *fmt)
{
	char *p;
	size_t r;

	if ((p = strstr(fmt, "%m")) == NULL)
		return fmt;

	r = (size_t)(p - fmt);
	if (r >= len)
		return fmt;

	strlcpy(buf, fmt, r + 1);
	strlcat(buf, strerror(errno), len);
	strlcat(buf, fmt + r + 2, len);

	return buf;
}

static void
dlog(int level, const char *fmt, ...)
{
	char buf[BUFSIZ];
	va_list ap;

	fprintf(stderr, "%s: ", getprogname());
	va_start(ap, fmt);
	vfprintf(stderr, expandm(buf, sizeof(buf), fmt), ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

static const char *
fmttime(char *b, size_t l, time_t t)
{
	struct tm tm;
	if (localtime_r(&t, &tm) == NULL)
		snprintf(b, l, "*%jd*", (intmax_t)t);
	else
		strftime(b, l, "%Y/%m/%d %H:%M:%S", &tm);
	return b;
}

static void
process(bl_t bl)
{
	int rfd;
	struct sockaddr_storage rss;
	socklen_t rsl;
	char rbuf[BUFSIZ];
	bl_info_t *bi;
	struct conf c;
	struct dbinfo dbi;
	struct timespec ts;

	if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
		(*lfun)(LOG_ERR, "clock_gettime failed (%m)"); 
		return;
	}

	if ((bi = bl_recv(bl)) == NULL)
		return;

	if (debug)
		printf("got type=%d fd=[%d %d] msg=%s cred=[u=%lu, g=%lu]\n",
		    bi->bi_type, bi->bi_fd[0], bi->bi_fd[1], bi->bi_msg,
		    (unsigned long)bi->bi_cred->sc_euid,
		    (unsigned long)bi->bi_cred->sc_egid);

	if (findconf(bi, &c) == NULL)
		goto out;

	rfd = bi->bi_fd[1];
	rsl = sizeof(rss);
	memset(&rss, 0, rsl);
	if (getpeername(rfd, (void *)&rss, &rsl) == -1) {
		(*lfun)(LOG_ERR, "getsockname failed (%m)"); 
		goto out;
	}
	if (state_get(state, &rss, &c, &dbi) == -1)
		goto out;
	if (debug) {
		char b1[128], b2[128];
		sockaddr_snprintf(rbuf, sizeof(rbuf), "%a:%p", (void *)&rss);
		printf("%s: %s count=%d nfail=%d last=%s now=%s\n", __func__,
		    rbuf, dbi.count, c.c_nfail,
		    fmttime(b1, sizeof(b1), dbi.last),
		    fmttime(b2, sizeof(b2), ts.tv_sec));
	}
	dbi.count++;
	dbi.last = ts.tv_sec;
	if (dbi.count >= c.c_nfail) {
		int res = run_add(c.c_proto, (in_port_t)c.c_port, &rss);
		if (res == -1)
			goto out;
		dbi.id = res;
	}
	if (state_put(state, &rss, &c, &dbi) == -1)
		goto out;
out:
	close(bi->bi_fd[0]);
	close(bi->bi_fd[1]);
}

static void
update(void)
{
	struct timespec ts;
	struct sockaddr_storage ss;
	struct conf c;
	struct dbinfo dbi;
	unsigned int f, n;

	if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
		(*lfun)(LOG_ERR, "clock_gettime failed (%m)"); 
		return;
	}

	for (n = 0, f = 1; state_iterate(state, &ss, &c, &dbi, f) == 1;
	    f = 0, n++)
	{
		time_t when = c.c_duration + dbi.last;
		if (debug) {
			char buf[128], b1[64], b2[64];
			sockaddr_snprintf(buf, sizeof(buf), "%a:%p",
			    (void *)&ss);
			printf("%s:[%u] %s count=%d duration=%d exp=%s "
			   "now=%s\n", __func__, n, buf, dbi.count,
			   c.c_duration, fmttime(b1, sizeof(b1), when),
			   fmttime(b2, sizeof(b2), ts.tv_sec));
		}
		if (when >= ts.tv_sec)
			continue;
		if (dbi.id != -1)
			run_rem(dbi.id);
		state_del(state, &ss, &c);
	}
}

int
main(int argc, char *argv[])
{
	int c;
	bl_t bl;
	int tout;
	int flags = O_RDWR|O_EXCL|O_CLOEXEC;
	const char *spath = _PATH_BLSOCK;

	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "C:c:D:ds:r:")) != -1) {
		switch (c) {
		case 'C':
			controlprog = optarg;
			break;
		case 'c':
			configfile = optarg;
			break;
		case 'D':
			dbfile = optarg;
			break;
		case 'd':
			debug++;
			break;
		case 'r':
			rulename = optarg;
			break;
		case 's':
			spath = optarg;
			break;
		default:
			usage();
		}
	}

	signal(SIGHUP, sighup);

	if (debug) {
		lfun = dlog;
		tout = 1000;
	} else {
		daemon(0, 0);
		tout = 15000;
	}

	run_flush();

	bl = bl_create2(true, spath, lfun);
	if (bl == NULL || !bl_isconnected(bl))
		return EXIT_FAILURE;
	state = state_open(dbfile, flags, 0600);
	if (state == NULL)
		state = state_open(dbfile,  flags | O_CREAT, 0600);
	if (state == NULL)
		return EXIT_FAILURE;

	struct pollfd pfd;
	pfd.fd = bl_getfd(bl);
	pfd.events = POLLIN;
	for (;;) {
		if (rconf) {
			rconf = 0;
			parseconf(configfile);
		}
		switch (poll(&pfd, 1, tout)) {
		case -1:
			if (errno == EINTR)
				continue;
			(*lfun)(LOG_ERR, "poll (%m)");
			return EXIT_FAILURE;
		case 0:
			update();
			break;
		default:
			process(bl);
		}
	}
	return 0;
}
