/*	$NetBSD: route.c,v 1.33 2000/03/13 23:52:39 soren Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#include "opt_ns.h"

#include <sys/param.h>
#include <sys/systm.h>
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

#ifdef NS
#include <netns/ns.h>
#endif

#define	SA(p) ((struct sockaddr *)(p))

int	rttrash;		/* routes not in table but not freed */
struct	sockaddr wildcard;	/* zero valued cookie for wildcard searches */

struct pool rtentry_pool;	/* pool for rtentry structures */
struct pool rttimer_pool;	/* pool for rttimer structures */

void
rtable_init(table)
	void **table;
{
	struct domain *dom;
	for (dom = domains; dom; dom = dom->dom_next)
		if (dom->dom_rtattach)
			dom->dom_rtattach(&table[dom->dom_family],
			    dom->dom_rtoffset);
}

void
route_init()
{

	pool_init(&rtentry_pool, sizeof(struct rtentry), 0, 0, 0, "rtentpl",
	    0, NULL, NULL, M_RTABLE);

	rn_init();	/* initialize all zeroes, all ones, mask table */
	rtable_init((void **)rt_tables);
}

/*
 * Packet routing routines.
 */
void
rtalloc(ro)
	register struct route *ro;
{
	if (ro->ro_rt && ro->ro_rt->rt_ifp && (ro->ro_rt->rt_flags & RTF_UP))
		return;				 /* XXX */
	ro->ro_rt = rtalloc1(&ro->ro_dst, 1);
}

struct rtentry *
rtalloc1(dst, report)
	register struct sockaddr *dst;
	int report;
{
	register struct radix_node_head *rnh = rt_tables[dst->sa_family];
	register struct rtentry *rt;
	register struct radix_node *rn;
	struct rtentry *newrt = 0;
	struct rt_addrinfo info;
	int  s = splsoftnet(), err = 0, msgtype = RTM_MISS;

	if (rnh && (rn = rnh->rnh_matchaddr((caddr_t)dst, rnh)) &&
	    ((rn->rn_flags & RNF_ROOT) == 0)) {
		newrt = rt = (struct rtentry *)rn;
		if (report && (rt->rt_flags & RTF_CLONING)) {
			err = rtrequest(RTM_RESOLVE, dst, SA(0),
					      SA(0), 0, &newrt);
			if (err) {
				newrt = rt;
				rt->rt_refcnt++;
				goto miss;
			}
			if ((rt = newrt) && (rt->rt_flags & RTF_XRESOLVE)) {
				msgtype = RTM_RESOLVE;
				goto miss;
			}
		} else
			rt->rt_refcnt++;
	} else {
		rtstat.rts_unreach++;
	miss:	if (report) {
			bzero((caddr_t)&info, sizeof(info));
			info.rti_info[RTAX_DST] = dst;
			rt_missmsg(msgtype, &info, 0, err);
		}
	}
	splx(s);
	return (newrt);
}

void
rtfree(rt)
	register struct rtentry *rt;
{
	register struct ifaddr *ifa;

	if (rt == 0)
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
		rt_timer_remove_all(rt);
		ifa = rt->rt_ifa;
		IFAFREE(ifa);
		Free(rt_key(rt));
		pool_put(&rtentry_pool, rt);
	}
}

void
ifafree(ifa)
	register struct ifaddr *ifa;
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
rtredirect(dst, gateway, netmask, flags, src, rtp)
	struct sockaddr *dst, *gateway, *netmask, *src;
	int flags;
	struct rtentry **rtp;
{
	register struct rtentry *rt;
	int error = 0;
	short *stat = 0;
	struct rt_addrinfo info;
	struct ifaddr *ifa;

	/* verify the gateway is directly reachable */
	if ((ifa = ifa_ifwithnet(gateway)) == 0) {
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
#define	equal(a1, a2) (bcmp((caddr_t)(a1), (caddr_t)(a2), (a1)->sa_len) == 0)
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
	if ((rt == 0) || (rt_mask(rt) && rt_mask(rt)->sa_len < 2))
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
			flags |=  RTF_GATEWAY | RTF_DYNAMIC;
			error = rtrequest((int)RTM_ADD, dst, gateway,
				    netmask, flags,
				    (struct rtentry **)0);
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
	bzero((caddr_t)&info, sizeof(info));
	info.rti_info[RTAX_DST] = dst;
	info.rti_info[RTAX_GATEWAY] = gateway;
	info.rti_info[RTAX_NETMASK] = netmask;
	info.rti_info[RTAX_AUTHOR] = src;
	rt_missmsg(RTM_REDIRECT, &info, flags, error);
}

/*
 * Routing table ioctl interface.
 */
int
rtioctl(req, data, p)
	u_long req;
	caddr_t data;
	struct proc *p;
{
	return (EOPNOTSUPP);
}

struct ifaddr *
ifa_ifwithroute(flags, dst, gateway)
	int flags;
	struct sockaddr	*dst, *gateway;
{
	register struct ifaddr *ifa;
	if ((flags & RTF_GATEWAY) == 0) {
		/*
		 * If we are adding a route to an interface,
		 * and the interface is a pt to pt link
		 * we should search for the destination
		 * as our clue to the interface.  Otherwise
		 * we can use the local address.
		 */
		ifa = 0;
		if (flags & RTF_HOST) 
			ifa = ifa_ifwithdstaddr(dst);
		if (ifa == 0)
			ifa = ifa_ifwithaddr(gateway);
	} else {
		/*
		 * If we are adding a route to a remote net
		 * or host, the gateway may still be on the
		 * other end of a pt to pt link.
		 */
		ifa = ifa_ifwithdstaddr(gateway);
	}
	if (ifa == 0)
		ifa = ifa_ifwithnet(gateway);
	if (ifa == 0) {
		struct rtentry *rt = rtalloc1(dst, 0);
		if (rt == 0)
			return (0);
		rt->rt_refcnt--;
		if ((ifa = rt->rt_ifa) == 0)
			return (0);
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
rtrequest(req, dst, gateway, netmask, flags, ret_nrt)
	int req, flags;
	struct sockaddr *dst, *gateway, *netmask;
	struct rtentry **ret_nrt;
{
	int s = splsoftnet(); int error = 0;
	register struct rtentry *rt;
	register struct radix_node *rn;
	register struct radix_node_head *rnh;
	struct ifaddr *ifa;
	struct sockaddr *ndst;
#define senderr(x) { error = x ; goto bad; }

	if ((rnh = rt_tables[dst->sa_family]) == 0)
		senderr(ESRCH);
	if (flags & RTF_HOST)
		netmask = 0;
	switch (req) {
	case RTM_DELETE:
		if ((rn = rnh->rnh_deladdr(dst, netmask, rnh)) == 0)
			senderr(ESRCH);
		if (rn->rn_flags & (RNF_ACTIVE | RNF_ROOT))
			panic ("rtrequest delete");
		rt = (struct rtentry *)rn;
		if (rt->rt_gwroute) {
			rt = rt->rt_gwroute; RTFREE(rt);
			(rt = (struct rtentry *)rn)->rt_gwroute = 0;
		}
		rt->rt_flags &= ~RTF_UP;
		if ((ifa = rt->rt_ifa) && ifa->ifa_rtrequest)
			ifa->ifa_rtrequest(RTM_DELETE, rt, SA(0));
		rttrash++;
		if (ret_nrt)
			*ret_nrt = rt;
		else if (rt->rt_refcnt <= 0) {
			rt->rt_refcnt++;
			rtfree(rt);
		}
		break;

	case RTM_RESOLVE:
		if (ret_nrt == 0 || (rt = *ret_nrt) == 0)
			senderr(EINVAL);
		ifa = rt->rt_ifa;
		flags = rt->rt_flags & ~RTF_CLONING;
		gateway = rt->rt_gateway;
		if ((netmask = rt->rt_genmask) == 0)
			flags |= RTF_HOST;
		goto makeroute;

	case RTM_ADD:
		if ((ifa = ifa_ifwithroute(flags, dst, gateway)) == 0)
			senderr(ENETUNREACH);

		/* The interface found in the previous statement may
		 * be overridden later by rt_setif.  See the code
		 * for case RTM_ADD in rtsock.c:route_output.
		 */
	makeroute:
		rt = pool_get(&rtentry_pool, PR_NOWAIT);
		if (rt == 0)
			senderr(ENOBUFS);
		Bzero(rt, sizeof(*rt));
		rt->rt_flags = RTF_UP | flags;
		LIST_INIT(&rt->rt_timer);
		if (rt_setgate(rt, dst, gateway)) {
			pool_put(&rtentry_pool, rt);
			senderr(ENOBUFS);
		}
		ndst = rt_key(rt);
		if (netmask) {
			rt_maskedcopy(dst, ndst, netmask);
		} else
			Bcopy(dst, ndst, dst->sa_len);
		rn = rnh->rnh_addaddr((caddr_t)ndst, (caddr_t)netmask,
					rnh, rt->rt_nodes);
		if (rn == 0) {
			if (rt->rt_gwroute)
				rtfree(rt->rt_gwroute);
			Free(rt_key(rt));
			pool_put(&rtentry_pool, rt);
			senderr(EEXIST);
		}
		IFAREF(ifa);
		rt->rt_ifa = ifa;
		rt->rt_ifp = ifa->ifa_ifp;
		if (req == RTM_RESOLVE) {
			rt->rt_rmx = (*ret_nrt)->rt_rmx; /* copy metrics */
		} else if (rt->rt_rmx.rmx_mtu == 0
			    && !(rt->rt_rmx.rmx_locks & RTV_MTU)) { /* XXX */
			if (rt->rt_gwroute != NULL) {
				rt->rt_rmx.rmx_mtu = rt->rt_gwroute->rt_rmx.rmx_mtu;
			} else {
				rt->rt_rmx.rmx_mtu = ifa->ifa_ifp->if_mtu;
			}
		}
		if (ifa->ifa_rtrequest)
			ifa->ifa_rtrequest(req, rt, SA(ret_nrt ? *ret_nrt : 0));
		if (ret_nrt) {
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
rt_setgate(rt0, dst, gate)
	struct rtentry *rt0;
	struct sockaddr *dst, *gate;
{
	caddr_t new, old;
	int dlen = ROUNDUP(dst->sa_len), glen = ROUNDUP(gate->sa_len);
	register struct rtentry *rt = rt0;

	if (rt->rt_gateway == 0 || glen > ROUNDUP(rt->rt_gateway->sa_len)) {
		old = (caddr_t)rt_key(rt);
		R_Malloc(new, caddr_t, dlen + glen);
		if (new == 0)
			return 1;
		Bzero(new, dlen + glen);
		rt->rt_nodes->rn_key = new;
	} else {
		new = rt->rt_nodes->rn_key;
		old = 0;
	}
	Bcopy(gate, (rt->rt_gateway = (struct sockaddr *)(new + dlen)), glen);
	if (old) {
		Bcopy(dst, new, dlen);
		Free(old);
	}
	if (rt->rt_gwroute) {
		rt = rt->rt_gwroute; RTFREE(rt);
		rt = rt0; rt->rt_gwroute = 0;
	}
	if (rt->rt_flags & RTF_GATEWAY) {
		rt->rt_gwroute = rtalloc1(gate, 1);
		/*
		 * If we switched gateways, grab the MTU from the new
		 * gateway route if the current MTU is 0 or greater
		 * than the MTU of gateway.
		 */
		if (rt->rt_gwroute
		    && !(rt->rt_rmx.rmx_locks & RTV_MTU)
		    && (rt->rt_rmx.rmx_mtu == 0
		    || rt->rt_rmx.rmx_mtu > rt->rt_gwroute->rt_rmx.rmx_mtu)) { /* XXX */
			rt->rt_rmx.rmx_mtu = rt->rt_gwroute->rt_rmx.rmx_mtu;
		}
	}
	return 0;
}

void
rt_maskedcopy(src, dst, netmask)
	struct sockaddr *src, *dst, *netmask;
{
	register u_char *cp1 = (u_char *)src;
	register u_char *cp2 = (u_char *)dst;
	register u_char *cp3 = (u_char *)netmask;
	u_char *cplim = cp2 + *cp3;
	u_char *cplim2 = cp2 + *cp1;

	*cp2++ = *cp1++; *cp2++ = *cp1++; /* copies sa_len & sa_family */
	cp3 += 2;
	if (cplim > cplim2)
		cplim = cplim2;
	while (cp2 < cplim)
		*cp2++ = *cp1++ & *cp3++;
	if (cp2 < cplim2)
		bzero((caddr_t)cp2, (unsigned)(cplim2 - cp2));
}

/*
 * Set up or tear down a routing table entry, normally
 * for an interface.
 */
int
rtinit(ifa, cmd, flags)
	register struct ifaddr *ifa;
	int cmd, flags;
{
	register struct rtentry *rt;
	register struct sockaddr *dst, *odst;
	struct sockaddr_storage deldst;
	struct rtentry *nrt = 0;
	int error;

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
				return (flags & RTF_HOST ? EHOSTUNREACH
							: ENETUNREACH);
		}
	}
	error = rtrequest(cmd, dst, ifa->ifa_addr, ifa->ifa_netmask,
			flags | ifa->ifa_flags, &nrt);
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
			    rt->rt_ifa->ifa_rtrequest(RTM_DELETE, rt, SA(0));
			IFAFREE(rt->rt_ifa);
			rt->rt_ifa = ifa;
			rt->rt_ifp = ifa->ifa_ifp;
			rt->rt_rmx.rmx_mtu = ifa->ifa_ifp->if_mtu;	/*XXX*/
			IFAREF(ifa);
			if (ifa->ifa_rtrequest)
			    ifa->ifa_rtrequest(RTM_ADD, rt, SA(0));
		}
		rt_newaddrmsg(cmd, ifa, error, nrt);
	}
	return (error);
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

#define RTTIMER_CALLOUT(r)	{				\
	if (r->rtt_func != NULL) {				\
		(*r->rtt_func)(r->rtt_rt, r);			\
	} else {						\
		rtrequest((int) RTM_DELETE,			\
			  (struct sockaddr *)rt_key(r->rtt_rt),	\
			  0, 0, 0, 0);				\
	}							\
}

/* 
 * Some subtle order problems with domain initialization mean that
 * we cannot count on this being run from rt_init before various
 * protocol initializations are done.  Therefore, we make sure
 * that this is run when the first queue is added...
 */

void	 
rt_timer_init()
{
	assert(rt_init_done == 0);

	pool_init(&rttimer_pool, sizeof(struct rttimer), 0, 0, 0, "rttmrpl",
	    0, NULL, NULL, M_RTABLE);

	LIST_INIT(&rttimer_queue_head);
	timeout(rt_timer_timer, NULL, hz);  /* every second */
	rt_init_done = 1;
}

struct rttimer_queue *
rt_timer_queue_create(timeout)
	u_int	timeout;
{
	struct rttimer_queue *rtq;

	if (rt_init_done == 0)
		rt_timer_init();

	R_Malloc(rtq, struct rttimer_queue *, sizeof *rtq);
	if (rtq == NULL)
		return (NULL);		
	Bzero(rtq, sizeof *rtq);

	rtq->rtq_timeout = timeout;
	TAILQ_INIT(&rtq->rtq_head);
	LIST_INSERT_HEAD(&rttimer_queue_head, rtq, rtq_link);

	return (rtq);
}

void
rt_timer_queue_change(rtq, timeout)
	struct rttimer_queue *rtq;
	long timeout;
{

	rtq->rtq_timeout = timeout;
}


void
rt_timer_queue_destroy(rtq, destroy)
	struct rttimer_queue *rtq;
	int destroy;
{
	struct rttimer *r;

	while ((r = TAILQ_FIRST(&rtq->rtq_head)) != NULL) {
		LIST_REMOVE(r, rtt_link);
		TAILQ_REMOVE(&rtq->rtq_head, r, rtt_next);
		if (destroy)
			RTTIMER_CALLOUT(r);
		pool_put(&rttimer_pool, r);
	}

	LIST_REMOVE(rtq, rtq_link);

	/*
	 * Caller is responsible for freeing the rttimer_queue structure.
	 */
}

void     
rt_timer_remove_all(rt)
	struct rtentry *rt;
{
	struct rttimer *r;

	while ((r = LIST_FIRST(&rt->rt_timer)) != NULL) {
		LIST_REMOVE(r, rtt_link);
		TAILQ_REMOVE(&r->rtt_queue->rtq_head, r, rtt_next);
		pool_put(&rttimer_pool, r);
	}
}

int      
rt_timer_add(rt, func, queue)
	struct rtentry *rt;
	void(*func) __P((struct rtentry *, struct rttimer *));
	struct rttimer_queue *queue;
{
	struct rttimer *r;
	long current_time;
	int s;

	s = splclock();
	current_time = mono_time.tv_sec;
	splx(s);

	/*
	 * If there's already a timer with this action, destroy it before
	 * we add a new one.
	 */
	for (r = LIST_FIRST(&rt->rt_timer); r != NULL;
	     r = LIST_NEXT(r, rtt_link)) {
		if (r->rtt_func == func) {
			LIST_REMOVE(r, rtt_link);
			TAILQ_REMOVE(&r->rtt_queue->rtq_head, r, rtt_next);
			pool_put(&rttimer_pool, r);
			break;  /* only one per list, so we can quit... */
		}
	}

	r = pool_get(&rttimer_pool, PR_NOWAIT);
	if (r == NULL)
		return (ENOBUFS);
	Bzero(r, sizeof(*r));

	r->rtt_rt = rt;
	r->rtt_time = current_time;
	r->rtt_func = func;
	r->rtt_queue = queue;
	LIST_INSERT_HEAD(&rt->rt_timer, r, rtt_link);
	TAILQ_INSERT_TAIL(&queue->rtq_head, r, rtt_next);
	
	return (0);
}

/* ARGSUSED */
void
rt_timer_timer(arg)
	void *arg;
{
	struct rttimer_queue *rtq;
	struct rttimer *r;
	long current_time;
	int s;

	s = splclock();
	current_time = mono_time.tv_sec;
	splx(s);

	s = splsoftnet();
	for (rtq = LIST_FIRST(&rttimer_queue_head); rtq != NULL; 
	     rtq = LIST_NEXT(rtq, rtq_link)) {
		while ((r = TAILQ_FIRST(&rtq->rtq_head)) != NULL &&
		    (r->rtt_time + rtq->rtq_timeout) < current_time) {
			LIST_REMOVE(r, rtt_link);
			TAILQ_REMOVE(&rtq->rtq_head, r, rtt_next);
			RTTIMER_CALLOUT(r);
			pool_put(&rttimer_pool, r);
		}
	}
	splx(s);

	timeout(rt_timer_timer, NULL, hz);  /* every second */
}
