#include <sys/cdefs.h>
 __RCSID("$NetBSD: ipv4.c,v 1.14 2015/05/02 15:18:37 roy Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2015 Roy Marples <roy@marples.name>
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

#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/route.h>

#include <ctype.h>
#include <errno.h>
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
#include "script.h"

#define IPV4_LOOPBACK_ROUTE
#if defined(__linux__) || (defined(BSD) && defined(RTF_LOCAL))
/* Linux has had loopback routes in the local table since 2.2 */
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
    const struct in_addr *addr, const struct in_addr *net)
{
	struct ipv4_state *state;
	struct ipv4_addr *ap;

	state = IPV4_STATE(ifp);
	if (state) {
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if ((addr == NULL || ap->addr.s_addr == addr->s_addr) &&
			    (net == NULL || ap->net.s_addr == net->s_addr))
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

int
ipv4_addrexists(struct dhcpcd_ctx *ctx, const struct in_addr *addr)
{
	struct interface *ifp;
	struct dhcp_state *state;

	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		state = D_STATE(ifp);
		if (state) {
			if (addr == NULL) {
				if (state->addr.s_addr != INADDR_ANY)
					return 1;
			} else if (addr->s_addr == state->addr.s_addr)
				return 1;
		}
		if (addr != NULL && ipv4_iffindaddr(ifp, addr, NULL))
			return 1;
	}
	return 0;
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
		int sill = (sis->new->cookie == htonl(MAGIC_COOKIE));
		int till = (tis->new->cookie == htonl(MAGIC_COOKIE));
		if (sill && !till)
			return -1;
		if (!sill && till)
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
		    rt->net.s_addr == r->net.s_addr)
			return rt;
	}
	return NULL;
}

static void
desc_route(const char *cmd, const struct rt *rt)
{
	char addr[sizeof("000.000.000.000") + 1];
	struct dhcpcd_ctx *ctx = rt->iface ? rt->iface->ctx : NULL;
	const char *ifname = rt->iface ? rt->iface->name : NULL;

	strlcpy(addr, inet_ntoa(rt->dest), sizeof(addr));
	if (rt->net.s_addr == htonl(INADDR_BROADCAST) &&
	    rt->gate.s_addr == htonl(INADDR_ANY))
		logger(ctx, LOG_INFO, "%s: %s host route to %s",
		    ifname, cmd, addr);
	else if (rt->net.s_addr == htonl(INADDR_BROADCAST))
		logger(ctx, LOG_INFO, "%s: %s host route to %s via %s",
		    ifname, cmd, addr, inet_ntoa(rt->gate));
	else if (rt->gate.s_addr == htonl(INADDR_ANY))
		logger(ctx, LOG_INFO, "%s: %s route to %s/%d",
		    ifname, cmd, addr, inet_ntocidr(rt->net));
	else if (rt->dest.s_addr == htonl(INADDR_ANY) &&
	    rt->net.s_addr == htonl(INADDR_ANY))
		logger(ctx, LOG_INFO, "%s: %s default route via %s",
		    ifname, cmd, inet_ntoa(rt->gate));
	else
		logger(ctx, LOG_INFO, "%s: %s route to %s/%d via %s",
		    ifname, cmd, addr, inet_ntocidr(rt->net),
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
		    rt->iface == r->iface &&
		    (!flags || rt->metric == r->metric) &&
#else
		    (!flags || rt->iface == r->iface) &&
#endif
		    rt->net.s_addr == r->net.s_addr)
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
ipv4_handlert(struct dhcpcd_ctx *ctx, int cmd, struct rt *rt)
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
	return 0;
}

#define n_route(a)	 nc_route(NULL, a)
#define c_route(a, b)	 nc_route(a, b)
static int
nc_route(struct rt *ort, struct rt *nrt)
{

	/* Don't set default routes if not asked to */
	if (nrt->dest.s_addr == 0 &&
	    nrt->net.s_addr == 0 &&
	    !(nrt->iface->options->options & DHCPCD_GATEWAY))
		return -1;

	desc_route(ort == NULL ? "adding" : "changing", nrt);

	if (ort == NULL) {
		ort = ipv4_findrt(nrt->iface->ctx, nrt, 0);
		if (ort &&
		    ((ort->flags & RTF_REJECT && nrt->flags & RTF_REJECT) ||
		     (ort->iface == nrt->iface &&
#ifdef HAVE_ROUTE_METRIC
		    ort->metric == nrt->metric &&
#endif
		    ort->gate.s_addr == nrt->gate.s_addr)))
			return 0;
	} else if (ort->flags & STATE_FAKE && !(nrt->flags & STATE_FAKE) &&
	    ort->iface == nrt->iface &&
#ifdef HAVE_ROUTE_METRIC
	    ort->metric == nrt->metric &&
#endif
	    ort->dest.s_addr == nrt->dest.s_addr &&
	    ort->net.s_addr ==  nrt->net.s_addr &&
	    ort->gate.s_addr == nrt->gate.s_addr)
		return 0;

#ifdef HAVE_ROUTE_METRIC
	/* With route metrics, we can safely add the new route before
	 * deleting the old route. */
	if (if_route(RTM_ADD, nrt)  == 0) {
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
	if (ort && if_route(RTM_DELETE, ort) == -1 && errno != ESRCH)
		logger(nrt->iface->ctx, LOG_ERR, "if_route (DEL): %m");
	if (if_route(RTM_ADD, nrt) == 0)
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
	retval = if_route(RTM_DELETE, rt);
	if (retval != 0 && errno != ENOENT && errno != ESRCH)
		logger(rt->iface->ctx, LOG_ERR,
		    "%s: if_delroute: %m", rt->iface->name);
	return retval;
}

static struct rt_head *
add_subnet_route(struct rt_head *rt, const struct interface *ifp)
{
	const struct dhcp_state *s;
	struct rt *r;

	if (rt == NULL) /* earlier malloc failed */
		return NULL;

	s = D_CSTATE(ifp);
	/* Don't create a subnet route for these addresses */
	if (s->net.s_addr == INADDR_ANY)
		return rt;
#ifndef BSD
	/* BSD adds a route in this instance */
	if (s->net.s_addr == INADDR_BROADCAST)
		return rt;
#endif

	if ((r = malloc(sizeof(*r))) == NULL) {
		logger(ifp->ctx, LOG_ERR, "%s: %m", __func__);
		ipv4_freeroutes(rt);
		return NULL;
	}
	r->dest.s_addr = s->addr.s_addr & s->net.s_addr;
	r->net.s_addr = s->net.s_addr;
	r->gate.s_addr = INADDR_ANY;

	TAILQ_INSERT_HEAD(rt, r, next);
	return rt;
}

#ifdef IPV4_LOOPBACK_ROUTE
static struct rt_head *
add_loopback_route(struct rt_head *rt, const struct interface *ifp)
{
	struct rt *r;
	const struct dhcp_state *s;

	if (rt == NULL) /* earlier malloc failed */
		return NULL;

	s = D_CSTATE(ifp);
	if (s->addr.s_addr == INADDR_ANY)
		return rt;

	r = malloc(sizeof(*r));
	if (r == NULL) {
		logger(ifp->ctx, LOG_ERR, "%s: %m", __func__);
		ipv4_freeroutes(rt);
		return NULL;
	}
	r->dest.s_addr = s->addr.s_addr;
	r->net.s_addr = INADDR_BROADCAST;
	r->gate.s_addr = htonl(INADDR_LOOPBACK);
	TAILQ_INSERT_HEAD(rt, r, next);
	return rt;
}
#endif

static struct rt_head *
get_routes(struct interface *ifp)
{
	struct rt_head *nrt;
	struct rt *rt, *r = NULL;

	if (ifp->options->routes && TAILQ_FIRST(ifp->options->routes)) {
		nrt = malloc(sizeof(*nrt));
		TAILQ_INIT(nrt);
		TAILQ_FOREACH(rt, ifp->options->routes, next) {
			if (rt->gate.s_addr == 0)
				break;
			r = malloc(sizeof(*r));
			if (r == NULL) {
				logger(ifp->ctx, LOG_ERR, "%s: %m", __func__);
				ipv4_freeroutes(nrt);
				return NULL;
			}
			memcpy(r, rt, sizeof(*r));
			TAILQ_INSERT_TAIL(nrt, r, next);
		}
		return nrt;
	}

	return get_option_routes(ifp, D_STATE(ifp)->new);
}

/* Some DHCP servers add set host routes by setting the gateway
 * to the assigned IP address or the destination address.
 * We need to change this. */
static struct rt_head *
massage_host_routes(struct rt_head *rt, const struct interface *ifp)
{
	struct rt *r;

	if (rt) {
		TAILQ_FOREACH(r, rt, next) {
			if (r->gate.s_addr == D_CSTATE(ifp)->addr.s_addr ||
			    r->gate.s_addr == r->dest.s_addr)
			{
				r->gate.s_addr = htonl(INADDR_ANY);
				r->net.s_addr = htonl(INADDR_BROADCAST);
			}
		}
	}
	return rt;
}


static struct rt_head *
add_destination_route(struct rt_head *rt, const struct interface *ifp)
{
	struct rt *r;

	if (rt == NULL || /* failed a malloc earlier probably */
	    !(ifp->flags & IFF_POINTOPOINT) ||
	    !has_option_mask(ifp->options->dstmask, DHO_ROUTER))
		return rt;

	r = malloc(sizeof(*r));
	if (r == NULL) {
		logger(ifp->ctx, LOG_ERR, "%s: %m", __func__);
		ipv4_freeroutes(rt);
		return NULL;
	}
	r->dest.s_addr = INADDR_ANY;
	r->net.s_addr = INADDR_ANY;
	r->gate.s_addr = D_CSTATE(ifp)->dst.s_addr;
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

	TAILQ_FOREACH(rtp, rt, next) {
		if (rtp->dest.s_addr != INADDR_ANY)
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
			cp3 = (const char *)&rtn->net.s_addr;
			cplim = cp3 + sizeof(rtn->net.s_addr);
			while (cp3 < cplim) {
				if ((*cp++ ^ *cp2++) & *cp3++)
					break;
			}
			if (cp3 == cplim)
				break;
		}
		if (rtn != rtp)
			continue;
		state = D_CSTATE(ifp);
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
		rtn = malloc(sizeof(*rtn));
		if (rtn == NULL) {
			logger(ifp->ctx, LOG_ERR, "%s: %m", __func__);
			ipv4_freeroutes(rt);
			return NULL;
		}
		rtn->dest.s_addr = rtp->gate.s_addr;
		rtn->net.s_addr = htonl(INADDR_BROADCAST);
		rtn->gate.s_addr = htonl(INADDR_ANY);
		TAILQ_INSERT_BEFORE(rtp, rtn, next);
	}
	return rt;
}

void
ipv4_buildroutes(struct dhcpcd_ctx *ctx)
{
	struct rt_head *nrs, *dnr;
	struct rt *or, *rt, *rtn;
	struct interface *ifp;
	const struct dhcp_state *state;

	/* We need to have the interfaces in the correct order to ensure
	 * our routes are managed correctly. */
	if_sortinterfaces(ctx);

	nrs = malloc(sizeof(*nrs));
	if (nrs == NULL) {
		logger(ctx, LOG_ERR, "%s: %m", __func__);
		return;
	}
	TAILQ_INIT(nrs);

	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		state = D_CSTATE(ifp);
		if (state == NULL || state->new == NULL || !state->added)
			continue;
		dnr = get_routes(ifp);
		dnr = massage_host_routes(dnr, ifp);
		dnr = add_subnet_route(dnr, ifp);
#ifdef IPV4_LOOPBACK_ROUTE
		dnr = add_loopback_route(dnr, ifp);
#endif
		if (ifp->options->options & DHCPCD_GATEWAY) {
			dnr = add_router_host_route(dnr, ifp);
			dnr = add_destination_route(dnr, ifp);
		}
		if (dnr == NULL) /* failed to malloc all new routes */
			continue;
		TAILQ_FOREACH_SAFE(rt, dnr, next, rtn) {
			rt->iface = ifp;
#ifdef HAVE_ROUTE_METRIC
			rt->metric = ifp->metric;
#endif
			rt->flags = state->added & STATE_FAKE;
			/* Is this route already in our table? */
			if ((find_route(nrs, rt, NULL)) != NULL)
				continue;
			rt->src.s_addr = state->addr.s_addr;
			/* Do we already manage it? */
			if ((or = find_route(ctx->ipv4_routes, rt, NULL))) {
				if (state->added & STATE_FAKE)
					continue;
				if (or->flags & STATE_FAKE ||
				    or->iface != ifp ||
#ifdef HAVE_ROUTE_METRIC
				    rt->metric != or->metric ||
#endif
				    or->src.s_addr != state->addr.s_addr ||
				    rt->gate.s_addr != or->gate.s_addr)
				{
					if (c_route(or, rt) != 0)
						continue;
				}
				TAILQ_REMOVE(ctx->ipv4_routes, or, next);
				free(or);
			} else {
				if (state->added & STATE_FAKE) {
					or = ipv4_findrt(ctx, rt, 1);
					if (or == NULL ||
					    or->gate.s_addr != rt->gate.s_addr)
 						continue;
				} else {
					if (n_route(rt) != 0)
						continue;
				}
			}
			rt->flags |= STATE_ADDED;
			TAILQ_REMOVE(dnr, rt, next);
			TAILQ_INSERT_TAIL(nrs, rt, next);
		}
		ipv4_freeroutes(dnr);
	}

	/* Remove old routes we used to manage */
	if (ctx->ipv4_routes) {
		TAILQ_FOREACH(rt, ctx->ipv4_routes, next) {
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
ipv4_deladdr(struct interface *ifp,
    const struct in_addr *addr, const struct in_addr *net)
{
	struct dhcp_state *dstate;
	int r;
	struct ipv4_state *state;
	struct ipv4_addr *ap;

	logger(ifp->ctx, LOG_DEBUG, "%s: deleting IP address %s/%d",
	    ifp->name, inet_ntoa(*addr), inet_ntocidr(*net));

	r = if_deladdress(ifp, addr, net);
	if (r == -1 && errno != EADDRNOTAVAIL && errno != ENXIO &&
	    errno != ENODEV)
		logger(ifp->ctx, LOG_ERR, "%s: %s: %m", ifp->name, __func__);

	dstate = D_STATE(ifp);
	if (dstate->addr.s_addr == addr->s_addr &&
	    dstate->net.s_addr == net->s_addr)
	{
		dstate->added = 0;
		dstate->addr.s_addr = 0;
		dstate->net.s_addr = 0;
	}

	state = IPV4_STATE(ifp);
	TAILQ_FOREACH(ap, &state->addrs, next) {
		if (ap->addr.s_addr == addr->s_addr &&
		    ap->net.s_addr == net->s_addr)
		{
			TAILQ_REMOVE(&state->addrs, ap, next);
			free(ap);
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
	if (ifo->options & DHCPCD_INFORM ||
	    (ifo->options & DHCPCD_STATIC && ifo->req_addr.s_addr == 0))
		return 0;
	r = ipv4_deladdr(ifp, &state->addr, &state->net);
	return r;
}

static struct ipv4_state *
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
	}
	return state;
}

static int
ipv4_addaddr(struct interface *ifp, const struct dhcp_lease *lease)
{
	struct ipv4_state *state;
	struct ipv4_addr *ia;

	if ((state = ipv4_getstate(ifp)) == NULL) {
		logger(ifp->ctx, LOG_ERR, "%s: ipv4_getstate: %m", __func__);
		return -1;
	}
	if (ifp->options->options & DHCPCD_NOALIAS) {
		struct ipv4_addr *ian;

		TAILQ_FOREACH_SAFE(ia, &state->addrs, next, ian) {
			if (ia->addr.s_addr != lease->addr.s_addr)
				ipv4_deladdr(ifp, &ia->addr, &ia->net);
		}
	}

	if ((ia = malloc(sizeof(*ia))) == NULL) {
		logger(ifp->ctx, LOG_ERR, "%s: %m", __func__);
		return -1;
	}

	logger(ifp->ctx, LOG_DEBUG, "%s: adding IP address %s/%d",
	    ifp->name, inet_ntoa(lease->addr),
	    inet_ntocidr(lease->net));
	if (if_addaddress(ifp, &lease->addr, &lease->net, &lease->brd) == -1) {
		if (errno != EEXIST)
			logger(ifp->ctx, LOG_ERR, "%s: if_addaddress: %m",
			    __func__);
		free(ia);
		return -1;
	}

	ia->iface = ifp;
	ia->addr = lease->addr;
	ia->net = lease->net;
#ifdef IN_IFF_TENTATIVE
	ia->addr_flags = IN_IFF_TENTATIVE;
#endif
	TAILQ_INSERT_TAIL(&state->addrs, ia, next);
	return 0;
}

static void
ipv4_finalisert(struct interface *ifp)
{
	const struct dhcp_state *state = D_CSTATE(ifp);

	/* Find any freshly added routes, such as the subnet route.
	 * We do this because we cannot rely on recieving the kernel
	 * notification right now via our link socket. */
	if_initrt(ifp);
	ipv4_buildroutes(ifp->ctx);
	script_runreason(ifp, state->reason);

	dhcpcd_daemonise(ifp->ctx);
}

void
ipv4_finaliseaddr(struct interface *ifp)
{
	struct dhcp_state *state = D_STATE(ifp);
	struct dhcp_lease *lease;

	lease = &state->lease;

	/* Delete the old address if different */
	if (state->addr.s_addr != lease->addr.s_addr &&
	    state->addr.s_addr != 0 &&
	    ipv4_iffindaddr(ifp, &lease->addr, NULL))
		delete_address(ifp);

	state->added = STATE_ADDED;
	state->defend = 0;
	state->addr.s_addr = lease->addr.s_addr;
	state->net.s_addr = lease->net.s_addr;
	ipv4_finalisert(ifp);
}

void
ipv4_applyaddr(void *arg)
{
	struct interface *ifp = arg, *ifn;
	struct dhcp_state *state = D_STATE(ifp), *nstate;
	struct dhcp_message *dhcp;
	struct dhcp_lease *lease;
	struct if_options *ifo = ifp->options;
	struct ipv4_addr *ap;
	int r;

	if (state == NULL)
		return;
	dhcp = state->new;
	lease = &state->lease;

	if (dhcp == NULL) {
		if ((ifo->options & (DHCPCD_EXITING | DHCPCD_PERSISTENT)) !=
		    (DHCPCD_EXITING | DHCPCD_PERSISTENT))
		{
			if (state->added) {
				struct in_addr addr;

				addr = state->addr;
				delete_address(ifp);
				TAILQ_FOREACH(ifn, ifp->ctx->ifaces, next) {
					if (ifn == ifp ||
					    strcmp(ifn->name, ifp->name) == 0)
						continue;
					nstate = D_STATE(ifn);
					if (nstate && !nstate->added &&
					    nstate->addr.s_addr == addr.s_addr)
					{
						if (ifn->options->options &
						    DHCPCD_ARP)
						{
							dhcp_bind(ifn, NULL);
						} else {
							ipv4_addaddr(ifn,
							    &nstate->lease);
							nstate->added =
							    STATE_ADDED;
						}
						break;
					}
				}
			}
			ipv4_buildroutes(ifp->ctx);
			script_runreason(ifp, state->reason);
		} else
			ipv4_buildroutes(ifp->ctx);
		return;
	}

	/* Ensure only one interface has the address */
	TAILQ_FOREACH(ifn, ifp->ctx->ifaces, next) {
		if (ifn == ifp || strcmp(ifn->name, ifp->name) == 0)
			continue;
		nstate = D_STATE(ifn);
		if (nstate && nstate->added &&
		    nstate->addr.s_addr == lease->addr.s_addr)
		{
			if (ifn->metric <= ifp->metric) {
				logger(ifp->ctx, LOG_INFO,
				    "%s: preferring %s on %s",
				    ifp->name,
				    inet_ntoa(lease->addr),
				    ifn->name);
				state->addr.s_addr = lease->addr.s_addr;
				state->net.s_addr = lease->net.s_addr;
				ipv4_finalisert(ifp);
			}
			logger(ifp->ctx, LOG_INFO, "%s: preferring %s on %s",
			    ifn->name,
			    inet_ntoa(lease->addr),
			    ifp->name);
			ipv4_deladdr(ifn, &nstate->addr, &nstate->net);
			nstate->added = 0;
			break;
		}
	}

	/* Does another interface already have the address from a prior boot? */
	if (ifn == NULL) {
		TAILQ_FOREACH(ifn, ifp->ctx->ifaces, next) {
			if (ifn == ifp || strcmp(ifn->name, ifp->name) == 0)
				continue;
			ap = ipv4_iffindaddr(ifn, &lease->addr, NULL);
			if (ap)
				ipv4_deladdr(ifn, &ap->addr, &ap->net);
		}
	}

	/* If the netmask is different, delete the addresss */
	ap = ipv4_iffindaddr(ifp, &lease->addr, NULL);
	if (ap && ap->net.s_addr != lease->net.s_addr)
		ipv4_deladdr(ifp, &ap->addr, &ap->net);

	if (ipv4_iffindaddr(ifp, &lease->addr, &lease->net))
		logger(ifp->ctx, LOG_DEBUG,
		    "%s: IP address %s/%d already exists",
		    ifp->name, inet_ntoa(lease->addr),
		    inet_ntocidr(lease->net));
	else {
		r = ipv4_addaddr(ifp, lease);
		if (r == -1 && errno != EEXIST)
			return;
	}

#ifdef IN_IFF_NOTUSEABLE
	ap = ipv4_iffindaddr(ifp, &lease->addr, NULL);
	if (ap == NULL) {
		logger(ifp->ctx, LOG_ERR, "%s: added address vanished",
		    ifp->name);
		return;
	} else if (ap->addr_flags & IN_IFF_NOTUSEABLE)
		return;
#endif

	ipv4_finaliseaddr(ifp);
}

void
ipv4_handleifa(struct dhcpcd_ctx *ctx,
    int cmd, struct if_head *ifs, const char *ifname,
    const struct in_addr *addr, const struct in_addr *net,
    const struct in_addr *dst, int flags)
{
	struct interface *ifp;
	struct ipv4_state *state;
	struct ipv4_addr *ap;

	if (ifs == NULL)
		ifs = ctx->ifaces;
	if (ifs == NULL) {
		errno = ESRCH;
		return;
	}
	if (addr->s_addr == INADDR_ANY) {
		errno = EINVAL;
		return;
	}
	if ((ifp = if_find(ifs, ifname)) == NULL)
		return;
	if ((state = ipv4_getstate(ifp)) == NULL) {
		errno = ENOENT;
		return;
	}

	ap = ipv4_iffindaddr(ifp, addr, net);
	if (cmd == RTM_NEWADDR) {
		if (ap == NULL) {
			if ((ap = malloc(sizeof(*ap))) == NULL) {
				logger(ifp->ctx, LOG_ERR, "%s: %m", __func__);
				return;
			}
			ap->iface = ifp;
			ap->addr = *addr;
			ap->net = *net;
			if (dst)
				ap->dst.s_addr = dst->s_addr;
			else
				ap->dst.s_addr = INADDR_ANY;
			TAILQ_INSERT_TAIL(&state->addrs, ap, next);
		}
		ap->addr_flags = flags;
	} else if (cmd == RTM_DELADDR) {
		if (ap) {
			TAILQ_REMOVE(&state->addrs, ap, next);
			free(ap);
		}
	}

	dhcp_handleifa(cmd, ifp, addr, net, dst, flags);
	arp_handleifa(cmd, ifp, addr, flags);
}

void
ipv4_free(struct interface *ifp)
{
	struct ipv4_state *state;
	struct ipv4_addr *addr;

	if (ifp) {
		state = IPV4_STATE(ifp);
		if (state) {
		        while ((addr = TAILQ_FIRST(&state->addrs))) {
				TAILQ_REMOVE(&state->addrs, addr, next);
				free(addr);
			}
			ipv4_freerts(&state->routes);
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
