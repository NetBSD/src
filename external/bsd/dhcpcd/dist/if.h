/* $NetBSD: if.h,v 1.1.1.6 2014/10/06 18:20:19 roy Exp $ */

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

#ifndef INTERFACE_H
#define INTERFACE_H

#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>

#include "config.h"
#include "dhcpcd.h"
#include "ipv4.h"
#include "ipv6.h"

/* Some systems have route metrics */
#ifndef HAVE_ROUTE_METRIC
# if defined(__linux__) || defined(SIOCGIFPRIORITY)
#  define HAVE_ROUTE_METRIC 1
# endif
# ifndef HAVE_ROUTE_METRIC
#  define HAVE_ROUTE_METRIC 0
# endif
#endif

/* Neighbour reachability and router updates */
#ifndef HAVE_RTM_GETNEIGH
# ifdef __linux__
#  define HAVE_RTM_GETNEIGH
# endif
#endif

#define EUI64_ADDR_LEN			8
#define INFINIBAND_ADDR_LEN		20

/* Linux 2.4 doesn't define this */
#ifndef ARPHRD_IEEE1394
#  define ARPHRD_IEEE1394		24
#endif

/* The BSD's don't define this yet */
#ifndef ARPHRD_INFINIBAND
#  define ARPHRD_INFINIBAND		32
#endif

/* Work out if we have a private address or not
 * 10/8
 * 172.16/12
 * 192.168/16
 */
#ifndef IN_PRIVATE
# define IN_PRIVATE(addr) (((addr & IN_CLASSA_NET) == 0x0a000000) ||	      \
	    ((addr & 0xfff00000)    == 0xac100000) ||			      \
	    ((addr & IN_CLASSB_NET) == 0xc0a80000))
#endif

#define LINKLOCAL_ADDR	0xa9fe0000
#define LINKLOCAL_MASK	IN_CLASSB_NET
#define LINKLOCAL_BRDC	(LINKLOCAL_ADDR | ~LINKLOCAL_MASK)

#ifndef IN_LINKLOCAL
# define IN_LINKLOCAL(addr) ((addr & IN_CLASSB_NET) == LINKLOCAL_ADDR)
#endif

#define RAW_EOF			1 << 0
#define RAW_PARTIALCSUM		2 << 0

int if_setflag(struct interface *ifp, short flag);
#define if_up(ifp) if_setflag((ifp), (IFF_UP | IFF_RUNNING))
struct if_head *if_discover(struct dhcpcd_ctx *, int, char * const *);
struct interface *if_find(struct dhcpcd_ctx *, const char *);
struct interface *if_findindex(struct dhcpcd_ctx *, unsigned int);
void if_free(struct interface *);
int if_domtu(const char *, short int);
#define if_getmtu(iface) if_domtu(iface, 0)
#define if_setmtu(iface, mtu) if_domtu(iface, mtu)
int if_carrier(struct interface *);

/* The below functions are provided by if-KERNEL.c */
int if_conf(struct interface *);
int if_init(struct interface *);
int if_getssid(struct interface *);
int if_vimaster(const char *);
int if_openlinksocket(void);
int if_managelink(struct dhcpcd_ctx *);

#ifdef INET
int if_openrawsocket(struct interface *, int);
ssize_t if_sendrawpacket(const struct interface *,
    int, const void *, size_t);
ssize_t if_readrawpacket(struct interface *, int, void *, size_t, int *);

int if_address(const struct interface *,
    const struct in_addr *, const struct in_addr *,
    const struct in_addr *, int);
#define if_addaddress(iface, addr, net, brd)				      \
	if_address(iface, addr, net, brd, 1)
#define if_setaddress(iface, addr, net, brd)				      \
	if_address(iface, addr, net, brd, 2)
#define if_deladdress(iface, addr, net)				      \
	if_address(iface, addr, net, NULL, -1)

int if_route(const struct rt *rt, int);
#define if_addroute(rt) if_route(rt, 1)
#define if_chgroute(rt) if_route(rt, 0)
#define if_delroute(rt) if_route(rt, -1)
#endif

#ifdef INET6
int if_checkipv6(struct dhcpcd_ctx *ctx, const struct interface *, int);
int if_nd6reachable(const char *ifname, struct in6_addr *addr);

int if_address6(const struct ipv6_addr *, int);
#define if_addaddress6(a) if_address6(a, 1)
#define if_deladdress6(a) if_address6(a, -1)
int if_addrflags6(const struct in6_addr *, const struct interface *);

int if_route6(const struct rt6 *rt, int);
#define if_addroute6(rt) if_route6(rt, 1)
#define if_chgroute6(rt) if_route6(rt, 0)
#define if_delroute6(rt) if_route6(rt, -1)
#else
#define if_checkipv6(a, b, c) (-1)
#endif

int if_machinearch(char *, size_t);
#endif
