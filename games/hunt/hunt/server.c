/*	$NetBSD: server.c,v 1.5 2014/03/30 03:26:19 dholland Exp $	*/
/*
 * Copyright (c) 1983-2003, Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * + Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * + Neither the name of the University of California, San Francisco nor
 *   the names of its contributors may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: server.c,v 1.5 2014/03/30 03:26:19 dholland Exp $");

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <ifaddrs.h>

#include "hunt_common.h"
#include "pathnames.h"
#include "hunt_private.h"

#ifdef INTERNET

/*
 * Code for finding and talking to hunt daemons.
 */

static SOCKET *daemons;
static unsigned int numdaemons, maxdaemons;

static bool initial = true;
static struct in_addr local_address;
static int brdc;
static SOCKET *brdv;

static void
serverlist_setup(void)
{
	char local_name[MAXHOSTNAMELEN + 1];
	struct hostent *hp;

	if (gethostname(local_name, sizeof(local_name)) < 0) {
		leavex(1, "Sorry, I have no hostname.");
	}
	local_name[sizeof(local_name) - 1] = '\0';
	if ((hp = gethostbyname(local_name)) == NULL) {
		leavex(1, "Can't find myself.");
	}
	memcpy(&local_address, hp->h_addr, sizeof(local_address));

	numdaemons = 0;
	maxdaemons = 20;
	daemons = malloc(maxdaemons * sizeof(daemons[0]));
	if (daemons == NULL) {
		leavex(1, "Out of memory.");
	}
}

static void
add_daemon_addr(const struct sockaddr_storage *addr, uint16_t port_num)
{
	const struct sockaddr_in *sin;

	if (addr->ss_family != AF_INET) {
		return;
	}
	sin = (const struct sockaddr_in *)addr;

	assert(numdaemons <= maxdaemons);
	if (numdaemons == maxdaemons) {
		maxdaemons += 20;
		daemons = realloc(daemons, maxdaemons * sizeof(daemons[0]));
		if (daemons == NULL) {
			leave(1, "realloc");
		}
	}

	/*
	 * Note that we do *not* convert from network to host
	 * order since the port number we were sent *should*
	 * already be in network order.
	 */
	daemons[numdaemons] = *sin;
	daemons[numdaemons].sin_port = port_num;
	numdaemons++;
}

static bool
have_daemon_addr(const struct sockaddr_storage *addr)
{
	unsigned j;
	const struct sockaddr_in *sin;

	if (addr->ss_family != AF_INET) {
		return false;
	}
	sin = (const struct sockaddr_in *)addr;

	for (j = 0; j < numdaemons; j++) {
		if (sin->sin_addr.s_addr == daemons[j].sin_addr.s_addr) {
			return true;
		}
	}
	return false;
}

static int
getbroadcastaddrs(struct sockaddr_in **vector)
{
	int vec_cnt;
	struct ifaddrs *ifp, *ip;

	*vector = NULL;
	if (getifaddrs(&ifp) < 0)
		return 0;

	vec_cnt = 0;
	for (ip = ifp; ip; ip = ip->ifa_next)
		if ((ip->ifa_addr->sa_family == AF_INET) &&
		    (ip->ifa_flags & IFF_BROADCAST))
			vec_cnt++;

	*vector = malloc(vec_cnt * sizeof(struct sockaddr_in));

	vec_cnt = 0;
	for (ip = ifp; ip; ip = ip->ifa_next)
		if ((ip->ifa_addr->sa_family == AF_INET) &&
		    (ip->ifa_flags & IFF_BROADCAST))
			memcpy(&(*vector)[vec_cnt++], ip->ifa_broadaddr,
			       sizeof(struct sockaddr_in));

	freeifaddrs(ifp);
	return vec_cnt;
}

static void
get_responses(int contactsock)
{
	struct pollfd set[1];
	struct sockaddr_storage addr;
	socklen_t addrlen;
	int r;
	uint16_t port_num;
	ssize_t portlen;

	/* forget all old responses */
	numdaemons = 0;

	errno = 0;
	set[0].fd = contactsock;
	set[0].events = POLLIN;
	for (;;) {
		r = poll(set, 1, 1000);
		if (r < 0) {
			if (errno == EINTR) {
				continue;
			}
			leave(1, "poll");
		}
		if (r == 0) {
			break;
		}

		addrlen = sizeof(addr);
		portlen = recvfrom(contactsock, &port_num, sizeof(port_num), 0,
				   (struct sockaddr *)&addr, &addrlen);
		if (portlen < 0) {
			if (errno == EINTR) {
				continue;
			}
			leave(1, "recvfrom");
		}
		if (portlen == 0) {
			leavex(1, "recvfrom: Unexpected EOF");
		}
		if ((size_t)portlen != sizeof(port_num)) {
			/* trash, ignore it */
			continue;
		}
		if (have_daemon_addr(&addr)) {
			/* this shouldn't happen */
			continue;
		}

		add_daemon_addr(&addr, port_num);
	}

	/* terminate list with local address */
	{
		struct sockaddr_in sin;

		sin.sin_family = SOCK_FAMILY;
		sin.sin_addr = local_address;
		sin.sin_port = htons(0);
		add_daemon_addr((struct sockaddr_storage *)&sin, htons(0));
	}

	initial = false;
}

SOCKET *
list_drivers(unsigned short msg)
{
	struct hostent *hp;
	struct sockaddr_in contactaddr;
	int option;
	uint16_t wiremsg;
	int contactsock;
	int i;

	if (initial) {
		/* do one time initialization */
		serverlist_setup();
	}

	if (!initial && Sock_host != NULL) {
		/* address already valid */
		return daemons;
	}

	contactsock = socket(SOCK_FAMILY, SOCK_DGRAM, 0);
	if (contactsock < 0) {
		leave(1, "socket system call failed");
	}
	contactaddr.sin_family = SOCK_FAMILY;
	contactaddr.sin_port = htons(Test_port);

	if (Sock_host != NULL) {	/* explicit host given */
		if ((hp = gethostbyname(Sock_host)) == NULL) {
			leavex(1, "Unknown host");
			/* NOTREACHED */
		}
		memcpy(&contactaddr.sin_addr, hp->h_addr,
		       sizeof(contactaddr.sin_addr));
		wiremsg = htons(msg);
		(void) sendto(contactsock, &wiremsg, sizeof(wiremsg), 0,
			      (struct sockaddr *)&contactaddr,
			      sizeof(contactaddr));
		get_responses(contactsock);
		(void) close(contactsock);
		return daemons;
	}

	if (!initial) {
		/* favor host of previous session by contacting it first */
		contactaddr.sin_addr = Daemon.sin_addr;
		wiremsg = htons(C_PLAYER);		/* Must be playing! */
		(void) sendto(contactsock, &wiremsg, sizeof(wiremsg), 0,
		    (struct sockaddr *)&contactaddr, sizeof(contactaddr));
	}

	if (initial)
		brdc = getbroadcastaddrs(&brdv);

#ifdef SO_BROADCAST
	/* Sun's will broadcast even though this option can't be set */
	option = 1;
	if (setsockopt(contactsock, SOL_SOCKET, SO_BROADCAST,
	    &option, sizeof option) < 0) {
		leave(1, "setsockopt broadcast");
		/* NOTREACHED */
	}
#endif

	/* send broadcast packets on all interfaces */
	wiremsg = htons(msg);
	for (i = 0; i < brdc; i++) {
		contactaddr.sin_addr = brdv[i].sin_addr;
		if (sendto(contactsock, &wiremsg, sizeof(wiremsg), 0,
		    (struct sockaddr *)&contactaddr,
		    sizeof(contactaddr)) < 0) {
			leave(1, "sendto");
		}
	}
	contactaddr.sin_addr = local_address;
	if (sendto(contactsock, &wiremsg, sizeof(wiremsg), 0,
	    (struct sockaddr *)&contactaddr, sizeof(contactaddr)) < 0) {
		leave(1, "sendto");
	}

	get_responses(contactsock);
	(void) close(contactsock);
	return daemons;
}

#endif /* INTERNET */
