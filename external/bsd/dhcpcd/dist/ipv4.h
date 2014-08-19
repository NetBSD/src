/* $NetBSD: ipv4.h,v 1.1.1.1.2.3 2014/08/19 23:46:43 tls Exp $ */

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

#ifndef IPV4_H
#define IPV4_H

#include "dhcpcd.h"

struct rt {
	TAILQ_ENTRY(rt) next;
	struct in_addr dest;
	struct in_addr net;
	struct in_addr gate;
	const struct interface *iface;
	unsigned int metric;
	struct in_addr src;
};
TAILQ_HEAD(rt_head, rt);

struct ipv4_addr {
	TAILQ_ENTRY(ipv4_addr) next;
	struct in_addr addr;
	struct in_addr net;
	struct in_addr dst;
};
TAILQ_HEAD(ipv4_addrhead, ipv4_addr);

struct ipv4_state {
	struct ipv4_addrhead addrs;
};

#define IPV4_STATE(ifp)							       \
	((struct ipv4_state *)(ifp)->if_data[IF_DATA_IPV4])
#define IPV4_CSTATE(ifp)						       \
	((const struct ipv4_state *)(ifp)->if_data[IF_DATA_IPV4])

#ifdef INET
int ipv4_init(struct dhcpcd_ctx *);
void ipv4_sortinterfaces(struct dhcpcd_ctx *);
uint8_t inet_ntocidr(struct in_addr);
int inet_cidrtoaddr(int, struct in_addr *);
uint32_t ipv4_getnetmask(uint32_t);
int ipv4_addrexists(struct dhcpcd_ctx *, const struct in_addr *);

void ipv4_buildroutes(struct dhcpcd_ctx *);
void ipv4_applyaddr(void *);
int ipv4_routedeleted(struct dhcpcd_ctx *, const struct rt *);

struct ipv4_addr *ipv4_findaddr(struct interface *,
    const struct in_addr *, const struct in_addr *);
void ipv4_handleifa(struct dhcpcd_ctx *, int, struct if_head *, const char *,
    const struct in_addr *, const struct in_addr *, const struct in_addr *);

void ipv4_freeroutes(struct rt_head *);

void ipv4_free(struct interface *);
void ipv4_ctxfree(struct dhcpcd_ctx *);
#else
#define ipv4_init(a) (-1)
#define ipv4_sortinterfaces(a) {}
#define ipv4_applyaddr(a) {}
#define ipv4_freeroutes(a) {}
#define ipv4_free(a) {}
#define ipv4_ctxfree(a) {}
#define ipv4_addrexists(a, b) (0)
#endif

#endif
