/*	$NetBSD: rup.c,v 1.29 2016/09/05 00:40:29 sevan Exp $	*/

/*-
 * Copyright (c) 1993, John Brezak
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: rup.c,v 1.29 2016/09/05 00:40:29 sevan Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#undef FSHIFT			/* Use protocol's shift and scale values */
#undef FSCALE
#include <rpcsvc/rstat.h>

#define HOST_WIDTH 24

static int printtime;		/* print the remote host(s)'s time */

static struct host_list {
	struct host_list *next;
	int family;
	union {
		struct in6_addr _addr6;
		struct in_addr _addr4;
	} addr;
} *hosts;

#define addr6 addr._addr6
#define addr4 addr._addr4

static int search_host(struct sockaddr *);
static void remember_host(struct sockaddr *);

static int
search_host(struct sockaddr *sa)
{
	struct host_list *hp;
	
	if (!hosts)
		return 0;

	for (hp = hosts; hp != NULL; hp = hp->next) {
		switch (hp->family) {
		case AF_INET6:
			if (!memcmp(&hp->addr6,
			    &((struct sockaddr_in6 *)(void *)sa)->sin6_addr,
			    sizeof (struct in6_addr)))
				return 1;
			break;
		case AF_INET:
			if (!memcmp(&hp->addr4,
			    &((struct sockaddr_in *)(void *)sa)->sin_addr,
			    sizeof (struct in_addr)))
				return 1;
			break;
		default:
			break;
		}
	}
	return 0;
}

static void
remember_host(struct sockaddr *sa)
{
	struct host_list *hp;

	if ((hp = malloc(sizeof(struct host_list))) == NULL) {
		err(1, "malloc");
		/* NOTREACHED */
	}
	hp->family = sa->sa_family;
	hp->next = hosts;
	switch (sa->sa_family) {
	case AF_INET6:
		(void)memcpy(&hp->addr6,
		    &((struct sockaddr_in6 *)(void *)sa)->sin6_addr,
		    sizeof (struct in6_addr));
		break;
	case AF_INET:
		(void)memcpy(&hp->addr4,
		    &((struct sockaddr_in *)(void *)sa)->sin_addr,
		    sizeof (struct in_addr));
		break;
	default:
		errx(1, "unknown address family");
		/* NOTREACHED */
	}
	hosts = hp;
}

struct rup_data {
	const char *host;
	struct statstime statstime;
};
static struct rup_data *rup_data;
static size_t rup_data_idx = 0;
static size_t rup_data_max = 0;

enum sort_type { 
	SORT_NONE,
	SORT_HOST,
	SORT_LDAV,
	SORT_UPTIME
};
static enum sort_type sort_type;

static int compare(struct rup_data *, struct rup_data *);
static void remember_rup_data(const char *, struct statstime *);
static int rstat_reply(char *, struct netbuf *, struct netconfig *);
static void print_rup_data(const char *, statstime *);
static int onehost(char *);
static void allhosts(void);
static void usage(void) __dead;

int
compare(struct rup_data *d1, struct rup_data *d2)
{
	switch(sort_type) {
	case SORT_HOST:
		return strcmp(d1->host, d2->host);
	case SORT_LDAV:
		return d1->statstime.avenrun[0] 
		    - d2->statstime.avenrun[0];
	case SORT_UPTIME:
		return d1->statstime.boottime.tv_sec 
		    - d2->statstime.boottime.tv_sec;
	default:
		/* something's really wrong here */
		abort();
		/*NOTREACHED*/
	}
}

static void
remember_rup_data(const char *host, struct statstime *st)
{
	struct rup_data *n;

        if (rup_data_idx >= rup_data_max) {
                n = realloc(rup_data,
                    (rup_data_max + 16) * sizeof(struct rup_data));
                if (n == NULL) {
                        err (1, "realloc");
			/* NOTREACHED */
                }
		rup_data = n;
                rup_data_max += 16;
        }
	
	rup_data[rup_data_idx].host = strdup(host);
	rup_data[rup_data_idx].statstime = *st;
	rup_data_idx++;
}


static int
/*ARGSUSED*/
rstat_reply(char *replyp, struct netbuf *raddrp, struct netconfig *nconf)
{
	char host[NI_MAXHOST];
	statstime *host_stat = (statstime *)(void *)replyp;
	struct sockaddr *sa = raddrp->buf;

	if (!search_host(sa)) {
		if (getnameinfo(sa, (socklen_t)sa->sa_len, host, sizeof host,
		    NULL, 0, 0))
			return 0;

		remember_host(sa);

		if (sort_type != SORT_NONE) {
			remember_rup_data(host, host_stat);
		} else {
			print_rup_data(host, host_stat);
		}
	}

	return 0;
}


static void
print_rup_data(const char *host, statstime *host_stat)
{
	struct tm *tmp_time;
	struct tm host_time;
	unsigned ups = 0, upm = 0, uph = 0, upd = 0;
	time_t now;

	char days_buf[16];
	char hours_buf[16];

	if (printtime)
		(void)printf("%-*.*s", HOST_WIDTH-4, HOST_WIDTH-4, host);
	else
		(void)printf("%-*.*s", HOST_WIDTH, HOST_WIDTH, host);

	now = host_stat->curtime.tv_sec;
	tmp_time = localtime(&now);
	host_time = *tmp_time;

	host_stat->curtime.tv_sec -= host_stat->boottime.tv_sec;

	ups=host_stat->curtime.tv_sec;
	upd=ups / (3600 * 24);
	ups-=upd * 3600 * 24;
	uph=ups / 3600;
	ups-=uph * 3600;
	upm=ups / 60;

	if (upd != 0)
		(void)snprintf(days_buf, sizeof(days_buf), "%3u day%s, ", upd,
		    (upd > 1) ? "s" : "");
	else
		days_buf[0] = '\0';

	if (uph != 0)
		(void)snprintf(hours_buf, sizeof(hours_buf), "%2u:%02u, ",
		    uph, upm);
	else
		if (upm != 0)
			(void)snprintf(hours_buf, sizeof(hours_buf),
			    "%2u min%s ", upm, (upm == 1) ? ", " : "s,");
		else if (ups < 60)
			(void)snprintf(hours_buf, sizeof(hours_buf),
			    "%2u secs ", ups);
		else
			hours_buf[0] = '\0';
	if (printtime)
		(void)printf(" %2d:%02d%cm",
		    (host_time.tm_hour % 12) ? (host_time.tm_hour % 12) : 12,
		    host_time.tm_min, (host_time.tm_hour >= 12) ? 'p' : 'a');

	(void)printf(" up %9.9s%9.9s load average: %.2f %.2f %.2f\n",
	    days_buf, hours_buf, (double)host_stat->avenrun[0]/FSCALE,
	    (double)host_stat->avenrun[1]/FSCALE,
	    (double)host_stat->avenrun[2]/FSCALE);
}


static int
onehost(char *host)
{
	CLIENT *rstat_clnt;
	statstime host_stat;
	static struct timeval timeout = {25, 0};
	
	rstat_clnt = clnt_create(host, RSTATPROG, RSTATVERS_TIME, "udp");
	if (rstat_clnt == NULL) {
		warnx("%s", clnt_spcreateerror(host));
		return 1;
	}

	(void)memset(&host_stat, 0, sizeof(host_stat));
	if (clnt_call(rstat_clnt, RSTATPROC_STATS, xdr_void, NULL,
	    xdr_statstime, &host_stat, timeout) != RPC_SUCCESS) {
		warnx("%s",  clnt_sperror(rstat_clnt, host));
		clnt_destroy(rstat_clnt);
		return 1;
	}

	print_rup_data(host, &host_stat);
	clnt_destroy(rstat_clnt);
	return 0;
}

static void
allhosts(void)
{
	statstime host_stat;
	enum clnt_stat clnt_stat;
	size_t i;

	if (sort_type != SORT_NONE) {
		(void)printf("collecting responses...");
		(void)fflush(stdout);
	}

	clnt_stat = rpc_broadcast(RSTATPROG, RSTATVERS_TIME, RSTATPROC_STATS,
	    (xdrproc_t)xdr_void, NULL, (xdrproc_t)xdr_statstime, (caddr_t)(void *)&host_stat,
	    (resultproc_t)rstat_reply, "udp");
	if (clnt_stat != RPC_SUCCESS && clnt_stat != RPC_TIMEDOUT)
		errx(1, "%s", clnt_sperrno(clnt_stat));

	if (sort_type != SORT_NONE) {
		(void)putchar('\n');
		qsort(rup_data, rup_data_idx, sizeof(struct rup_data),
		      (int (*)(const void*, const void*))compare);

		for (i = 0; i < rup_data_idx; i++) {
			print_rup_data(rup_data[i].host, &rup_data[i].statstime);
		}
	}
}

int
main(int argc, char *argv[])
{
	int ch, retval;

	setprogname(*argv);
	sort_type = SORT_NONE;
	retval = 0;
	while ((ch = getopt(argc, argv, "dhlt")) != -1)
		switch (ch) {
		case 'd':
			printtime = 1;
			break;
		case 'h':
			sort_type = SORT_HOST;
			break;
		case 'l':
			sort_type = SORT_LDAV;
			break;
		case 't':
			sort_type = SORT_UPTIME;
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	
	(void)setlinebuf(stdout);

	if (argc == optind)
		allhosts();
	else {
		for (; optind < argc; optind++)
			retval += onehost(argv[optind]);
	}

	return retval ? EXIT_FAILURE : EXIT_SUCCESS;
}

void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-dhlt] [hosts ...]\n",
	    getprogname());
	exit(EXIT_SUCCESS);
}
