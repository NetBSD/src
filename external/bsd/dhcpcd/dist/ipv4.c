#include <sys/cdefs.h>
 __RCSID("$NetBSD: ipv4.c,v 1.1.1.1.2.3 2014/08/19 23:46:43 tls Exp $");

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

#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/route.h>

#ifdef __linux__
#  include <asm/types.h> /* for systems with broken headers */
#  include <linux/rtnetlink.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "dhcpcd.h"
#include "dhcp.h"
#include "if.h"
#include "if-options.h"
#include "ipv4.h"
#include "script.h"

#define IPV4_LOOPBACK_ROUTE
#ifdef __linux__
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
ipv4_findaddr(struct interface *ifp,
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
		if (addr != NULL && ipv4_findaddr(ifp, addr, NULL))
			return 1;
	}
	return 0;
}

void
ipv4_freeroutes(struct rt_head *rts)
{
	struct rt *r;

	if (rts) {
		while ((r = TAILQ_FIRST(rts))) {
			TAILQ_REMOVE(rts, r, next);
			free(r);
		}
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
	return 0;
}


/* Interface comparer for working out ordering. */
static int
ipv4_ifcmp(const struct interface *si, const struct interface *ti)
{
	int sill, till;
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
	/* If we are either, they neither have a lease, or they both have.
	 * We need to check for IPv4LL and make it non-preferred. */
	if (sis->new && tis->new) {
		sill = (sis->new->cookie == htonl(MAGIC_COOKIE));
		till = (tis->new->cookie == htonl(MAGIC_COOKIE));
		if (!sill && till)
			return 1;
		if (sill && !till)
			return -1;
	}
	/* Then carrier status. */
	if (si->carrier > ti->carrier)
		return -1;
	if (si->carrier < ti->carrier)
		return 1;
	/* Finally, metric */
	if (si->metric < ti->metric)
		return -1;
	if (si->metric > ti->metric)
		return 1;
	return 0;
}

/* Sort the interfaces into a preferred order - best first, worst last. */
void
ipv4_sortinterfaces(struct dhcpcd_ctx *ctx)
{
	struct if_head sorted;
	struct interface *ifp, *ift;

	if (ctx->ifaces == NULL ||
	    (ifp = TAILQ_FIRST(ctx->ifaces)) == NULL ||
	    TAILQ_NEXT(ifp, next) == NULL)
		return;

	TAILQ_INIT(&sorted);
	TAILQ_REMOVE(ctx->ifaces, ifp, next);
	TAILQ_INSERT_HEAD(&sorted, ifp, next);
	while ((ifp = TAILQ_FIRST(ctx->ifaces))) {
		TAILQ_REMOVE(ctx->ifaces, ifp, next);
		TAILQ_FOREACH(ift, &sorted, next) {
			if (ipv4_ifcmp(ifp, ift) == -1) {
				TAILQ_INSERT_BEFORE(ift, ifp, next);
				break;
			}
		}
		if (ift == NULL)
			TAILQ_INSERT_TAIL(&sorted, ifp, next);
	}
	TAILQ_CONCAT(ctx->ifaces, &sorted, next);
}

static struct rt *
find_route(struct rt_head *rts, const struct rt *r, const struct rt *srt)
{
	struct rt *rt;

	if (rts == NULL)
		return NULL;
	TAILQ_FOREACH(rt, rts, next) {
		if (rt->dest.s_addr == r->dest.s_addr &&
#if HAVE_ROUTE_METRIC
		    (srt || (!rt->iface ||
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
	const char *ifname = rt->iface->name;

	strlcpy(addr, inet_ntoa(rt->dest), sizeof(addr));
	if (rt->gate.s_addr == INADDR_ANY)
		syslog(LOG_INFO, "%s: %s route to %s/%d", ifname, cmd,
		    addr, inet_ntocidr(rt->net));
	else if (rt->gate.s_addr == rt->dest.s_addr &&
	    rt->net.s_addr == INADDR_BROADCAST)
		syslog(LOG_INFO, "%s: %s host route to %s", ifname, cmd,
		    addr);
	else if (rt->gate.s_addr == htonl(INADDR_LOOPBACK) &&
	    rt->net.s_addr == INADDR_BROADCAST)
		syslog(LOG_INFO, "%s: %s host route to %s via %s", ifname, cmd,
		    addr, inet_ntoa(rt->gate));
	else if (rt->dest.s_addr == INADDR_ANY && rt->net.s_addr == INADDR_ANY)
		syslog(LOG_INFO, "%s: %s default route via %s", ifname, cmd,
		    inet_ntoa(rt->gate));
	else
		syslog(LOG_INFO, "%s: %s route to %s/%d via %s", ifname, cmd,
		    addr, inet_ntocidr(rt->net), inet_ntoa(rt->gate));
}

/* If something other than dhcpcd removes a route,
 * we need to remove it from our internal table. */
int
ipv4_routedeleted(struct dhcpcd_ctx *ctx, const struct rt *rt)
{
	struct rt *f;

	f = find_route(ctx->ipv4_routes, rt, NULL);
	if (f == NULL)
		return 0;
	desc_route("removing", f);
	TAILQ_REMOVE(ctx->ipv4_routes, f, next);
	free(f);
	return 1;
}

#define n_route(a)	 nc_route(1, a, a)
#define c_route(a, b)	 nc_route(0, a, b)
static int
nc_route(int add, struct rt *ort, struct rt *nrt)
{

	/* Don't set default routes if not asked to */
	if (nrt->dest.s_addr == 0 &&
	    nrt->net.s_addr == 0 &&
	    !(nrt->iface->options->options & DHCPCD_GATEWAY))
		return -1;

	desc_route(add ? "adding" : "changing", nrt);
	/* We delete and add the route so that we can change metric and
	 * prefer the interface.
	 * This also has the nice side effect of flushing ARP entries so
	 * we don't have to do that manually. */
	if (if_delroute(ort) == -1 && errno != ESRCH)
		syslog(LOG_ERR, "%s: ipv4_delroute: %m", ort->iface->name);
	if (!if_addroute(nrt))
		return 0;
	syslog(LOG_ERR, "%s: if_addroute: %m", nrt->iface->name);
	return -1;
}

static int
d_route(struct rt *rt)
{
	int retval;

	desc_route("deleting", rt);
	retval = if_delroute(rt);
	if (retval != 0 && errno != ENOENT && errno != ESRCH)
		syslog(LOG_ERR,"%s: if_delroute: %m", rt->iface->name);
	return retval;
}

static struct rt *
get_subnet_route(struct dhcpcd_ctx *ctx, struct dhcp_message *dhcp)
{
	in_addr_t addr;
	struct in_addr net;
	struct rt *rt;

	addr = dhcp->yiaddr;
	if (addr == 0)
		addr = dhcp->ciaddr;
	/* Ensure we have all the needed values */
	if (get_option_addr(ctx, &net, dhcp, DHO_SUBNETMASK) == -1)
		net.s_addr = ipv4_getnetmask(addr);
	if (net.s_addr == INADDR_BROADCAST || net.s_addr == INADDR_ANY)
		return NULL;
	rt = malloc(sizeof(*rt));
	rt->dest.s_addr = addr & net.s_addr;
	rt->net.s_addr = net.s_addr;
	rt->gate.s_addr = 0;
	return rt;
}

static struct rt_head *
add_subnet_route(struct rt_head *rt, const struct interface *ifp)
{
	struct rt *r;
	const struct dhcp_state *s;

	if (rt == NULL) /* earlier malloc failed */
		return NULL;

	s = D_CSTATE(ifp);
	if (s->net.s_addr == INADDR_BROADCAST ||
	    s->net.s_addr == INADDR_ANY)
		return rt;

	r = malloc(sizeof(*r));
	if (r == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		ipv4_freeroutes(rt);
		return NULL;
	}
	r->dest.s_addr = s->addr.s_addr & s->net.s_addr;
	r->net.s_addr = s->net.s_addr;
	r->gate.s_addr = 0;
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
		syslog(LOG_ERR, "%s: %m", __func__);
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
				syslog(LOG_ERR, "%s: %m", __func__);
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
 * to the assinged IP address. This differs from our notion of a host route
 * where the gateway is the destination address, so we fix it. */
static struct rt_head *
massage_host_routes(struct rt_head *rt, const struct interface *ifp)
{
	struct rt *r;

	if (rt) {
		TAILQ_FOREACH(r, rt, next) {
			if (r->gate.s_addr == D_CSTATE(ifp)->addr.s_addr &&
			    r->net.s_addr == INADDR_BROADCAST)
				r->gate.s_addr = r->dest.s_addr;
		}
	}
	return rt;
}

static struct rt_head *
add_destination_route(struct rt_head *rt, const struct interface *iface)
{
	struct rt *r;

	if (rt == NULL || /* failed a malloc earlier probably */
	    !(iface->flags & IFF_POINTOPOINT) ||
	    !has_option_mask(iface->options->dstmask, DHO_ROUTER))
		return rt;

	r = malloc(sizeof(*r));
	if (r == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		ipv4_freeroutes(rt);
		return NULL;
	}
	r->dest.s_addr = INADDR_ANY;
	r->net.s_addr = INADDR_ANY;
	r->gate.s_addr = D_CSTATE(iface)->dst.s_addr;
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
		if (ifp->flags & IFF_NOARP) {
			syslog(LOG_WARNING,
			    "%s: forcing router %s through interface",
			    ifp->name, inet_ntoa(rtp->gate));
			rtp->gate.s_addr = 0;
			continue;
		}
		syslog(LOG_WARNING, "%s: router %s requires a host route",
		    ifp->name, inet_ntoa(rtp->gate));
		rtn = malloc(sizeof(*rtn));
		if (rtn == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			ipv4_freeroutes(rt);
			return NULL;
		}
		rtn->dest.s_addr = rtp->gate.s_addr;
		rtn->net.s_addr = INADDR_BROADCAST;
		rtn->gate.s_addr = rtp->gate.s_addr;
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

	nrs = malloc(sizeof(*nrs));
	if (nrs == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return;
	}
	TAILQ_INIT(nrs);
	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		state = D_CSTATE(ifp);
		if (state == NULL || state->new == NULL)
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
			rt->metric = ifp->metric;
			/* Is this route already in our table? */
			if ((find_route(nrs, rt, NULL)) != NULL)
				continue;
			rt->src.s_addr = state->addr.s_addr;
			/* Do we already manage it? */
			if ((or = find_route(ctx->ipv4_routes, rt, NULL))) {
				if (or->iface != ifp ||
				    or->src.s_addr != state->addr.s_addr ||
				    rt->gate.s_addr != or->gate.s_addr ||
				    rt->metric != or->metric)
				{
					if (c_route(or, rt) != 0)
						continue;
				}
				TAILQ_REMOVE(ctx->ipv4_routes, or, next);
				free(or);
			} else {
				if (n_route(rt) != 0)
					continue;
			}
			TAILQ_REMOVE(dnr, rt, next);
			TAILQ_INSERT_TAIL(nrs, rt, next);
		}
		ipv4_freeroutes(dnr);
	}

	/* Remove old routes we used to manage */
	TAILQ_FOREACH(rt, ctx->ipv4_routes, next) {
		if (find_route(nrs, rt, NULL) == NULL &&
		    (rt->iface->options->options &
		    (DHCPCD_EXITING | DHCPCD_PERSISTENT)) !=
		    (DHCPCD_EXITING | DHCPCD_PERSISTENT))
			d_route(rt);
	}
	ipv4_freeroutes(ctx->ipv4_routes);

	ctx->ipv4_routes = nrs;
}

static int
delete_address1(struct interface *ifp,
    const struct in_addr *addr, const struct in_addr *net)
{
	int r;
	struct ipv4_state *state;
	struct ipv4_addr *ap;

	syslog(LOG_DEBUG, "%s: deleting IP address %s/%d",
	    ifp->name, inet_ntoa(*addr), inet_ntocidr(*net));
	r = if_deladdress(ifp, addr, net);
	if (r == -1 && errno != EADDRNOTAVAIL && errno != ENXIO &&
	    errno != ENODEV)
		syslog(LOG_ERR, "%s: %s: %m", ifp->name, __func__);

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
	r = delete_address1(ifp, &state->addr, &state->net);
	state->addr.s_addr = 0;
	state->net.s_addr = 0;
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
			syslog(LOG_ERR, "%s: %m", __func__);
			return NULL;
		}
		TAILQ_INIT(&state->addrs);
	}
	return state;
}

void
ipv4_applyaddr(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);
	struct dhcp_message *dhcp;
	struct dhcp_lease *lease;
	struct if_options *ifo = ifp->options;
	struct ipv4_addr *ap;
	struct ipv4_state *istate;
	struct rt *rt;
	int r;

	/* As we are now adjusting an interface, we need to ensure
	 * we have them in the right order for routing and configuration. */
	ipv4_sortinterfaces(ifp->ctx);

	if (state == NULL)
		return;
	dhcp = state->new;
	lease = &state->lease;

	if (dhcp == NULL) {
		ipv4_buildroutes(ifp->ctx);
		if ((ifo->options & (DHCPCD_EXITING | DHCPCD_PERSISTENT)) !=
		    (DHCPCD_EXITING | DHCPCD_PERSISTENT))
		{
			if (state->addr.s_addr != 0)
				delete_address(ifp);
			script_runreason(ifp, state->reason);
		}
		return;
	}

	/* If the netmask is different, delete the addresss */
	ap = ipv4_findaddr(ifp, &lease->addr, NULL);
	if (ap && ap->net.s_addr != lease->net.s_addr)
		delete_address1(ifp, &ap->addr, &ap->net);

	if (ipv4_findaddr(ifp, &lease->addr, &lease->net))
		syslog(LOG_DEBUG, "%s: IP address %s/%d already exists",
		    ifp->name, inet_ntoa(lease->addr),
		    inet_ntocidr(lease->net));
	else {
		syslog(LOG_DEBUG, "%s: adding IP address %s/%d",
		    ifp->name, inet_ntoa(lease->addr),
		    inet_ntocidr(lease->net));
		if (ifo->options & DHCPCD_NOALIAS)
			r = if_setaddress(ifp,
			    &lease->addr, &lease->net, &lease->brd);
		else
			r = if_addaddress(ifp,
			    &lease->addr, &lease->net, &lease->brd);
		if (r == -1 && errno != EEXIST) {
			syslog(LOG_ERR, "%s: if_addaddress: %m", __func__);
			return;
		}
		istate = ipv4_getstate(ifp);
		ap = malloc(sizeof(*ap));
		ap->addr = lease->addr;
		ap->net = lease->net;
		ap->dst.s_addr = INADDR_ANY;
		TAILQ_INSERT_TAIL(&istate->addrs, ap, next);
	}

	/* Now delete the old address if different */
	if (state->addr.s_addr != lease->addr.s_addr &&
	    state->addr.s_addr != 0)
		delete_address(ifp);

	state->addr.s_addr = lease->addr.s_addr;
	state->net.s_addr = lease->net.s_addr;

	/* We need to delete the subnet route to have our metric or
	 * prefer the interface. */
	rt = get_subnet_route(ifp->ctx, dhcp);
	if (rt != NULL) {
		rt->iface = ifp;
		rt->metric = 0;
		if (!find_route(ifp->ctx->ipv4_routes, rt, NULL))
			if_delroute(rt);
		free(rt);
	}

	ipv4_buildroutes(ifp->ctx);
	if (!state->lease.frominfo &&
	    !(ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC)))
		if (write_lease(ifp, dhcp) == -1)
			syslog(LOG_ERR, "%s: write_lease: %m", __func__);
	script_runreason(ifp, state->reason);
}

void
ipv4_handleifa(struct dhcpcd_ctx *ctx,
    int type, struct if_head *ifs, const char *ifname,
    const struct in_addr *addr, const struct in_addr *net,
    const struct in_addr *dst)
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

	TAILQ_FOREACH(ifp, ifs, next) {
		if (strcmp(ifp->name, ifname) == 0)
			break;
	}
	if (ifp == NULL) {
		errno = ESRCH;
		return;
	}
	state = ipv4_getstate(ifp);
	if (state == NULL) {
		errno = ENOENT;
		return;
	}

	ap = ipv4_findaddr(ifp, addr, net);
	if (type == RTM_NEWADDR && ap == NULL) {
		ap = malloc(sizeof(*ap));
		if (ap == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return;
		}
		ap->addr.s_addr = addr->s_addr;
		ap->net.s_addr = net->s_addr;
		if (dst)
			ap->dst.s_addr = dst->s_addr;
		else
			ap->dst.s_addr = INADDR_ANY;
		TAILQ_INSERT_TAIL(&state->addrs, ap, next);
	} else if (type == RTM_DELADDR) {
		if (ap == NULL)
			return;
		TAILQ_REMOVE(&state->addrs, ap, next);
		free(ap);
	}

	dhcp_handleifa(type, ifp, addr, net, dst);
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
			free(state);
		}
	}
}

void
ipv4_ctxfree(struct dhcpcd_ctx *ctx)
{

	ipv4_freeroutes(ctx->ipv4_routes);
}
