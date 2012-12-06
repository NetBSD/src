/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2012 Roy Marples <roy@marples.name>
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

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <errno.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "common.h"
#include "configure.h"
#include "dhcpcd.h"
#include "ipv6.h"
#include "ipv6rs.h"

/* Hackery at it's finest. */
#ifndef s6_addr32
#  define s6_addr32 __u6_addr.__u6_addr32
#endif

int socket_afnet6;
static struct rt6head *routes;

#ifdef DEBUG_MEMORY
static void
ipv6_cleanup()
{
	struct rt6 *rt;

	while ((rt = TAILQ_FIRST(routes))) {
		TAILQ_REMOVE(routes, rt, next);
		free(rt);
	}
	free(routes);
}
#endif

int
ipv6_open(void)
{
	socket_afnet6 = socket(AF_INET6, SOCK_DGRAM, 0);
	if (socket_afnet6 == -1)
		return -1;
	set_cloexec(socket_afnet6);
	routes = xmalloc(sizeof(*routes));
	TAILQ_INIT(routes);
#ifdef DEBUG_MEMORY
	atexit(ipv6_cleanup);
#endif
	return socket_afnet6;
}

ssize_t
ipv6_printaddr(char *s, ssize_t sl, const uint8_t *d, const char *ifname)
{
	char buf[INET6_ADDRSTRLEN];
	const char *p;
	ssize_t l;

	p = inet_ntop(AF_INET6, d, buf, sizeof(buf));
	if (p == NULL)
		return -1;

	l = strlen(p);
	if (d[0] == 0xfe && (d[1] & 0xc0) == 0x80)
		l += 1 + strlen(ifname);

	if (s == NULL)
		return l;

	if (sl < l) {
		errno = ENOMEM;
		return -1;
	}
		
	s += strlcpy(s, p, sl);
	if (d[0] == 0xfe && (d[1] & 0xc0) == 0x80) {
		*s++ = '%';
		s += strlcpy(s, ifname, sl);
	}
	*s = '\0';
	return l;
}

struct in6_addr *
ipv6_linklocal(const char *ifname)
{
	struct ifaddrs *ifaddrs, *ifa;
	struct sockaddr_in6 *sa6;
	struct in6_addr *in6;

	if (getifaddrs(&ifaddrs) == -1)
		return NULL;

	for (ifa = ifaddrs; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL ||
		    ifa->ifa_addr->sa_family != AF_INET6)
			continue;
		if (strcmp(ifa->ifa_name, ifname))
			continue;
		sa6 = (struct sockaddr_in6 *)(void *)ifa->ifa_addr;
		if (IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr))
			break;
	}

	if (ifa) {
		in6 = xmalloc(sizeof(*in6));
		memcpy(in6, &sa6->sin6_addr, sizeof(*in6));
	} else
		in6 = NULL;

	freeifaddrs(ifaddrs);
	return in6;
}

int
ipv6_makeaddr(struct in6_addr *addr, const char *ifname,
    const struct in6_addr *prefix, int prefix_len)
{
	struct in6_addr *lla;

	if (prefix_len > 64) {
		errno = EINVAL;
		return -1;
	}

	lla = ipv6_linklocal(ifname);
	if (lla == NULL) {
		errno = ENOENT;
		return -1;
	}

	memcpy(addr, prefix, sizeof(*prefix));
	addr->s6_addr32[2] = lla->s6_addr32[2];
	addr->s6_addr32[3] = lla->s6_addr32[3];
	free(lla);
	return 0;
}

int
ipv6_mask(struct in6_addr *mask, int len)
{
	static const unsigned char masks[NBBY] =
	    { 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };
	int bytes, bits, i;

	if (len < 0 || len > 128) {
		errno = EINVAL;
		return -1;
	}

	memset(mask, 0, sizeof(*mask));
	bytes = len / NBBY;
	bits = len % NBBY;
	for (i = 0; i < bytes; i++)
		mask->s6_addr[i] = 0xff;
	if (bits)
		mask->s6_addr[bytes] = masks[bits - 1]; 
	return 0;
}

int
ipv6_prefixlen(const struct in6_addr *mask)
{
	int x = 0, y;
	const unsigned char *lim, *p;

	lim = (const unsigned char *)mask + sizeof(*mask);
	for (p = (const unsigned char *)mask; p < lim; x++, p++) {
		if (*p != 0xff)
			break;
	}
	y = 0;
	if (p < lim) {
		for (y = 0; y < NBBY; y++) {
			if ((*p & (0x80 >> y)) == 0)
				break;
		}
	}

	/*
	 * when the limit pointer is given, do a stricter check on the
	 * remaining bits.
	 */
	if (p < lim) {
		if (y != 0 && (*p & (0x00ff >> y)) != 0)
			return -1;
		for (p = p + 1; p < lim; p++)
			if (*p != 0)
				return -1;
	}

	return x * NBBY + y;
}

static struct rt6 *
find_route6(struct rt6head *rts, const struct rt6 *r)
{
	struct rt6 *rt;

	TAILQ_FOREACH(rt, rts, next) {
		if (IN6_ARE_ADDR_EQUAL(&rt->dest, &r->dest) &&
#if HAVE_ROUTE_METRIC
		    rt->iface->metric == r->iface->metric &&
#endif
		    IN6_ARE_ADDR_EQUAL(&rt->net, &r->net))
			return rt;
	}
	return NULL;
}

static void
desc_route(const char *cmd, const struct rt6 *rt)
{
	char destbuf[INET6_ADDRSTRLEN];
	char gatebuf[INET6_ADDRSTRLEN];
	const char *ifname = rt->iface->name, *dest, *gate;

	dest = inet_ntop(AF_INET6, &rt->dest.s6_addr,
	    destbuf, INET6_ADDRSTRLEN);
	gate = inet_ntop(AF_INET6, &rt->gate.s6_addr,
	    gatebuf, INET6_ADDRSTRLEN);
	if (IN6_ARE_ADDR_EQUAL(&rt->gate, &in6addr_any))
		syslog(LOG_INFO, "%s: %s route to %s/%d", ifname, cmd,
		    dest, ipv6_prefixlen(&rt->net));
	else if (IN6_ARE_ADDR_EQUAL(&rt->dest, &in6addr_any) &&
	    IN6_ARE_ADDR_EQUAL(&rt->net, &in6addr_any))
		syslog(LOG_INFO, "%s: %s default route via %s", ifname, cmd,
		    gate);
	else
		syslog(LOG_INFO, "%s: %s route to %s/%d via %s", ifname, cmd,
		    dest, ipv6_prefixlen(&rt->net), gate);
}

static int
n_route(struct rt6 *rt)
{

	/* Don't set default routes if not asked to */
	if (IN6_IS_ADDR_UNSPECIFIED(&rt->dest) &&
	    IN6_IS_ADDR_UNSPECIFIED(&rt->net) &&
	    !(rt->iface->state->options->options & DHCPCD_GATEWAY))
		return -1;

	/* Delete the route first as it could exist prior to dhcpcd running
	 * and we need to ensure it leaves via our preffered interface */
	del_route6(rt);
	desc_route("adding", rt);
	if (!add_route6(rt))
		return 0;

	syslog(LOG_ERR, "%s: add_route: %m", rt->iface->name);
	return -1;
}

static int
c_route(struct rt6 *ort, struct rt6 *nrt)
{

	/* Don't set default routes if not asked to */
	if (IN6_IS_ADDR_UNSPECIFIED(&nrt->dest) &&
	    IN6_IS_ADDR_UNSPECIFIED(&nrt->net) &&
	    !(nrt->iface->state->options->options & DHCPCD_GATEWAY))
		return -1;

	desc_route("changing", nrt);
	/* We delete and add the route so that we can change metric.
	 * This also has the nice side effect of flushing ARP entries so
	 * we don't have to do that manually. */
	del_route6(ort);
	if (!add_route6(nrt))
		return 0;
	syslog(LOG_ERR, "%s: add_route: %m", nrt->iface->name);
	return -1;
}

static int
d_route(struct rt6 *rt)
{
	int retval;

	desc_route("deleting", rt);
	retval = del_route6(rt);
	if (retval != 0 && errno != ENOENT && errno != ESRCH)
		syslog(LOG_ERR,"%s: del_route: %m", rt->iface->name);
	return retval;
}

static struct rt6 *
make_route(struct ra *rap)
{
	struct rt6 *r;

	r = xzalloc(sizeof(*r));
	r->ra = rap;
	r->iface = rap->iface;
	r->metric = rap->iface->metric;
	r->mtu = rap->mtu;
	return r;
}

static struct rt6 *
make_prefix(struct ra *rap, struct ipv6_addr *addr)
{
	struct rt6 *r;

	if (addr == NULL || addr->prefix_len > 128)
		return NULL;

	r = make_route(rap);
	r->dest = addr->prefix;
	ipv6_mask(&r->net, addr->prefix_len);
	r->gate = in6addr_any;
	return r;
}

static struct rt6 *
make_router(struct ra *rap)
{
	struct rt6 *r;

	r = make_route(rap);
	r->dest = in6addr_any;
	r->net = in6addr_any;
	r->gate = rap->from;
	return r;
}

int
ipv6_remove_subnet(struct ra *rap, struct ipv6_addr *addr)
{
	struct rt6 *rt;
#if HAVE_ROUTE_METRIC
	struct rt6 *ort;
#endif
	int r;

	/* We need to delete the subnet route to have our metric or
	 * prefer the interface. */
	r = 0;
	rt = make_prefix(rap, addr);
	if (rt) {
		rt->iface = rap->iface;
#ifdef __linux__
		rt->metric = 256;
#else
		rt->metric = 0;
#endif
#if HAVE_ROUTE_METRIC
		/* For some reason, Linux likes to re-add the subnet
		   route under the original metric.
		   I would love to find a way of stopping this! */
		if ((ort = find_route6(routes, rt)) == NULL ||
		    ort->metric != rt->metric)
#else
		if (!find_route6(routes, rt))
#endif
			r = del_route6(rt);
		free(rt);
	}
	return r;
}

#define RT_IS_DEFAULT(rtp) \
	(IN6_ARE_ADDR_EQUAL(&((rtp)->dest), &in6addr_any) &&		      \
	    IN6_ARE_ADDR_EQUAL(&((rtp)->net), &in6addr_any))

void
ipv6_build_routes(void)
{
	struct rt6head dnr, *nrs;
	struct rt6 *rt, *rtn, *or;
	struct ra *rap;
	struct ipv6_addr *addr;
	int have_default;

	if (!(options & (DHCPCD_IPV6RA_OWN | DHCPCD_IPV6RA_OWN_DEFAULT)))
		return;

	TAILQ_INIT(&dnr);
	TAILQ_FOREACH(rap, &ipv6_routers, next) {
		if (options & DHCPCD_IPV6RA_OWN) {
			TAILQ_FOREACH(addr, &rap->addrs, next) {
				rt = make_prefix(rap, addr);
				if (rt)
					TAILQ_INSERT_TAIL(&dnr, rt, next);
			}
		}
		if (!rap->expired) {
			rt = make_router(rap);
			if (rt)
				TAILQ_INSERT_TAIL(&dnr, rt, next);
		}
	}

	nrs = xmalloc(sizeof(*nrs));
	TAILQ_INIT(nrs);
	have_default = 0;
	TAILQ_FOREACH_SAFE(rt, &dnr, next, rtn) {
		/* Is this route already in our table? */
		if (find_route6(nrs, rt) != NULL)
			continue;
		//rt->src.s_addr = ifp->addr.s_addr;
		/* Do we already manage it? */
		if ((or = find_route6(routes, rt))) {
			if (or->iface != rt->iface ||
		//	    or->src.s_addr != ifp->addr.s_addr ||
			    !IN6_ARE_ADDR_EQUAL(&rt->gate, &or->gate) ||
			    rt->metric != or->metric)
			{
				if (c_route(or, rt) != 0)
					continue;
			}
			TAILQ_REMOVE(routes, or, next);
			free(or);
		} else {
			if (n_route(rt) != 0)
				continue;
		}
		if (RT_IS_DEFAULT(rt))
			have_default = 1;
		TAILQ_REMOVE(&dnr, rt, next);
		TAILQ_INSERT_TAIL(nrs, rt, next);
	}

	/* Free any routes we failed to add/change */
	while ((rt = TAILQ_FIRST(&dnr))) {
		TAILQ_REMOVE(&dnr, rt, next);
		free(rt);
	}

	/* Remove old routes we used to manage
	 * If we own the default route, but not RA management itself
	 * then we need to preserve the last best default route we had */
	while ((rt = TAILQ_LAST(routes, rt6head))) {
		TAILQ_REMOVE(routes, rt, next);
		if (find_route6(nrs, rt) == NULL) {
			if (!have_default &&
			    (options & DHCPCD_IPV6RA_OWN_DEFAULT) &&
			    !(options & DHCPCD_IPV6RA_OWN) &&
			    RT_IS_DEFAULT(rt))
				have_default = 1;
				/* no need to add it back to our routing table
				 * as we delete an exiting route when we add
				 * a new one */
			else
				d_route(rt);
		}
		free(rt);
	}

	free(routes);
	routes = nrs;
}
