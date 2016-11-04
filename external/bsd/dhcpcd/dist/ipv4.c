#include <sys/cdefs.h>
 __RCSID("$NetBSD: ipv4.c,v 1.23.2.2 2016/11/04 14:42:45 pgoyette Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2016 Roy Marples <roy@marples.name>
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

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "arp.h"
#include "common.h"
#include "dhcpcd.h"
#include "dhcp.h"
#include "if.h"
#include "if-options.h"
#include "ipv4.h"
#include "ipv4ll.h"
#include "script.h"

#define IPV4_LOOPBACK_ROUTE
#if defined(__linux__) || defined(__sun) || (defined(BSD) && defined(RTF_LOCAL))
/* Linux has had loopback routes in the local table since 2.2
 * Solaris does not seem to support loopback routes. */
#undef IPV4_LOOPBACK_ROUTE
#endif

uint8_t
inet_ntocidr(struct in_addr address)
{
	uint8_t cidr = 0;
	uint32_t mask = htonl(address.s_addr);

	while (mask) {
		cidr++;
		mask <<= 1;
	}
	return cidr;
}

int
inet_cidrtoaddr(int cidr, struct in_addr *addr)
{
	int ocets;

	if (cidr < 1 || cidr > 32) {
		errno = EINVAL;
		return -1;
	}
	ocets = (cidr + 7) / NBBY;

	addr->s_addr = 0;
	if (ocets > 0) {
		memset(&addr->s_addr, 255, (size_t)ocets - 1);
		memset((unsigned char *)&addr->s_addr + (ocets - 1),
		    (256 - (1 << (32 - cidr) % NBBY)), 1);
	}

	return 0;
}

uint32_t
ipv4_getnetmask(uint32_t addr)
{
	uint32_t dst;

	if (addr == 0)
		return 0;

	dst = htonl(addr);
	if (IN_CLASSA(dst))
		return ntohl(IN_CLASSA_NET);
	if (IN_CLASSB(dst))
		return ntohl(IN_CLASSB_NET);
	if (IN_CLASSC(dst))
		return ntohl(IN_CLASSC_NET);

	return 0;
}

struct ipv4_addr *
ipv4_iffindaddr(struct interface *ifp,
    const struct in_addr *addr, const struct in_addr *mask)
{
	struct ipv4_state *state;
	struct ipv4_addr *ap;

	state = IPV4_STATE(ifp);
	if (state) {
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if ((addr == NULL || ap->addr.s_addr == addr->s_addr) &&
			    (mask == NULL || ap->mask.s_addr == mask->s_addr))
				return ap;
		}
	}
	return NULL;
}

struct ipv4_addr *
ipv4_iffindlladdr(struct interface *ifp)
{
	struct ipv4_state *state;
	struct ipv4_addr *ap;

	state = IPV4_STATE(ifp);
	if (state) {
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (IN_LINKLOCAL(htonl(ap->addr.s_addr)))
				return ap;
		}
	}
	return NULL;
}

static struct ipv4_addr *
ipv4_iffindmaskaddr(struct interface *ifp, const struct in_addr *addr)
{
	struct ipv4_state *state;
	struct ipv4_addr *ap;

	state = IPV4_STATE(ifp);
	if (state) {
		TAILQ_FOREACH (ap, &state->addrs, next) {
			if ((ap->addr.s_addr & ap->mask.s_addr) ==
			    (addr->s_addr & ap->mask.s_addr))
				return ap;
		}
	}
	return NULL;
}

struct ipv4_addr *
ipv4_findaddr(struct dhcpcd_ctx *ctx, const struct in_addr *addr)
{
	struct interface *ifp;
	struct ipv4_addr *ap;

	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		ap = ipv4_iffindaddr(ifp, addr, NULL);
		if (ap)
			return ap;
	}
	return NULL;
}

struct ipv4_addr *
ipv4_findmaskaddr(struct dhcpcd_ctx *ctx, const struct in_addr *addr)
{
	struct interface *ifp;
	struct ipv4_addr *ap;

	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		ap = ipv4_iffindmaskaddr(ifp, addr);
		if (ap)
			return ap;
	}
	return NULL;
}

int
ipv4_hasaddr(const struct interface *ifp)
{
	const struct dhcp_state *dstate;

	if (IPV4LL_STATE_RUNNING(ifp))
		return 1;

	dstate = D_CSTATE(ifp);
	return (dstate &&
	    dstate->added == STATE_ADDED &&
	    dstate->addr != NULL);
}

void
ipv4_freeroutes(struct rt_head *rts)
{

	if (rts) {
		ipv4_freerts(rts);
		free(rts);
	}
}

int
ipv4_init(struct dhcpcd_ctx *ctx)
{

	if (ctx->ipv4_routes == NULL) {
		ctx->ipv4_routes = malloc(sizeof(*ctx->ipv4_routes));
		if (ctx->ipv4_routes == NULL)
			return -1;
		TAILQ_INIT(ctx->ipv4_routes);
	}
	if (ctx->ipv4_kroutes == NULL) {
		ctx->ipv4_kroutes = malloc(sizeof(*ctx->ipv4_kroutes));
		if (ctx->ipv4_kroutes == NULL)
			return -1;
		TAILQ_INIT(ctx->ipv4_kroutes);
	}
	return 0;
}

/* Interface comparer for working out ordering. */
int
ipv4_ifcmp(const struct interface *si, const struct interface *ti)
{
	const struct dhcp_state *sis, *tis;

	sis = D_CSTATE(si);
	tis = D_CSTATE(ti);
	if (sis && !tis)
		return -1;
	if (!sis && tis)
		return 1;
	if (!sis && !tis)
		return 0;
	/* If one has a lease and the other not, it takes precedence. */
	if (sis->new && !tis->new)
		return -1;
	if (!sis->new && tis->new)
		return 1;
	/* Always prefer proper leases */
	if (!(sis->added & STATE_FAKE) && (sis->added & STATE_FAKE))
		return -1;
	if ((sis->added & STATE_FAKE) && !(sis->added & STATE_FAKE))
		return 1;
	/* If we are either, they neither have a lease, or they both have.
	 * We need to check for IPv4LL and make it non-preferred. */
	if (sis->new && tis->new) {
		if (IS_DHCP(sis->new) && !IS_DHCP(tis->new))
			return -1;
		if (!IS_DHCP(sis->new) && IS_DHCP(tis->new))
			return 1;
	}
	return 0;
}

static struct rt *
find_route(struct rt_head *rts, const struct rt *r, const struct rt *srt)
{
	struct rt *rt;

	if (rts == NULL)
		return NULL;
	TAILQ_FOREACH(rt, rts, next) {
		if (rt->dest.s_addr == r->dest.s_addr &&
#ifdef HAVE_ROUTE_METRIC
		    (srt || (r->iface == NULL || rt->iface == NULL ||
		    rt->iface->metric == r->iface->metric)) &&
#endif
                    (!srt || srt != rt) &&
		    rt->mask.s_addr == r->mask.s_addr)
			return rt;
	}
	return NULL;
}

static void
desc_route(const char *cmd, const struct rt *rt)
{
	char addr[INET_ADDRSTRLEN];
	struct dhcpcd_ctx *ctx = rt->iface ? rt->iface->ctx : NULL;
	const char *ifname = rt->iface ? rt->iface->name : NULL;

	strlcpy(addr, inet_ntoa(rt->dest), sizeof(addr));
	if (rt->mask.s_addr == htonl(INADDR_BROADCAST) &&
	    rt->gate.s_addr == htonl(INADDR_ANY))
		logger(ctx, LOG_INFO, "%s: %s host route to %s",
		    ifname, cmd, addr);
	else if (rt->mask.s_addr == htonl(INADDR_BROADCAST))
		logger(ctx, LOG_INFO, "%s: %s host route to %s via %s",
		    ifname, cmd, addr, inet_ntoa(rt->gate));
	else if (rt->dest.s_addr == htonl(INADDR_ANY) &&
	    rt->mask.s_addr == htonl(INADDR_ANY) &&
	    rt->gate.s_addr == htonl(INADDR_ANY))
		logger(ctx, LOG_INFO, "%s: %s default route",
		    ifname, cmd);
	else if (rt->gate.s_addr == htonl(INADDR_ANY))
		logger(ctx, LOG_INFO, "%s: %s route to %s/%d",
		    ifname, cmd, addr, inet_ntocidr(rt->mask));
	else if (rt->dest.s_addr == htonl(INADDR_ANY) &&
	    rt->mask.s_addr == htonl(INADDR_ANY))
		logger(ctx, LOG_INFO, "%s: %s default route via %s",
		    ifname, cmd, inet_ntoa(rt->gate));
	else
		logger(ctx, LOG_INFO, "%s: %s route to %s/%d via %s",
		    ifname, cmd, addr, inet_ntocidr(rt->mask),
		    inet_ntoa(rt->gate));
}

static struct rt *
ipv4_findrt(struct dhcpcd_ctx *ctx, const struct rt *rt, int flags)
{
	struct rt *r;

	if (ctx->ipv4_kroutes == NULL)
		return NULL;
	TAILQ_FOREACH(r, ctx->ipv4_kroutes, next) {
		if (rt->dest.s_addr == r->dest.s_addr &&
#ifdef HAVE_ROUTE_METRIC
		    (rt->iface == NULL || rt->iface == r->iface) &&
		    (!flags || rt->metric == r->metric) &&
#else
		    (!flags || rt->iface == r->iface) &&
#endif
		    rt->mask.s_addr == r->mask.s_addr)
			return r;
	}
	return NULL;
}

void
ipv4_freerts(struct rt_head *routes)
{
	struct rt *rt;

	while ((rt = TAILQ_FIRST(routes))) {
		TAILQ_REMOVE(routes, rt, next);
		free(rt);
	}
}

/* If something other than dhcpcd removes a route,
 * we need to remove it from our internal table. */
int
ipv4_handlert(struct dhcpcd_ctx *ctx, int cmd, const struct rt *rt, int flags)
{
	struct rt *f;

	if (ctx->ipv4_kroutes == NULL)
		return 0;

	f = ipv4_findrt(ctx, rt, 1);
	switch (cmd) {
	case RTM_ADD:
		if (f == NULL) {
			if ((f = malloc(sizeof(*f))) == NULL)
				return -1;
			*f = *rt;
			TAILQ_INSERT_TAIL(ctx->ipv4_kroutes, f, next);
		}
		break;
	case RTM_DELETE:
		if (f) {
			TAILQ_REMOVE(ctx->ipv4_kroutes, f, next);
			free(f);
		}

		/* If we manage the route, remove it */
		if ((f = find_route(ctx->ipv4_routes, rt, NULL))) {
			desc_route("removing", f);
			TAILQ_REMOVE(ctx->ipv4_routes, f, next);
			free(f);
		}
		break;
	}

	return flags ? 0 : ipv4ll_handlert(ctx, cmd, rt);
}

static void
d_kroute(struct rt *rt)
{
	struct dhcpcd_ctx *ctx;

	ctx = rt->iface->ctx;
	rt = ipv4_findrt(ctx, rt, 1);
	if (rt != NULL) {
		TAILQ_REMOVE(ctx->ipv4_kroutes, rt, next);
		free(rt);
	}
}

#define n_route(a)	 nc_route(NULL, a)
#define c_route(a, b)	 nc_route(a, b)
static int
nc_route(struct rt *ort, struct rt *nrt)
{
	int change;

	/* Don't set default routes if not asked to */
	if (nrt->dest.s_addr == 0 &&
	    nrt->mask.s_addr == 0 &&
	    !(nrt->iface->options->options & DHCPCD_GATEWAY))
		return -1;

	desc_route(ort == NULL ? "adding" : "changing", nrt);

	change = 0;
	if (ort == NULL) {
		ort = ipv4_findrt(nrt->iface->ctx, nrt, 0);
		if (ort &&
		    ((ort->flags & RTF_REJECT && nrt->flags & RTF_REJECT) ||
		     (ort->iface == nrt->iface &&
#ifdef HAVE_ROUTE_METRIC
		    ort->metric == nrt->metric &&
#endif
		    ort->gate.s_addr == nrt->gate.s_addr)))
		{
			if (ort->mtu == nrt->mtu)
				return 0;
			change = 1;
		}
	} else if (ort->state & STATE_FAKE && !(nrt->state & STATE_FAKE) &&
	    ort->iface == nrt->iface &&
#ifdef HAVE_ROUTE_METRIC
	    ort->metric == nrt->metric &&
#endif
	    ort->dest.s_addr == nrt->dest.s_addr &&
	    ort->mask.s_addr ==  nrt->mask.s_addr &&
	    ort->gate.s_addr == nrt->gate.s_addr)
	{
		if (ort->mtu == nrt->mtu)
			return 0;
		change = 1;
	}

#ifdef RTF_CLONING
	/* BSD can set routes to be cloning routes.
	 * Cloned routes inherit the parent flags.
	 * As such, we need to delete and re-add the route to flush children
	 * to correct the flags. */
	if (change && ort != NULL && ort->flags & RTF_CLONING)
		change = 0;
#endif

	if (change) {
		if (if_route(RTM_CHANGE, nrt) != -1)
			return 0;
		if (errno != ESRCH)
			logger(nrt->iface->ctx, LOG_ERR, "if_route (CHG): %m");
	}

#ifdef HAVE_ROUTE_METRIC
	/* With route metrics, we can safely add the new route before
	 * deleting the old route. */
	if (if_route(RTM_ADD, nrt) != -1) {
		if (ort && if_route(RTM_DELETE, ort) == -1 && errno != ESRCH)
			logger(nrt->iface->ctx, LOG_ERR, "if_route (DEL): %m");
		return 0;
	}

	/* If the kernel claims the route exists we need to rip out the
	 * old one first. */
	if (errno != EEXIST || ort == NULL)
		goto logerr;
#endif

	/* No route metrics, we need to delete the old route before
	 * adding the new one. */
#ifdef ROUTE_PER_GATEWAY
	errno = 0;
#endif
	if (ort) {
		if (if_route(RTM_DELETE, ort) == -1 && errno != ESRCH)
			logger(nrt->iface->ctx, LOG_ERR, "if_route (DEL): %m");
		else
			d_kroute(ort);
	}
#ifdef ROUTE_PER_GATEWAY
	/* The OS allows many routes to the same dest with different gateways.
	 * dhcpcd does not support this yet, so for the time being just keep on
	 * deleting the route until there is an error. */
	if (ort && errno == 0) {
		for (;;) {
			if (if_route(RTM_DELETE, ort) == -1)
				break;
		}
	}
#endif
	if (if_route(RTM_ADD, nrt) != -1)
		return 0;
#ifdef HAVE_ROUTE_METRIC
logerr:
#endif
	logger(nrt->iface->ctx, LOG_ERR, "if_route (ADD): %m");
	return -1;
}

static int
d_route(struct rt *rt)
{
	int retval;

	desc_route("deleting", rt);
	retval = if_route(RTM_DELETE, rt) == -1 ? -1 : 0;
	if (retval == -1 && errno != ENOENT && errno != ESRCH)
		logger(rt->iface->ctx, LOG_ERR,
		    "%s: if_delroute: %m", rt->iface->name);
	/* Remove the route from our kernel table so we can add a
	 * IPv4LL default route if possible. */
	else
		d_kroute(rt);
	return retval;
}

static struct rt_head *
add_subnet_route(struct rt_head *rt, const struct interface *ifp)
{
	const struct dhcp_state *state;
	struct rt *r;

	if (rt == NULL) /* earlier malloc failed */
		return NULL;

	/* P2P interfaces don't have subnet routes as such. */
	if (ifp->flags & IFF_POINTOPOINT)
		return rt;

	state = D_CSTATE(ifp);
	/* Don't create a subnet route for these addresses */
	if (state->addr->mask.s_addr == INADDR_ANY)
		return rt;
#ifndef BSD
	/* BSD adds a route in this instance */
	if (state->addr->mask.s_addr == INADDR_BROADCAST)
		return rt;
#endif

	if ((r = calloc(1, sizeof(*r))) == NULL) {
		logger(ifp->ctx, LOG_ERR, "%s: %m", __func__);
		ipv4_freeroutes(rt);
		return NULL;
	}
	r->dest.s_addr = state->addr->addr.s_addr & state->addr->mask.s_addr;
	r->mask.s_addr = state->addr->mask.s_addr;
	r->gate.s_addr = INADDR_ANY;
	r->mtu = dhcp_get_mtu(ifp);
	r->src = state->addr->addr;

	TAILQ_INSERT_HEAD(rt, r, next);
	return rt;
}

#ifdef IPV4_LOOPBACK_ROUTE
static struct rt_head *
add_loopback_route(struct rt_head *rt, const struct interface *ifp)
{
	struct rt *r;
	const struct dhcp_state *state;

	if (rt == NULL) /* earlier malloc failed */
		return NULL;

	state = D_CSTATE(ifp);
	if (state->addr == NULL)
		return rt;

	if ((r = calloc(1, sizeof(*r))) == NULL) {
		logger(ifp->ctx, LOG_ERR, "%s: %m", __func__);
		ipv4_freeroutes(rt);
		return NULL;
	}
	r->dest = state->addr->addr;
	r->mask.s_addr = INADDR_BROADCAST;
	r->gate.s_addr = htonl(INADDR_LOOPBACK);
	r->mtu = dhcp_get_mtu(ifp);
	r->src = state->addr->addr;
	TAILQ_INSERT_HEAD(rt, r, next);
	return rt;
}
#endif

static struct rt_head *
get_routes(struct interface *ifp)
{
	struct rt_head *nrt;
	struct rt *rt, *r = NULL;
	const struct dhcp_state *state;

	if (ifp->options->routes && TAILQ_FIRST(ifp->options->routes)) {
		if ((nrt = malloc(sizeof(*nrt))) == NULL)
			return NULL;
		TAILQ_INIT(nrt);
		TAILQ_FOREACH(rt, ifp->options->routes, next) {
			if (rt->gate.s_addr == 0)
				break;
			if ((r = calloc(1, sizeof(*r))) == NULL) {
				logger(ifp->ctx, LOG_ERR, "%s: %m", __func__);
				ipv4_freeroutes(nrt);
				return NULL;
			}
			memcpy(r, rt, sizeof(*r));
			TAILQ_INSERT_TAIL(nrt, r, next);
		}
	} else
		nrt = dhcp_get_routes(ifp);

	/* Copy our address as the source address */
	if (nrt) {
		state = D_CSTATE(ifp);
		TAILQ_FOREACH(rt, nrt, next) {
			rt->src = state->addr->addr;
		}
	}

	return nrt;
}

static struct rt_head *
add_destination_route(struct rt_head *rt, const struct interface *ifp)
{
	struct rt *r;
	const struct dhcp_state *state;

	if (rt == NULL || /* failed a malloc earlier probably */
	    !(ifp->flags & IFF_POINTOPOINT) ||
	    !has_option_mask(ifp->options->dstmask, DHO_ROUTER) ||
	    (state = D_CSTATE(ifp)) == NULL)
		return rt;

	if ((r = calloc(1, sizeof(*r))) == NULL) {
		logger(ifp->ctx, LOG_ERR, "%s: %m", __func__);
		ipv4_freeroutes(rt);
		return NULL;
	}
	r->dest.s_addr = INADDR_ANY;
	r->mask.s_addr = INADDR_ANY;
	r->gate = state->addr->brd;
	r->mtu = dhcp_get_mtu(ifp);
	r->src = state->addr->addr;
	TAILQ_INSERT_HEAD(rt, r, next);
	return rt;
}

/* We should check to ensure the routers are on the same subnet
 * OR supply a host route. If not, warn and add a host route. */
static struct rt_head *
add_router_host_route(struct rt_head *rt, const struct interface *ifp)
{
	struct rt *rtp, *rtn;
	const char *cp, *cp2, *cp3, *cplim;
	struct if_options *ifo;
	const struct dhcp_state *state;

	if (rt == NULL) /* earlier malloc failed */
		return NULL;

	/* Don't add a host route for these interfaces. */
	if (ifp->flags & (IFF_LOOPBACK | IFF_POINTOPOINT))
		return rt;

	TAILQ_FOREACH(rtp, rt, next) {
		if (rtp->dest.s_addr != INADDR_ANY ||
		    rtp->gate.s_addr == INADDR_ANY)
			continue;
		/* Scan for a route to match */
		TAILQ_FOREACH(rtn, rt, next) {
			if (rtn == rtp)
				break;
			/* match host */
			if (rtn->dest.s_addr == rtp->gate.s_addr)
				break;
			/* match subnet */
			cp = (const char *)&rtp->gate.s_addr;
			cp2 = (const char *)&rtn->dest.s_addr;
			cp3 = (const char *)&rtn->mask.s_addr;
			cplim = cp3 + sizeof(rtn->mask.s_addr);
			while (cp3 < cplim) {
				if ((*cp++ ^ *cp2++) & *cp3++)
					break;
			}
			if (cp3 == cplim)
				break;
		}
		if (rtn != rtp)
			continue;
		if ((state = D_CSTATE(ifp)) == NULL)
			continue;
		ifo = ifp->options;
		if (ifp->flags & IFF_NOARP) {
			if (!(ifo->options & DHCPCD_ROUTER_HOST_ROUTE_WARNED) &&
			    !(state->added & STATE_FAKE))
			{
				ifo->options |= DHCPCD_ROUTER_HOST_ROUTE_WARNED;
				logger(ifp->ctx, LOG_WARNING,
				    "%s: forcing router %s through interface",
				    ifp->name, inet_ntoa(rtp->gate));
			}
			rtp->gate.s_addr = 0;
			continue;
		}
		if (!(ifo->options & DHCPCD_ROUTER_HOST_ROUTE_WARNED) &&
		    !(state->added & STATE_FAKE))
		{
			ifo->options |= DHCPCD_ROUTER_HOST_ROUTE_WARNED;
			logger(ifp->ctx, LOG_WARNING,
			    "%s: router %s requires a host route",
			    ifp->name, inet_ntoa(rtp->gate));
		}
		if ((rtn = calloc(1, sizeof(*rtn))) == NULL) {
			logger(ifp->ctx, LOG_ERR, "%s: %m", __func__);
			ipv4_freeroutes(rt);
			return NULL;
		}
		rtn->dest.s_addr = rtp->gate.s_addr;
		rtn->mask.s_addr = htonl(INADDR_BROADCAST);
		rtn->gate.s_addr = htonl(INADDR_ANY);
		rtn->mtu = dhcp_get_mtu(ifp);
		rtn->src = state->addr->addr;
		TAILQ_INSERT_BEFORE(rtp, rtn, next);
	}
	return rt;
}

static int
ipv4_doroute(struct rt *rt, struct rt_head *nrs)
{
	const struct dhcp_state *state;
	struct rt *or;

	state = D_CSTATE(rt->iface);
	rt->state = state->added & STATE_FAKE;
#ifdef HAVE_ROUTE_METRIC
	rt->metric = rt->iface->metric;
#endif
	/* Is this route already in our table? */
	if ((find_route(nrs, rt, NULL)) != NULL)
		return 0;
	/* Do we already manage it? */
	if ((or = find_route(rt->iface->ctx->ipv4_routes, rt, NULL))) {
		if (state->added & STATE_FAKE)
			return 0;
		if (or->state & STATE_FAKE ||
		    or->iface != rt->iface ||
#ifdef HAVE_ROUTE_METRIC
		    or->metric != rt->metric ||
#endif
		    or->gate.s_addr != rt->gate.s_addr ||
		    or->src.s_addr != rt->src.s_addr ||
		    or->mtu != rt->mtu)
		{
			if (c_route(or, rt) != 0)
				return 0;
		}
		TAILQ_REMOVE(rt->iface->ctx->ipv4_routes, or, next);
		free(or);
	} else {
		if (state->added & STATE_FAKE) {
			if ((or = ipv4_findrt(rt->iface->ctx, rt, 1)) == NULL)
				return 0;
			rt->iface = or->iface;
			rt->gate.s_addr = or->gate.s_addr;
#ifdef HAVE_ROUTE_METRIC
			rt->metric = or->metric;
#endif
			rt->mtu = or->mtu;
			rt->flags = or->flags;
		} else {
			if (n_route(rt) != 0)
				return 0;
		}
	}
	return 1;
}

void
ipv4_buildroutes(struct dhcpcd_ctx *ctx)
{
	struct rt_head *nrs, *dnr;
	struct rt *rt, *rtn;
	struct interface *ifp;
	const struct dhcp_state *state;
	int has_default;

	/* We need to have the interfaces in the correct order to ensure
	 * our routes are managed correctly. */
	if_sortinterfaces(ctx);

	if ((nrs = malloc(sizeof(*nrs))) == NULL) {
		logger(ctx, LOG_ERR, "%s: %m", __func__);
		return;
	}
	TAILQ_INIT(nrs);

	has_default = 0;
	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		state = D_CSTATE(ifp);
		if (state != NULL && state->new != NULL && state->added) {
			dnr = get_routes(ifp);
			dnr = add_subnet_route(dnr, ifp);
		} else
			dnr = NULL;
		if ((rt = ipv4ll_subnet_route(ifp)) != NULL) {
			if (dnr == NULL) {
				if ((dnr = malloc(sizeof(*dnr))) == NULL) {
					logger(ifp->ctx, LOG_ERR,
					    "%s: malloc %m", __func__);
					free(rt);
					continue;
				}
				TAILQ_INIT(dnr);
			}
			TAILQ_INSERT_HEAD(dnr, rt, next);
		}
		if (dnr == NULL)
			continue;
#ifdef IPV4_LOOPBACK_ROUTE
		dnr = add_loopback_route(dnr, ifp);
#endif
		if (ifp->options->options & DHCPCD_GATEWAY) {
			dnr = add_router_host_route(dnr, ifp);
			dnr = add_destination_route(dnr, ifp);
		}
		if (dnr == NULL)
			continue;
		TAILQ_FOREACH_SAFE(rt, dnr, next, rtn) {
			rt->iface = ifp;
			if (ipv4_doroute(rt, nrs) == 1) {
				TAILQ_REMOVE(dnr, rt, next);
				TAILQ_INSERT_TAIL(nrs, rt, next);
				if (rt->dest.s_addr == INADDR_ANY)
					has_default = 1;
			}
		}
		ipv4_freeroutes(dnr);
	}

	/* If there is no default route, grab one without a
	 * gateway for any IPv4LL enabled interfaces. */
	if (!has_default) {
		struct rt def;

		memset(&def, 0, sizeof(def));
		if (ipv4_findrt(ctx, &def, 0) == NULL) {
			TAILQ_FOREACH(ifp, ctx->ifaces, next) {
				if ((rt = ipv4ll_default_route(ifp)) != NULL) {
					if (ipv4_doroute(rt, nrs) == 1) {
						TAILQ_INSERT_TAIL(nrs, rt, next);
						break;
					} else
						free(rt);
				}
			}
		}
	}

	/* Remove old routes we used to manage */
	if (ctx->ipv4_routes) {
		TAILQ_FOREACH_REVERSE(rt, ctx->ipv4_routes, rt_head, next) {
			if (find_route(nrs, rt, NULL) == NULL &&
			    (rt->iface->options->options &
			    (DHCPCD_EXITING | DHCPCD_PERSISTENT)) !=
			    (DHCPCD_EXITING | DHCPCD_PERSISTENT))
				d_route(rt);
		}
	}
	ipv4_freeroutes(ctx->ipv4_routes);
	ctx->ipv4_routes = nrs;
}

int
ipv4_deladdr(struct ipv4_addr *addr, int keeparp)
{
	int r;
	struct ipv4_state *state;
	struct ipv4_addr *ap;
	struct arp_state *astate;

	logger(addr->iface->ctx, LOG_DEBUG,
	    "%s: deleting IP address %s", addr->iface->name, addr->saddr);

	r = if_address(RTM_DELADDR, addr);
	if (r == -1 &&
	    errno != EADDRNOTAVAIL && errno != ESRCH &&
	    errno != ENXIO && errno != ENODEV)
		logger(addr->iface->ctx, LOG_ERR, "%s: %s: %m",
		    addr->iface->name, __func__);

	if (!keeparp && (astate = arp_find(addr->iface, &addr->addr)) != NULL)
		arp_free(astate);

	state = IPV4_STATE(addr->iface);
	TAILQ_FOREACH(ap, &state->addrs, next) {
		if (IPV4_MASK_EQ(ap, addr)) {
			struct dhcp_state *dstate;

			dstate = D_STATE(ap->iface);
			TAILQ_REMOVE(&state->addrs, ap, next);
			free(ap);

			if (dstate && dstate->addr == ap) {
				dstate->added = 0;
				dstate->addr = NULL;
			}
			break;
		}
	}

	return r;
}

static int
delete_address(struct interface *ifp)
{
	int r;
	struct if_options *ifo;
	struct dhcp_state *state;

	state = D_STATE(ifp);
	ifo = ifp->options;
	/* The lease could have been added, but the address deleted
	 * by a 3rd party. */
	if (state->addr == NULL ||
	    ifo->options & DHCPCD_INFORM ||
	    (ifo->options & DHCPCD_STATIC && ifo->req_addr.s_addr == 0))
		return 0;
	r = ipv4_deladdr(state->addr, 0);
	return r;
}

struct ipv4_state *
ipv4_getstate(struct interface *ifp)
{
	struct ipv4_state *state;

	state = IPV4_STATE(ifp);
	if (state == NULL) {
	        ifp->if_data[IF_DATA_IPV4] = malloc(sizeof(*state));
		state = IPV4_STATE(ifp);
		if (state == NULL) {
			logger(ifp->ctx, LOG_ERR, "%s: %m", __func__);
			return NULL;
		}
		TAILQ_INIT(&state->addrs);
		TAILQ_INIT(&state->routes);
#ifdef BSD
		state->buffer_size = state->buffer_len = state->buffer_pos = 0;
		state->buffer = NULL;
#endif
	}
	return state;
}

#ifdef ALIAS_ADDR
/* Find the next logical aliase address we can use. */
static int
ipv4_aliasaddr(struct ipv4_addr *ia, struct ipv4_addr **repl)
{
	struct ipv4_state *state;
	struct ipv4_addr *iap;
	unsigned int lun;
	char alias[IF_NAMESIZE];

	if (ia->alias[0] != '\0')
		return 0;

	lun = 0;
	state = IPV4_STATE(ia->iface);
find_lun:
	if (lun == 0)
		strlcpy(alias, ia->iface->name, sizeof(alias));
	else
		snprintf(alias, sizeof(alias), "%s:%u", ia->iface->name, lun);
	TAILQ_FOREACH(iap, &state->addrs, next) {
		if (iap->alias[0] != '\0' && iap->addr.s_addr == INADDR_ANY) {
			/* No address assigned? Lets use it. */
			strlcpy(ia->alias, iap->alias, sizeof(ia->alias));
			if (repl)
				*repl = iap;
			return 1;
		}
		if (strcmp(iap->alias, alias) == 0)
			break;
	}

	if (iap != NULL) {
		if (lun == UINT_MAX) {
			errno = ERANGE;
			return -1;
		}
		lun++;
		goto find_lun;
	}

	strlcpy(ia->alias, alias, sizeof(ia->alias));
	return 0;
}
#endif

struct ipv4_addr *
ipv4_addaddr(struct interface *ifp, const struct in_addr *addr,
    const struct in_addr *mask, const struct in_addr *bcast)
{
	struct ipv4_state *state;
	struct ipv4_addr *ia;
#ifdef ALIAS_ADDR
	int replaced, blank;
	struct ipv4_addr *replaced_ia;
#endif

	if ((state = ipv4_getstate(ifp)) == NULL) {
		logger(ifp->ctx, LOG_ERR, "%s: ipv4_getstate: %m", __func__);
		return NULL;
	}
	if (ifp->options->options & DHCPCD_NOALIAS) {
		struct ipv4_addr *ian;

		TAILQ_FOREACH_SAFE(ia, &state->addrs, next, ian) {
			if (ia->addr.s_addr != addr->s_addr)
				ipv4_deladdr(ia, 0);
		}
	}

	if ((ia = malloc(sizeof(*ia))) == NULL) {
		logger(ifp->ctx, LOG_ERR, "%s: %m", __func__);
		return NULL;
	}

	ia->iface = ifp;
	ia->addr = *addr;
	ia->mask = *mask;
	ia->brd = *bcast;
#ifdef IN_IFF_TENTATIVE
	ia->addr_flags = IN_IFF_TENTATIVE;
#endif
	snprintf(ia->saddr, sizeof(ia->saddr), "%s/%d",
	    inet_ntoa(*addr), inet_ntocidr(*mask));

#ifdef ALIAS_ADDR
	blank = (ia->alias[0] == '\0');
	if ((replaced = ipv4_aliasaddr(ia, &replaced_ia)) == -1) {
		logger(ifp->ctx, LOG_ERR, "%s: ipv4_aliasaddr: %m", ifp->name);
		free(ia);
		return NULL;
	}
	if (blank)
		logger(ia->iface->ctx, LOG_DEBUG, "%s: aliased %s",
		    ia->alias, ia->saddr);
#endif

	logger(ifp->ctx, LOG_DEBUG, "%s: adding IP address %s broadcast %s",
	    ifp->name, ia->saddr, inet_ntoa(*bcast));
	if (if_address(RTM_NEWADDR, ia) == -1) {
		if (errno != EEXIST)
			logger(ifp->ctx, LOG_ERR, "%s: if_addaddress: %m",
			    __func__);
		free(ia);
		return NULL;
	}

#ifdef ALIAS_ADDR
	if (replaced) {
		TAILQ_REMOVE(&state->addrs, replaced_ia, next);
		free(replaced_ia);
	}
#endif

	TAILQ_INSERT_TAIL(&state->addrs, ia, next);
	return ia;
}

static int
ipv4_daddaddr(struct interface *ifp, const struct dhcp_lease *lease)
{
	struct dhcp_state *state;
	struct ipv4_addr *ia;

	ia = ipv4_addaddr(ifp, &lease->addr, &lease->mask, &lease->brd);
	if (ia == NULL)
		return -1;

	state = D_STATE(ifp);
	state->added = STATE_ADDED;
	state->addr = ia;
	return 0;
}

int
ipv4_preferanother(struct interface *ifp)
{
	struct dhcp_state *state = D_STATE(ifp), *nstate;
	struct interface *ifn;
	int preferred;

	if (state == NULL)
		return 0;

	preferred = 0;
	if (!state->added)
		goto out;

	TAILQ_FOREACH(ifn, ifp->ctx->ifaces, next) {
		if (ifn == ifp)
			break; /* We are already the most preferred */
		nstate = D_STATE(ifn);
		if (nstate && !nstate->added &&
		    state->addr != NULL &&
		    nstate->lease.addr.s_addr == state->addr->addr.s_addr)
		{
			preferred = 1;
			delete_address(ifp);
			if (ifn->options->options & DHCPCD_ARP)
				dhcp_bind(ifn);
			else {
				ipv4_daddaddr(ifn, &nstate->lease);
				nstate->added = STATE_ADDED;
			}
			break;
		}
	}

out:
	ipv4_buildroutes(ifp->ctx);
	return preferred;
}

void
ipv4_applyaddr(void *arg)
{
	struct interface *ifp = arg, *ifn;
	struct dhcp_state *state = D_STATE(ifp), *nstate;
	struct dhcp_lease *lease;
	struct if_options *ifo = ifp->options;
	struct ipv4_addr *ia;
	int r;

	if (state == NULL)
		return;

	lease = &state->lease;
	if_sortinterfaces(ifp->ctx);

	if (state->new == NULL) {
		if ((ifo->options & (DHCPCD_EXITING | DHCPCD_PERSISTENT)) !=
		    (DHCPCD_EXITING | DHCPCD_PERSISTENT))
		{
			if (state->added && !ipv4_preferanother(ifp)) {
				delete_address(ifp);
				ipv4_buildroutes(ifp->ctx);
			}
			script_runreason(ifp, state->reason);
		} else
			ipv4_buildroutes(ifp->ctx);
		return;
	}

	/* Ensure only one interface has the address */
	r = 0;
	TAILQ_FOREACH(ifn, ifp->ctx->ifaces, next) {
		if (ifn == ifp) {
			r = 1; /* past ourselves */
			continue;
		}
		nstate = D_STATE(ifn);
		if (nstate && nstate->added &&
		    nstate->addr &&
		    nstate->addr->addr.s_addr == lease->addr.s_addr)
		{
			if (r == 0) {
				logger(ifp->ctx, LOG_INFO,
				    "%s: preferring %s on %s",
				    ifp->name,
				    inet_ntoa(lease->addr),
				    ifn->name);
				return;
			}
			logger(ifp->ctx, LOG_INFO, "%s: preferring %s on %s",
			    ifn->name,
			    inet_ntoa(lease->addr),
			    ifp->name);
			ipv4_deladdr(nstate->addr, 0);
			break;
		}
	}

	/* Does another interface already have the address from a prior boot? */
	if (ifn == NULL) {
		TAILQ_FOREACH(ifn, ifp->ctx->ifaces, next) {
			if (ifn == ifp)
				continue;
			ia = ipv4_iffindaddr(ifn, &lease->addr, NULL);
			if (ia != NULL)
				ipv4_deladdr(ia, 0);
		}
	}

	/* If the netmask or broadcast is different, re-add the addresss */
	ia = ipv4_iffindaddr(ifp, &lease->addr, NULL);
	if (ia &&
	    ia->mask.s_addr == lease->mask.s_addr &&
	    ia->brd.s_addr == lease->brd.s_addr)
		logger(ifp->ctx, LOG_DEBUG,
		    "%s: IP address %s already exists",
		    ifp->name, ia->saddr);
	else {
#if __linux__
		/* Linux does not change netmask/broadcast address
		 * for re-added addresses, so we need to delete the old one
		 * first. */
		if (ia != NULL)
			ipv4_deladdr(ia, 0);
#endif
		r = ipv4_daddaddr(ifp, lease);
		if (r == -1 && errno != EEXIST)
			return;
	}

	ia = ipv4_iffindaddr(ifp, &lease->addr, NULL);
	if (ia == NULL) {
		logger(ifp->ctx, LOG_ERR, "%s: added address vanished",
		    ifp->name);
		return;
	}
#ifdef IN_IFF_NOTUSEABLE
	if (ia->addr_flags & IN_IFF_NOTUSEABLE)
		return;
#endif

	/* Delete the old address if different */
	if (state->addr &&
	    state->addr->addr.s_addr != lease->addr.s_addr &&
	    ipv4_iffindaddr(ifp, &lease->addr, NULL))
		delete_address(ifp);

	state->addr = ia;
	state->added = STATE_ADDED;

	/* Find any freshly added routes, such as the subnet route.
	 * We do this because we cannot rely on recieving the kernel
	 * notification right now via our link socket. */
	if_initrt(ifp->ctx);
	ipv4_buildroutes(ifp->ctx);
	/* Announce the address */
	if (ifo->options & DHCPCD_ARP) {
		struct arp_state *astate;

		if ((astate = arp_new(ifp, &state->addr->addr)) != NULL)
			arp_announce(astate);
	}
	if (state->state == DHS_BOUND) {
		script_runreason(ifp, state->reason);
		dhcpcd_daemonise(ifp->ctx);
	}
}

void
ipv4_handleifa(struct dhcpcd_ctx *ctx,
    int cmd, struct if_head *ifs, const char *ifname,
    const struct in_addr *addr, const struct in_addr *mask,
    const struct in_addr *brd, const int addrflags)
{
	struct interface *ifp;
	struct ipv4_state *state;
	struct ipv4_addr *ia;
	bool ia_is_new;

	if (ifs == NULL)
		ifs = ctx->ifaces;
	if (ifs == NULL) {
		errno = ESRCH;
		return;
	}
	if ((ifp = if_find(ifs, ifname)) == NULL)
		return;
	if ((state = ipv4_getstate(ifp)) == NULL) {
		errno = ENOENT;
		return;
	}

	ia = ipv4_iffindaddr(ifp, addr, NULL);
	switch (cmd) {
	case RTM_NEWADDR:
		if (ia == NULL) {
			if ((ia = malloc(sizeof(*ia))) == NULL) {
				logger(ifp->ctx, LOG_ERR, "%s: %m", __func__);
				return;
			}
			ia->iface = ifp;
			ia->addr = *addr;
			ia->mask = *mask;
			ia_is_new = true;
#ifdef ALIAS_ADDR
			strlcpy(ia->alias, ifname, sizeof(ia->alias));
#endif
			TAILQ_INSERT_TAIL(&state->addrs, ia, next);
		} else
			ia_is_new = false;
		/* Mask could have changed */
		if (ia_is_new ||
		    (mask->s_addr != INADDR_ANY &&
		    mask->s_addr != ia->mask.s_addr))
		{
			ia->mask = *mask;
			snprintf(ia->saddr, sizeof(ia->saddr), "%s/%d",
			    inet_ntoa(*addr), inet_ntocidr(*mask));
		}
		if (brd != NULL)
			ia->brd = *brd;
		else
			ia->brd.s_addr = INADDR_ANY;
		ia->addr_flags = addrflags;
		break;
	case RTM_DELADDR:
		if (ia == NULL)
			return;
		TAILQ_REMOVE(&state->addrs, ia, next);
		break;
	default:
		return;
	}

	if (addr->s_addr != INADDR_ANY && addr->s_addr != INADDR_BROADCAST) {
		arp_handleifa(cmd, ia);
		dhcp_handleifa(cmd, ia);
	}

	if (cmd == RTM_DELADDR)
		free(ia);
}

void
ipv4_free(struct interface *ifp)
{
	struct ipv4_state *state;
	struct ipv4_addr *ia;

	if (ifp) {
		state = IPV4_STATE(ifp);
		if (state) {
		        while ((ia = TAILQ_FIRST(&state->addrs))) {
				TAILQ_REMOVE(&state->addrs, ia, next);
				free(ia);
			}
			ipv4_freerts(&state->routes);
#ifdef BSD
			free(state->buffer);
#endif
			free(state);
		}
	}
}

void
ipv4_ctxfree(struct dhcpcd_ctx *ctx)
{

	ipv4_freeroutes(ctx->ipv4_routes);
	ipv4_freeroutes(ctx->ipv4_kroutes);
}
