
#include <sys/cdefs.h>
__RCSID("$NetBSD: blacklistd.c,v 1.1 2015/01/18 17:28:37 christos Exp $");

#include <sys/types.h>
#include <sys/queue.h>

#include <util.h>
#include <signal.h>
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

#include "bl.h"
#include "internal.h"

static int debug;
static const char *configfile = _PATH_BLCONF;


struct conf {
	SLIST_ENTRY(conf)	c_next;
	in_port_t		c_port;
	uint8_t			c_proto;
	size_t			c_nfail;
	time_t			c_disable;
};

SLIST_HEAD(confhead, conf) head;

static __dead void
usage(void)
{
	fprintf(stderr, "Usage: %s -d [-c <config>]\n", getprogname());
	exit(EXIT_FAILURE);
}

static int
getnum(void *v, size_t l, char **p, intmax_t min, intmax_t max)
{
	char *ep;
	int e;
	intmax_t im = strtoi(*p, &ep, 0, min, max, &e);

	if (e != ENOTSUP)
		return e;

	if (*ep && !isspace((unsigned char)*ep))
		return e;

	for (; isspace((unsigned char)*ep); ep++)
		continue;
	*p = ep;

	switch (l) {
	case 1:
		*(int8_t *)v = *(int8_t *)im;
		break;
	case 2:
		*(int16_t *)v = *(int16_t *)im;
		break;
	case 4:
		*(int32_t *)v = *(int32_t *)im;
		break;
	case 8:
		*(int64_t *)v = *(int64_t *)im;
		break;
	default:
		abort();
	}
	return 0;
}

static struct conf *
parseconfline(char *l)
{
	struct conf c, *cp;
	int e;

	e = getnum(&c.c_port, sizeof(c.c_port), &l, 0, 65536);
	if (e) goto out;
	e = getnum(&c.c_proto, sizeof(c.c_proto), &l, 0, 255);
	if (e) goto out;
	e = getnum(&c.c_nfail, sizeof(c.c_nfail), &l, 0, SIZE_T_MAX);
	if (e) goto out;
	e = getnum(&c.c_disable, sizeof(c.c_disable), &l, -1, INTMAX_MAX);
	if (e) goto out;

	if ((cp = malloc(sizeof(*cp))) == NULL)
		return NULL;

	*cp = c;
	return cp;
out:
	errno = e;
	return NULL;
}

static void
parseconf(const char *f)
{
	FILE *fp;
	char *line;
	size_t lineno, len;
	struct confhead nhead, thead;
	struct conf *c, *tc;

	if ((fp = fopen(f, "r")) == NULL) {
		syslog(LOG_ERR, "%s: Cannot open `%s' (%m)", __func__, f);
		return;
	}

	lineno = 1;
	SLIST_INIT(&nhead);
	for (; (line = fparseln(fp, &len, &lineno, NULL, 0)) != NULL;
	    free(line))
	{
		c = parseconfline(line);
		if (c == NULL) {
			syslog(LOG_ERR, "%s: %s, %zu: Syntax error [%s] (%m)",
			    __func__, f, lineno, line);
			continue;
		}
		SLIST_INSERT_HEAD(&nhead, c, c_next);
	}
	fclose(fp);
	thead = head;
	head = nhead;
	SLIST_FOREACH_SAFE(c, &thead, c_next, tc)
		free(c);
}

int
main(int argc, char *argv[])
{
	int c;
	bl_t bl;

	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "c:d")) != -1) {
		switch (c) {
		case 'c':
			configfile = optarg;
			break;
		case 'd':
			debug++;
			break;
		default:
			usage();
		}
	}

	parseconf(configfile);

	if (!debug)
		daemon(0, 0);

	bl = bl_create(true);
	if (bl == NULL || !bl->b_connected)
		return EXIT_FAILURE;

	struct pollfd pfd;
	pfd.fd = bl->b_fd;
	pfd.events = POLLIN;
	for (;;) {
	}
}
