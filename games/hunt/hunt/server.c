/*	$NetBSD: server.c,v 1.1 2014/03/30 02:26:09 dholland Exp $	*/
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
__RCSID("$NetBSD: server.c,v 1.1 2014/03/30 02:26:09 dholland Exp $");

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>

#include "hunt_common.h"
#include "pathnames.h"
#include "hunt_private.h"

#ifdef INTERNET

static int
broadcast_vec(int s /*socket*/, struct sockaddr_in **vector)
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

SOCKET *
list_drivers(void)
{
	int option;
	u_short msg;
	u_short port_num;
	static SOCKET test;
	int test_socket;
	socklen_t namelen;
	char local_name[MAXHOSTNAMELEN + 1];
	static bool initial = true;
	static struct in_addr local_address;
	struct hostent *hp;
	static int brdc;
	static SOCKET *brdv;
	int i;
	unsigned j;
	static SOCKET *listv;
	static unsigned int listmax;
	unsigned int listc;
	struct pollfd set[1];

	if (initial) {			/* do one time initialization */
		if (gethostname(local_name, sizeof local_name) < 0) {
			leavex(1, "Sorry, I have no name.");
			/* NOTREACHED */
		}
		local_name[sizeof(local_name) - 1] = '\0';
		if ((hp = gethostbyname(local_name)) == NULL) {
			leavex(1, "Can't find myself.");
			/* NOTREACHED */
		}
		local_address = * ((struct in_addr *) hp->h_addr);

		listmax = 20;
		listv = (SOCKET *) malloc(listmax * sizeof (SOCKET));
	} else if (Sock_host != NULL)
		return listv;		/* address already valid */

	test_socket = socket(SOCK_FAMILY, SOCK_DGRAM, 0);
	if (test_socket < 0) {
		leave(1, "socket system call failed");
		/* NOTREACHED */
	}
	test.sin_family = SOCK_FAMILY;
	test.sin_port = htons(Test_port);
	listc = 0;

	if (Sock_host != NULL) {	/* explicit host given */
		if ((hp = gethostbyname(Sock_host)) == NULL) {
			leavex(1, "Unknown host");
			/* NOTREACHED */
		}
		test.sin_addr = *((struct in_addr *) hp->h_addr);
		goto test_one_host;
	}

	if (!initial) {
		/* favor host of previous session by broadcasting to it first */
		test.sin_addr = Daemon.sin_addr;
		msg = htons(C_PLAYER);		/* Must be playing! */
		(void) sendto(test_socket, &msg, sizeof msg, 0,
		    (struct sockaddr *) &test, sizeof(test));
	}

	if (initial)
		brdc = broadcast_vec(test_socket, &brdv);

#ifdef SO_BROADCAST
	/* Sun's will broadcast even though this option can't be set */
	option = 1;
	if (setsockopt(test_socket, SOL_SOCKET, SO_BROADCAST,
	    &option, sizeof option) < 0) {
		leave(1, "setsockopt broadcast");
		/* NOTREACHED */
	}
#endif

	/* send broadcast packets on all interfaces */
	msg = htons(C_TESTMSG());
	for (i = 0; i < brdc; i++) {
		test.sin_addr = brdv[i].sin_addr;
		if (sendto(test_socket, &msg, sizeof msg, 0,
		    (struct sockaddr *) &test, sizeof(test)) < 0) {
			leave(1, "sendto");
			/* NOTREACHED */
		}
	}
	test.sin_addr = local_address;
	if (sendto(test_socket, &msg, sizeof msg, 0,
	    (struct sockaddr *) &test, sizeof(test)) < 0) {
		leave(1, "sendto");
		/* NOTREACHED */
	}

get_response:
	namelen = sizeof(test);
	errno = 0;
	set[0].fd = test_socket;
	set[0].events = POLLIN;
	for (;;) {
		if (listc + 1 >= listmax) {
			SOCKET *newlistv;

			listmax += 20;
			newlistv = realloc(listv, listmax * sizeof(*listv));
			if (newlistv == NULL)
				leave(1, "realloc");
			listv = newlistv;
		}

		if (poll(set, 1, 1000) == 1 &&
		    recvfrom(test_socket, &port_num, sizeof(port_num),
		    0, (struct sockaddr *) &listv[listc], &namelen) > 0) {
			/*
			 * Note that we do *not* convert from network to host
			 * order since the port number *should* be in network
			 * order:
			 */
			for (j = 0; j < listc; j += 1)
				if (listv[listc].sin_addr.s_addr
				== listv[j].sin_addr.s_addr)
					break;
			if (j == listc)
				listv[listc++].sin_port = port_num;
			continue;
		}

		if (errno != 0 && errno != EINTR) {
			leave(1, "poll/recvfrom");
			/* NOTREACHED */
		}

		/* terminate list with local address */
		listv[listc].sin_family = SOCK_FAMILY;
		listv[listc].sin_addr = local_address;
		listv[listc].sin_port = htons(0);

		(void) close(test_socket);
		initial = false;
		return listv;
	}

test_one_host:
	msg = htons(C_TESTMSG());
	(void) sendto(test_socket, &msg, sizeof msg, 0,
	    (struct sockaddr *) &test, sizeof(test));
	goto get_response;
}

#endif /* INTERNET */
