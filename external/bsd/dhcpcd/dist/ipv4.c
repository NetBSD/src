#include <sys/cdefs.h>
 __RCSID("$NetBSD: ipv4.c,v 1.1.1.1 2013/06/21 19:33:07 roy Exp $");

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
#include <ifaddrs.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "dhcp.h"
#include "if-options.h"
#include "if-pref.h"
#include "ipv4.h"
#include "net.h"
#include "script.h"

static struct rt_head *routes;

int
inet_ntocidr(struct in_addr address)
{
	int cidr = 0;
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
	ocets = (cidr + 7) / 8;

	addr->s_addr = 0;
	if (ocets > 0) {
		memset(&addr->s_addr, 255, (size_t)ocets - 1);
		memset((unsigned char *)&addr->s_addr + (ocets - 1),
		    (256 - (1 << (32 - cidr) % 8)), 1);
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

int
ipv4_doaddress(const char *ifname,
    struct in_addr *addr, struct in_addr *net, struct in_addr *dst, int act)
{
	struct ifaddrs *ifaddrs, *ifa;
	const struct sockaddr_in *a, *n, *d;
	int retval;

	if (getifaddrs(&ifaddrs) == -1)
		return -1;

	retval = 0;
	for (ifa = ifaddrs; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL ||
		    ifa->ifa_addr->sa_family != AF_INET ||
		    strcmp(ifa->ifa_name, ifname) != 0)
			continue;
		a = (const struct sockaddr_in *)(void *)ifa->ifa_addr;
		n = (const struct sockaddr_in *)(void *)ifa->ifa_netmask;
		if (ifa->ifa_flags & IFF_POINTOPOINT)
			d = (const struct sockaddr_in *)(void *)
			    ifa->ifa_dstaddr;
		else
			d = NULL;
		if (act == 1) {
			addr->s_addr = a->sin_addr.s_addr;
			net->s_addr = n->sin_addr.s_addr;
			if (dst) {
				if (ifa->ifa_flags & IFF_POINTOPOINT)
					dst->s_addr = d->sin_addr.s_addr;
				else
					dst->s_addr = INADDR_ANY;
			}
			retval = 1;
			break;
		}
		if (addr->s_addr == a->sin_addr.s_addr &&
		    (net == NULL || net->s_addr == n->sin_addr.s_addr))
		{
			retval = 1;
			break;
		}
	}
	freeifaddrs(ifaddrs);
	return retval;
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

#ifdef DEBUG_MEMORY
static void
ipv4_cleanup()
{

	ipv4_freeroutes(routes);
}
#endif

int
ipv4_init(void)
{

	if (routes == NULL) {
		routes = malloc(sizeof(*routes));
		if (routes == NULL)
			return -1;
		TAILQ_INIT(routes);
#ifdef DEBUG_MEMORY
		atexit(ipv4_cleanup);
#endif
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
ipv4_routedeleted(const struct rt *rt)
{
	struct rt *f;

	f = find_route(routes, rt, NULL);
	if (f == NULL)
		return 0;
	desc_route("removing", f);
	TAILQ_REMOVE(routes, f, next);
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
	ipv4_deleteroute(ort);
	if (!ipv4_addroute(nrt))
		return 0;
	syslog(LOG_ERR, "%s: ipv4_addroute: %m", nrt->iface->name);
	return -1;
}

static int
d_route(struct rt *rt)
{
	int retval;

	desc_route("deleting", rt);
	retval = ipv4_deleteroute(rt);
	if (retval != 0 && errno != ENOENT && errno != ESRCH)
		syslog(LOG_ERR,"%s: ipv4_deleteroute: %m", rt->iface->name);
	return retval;
}

static struct rt *
get_subnet_route(struct dhcp_message *dhcp)
{
	in_addr_t addr;
	struct in_addr net;
	struct rt *rt;

	addr = dhcp->yiaddr;
	if (addr == 0)
		addr = dhcp->ciaddr;
	/* Ensure we have all the needed values */
	if (get_option_addr(&net, dhcp, DHO_SUBNETMASK) == -1)
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
	    s->net.s_addr == INADDR_ANY ||
	    (ifp->options->options &
	     (DHCPCD_INFORM | DHCPCD_STATIC) &&
	     ifp->options->req_addr.s_addr == INADDR_ANY))
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
			TAILQ_INSERT_TAIL(nrt, rt, next);
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
ipv4_buildroutes(void)
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
	TAILQ_FOREACH(ifp, ifaces, next) {
		state = D_CSTATE(ifp);
		if (state == NULL || state->new == NULL)
			continue;
		dnr = get_routes(ifp);
		dnr = massage_host_routes(dnr, ifp);
		dnr = add_subnet_route(dnr, ifp);
		dnr = add_loopback_route(dnr, ifp);
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
			if ((or = find_route(routes, rt, NULL))) {
				if (or->iface != ifp ||
				    or->src.s_addr != state->addr.s_addr ||
				    rt->gate.s_addr != or->gate.s_addr ||
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
			TAILQ_REMOVE(dnr, rt, next);
			TAILQ_INSERT_TAIL(nrs, rt, next);
		}
		ipv4_freeroutes(dnr);
	}

	/* Remove old routes we used to manage */
	TAILQ_FOREACH(rt, routes, next) {
		if (find_route(nrs, rt, NULL) == NULL)
			d_route(rt);
	}
	ipv4_freeroutes(routes);

	routes = nrs;
}

static int
delete_address(struct interface *iface)
{
	int retval;
	struct if_options *ifo;
	struct dhcp_state *state;

	state = D_STATE(iface);
	ifo = iface->options;
	if (ifo->options & DHCPCD_INFORM ||
	    (ifo->options & DHCPCD_STATIC && ifo->req_addr.s_addr == 0))
		return 0;
	syslog(LOG_DEBUG, "%s: deleting IP address %s/%d",
	    iface->name, inet_ntoa(state->addr), inet_ntocidr(state->net));
	retval = ipv4_deleteaddress(iface, &state->addr, &state->net);
	if (retval == -1 && errno != EADDRNOTAVAIL && errno != ENXIO &&
	    errno != ENODEV)
		syslog(LOG_ERR, "del_address: %m");
	state->addr.s_addr = 0;
	state->net.s_addr = 0;
	return retval;
}

void
ipv4_applyaddr(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);
	struct dhcp_message *dhcp;
	struct dhcp_lease *lease;
	struct if_options *ifo = ifp->options;
	struct rt *rt;
	int r;

	/* As we are now adjusting an interface, we need to ensure
	 * we have them in the right order for routing and configuration. */
	sort_interfaces();

	if (state == NULL)
		return;
	dhcp = state->new;
	lease = &state->lease;

	if (dhcp == NULL) {
		if (!(ifo->options & DHCPCD_PERSISTENT)) {
			ipv4_buildroutes();
			if (state->addr.s_addr != 0)
				delete_address(ifp);
			script_runreason(ifp, state->reason);
		}
		return;
	}

	/* This also changes netmask */
	if (!(ifo->options & DHCPCD_INFORM) ||
	    !ipv4_hasaddress(ifp->name, &lease->addr, &lease->net))
	{
		syslog(LOG_DEBUG, "%s: adding IP address %s/%d",
		    ifp->name, inet_ntoa(lease->addr),
		    inet_ntocidr(lease->net));
		if (ifo->options & DHCPCD_NOALIAS)
			r = ipv4_setaddress(ifp,
			    &lease->addr, &lease->net, &lease->brd);
		else
			r = ipv4_addaddress(ifp,
			    &lease->addr, &lease->net, &lease->brd);
		if (r == -1 && errno != EEXIST) {
			syslog(LOG_ERR, "%s: ipv4_addaddress: %m", __func__);
			return;
		}
	}

	/* Now delete the old address if different */
	if (state->addr.s_addr != lease->addr.s_addr &&
	    state->addr.s_addr != 0)
		delete_address(ifp);

	state->addr.s_addr = lease->addr.s_addr;
	state->net.s_addr = lease->net.s_addr;

	/* We need to delete the subnet route to have our metric or
	 * prefer the interface. */
	rt = get_subnet_route(dhcp);
	if (rt != NULL) {
		rt->iface = ifp;
		rt->metric = 0;
		if (!find_route(routes, rt, NULL))
			ipv4_deleteroute(rt);
		free(rt);
	}

	ipv4_buildroutes();
	if (!state->lease.frominfo &&
	    !(ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC)))
		if (write_lease(ifp, dhcp) == -1)
			syslog(LOG_ERR, "%s: write_lease: %m", __func__);
	script_runreason(ifp, state->reason);
}

void
ipv4_handleifa(int type, const char *ifname,
    struct in_addr *addr, struct in_addr *net, struct in_addr *dst)
{
	struct interface *ifp;
	struct if_options *ifo;
	struct dhcp_state *state;
	int i;

	if (addr->s_addr == INADDR_ANY)
		return;
	TAILQ_FOREACH(ifp, ifaces, next) {
		if (strcmp(ifp->name, ifname) == 0)
			break;
	}
	if (ifp == NULL)
		return;

	state = D_STATE(ifp);
	if (state == NULL)
		return;

	if (type == RTM_DELADDR) {
		if (state->new &&
		    state->new->yiaddr == addr->s_addr)
			syslog(LOG_INFO, "%s: removing IP address %s/%d",
			    ifp->name, inet_ntoa(state->lease.addr),
			    inet_ntocidr(state->lease.net));
		return;
	}

	if (type != RTM_NEWADDR)
		return;

	ifo = ifp->options;
	if ((ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC)) == 0 ||
	    ifo->req_addr.s_addr != INADDR_ANY)
		return;

	free(state->old);
	state->old = state->new;
	state->new = dhcp_message_new(addr, net);
	state->dst.s_addr = dst ? dst->s_addr : INADDR_ANY;
	if (dst) {
		for (i = 1; i < 255; i++)
			if (i != DHO_ROUTER && has_option_mask(ifo->dstmask,i))
				dhcp_message_add_addr(state->new, i, *dst);
	}
	state->reason = "STATIC";
	ipv4_buildroutes();
	script_runreason(ifp, state->reason);
	if (ifo->options & DHCPCD_INFORM) {
		state->state = DHS_INFORM;
		state->xid = dhcp_xid(ifp);
		state->lease.server.s_addr = dst ? dst->s_addr : INADDR_ANY;
		state->addr = *addr;
		state->net = *net;
		dhcp_inform(ifp);
	}
}
