/*	$NetBSD: siocXmulti.c,v 1.3 2021/08/19 03:36:42 yamaguchi Exp $	*/

/*
 * Copyright (c) 2021 Internet Initiative Japan Inc.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: siocXmulti.c,v 1.3 2021/08/19 03:36:42 yamaguchi Exp $");

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <err.h>

enum{
	ARG_PROG = 0,
	ARG_OP,
	ARG_IFNAME,
	ARG_ADDR,
	ARG_NUM
};

static void
usage(void)
{

	printf("%s <add|del> <ifname> <IPv4 addr>\n",
	    getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	int			 fd, rv;
	unsigned long		 req;
	struct ifreq		 ifr;
	unsigned int		 ifidx;
	struct sockaddr_in	*sin;
	struct sockaddr_in6	*sin6;

	bzero(&ifr, sizeof(ifr));

	if (argc != ARG_NUM)
		usage();

	if (strcmp(argv[ARG_OP], "add") == 0)
		req = SIOCADDMULTI;
	else if (strcmp(argv[ARG_OP], "del") == 0)
		req = SIOCDELMULTI;
	else
		usage();

	ifidx = if_nametoindex(argv[ARG_IFNAME]);
	if (ifidx == 0)
		err(1, "if_nametoindex(%s)", argv[ARG_IFNAME]);

	strncpy(ifr.ifr_name, argv[ARG_IFNAME], sizeof(ifr.ifr_name) - 1);
	ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';

	sin = (struct sockaddr_in *)&ifr.ifr_addr;
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);
	rv = inet_pton(AF_INET, argv[ARG_ADDR], &sin->sin_addr);

	if (rv != 1) {
		sin6 = (struct sockaddr_in6 *)&ifr.ifr_addr;
		sin6->sin6_family = AF_INET6;
		sin6->sin6_len = sizeof(*sin6);
		rv = inet_pton(AF_INET6, argv[ARG_ADDR], &sin6->sin6_addr);

		if (rv != 1)
			errx(1, "inet_pton(%s)", argv[ARG_ADDR]);
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		err(1, "socket");

	if (ioctl(fd, req, (caddr_t)&ifr) < 0) {
		err(1, "ioctl(%s)",
		    (req == SIOCADDMULTI) ? "SIOCADDMULTI" : "SIOCDELMULTI");
	}

	close(fd);

	return 0;
}
