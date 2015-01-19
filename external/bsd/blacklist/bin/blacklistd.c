
#include <sys/cdefs.h>
__RCSID("$NetBSD: blacklistd.c,v 1.3 2015/01/19 19:02:35 christos Exp $");

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
#include <err.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

#include "bl.h"
#include "internal.h"
#include "conf.h"

static const char *configfile = _PATH_BLCONF;

int debug;
struct conf *conf;
size_t nconf;

static sig_atomic_t rconf = 1;

void (*lfun)(int, const char *, ...) = syslog;

void
sighup(int n)
{
	rconf++;
}

static __dead void
usage(void)
{
	fprintf(stderr, "Usage: %s -d [-c <config>] [-s <sockpath>]\n",
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

	r = p - fmt;
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

static void
process(bl_t bl)
{
	bl_type_t e;
	int rfd;
	struct sockaddr_storage rss;
	socklen_t rsl;
	char buf[BUFSIZ], rbuf[BUFSIZ];
	bl_info_t *bi;
	const struct conf *c;

	if ((bi = bl_recv(bl)) == NULL)
		return;

	if (debug)
		printf("got type=%d fd=[%d %d] msg=%s cred=[u=%lu, g=%lu]\n",
		    bi->bi_type, bi->bi_fd[0], bi->bi_fd[1], bi->bi_msg,
		    (unsigned long)bi->bi_cred->sc_euid,
		    (unsigned long)bi->bi_cred->sc_egid);

	if ((c = findconf(bi)) == NULL)
		goto out;

	rfd = bi->bi_fd[1];
	rsl = sizeof(rss);
	if (getpeername(rfd, (void *)&rss, &rsl) == -1) {
		(*lfun)(LOG_ERR, "getsockname failed (%m)"); 
		goto out;
	}
	sockaddr_snprintf(rbuf, sizeof(rbuf), "%a:%p", (void *)&rss);
	printf("rbuf = %s\n", rbuf);
out:
	close(bi->bi_fd[0]);
	close(bi->bi_fd[1]);
}

static void
update(void)
{
}

int
main(int argc, char *argv[])
{
	int c;
	bl_t bl;
	int tout;
	const char *spath = _PATH_BLSOCK;

	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "c:ds:")) != -1) {
		switch (c) {
		case 'c':
			configfile = optarg;
			break;
		case 'd':
			debug++;
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

	bl = bl_create2(true, spath, lfun);
	if (bl == NULL || !bl_isconnected(bl))
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
			break;
		default:
			process(bl);
		}
	}
	return 0;
}
