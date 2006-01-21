/*	$NetBSD: udp_xfer.c,v 1.1 2006/01/21 10:32:23 dsl Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David Laight.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: udp_xfer.c,v 1.1 2006/01/21 10:32:23 dsl Exp $");

#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <netdb.h>
#include <err.h>

typedef uint32_t n_long;
#include <net.h>

void
set_port(struct sockaddr *sa, int port)
{

	switch (sa->sa_family) {
	case AF_INET:
		((struct sockaddr_in *)sa)->sin_port = port;
		break;
#ifdef INET6
	case AF_INET6:
		((struct sockaddr_in6 *)sa)->sin6_port = port;
		break;
#endif
	default:
		errx(1, "Unsupported socket family %d",
		    sa->sa_family);
	}
}

ssize_t
sendudp(struct iodesc *d, void *pkt, size_t len)
{
	int sock;
	int range = IP_PORTRANGE_LOW;

	if (d->socket >= 0) {
		close(d->socket);
		d->socket = -1;
	}

	sock = socket(d->ai->ai_family, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
		return -1;
	d->socket = sock;
	set_port(d->ai->ai_addr, d->destport);

	setsockopt(sock, IPPROTO_IP, IP_PORTRANGE, &range, sizeof(range));

	if (connect(sock, d->ai->ai_addr, d->ai->ai_addrlen) != 0)
		return -1;

	return send(sock, pkt, len, 0);
}

ssize_t
readudp(struct iodesc *d, void *pkt, size_t len, time_t tleft)
{
	struct pollfd pfd = {d->socket, POLLIN, 0};

	if (poll(&pfd, 1, tleft * 1000) != 1)
		return -1;

	return recv(d->socket, pkt, len, 0);
}
