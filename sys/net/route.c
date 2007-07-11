/*	$NetBSD: route.c,v 1.88.2.1 2007/07/11 20:11:02 mjf Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Kevin M. Lahey of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1980, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)route.c	8.3 (Berkeley) 1/9/95
 */

#include "opt_route.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: route.c,v 1.88.2.1 2007/07/11 20:11:02 mjf Exp $");

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/pool.h>

#include <net/if.h>
#include <net/route.h>
#include <net/raw_cb.h>

#include <netinet/in.h>
#include <netinet/in_var.h>

#ifdef RTFLUSH_DEBUG
#define	rtcache_debug() __predict_false(_rtcache_debug)
#else /* RTFLUSH_DEBUG */
#define	rtcache_debug() 0
#endif /* RTFLUSH_DEBUG */

struct	route_cb route_cb;
struct	rtstat	rtstat;
struct	radix_node_head *rt_tables[AF_MAX+1];

int	rttrash;		/* routes not in table but not freed */
struct	sockaddr wildcard;	/* zero valued cookie for wildcard searches */

POOL_INIT(rtentry_pool, sizeof(struct rtentry), 0, 0, 0, "rtentpl", NULL,
    IPL_SOFTNET);
POOL_INIT(rttimer_pool, sizeof(struct rttimer), 0, 0, 0, "rttmrpl", NULL,
    IPL_SOFTNET);

struct callout rt_timer_ch; /* callout for rt_timer_timer() */

#ifdef RTFLUSH_DEBUG
static int _rtcache_debug = 0;
#endif /* RTFLUSH_DEBUG */

static int rtdeletemsg(struct rtentry *);
static int rtflushclone1(struct rtentry *, void *);
static void rtflushclone(sa_family_t family, struct rtentry *);

#ifdef RTFLUSH_DEBUG
SYSCTL_SETUP(sysctl_net_rtcache_setup, "sysctl net.rtcache.debug setup")
{
	const struct sysctlnode *rnode;

	/* XXX do not duplicate */
	if (sysctl_createv(clog, 0, NULL, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "net", NULL, NULL, 0, NULL, 0, CTL_NET, CTL_EOL) != 0)
		return;
	if (sysctl_createv(clog, 0, &rnode, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE,
	    "rtcache", SYSCTL_DESCR("Route cache related settings"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL) != 0)
		return;
	if (sysctl_createv(clog, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Debug route caches"),
	    NULL, 0, &_rtcache_debug, 0, CTL_CREATE, CTL_EOL) != 0)
		return;
}
#endif /* RTFLUSH_DEBUG */

struct ifaddr *
rt_get_ifa(struct rtentry *rt)
{
	struct ifaddr *ifa;

	if ((ifa = rt->rt_ifa) == NULL)
		return ifa;
	else if (ifa->ifa_getifa == NULL)
		return ifa;
#if 0
	else if (ifa->ifa_seqno != NULL && *ifa->ifa_seqno == rt->rt_ifa_seqno)
		return ifa;
#endif
	else {
		ifa = (*ifa->ifa_getifa)(ifa, rt_key(rt));
		rt_replace_ifa(rt, ifa);
		return ifa;
	}
}

static void
rt_set_ifa1(struct rtentry *rt, struct ifaddr *ifa)
{
	rt->rt_ifa = ifa;
	if (ifa->ifa_seqno != NULL)
		rt->rt_ifa_seqno = *ifa->ifa_seqno;
}

void
rt_replace_ifa(struct rtentry *rt, struct ifaddr *ifa)
{
	IFAREF(ifa);
	IFAFREE(rt->rt_ifa);
	rt_set_ifa1(rt, ifa);
}

static void
rt_set_ifa(struct rtentry *rt, struct ifaddr *ifa)
{
	IFAREF(ifa);
	rt_set_ifa1(rt, ifa);
}

void
rtable_init(void **table)
{
	struct domain *dom;
	DOMAIN_FOREACH(dom)
		if (dom->dom_rtattach)
			dom->dom_rtattach(&table[dom->dom_family],
			    dom->dom_rtoffset);
}

void
route_init(void)
{

	rn_init();	/* initialize all zeroes, all ones, mask table */
	rtable_init((void **)rt_tables);
}

void
rtflushall(int family)
{
	int s;
	struct domain *dom;
	struct route *ro;

	if (rtcache_debug())
		printf("%s: enter\n", __func__);

	if ((dom = pffinddomain(family)) == NULL)
		return;

	s = splnet();
	while ((ro = LIST_FIRST(&dom->dom_rtcache)) != NULL) {
		KASSERT(ro->ro_rt != NULL);
		rtcache_clear(ro);
	}
	splx(s);
}

void
rtflush(struct route *ro)
{
	KASSERT(ro->ro_rt != NULL);
	KASSERT(rtcache_getdst(ro) != NULL);

	RTFREE(ro->ro_rt);
	ro->ro_rt = NULL;

	LIST_REMOVE(ro, ro_rtcache_next);
 
#if 0
	if (rtcache_debug()) {
		printf("%s: flushing %s\n", __func__,
		    inet_ntoa((satocsin(rtcache_getdst(ro)))->sin_addr));
	}
#endif
}

void
rtcache(struct route *ro)
{
	struct domain *dom;

	KASSERT(ro->ro_rt != NULL);
	KASSERT(rtcache_getdst(ro) != NULL);

	if ((dom = pffinddomain(rtcache_getdst(ro)->sa_family)) == NULL)
		return;

	LIST_INSERT_HEAD(&dom->dom_rtcache, ro, ro_rtcache_next);
}

/*
 * Packet routing routines.
 */
void
rtalloc(struct route *ro)
{
	if (ro->ro_rt != NULL) {
		if (ro->ro_rt->rt_ifp != NULL &&
		    (ro->ro_rt->rt_flags & RTF_UP) != 0)
			return;
		rtflush(ro);
	}
	if (rtcache_getdst(ro) == NULL ||
	    (ro->ro_rt = rtalloc1(rtcache_getdst(ro), 1)) == NULL)
		return;
	rtcache(ro);
}

struct rtentry *
rtalloc1(const struct sockaddr *dst, int report)
{
	struct radix_node_head *rnh = rt_tables[dst->sa_family];
	struct rtentry *rt;
	struct radix_node *rn;
	struct rtentry *newrt = NULL;
	struct rt_addrinfo info;
	int  s = splsoftnet(), err = 0, msgtype = RTM_MISS;

	if (rnh && (rn = rnh->rnh_matchaddr(dst, rnh)) &&
	    ((rn->rn_flags & RNF_ROOT) == 0)) {
		newrt = rt = (struct rtentry *)rn;
		if (report && (rt->rt_flags & RTF_CLONING)) {
			err = rtrequest(RTM_RESOLVE, dst, NULL, NULL, 0,
			    &newrt);
			if (err) {
				newrt = rt;
				rt->rt_refcnt++;
				goto miss;
			}
			KASSERT(newrt != NULL);
			if ((rt = newrt) && (rt->rt_flags & RTF_XRESOLVE)) {
				msgtype = RTM_RESOLVE;
				goto miss;
			}
			/* Inform listeners of the new route */
			memset(&info, 0, sizeof(info));
			info.rti_info[RTAX_DST] = rt_key(rt);
			info.rti_info[RTAX_NETMASK] = rt_mask(rt);
			info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
			if (rt->rt_ifp != NULL) {
				info.rti_info[RTAX_IFP] =
				    TAILQ_FIRST(&rt->rt_ifp->if_addrlist)->ifa_addr;
				info.rti_info[RTAX_IFA] = rt->rt_ifa->ifa_addr;
			}
			rt_missmsg(RTM_ADD, &info, rt->rt_flags, 0);
		} else
			rt->rt_refcnt++;
	} else {
		rtstat.rts_unreach++;
	miss:	if (report) {
			memset((void *)&info, 0, sizeof(info));
			info.rti_info[RTAX_DST] = dst;
			rt_missmsg(msgtype, &info, 0, err);
		}
	}
	splx(s);
	return (newrt);
}

void
rtfree(struct rtentry *rt)
{
	struct ifaddr *ifa;

	if (rt == NULL)
		panic("rtfree");
	rt->rt_refcnt--;
	if (rt->rt_refcnt <= 0 && (rt->rt_flags & RTF_UP) == 0) {
		if (rt->rt_nodes->rn_flags & (RNF_ACTIVE | RNF_ROOT))
			panic ("rtfree 2");
		rttrash--;
		if (rt->rt_refcnt < 0) {
			printf("rtfree: %p not freed (neg refs)\n", rt);
			return;
		}
		rt_timer_remove_all(rt, 0);
		ifa = rt->rt_ifa;
		rt->rt_ifa = NULL;
		IFAFREE(ifa);
		rt->rt_ifp = NULL;
		Free(rt_key(rt));
		pool_put(&rtentry_pool, rt);
	}
}

void
ifafree(struct ifaddr *ifa)
{

#ifdef DIAGNOSTIC
	if (ifa == NULL)
		panic("ifafree: null ifa");
	if (ifa->ifa_refcnt != 0)
		panic("ifafree: ifa_refcnt != 0 (%d)", ifa->ifa_refcnt);
#endif
#ifdef IFAREF_DEBUG
	printf("ifafree: freeing ifaddr %p\n", ifa);
#endif
	free(ifa, M_IFADDR);
}

/*
 * Force a routing table entry to the specified
 * destination to go through the given gateway.
 * Normally called as a result of a routing redirect
 * message from the network layer.
 *
 * N.B.: must be called at splsoftnet
 */
void
rtredirect(const struct sockaddr *dst, const struct sockaddr *gateway,
	const struct sockaddr *netmask, int flags, const struct sockaddr *src,
	struct rtentry **rtp)
{
	struct rtentry *rt;
	int error = 0;
	u_quad_t *stat = NULL;
	struct rt_addrinfo info;
	struct ifaddr *ifa;

	/* verify the gateway is directly reachable */
	if ((ifa = ifa_ifwithnet(gateway)) == NULL) {
		error = ENETUNREACH;
		goto out;
	}
	rt = rtalloc1(dst, 0);
	/*
	 * If the redirect isn't from our current router for this dst,
	 * it's either old or wrong.  If it redirects us to ourselves,
	 * we have a routing loop, perhaps as a result of an interface
	 * going down recently.
	 */
#define	equal(a1, a2) \
	((a1)->sa_len == (a2)->sa_len && \
	 memcmp((a1), (a2), (a1)->sa_len) == 0)
	if (!(flags & RTF_DONE) && rt &&
	     (!equal(src, rt->rt_gateway) || rt->rt_ifa != ifa))
		error = EINVAL;
	else if (ifa_ifwithaddr(gateway))
		error = EHOSTUNREACH;
	if (error)
		goto done;
	/*
	 * Create a new entry if we just got back a wildcard entry
	 * or the lookup failed.  This is necessary for hosts
	 * which use routing redirects generated by smart gateways
	 * to dynamically build the routing tables.
	 */
	if ((rt == NULL) || (rt_mask(rt) && rt_mask(rt)->sa_len < 2))
		goto create;
	/*
	 * Don't listen to the redirect if it's
	 * for a route to an interface.
	 */
	if (rt->rt_flags & RTF_GATEWAY) {
		if (((rt->rt_flags & RTF_HOST) == 0) && (flags & RTF_HOST)) {
			/*
			 * Changing from route to net => route to host.
			 * Create new route, rather than smashing route to net.
			 */
		create:
			if (rt)
				rtfree(rt);
			flags |=  RTF_GATEWAY | RTF_DYNAMIC;
			info.rti_info[RTAX_DST] = dst;
			info.rti_info[RTAX_GATEWAY] = gateway;
			info.rti_info[RTAX_NETMASK] = netmask;
			info.rti_ifa = ifa;
			info.rti_flags = flags;
			rt = NULL;
			error = rtrequest1(RTM_ADD, &info, &rt);
			if (rt != NULL)
				flags = rt->rt_flags;
			stat = &rtstat.rts_dynamic;
		} else {
			/*
			 * Smash the current notion of the gateway to
			 * this destination.  Should check about netmask!!!
			 */
			rt->rt_flags |= RTF_MODIFIED;
			flags |= RTF_MODIFIED;
			stat = &rtstat.rts_newgateway;
			rt_setgate(rt, rt_key(rt), gateway);
		}
	} else
		error = EHOSTUNREACH;
done:
	if (rt) {
		if (rtp && !error)
			*rtp = rt;
		else
			rtfree(rt);
	}
out:
	if (error)
		rtstat.rts_badredirect++;
	else if (stat != NULL)
		(*stat)++;
	memset((void *)&info, 0, sizeof(info));
	info.rti_info[RTAX_DST] = dst;
	info.rti_info[RTAX_GATEWAY] = gateway;
	info.rti_info[RTAX_NETMASK] = netmask;
	info.rti_info[RTAX_AUTHOR] = src;
	rt_missmsg(RTM_REDIRECT, &info, flags, error);
}

/*
 * Delete a route and generate a message
 */
static int
rtdeletemsg(struct rtentry *rt)
{
	int error;
	struct rt_addrinfo info;

	/*
	 * Request the new route so that the entry is not actually
	 * deleted.  That will allow the information being reported to
	 * be accurate (and consistent with route_output()).
	 */
	memset((void *)&info, 0, sizeof(info));
	info.rti_info[RTAX_DST] = rt_key(rt);
	info.rti_info[RTAX_NETMASK] = rt_mask(rt);
	info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
	info.rti_flags = rt->rt_flags;
	error = rtrequest1(RTM_DELETE, &info, &rt);

	rt_missmsg(RTM_DELETE, &info, info.rti_flags, error);

	/* Adjust the refcount */
	if (error == 0 && rt->rt_refcnt <= 0) {
		rt->rt_refcnt++;
		rtfree(rt);
	}
	return (error);
}

static int
rtflushclone1(struct rtentry *rt, void *arg)
{
	struct rtentry *parent;

	parent = (struct rtentry *)arg;
	if ((rt->rt_flags & RTF_CLONED) != 0 && rt->rt_parent == parent)
		rtdeletemsg(rt);
	return 0;
}

static void
rtflushclone(sa_family_t family, struct rtentry *parent)
{

#ifdef DIAGNOSTIC
	if (!parent || (parent->rt_flags & RTF_CLONING) == 0)
		panic("rtflushclone: called with a non-cloning route");
#endif
	rt_walktree(family, rtflushclone1, (void *)parent);
}

/*
 * Routing table ioctl interface.
 */
int
rtioctl(u_long req, void *data, struct lwp *l)
{
	return (EOPNOTSUPP);
}

struct ifaddr *
ifa_ifwithroute(int flags, const struct sockaddr *dst,
	const struct sockaddr *gateway)
{
	struct ifaddr *ifa;
	if ((flags & RTF_GATEWAY) == 0) {
		/*
		 * If we are adding a route to an interface,
		 * and the interface is a pt to pt link
		 * we should search for the destination
		 * as our clue to the interface.  Otherwise
		 * we can use the local address.
		 */
		ifa = NULL;
		if (flags & RTF_HOST)
			ifa = ifa_ifwithdstaddr(dst);
		if (ifa == NULL)
			ifa = ifa_ifwithaddr(gateway);
	} else {
		/*
		 * If we are adding a route to a remote net
		 * or host, the gateway may still be on the
		 * other end of a pt to pt link.
		 */
		ifa = ifa_ifwithdstaddr(gateway);
	}
	if (ifa == NULL)
		ifa = ifa_ifwithnet(gateway);
	if (ifa == NULL) {
		struct rtentry *rt = rtalloc1(dst, 0);
		if (rt == NULL)
			return NULL;
		rt->rt_refcnt--;
		if ((ifa = rt->rt_ifa) == NULL)
			return NULL;
	}
	if (ifa->ifa_addr->sa_family != dst->sa_family) {
		struct ifaddr *oifa = ifa;
		ifa = ifaof_ifpforaddr(dst, ifa->ifa_ifp);
		if (ifa == 0)
			ifa = oifa;
	}
	return (ifa);
}

#define ROUNDUP(a) (a>0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

int
rtrequest(int req, const struct sockaddr *dst, const struct sockaddr *gateway,
	const struct sockaddr *netmask, int flags, struct rtentry **ret_nrt)
{
	struct rt_addrinfo info;

	memset(&info, 0, sizeof(info));
	info.rti_flags = flags;
	info.rti_info[RTAX_DST] = dst;
	info.rti_info[RTAX_GATEWAY] = gateway;
	info.rti_info[RTAX_NETMASK] = netmask;
	return rtrequest1(req, &info, ret_nrt);
}

int
rt_getifa(struct rt_addrinfo *info)
{
	struct ifaddr *ifa;
	const struct sockaddr *dst = info->rti_info[RTAX_DST];
	const struct sockaddr *gateway = info->rti_info[RTAX_GATEWAY];
	const struct sockaddr *ifaaddr = info->rti_info[RTAX_IFA];
	const struct sockaddr *ifpaddr = info->rti_info[RTAX_IFP];
	int flags = info->rti_flags;

	/*
	 * ifp may be specified by sockaddr_dl when protocol address
	 * is ambiguous
	 */
	if (info->rti_ifp == NULL && ifpaddr != NULL
	    && ifpaddr->sa_family == AF_LINK &&
	    (ifa = ifa_ifwithnet((const struct sockaddr *)ifpaddr)) != NULL)
		info->rti_ifp = ifa->ifa_ifp;
	if (info->rti_ifa == NULL && ifaaddr != NULL)
		info->rti_ifa = ifa_ifwithaddr(ifaaddr);
	if (info->rti_ifa == NULL) {
		const struct sockaddr *sa;

		sa = ifaaddr != NULL ? ifaaddr :
		    (gateway != NULL ? gateway : dst);
		if (sa != NULL && info->rti_ifp != NULL)
			info->rti_ifa = ifaof_ifpforaddr(sa, info->rti_ifp);
		else if (dst != NULL && gateway != NULL)
			info->rti_ifa = ifa_ifwithroute(flags, dst, gateway);
		else if (sa != NULL)
			info->rti_ifa = ifa_ifwithroute(flags, sa, sa);
	}
	if ((ifa = info->rti_ifa) == NULL)
		return ENETUNREACH;
	if (ifa->ifa_getifa != NULL)
		info->rti_ifa = ifa = (*ifa->ifa_getifa)(ifa, dst);
	if (info->rti_ifp == NULL)
		info->rti_ifp = ifa->ifa_ifp;
	return 0;
}

int
rtrequest1(int req, struct rt_addrinfo *info, struct rtentry **ret_nrt)
{
	int s = splsoftnet();
	int error = 0;
	struct rtentry *rt, *crt;
	struct radix_node *rn;
	struct radix_node_head *rnh;
	struct ifaddr *ifa;
	struct sockaddr_storage deldst;
	const struct sockaddr *dst = info->rti_info[RTAX_DST];
	const struct sockaddr *gateway = info->rti_info[RTAX_GATEWAY];
	const struct sockaddr *netmask = info->rti_info[RTAX_NETMASK];
	int flags = info->rti_flags;
#define senderr(x) { error = x ; goto bad; }

	if ((rnh = rt_tables[dst->sa_family]) == NULL)
		senderr(ESRCH);
	if (flags & RTF_HOST)
		netmask = NULL;
	switch (req) {
	case RTM_DELETE:
		if (netmask) {
			rt_maskedcopy(dst, (struct sockaddr *)&deldst, netmask);
			dst = (struct sockaddr *)&deldst;
		}
		if ((rn = rnh->rnh_lookup(dst, netmask, rnh)) == NULL)
			senderr(ESRCH);
		rt = (struct rtentry *)rn;
		if ((rt->rt_flags & RTF_CLONING) != 0) {
			/* clean up any cloned children */
			rtflushclone(dst->sa_family, rt);
		}
		if ((rn = rnh->rnh_deladdr(dst, netmask, rnh)) == NULL)
			senderr(ESRCH);
		if (rn->rn_flags & (RNF_ACTIVE | RNF_ROOT))
			panic ("rtrequest delete");
		rt = (struct rtentry *)rn;
		if (rt->rt_gwroute) {
			RTFREE(rt->rt_gwroute);
			rt->rt_gwroute = NULL;
		}
		if (rt->rt_parent) {
			rt->rt_parent->rt_refcnt--;
			rt->rt_parent = NULL;
		}
		rt->rt_flags &= ~RTF_UP;
		if ((ifa = rt->rt_ifa) && ifa->ifa_rtrequest)
			ifa->ifa_rtrequest(RTM_DELETE, rt, info);
		rttrash++;
		if (ret_nrt)
			*ret_nrt = rt;
		else if (rt->rt_refcnt <= 0) {
			rt->rt_refcnt++;
			rtfree(rt);
		}
		break;

	case RTM_RESOLVE:
		if (ret_nrt == NULL || (rt = *ret_nrt) == NULL)
			senderr(EINVAL);
		if ((rt->rt_flags & RTF_CLONING) == 0)
			senderr(EINVAL);
		ifa = rt->rt_ifa;
		flags = rt->rt_flags & ~(RTF_CLONING | RTF_STATIC);
		flags |= RTF_CLONED;
		gateway = rt->rt_gateway;
		if ((netmask = rt->rt_genmask) == NULL)
			flags |= RTF_HOST;
		goto makeroute;

	case RTM_ADD:
		if (info->rti_ifa == NULL && (error = rt_getifa(info)))
			senderr(error);
		ifa = info->rti_ifa;
	makeroute:
		/* Already at splsoftnet() so pool_get/pool_put are safe */
		rt = pool_get(&rtentry_pool, PR_NOWAIT);
		if (rt == NULL)
			senderr(ENOBUFS);
		Bzero(rt, sizeof(*rt));
		rt->rt_flags = RTF_UP | flags;
		LIST_INIT(&rt->rt_timer);
		if (rt_setgate(rt, dst, gateway)) {
			pool_put(&rtentry_pool, rt);
			senderr(ENOBUFS);
		}
		if (netmask) {
			rt_maskedcopy(dst, rt_key(rt), netmask);
		} else
			Bcopy(dst, rt_key(rt), dst->sa_len);
		rt_set_ifa(rt, ifa);
		rt->rt_ifp = ifa->ifa_ifp;
		if (req == RTM_RESOLVE) {
			rt->rt_rmx = (*ret_nrt)->rt_rmx; /* copy metrics */
			rt->rt_parent = *ret_nrt;
			rt->rt_parent->rt_refcnt++;
		}
		rn = rnh->rnh_addaddr(rt_key(rt), netmask, rnh, rt->rt_nodes);
		if (rn == NULL && (crt = rtalloc1(rt_key(rt), 0)) != NULL) {
			/* overwrite cloned route */
			if ((crt->rt_flags & RTF_CLONED) != 0) {
				rtdeletemsg(crt);
				rn = rnh->rnh_addaddr(rt_key(rt),
				    netmask, rnh, rt->rt_nodes);
			}
			RTFREE(crt);
		}
		if (rn == NULL) {
			IFAFREE(ifa);
			if ((rt->rt_flags & RTF_CLONED) != 0 && rt->rt_parent)
				rtfree(rt->rt_parent);
			if (rt->rt_gwroute)
				rtfree(rt->rt_gwroute);
			Free(rt_key(rt));
			pool_put(&rtentry_pool, rt);
			senderr(EEXIST);
		}
		if (ifa->ifa_rtrequest)
			ifa->ifa_rtrequest(req, rt, info);
		if (ret_nrt) {
			*ret_nrt = rt;
			rt->rt_refcnt++;
		}
		if ((rt->rt_flags & RTF_CLONING) != 0) {
			/* clean up any cloned children */
			rtflushclone(dst->sa_family, rt);
		}
		rtflushall(dst->sa_family);
		break;
	case RTM_GET:
		rn = rnh->rnh_lookup(dst, netmask, rnh);
		if (rn == NULL || (rn->rn_flags & RNF_ROOT) != 0)
			senderr(ESRCH);
		if (ret_nrt != NULL) {
			rt = (struct rtentry *)rn;
			*ret_nrt = rt;
			rt->rt_refcnt++;
		}
		break;
	}
bad:
	splx(s);
	return (error);
}

int
rt_setgate( struct rtentry *rt0, const struct sockaddr *dst,
	const struct sockaddr *gate)
{
	char *new, *old;
	u_int dlen = ROUNDUP(dst->sa_len), glen = ROUNDUP(gate->sa_len);
	struct rtentry *rt = rt0;

	if (rt->rt_gateway == NULL || glen > ROUNDUP(rt->rt_gateway->sa_len)) {
		old = (void *)rt_key(rt);
		R_Malloc(new, void *, dlen + glen);
		if (new == NULL)
			return 1;
		Bzero(new, dlen + glen);
		rt->rt_nodes->rn_key = new;
	} else {
		new = __UNCONST(rt->rt_nodes->rn_key); /*XXXUNCONST*/
		old = NULL;
	}
	Bcopy(gate, (rt->rt_gateway = (struct sockaddr *)(new + dlen)), glen);
	if (old) {
		Bcopy(dst, new, dlen);
		Free(old);
	}
	if (rt->rt_gwroute) {
		RTFREE(rt->rt_gwroute);
		rt->rt_gwroute = NULL;
	}
	if (rt->rt_flags & RTF_GATEWAY) {
		rt->rt_gwroute = rtalloc1(gate, 1);
		/*
		 * If we switched gateways, grab the MTU from the new
		 * gateway route if the current MTU, if the current MTU is
		 * greater than the MTU of gateway.
		 * Note that, if the MTU of gateway is 0, we will reset the
		 * MTU of the route to run PMTUD again from scratch. XXX
		 */
		if (rt->rt_gwroute
		    && !(rt->rt_rmx.rmx_locks & RTV_MTU)
		    && rt->rt_rmx.rmx_mtu
		    && rt->rt_rmx.rmx_mtu > rt->rt_gwroute->rt_rmx.rmx_mtu) {
			rt->rt_rmx.rmx_mtu = rt->rt_gwroute->rt_rmx.rmx_mtu;
		}
	}
	return 0;
}

void
rt_maskedcopy(const struct sockaddr *src, struct sockaddr *dst,
	const struct sockaddr *netmask)
{
	const u_char *cp1 = (const u_char *)src;
	u_char *cp2 = (u_char *)dst;
	const u_char *cp3 = (const u_char *)netmask;
	u_char *cplim = cp2 + *cp3;
	u_char *cplim2 = cp2 + *cp1;

	*cp2++ = *cp1++; *cp2++ = *cp1++; /* copies sa_len & sa_family */
	cp3 += 2;
	if (cplim > cplim2)
		cplim = cplim2;
	while (cp2 < cplim)
		*cp2++ = *cp1++ & *cp3++;
	if (cp2 < cplim2)
		memset(cp2, 0, (unsigned)(cplim2 - cp2));
}

/*
 * Set up or tear down a routing table entry, normally
 * for an interface.
 */
int
rtinit(struct ifaddr *ifa, int cmd, int flags)
{
	struct rtentry *rt;
	struct sockaddr *dst, *odst;
	struct sockaddr_storage deldst;
	struct rtentry *nrt = NULL;
	int error;
	struct rt_addrinfo info;

	dst = flags & RTF_HOST ? ifa->ifa_dstaddr : ifa->ifa_addr;
	if (cmd == RTM_DELETE) {
		if ((flags & RTF_HOST) == 0 && ifa->ifa_netmask) {
			/* Delete subnet route for this interface */
			odst = dst;
			dst = (struct sockaddr *)&deldst;
			rt_maskedcopy(odst, dst, ifa->ifa_netmask);
		}
		if ((rt = rtalloc1(dst, 0)) != NULL) {
			rt->rt_refcnt--;
			if (rt->rt_ifa != ifa)
				return (flags & RTF_HOST) ? EHOSTUNREACH
							: ENETUNREACH;
		}
	}
	memset(&info, 0, sizeof(info));
	info.rti_ifa = ifa;
	info.rti_flags = flags | ifa->ifa_flags;
	info.rti_info[RTAX_DST] = dst;
	info.rti_info[RTAX_GATEWAY] = ifa->ifa_addr;
	/*
	 * XXX here, it seems that we are assuming that ifa_netmask is NULL
	 * for RTF_HOST.  bsdi4 passes NULL explicitly (via intermediate
	 * variable) when RTF_HOST is 1.  still not sure if i can safely
	 * change it to meet bsdi4 behavior.
	 */
	info.rti_info[RTAX_NETMASK] = ifa->ifa_netmask;
	error = rtrequest1(cmd, &info, &nrt);
	if (cmd == RTM_DELETE && error == 0 && (rt = nrt)) {
		rt_newaddrmsg(cmd, ifa, error, nrt);
		if (rt->rt_refcnt <= 0) {
			rt->rt_refcnt++;
			rtfree(rt);
		}
	}
	if (cmd == RTM_ADD && error == 0 && (rt = nrt)) {
		rt->rt_refcnt--;
		if (rt->rt_ifa != ifa) {
			printf("rtinit: wrong ifa (%p) was (%p)\n", ifa,
				rt->rt_ifa);
			if (rt->rt_ifa->ifa_rtrequest)
				rt->rt_ifa->ifa_rtrequest(RTM_DELETE, rt, NULL);
			rt_replace_ifa(rt, ifa);
			rt->rt_ifp = ifa->ifa_ifp;
			if (ifa->ifa_rtrequest)
				ifa->ifa_rtrequest(RTM_ADD, rt, NULL);
		}
		rt_newaddrmsg(cmd, ifa, error, nrt);
	}
	return error;
}

/*
 * Route timer routines.  These routes allow functions to be called
 * for various routes at any time.  This is useful in supporting
 * path MTU discovery and redirect route deletion.
 *
 * This is similar to some BSDI internal functions, but it provides
 * for multiple queues for efficiency's sake...
 */

LIST_HEAD(, rttimer_queue) rttimer_queue_head;
static int rt_init_done = 0;

#define RTTIMER_CALLOUT(r)	do {					\
		if (r->rtt_func != NULL) {				\
			(*r->rtt_func)(r->rtt_rt, r);			\
		} else {						\
			rtrequest((int) RTM_DELETE,			\
				  (struct sockaddr *)rt_key(r->rtt_rt),	\
				  0, 0, 0, 0);				\
		}							\
	} while (/*CONSTCOND*/0)

/*
 * Some subtle order problems with domain initialization mean that
 * we cannot count on this being run from rt_init before various
 * protocol initializations are done.  Therefore, we make sure
 * that this is run when the first queue is added...
 */

void
rt_timer_init(void)
{
	assert(rt_init_done == 0);

	LIST_INIT(&rttimer_queue_head);
	callout_init(&rt_timer_ch, 0);
	callout_reset(&rt_timer_ch, hz, rt_timer_timer, NULL);
	rt_init_done = 1;
}

struct rttimer_queue *
rt_timer_queue_create(u_int timeout)
{
	struct rttimer_queue *rtq;

	if (rt_init_done == 0)
		rt_timer_init();

	R_Malloc(rtq, struct rttimer_queue *, sizeof *rtq);
	if (rtq == NULL)
		return NULL;
	Bzero(rtq, sizeof *rtq);

	rtq->rtq_timeout = timeout;
	rtq->rtq_count = 0;
	TAILQ_INIT(&rtq->rtq_head);
	LIST_INSERT_HEAD(&rttimer_queue_head, rtq, rtq_link);

	return rtq;
}

void
rt_timer_queue_change(struct rttimer_queue *rtq, long timeout)
{

	rtq->rtq_timeout = timeout;
}

void
rt_timer_queue_remove_all(struct rttimer_queue *rtq, int destroy)
{
	struct rttimer *r;

	while ((r = TAILQ_FIRST(&rtq->rtq_head)) != NULL) {
		LIST_REMOVE(r, rtt_link);
		TAILQ_REMOVE(&rtq->rtq_head, r, rtt_next);
		if (destroy)
			RTTIMER_CALLOUT(r);
		/* we are already at splsoftnet */
		pool_put(&rttimer_pool, r);
		if (rtq->rtq_count > 0)
			rtq->rtq_count--;
		else
			printf("rt_timer_queue_remove_all: "
			    "rtq_count reached 0\n");
	}
}

void
rt_timer_queue_destroy(struct rttimer_queue *rtq, int destroy)
{

	rt_timer_queue_remove_all(rtq, destroy);

	LIST_REMOVE(rtq, rtq_link);

	/*
	 * Caller is responsible for freeing the rttimer_queue structure.
	 */
}

unsigned long
rt_timer_count(struct rttimer_queue *rtq)
{
	return rtq->rtq_count;
}

void
rt_timer_remove_all(struct rtentry *rt, int destroy)
{
	struct rttimer *r;

	while ((r = LIST_FIRST(&rt->rt_timer)) != NULL) {
		LIST_REMOVE(r, rtt_link);
		TAILQ_REMOVE(&r->rtt_queue->rtq_head, r, rtt_next);
		if (destroy)
			RTTIMER_CALLOUT(r);
		if (r->rtt_queue->rtq_count > 0)
			r->rtt_queue->rtq_count--;
		else
			printf("rt_timer_remove_all: rtq_count reached 0\n");
		/* we are already at splsoftnet */
		pool_put(&rttimer_pool, r);
	}
}

int
rt_timer_add(struct rtentry *rt,
	void (*func)(struct rtentry *, struct rttimer *),
	struct rttimer_queue *queue)
{
	struct rttimer *r;
	int s;

	/*
	 * If there's already a timer with this action, destroy it before
	 * we add a new one.
	 */
	LIST_FOREACH(r, &rt->rt_timer, rtt_link) {
		if (r->rtt_func == func)
			break;
	}
	if (r != NULL) {
		LIST_REMOVE(r, rtt_link);
		TAILQ_REMOVE(&r->rtt_queue->rtq_head, r, rtt_next);
		if (r->rtt_queue->rtq_count > 0)
			r->rtt_queue->rtq_count--;
		else
			printf("rt_timer_add: rtq_count reached 0\n");
	} else {
		s = splsoftnet();
		r = pool_get(&rttimer_pool, PR_NOWAIT);
		splx(s);
		if (r == NULL)
			return ENOBUFS;
	}

	memset(r, 0, sizeof(*r));

	r->rtt_rt = rt;
	r->rtt_time = time_uptime;
	r->rtt_func = func;
	r->rtt_queue = queue;
	LIST_INSERT_HEAD(&rt->rt_timer, r, rtt_link);
	TAILQ_INSERT_TAIL(&queue->rtq_head, r, rtt_next);
	r->rtt_queue->rtq_count++;

	return (0);
}

/* ARGSUSED */
void
rt_timer_timer(void *arg)
{
	struct rttimer_queue *rtq;
	struct rttimer *r;
	int s;

	s = splsoftnet();
	LIST_FOREACH(rtq, &rttimer_queue_head, rtq_link) {
		while ((r = TAILQ_FIRST(&rtq->rtq_head)) != NULL &&
		    (r->rtt_time + rtq->rtq_timeout) < time_uptime) {
			LIST_REMOVE(r, rtt_link);
			TAILQ_REMOVE(&rtq->rtq_head, r, rtt_next);
			RTTIMER_CALLOUT(r);
			pool_put(&rttimer_pool, r);
			if (rtq->rtq_count > 0)
				rtq->rtq_count--;
			else
				printf("rt_timer_timer: rtq_count reached 0\n");
		}
	}
	splx(s);

	callout_reset(&rt_timer_ch, hz, rt_timer_timer, NULL);
}

#ifdef RTCACHE_DEBUG
#ifndef	RTCACHE_DEBUG_SIZE 
#define	RTCACHE_DEBUG_SIZE (1024 * 1024)
#endif
static const char *cache_caller[RTCACHE_DEBUG_SIZE];
static struct route *cache_entry[RTCACHE_DEBUG_SIZE];
size_t cache_cur;
#endif

#ifdef RTCACHE_DEBUG
static void
_rtcache_init_debug(const char *caller, struct route *ro, int flag)
#else
static void
_rtcache_init(struct route *ro, int flag)
#endif
{
#ifdef RTCACHE_DEBUG
	size_t i;
	for (i = 0; i < cache_cur; ++i) {
		if (cache_entry[i] == ro)
			panic("Reinit of route %p, initialised from %s", ro, cache_caller[i]);
	}
#endif

	if (rtcache_getdst(ro) == NULL)
		return;
	ro->ro_rt = rtalloc1(rtcache_getdst(ro), flag);
	if (ro->ro_rt != NULL) {
#ifdef RTCACHE_DEBUG
		if (cache_cur == RTCACHE_DEBUG_SIZE)
			panic("Route cache debug overflow");
		cache_caller[cache_cur] = caller;
		cache_entry[cache_cur] = ro;
		++cache_cur;
#endif
		rtcache(ro);
	}
}

#ifdef RTCACHE_DEBUG
void
rtcache_init_debug(const char *caller, struct route *ro)
{
	_rtcache_init_debug(caller, ro, 1);
}

void
rtcache_init_noclone_debug(const char *caller, struct route *ro)
{
	_rtcache_init_debug(caller, ro, 0);
}

void
rtcache_update(struct route *ro, int clone)
{
	rtcache_clear(ro);
	_rtcache_init_debug(__func__, ro, clone);
}
#else
void
rtcache_init(struct route *ro)
{
	_rtcache_init(ro, 1);
}

void
rtcache_init_noclone(struct route *ro)
{
	_rtcache_init(ro, 0);
}

void
rtcache_update(struct route *ro, int clone)
{
	rtcache_clear(ro);
	_rtcache_init(ro, clone);
}
#endif

#ifdef RTCACHE_DEBUG
void
rtcache_copy_debug(const char *caller, struct route *new_ro, const struct route *old_ro)
#else
void
rtcache_copy(struct route *new_ro, const struct route *old_ro)
#endif
{
	/* XXX i doubt this DTRT any longer --dyoung */
#ifdef RTCACHE_DEBUG
	size_t i;

	for (i = 0; i < cache_cur; ++i) {
		if (cache_entry[i] == new_ro)
			panic("Copy to initalised route %p (before %s)", new_ro, cache_caller[i]);
	}
#endif

	if (rtcache_getdst(old_ro) == NULL ||
	    rtcache_setdst(new_ro, rtcache_getdst(old_ro)) != 0)
		return;
	new_ro->ro_rt = old_ro->ro_rt;
	if (new_ro->ro_rt != NULL) {
#ifdef RTCACHE_DEBUG
		if (cache_cur == RTCACHE_DEBUG_SIZE)
			panic("Route cache debug overflow");
		cache_caller[cache_cur] = caller;
		cache_entry[cache_cur] = new_ro;
		++cache_cur;
#endif
		rtcache(new_ro);
		++new_ro->ro_rt->rt_refcnt;
	}
}

void
rtcache_clear(struct route *ro)
{
#ifdef RTCACHE_DEBUG
	size_t j, i = cache_cur;
	for (i = j = 0; i < cache_cur; ++i, ++j) {
		if (cache_entry[i] == ro) {
			if (ro->ro_rt == NULL)
				panic("Route cache manipulated (allocated by %s)", cache_caller[i]);
			--j;
		} else {
			cache_caller[j] = cache_caller[i];
			cache_entry[j] = cache_entry[i];
		}
	}
	if (ro->ro_rt != NULL) {
		if (i != j + 1)
			panic("Wrong entries after rtcache_free: %zu (expected %zu)", j, i - 1);
		--cache_cur;
	}
#endif

	if (ro->ro_rt != NULL)
		rtflush(ro);
	ro->ro_rt = NULL;
}

struct rtentry *
rtcache_lookup2(struct route *ro, const struct sockaddr *dst, int clone,
    int *hitp)
{
	const struct sockaddr *odst;

	odst = rtcache_getdst(ro);

	if (odst == NULL)
		;
	else if (sockaddr_cmp(odst, dst) != 0)
		rtcache_free(ro);
	else if (rtcache_down(ro))
		rtcache_clear(ro);

	if (ro->ro_rt == NULL) {
		*hitp = 0;
		rtcache_setdst(ro, dst);
		_rtcache_init(ro, clone);
	} else
		*hitp = 1;

	return ro->ro_rt;
}

void
rtcache_free(struct route *ro)
{
	rtcache_clear(ro);
	if (ro->ro_sa != NULL) {
		sockaddr_free(ro->ro_sa);
		ro->ro_sa = NULL;
	}
}

int
rtcache_setdst(struct route *ro, const struct sockaddr *sa)
{
	KASSERT(sa != NULL);

	if (ro->ro_sa != NULL && ro->ro_sa->sa_family == sa->sa_family) {
		rtcache_clear(ro);
		sockaddr_copy(ro->ro_sa, sa);
		return 0;
	} else if (ro->ro_sa != NULL)
		rtcache_free(ro);	/* free ro_sa, wrong family */

	if ((ro->ro_sa = sockaddr_dup(sa, PR_NOWAIT)) == NULL)
		return ENOMEM;
	return 0;
}

static int
rt_walktree_visitor(struct radix_node *rn, void *v)
{
	struct rtwalk *rw = (struct rtwalk *)v;

	return (*rw->rw_f)((struct rtentry *)rn, rw->rw_v);
}

int
rt_walktree(sa_family_t family, int (*f)(struct rtentry *, void *), void *v)
{
	struct radix_node_head *rnh = rt_tables[family];
	struct rtwalk rw;

	if (rnh == NULL)
		return 0;

	rw.rw_f = f;
	rw.rw_v = v;

	return rn_walktree(rnh, rt_walktree_visitor, &rw);
}
