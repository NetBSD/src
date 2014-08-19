/*	$NetBSD: server.c,v 1.8.8.2 2014/08/20 00:00:23 tls Exp $	*/
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
__RCSID("$NetBSD: server.c,v 1.8.8.2 2014/08/20 00:00:23 tls Exp $");

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

static struct sockaddr_in *daemons;
static unsigned int numdaemons, maxdaemons;

static struct sockaddr_in *broadcastaddrs;
static int numbroadcastaddrs;

static bool initial = true;
static struct in_addr local_address;
static const char *explicit_host;
static uint16_t port;

void
serverlist_setup(const char *explicit_host_arg, uint16_t port_arg)
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

	if (explicit_host_arg) {
		explicit_host = explicit_host_arg;
	}
	port = port_arg;
}

static void
add_daemon_addr(const struct sockaddr_storage *addr, socklen_t addrlen,
		uint16_t port_num)
{
	const struct sockaddr_in *sin;

	if (addr->ss_family != AF_INET) {
		return;
	}
	assert(addrlen == sizeof(struct sockaddr_in));
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
	 *
	 * The result may be either the port number for the hunt
	 * socket or the port number for the stats socket... or the
	 * number of players connected and not a port number at all,
	 * depending on the packet type.
	 *
	 * For now at least it is ok to stuff it in here, because the
	 * usage of the various different packet types and the
	 * persistence of the results vs. the program exiting does not
	 * cause us to get confused. If the game is made more
	 * self-contained in the future we'll need to be more careful
	 * about this, especially if we make the caching of results
	 * less scattershot.
	 */
	daemons[numdaemons] = *sin;
	daemons[numdaemons].sin_port = port_num;
	numdaemons++;
}

static bool
have_daemon_addr(const struct sockaddr_storage *addr, socklen_t addrlen)
{
	unsigned j;
	const struct sockaddr_in *sin;

	if (addr->ss_family != AF_INET) {
		return false;
	}
	assert(addrlen == sizeof(struct sockaddr_in));
	sin = (const struct sockaddr_in *)addr;

	for (j = 0; j < numdaemons; j++) {
		if (sin->sin_addr.s_addr == daemons[j].sin_addr.s_addr) {
			return true;
		}
	}
	return false;
}

static void
getbroadcastaddrs(void)
{
	unsigned num, i;
	struct ifaddrs *ifp, *ip;

	broadcastaddrs = NULL;
	numbroadcastaddrs = 0;

	if (getifaddrs(&ifp) < 0) {
		return;
	}

	num = 0;
	for (ip = ifp; ip; ip = ip->ifa_next) {
		if ((ip->ifa_addr->sa_family == AF_INET) &&
		    (ip->ifa_flags & IFF_BROADCAST)) {
			num++;
		}
	}

	broadcastaddrs = malloc(num * sizeof(broadcastaddrs[0]));
	if (broadcastaddrs == NULL) {
		leavex(1, "Out of memory");
	}

	i = 0;
	for (ip = ifp; ip; ip = ip->ifa_next) {
		if ((ip->ifa_addr->sa_family == AF_INET) &&
		    (ip->ifa_flags & IFF_BROADCAST)) {
			memcpy(&broadcastaddrs[i], ip->ifa_broadaddr,
			       sizeof(broadcastaddrs[i]));
			i++;
		}
	}
	assert(i == num);
	numbroadcastaddrs = num;

	freeifaddrs(ifp);
}

static void
send_messages(int contactsock, unsigned short msg)
{
	struct sockaddr_in contactaddr;
	struct hostent *hp;
	uint16_t wiremsg;
	int option;
	int i;

	memset(&contactaddr, 0, sizeof(contactaddr));
	contactaddr.sin_family = SOCK_FAMILY;
	contactaddr.sin_port = htons(port);

	if (explicit_host != NULL) {	/* explicit host given */
		hp = gethostbyname(explicit_host);
		if (hp == NULL) {
			leavex(1, "%s: Unknown host", explicit_host);
		}
		memcpy(&contactaddr.sin_addr, hp->h_addr,
		       sizeof(contactaddr.sin_addr));
		wiremsg = htons(msg);
		(void) sendto(contactsock, &wiremsg, sizeof(wiremsg), 0,
			      (struct sockaddr *)&contactaddr,
			      sizeof(contactaddr));
		return;
	}

	if (!initial) {
		/* favor host of previous session by contacting it first */
		contactaddr.sin_addr = Daemon.sin_addr;

		/* Must be playing! */
		assert(msg == C_PLAYER);
		wiremsg = htons(msg);

		(void) sendto(contactsock, &wiremsg, sizeof(wiremsg), 0,
		    (struct sockaddr *)&contactaddr, sizeof(contactaddr));
	}

	if (initial) {
		getbroadcastaddrs();
	}

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
	for (i = 0; i < numbroadcastaddrs; i++) {
		contactaddr.sin_addr = broadcastaddrs[i].sin_addr;
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
		if (have_daemon_addr(&addr, addrlen)) {
			/* this shouldn't happen */
			continue;
		}

		add_daemon_addr(&addr, addrlen, port_num);
	}

	initial = false;
}

void
serverlist_query(unsigned short msg)
{
	int contactsock;

	if (!initial && explicit_host != NULL) {
		/* already did the work, no point doing it again */
		return;
	}

	contactsock = socket(SOCK_FAMILY, SOCK_DGRAM, 0);
	if (contactsock < 0) {
		leave(1, "socket system call failed");
	}

	send_messages(contactsock, msg);
	get_responses(contactsock);

	(void) close(contactsock);
}

unsigned
serverlist_num(void)
{
	return numdaemons;
}

const struct sockaddr_storage *
serverlist_gethost(unsigned i, socklen_t *len_ret)
{
	struct sockaddr_in *ret;

	assert(i < numdaemons);
	ret = &daemons[i];
	*len_ret = sizeof(*ret);
	return (struct sockaddr_storage *)ret;
}

unsigned short
serverlist_getresponse(unsigned i)
{
	assert(i < numdaemons);
	return daemons[i].sin_port;
}

#endif /* INTERNET */
