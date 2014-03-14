/* $NetBSD: ipv6nd.h,v 1.1.1.3 2014/03/14 11:27:41 roy Exp $ */

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2014 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef IPV6ND_H
#define IPV6ND_H

#include <sys/queue.h>

#include <time.h>

#include "dhcpcd.h"
#include "ipv6.h"

struct ra_opt {
	TAILQ_ENTRY(ra_opt) next;
	uint8_t type;
	struct timeval expire;
	char *option;
};

struct ra {
	TAILQ_ENTRY(ra) next;
	struct interface *iface;
	struct in6_addr from;
	char sfrom[INET6_ADDRSTRLEN];
	unsigned char *data;
	ssize_t data_len;
	struct timeval received;
	unsigned char flags;
	uint32_t lifetime;
	uint32_t reachable;
	uint32_t retrans;
	uint32_t mtu;
	struct ipv6_addrhead addrs;
	TAILQ_HEAD(, ra_opt) options;

	unsigned char *ns;
	size_t nslen;
	int nsprobes;

	int expired;
};

TAILQ_HEAD(ra_head, ra);

struct rs_state {
	unsigned char *rs;
	size_t rslen;
	int rsprobes;
};

#define RS_STATE(a) ((struct rs_state *)(ifp)->if_data[IF_DATA_IPV6ND])

#define MAX_REACHABLE_TIME	3600	/* seconds */
#define REACHABLE_TIME		30	/* seconds */
#define RETRANS_TIMER		1000	/* milliseconds */
#define DELAY_FIRST_PROBE_TIME	5	/* seconds */

#ifdef INET6
int ipv6nd_startrs(struct interface *);
ssize_t ipv6nd_env(char **, const char *, const struct interface *);
int ipv6nd_addrexists(struct dhcpcd_ctx *, const struct ipv6_addr *);
void ipv6nd_freedrop_ra(struct ra *, int);
#define ipv6nd_free_ra(ra) ipv6nd_freedrop_ra((ra),  0)
#define ipv6nd_drop_ra(ra) ipv6nd_freedrop_ra((ra),  1)
ssize_t ipv6nd_free(struct interface *);
void ipv6nd_expirera(void *arg);
int ipv6nd_has_ra(const struct interface *);
void ipv6nd_handleifa(struct dhcpcd_ctx *, int,
    const char *, const struct in6_addr *, int);
void ipv6nd_drop(struct interface *);

void ipv6nd_proberouter(void *);
void ipv6nd_cancelproberouter(struct ra *);

#else
#define ipv6nd_startrs(a) {}
#define ipv6nd_addrexists(a, b) (0)
#define ipv6nd_free(a)
#define ipv6nd_has_ra(a) (0)
#define ipv6nd_drop(a)
#endif

#endif
