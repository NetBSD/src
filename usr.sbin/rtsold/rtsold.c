/*	$NetBSD: rtsold.c,v 1.41 2014/03/25 17:17:44 joerg Exp $	*/
/*	$KAME: rtsold.c,v 1.77 2004/01/03 01:35:13 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/param.h>

#include <net/if.h>
#include <net/if_dl.h>

#include <netinet/in.h>
#include <netinet/icmp6.h>

#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <stdarg.h>
#include <ifaddrs.h>
#include <util.h>
#include <poll.h>

#include "rtsold.h"

struct ifinfo *iflist;
struct timeval tm_max =	{0x7fffffff, 0x7fffffff};
static int log_upto = 999;
static int fflag = 0;
static int isdaemon = 0;

int aflag = 0;
int dflag = 0;

/* protocol constatns */
#define MAX_RTR_SOLICITATION_DELAY	1 /* second */
#define RTR_SOLICITATION_INTERVAL	4 /* seconds */
#define MAX_RTR_SOLICITATIONS		3 /* times */

/* 
 * implementation dependent constants in seconds
 * XXX: should be configurable 
 */
#define PROBE_INTERVAL 60

/* static variables and functions */
static int mobile_node = 0;
#ifndef SMALL
static int do_dump;
static const char *dumpfilename = "/var/run/rtsold.dump"; /* XXX: should be configurable */
#endif

#if 0
static int ifreconfig(char *);
#endif
static int make_packet(struct ifinfo *);
static struct timeval *rtsol_check_timer(void);

#ifndef SMALL
static void rtsold_set_dump_file(int);
#endif
__dead static void usage(void);

int
main(int argc, char **argv)
{
	int s, ch, once = 0;
	struct timeval *timeout;
	const char *ident;
	const char *opts;
	struct pollfd set[2];
#ifdef USE_RTSOCK
	int rtsock;
#endif

	/*
	 * Initialization
	 */
	ident = getprogname();
	isdaemon = ident[strlen(ident) - 1] == 'd';

	/* get option */
	if (!isdaemon) {
		fflag = 1;
		once = 1;
		opts = "adD";
	} else
		opts = "adDfm1";

	while ((ch = getopt(argc, argv, opts)) != -1) {
		switch (ch) {
		case 'a':
			aflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'D':
			dflag = 2;
			break;
		case 'f':
			fflag = 1;
			break;
		case 'm':
			mobile_node = 1;
			break;
		case '1':
			once = 1;
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}
	argc -= optind;
	argv += optind;

	if ((!aflag && argc == 0) || (aflag && argc != 0)) {
		usage();
		/*NOTREACHED*/
	}

	/* set log level */
	if (dflag == 0)
		log_upto = LOG_NOTICE;
	if (!fflag) {
		openlog(ident, LOG_NDELAY|LOG_PID, LOG_DAEMON);
		if (log_upto >= 0)
			setlogmask(LOG_UPTO(log_upto));
	}

	/* warn if accept_rtadv is down */
	if (!getinet6sysctl(IPV6CTL_ACCEPT_RTADV))
		warnx("kernel is configured not to accept RAs");
	/* warn if forwarding is up */
	if (getinet6sysctl(IPV6CTL_FORWARDING))
		warnx("kernel is configured as a router, not a host");

#ifndef SMALL
	/* initialization to dump internal status to a file */
	signal(SIGUSR1, rtsold_set_dump_file);
#endif

	if (!fflag)
		daemon(0, 0);		/* act as a daemon */

	/*
	 * Open a socket for sending RS and receiving RA.
	 * This should be done before calling ifinit(), since the function
	 * uses the socket.
	 */
	if ((s = sockopen()) < 0) {
		warnmsg(LOG_ERR, __func__, "failed to open a socket");
		exit(1);
		/*NOTREACHED*/
	}
	set[0].fd = s;
	set[0].events = POLLIN;

	set[1].fd = -1;

#ifdef USE_RTSOCK
	if ((rtsock = rtsock_open()) < 0) {
		warnmsg(LOG_ERR, __func__, "failed to open a socket");
		exit(1);
		/*NOTREACHED*/
	}
	set[1].fd = rtsock;
	set[1].events = POLLIN;
#endif

	/* configuration per interface */
	if (ifinit()) {
		warnmsg(LOG_ERR, __func__,
		    "failed to initilizatoin interfaces");
		exit(1);
		/*NOTREACHED*/
	}
	if (aflag)
		argv = autoifprobe();
	while (argv && *argv) {
		if (ifconfig(*argv)) {
			warnmsg(LOG_ERR, __func__,
			    "failed to initialize %s", *argv);
			exit(1);
			/*NOTREACHED*/
		}
		argv++;
	}

	/* setup for probing default routers */
	if (probe_init()) {
		warnmsg(LOG_ERR, __func__,
		    "failed to setup for probing routers");
		exit(1);
		/*NOTREACHED*/
	}

	/* dump the current pid */
	if (!once) {
		if (pidfile(NULL) < 0) {
			warnmsg(LOG_ERR, __func__,
			    "failed to open a pid log file: %s",
			    strerror(errno));
		}
	}

	for (;;) {		/* main loop */
		int e;

#ifndef SMALL
		if (do_dump) {	/* SIGUSR1 */
			do_dump = 0;
			rtsold_dump_file(dumpfilename);
		}
#endif

		timeout = rtsol_check_timer();

		if (once) {
			struct ifinfo *ifi;

			/* if we have no timeout, we are done (or failed) */
			if (timeout == NULL)
				break;

			/* if all interfaces have got RA packet, we are done */
			for (ifi = iflist; ifi; ifi = ifi->next) {
				if (ifi->state != IFS_DOWN && ifi->racnt == 0)
					break;
			}
			if (ifi == NULL)
				break;
		}
		e = poll(set, 2, (int)(timeout ?
		    (timeout->tv_sec * 1000 + timeout->tv_usec / 1000) :
		    INFTIM));
		if (e < 1) {
			if (e < 0 && errno != EINTR) {
				warnmsg(LOG_ERR, __func__, "poll: %s",
				    strerror(errno));
			}
			continue;
		}

		/* packet reception */
#ifdef USE_RTSOCK
		if (set[1].revents & POLLIN)
			rtsock_input(rtsock);
#endif
		if (set[0].revents & POLLIN)
			rtsol_input(s);
	}
	/* NOTREACHED */

	return 0;
}

int
ifconfig(char *ifname)
{
	struct ifinfo *ifinfo;
	struct sockaddr_dl *sdl;
	int flags;

	if ((sdl = if_nametosdl(ifname)) == NULL) {
		warnmsg(LOG_ERR, __func__,
		    "failed to get link layer information for %s", ifname);
		return -1;
	}
	if (find_ifinfo(sdl->sdl_index)) {
		warnmsg(LOG_ERR, __func__,
		    "interface %s was already configured", ifname);
		free(sdl);
		return -1;
	}

	if ((ifinfo = malloc(sizeof(*ifinfo))) == NULL) {
		warnmsg(LOG_ERR, __func__, "memory allocation failed");
		free(sdl);
		return -1;
	}
	memset(ifinfo, 0, sizeof(*ifinfo));
	ifinfo->sdl = sdl;

	strlcpy(ifinfo->ifname, ifname, sizeof(ifinfo->ifname));

	/* construct a router solicitation message */
	if (make_packet(ifinfo))
		goto bad;

	/* set link ID of this interface. */
#ifdef HAVE_SCOPELIB
	if (inet_zoneid(AF_INET6, 2, ifname, &ifinfo->linkid))
		goto bad;
#else
	/* XXX: assume interface IDs as link IDs */
	ifinfo->linkid = ifinfo->sdl->sdl_index;
#endif

	/*
	 * check if the interface is available.
	 * also check if SIOCGIFMEDIA ioctl is OK on the interface.
	 */
	ifinfo->mediareqok = 1;
	ifinfo->active = interface_status(ifinfo);
	if (!ifinfo->mediareqok) {
		/*
		 * probe routers periodically even if the link status
		 * does not change.
		 */
		ifinfo->probeinterval = PROBE_INTERVAL;
	}

	/* activate interface: interface_up returns 0 on success */
	flags = interface_up(ifinfo->ifname);
	if (flags == 0)
		ifinfo->state = IFS_DELAY;
	else if (flags == IFS_TENTATIVE)
		ifinfo->state = IFS_TENTATIVE;
	else
		ifinfo->state = IFS_DOWN;

	rtsol_timer_update(ifinfo);

	/* link into chain */
	if (iflist)
		ifinfo->next = iflist;
	iflist = ifinfo;

	return 0;

bad:
	free(ifinfo->sdl);
	free(ifinfo);
	return -1;
}

void
iflist_init(void)
{
	struct ifinfo *ifi, *next;

	for (ifi = iflist; ifi; ifi = next) {
		next = ifi->next;
		if (ifi->sdl)
			free(ifi->sdl);
		if (ifi->rs_data)
			free(ifi->rs_data);
		free(ifi);
		iflist = NULL;
	}
}

#if 0
static int
ifreconfig(char *ifname)
{
	struct ifinfo *ifi, *prev;
	int rv;

	prev = NULL;
	for (ifi = iflist; ifi; ifi = ifi->next) {
		if (strncmp(ifi->ifname, ifname, sizeof(ifi->ifname)) == 0)
			break;
		prev = ifi;
	}
	prev->next = ifi->next;

	rv = ifconfig(ifname);

	/* reclaim it after ifconfig() in case ifname is pointer inside ifi */
	if (ifi->rs_data)
		free(ifi->rs_data);
	free(ifi->sdl);
	free(ifi);
	return rv;
}
#endif

struct ifinfo *
find_ifinfo(size_t ifindex)
{
	struct ifinfo *ifi;

	for (ifi = iflist; ifi; ifi = ifi->next)
		if (ifi->sdl->sdl_index == ifindex)
			return ifi;
	return NULL;
}

static int
make_packet(struct ifinfo *ifinfo)
{
	size_t packlen = sizeof(struct nd_router_solicit), lladdroptlen;
	struct nd_router_solicit *rs;
	u_char *buf;

	if ((lladdroptlen = lladdropt_length(ifinfo->sdl)) == 0) {
		warnmsg(LOG_INFO, __func__,
		    "link-layer address option has null length"
		    " on %s. Treat as not included.", ifinfo->ifname);
	}
	packlen += lladdroptlen;
	ifinfo->rs_datalen = packlen;

	/* allocate buffer */
	if ((buf = malloc(packlen)) == NULL) {
		warnmsg(LOG_ERR, __func__,
		    "memory allocation failed for %s", ifinfo->ifname);
		return -1;
	}
	ifinfo->rs_data = buf;

	/* fill in the message */
	rs = (struct nd_router_solicit *)buf;
	rs->nd_rs_type = ND_ROUTER_SOLICIT;
	rs->nd_rs_code = 0;
	rs->nd_rs_cksum = 0;
	rs->nd_rs_reserved = 0;
	buf += sizeof(*rs);

	/* fill in source link-layer address option */
	if (lladdroptlen)
		lladdropt_fill(ifinfo->sdl, (struct nd_opt_hdr *)buf);

	return 0;
}

static struct timeval *
rtsol_check_timer(void)
{
	static struct timeval returnval;
	struct timeval now, rtsol_timer;
	struct ifinfo *ifinfo;
	int flags;

	gettimeofday(&now, NULL);

	rtsol_timer = tm_max;

	for (ifinfo = iflist; ifinfo; ifinfo = ifinfo->next) {
		if (timercmp(&ifinfo->expire, &now, <=)) {
			if (dflag > 1)
				warnmsg(LOG_DEBUG, __func__,
				    "timer expiration on %s, "
				    "state = %d", ifinfo->ifname,
				    ifinfo->state);

			switch (ifinfo->state) {
			case IFS_DOWN:
			case IFS_TENTATIVE:
				/* interface_up returns 0 on success */
				flags = interface_up(ifinfo->ifname);
				if (flags == 0)
					ifinfo->state = IFS_DELAY;
				else if (flags == IFS_TENTATIVE)
					ifinfo->state = IFS_TENTATIVE;
				else
					ifinfo->state = IFS_DOWN;
				break;
			case IFS_IDLE:
			{
				int oldstatus = ifinfo->active;
				int probe = 0;

				ifinfo->active = interface_status(ifinfo);

				if (oldstatus != ifinfo->active) {
					warnmsg(LOG_DEBUG, __func__,
					    "%s status is changed"
					    " from %d to %d",
					    ifinfo->ifname,
					    oldstatus, ifinfo->active);
					probe = 1;
					ifinfo->state = IFS_DELAY;
				} else if (ifinfo->probeinterval &&
				    (ifinfo->probetimer -= (int)
				    ifinfo->timer.tv_sec) <= 0) {
					/* probe timer expired */
					ifinfo->probetimer =
					    ifinfo->probeinterval;
					probe = 1;
					ifinfo->state = IFS_PROBE;
				}

				if (probe && mobile_node)
					defrouter_probe(ifinfo);
				break;
			}
			case IFS_DELAY:
				ifinfo->state = IFS_PROBE;
				sendpacket(ifinfo);
				break;
			case IFS_PROBE:
				if (ifinfo->probes < MAX_RTR_SOLICITATIONS)
					sendpacket(ifinfo);
				else {
					warnmsg(LOG_INFO, __func__,
					    "No answer after sending %d RSs",
					    ifinfo->probes);
					ifinfo->probes = 0;
					ifinfo->state = IFS_IDLE;
				}
				break;
			}
			rtsol_timer_update(ifinfo);
		}

		if (timercmp(&ifinfo->expire, &rtsol_timer, <))
			rtsol_timer = ifinfo->expire;
	}

	if (timercmp(&rtsol_timer, &tm_max, ==)) {
		warnmsg(LOG_DEBUG, __func__, "there is no timer");
		return NULL;
	} else if (timercmp(&rtsol_timer, &now, <))
		/* this may occur when the interval is too small */
		returnval.tv_sec = returnval.tv_usec = 0;
	else
		timersub(&rtsol_timer, &now, &returnval);

	if (dflag > 1)
		warnmsg(LOG_DEBUG, __func__, "New timer is %ld:%08ld",
		    (long)returnval.tv_sec, (long)returnval.tv_usec);

	return &returnval;
}

void
rtsol_timer_update(struct ifinfo *ifinfo)
{
#define MILLION 1000000
#define DADRETRY 10		/* XXX: adhoc */
	uint32_t interval;
	struct timeval now;

	bzero(&ifinfo->timer, sizeof(ifinfo->timer));

	switch (ifinfo->state) {
	case IFS_DOWN:
	case IFS_TENTATIVE:
		if (++ifinfo->dadcount > DADRETRY) {
			ifinfo->dadcount = 0;
			ifinfo->timer.tv_sec = PROBE_INTERVAL;
		} else
			ifinfo->timer.tv_sec = 1;
		break;
	case IFS_IDLE:
		if (mobile_node) {
			/* XXX should be configurable */
			ifinfo->timer.tv_sec = 3;
		} else
			ifinfo->timer = tm_max;	/* stop timer(valid?) */
		break;
	case IFS_DELAY:
		interval = arc4random() % (MAX_RTR_SOLICITATION_DELAY * MILLION);
		ifinfo->timer.tv_sec = interval / MILLION;
		ifinfo->timer.tv_usec = (suseconds_t)(interval % MILLION);
		break;
	case IFS_PROBE:
		if (ifinfo->probes < MAX_RTR_SOLICITATIONS)
			ifinfo->timer.tv_sec = RTR_SOLICITATION_INTERVAL;
		else {
			/*
			 * After sending MAX_RTR_SOLICITATIONS solicitations,
			 * we're just waiting for possible replies; there
			 * will be no more solicatation.  Thus, we change
			 * the timer value to MAX_RTR_SOLICITATION_DELAY based
			 * on RFC 2461, Section 6.3.7.
			 */
			ifinfo->timer.tv_sec = MAX_RTR_SOLICITATION_DELAY;
		}
		break;
	default:
		warnmsg(LOG_ERR, __func__,
		    "illegal interface state(%d) on %s",
		    ifinfo->state, ifinfo->ifname);
		return;
	}

	/* reset the timer */
	if (timercmp(&ifinfo->timer, &tm_max, ==)) {
		ifinfo->expire = tm_max;
		warnmsg(LOG_DEBUG, __func__,
		    "stop timer for %s", ifinfo->ifname);
	} else {
		gettimeofday(&now, NULL);
		timeradd(&now, &ifinfo->timer, &ifinfo->expire);

		if (dflag > 1)
			warnmsg(LOG_DEBUG, __func__,
			    "set timer for %s to %d:%d", ifinfo->ifname,
			    (int)ifinfo->timer.tv_sec,
			    (int)ifinfo->timer.tv_usec);
	}

#undef MILLION
}

/* timer related utility functions */
#define MILLION 1000000

#ifndef SMALL
static void
rtsold_set_dump_file(int sig)
{
	do_dump = 1;
}
#endif

static void
usage(void)
{
	const char *opts = isdaemon ? "-1Ddfm" : "Dd";
	fprintf(stderr, "Usage: %s [%s] interface ...\n", getprogname(), opts);
	fprintf(stderr, "\t%s [%s] -a\n", getprogname(), opts);
	exit(1);
}

void
warnmsg(int priority, const char *func, const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	if (fflag) {
		if (priority <= log_upto)
			vwarnx(msg, ap);
	} else {
		char buf[BUFSIZ];
		snprintf(buf, sizeof(buf), "<%s> %s", func, msg);
		vsyslog(priority, buf, ap);
	}
	va_end(ap);
}

/*
 * return a list of interfaces which is suitable to sending an RS.
 */
char **
autoifprobe(void)
{
	static char **argv = NULL;
	static size_t n = 0;
	char **a;
	size_t i;
	int found;
	struct ifaddrs *ifap, *ifa;

	/* initialize */
	while (n--)
		free(argv[n]);
	if (argv) {
		free(argv);
		argv = NULL;
	}
	n = 0;

	if (getifaddrs(&ifap) != 0)
		return NULL;

	/* find an ethernet */
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if ((ifa->ifa_flags & IFF_UP) == 0)
			continue;
		if ((ifa->ifa_flags & IFF_POINTOPOINT) != 0)
			continue;
		if ((ifa->ifa_flags & IFF_LOOPBACK) != 0)
			continue;
		if ((ifa->ifa_flags & IFF_MULTICAST) == 0)
			continue;

		if (ifa->ifa_addr->sa_family != AF_INET6)
			continue;

		found = 0;
		for (i = 0; i < n; i++) {
			if (strcmp(argv[i], ifa->ifa_name) == 0) {
				found++;
				break;
			}
		}
		if (found)
			continue;

		/* if we find multiple candidates, just warn. */
		if (n != 0 && dflag > 1)
			warnx("multiple interfaces found");

		a = realloc(argv, (n + 1) * sizeof(char **));
		if (a == NULL)
			err(EXIT_FAILURE, "realloc");
		argv = a;
		argv[n] = strdup(ifa->ifa_name);
		if (!argv[n])
			err(EXIT_FAILURE, "strdup");
		n++;
	}

	if (n) {
		a = realloc(argv, (n + 1) * sizeof(char **));
		if (a == NULL)
			err(EXIT_FAILURE, "realloc");
		argv = a;
		argv[n] = NULL;

		if (dflag > 0) {
			for (i = 0; i < n; i++)
				warnx("probing %s", argv[i]);
		}
	}
	freeifaddrs(ifap);
	return argv;
}
