/*	$NetBSD: ifwatchd.c,v 1.11 2003/03/06 13:33:29 martin Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Martin Husemann <martin@NetBSD.ORG>.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Define this for special treatment of sys/net/if_spppsubr.c based interfaces.
 */
#define SPPP_IF_SUPPORT
 
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/wait.h>
#include <net/if.h>
#include <net/if_dl.h>
#ifdef SPPP_IF_SUPPORT
#include <net/if_sppp.h>
#endif
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <err.h>
#include <ifaddrs.h>

enum event { ARRIVAL, DEPARTURE, UP, DOWN };
/* local functions */
static void usage(void);
static void dispatch(void*, size_t);
static void check_addrs(char *cp, int addrs, enum event ev);
static void invoke_script(struct sockaddr *sa, struct sockaddr *dst, enum event ev, int ifindex, const char *ifname_hint);
static void list_interfaces(const char *ifnames);
static void check_announce(struct if_announcemsghdr *ifan);
static void rescan_interfaces(void);
static void free_interfaces(void);
static int find_interface(int index);
static void run_initial_ups(void);

#ifdef SPPP_IF_SUPPORT
static int if_is_connected(const char * ifname);
#else
#define	if_is_connected(X)	1
#endif

/* stolen from /sbin/route */
#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

/* global variables */
static int verbose = 0;
static int inhibit_initial = 0;
static const char *arrival_script = NULL;
static const char *departure_script = NULL;
static const char *up_script = NULL;
static const char *down_script = NULL;
static char DummyTTY[] = _PATH_DEVNULL;
static char DummySpeed[] = "9600";
static const char **scripts[] = {
	&arrival_script,
	&departure_script,
	&up_script,
	&down_script
    };

struct interface_data {
	SLIST_ENTRY(interface_data) next;
	int index;
	char * ifname;
};
SLIST_HEAD(,interface_data) ifs = SLIST_HEAD_INITIALIZER(ifs);

int
main(int argc, char **argv)
{
	int c, s, n;
	int errs = 0;
	char msg[2048], *msgp;

	while ((c = getopt(argc, argv, "vhiu:d:A:D:")) != -1)
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
		printf("verbosity = %d\n", verbose);
	}

	while (argc > 0) {
	    list_interfaces(argv[0]);
	    argv++; argc--;
	}

	if (!verbose)
		daemon(0,0);

	s = socket(PF_ROUTE, SOCK_RAW, 0);
	if (s < 0) {
		perror("open routing socket");
		exit(EXIT_FAILURE);
	}

	if (!inhibit_initial)
		run_initial_ups();

	for (;;) {
		n = read(s, msg, sizeof msg);
		msgp = msg;
		for (msgp = msg; n > 0; n -= ((struct rt_msghdr*)msgp)->rtm_msglen, msgp += ((struct rt_msghdr*)msgp)->rtm_msglen) {
			dispatch(msgp, n);
			
		}
	}

	close(s);
	free_interfaces();

	return EXIT_SUCCESS;
}

static void
usage()
{
	fprintf(stderr, 
	    "usage:\n"
	    "\tifwatchd [-h] [-v] [-u up-script] [-d down-script] [-A arrival-script] [-D departure-script] ifname(s)\n"
	    "\twhere:\n"
	    "\t -h       show this help message\n"
	    "\t -v       verbose/debug output, don't run in background\n"
	    "\t -i       no (!) initial run of the up script if the interface\n"
	    "\t          is already up on ifwatchd startup\n"
	    "\t -u <cmd> specify command to run on interface up event\n"
	    "\t -d <cmd> specify command to run on interface down event\n"
	    "\t -A <cmd> specify command to run on interface arrival event\n"
	    "\t -D <cmd> specify command to run on interface departure event\n");
	exit(EXIT_FAILURE);
}

static void
dispatch(void *msg, size_t len)
{
	struct rt_msghdr *hd = msg;
	struct ifa_msghdr *ifam;
	enum event ev;

	switch (hd->rtm_type) {
	case RTM_NEWADDR:
		ev = UP;
		goto work;
	case RTM_DELADDR:
		ev = DOWN;
		goto work;
	case RTM_IFANNOUNCE:
		rescan_interfaces();
		check_announce((struct if_announcemsghdr *)msg);
		return;
	}
	if (verbose)
		printf("unknown message ignored\n");
	return;

work:
	ifam = (struct ifa_msghdr *)msg;
	check_addrs((char *)(ifam + 1), ifam->ifam_addrs, ev);
}

static void
check_addrs(cp, addrs, ev)
	char    *cp;
	int     addrs;
	enum event ev;
{
	struct sockaddr *sa, *ifa = NULL, *brd = NULL;
	int ifndx = 0, i;

	if (addrs == 0)
	    return;
	for (i = 1; i; i <<= 1) {
	    if (i & addrs) {
		sa = (struct sockaddr *)cp;
		if (i == RTA_IFP) {
		    struct sockaddr_dl * li = (struct sockaddr_dl*)sa;
		    ifndx = li->sdl_index;
		    if (!find_interface(ifndx)) {
			if (verbose)
			    printf("ignoring change on interface #%d\n", ifndx);
			return;
		    }
		} else if (i == RTA_IFA) {
		    ifa = sa;
		} else if (i == RTA_BRD) {
		    brd = sa;
		}
		ADVANCE(cp, sa);
	    }
	}
	if (ifa != NULL)
	    invoke_script(ifa, brd, ev, ifndx, NULL);
}

static void
invoke_script(sa, dest, ev, ifindex, ifname_hint)
	struct sockaddr *sa, *dest;
	enum event ev;
	int ifindex;
	const char *ifname_hint;
{
	char addr[NI_MAXHOST], daddr[NI_MAXHOST], ifname_buf[IFNAMSIZ];
	const char *ifname;
	const char *script;
	int status;

	if (sa != NULL && sa->sa_len == 0) {
	    fprintf(stderr, "illegal socket address (sa_len == 0)\n");
	    return;
	}
	if (sa != NULL && sa->sa_family == AF_INET6) {
		struct sockaddr_in6 sin6;

		(void) memcpy(&sin6, (struct sockaddr_in6 *)sa, sizeof (sin6));
		if (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr))
			return;
	}

	addr[0] = daddr[0] = 0;
	ifname = if_indextoname(ifindex, ifname_buf);
	ifname = ifname ? ifname : ifname_hint;
	if (ifname == NULL)
	    return;

	if (sa != NULL) {
	if (getnameinfo(sa, sa->sa_len, addr, sizeof addr, NULL, 0, NI_NUMERICHOST)) {
	    if (verbose)
		printf("getnameinfo failed\n");
	    return;	/* this address can not be handled */
	}
	}
	if (dest != NULL) {
	    if (getnameinfo(dest, dest->sa_len, daddr, sizeof daddr, NULL, 0, NI_NUMERICHOST)) {
		if (verbose)
		    printf("getnameinfo failed\n");
		return;	/* this address can not be handled */
	    }
	}

	script = *scripts[ev];
	if (script == NULL) return;

	if (verbose)
	    (void) printf("calling: %s %s %s %s %s %s\n",
		script, ifname, DummyTTY, DummySpeed, addr, daddr);

	switch (vfork()) {
	case -1:
	    fprintf(stderr, "cannot fork\n");
	    break;
	case 0:
	    (void) execl(script, script, ifname, DummyTTY, DummySpeed,
		addr, daddr, NULL);
	    _exit(EXIT_FAILURE);
	default:
	    (void) wait(&status);
	}
}

static void list_interfaces(const char *ifnames)
{
	char * names = strdup(ifnames);
	char * name, *lasts;
	static const char sep[] = " \t";
	struct interface_data * p;

	for (name = strtok_r(names, sep, &lasts); name != NULL; name = strtok_r(NULL, sep, &lasts)) {
	    p = malloc(sizeof(*p));
	    SLIST_INSERT_HEAD(&ifs, p, next);
	    p->ifname = strdup(name);
	    p->index = if_nametoindex(p->ifname);
	    if (verbose)
		printf("interface \"%s\" has index %d\n", p->ifname, p->index);
	}
	free(names);
}

static void
check_announce(struct if_announcemsghdr *ifan)
{
	struct interface_data * p;
	const char *ifname = ifan->ifan_name;

	SLIST_FOREACH(p, &ifs, next) {
	    if (strcmp(p->ifname, ifname) == 0) {
	        switch (ifan->ifan_what) {
		case IFAN_ARRIVAL:
		    invoke_script(NULL, NULL, ARRIVAL, p->index, NULL);
		    break;
		case IFAN_DEPARTURE:
		    invoke_script(NULL, NULL, DEPARTURE, p->index, p->ifname);
		    break;
		default:
		    if (verbose)
			(void) printf("unknown announce: what=%d\n", ifan->ifan_what);
		    break;
		}
		return;
	    }
	}
}

static void rescan_interfaces()
{
	struct interface_data * p;
	
	SLIST_FOREACH(p, &ifs, next) {
	    p->index = if_nametoindex(p->ifname);
	    if (verbose)
		printf("interface \"%s\" has index %d\n", p->ifname, p->index);
	}
}

static void free_interfaces()
{
	struct interface_data * p;

	while (!SLIST_EMPTY(&ifs)) {
	    p = SLIST_FIRST(&ifs);
	    SLIST_REMOVE_HEAD(&ifs, next);
	    free(p->ifname);
	    free(p);
	}
}

static int find_interface(index)
	int index;
{
	struct interface_data * p;
	
	SLIST_FOREACH(p, &ifs, next)
	    if (p->index == index)
		return 1;
	return 0;
}

static void run_initial_ups()
{
	struct interface_data * ifd;
	struct ifaddrs *res = NULL, *p;

	if (getifaddrs(&res) == 0) {
	    for (p = res; p; p = p->ifa_next) {
		SLIST_FOREACH(ifd, &ifs, next) {
		    if (strcmp(ifd->ifname, p->ifa_name) == 0)
			break;
		}
		if (ifd == NULL)
		    continue;

		if (p->ifa_addr && p->ifa_addr->sa_family == AF_LINK)
		    invoke_script(NULL, NULL, ARRIVAL, ifd->index, NULL);

		if ((p->ifa_flags & IFF_UP) == 0)
		    continue;
		if (p->ifa_addr == NULL)
		    continue;
		if (p->ifa_addr->sa_family == AF_LINK)
		    continue;
		if (if_is_connected(ifd->ifname))
		    invoke_script(p->ifa_addr, p->ifa_dstaddr, UP, ifd->index, ifd->ifname);
	    }
	    freeifaddrs(res);
	}
}

#ifdef SPPP_IF_SUPPORT
/*
 * Special case support for in-kernel PPP interfaces.
 * If these are IFF_UP, but have not yet connected or completed authentication
 * we don't want to call the up script in the initial interface scan (there
 * will be an UP event generated later, when IPCP completes, anyway).
 *
 * If this is no if_spppsubr.c based interface, this ioctl just fails and we
 * treat is as connected.
 */
static int
if_is_connected(const char * ifname)
{
	int s, err;
	struct spppstatus status;

	memset(&status, 0, sizeof status);
	strncpy(status.ifname, ifname, sizeof status.ifname);
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
	    return 1;	/* no idea how to handle this... */
	err = ioctl(s, SPPPGETSTATUS, &status);
	if (err != 0)
	    /* not if_spppsubr.c based - call it connected */
	    status.phase = SPPP_PHASE_NETWORK;
	close(s);
	return status.phase == SPPP_PHASE_NETWORK;
}
#endif
