/*	$NetBSD: npf.c,v 1.2 2016/12/10 22:09:18 christos Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: npf.c,v 1.2 2016/12/10 22:09:18 christos Exp $");

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

#include <net/if.h>
#include <netinet/in.h>

#include <npf.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>

#include "identd.h"

int
npf_natlookup(const struct sockaddr_storage *ss,
    struct sockaddr_storage *nat_addr, in_port_t *nat_lport)
{
	npf_addr_t *addr[2];
	in_port_t port[2];
	int dev, af;

	/* copy the source into nat_addr so it is writable */
	memcpy(nat_addr, &ss[0], sizeof(ss[0]));

	switch (af = ss[0].ss_family) {
	case AF_INET:
		addr[0] = (void *)&satosin(nat_addr)->sin_addr;
		addr[1] = __UNCONST(&csatosin(&ss[1])->sin_addr);
		port[0] = csatosin(&ss[0])->sin_port;
		port[1] = csatosin(&ss[1])->sin_port;
		break;
	case AF_INET6:
		addr[0] = (void *)&satosin6(&nat_addr)->sin6_addr;
		addr[1] = __UNCONST(&csatosin6(&ss[0])->sin6_addr);
		port[0] = csatosin6(&ss[0])->sin6_port;
		port[1] = csatosin6(&ss[1])->sin6_port;
		break;
	default:
		errno = EAFNOSUPPORT;
		maybe_syslog(LOG_ERR, "NAT lookup for %d: %m" , af);
		return 0;
	}

	/* Open the /dev/pf device and do the lookup. */
	if ((dev = open("/dev/npf", O_RDWR)) == -1) {
		maybe_syslog(LOG_ERR, "Cannot open /dev/npf: %m");
		return 0;
	}
	if (npf_nat_lookup(dev, af, addr, port, IPPROTO_TCP, PFIL_OUT) == -1) {
		maybe_syslog(LOG_ERR, "NAT lookup failure: %m");
		(void)close(dev);
		return 0;
	}
	(void)close(dev);

	/*
	 * The originating address is already set into nat_addr so fill
	 * in the rest, family, port (ident), len....
	 */
	switch (af) {
	case AF_INET:
		satosin(nat_addr)->sin_port = htons(113);
		satosin(nat_addr)->sin_len = sizeof(struct sockaddr_in);
		satosin(nat_addr)->sin_family = AF_INET;
		break;
	case AF_INET6:
		satosin6(nat_addr)->sin6_port = htons(113);
		satosin6(nat_addr)->sin6_len = sizeof(struct sockaddr_in6);
		satosin6(nat_addr)->sin6_family = AF_INET6;
		break;
	}
	/* Put the originating port into nat_lport. */
	*nat_lport = ntohs(port[0]);

	return 1;
}
