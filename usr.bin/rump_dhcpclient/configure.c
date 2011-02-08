/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2010 Roy Marples <roy@marples.name>
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

#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rump/rump_syscalls.h>

#include "common.h"
#include "configure.h"
#include "dhcp.h"
#include "if-options.h"
#include "net.h"

/* Some systems have route metrics */
#ifndef HAVE_ROUTE_METRIC
# ifdef __linux__
#  define HAVE_ROUTE_METRIC 1
# endif
# ifndef HAVE_ROUTE_METRIC
#  define HAVE_ROUTE_METRIC 0
# endif
#endif

static struct rt *routes;

static struct rt *
find_route(struct rt *rts, const struct rt *r, struct rt **lrt,
    const struct rt *srt)
{
	struct rt *rt;

	if (lrt)
		*lrt = NULL;
	for (rt = rts; rt; rt = rt->next) {
		if (rt->dest.s_addr == r->dest.s_addr &&
#if HAVE_ROUTE_METRIC
		    (srt || (!rt->iface ||
			rt->iface->metric == r->iface->metric)) &&
#endif
                    (!srt || srt != rt) &&
		    rt->net.s_addr == r->net.s_addr)
			return rt;
		if (lrt)
			*lrt = rt;
	}
	return NULL;
}

static void
desc_route(const char *cmd, const struct rt *rt, const char *ifname)
{
	char addr[sizeof("000.000.000.000") + 1];

	strlcpy(addr, inet_ntoa(rt->dest), sizeof(addr));
	if (rt->gate.s_addr == INADDR_ANY)
		fprintf(stderr, "%s: %s route to %s/%d\n", ifname, cmd,
		    addr, inet_ntocidr(rt->net));
	else if (rt->gate.s_addr == rt->dest.s_addr &&
	    rt->net.s_addr == INADDR_BROADCAST)
		fprintf(stderr, "%s: %s host route to %s\n", ifname, cmd,
		    addr);
	else if (rt->dest.s_addr == INADDR_ANY && rt->net.s_addr == INADDR_ANY)
		fprintf(stderr, "%s: %s default route via %s\n", ifname, cmd,
		    inet_ntoa(rt->gate));
	else
		fprintf(stderr, "%s: %s route to %s/%d via %s\n", ifname, cmd,
		    addr, inet_ntocidr(rt->net), inet_ntoa(rt->gate));
}

/* If something other than dhcpcd removes a route,
 * we need to remove it from our internal table. */
int
route_deleted(const struct rt *rt)
{
	struct rt *f, *l;

	f = find_route(routes, rt, &l, NULL);
	if (f == NULL)
		return 0;
	desc_route("removing", f, f->iface->name);
	if (l)
		l->next = f->next;
	else
		routes = f->next;
	free(f);
	return 1;
}

static int
n_route(struct rt *rt, const struct interface *iface)
{
	/* Don't set default routes if not asked to */
	if (rt->dest.s_addr == 0 &&
	    rt->net.s_addr == 0 &&
	    !(iface->state->options->options & DHCPCD_GATEWAY))
		return -1;

	desc_route("adding", rt, iface->name);
	if (!add_route(iface, &rt->dest, &rt->net, &rt->gate, iface->metric))
		return 0;
	if (errno == EEXIST) {
		/* Pretend we added the subnet route */
		if (rt->dest.s_addr == (iface->addr.s_addr & iface->net.s_addr) &&
		    rt->net.s_addr == iface->net.s_addr &&
		    rt->gate.s_addr == 0)
			return 0;
		else
			return -1;
	}
	fprintf(stderr, "%s: add_route failed: %d\n", iface->name, errno);
	return -1;
}

static int
c_route(struct rt *ort, struct rt *nrt, const struct interface *iface)
{
	/* Don't set default routes if not asked to */
	if (nrt->dest.s_addr == 0 &&
	    nrt->net.s_addr == 0 &&
	    !(iface->state->options->options & DHCPCD_GATEWAY))
		return -1;

	desc_route("changing", nrt, iface->name);
	/* We delete and add the route so that we can change metric.
	 * This also has the nice side effect of flushing ARP entries so
	 * we don't have to do that manually. */
	del_route(ort->iface, &ort->dest, &ort->net, &ort->gate,
	    ort->iface->metric);
	if (!add_route(iface, &nrt->dest, &nrt->net, &nrt->gate,
		iface->metric))
		return 0;
	fprintf(stderr, "%s: add_route failed: %d\n", iface->name, errno);
	return -1;
}

static int
d_route(struct rt *rt, const struct interface *iface, int metric)
{
	int retval;

	desc_route("deleting", rt, iface->name);
	retval = del_route(iface, &rt->dest, &rt->net, &rt->gate, metric);
	if (retval != 0 && errno != ENOENT && errno != ESRCH)
		fprintf(stderr,"%s: del_route: %d\n", iface->name, errno);
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
		net.s_addr = get_netmask(addr);
	if (net.s_addr == INADDR_BROADCAST || net.s_addr == INADDR_ANY)
		return NULL;
	rt = malloc(sizeof(*rt));
	rt->dest.s_addr = addr & net.s_addr;
	rt->net.s_addr = net.s_addr;
	rt->gate.s_addr = 0;
	return rt;
}

static struct rt *
add_subnet_route(struct rt *rt, const struct interface *iface)
{
	struct rt *r;

	if (iface->net.s_addr == INADDR_BROADCAST ||
	    iface->net.s_addr == INADDR_ANY ||
	    (iface->state->options->options &
	     (DHCPCD_INFORM | DHCPCD_STATIC) &&
	     iface->state->options->req_addr.s_addr == INADDR_ANY))
		return rt;

	r = xmalloc(sizeof(*r));
	r->dest.s_addr = iface->addr.s_addr & iface->net.s_addr;
	r->net.s_addr = iface->net.s_addr;
	r->gate.s_addr = 0;
	r->next = rt;
	return r;
}

static struct rt *
get_routes(const struct interface *iface)
{
	struct rt *rt, *nrt = NULL, *r = NULL;

	if (iface->state->options->routes != NULL) {
		for (rt = iface->state->options->routes;
		     rt != NULL;
		     rt = rt->next)
		{
			if (rt->gate.s_addr == 0)
				break;
			if (r == NULL)
				r = nrt = xmalloc(sizeof(*r));
			else {
				r->next = xmalloc(sizeof(*r));
				r = r->next;
			}
			memcpy(r, rt, sizeof(*r));
			r->next = NULL;
		}
		return nrt;
	}

	return get_option_routes(iface->state->new,
	    iface->name, &iface->state->options->options);
}

/* Some DHCP servers add set host routes by setting the gateway
 * to the assinged IP address. This differs from our notion of a host route
 * where the gateway is the destination address, so we fix it. */
static struct rt *
massage_host_routes(struct rt *rt, const struct interface *iface)
{
	struct rt *r;

	for (r = rt; r; r = r->next)
		if (r->gate.s_addr == iface->addr.s_addr &&
		    r->net.s_addr == INADDR_BROADCAST)
			r->gate.s_addr = r->dest.s_addr;
	return rt;
}

static struct rt *
add_destination_route(struct rt *rt, const struct interface *iface)
{
	struct rt *r;

	if (!(iface->flags & IFF_POINTOPOINT) ||
	    !has_option_mask(iface->state->options->dstmask, DHO_ROUTER))
		return rt;
	r = xmalloc(sizeof(*r));
	r->dest.s_addr = INADDR_ANY;
	r->net.s_addr = INADDR_ANY;
	r->gate.s_addr = iface->dst.s_addr;
	r->next = rt;
	return r;
}

/* We should check to ensure the routers are on the same subnet
 * OR supply a host route. If not, warn and add a host route. */
static struct rt *
add_router_host_route(struct rt *rt, const struct interface *ifp)
{
	struct rt *rtp, *rtl, *rtn;
	const char *cp, *cp2, *cp3, *cplim;

	for (rtp = rt, rtl = NULL; rtp; rtl = rtp, rtp = rtp->next) {
		if (rtp->dest.s_addr != INADDR_ANY)
			continue;
		/* Scan for a route to match */
		for (rtn = rt; rtn != rtp; rtn = rtn->next) {
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
			fprintf(stderr,
			    "%s: forcing router %s through interface\n",
			    ifp->name, inet_ntoa(rtp->gate));
			rtp->gate.s_addr = 0;
			continue;
		}
		fprintf(stderr, "%s: router %s requires a host route\n",
		    ifp->name, inet_ntoa(rtp->gate));
		rtn = xmalloc(sizeof(*rtn));
		rtn->dest.s_addr = rtp->gate.s_addr;
		rtn->net.s_addr = INADDR_BROADCAST;
		rtn->gate.s_addr = rtp->gate.s_addr;
		rtn->next = rtp;
		if (rtl == NULL)
			rt = rtn;
		else
			rtl->next = rtn;
	}
	return rt;
}

void
build_routes(void)
{
	struct rt *nrs = NULL, *dnr, *or, *rt, *rtn, *rtl, *lrt = NULL;
	const struct interface *ifp;

	for (ifp = ifaces; ifp; ifp = ifp->next) {
		if (ifp->state->new == NULL)
			continue;
		dnr = get_routes(ifp);
		dnr = massage_host_routes(dnr, ifp);
		dnr = add_subnet_route(dnr, ifp);
		dnr = add_router_host_route(dnr, ifp);
		dnr = add_destination_route(dnr, ifp);
		for (rt = dnr; rt && (rtn = rt->next, 1); lrt = rt, rt = rtn) {
			rt->iface = ifp;
			/* Is this route already in our table? */
			if ((find_route(nrs, rt, NULL, NULL)) != NULL)
				continue;
			/* Do we already manage it? */
			if ((or = find_route(routes, rt, &rtl, NULL))) {
				if (or->iface != ifp ||
				    rt->gate.s_addr != or->gate.s_addr)
				{
					if (c_route(or, rt, ifp) != 0)
						continue;
				}
				if (rtl != NULL)
					rtl->next = or->next;
				else
					routes = or->next;
				free(or);
			} else {
				if (n_route(rt, ifp) != 0)
					continue;
			}
			if (dnr == rt)
				dnr = rtn;
			else if (lrt)
				lrt->next = rtn;
			rt->next = nrs;
			nrs = rt;
		}
		free_routes(dnr);
	}

	/* Remove old routes we used to manage */
	for (rt = routes; rt; rt = rt->next) {
		if (find_route(nrs, rt, NULL, NULL) == NULL)
			d_route(rt, rt->iface, rt->iface->metric);
	}

	free_routes(routes);
	routes = nrs;
}

static int
delete_address(struct interface *iface)
{
	int retval;
	struct if_options *ifo;

	ifo = iface->state->options;
	if (ifo->options & DHCPCD_INFORM ||
	    (ifo->options & DHCPCD_STATIC && ifo->req_addr.s_addr == 0))
		return 0;
	fprintf(stderr, "%s: deleting IP address %s/%d\n",
	    iface->name,
	    inet_ntoa(iface->addr),
	    inet_ntocidr(iface->net));
	retval = del_address(iface, &iface->addr, &iface->net);
	if (retval == -1 && errno != EADDRNOTAVAIL) 
		fprintf(stderr, "del_address failed: %d\n", errno);
	iface->addr.s_addr = 0;
	iface->net.s_addr = 0;
	return retval;
}

int
configure(struct interface *iface)
{
	struct dhcp_message *dhcp = iface->state->new;
	struct dhcp_lease *lease = &iface->state->lease;
	struct if_options *ifo = iface->state->options;
	struct rt *rt;

	/* This also changes netmask */
	if (!(ifo->options & DHCPCD_INFORM) ||
	    !has_address(iface->name, &lease->addr, &lease->net))
	{
		fprintf(stderr, "%s: adding IP address %s/%d\n",
		    iface->name, inet_ntoa(lease->addr),
		    inet_ntocidr(lease->net));
		if (add_address(iface,
			&lease->addr, &lease->net, &lease->brd) == -1 &&
		    errno != EEXIST)
		{
			fprintf(stderr, "add_address failed\n");
			return -1;
		}
	}

	/* Now delete the old address if different */
	if (iface->addr.s_addr != lease->addr.s_addr &&
	    iface->addr.s_addr != 0)
		delete_address(iface);

	iface->addr.s_addr = lease->addr.s_addr;
	iface->net.s_addr = lease->net.s_addr;

	/* We need to delete the subnet route to have our metric or
	 * prefer the interface. */
	rt = get_subnet_route(dhcp);
	if (rt != NULL) {
		rt->iface = iface;
		if (!find_route(routes, rt, NULL, NULL))
			del_route(iface, &rt->dest, &rt->net, &rt->gate, 0);
		free(rt);
	}

	build_routes();

	fprintf(stderr, "lease time: "); 
	if (lease->leasetime == ~0U)
		fprintf(stderr, "infinite\n");
	else 
		fprintf(stderr, "%u seconds (%.2f days)\n",
		    lease->leasetime, lease->leasetime / (60*60*24+.0));

	return 0;
}
