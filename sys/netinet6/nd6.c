/*	$NetBSD: nd6.c,v 1.278 2021/12/31 12:41:50 andvar Exp $	*/
/*	$KAME: nd6.c,v 1.279 2002/06/08 11:16:51 itojun Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nd6.c,v 1.278 2021/12/31 12:41:50 andvar Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_net_mpsafe.h"
#endif

#include "bridge.h"
#include "carp.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sockio.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/queue.h>
#include <sys/cprng.h>
#include <sys/workqueue.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_llatbl.h>
#include <net/if_types.h>
#include <net/nd.h>
#include <net/route.h>
#include <net/if_ether.h>
#include <net/if_arc.h>

#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/scope6_var.h>
#include <netinet6/nd6.h>
#include <netinet6/in6_ifattach.h>
#include <netinet/icmp6.h>
#include <netinet6/icmp6_private.h>

#ifdef COMPAT_90
#include <compat/netinet6/in6_var.h>
#include <compat/netinet6/nd6.h>
#endif

#define ND6_SLOWTIMER_INTERVAL (60 * 60) /* 1 hour */
#define ND6_RECALC_REACHTM_INTERVAL (60 * 120) /* 2 hours */

/* timer values */
int	nd6_prune	= 1;	/* walk list every 1 seconds */
int	nd6_useloopback = 1;	/* use loopback interface for local traffic */

/* preventing too many loops in ND option parsing */
int nd6_maxndopt = 10;	/* max # of ND options allowed */

#ifdef ND6_DEBUG
int nd6_debug = 1;
#else
int nd6_debug = 0;
#endif

krwlock_t nd6_lock __cacheline_aligned;

int nd6_recalc_reachtm_interval = ND6_RECALC_REACHTM_INTERVAL;

static void nd6_slowtimo(void *);
static void nd6_free(struct llentry *, int);
static bool nd6_nud_enabled(struct ifnet *);
static unsigned int nd6_llinfo_reachable(struct ifnet *);
static unsigned int nd6_llinfo_retrans(struct ifnet *);
static union l3addr *nd6_llinfo_holdsrc(struct llentry *, union l3addr *);
static void nd6_llinfo_output(struct ifnet *, const union l3addr *,
    const union l3addr *, const uint8_t *, const union l3addr *);
static void nd6_llinfo_missed(struct ifnet *, const union l3addr *,
    int16_t, struct mbuf *);
static void nd6_timer(void *);
static void nd6_timer_work(struct work *, void *);
static struct nd_opt_hdr *nd6_option(union nd_opts *);

static callout_t nd6_slowtimo_ch;
static callout_t nd6_timer_ch;
static struct workqueue	*nd6_timer_wq;
static struct work	nd6_timer_wk;

struct nd_domain nd6_nd_domain = {
	.nd_family = AF_INET6,
	.nd_delay = 5,		/* delay first probe time 5 second */
	.nd_mmaxtries = 3,	/* maximum unicast query */
	.nd_umaxtries = 3,	/* maximum multicast query */
	.nd_retransmultiple = BACKOFF_MULTIPLE,
	.nd_maxretrans = MAX_RETRANS_TIMER,
	.nd_maxnudhint = 0,	/* max # of subsequent upper layer hints */
	.nd_maxqueuelen = 1,	/* max # of packets in unresolved ND entries */
	.nd_nud_enabled = nd6_nud_enabled,
	.nd_reachable = nd6_llinfo_reachable,
	.nd_retrans = nd6_llinfo_retrans,
	.nd_holdsrc = nd6_llinfo_holdsrc,
	.nd_output = nd6_llinfo_output,
	.nd_missed = nd6_llinfo_missed,
	.nd_free = nd6_free,
};

MALLOC_DEFINE(M_IP6NDP, "NDP", "IPv6 Neighbour Discovery");

void
nd6_init(void)
{
	int error;

	nd_attach_domain(&nd6_nd_domain);
	nd6_nbr_init();

	rw_init(&nd6_lock);

	callout_init(&nd6_slowtimo_ch, CALLOUT_MPSAFE);
	callout_init(&nd6_timer_ch, CALLOUT_MPSAFE);

	error = workqueue_create(&nd6_timer_wq, "nd6_timer",
	    nd6_timer_work, NULL, PRI_SOFTNET, IPL_SOFTNET, WQ_MPSAFE);
	if (error)
		panic("%s: workqueue_create failed (%d)\n", __func__, error);

	/* start timer */
	callout_reset(&nd6_slowtimo_ch, ND6_SLOWTIMER_INTERVAL * hz,
	    nd6_slowtimo, NULL);
	callout_reset(&nd6_timer_ch, hz, nd6_timer, NULL);
}

struct nd_kifinfo *
nd6_ifattach(struct ifnet *ifp)
{
	struct nd_kifinfo *nd;

	nd = kmem_zalloc(sizeof(*nd), KM_SLEEP);

	nd->chlim = IPV6_DEFHLIM;
	nd->basereachable = REACHABLE_TIME;
	nd->reachable = ND_COMPUTE_RTIME(nd->basereachable);
	nd->retrans = RETRANS_TIMER;

	nd->flags = ND6_IFF_PERFORMNUD;

	/* A loopback interface always has ND6_IFF_AUTO_LINKLOCAL.
	 * A bridge interface should not have ND6_IFF_AUTO_LINKLOCAL
	 * because one of its members should. */
	if ((ip6_auto_linklocal && ifp->if_type != IFT_BRIDGE) ||
	    (ifp->if_flags & IFF_LOOPBACK))
		nd->flags |= ND6_IFF_AUTO_LINKLOCAL;

	return nd;
}

void
nd6_ifdetach(struct ifnet *ifp, struct in6_ifextra *ext)
{

	/* Ensure all IPv6 addresses are purged before calling nd6_purge */
	if_purgeaddrs(ifp, AF_INET6, in6_purgeaddr);
	nd6_purge(ifp, ext);
	kmem_free(ext->nd_ifinfo, sizeof(struct nd_kifinfo));
}

void
nd6_option_init(void *opt, int icmp6len, union nd_opts *ndopts)
{

	memset(ndopts, 0, sizeof(*ndopts));
	ndopts->nd_opts_search = (struct nd_opt_hdr *)opt;
	ndopts->nd_opts_last
		= (struct nd_opt_hdr *)(((u_char *)opt) + icmp6len);

	if (icmp6len == 0) {
		ndopts->nd_opts_done = 1;
		ndopts->nd_opts_search = NULL;
	}
}

/*
 * Take one ND option.
 */
static struct nd_opt_hdr *
nd6_option(union nd_opts *ndopts)
{
	struct nd_opt_hdr *nd_opt;
	int olen;

	KASSERT(ndopts != NULL);
	KASSERT(ndopts->nd_opts_last != NULL);

	if (ndopts->nd_opts_search == NULL)
		return NULL;
	if (ndopts->nd_opts_done)
		return NULL;

	nd_opt = ndopts->nd_opts_search;

	/* make sure nd_opt_len is inside the buffer */
	if ((void *)&nd_opt->nd_opt_len >= (void *)ndopts->nd_opts_last) {
		memset(ndopts, 0, sizeof(*ndopts));
		return NULL;
	}

	olen = nd_opt->nd_opt_len << 3;
	if (olen == 0) {
		/*
		 * Message validation requires that all included
		 * options have a length that is greater than zero.
		 */
		memset(ndopts, 0, sizeof(*ndopts));
		return NULL;
	}

	ndopts->nd_opts_search = (struct nd_opt_hdr *)((char *)nd_opt + olen);
	if (ndopts->nd_opts_search > ndopts->nd_opts_last) {
		/* option overruns the end of buffer, invalid */
		memset(ndopts, 0, sizeof(*ndopts));
		return NULL;
	} else if (ndopts->nd_opts_search == ndopts->nd_opts_last) {
		/* reached the end of options chain */
		ndopts->nd_opts_done = 1;
		ndopts->nd_opts_search = NULL;
	}
	return nd_opt;
}

/*
 * Parse multiple ND options.
 * This function is much easier to use, for ND routines that do not need
 * multiple options of the same type.
 */
int
nd6_options(union nd_opts *ndopts)
{
	struct nd_opt_hdr *nd_opt;
	int i = 0;

	KASSERT(ndopts != NULL);
	KASSERT(ndopts->nd_opts_last != NULL);

	if (ndopts->nd_opts_search == NULL)
		return 0;
 
	while (1) {
		nd_opt = nd6_option(ndopts);
		if (nd_opt == NULL && ndopts->nd_opts_last == NULL) {
			/*
			 * Message validation requires that all included
			 * options have a length that is greater than zero.
			 */
			ICMP6_STATINC(ICMP6_STAT_ND_BADOPT);
			memset(ndopts, 0, sizeof(*ndopts));
			return -1;
		}

		if (nd_opt == NULL)
			goto skip1;

		switch (nd_opt->nd_opt_type) {
		case ND_OPT_SOURCE_LINKADDR:
		case ND_OPT_TARGET_LINKADDR:
		case ND_OPT_MTU:
		case ND_OPT_REDIRECTED_HEADER:
		case ND_OPT_NONCE:
			if (ndopts->nd_opt_array[nd_opt->nd_opt_type]) {
				nd6log(LOG_INFO,
				    "duplicated ND6 option found (type=%d)\n",
				    nd_opt->nd_opt_type);
				/* XXX bark? */
			} else {
				ndopts->nd_opt_array[nd_opt->nd_opt_type]
					= nd_opt;
			}
			break;
		case ND_OPT_PREFIX_INFORMATION:
			if (ndopts->nd_opt_array[nd_opt->nd_opt_type] == 0) {
				ndopts->nd_opt_array[nd_opt->nd_opt_type]
					= nd_opt;
			}
			ndopts->nd_opts_pi_end =
				(struct nd_opt_prefix_info *)nd_opt;
			break;
		default:
			/*
			 * Unknown options must be silently ignored,
			 * to accommodate future extension to the protocol.
			 */
			nd6log(LOG_DEBUG,
			    "nd6_options: unsupported option %d - "
			    "option ignored\n", nd_opt->nd_opt_type);
		}

skip1:
		i++;
		if (i > nd6_maxndopt) {
			ICMP6_STATINC(ICMP6_STAT_ND_TOOMANYOPT);
			nd6log(LOG_INFO, "too many loop in nd opt\n");
			break;
		}

		if (ndopts->nd_opts_done)
			break;
	}

	return 0;
}

/*
 * Gets source address of the first packet in hold queue
 * and stores it in @src.
 * Returns pointer to @src (if hold queue is not empty) or NULL.
 */
static struct in6_addr *
nd6_llinfo_get_holdsrc(struct llentry *ln, struct in6_addr *src)
{
	struct ip6_hdr *hip6;

	if (ln == NULL || ln->ln_hold == NULL)
		return NULL;

	/*
	 * assuming every packet in ln_hold has the same IP header
	 */
	hip6 = mtod(ln->ln_hold, struct ip6_hdr *);
	/* XXX pullup? */
	if (sizeof(*hip6) < ln->ln_hold->m_len)
		*src = hip6->ip6_src;
	else
		src = NULL;

	return src;
}

static union l3addr *
nd6_llinfo_holdsrc(struct llentry *ln, union l3addr *src)
{

	if (nd6_llinfo_get_holdsrc(ln, &src->addr6) == NULL)
		return NULL;
	return src;
}

static void
nd6_llinfo_output(struct ifnet *ifp, const union l3addr *daddr,
    const union l3addr *taddr, __unused const uint8_t *tlladdr,
    const union l3addr *hsrc)
{

	nd6_ns_output(ifp,
	    daddr != NULL ? &daddr->addr6 : NULL,
	    taddr != NULL ? &taddr->addr6 : NULL,
	    hsrc != NULL ? &hsrc->addr6 : NULL, NULL);
}

static bool
nd6_nud_enabled(struct ifnet *ifp)
{
	struct nd_kifinfo *ndi = ND_IFINFO(ifp);

	return ndi->flags & ND6_IFF_PERFORMNUD;
}

static unsigned int
nd6_llinfo_reachable(struct ifnet *ifp)
{
	struct nd_kifinfo *ndi = ND_IFINFO(ifp);

	return ndi->reachable;
}

static unsigned int
nd6_llinfo_retrans(struct ifnet *ifp)
{
	struct nd_kifinfo *ndi = ND_IFINFO(ifp);

	return ndi->retrans;
}

static void
nd6_llinfo_missed(struct ifnet *ifp, const union l3addr *taddr,
    int16_t type, struct mbuf *m)
{
	struct in6_addr mdaddr6 = zeroin6_addr;
	struct sockaddr_in6 dsin6, tsin6;
	struct sockaddr *sa;

	if (m != NULL) {
		if (type == ND_LLINFO_PROBE) {
			struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);

			/* XXX pullup? */
			if (sizeof(*ip6) < m->m_len)
				mdaddr6 = ip6->ip6_src;
			m_freem(m);
		} else
			icmp6_error2(m, ICMP6_DST_UNREACH,
			    ICMP6_DST_UNREACH_ADDR, 0, ifp, &mdaddr6);
	}
	if (!IN6_IS_ADDR_UNSPECIFIED(&mdaddr6)) {
		sockaddr_in6_init(&dsin6, &mdaddr6, 0, 0, 0);
		sa = sin6tosa(&dsin6);
	} else
		sa = NULL;

	sockaddr_in6_init(&tsin6, &taddr->addr6, 0, 0, 0);
	rt_clonedmsg(RTM_MISS, sa, sin6tosa(&tsin6), NULL, ifp);
}

/*
 * ND6 timer routine to expire default route list and prefix list
 */
static void
nd6_timer_work(struct work *wk, void *arg)
{
	struct in6_ifaddr *ia6, *nia6;
	int s, bound;
	struct psref psref;

	callout_reset(&nd6_timer_ch, nd6_prune * hz,
	    nd6_timer, NULL);

	SOFTNET_KERNEL_LOCK_UNLESS_NET_MPSAFE();

	/* expire interface addresses */
	bound = curlwp_bind();
	s = pserialize_read_enter();
	for (ia6 = IN6_ADDRLIST_READER_FIRST(); ia6; ia6 = nia6) {
		nia6 = IN6_ADDRLIST_READER_NEXT(ia6);

		ia6_acquire(ia6, &psref);
		pserialize_read_exit(s);

		/* check address lifetime */
		if (IFA6_IS_INVALID(ia6)) {
			struct ifnet *ifp;

			ifp = ia6->ia_ifa.ifa_ifp;
			IFNET_LOCK(ifp);
			/*
			 * Need to take the lock first to prevent if_detach
			 * from running in6_purgeaddr concurrently.
			 */
			if (!if_is_deactivated(ifp)) {
				ia6_release(ia6, &psref);
				in6_purgeaddr(&ia6->ia_ifa);
			} else {
				/*
				 * ifp is being destroyed, ia6 will be destroyed
				 * by if_detach.
				 */
				ia6_release(ia6, &psref);
			}
			ia6 = NULL;
			IFNET_UNLOCK(ifp);
		} else if (IFA6_IS_DEPRECATED(ia6)) {
			int oldflags = ia6->ia6_flags;

			if ((oldflags & IN6_IFF_DEPRECATED) == 0) {
				ia6->ia6_flags |= IN6_IFF_DEPRECATED;
				rt_addrmsg(RTM_NEWADDR, (struct ifaddr *)ia6);
			}
		} else {
			/*
			 * A new RA might have made a deprecated address
			 * preferred.
			 */
			if (ia6->ia6_flags & IN6_IFF_DEPRECATED) {
				ia6->ia6_flags &= ~IN6_IFF_DEPRECATED;
				rt_addrmsg(RTM_NEWADDR, (struct ifaddr *)ia6);
			}
		}
		s = pserialize_read_enter();
		ia6_release(ia6, &psref);
	}
	pserialize_read_exit(s);
	curlwp_bindx(bound);

	SOFTNET_KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
}

static void
nd6_timer(void *ignored_arg)
{

	workqueue_enqueue(nd6_timer_wq, &nd6_timer_wk, NULL);
}

/*
 * Nuke neighbor cache/prefix/default router management table, right before
 * ifp goes away.
 */
void
nd6_purge(struct ifnet *ifp, struct in6_ifextra *ext)
{

	/*
	 * During detach, the ND info might be already removed, but
	 * then is explitly passed as argument.
	 * Otherwise get it from ifp->if_afdata.
	 */
	if (ext == NULL)
		ext = ifp->if_afdata[AF_INET6];
	if (ext == NULL)
		return;

	/*
	 * We may not need to nuke the neighbor cache entries here
	 * because the neighbor cache is kept in if_afdata[AF_INET6].
	 * nd6_purge() is invoked by in6_ifdetach() which is called
	 * from if_detach() where everything gets purged. However
	 * in6_ifdetach is directly called from vlan(4), so we still
	 * need to purge entries here.
	 */
	if (ext->lltable != NULL)
		lltable_purge_entries(ext->lltable);
}

struct llentry *
nd6_lookup(const struct in6_addr *addr6, const struct ifnet *ifp, bool wlock)
{
	struct sockaddr_in6 sin6;
	struct llentry *ln;

	sockaddr_in6_init(&sin6, addr6, 0, 0, 0);

	IF_AFDATA_RLOCK(ifp);
	ln = lla_lookup(LLTABLE6(ifp), wlock ? LLE_EXCLUSIVE : 0,
	    sin6tosa(&sin6));
	IF_AFDATA_RUNLOCK(ifp);

	return ln;
}

struct llentry *
nd6_create(const struct in6_addr *addr6, const struct ifnet *ifp)
{
	struct sockaddr_in6 sin6;
	struct llentry *ln;
	struct rtentry *rt;

	sockaddr_in6_init(&sin6, addr6, 0, 0, 0);
	rt = rtalloc1(sin6tosa(&sin6), 0);

	IF_AFDATA_WLOCK(ifp);
	ln = lla_create(LLTABLE6(ifp), LLE_EXCLUSIVE, sin6tosa(&sin6), rt);
	IF_AFDATA_WUNLOCK(ifp);

	if (rt != NULL)
		rt_unref(rt);
	if (ln != NULL)
		ln->ln_state = ND_LLINFO_NOSTATE;

	return ln;
}

/*
 * Test whether a given IPv6 address is a neighbor or not, ignoring
 * the actual neighbor cache.  The neighbor cache is ignored in order
 * to not reenter the routing code from within itself.
 */
static int
nd6_is_new_addr_neighbor(const struct sockaddr_in6 *addr, struct ifnet *ifp)
{
	struct ifaddr *dstaddr;
	int s;

	/*
	 * A link-local address is always a neighbor.
	 * XXX: a link does not necessarily specify a single interface.
	 */
	if (IN6_IS_ADDR_LINKLOCAL(&addr->sin6_addr)) {
		struct sockaddr_in6 sin6_copy;
		u_int32_t zone;

		/*
		 * We need sin6_copy since sa6_recoverscope() may modify the
		 * content (XXX).
		 */
		sin6_copy = *addr;
		if (sa6_recoverscope(&sin6_copy))
			return 0; /* XXX: should be impossible */
		if (in6_setscope(&sin6_copy.sin6_addr, ifp, &zone))
			return 0;
		if (sin6_copy.sin6_scope_id == zone)
			return 1;
		else
			return 0;
	}

	/*
	 * If the address is assigned on the node of the other side of
	 * a p2p interface, the address should be a neighbor.
	 */
	s = pserialize_read_enter();
	dstaddr = ifa_ifwithdstaddr(sin6tocsa(addr));
	if (dstaddr != NULL) {
		if (dstaddr->ifa_ifp == ifp) {
			pserialize_read_exit(s);
			return 1;
		}
	}
	pserialize_read_exit(s);

	return 0;
}

/*
 * Detect if a given IPv6 address identifies a neighbor on a given link.
 * XXX: should take care of the destination of a p2p link?
 */
int
nd6_is_addr_neighbor(const struct sockaddr_in6 *addr, struct ifnet *ifp)
{
	struct llentry *ln;
	struct rtentry *rt;

	/*
	 * A link-local address is always a neighbor.
	 * XXX: a link does not necessarily specify a single interface.
	 */
	if (IN6_IS_ADDR_LINKLOCAL(&addr->sin6_addr)) {
		struct sockaddr_in6 sin6_copy;
		u_int32_t zone;

		/*
		 * We need sin6_copy since sa6_recoverscope() may modify the
		 * content (XXX).
		 */
		sin6_copy = *addr;
		if (sa6_recoverscope(&sin6_copy))
			return 0; /* XXX: should be impossible */
		if (in6_setscope(&sin6_copy.sin6_addr, ifp, &zone))
			return 0;
		if (sin6_copy.sin6_scope_id == zone)
			return 1;
		else
			return 0;
	}

	if (nd6_is_new_addr_neighbor(addr, ifp))
		return 1;

	/*
	 * Even if the address matches none of our addresses, it might be
	 * in the neighbor cache or a connected route.
	 */
	ln = nd6_lookup(&addr->sin6_addr, ifp, false);
	if (ln != NULL) {
		LLE_RUNLOCK(ln);
		return 1;
	}

	rt = rtalloc1(sin6tocsa(addr), 0);
	if (rt == NULL)
		return 0;

	if ((rt->rt_flags & RTF_CONNECTED) && (rt->rt_ifp == ifp
#if NBRIDGE > 0
	    || rt->rt_ifp->if_bridge == ifp->if_bridge
#endif
#if NCARP > 0
	    || (ifp->if_type == IFT_CARP && rt->rt_ifp == ifp->if_carpdev) ||
	    (rt->rt_ifp->if_type == IFT_CARP && rt->rt_ifp->if_carpdev == ifp)||
	    (ifp->if_type == IFT_CARP && rt->rt_ifp->if_type == IFT_CARP &&
	    rt->rt_ifp->if_carpdev == ifp->if_carpdev)
#endif
	    )) {
		rt_unref(rt);
		return 1;
	}
	rt_unref(rt);

	return 0;
}

/*
 * Free an nd6 llinfo entry.
 * Since the function would cause significant changes in the kernel, DO NOT
 * make it global, unless you have a strong reason for the change, and are sure
 * that the change is safe.
 */
static void
nd6_free(struct llentry *ln, int gc)
{
	struct ifnet *ifp;

	KASSERT(ln != NULL);
	LLE_WLOCK_ASSERT(ln);

	/*
	 * If the reason for the deletion is just garbage collection,
	 * and the neighbor is an active router, do not delete it.
	 * Instead, reset the GC timer using the router's lifetime.
	 * XXX: the check for ln_state should be redundant,
	 *      but we intentionally keep it just in case.
	 */
	if (!ip6_forwarding && ln->ln_router &&
	    ln->ln_state == ND_LLINFO_STALE && gc)
	{
		nd_set_timer(ln, ND_TIMER_EXPIRE);
		LLE_WUNLOCK(ln);
		return;
	}

	ifp = ln->lle_tbl->llt_ifp;

	if (ln->la_flags & LLE_VALID || gc) {
		struct sockaddr_in6 sin6;
		const char *lladdr;

		sockaddr_in6_init(&sin6, &ln->r_l3addr.addr6, 0, 0, 0);
		lladdr = ln->la_flags & LLE_VALID ?
		    (const char *)&ln->ll_addr : NULL;
		rt_clonedmsg(RTM_DELETE, NULL, sin6tosa(&sin6), lladdr, ifp);
	}

	/*
	 * Save to unlock. We still hold an extra reference and will not
	 * free(9) in llentry_free() if someone else holds one as well.
	 */
	LLE_WUNLOCK(ln);
	IF_AFDATA_LOCK(ifp);
	LLE_WLOCK(ln);

	lltable_free_entry(LLTABLE6(ifp), ln);

	IF_AFDATA_UNLOCK(ifp);
}

/*
 * Upper-layer reachability hint for Neighbor Unreachability Detection.
 *
 * XXX cost-effective methods?
 */
void
nd6_nud_hint(struct rtentry *rt)
{
	struct llentry *ln;
	struct ifnet *ifp;

	if (rt == NULL)
		return;

	ifp = rt->rt_ifp;
	ln = nd6_lookup(&(satocsin6(rt_getkey(rt)))->sin6_addr, ifp, true);
	nd_nud_hint(ln);
}

struct gc_args {
	int gc_entries;
	const struct in6_addr *skip_in6;
};

static int
nd6_purge_entry(struct lltable *llt, struct llentry *ln, void *farg)
{
	struct gc_args *args = farg;
	int *n = &args->gc_entries;
	const struct in6_addr *skip_in6 = args->skip_in6;

	if (*n <= 0)
		return 0;

	if (ND_IS_LLINFO_PERMANENT(ln))
		return 0;

	if (IN6_ARE_ADDR_EQUAL(&ln->r_l3addr.addr6, skip_in6))
		return 0;

	LLE_WLOCK(ln);
	if (ln->ln_state > ND_LLINFO_INCOMPLETE)
		ln->ln_state = ND_LLINFO_STALE;
	else
		ln->ln_state = ND_LLINFO_PURGE;
	nd_set_timer(ln, ND_TIMER_IMMEDIATE);
	LLE_WUNLOCK(ln);

	(*n)--;
	return 0;
}

static void
nd6_gc_neighbors(struct lltable *llt, const struct in6_addr *in6)
{

	if (ip6_neighborgcthresh >= 0 &&
	    lltable_get_entry_count(llt) >= ip6_neighborgcthresh) {
		struct gc_args gc_args = {10, in6};
		/*
		 * XXX entries that are "less recently used" should be
		 * freed first.
		 */
		lltable_foreach_lle(llt, nd6_purge_entry, &gc_args);
	}
}

void
nd6_rtrequest(int req, struct rtentry *rt, const struct rt_addrinfo *info)
{
	struct sockaddr *gate = rt->rt_gateway;
	struct ifnet *ifp = rt->rt_ifp;
	uint8_t namelen = strlen(ifp->if_xname), addrlen = ifp->if_addrlen;
	struct ifaddr *ifa;

	RT_DPRINTF("rt_getkey(rt) = %p\n", rt_getkey(rt));

	if (req == RTM_LLINFO_UPD) {
		int rc;
		struct in6_addr *in6;
		struct in6_addr in6_all;
		int anycast;

		if ((ifa = info->rti_ifa) == NULL)
			return;

		in6 = &ifatoia6(ifa)->ia_addr.sin6_addr;
		anycast = ifatoia6(ifa)->ia6_flags & IN6_IFF_ANYCAST;

		in6_all = in6addr_linklocal_allnodes;
		if ((rc = in6_setscope(&in6_all, ifa->ifa_ifp, NULL)) != 0) {
			log(LOG_ERR, "%s: failed to set scope %s "
			    "(errno=%d)\n", __func__, if_name(ifp), rc);
			return;
		}

		/* XXX don't set Override for proxy addresses */
		nd6_na_output(ifa->ifa_ifp, &in6_all, in6,
		    (anycast ? 0 : ND_NA_FLAG_OVERRIDE)
#if 0
		    | (ip6_forwarding ? ND_NA_FLAG_ROUTER : 0)
#endif
		    , 1, NULL);
		return;
	}

	if ((rt->rt_flags & RTF_GATEWAY) != 0) {
		if (req != RTM_ADD)
			return;
		/*
		 * linklayers with particular MTU limitation.
		 */
		switch(ifp->if_type) {
#if NARCNET > 0
		case IFT_ARCNET:
			if (rt->rt_rmx.rmx_mtu > ARC_PHDS_MAXMTU) /* RFC2497 */
				rt->rt_rmx.rmx_mtu = ARC_PHDS_MAXMTU;
			break;
#endif
		}
		return;
	}

	if (nd6_need_cache(ifp) == 0 && (rt->rt_flags & RTF_HOST) == 0) {
		RT_DPRINTF("rt_getkey(rt) = %p\n", rt_getkey(rt));
		/*
		 * This is probably an interface direct route for a link
		 * which does not need neighbor caches (e.g. fe80::%lo0/64).
		 * We do not need special treatment below for such a route.
		 * Moreover, the RTF_LLINFO flag which would be set below
		 * would annoy the ndp(8) command.
		 */
		return;
	}

	switch (req) {
	case RTM_ADD: {
		struct psref psref;

		RT_DPRINTF("rt_getkey(rt) = %p\n", rt_getkey(rt));
		/*
		 * There is no backward compatibility :)
		 *
		 * if ((rt->rt_flags & RTF_HOST) == 0 &&
		 *     SIN(rt_mask(rt))->sin_addr.s_addr != 0xffffffff)
		 *	   rt->rt_flags |= RTF_CLONING;
		 */
		/* XXX should move to route.c? */
		if (rt->rt_flags & (RTF_CONNECTED | RTF_LOCAL)) {
			union {
				struct sockaddr sa;
				struct sockaddr_dl sdl;
				struct sockaddr_storage ss;
			} u;
			/*
			 * Case 1: This route should come from a route to
			 * interface (RTF_CLONING case) or the route should be
			 * treated as on-link but is currently not
			 * (RTF_LLINFO && ln == NULL case).
			 */
			if (sockaddr_dl_init(&u.sdl, sizeof(u.ss),
			    ifp->if_index, ifp->if_type,
			    NULL, namelen, NULL, addrlen) == NULL) {
				printf("%s.%d: sockaddr_dl_init(, %zu, ) "
				    "failed on %s\n", __func__, __LINE__,
				    sizeof(u.ss), if_name(ifp));
			}
			rt_setgate(rt, &u.sa);
			gate = rt->rt_gateway;
			RT_DPRINTF("rt_getkey(rt) = %p\n", rt_getkey(rt));
			if (gate == NULL) {
				log(LOG_ERR,
				    "%s: rt_setgate failed on %s\n", __func__,
				    if_name(ifp));
				break;
			}

			RT_DPRINTF("rt_getkey(rt) = %p\n", rt_getkey(rt));
			if ((rt->rt_flags & RTF_CONNECTED) != 0)
				break;
		}
		RT_DPRINTF("rt_getkey(rt) = %p\n", rt_getkey(rt));
		/*
		 * In IPv4 code, we try to annonuce new RTF_ANNOUNCE entry here.
		 * We don't do that here since llinfo is not ready yet.
		 *
		 * There are also couple of other things to be discussed:
		 * - unsolicited NA code needs improvement beforehand
		 * - RFC2461 says we MAY send multicast unsolicited NA
		 *   (7.2.6 paragraph 4), however, it also says that we
		 *   SHOULD provide a mechanism to prevent multicast NA storm.
		 *   we don't have anything like it right now.
		 *   note that the mechanism needs a mutual agreement
		 *   between proxies, which means that we need to implement
		 *   a new protocol, or a new kludge.
		 * - from RFC2461 6.2.4, host MUST NOT send an unsolicited NA.
		 *   we need to check ip6forwarding before sending it.
		 *   (or should we allow proxy ND configuration only for
		 *   routers?  there's no mention about proxy ND from hosts)
		 */
#if 0
		/* XXX it does not work */
		if (rt->rt_flags & RTF_ANNOUNCE)
			nd6_na_output(ifp,
			      &satocsin6(rt_getkey(rt))->sin6_addr,
			      &satocsin6(rt_getkey(rt))->sin6_addr,
			      ip6_forwarding ? ND_NA_FLAG_ROUTER : 0,
			      1, NULL);
#endif

		if ((ifp->if_flags & (IFF_POINTOPOINT | IFF_LOOPBACK)) == 0) {
			RT_DPRINTF("rt_getkey(rt) = %p\n", rt_getkey(rt));
			/*
			 * Address resolution isn't necessary for a point to
			 * point link, so we can skip this test for a p2p link.
			 */
			if (gate->sa_family != AF_LINK ||
			    gate->sa_len <
			    sockaddr_dl_measure(namelen, addrlen)) {
				log(LOG_DEBUG,
				    "nd6_rtrequest: bad gateway value: %s\n",
				    if_name(ifp));
				break;
			}
			satosdl(gate)->sdl_type = ifp->if_type;
			satosdl(gate)->sdl_index = ifp->if_index;
			RT_DPRINTF("rt_getkey(rt) = %p\n", rt_getkey(rt));
		}
		RT_DPRINTF("rt_getkey(rt) = %p\n", rt_getkey(rt));

		/*
		 * When called from rt_ifa_addlocal, we cannot depend on that
		 * the address (rt_getkey(rt)) exits in the address list of the
		 * interface. So check RTF_LOCAL instead.
		 */
		if (rt->rt_flags & RTF_LOCAL) {
			if (nd6_useloopback)
				rt->rt_ifp = lo0ifp;	/* XXX */
			break;
		}

		/*
		 * check if rt_getkey(rt) is an address assigned
		 * to the interface.
		 */
		ifa = (struct ifaddr *)in6ifa_ifpwithaddr_psref(ifp,
		    &satocsin6(rt_getkey(rt))->sin6_addr, &psref);
		if (ifa != NULL) {
			if (nd6_useloopback) {
				rt->rt_ifp = lo0ifp;	/* XXX */
				/*
				 * Make sure rt_ifa be equal to the ifaddr
				 * corresponding to the address.
				 * We need this because when we refer
				 * rt_ifa->ia6_flags in ip6_input, we assume
				 * that the rt_ifa points to the address instead
				 * of the loopback address.
				 */
				if (!ISSET(info->rti_flags, RTF_DONTCHANGEIFA)
				    && ifa != rt->rt_ifa)
					rt_replace_ifa(rt, ifa);
			}
		} else if (rt->rt_flags & RTF_ANNOUNCE) {
			/* join solicited node multicast for proxy ND */
			if (ifp->if_flags & IFF_MULTICAST) {
				struct in6_addr llsol;
				int error;

				llsol = satocsin6(rt_getkey(rt))->sin6_addr;
				llsol.s6_addr32[0] = htonl(0xff020000);
				llsol.s6_addr32[1] = 0;
				llsol.s6_addr32[2] = htonl(1);
				llsol.s6_addr8[12] = 0xff;
				if (in6_setscope(&llsol, ifp, NULL))
					goto out;
				if (!in6_addmulti(&llsol, ifp, &error, 0)) {
					char ip6buf[INET6_ADDRSTRLEN];
					nd6log(LOG_ERR, "%s: failed to join "
					    "%s (errno=%d)\n", if_name(ifp),
					    IN6_PRINT(ip6buf, &llsol), error);
				}
			}
		}
	out:
		ifa_release(ifa, &psref);
		/*
		 * If we have too many cache entries, initiate immediate
		 * purging for some entries.
		 */
		if (rt->rt_ifp != NULL)
			nd6_gc_neighbors(LLTABLE6(rt->rt_ifp), NULL);
		break;
	    }

	case RTM_DELETE:
		/* leave from solicited node multicast for proxy ND */
		if ((rt->rt_flags & RTF_ANNOUNCE) != 0 &&
		    (ifp->if_flags & IFF_MULTICAST) != 0) {
			struct in6_addr llsol;

			llsol = satocsin6(rt_getkey(rt))->sin6_addr;
			llsol.s6_addr32[0] = htonl(0xff020000);
			llsol.s6_addr32[1] = 0;
			llsol.s6_addr32[2] = htonl(1);
			llsol.s6_addr8[12] = 0xff;
			if (in6_setscope(&llsol, ifp, NULL) == 0)
				in6_lookup_and_delete_multi(&llsol, ifp);
		}
		break;
	}
}

static void
nd6_setifflags(struct ifnet *ifp, uint32_t flags)
{
	struct nd_kifinfo *ndi = ND_IFINFO(ifp);
	struct ifaddr *ifa;
	struct in6_ifaddr *ia;
	int s;

	if (ndi->flags & ND6_IFF_IFDISABLED && !(flags & ND6_IFF_IFDISABLED)) {
		/*
		 * If the interface is marked as ND6_IFF_IFDISABLED and
		 * has a link-local address with IN6_IFF_DUPLICATED,
		 * do not clear ND6_IFF_IFDISABLED.
		 * See RFC 4862, section 5.4.5.
		 */
		bool duplicated_linklocal = false;

		s = pserialize_read_enter();
		IFADDR_READER_FOREACH(ifa, ifp) {
			if (ifa->ifa_addr->sa_family != AF_INET6)
				continue;
			ia = (struct in6_ifaddr *)ifa;
			if ((ia->ia6_flags & IN6_IFF_DUPLICATED) &&
			    IN6_IS_ADDR_LINKLOCAL(IA6_IN6(ia)))
			{
				duplicated_linklocal = true;
				break;
			}
		}
		pserialize_read_exit(s);

		if (duplicated_linklocal) {
			flags |= ND6_IFF_IFDISABLED;
			log(LOG_ERR, "%s: Cannot enable an interface"
			    " with a link-local address marked"
			    " duplicate.\n", if_name(ifp));
		} else {
			ndi->flags &= ~ND6_IFF_IFDISABLED;
			if (ifp->if_flags & IFF_UP)
				in6_if_up(ifp);
		}
	} else if (!(ndi->flags & ND6_IFF_IFDISABLED) &&
	    (flags & ND6_IFF_IFDISABLED))
	{
		struct psref psref;
		int bound = curlwp_bind();

		/* Mark all IPv6 addresses as tentative. */

		ndi->flags |= ND6_IFF_IFDISABLED;
		s = pserialize_read_enter();
		IFADDR_READER_FOREACH(ifa, ifp) {
			if (ifa->ifa_addr->sa_family != AF_INET6)
				continue;
			ifa_acquire(ifa, &psref);
			pserialize_read_exit(s);

			nd6_dad_stop(ifa);

			ia = (struct in6_ifaddr *)ifa;
			ia->ia6_flags |= IN6_IFF_TENTATIVE;

			s = pserialize_read_enter();
			ifa_release(ifa, &psref);
		}
		pserialize_read_exit(s);
		curlwp_bindx(bound);
	}

	if (flags & ND6_IFF_AUTO_LINKLOCAL) {
		if (!(ndi->flags & ND6_IFF_AUTO_LINKLOCAL)) {
			/* auto_linklocal 0->1 transition */

			ndi->flags |= ND6_IFF_AUTO_LINKLOCAL;
			in6_ifattach(ifp, NULL);
		} else if (!(flags & ND6_IFF_IFDISABLED) &&
		    ifp->if_flags & IFF_UP)
		{
			/*
			 * When the IF already has
			 * ND6_IFF_AUTO_LINKLOCAL, no link-local
			 * address is assigned, and IFF_UP, try to
			 * assign one.
			 */
			bool haslinklocal = 0;

			s = pserialize_read_enter();
			IFADDR_READER_FOREACH(ifa, ifp) {
				if (ifa->ifa_addr->sa_family !=AF_INET6)
					continue;
				ia = (struct in6_ifaddr *)ifa;
				if (IN6_IS_ADDR_LINKLOCAL(IA6_IN6(ia))){
					haslinklocal = true;
					break;
				}
			}
			pserialize_read_exit(s);
			if (!haslinklocal)
				in6_ifattach(ifp, NULL);
		}
	}

	ndi->flags = flags;
}

int
nd6_ioctl(u_long cmd, void *data, struct ifnet *ifp)
{
#ifdef OSIOCGIFINFO_IN6_90
	struct in6_ndireq90 *ondi = (struct in6_ndireq90 *)data;
	struct in6_ndifreq90 *ndif = (struct in6_ndifreq90 *)data;
#define OND	ondi->ndi
#endif
	struct in6_ndireq *ndi = (struct in6_ndireq *)data;
	struct in6_nbrinfo *nbi = (struct in6_nbrinfo *)data;
	struct nd_kifinfo *ifndi = ND_IFINFO(ifp);
	int error = 0;
#define ND     ndi->ndi

	switch (cmd) {
#ifdef OSIOCSRTRFLUSH_IN6
	case OSIOCGDRLST_IN6:		/* FALLTHROUGH */
	case OSIOCGPRLST_IN6:		/* FALLTHROUGH */
	case OSIOCSNDFLUSH_IN6:		/* FALLTHROUGH */
	case OSIOCSPFXFLUSH_IN6:	/* FALLTHROUGH */
	case OSIOCSRTRFLUSH_IN6:	/* FALLTHROUGH */
		break;
	case OSIOCGDEFIFACE_IN6:
		ndif->ifindex = 0;
		break;
	case OSIOCSDEFIFACE_IN6:
		error = ENOTSUP;
		break;
#endif
#ifdef OSIOCGIFINFO_IN6
	case OSIOCGIFINFO_IN6:		/* FALLTHROUGH */
#endif
#ifdef OSIOCGIFINFO_IN6_90
	case OSIOCGIFINFO_IN6_90:
		memset(&OND, 0, sizeof(OND));
		OND.initialized = 1;
		OND.chlim = ifndi->chlim;
		OND.basereachable = ifndi->basereachable;
		OND.retrans = ifndi->retrans;
		OND.flags = ifndi->flags;
		break;
	case OSIOCSIFINFO_IN6_90:
		/* Allow userland to set Neighour Unreachability Detection
		 * timers. */
		if (OND.chlim != 0)
			ifndi->chlim = OND.chlim;
		if (OND.basereachable != 0 &&
		    OND.basereachable != ifndi->basereachable)
		{
			ifndi->basereachable = OND.basereachable;
			ifndi->reachable = ND_COMPUTE_RTIME(OND.basereachable);
		}
		if (OND.retrans != 0)
			ifndi->retrans = OND.retrans;
		/* Retain the old behaviour .... */
		/* FALLTHROUGH */
	case OSIOCSIFINFO_FLAGS_90:
		nd6_setifflags(ifp, OND.flags);
		break;
#undef OND
#endif
	case SIOCGIFINFO_IN6:
		ND.chlim = ifndi->chlim;
		ND.basereachable = ifndi->basereachable;
		ND.retrans = ifndi->retrans;
		ND.flags = ifndi->flags;
		break;
	case SIOCSIFINFO_IN6:
		/* Allow userland to set Neighour Unreachability Detection
		 * timers. */
		if (ND.chlim != 0)
			ifndi->chlim = ND.chlim;
		if (ND.basereachable != 0 &&
		    ND.basereachable != ifndi->basereachable)
		{
			ifndi->basereachable = ND.basereachable;
			ifndi->reachable = ND_COMPUTE_RTIME(ND.basereachable);
		}
		if (ND.retrans != 0)
			ifndi->retrans = ND.retrans;
		break;
	case SIOCSIFINFO_FLAGS:
		nd6_setifflags(ifp, ND.flags);
		break;
#undef ND
	case SIOCGNBRINFO_IN6:
	{
		struct llentry *ln;
		struct in6_addr nb_addr = nbi->addr; /* make local for safety */

		if ((error = in6_setscope(&nb_addr, ifp, NULL)) != 0)
			return error;

		ln = nd6_lookup(&nb_addr, ifp, false);
		if (ln == NULL) {
			error = EINVAL;
			break;
		}
		nbi->state = ln->ln_state;
		nbi->asked = ln->ln_asked;
		nbi->isrouter = ln->ln_router;
		nbi->expire = ln->ln_expire ?
		    time_mono_to_wall(ln->ln_expire) : 0;
		LLE_RUNLOCK(ln);

		break;
	}
	}
	return error;
}

void
nd6_llinfo_release_pkts(struct llentry *ln, struct ifnet *ifp)
{
	struct mbuf *m_hold, *m_hold_next;
	struct sockaddr_in6 sin6;

	LLE_WLOCK_ASSERT(ln);

	sockaddr_in6_init(&sin6, &ln->r_l3addr.addr6, 0, 0, 0);

	m_hold = ln->la_hold, ln->la_hold = NULL, ln->la_numheld = 0;

	LLE_ADDREF(ln);
	LLE_WUNLOCK(ln);
	for (; m_hold != NULL; m_hold = m_hold_next) {
		m_hold_next = m_hold->m_nextpkt;
		m_hold->m_nextpkt = NULL;

		/*
		 * we assume ifp is not a p2p here, so
		 * just set the 2nd argument as the 
		 * 1st one.
		 */
		ip6_if_output(ifp, ifp, m_hold, &sin6, NULL);
	}
	LLE_WLOCK(ln);
	LLE_REMREF(ln);
}

/*
 * Create neighbor cache entry and cache link-layer address,
 * on reception of inbound ND6 packets.  (RS/RA/NS/redirect)
 */
void
nd6_cache_lladdr(
    struct ifnet *ifp,
    struct in6_addr *from,
    char *lladdr,
    int lladdrlen,
    int type,	/* ICMP6 type */
    int code	/* type dependent information */
)
{
	struct llentry *ln = NULL;
	int is_newentry;
	int do_update;
	int olladdr;
	int llchange;
	int newstate = 0;

	KASSERT(ifp != NULL);
	KASSERT(from != NULL);

	/* nothing must be updated for unspecified address */
	if (IN6_IS_ADDR_UNSPECIFIED(from))
		return;

	/*
	 * Validation about ifp->if_addrlen and lladdrlen must be done in
	 * the caller.
	 *
	 * XXX If the link does not have link-layer adderss, what should
	 * we do? (ifp->if_addrlen == 0)
	 * Spec says nothing in sections for RA, RS and NA.  There's small
	 * description on it in NS section (RFC 2461 7.2.3).
	 */

	ln = nd6_lookup(from, ifp, true);
	if (ln == NULL) {
#if 0
		/* nothing must be done if there's no lladdr */
		if (!lladdr || !lladdrlen)
			return NULL;
#endif

		ln = nd6_create(from, ifp);
		is_newentry = 1;
	} else {
		/* do nothing if static ndp is set */
		if (ln->la_flags & LLE_STATIC) {
			LLE_WUNLOCK(ln);
			return;
		}
		is_newentry = 0;
	}

	if (ln == NULL)
		return;

	olladdr = (ln->la_flags & LLE_VALID) ? 1 : 0;
	if (olladdr && lladdr) {
		llchange = memcmp(lladdr, &ln->ll_addr, ifp->if_addrlen);
	} else
		llchange = 0;

	/*
	 * newentry olladdr  lladdr  llchange	(*=record)
	 *	0	n	n	--	(1)
	 *	0	y	n	--	(2)
	 *	0	n	y	--	(3) * STALE
	 *	0	y	y	n	(4) *
	 *	0	y	y	y	(5) * STALE
	 *	1	--	n	--	(6)   NOSTATE(= PASSIVE)
	 *	1	--	y	--	(7) * STALE
	 */

	if (lladdr) {		/* (3-5) and (7) */
		/*
		 * Record source link-layer address
		 * XXX is it dependent to ifp->if_type?
		 */
		memcpy(&ln->ll_addr, lladdr, ifp->if_addrlen);
		ln->la_flags |= LLE_VALID;
	}

	if (!is_newentry) {
		if ((!olladdr && lladdr) ||		/* (3) */
		    (olladdr && lladdr && llchange)) {	/* (5) */
			do_update = 1;
			newstate = ND_LLINFO_STALE;
		} else					/* (1-2,4) */
			do_update = 0;
	} else {
		do_update = 1;
		if (lladdr == NULL)			/* (6) */
			newstate = ND_LLINFO_NOSTATE;
		else					/* (7) */
			newstate = ND_LLINFO_STALE;
	}

	if (do_update) {
		/*
		 * Update the state of the neighbor cache.
		 */
		ln->ln_state = newstate;

		if (ln->ln_state == ND_LLINFO_STALE) {
			/*
			 * XXX: since nd6_output() below will cause
			 * state tansition to DELAY and reset the timer,
			 * we must set the timer now, although it is actually
			 * meaningless.
			 */
			nd_set_timer(ln, ND_TIMER_GC);

			nd6_llinfo_release_pkts(ln, ifp);
		} else if (ln->ln_state == ND_LLINFO_INCOMPLETE) {
			/* probe right away */
			nd_set_timer(ln, ND_TIMER_IMMEDIATE);
		}
	}

	/*
	 * ICMP6 type dependent behavior.
	 *
	 * NS: clear IsRouter if new entry
	 * RS: clear IsRouter
	 * RA: set IsRouter if there's lladdr
	 * redir: clear IsRouter if new entry
	 *
	 * RA case, (1):
	 * The spec says that we must set IsRouter in the following cases:
	 * - If lladdr exist, set IsRouter.  This means (1-5).
	 * - If it is old entry (!newentry), set IsRouter.  This means (7).
	 * So, based on the spec, in (1-5) and (7) cases we must set IsRouter.
	 * A question arises for (1) case.  (1) case has no lladdr in the
	 * neighbor cache, this is similar to (6).
	 * This case is rare but we figured that we MUST NOT set IsRouter.
	 *
	 * newentry olladdr  lladdr  llchange	    NS  RS  RA	redir
	 *							D R
	 *	0	n	n	--	(1)	c   ?     s
	 *	0	y	n	--	(2)	c   s     s
	 *	0	n	y	--	(3)	c   s     s
	 *	0	y	y	n	(4)	c   s     s
	 *	0	y	y	y	(5)	c   s     s
	 *	1	--	n	--	(6) c	c 	c s
	 *	1	--	y	--	(7) c	c   s	c s
	 *
	 *					(c=clear s=set)
	 */
	switch (type & 0xff) {
	case ND_NEIGHBOR_SOLICIT:
		/*
		 * New entry must have is_router flag cleared.
		 */
		if (is_newentry)	/* (6-7) */
			ln->ln_router = 0;
		break;
	case ND_REDIRECT:
		/*
		 * If the icmp is a redirect to a better router, always set the
		 * is_router flag.  Otherwise, if the entry is newly created,
		 * clear the flag.  [RFC 2461, sec 8.3]
		 */
		if (code == ND_REDIRECT_ROUTER)
			ln->ln_router = 1;
		else if (is_newentry) /* (6-7) */
			ln->ln_router = 0;
		break;
	case ND_ROUTER_SOLICIT:
		/*
		 * is_router flag must always be cleared.
		 */
		ln->ln_router = 0;
		break;
	case ND_ROUTER_ADVERT:
		/*
		 * Mark an entry with lladdr as a router.
		 */
		if ((!is_newentry && (olladdr || lladdr)) ||	/* (2-5) */
		    (is_newentry && lladdr)) {			/* (7) */
			ln->ln_router = 1;
		}
		break;
	}

	if (do_update && lladdr != NULL) {
		struct sockaddr_in6 sin6;

		sockaddr_in6_init(&sin6, from, 0, 0, 0);
		rt_clonedmsg(is_newentry ? RTM_ADD : RTM_CHANGE,
		    NULL, sin6tosa(&sin6), lladdr, ifp);
	}

	if (ln != NULL)
		LLE_WUNLOCK(ln);

	/*
	 * If we have too many cache entries, initiate immediate
	 * purging for some entries.
	 */
	if (is_newentry)
		nd6_gc_neighbors(LLTABLE6(ifp), &ln->r_l3addr.addr6);
}

static void
nd6_slowtimo(void *ignored_arg)
{
	struct nd_kifinfo *ndi;
	struct ifnet *ifp;
	int s;

	SOFTNET_KERNEL_LOCK_UNLESS_NET_MPSAFE();
	callout_reset(&nd6_slowtimo_ch, ND6_SLOWTIMER_INTERVAL * hz,
	    nd6_slowtimo, NULL);

	s = pserialize_read_enter();
	IFNET_READER_FOREACH(ifp) {
		ndi = ND_IFINFO(ifp);
		if (ndi->basereachable && /* already initialized */
		    (ndi->recalctm -= ND6_SLOWTIMER_INTERVAL) <= 0) {
			/*
			 * Since reachable time rarely changes by router
			 * advertisements, we SHOULD insure that a new random
			 * value gets recomputed at least once every few hours.
			 * (RFC 2461, 6.3.4)
			 */
			ndi->recalctm = nd6_recalc_reachtm_interval;
			ndi->reachable = ND_COMPUTE_RTIME(ndi->basereachable);
		}
	}
	pserialize_read_exit(s);

	SOFTNET_KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
}

/*
 * Return 0 if a neighbor cache is found. Return EWOULDBLOCK if a cache is not
 * found and trying to resolve a neighbor; in this case the mbuf is queued in
 * the list. Otherwise return errno after freeing the mbuf.
 */
int
nd6_resolve(struct ifnet *ifp, const struct rtentry *rt, struct mbuf *m,
    const struct sockaddr *_dst, uint8_t *lldst, size_t dstsize)
{
	struct llentry *ln = NULL;
	bool created = false;
	const struct sockaddr_in6 *dst = satocsin6(_dst);
	int error;
	struct nd_kifinfo *ndi = ND_IFINFO(ifp);

	/* discard the packet if IPv6 operation is disabled on the interface */
	if (ndi->flags & ND6_IFF_IFDISABLED) {
		m_freem(m);
		return ENETDOWN; /* better error? */
	}

	/*
	 * Address resolution or Neighbor Unreachability Detection
	 * for the next hop.
	 * At this point, the destination of the packet must be a unicast
	 * or an anycast address(i.e. not a multicast).
	 */

	/* Look up the neighbor cache for the nexthop */
	ln = nd6_lookup(&dst->sin6_addr, ifp, false);

	if (ln != NULL && (ln->la_flags & LLE_VALID) != 0 &&
	    ln->ln_state == ND_LLINFO_REACHABLE) {
		/* Fast path */
		memcpy(lldst, &ln->ll_addr, MIN(dstsize, ifp->if_addrlen));
		LLE_RUNLOCK(ln);
		return 0;
	}
	if (ln != NULL)
		LLE_RUNLOCK(ln);

	/* Slow path */
	ln = nd6_lookup(&dst->sin6_addr, ifp, true);
	if (ln == NULL && nd6_is_addr_neighbor(dst, ifp))  {
		/*
		 * Since nd6_is_addr_neighbor() internally calls nd6_lookup(),
		 * the condition below is not very efficient.  But we believe
		 * it is tolerable, because this should be a rare case.
		 */
		ln = nd6_create(&dst->sin6_addr, ifp);
		if (ln == NULL) {
			char ip6buf[INET6_ADDRSTRLEN];
			log(LOG_DEBUG,
			    "%s: can't allocate llinfo for %s "
			    "(ln=%p, rt=%p)\n", __func__,
			    IN6_PRINT(ip6buf, &dst->sin6_addr), ln, rt);
			m_freem(m);
			return ENOBUFS;
		}
		created = true;
	}

	if (ln == NULL) {
		m_freem(m);
		return ENETDOWN; /* better error? */
	}

	error = nd_resolve(ln, rt, m, lldst, dstsize);

	if (created)
		nd6_gc_neighbors(LLTABLE6(ifp), &dst->sin6_addr);

	return error;
}

int
nd6_need_cache(struct ifnet *ifp)
{
	/*
	 * XXX: we currently do not make neighbor cache on any interface
	 * other than ARCnet, Ethernet, and GIF.
	 *
	 * RFC2893 says:
	 * - unidirectional tunnels needs no ND
	 */
	switch (ifp->if_type) {
	case IFT_ARCNET:
	case IFT_ETHER:
	case IFT_IEEE1394:
	case IFT_CARP:
	case IFT_GIF:		/* XXX need more cases? */
	case IFT_PPP:
	case IFT_TUNNEL:
		return 1;
	default:
		return 0;
	}
}

int
nd6_sysctl(
    int name,
    void *oldp,	/* syscall arg, need copyout */
    size_t *oldlenp,
    void *newp,	/* syscall arg, need copyin */
    size_t newlen
)
{

	if (newp)
		return EPERM;

	switch (name) {
#ifdef COMPAT_90
	case OICMPV6CTL_ND6_DRLIST: /* FALLTHROUGH */
	case OICMPV6CTL_ND6_PRLIST:
		*oldlenp = 0;
		return 0;
#endif
	case ICMPV6CTL_ND6_MAXQLEN:
		return 0;
	default:
		return ENOPROTOOPT;
	}
}
