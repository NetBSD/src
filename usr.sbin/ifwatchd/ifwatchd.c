/*	$NetBSD: ifwatchd.c,v 1.42.10.1 2018/03/15 09:12:08 pgoyette Exp $	*/
#include <sys/cdefs.h>
__RCSID("$NetBSD: ifwatchd.c,v 1.42.10.1 2018/03/15 09:12:08 pgoyette Exp $");

/*-
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Martin Husemann <martin@NetBSD.org>.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/wait.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <arpa/inet.h>

#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <err.h>
#include <ifaddrs.h>
#include <syslog.h>

enum event { ARRIVAL, DEPARTURE, UP, DOWN, CARRIER, NO_CARRIER };
enum addrflag { NOTREADY, DETACHED, READY };

/* local functions */
__dead static void usage(void);
static void dispatch(const void *, size_t);
static enum addrflag check_addrflags(int af, int addrflags);
static void check_addrs(const struct ifa_msghdr *ifam);
static void invoke_script(const char *ifname, enum event ev,
    const struct sockaddr *sa, const struct sockaddr *dst);
static void list_interfaces(const char *ifnames);
static void check_announce(const struct if_announcemsghdr *ifan);
static void check_carrier(const struct if_msghdr *ifm);
static void free_interfaces(void);
static struct interface_data * find_interface(int index);
static void run_initial_ups(void);

/* global variables */
static int verbose = 0, quiet = 0;
static int inhibit_initial = 0;
static const char *arrival_script = NULL;
static const char *departure_script = NULL;
static const char *up_script = NULL;
static const char *down_script = NULL;
static const char *carrier_script = NULL;
static const char *no_carrier_script = NULL;
static const char DummyTTY[] = _PATH_DEVNULL;
static const char DummySpeed[] = "9600";
static const char **scripts[] = {
	&arrival_script,
	&departure_script,
	&up_script,
	&down_script,
	&carrier_script,
	&no_carrier_script
};

struct interface_data {
	SLIST_ENTRY(interface_data) next;
	int index;
	int last_carrier_status;
	char * ifname;
};
static SLIST_HEAD(,interface_data) ifs = SLIST_HEAD_INITIALIZER(ifs);

int
main(int argc, char **argv)
{
	int c, s, n;
	int errs = 0;
	struct msghdr msg;
	struct iovec iov[1];
	char buf[2048];
	unsigned char msgfilter[] = {
		RTM_IFINFO, RTM_IFANNOUNCE,
		RTM_NEWADDR, RTM_DELADDR,
	};

	openlog(argv[0], LOG_PID|LOG_CONS, LOG_DAEMON);
	while ((c = getopt(argc, argv, "qvhic:n:u:d:A:D:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			return 0;

		case 'i':
			inhibit_initial = 1;
			break;

		case 'v':
			verbose++;
			break;

		case 'q':
			quiet = 1;
			break;

		case 'c':
			carrier_script = optarg;
			break;

		case 'n':
			no_carrier_script = optarg;
			break;

		case 'u':
			up_script = optarg;
			break;

		case 'd':
			down_script = optarg;
			break;

		case 'A':
			arrival_script = optarg;
			break;

		case 'D':
			departure_script = optarg;
			break;

		default:
			errs++;
			break;
		}
	}

	if (errs)
		usage();

	argv += optind;
	argc -= optind;

	if (argc <= 0)
		usage();

	if (verbose) {
		printf("up_script: %s\ndown_script: %s\n",
			up_script, down_script);
		printf("arrival_script: %s\ndeparture_script: %s\n",
			arrival_script, departure_script);
		printf("carrier_script: %s\nno_carrier_script: %s\n",
			carrier_script, no_carrier_script);
		printf("verbosity = %d\n", verbose);
	}

	while (argc > 0) {
		list_interfaces(argv[0]);
		argv++;
		argc--;
	}

	if (!verbose)
		daemon(0, 0);

	s = socket(PF_ROUTE, SOCK_RAW, 0);
	if (s < 0) {
		syslog(LOG_ERR, "error opening routing socket: %m");
		exit(EXIT_FAILURE);
	}
	if (setsockopt(s, PF_ROUTE, RO_MSGFILTER,
	    &msgfilter, sizeof(msgfilter)) < 0)
		syslog(LOG_ERR, "RO_MSGFILTER: %m");

	if (!inhibit_initial)
		run_initial_ups();

	iov[0].iov_base = buf;
	iov[0].iov_len = sizeof(buf);
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	for (;;) {
		n = recvmsg(s, &msg, 0);
		if (n == -1) {
			syslog(LOG_ERR, "recvmsg: %m");
			exit(EXIT_FAILURE);
		}
		if (n != 0)
			dispatch(iov[0].iov_base, n);
	}

	close(s);
	free_interfaces();
	closelog();

	return EXIT_SUCCESS;
}

static void
usage(void)
{
	fprintf(stderr,
	    "usage:\n"
	    "\tifwatchd [-hiqv] [-A arrival-script] [-D departure-script]\n"
	    "\t\t  [-d down-script] [-u up-script]\n"
	    "\t\t  [-c carrier-script] [-n no-carrier-script] ifname(s)\n"
	    "\twhere:\n"
	    "\t -A <cmd> specify command to run on interface arrival event\n"
	    "\t -c <cmd> specify command to run on interface carrier-detect event\n"
	    "\t -D <cmd> specify command to run on interface departure event\n"
	    "\t -d <cmd> specify command to run on interface down event\n"
	    "\t -n <cmd> specify command to run on interface no-carrier-detect event\n"
	    "\t -h       show this help message\n"
	    "\t -i       no (!) initial run of the up script if the interface\n"
	    "\t          is already up on ifwatchd startup\n"
	    "\t -q       quiet mode, don't syslog informational messages\n"
	    "\t -u <cmd> specify command to run on interface up event\n"
	    "\t -v       verbose/debug output, don't run in background\n");
	exit(EXIT_FAILURE);
}

static void
dispatch(const void *msg, size_t len)
{
	const struct rt_msghdr *hd = msg;

	if (hd->rtm_version != RTM_VERSION)
		return;

	switch (hd->rtm_type) {
	case RTM_NEWADDR:
	case RTM_DELADDR:
		check_addrs(msg);
		break;
	case RTM_IFANNOUNCE:
		check_announce(msg);
		break;
	case RTM_IFINFO:
		check_carrier(msg);
		break;
	default:
		/* Should be impossible as we filter messages. */
		if (verbose)
			printf("unknown message ignored (%d)\n", hd->rtm_type);
		break;
	}
}

static enum addrflag
check_addrflags(int af, int addrflags)
{

	switch (af) {
	case AF_INET:
		if (addrflags & IN_IFF_NOTREADY)
			return NOTREADY;
		if (addrflags & IN_IFF_DETACHED)
			return DETACHED;
		break;
	case AF_INET6:
		if (addrflags & IN6_IFF_NOTREADY)
			return NOTREADY;
		if (addrflags & IN6_IFF_DETACHED)
			return DETACHED;
		break;
	}
	return READY;
}

static void
check_addrs(const struct ifa_msghdr *ifam)
{
	const char *cp = (const char *)(ifam + 1);
	const struct sockaddr *sa, *ifa = NULL, *brd = NULL;
	unsigned i;
	struct interface_data *ifd = NULL;
	int aflag;
	enum event ev;

	if (ifam->ifam_addrs == 0)
		return;
	for (i = 1; i; i <<= 1) {
		if ((i & ifam->ifam_addrs) == 0)
			continue;
		sa = (const struct sockaddr *)cp;
		if (i == RTA_IFP) {
			const struct sockaddr_dl *li;

			li = (const struct sockaddr_dl *)sa;
			if ((ifd = find_interface(li->sdl_index)) == NULL) {
				if (verbose)
					printf("ignoring change"
					    " on interface #%d\n",
					    li->sdl_index);
				return;
			}
		} else if (i == RTA_IFA)
			ifa = sa;
		else if (i == RTA_BRD)
			brd = sa;
		RT_ADVANCE(cp, sa);
	}
	if (ifa != NULL && ifd != NULL) {
		ev = ifam->ifam_type == RTM_DELADDR ? DOWN : UP;
		aflag = check_addrflags(ifa->sa_family, ifam->ifam_addrflags);
		if ((ev == UP && aflag == READY) || ev == DOWN)
			invoke_script(ifd->ifname, ev, ifa, brd);
	}
}

static void
invoke_script(const char *ifname, enum event ev,
    const struct sockaddr *sa, const struct sockaddr *dest)
{
	char addr[NI_MAXHOST], daddr[NI_MAXHOST];
	const char *script;
	int status;

	if (ifname == NULL)
		return;

	script = *scripts[ev];
	if (script == NULL)
		return;

	addr[0] = daddr[0] = 0;
	if (sa != NULL) {
		const struct sockaddr_in *sin;
		const struct sockaddr_in6 *sin6;

		if (sa->sa_len == 0) {
			syslog(LOG_ERR,
			    "illegal socket address (sa_len == 0)");
			return;
		}
		switch (sa->sa_family) {
		case AF_INET:
			sin = (const struct sockaddr_in *)sa;
			if (sin->sin_addr.s_addr == INADDR_ANY ||
			    sin->sin_addr.s_addr == INADDR_BROADCAST)
				return;
			break;
		case AF_INET6:
			sin6 = (const struct sockaddr_in6 *)sa;
			if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))
				return;
			break;
		default:
			break;
		}

		if (getnameinfo(sa, sa->sa_len, addr, sizeof addr, NULL, 0,
		    NI_NUMERICHOST)) {
			if (verbose)
				printf("getnameinfo failed\n");
			return;	/* this address can not be handled */
		}
	}

	if (dest != NULL) {
		if (getnameinfo(dest, dest->sa_len, daddr, sizeof daddr,
		    NULL, 0, NI_NUMERICHOST)) {
			if (verbose)
				printf("getnameinfo failed\n");
			return;	/* this address can not be handled */
		}
	}

	if (verbose)
		(void) printf("calling: %s %s %s %s %s %s\n",
		    script, ifname, DummyTTY, DummySpeed, addr, daddr);
	if (!quiet)
		syslog(LOG_INFO, "calling: %s %s %s %s %s %s\n",
		    script, ifname, DummyTTY, DummySpeed, addr, daddr);

	switch (vfork()) {
	case -1:
		syslog(LOG_ERR, "cannot fork: %m");
		break;
	case 0:
		if (execl(script, script, ifname, DummyTTY, DummySpeed,
		    addr, daddr, NULL) == -1) {
			syslog(LOG_ERR, "could not execute \"%s\": %m",
			    script);
		}
		_exit(EXIT_FAILURE);
	default:
		(void) wait(&status);
	}
}

static void
list_interfaces(const char *ifnames)
{
	char * names = strdup(ifnames);
	char * name, *lasts;
	static const char sep[] = " \t";
	struct interface_data * p;

	for (name = strtok_r(names, sep, &lasts);
	    name != NULL;
	    name = strtok_r(NULL, sep, &lasts)) {
		p = malloc(sizeof(*p));
		SLIST_INSERT_HEAD(&ifs, p, next);
		p->last_carrier_status = -1;
		p->ifname = strdup(name);
		p->index = if_nametoindex(p->ifname);
		if (!quiet)
			syslog(LOG_INFO, "watching interface %s", p->ifname);
		if (verbose)
			printf("interface \"%s\" has index %d\n",
			    p->ifname, p->index);
	}
	free(names);
}

static void
check_carrier(const struct if_msghdr *ifm)
{
	struct interface_data * p;
	int carrier_status;
	enum event ev;

	SLIST_FOREACH(p, &ifs, next)
		if (p->index == ifm->ifm_index)
			break;

	if (p == NULL)
		return;

	/*
	 * Treat it as an event worth handling if:
	 * - the carrier status changed, or
	 * - this is the first time we've been called, and
	 * inhibit_initial is not set
	 */
	carrier_status = ifm->ifm_data.ifi_link_state;
	if (carrier_status != p->last_carrier_status) {
		switch (carrier_status) {
		case LINK_STATE_UP:
			ev = CARRIER;
			break;
		case LINK_STATE_DOWN:
			ev = NO_CARRIER;
			break;
		default:
			if (verbose)
				printf("unknown link status ignored\n");
			return;
		}
		invoke_script(p->ifname, ev, NULL, NULL);
		p->last_carrier_status = carrier_status;
	}
}

static void
check_announce(const struct if_announcemsghdr *ifan)
{
	struct interface_data * p;
	const char *ifname = ifan->ifan_name;

	SLIST_FOREACH(p, &ifs, next) {
		if (strcmp(p->ifname, ifname) != 0)
			continue;

		switch (ifan->ifan_what) {
		case IFAN_ARRIVAL:
			p->index = ifan->ifan_index;
			invoke_script(p->ifname, ARRIVAL, NULL, NULL);
			break;
		case IFAN_DEPARTURE:
			p->index = -1;
			p->last_carrier_status = -1;
			invoke_script(p->ifname, DEPARTURE, NULL, NULL);
			break;
		default:
			if (verbose)
				(void) printf("unknown announce: "
				    "what=%d\n", ifan->ifan_what);
			break;
		}
		return;
	}
}

static void
free_interfaces(void)
{
	struct interface_data * p;

	while (!SLIST_EMPTY(&ifs)) {
		p = SLIST_FIRST(&ifs);
		SLIST_REMOVE_HEAD(&ifs, next);
		free(p->ifname);
		free(p);
	}
}

static struct interface_data *
find_interface(int idx)
{
	struct interface_data * p;

	SLIST_FOREACH(p, &ifs, next)
		if (p->index == idx)
			return p;
	return NULL;
}

static void
run_initial_ups(void)
{
	struct interface_data * ifd;
	struct ifaddrs *res = NULL, *p;
	struct sockaddr *ifa;
	int s, aflag;

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		return;

	if (getifaddrs(&res) != 0)
		goto out;

	for (p = res; p; p = p->ifa_next) {
		SLIST_FOREACH(ifd, &ifs, next) {
			if (strcmp(ifd->ifname, p->ifa_name) == 0)
				break;
		}
		if (ifd == NULL)
			continue;

		ifa = p->ifa_addr;
		if (ifa != NULL && ifa->sa_family == AF_LINK)
			invoke_script(ifd->ifname, ARRIVAL, NULL, NULL);

		if ((p->ifa_flags & IFF_UP) == 0)
			continue;
		if (ifa == NULL)
			continue;
		if (ifa->sa_family == AF_LINK) {
			struct ifmediareq ifmr;

			memset(&ifmr, 0, sizeof(ifmr));
			strncpy(ifmr.ifm_name, ifd->ifname,
			    sizeof(ifmr.ifm_name));
			if (ioctl(s, SIOCGIFMEDIA, &ifmr) != -1
			    && (ifmr.ifm_status & IFM_AVALID)
			    && (ifmr.ifm_status & IFM_ACTIVE)) {
				invoke_script(ifd->ifname, CARRIER, NULL, NULL);
				ifd->last_carrier_status =
				    LINK_STATE_UP;
			    }
			continue;
		}
		aflag = check_addrflags(ifa->sa_family, p->ifa_addrflags);
		if (aflag != READY)
			continue;
		invoke_script(ifd->ifname, UP, ifa, p->ifa_dstaddr);
	}
	freeifaddrs(res);
out:
	close(s);
}
