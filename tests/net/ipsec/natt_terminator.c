/*	$NetBSD: natt_terminator.c,v 1.1 2017/10/30 15:59:23 ozaki-r Exp $	*/

/*-
 * Copyright (c) 2017 Internet Initiative Japan Inc.
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
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <netinet/udp.h>

#include <stdio.h>
#include <err.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	struct addrinfo hints;
	struct addrinfo *res;
	int s, e;
	const char *addr, *port;
	int option;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <addr> <port>\n", argv[0]);
		return 1;
	}

	addr = argv[1];
	port = argv[2];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = 0;

	e = getaddrinfo(addr, port, &hints, &res);
	if (e != 0)
		errx(EXIT_FAILURE, "getaddrinfo failed: %s", gai_strerror(e));

	s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (s == -1)
		err(EXIT_FAILURE, "socket");

	/*
	 * Set the option to tell the kernel that the socket can handle
	 * UDP-encapsulated ESP packets for NAT-T.
	 */
	option = UDP_ENCAP_ESPINUDP;
	e = setsockopt(s, IPPROTO_UDP, UDP_ENCAP, &option, sizeof(option));
	if (e == -1)
		err(EXIT_FAILURE, "setsockopt(UDP_ENCAP)");

	e = bind(s, res->ai_addr, res->ai_addrlen);
	if (e == -1)
		err(EXIT_FAILURE, "bind");

	/* Receiving a packet make the NAPT create a mapping. */
	{
		char buf[64];
		struct sockaddr_storage z;
		socklen_t len = sizeof(z);

		e = recvfrom(s, buf, 64, MSG_PEEK,
		    (struct sockaddr *)&z, &len);
		if (e == -1)
			err(EXIT_FAILURE, "recvfrom");
	}

	/*
	 * Keep the socket in the kernel to handle UDP-encapsulated ESP packets.
	 */
	pause();

	close(s);

	return 0;
}
