/*	$NetBSD: blacklistd.c,v 1.25 2015/01/22 23:45:41 christos Exp $	*/

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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/cdefs.h>
__RCSID("$NetBSD: blacklistd.c,v 1.25 2015/01/22 23:45:41 christos Exp $");

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#ifdef HAVE_UTIL_H
#include <util.h>
#endif
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
#include <time.h>
#include <netinet/in.h>

#include "bl.h"
#include "internal.h"
#include "conf.h"
#include "run.h"
#include "state.h"
#include "support.h"

static const char *configfile = _PATH_BLCONF;
static DB *state;
static const char *dbfile = _PATH_BLSTATE;
static sig_atomic_t rconf;
static sig_atomic_t done;
static int vflag;

static void
sigusr1(int n __unused)
{
	debug++;
}

static void
sigusr2(int n __unused)
{
	debug--;
}

static void
sighup(int n __unused)
{
	rconf++;
}

static void
sigdone(int n __unused)
{
	done++;
}

static __dead void
usage(int c)
{
	warnx("Unknown option `%c'", (char)c);
	fprintf(stderr, "Usage: %s [-vdf] [-c <config>] [-r <rulename>] "
	    "[-P <sockpathsfile>] [-C <controlprog>] [-D <dbfile>] "
	    "[-t <timeout>]\n", getprogname());
	exit(EXIT_FAILURE);
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
		(*lfun)(LOG_DEBUG, "got type=%d fd=%d msg=%s uid=%lu gid=%lu",
		    bi->bi_type, bi->bi_fd, bi->bi_msg,
		    (unsigned long)bi->bi_uid,
		    (unsigned long)bi->bi_gid);

	if (conf_find(bi->bi_fd, bi->bi_uid, &c) == NULL)
		goto out;

	rfd = bi->bi_fd;
	rsl = sizeof(rss);
	memset(&rss, 0, rsl);
	if (getpeername(rfd, (void *)&rss, &rsl) == -1) {
		if (errno != ENOTCONN) {
			(*lfun)(LOG_ERR, "getpeername failed (%m)"); 
			goto out;
		}
		if (bi->bi_slen == 0) {
			(*lfun)(LOG_ERR,
			    "unconnected socket with no peer in message"); 
			goto out;
		}
		memcpy(&rss, &bi->bi_ss, bi->bi_slen);
		switch (rss.ss_family) {
		case AF_INET:
			rsl = sizeof(struct sockaddr_in);
			break;
		case AF_INET6:
			rsl = sizeof(struct sockaddr_in6);
			break;
		default:
			(*lfun)(LOG_ERR, "bad client passed socket family %u",
			    rss.ss_family); 
			goto out;
		}
		if (rsl != bi->bi_slen) {
		    (*lfun)(LOG_ERR,
			"bad client passed socket length %u != %u",
			(unsigned)rsl, bi->bi_slen); 
		    goto out;
		}
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
		if (rsl != rss.ss_len) {
		    (*lfun)(LOG_ERR,
			"bad client passed socket internal length %u != %u",
			(unsigned)rsl, rss.ss_len, rsl); 
		    goto out;
		}
#endif
	}
	if (state_get(state, &rss, &c, &dbi) == -1)
		goto out;

	if (debug) {
		char b1[128], b2[128];
		sockaddr_snprintf(rbuf, sizeof(rbuf), "%a:%p", (void *)&rss);
		(*lfun)(LOG_DEBUG, "%s: %s count=%d nfail=%d last=%s now=%s",
		    __func__, rbuf, dbi.count, c.c_nfail,
		    fmttime(b1, sizeof(b1), dbi.last),
		    fmttime(b2, sizeof(b2), ts.tv_sec));
	}

	switch (bi->bi_type) {
	case BL_ADD:
		dbi.count++;
		dbi.last = ts.tv_sec;
		if (dbi.id[0]) {
			(*lfun)(LOG_ERR, "rule exists %s", dbi.id);
			goto out;
		}
		if (c.c_nfail != -1 && dbi.count >= c.c_nfail) {
			int res = run_change("add", &c, &rss,
			    dbi.id, sizeof(dbi.id));
			if (res == -1)
				goto out;
			sockaddr_snprintf(rbuf, sizeof(rbuf), "%a",
			    (void *)&rss);
			syslog(LOG_INFO,
			    "Blocked %s at port %d for %d seconds",
			    rbuf, c.c_port, c.c_duration);
				
		}
		break;
	case BL_DELETE:
		if (dbi.last == 0)
			goto out;
		dbi.last = 0;
		break;
	default:
		(*lfun)(LOG_ERR, "unknown message %d", bi->bi_type); 
	}
	if (state_put(state, &rss, &c, &dbi) == -1)
		goto out;
out:
	close(bi->bi_fd);
}

static void
update(void)
{
	struct timespec ts;
	struct sockaddr_storage ss;
	struct conf c;
	struct dbinfo dbi;
	unsigned int f, n;
	char buf[128];

	if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
		(*lfun)(LOG_ERR, "clock_gettime failed (%m)"); 
		return;
	}

	for (n = 0, f = 1; state_iterate(state, &ss, &c, &dbi, f) == 1;
	    f = 0, n++)
	{
		time_t when = c.c_duration + dbi.last;
		if (debug > 1) {
			char b1[64], b2[64];
			sockaddr_snprintf(buf, sizeof(buf), "%a:%p",
			    (void *)&ss);
			(*lfun)(LOG_DEBUG,
			    "%s:[%u] %s count=%d duration=%d last=%s "
			   "now=%s", __func__, n, buf, dbi.count,
			   c.c_duration, fmttime(b1, sizeof(b1), dbi.last),
			   fmttime(b2, sizeof(b2), ts.tv_sec));
		}
		if (c.c_duration == -1 || when >= ts.tv_sec)
			continue;
		if (dbi.id[0]) {
			run_change("rem", &c, &ss, dbi.id, 0);
			sockaddr_snprintf(buf, sizeof(buf), "%a", (void *)&ss);
			syslog(LOG_INFO,
			    "Released %s at port %d after %d seconds",
			    buf, c.c_port, c.c_duration);
		}
		state_del(state, &ss, &c);
	}
}

static void
addfd(struct pollfd **pfdp, bl_t **blp, size_t *nfd, size_t *maxfd,
    const char *path)
{
	bl_t bl = bl_create(true, path, vflag ? vdlog : vsyslog);
	if (bl == NULL || !bl_isconnected(bl))
		exit(EXIT_FAILURE);
	if (*nfd >= *maxfd) {
		*maxfd += 10;
		*blp = realloc(*blp, sizeof(**blp) * *maxfd);
		if (*blp == NULL)
			err(EXIT_FAILURE, "malloc");
		*pfdp = realloc(*pfdp, sizeof(**pfdp) * *maxfd);
		if (*pfdp == NULL)
			err(EXIT_FAILURE, "malloc");
	}

	(*pfdp)[*nfd].fd = bl_getfd(bl);
	(*pfdp)[*nfd].events = POLLIN;
	(*blp)[*nfd] = bl;
	*nfd += 1;
}

int
main(int argc, char *argv[])
{
	int c, tout, flags, reset;
	const char *spath;

	setprogname(argv[0]);

	spath = NULL;
	reset = 0;
	tout = 0;
	flags = O_RDWR|O_EXCL|O_CLOEXEC;
	while ((c = getopt(argc, argv, "C:c:D:dfr:P:t:v")) != -1) {
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
		case 'f':
			reset++;
			break;
		case 'r':
			rulename = optarg;
			break;
		case 'P':
			spath = optarg;
			break;
		case 't':
			tout = atoi(optarg) * 1000;
			break;
		case 'v':
			vflag++;
			break;
		default:
			usage(c);
		}
	}

	signal(SIGHUP, sighup);
	signal(SIGINT, sigdone);
	signal(SIGQUIT, sigdone);
	signal(SIGTERM, sigdone);
	signal(SIGUSR1, sigusr1);
	signal(SIGUSR2, sigusr2);

	openlog(getprogname(), LOG_PID, LOG_DAEMON);

	if (debug) {
		lfun = dlog;
		if (tout == 0)
			tout = 5000;
	} else {
		if (tout == 0)
			tout = 15000;
	}

	conf_parse(configfile);
	if (reset) {
		for (size_t i = 0; i < nconf; i++)
			run_flush(&conf[i]);
		flags |= O_TRUNC;
	}

	struct pollfd *pfd = NULL;
	bl_t *bl = NULL;
	size_t nfd = 0;
	size_t maxfd = 0;

	if (spath == NULL)
		addfd(&pfd, &bl, &nfd, &maxfd, _PATH_BLSOCK);
	else {
		FILE *fp = fopen(spath, "r");
		char *line;
		if (fp == NULL)
			err(EXIT_FAILURE, "Can't open `%s'", spath);
		for (; (line = fparseln(fp, NULL, NULL, NULL, 0)) != NULL;
		    free(line))
			addfd(&pfd, &bl, &nfd, &maxfd, line);
		fclose(fp);
	}

	state = state_open(dbfile, flags, 0600);
	if (state == NULL)
		state = state_open(dbfile,  flags | O_CREAT, 0600);
	if (state == NULL)
		return EXIT_FAILURE;

	if (!debug) {
		if (daemon(0, 0) == -1)
			err(EXIT_FAILURE, "daemon failed");
		if (pidfile(NULL) == -1)
			err(EXIT_FAILURE, "Can't create pidfile");
	}

	while (!done) {
		if (rconf) {
			rconf = 0;
			conf_parse(configfile);
		}
		switch (poll(pfd, (nfds_t)nfd, tout)) {
		case -1:
			if (errno == EINTR)
				continue;
			(*lfun)(LOG_ERR, "poll (%m)");
			return EXIT_FAILURE;
		case 0:
			break;
		default:
			for (size_t i = 0; i < nfd; i++)
				if (pfd[i].revents & POLLIN)
					process(bl[i]);
		}
		update();
	}
	state_close(state);
	return 0;
}
