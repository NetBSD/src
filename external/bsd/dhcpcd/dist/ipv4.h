/* $NetBSD: ipv4.h,v 1.1.1.1.2.2 2013/06/23 06:26:31 tls Exp $ */

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2013 Roy Marples <roy@marples.name>
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
	int metric;
	struct in_addr src;
};
TAILQ_HEAD(rt_head, rt);

#ifdef INET
int ipv4_init(void);
int inet_ntocidr(struct in_addr);
int inet_cidrtoaddr(int, struct in_addr *);
uint32_t ipv4_getnetmask(uint32_t);

void ipv4_buildroutes(void);
void ipv4_applyaddr(void *);
int ipv4_routedeleted(const struct rt *);

void ipv4_handleifa(int, const char *,
    struct in_addr *, struct in_addr *, struct in_addr *);

int ipv4_doaddress(const char *,
    struct in_addr *, struct in_addr *, struct in_addr *, int);
int if_address(const struct interface *,
    const struct in_addr *, const struct in_addr *,
    const struct in_addr *, int);
#define ipv4_addaddress(iface, addr, net, brd)				      \
	if_address(iface, addr, net, brd, 1)
#define ipv4_setaddress(iface, addr, net, brd)				      \
	if_address(iface, addr, net, brd, 2)
#define ipv4_deleteaddress(iface, addr, net)				      \
	if_address(iface, addr, net, NULL, -1)
#define ipv4_hasaddress(iface, addr, net)				      \
	ipv4_doaddress(iface, addr, net, NULL, 0)
#define ipv4_getaddress(iface, addr, net, dst)				      \
	ipv4_doaddress(iface, addr, net, dst, 1)

int if_route(const struct rt *rt, int);
#define ipv4_addroute(rt) if_route(rt, 1)
#define ipv4_changeroute(rt) if_route(rt, 0)
#define ipv4_deleteroute(rt) if_route(rt, -1)
#define del_src_route(rt) i_route(rt, -2);
void ipv4_freeroutes(struct rt_head *);

int ipv4_opensocket(struct interface *, int);
ssize_t ipv4_sendrawpacket(const struct interface *,
    int, const void *, ssize_t);
ssize_t ipv4_getrawpacket(struct interface *, int, void *, ssize_t, int *);
#else
#define ipv4_init() -1
#define ipv4_applyaddr(a) {}
#define ipv4_freeroutes(a) {}
#endif

#endif
