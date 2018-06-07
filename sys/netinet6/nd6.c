/*	$NetBSD: nd6.c,v 1.232.2.8 2018/06/07 17:48:31 martin Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: nd6.c,v 1.232.2.8 2018/06/07 17:48:31 martin Exp $");

#ifdef _KERNEL_OPT
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
#include <net/route.h>
#include <net/if_ether.h>
#include <net/if_fddi.h>
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

#include <net/net_osdep.h>

#define ND6_SLOWTIMER_INTERVAL (60 * 60) /* 1 hour */
#define ND6_RECALC_REACHTM_INTERVAL (60 * 120) /* 2 hours */

/* timer values */
int	nd6_prune	= 1;	/* walk list every 1 seconds */
int	nd6_delay	= 5;	/* delay first probe time 5 second */
int	nd6_umaxtries	= 3;	/* maximum unicast query */
int	nd6_mmaxtries	= 3;	/* maximum multicast query */
int	nd6_useloopback = 1;	/* use loopback interface for local traffic */
int	nd6_gctimer	= (60 * 60 * 24); /* 1 day: garbage collection timer */

/* preventing too many loops in ND option parsing */
int nd6_maxndopt = 10;	/* max # of ND options allowed */

int nd6_maxnudhint = 0;	/* max # of subsequent upper layer hints */

int nd6_maxqueuelen = 1; /* max # of packets cached in unresolved ND entries */

#ifdef ND6_DEBUG
int nd6_debug = 1;
#else
int nd6_debug = 0;
#endif

krwlock_t nd6_lock __cacheline_aligned;

struct nd_drhead nd_defrouter;
struct nd_prhead nd_prefix = { 0 };

int nd6_recalc_reachtm_interval = ND6_RECALC_REACHTM_INTERVAL;

static void nd6_setmtu0(struct ifnet *, struct nd_ifinfo *);
static void nd6_slowtimo(void *);
static int regen_tmpaddr(const struct in6_ifaddr *);
static void nd6_free(struct llentry *, int);
static void nd6_llinfo_timer(void *);
static void nd6_timer(void *);
static void nd6_timer_work(struct work *, void *);
static void clear_llinfo_pqueue(struct llentry *);
static struct nd_opt_hdr *nd6_option(union nd_opts *);

static callout_t nd6_slowtimo_ch;
static callout_t nd6_timer_ch;
static struct workqueue	*nd6_timer_wq;
static struct work	nd6_timer_wk;

static int fill_drlist(void *, size_t *);
static int fill_prlist(void *, size_t *);

static struct ifnet *nd6_defifp;
static int nd6_defifindex;

static int nd6_setdefaultiface(int);

MALLOC_DEFINE(M_IP6NDP, "NDP", "IPv6 Neighbour Discovery");

void
nd6_init(void)
{
	int error;

	rw_init(&nd6_lock);

	/* initialization of the default router list */
	ND_DEFROUTER_LIST_INIT();

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

struct nd_ifinfo *
nd6_ifattach(struct ifnet *ifp)
{
	struct nd_ifinfo *nd;

	nd = kmem_zalloc(sizeof(*nd), KM_SLEEP);

	nd->initialized = 1;

	nd->chlim = IPV6_DEFHLIM;
	nd->basereachable = REACHABLE_TIME;
	nd->reachable = ND_COMPUTE_RTIME(nd->basereachable);
	nd->retrans = RETRANS_TIMER;

	nd->flags = ND6_IFF_PERFORMNUD | ND6_IFF_ACCEPT_RTADV;

	/* A loopback interface always has ND6_IFF_AUTO_LINKLOCAL.
	 * A bridge interface should not have ND6_IFF_AUTO_LINKLOCAL
	 * because one of its members should. */
	if ((ip6_auto_linklocal && ifp->if_type != IFT_BRIDGE) ||
	    (ifp->if_flags & IFF_LOOPBACK))
		nd->flags |= ND6_IFF_AUTO_LINKLOCAL;

	/* A loopback interface does not need to accept RTADV.
	 * A bridge interface should not accept RTADV
	 * because one of its members should. */
	if (ip6_accept_rtadv &&
	    !(ifp->if_flags & IFF_LOOPBACK) &&
	    !(ifp->if_type != IFT_BRIDGE))
		nd->flags |= ND6_IFF_ACCEPT_RTADV;

	/* XXX: we cannot call nd6_setmtu since ifp is not fully initialized */
	nd6_setmtu0(ifp, nd);

	return nd;
}

void
nd6_ifdetach(struct ifnet *ifp, struct in6_ifextra *ext)
{

	/* Ensure all IPv6 addresses are purged before calling nd6_purge */
	if_purgeaddrs(ifp, AF_INET6, in6_purgeaddr);
	nd6_purge(ifp, ext);
	kmem_free(ext->nd_ifinfo, sizeof(struct nd_ifinfo));
}

void
nd6_setmtu(struct ifnet *ifp)
{
	nd6_setmtu0(ifp, ND_IFINFO(ifp));
}

void
nd6_setmtu0(struct ifnet *ifp, struct nd_ifinfo *ndi)
{
	u_int32_t omaxmtu;

	omaxmtu = ndi->maxmtu;

	switch (ifp->if_type) {
	case IFT_ARCNET:
		ndi->maxmtu = MIN(ARC_PHDS_MAXMTU, ifp->if_mtu); /* RFC2497 */
		break;
	case IFT_FDDI:
		ndi->maxmtu = MIN(FDDIIPMTU, ifp->if_mtu);
		break;
	default:
		ndi->maxmtu = ifp->if_mtu;
		break;
	}

	/*
	 * Decreasing the interface MTU under IPV6 minimum MTU may cause
	 * undesirable situation.  We thus notify the operator of the change
	 * explicitly.  The check for omaxmtu is necessary to restrict the
	 * log to the case of changing the MTU, not initializing it.
	 */
	if (omaxmtu >= IPV6_MMTU && ndi->maxmtu < IPV6_MMTU) {
		log(LOG_NOTICE, "nd6_setmtu0: new link MTU on %s (%lu) is too"
		    " small for IPv6 which needs %lu\n",
		    if_name(ifp), (unsigned long)ndi->maxmtu, (unsigned long)
		    IPV6_MMTU);
	}

	if (ndi->maxmtu > in6_maxmtu)
		in6_setmaxmtu(); /* check all interfaces just in case */
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
 * ND6 timer routine to handle ND6 entries
 */
void
nd6_llinfo_settimer(struct llentry *ln, time_t xtick)
{

	CTASSERT(sizeof(time_t) > sizeof(int));
	LLE_WLOCK_ASSERT(ln);

	KASSERT(xtick >= 0);

	/*
	 * We have to take care of a reference leak which occurs if
	 * callout_reset overwrites a pending callout schedule.  Unfortunately
	 * we don't have a mean to know the overwrite, so we need to know it
	 * using callout_stop.  We need to call callout_pending first to exclude
	 * the case that the callout has never been scheduled.
	 */
	if (callout_pending(&ln->la_timer)) {
		bool expired = callout_stop(&ln->la_timer);
		if (!expired)
			LLE_REMREF(ln);
	}

	ln->ln_expire = time_uptime + xtick / hz;
	LLE_ADDREF(ln);
	if (xtick > INT_MAX) {
		ln->ln_ntick = xtick - INT_MAX;
		callout_reset(&ln->ln_timer_ch, INT_MAX,
		    nd6_llinfo_timer, ln);
	} else {
		ln->ln_ntick = 0;
		callout_reset(&ln->ln_timer_ch, xtick,
		    nd6_llinfo_timer, ln);
	}
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

static void
nd6_llinfo_timer(void *arg)
{
	struct llentry *ln = arg;
	struct ifnet *ifp;
	struct nd_ifinfo *ndi = NULL;
	bool send_ns = false;
	const struct in6_addr *daddr6 = NULL;

	SOFTNET_KERNEL_LOCK_UNLESS_NET_MPSAFE();

	LLE_WLOCK(ln);
	if ((ln->la_flags & LLE_LINKED) == 0)
		goto out;
	if (ln->ln_ntick > 0) {
		nd6_llinfo_settimer(ln, ln->ln_ntick);
		goto out;
	}


	ifp = ln->lle_tbl->llt_ifp;
	KASSERT(ifp != NULL);

	ndi = ND_IFINFO(ifp);

	switch (ln->ln_state) {
	case ND6_LLINFO_INCOMPLETE:
		if (ln->ln_asked < nd6_mmaxtries) {
			ln->ln_asked++;
			send_ns = true;
		} else {
			struct mbuf *m = ln->ln_hold;
			if (m) {
				struct mbuf *m0;

				/*
				 * assuming every packet in ln_hold has
				 * the same IP header
				 */
				m0 = m->m_nextpkt;
				m->m_nextpkt = NULL;
				ln->ln_hold = m0;
				clear_llinfo_pqueue(ln);
 			}
			nd6_free(ln, 0);
			ln = NULL;
			if (m != NULL) {
				icmp6_error2(m, ICMP6_DST_UNREACH,
				    ICMP6_DST_UNREACH_ADDR, 0, ifp);
			}
		}
		break;
	case ND6_LLINFO_REACHABLE:
		if (!ND6_LLINFO_PERMANENT(ln)) {
			ln->ln_state = ND6_LLINFO_STALE;
			nd6_llinfo_settimer(ln, nd6_gctimer * hz);
		}
		break;

	case ND6_LLINFO_PURGE:
	case ND6_LLINFO_STALE:
		/* Garbage Collection(RFC 2461 5.3) */
		if (!ND6_LLINFO_PERMANENT(ln)) {
			nd6_free(ln, 1);
			ln = NULL;
		}
		break;

	case ND6_LLINFO_DELAY:
		if (ndi && (ndi->flags & ND6_IFF_PERFORMNUD) != 0) {
			/* We need NUD */
			ln->ln_asked = 1;
			ln->ln_state = ND6_LLINFO_PROBE;
			daddr6 = &ln->r_l3addr.addr6;
			send_ns = true;
		} else {
			ln->ln_state = ND6_LLINFO_STALE; /* XXX */
			nd6_llinfo_settimer(ln, nd6_gctimer * hz);
		}
		break;
	case ND6_LLINFO_PROBE:
		if (ln->ln_asked < nd6_umaxtries) {
			ln->ln_asked++;
			daddr6 = &ln->r_l3addr.addr6;
			send_ns = true;
		} else {
			nd6_free(ln, 0);
			ln = NULL;
		}
		break;
	}

	if (send_ns) {
		struct in6_addr src, *psrc;
		const struct in6_addr *taddr6 = &ln->r_l3addr.addr6;

		nd6_llinfo_settimer(ln, ndi->retrans * hz / 1000);
		psrc = nd6_llinfo_get_holdsrc(ln, &src);
		LLE_FREE_LOCKED(ln);
		ln = NULL;
		nd6_ns_output(ifp, daddr6, taddr6, psrc, 0);
	}

out:
	if (ln != NULL)
		LLE_FREE_LOCKED(ln);
	SOFTNET_KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
}

/*
 * ND6 timer routine to expire default route list and prefix list
 */
static void
nd6_timer_work(struct work *wk, void *arg)
{
	struct nd_defrouter *next_dr, *dr;
	struct nd_prefix *next_pr, *pr;
	struct in6_ifaddr *ia6, *nia6;
	int s, bound;
	struct psref psref;

	callout_reset(&nd6_timer_ch, nd6_prune * hz,
	    nd6_timer, NULL);

	SOFTNET_KERNEL_LOCK_UNLESS_NET_MPSAFE();

	/* expire default router list */

	ND6_WLOCK();
	ND_DEFROUTER_LIST_FOREACH_SAFE(dr, next_dr) {
		if (dr->expire && dr->expire < time_uptime) {
			nd6_defrtrlist_del(dr, NULL);
		}
	}
	ND6_UNLOCK();

	/*
	 * expire interface addresses.
	 * in the past the loop was inside prefix expiry processing.
	 * However, from a stricter speci-confrmance standpoint, we should
	 * rather separate address lifetimes and prefix lifetimes.
	 */
	bound = curlwp_bind();
  addrloop:
	s = pserialize_read_enter();
	for (ia6 = IN6_ADDRLIST_READER_FIRST(); ia6; ia6 = nia6) {
		nia6 = IN6_ADDRLIST_READER_NEXT(ia6);

		ia6_acquire(ia6, &psref);
		pserialize_read_exit(s);

		/* check address lifetime */
		if (IFA6_IS_INVALID(ia6)) {
			int regen = 0;
			struct ifnet *ifp;

			/*
			 * If the expiring address is temporary, try
			 * regenerating a new one.  This would be useful when
			 * we suspended a laptop PC, then turned it on after a
			 * period that could invalidate all temporary
			 * addresses.  Although we may have to restart the
			 * loop (see below), it must be after purging the
			 * address.  Otherwise, we'd see an infinite loop of
			 * regeneration.
			 */
			if (ip6_use_tempaddr &&
			    (ia6->ia6_flags & IN6_IFF_TEMPORARY) != 0) {
				IFNET_LOCK(ia6->ia_ifa.ifa_ifp);
				if (regen_tmpaddr(ia6) == 0)
					regen = 1;
				IFNET_UNLOCK(ia6->ia_ifa.ifa_ifp);
			}

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

			if (regen)
				goto addrloop; /* XXX: see below */
		} else if (IFA6_IS_DEPRECATED(ia6)) {
			int oldflags = ia6->ia6_flags;

			if ((oldflags & IN6_IFF_DEPRECATED) == 0) {
				ia6->ia6_flags |= IN6_IFF_DEPRECATED;
				rt_newaddrmsg(RTM_NEWADDR,
				    (struct ifaddr *)ia6, 0, NULL);
			}

			/*
			 * If a temporary address has just become deprecated,
			 * regenerate a new one if possible.
			 */
			if (ip6_use_tempaddr &&
			    (ia6->ia6_flags & IN6_IFF_TEMPORARY) != 0 &&
			    (oldflags & IN6_IFF_DEPRECATED) == 0) {

				if (regen_tmpaddr(ia6) == 0) {
					/*
					 * A new temporary address is
					 * generated.
					 * XXX: this means the address chain
					 * has changed while we are still in
					 * the loop.  Although the change
					 * would not cause disaster (because
					 * it's not a deletion, but an
					 * addition,) we'd rather restart the
					 * loop just for safety.  Or does this
					 * significantly reduce performance??
					 */
					ia6_release(ia6, &psref);
					goto addrloop;
				}
			}
		} else {
			/*
			 * A new RA might have made a deprecated address
			 * preferred.
			 */
			if (ia6->ia6_flags & IN6_IFF_DEPRECATED) {
				ia6->ia6_flags &= ~IN6_IFF_DEPRECATED;
				rt_newaddrmsg(RTM_NEWADDR,
				    (struct ifaddr *)ia6, 0, NULL);
			}
		}
		s = pserialize_read_enter();
		ia6_release(ia6, &psref);
	}
	pserialize_read_exit(s);
	curlwp_bindx(bound);

	/* expire prefix list */
	ND6_WLOCK();
	ND_PREFIX_LIST_FOREACH_SAFE(pr, next_pr) {
		/*
		 * check prefix lifetime.
		 * since pltime is just for autoconf, pltime processing for
		 * prefix is not necessary.
		 */
		if (pr->ndpr_vltime != ND6_INFINITE_LIFETIME &&
		    time_uptime - pr->ndpr_lastupdate > pr->ndpr_vltime) {
			/*
			 * Just invalidate the prefix here. Removing it
			 * will be done when purging an associated address.
			 */
			KASSERT(pr->ndpr_refcnt > 0);
			nd6_invalidate_prefix(pr);
		}
	}
	ND6_UNLOCK();

	SOFTNET_KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
}

static void
nd6_timer(void *ignored_arg)
{

	workqueue_enqueue(nd6_timer_wq, &nd6_timer_wk, NULL);
}

/* ia6: deprecated/invalidated temporary address */
static int
regen_tmpaddr(const struct in6_ifaddr *ia6)
{
	struct ifaddr *ifa;
	struct ifnet *ifp;
	struct in6_ifaddr *public_ifa6 = NULL;
	int s;

	ifp = ia6->ia_ifa.ifa_ifp;
	s = pserialize_read_enter();
	IFADDR_READER_FOREACH(ifa, ifp) {
		struct in6_ifaddr *it6;

		if (ifa->ifa_addr->sa_family != AF_INET6)
			continue;

		it6 = (struct in6_ifaddr *)ifa;

		/* ignore no autoconf addresses. */
		if ((it6->ia6_flags & IN6_IFF_AUTOCONF) == 0)
			continue;

		/* ignore autoconf addresses with different prefixes. */
		if (it6->ia6_ndpr == NULL || it6->ia6_ndpr != ia6->ia6_ndpr)
			continue;

		/*
		 * Now we are looking at an autoconf address with the same
		 * prefix as ours.  If the address is temporary and is still
		 * preferred, do not create another one.  It would be rare, but
		 * could happen, for example, when we resume a laptop PC after
		 * a long period.
		 */
		if ((it6->ia6_flags & IN6_IFF_TEMPORARY) != 0 &&
		    !IFA6_IS_DEPRECATED(it6)) {
			public_ifa6 = NULL;
			break;
		}

		/*
		 * This is a public autoconf address that has the same prefix
		 * as ours.  If it is preferred, keep it.  We can't break the
		 * loop here, because there may be a still-preferred temporary
		 * address with the prefix.
		 */
		if (!IFA6_IS_DEPRECATED(it6))
			public_ifa6 = it6;
	}

	if (public_ifa6 != NULL) {
		int e;
		struct psref psref;

		ia6_acquire(public_ifa6, &psref);
		pserialize_read_exit(s);
		/*
		 * Random factor is introduced in the preferred lifetime, so
		 * we do not need additional delay (3rd arg to in6_tmpifadd).
		 */
		ND6_WLOCK();
		e = in6_tmpifadd(public_ifa6, 0, 0);
		ND6_UNLOCK();
		if (e != 0) {
			ia6_release(public_ifa6, &psref);
			log(LOG_NOTICE, "regen_tmpaddr: failed to create a new"
			    " tmp addr, errno=%d\n", e);
			return -1;
		}
		ia6_release(public_ifa6, &psref);
		return 0;
	}
	pserialize_read_exit(s);

	return -1;
}

bool
nd6_accepts_rtadv(const struct nd_ifinfo *ndi)
{
	switch (ndi->flags & (ND6_IFF_ACCEPT_RTADV|ND6_IFF_OVERRIDE_RTADV)) {
	case ND6_IFF_OVERRIDE_RTADV|ND6_IFF_ACCEPT_RTADV:
		return true;
	case ND6_IFF_ACCEPT_RTADV:
		return ip6_accept_rtadv != 0;
	case ND6_IFF_OVERRIDE_RTADV:
	case 0:
	default:
		return false;
	}
}

/*
 * Nuke neighbor cache/prefix/default router management table, right before
 * ifp goes away.
 */
void
nd6_purge(struct ifnet *ifp, struct in6_ifextra *ext)
{
	struct nd_defrouter *dr, *ndr;
	struct nd_prefix *pr, *npr;

	/*
	 * During detach, the ND info might be already removed, but
	 * then is explitly passed as argument.
	 * Otherwise get it from ifp->if_afdata.
	 */
	if (ext == NULL)
		ext = ifp->if_afdata[AF_INET6];
	if (ext == NULL)
		return;

	ND6_WLOCK();
	/*
	 * Nuke default router list entries toward ifp.
	 * We defer removal of default router list entries that is installed
	 * in the routing table, in order to keep additional side effects as
	 * small as possible.
	 */
	ND_DEFROUTER_LIST_FOREACH_SAFE(dr, ndr) {
		if (dr->installed)
			continue;

		if (dr->ifp == ifp) {
			KASSERT(ext != NULL);
			nd6_defrtrlist_del(dr, ext);
		}
	}

	ND_DEFROUTER_LIST_FOREACH_SAFE(dr, ndr) {
		if (!dr->installed)
			continue;

		if (dr->ifp == ifp) {
			KASSERT(ext != NULL);
			nd6_defrtrlist_del(dr, ext);
		}
	}

	/* Nuke prefix list entries toward ifp */
	ND_PREFIX_LIST_FOREACH_SAFE(pr, npr) {
		if (pr->ndpr_ifp == ifp) {
			/*
			 * All addresses referencing pr should be already freed.
			 */
			KASSERT(pr->ndpr_refcnt == 0);
			nd6_prelist_remove(pr);
		}
	}

	/* cancel default outgoing interface setting */
	if (nd6_defifindex == ifp->if_index)
		nd6_setdefaultiface(0);

	/* XXX: too restrictive? */
	if (!ip6_forwarding && ifp->if_afdata[AF_INET6]) {
		struct nd_ifinfo *ndi = ND_IFINFO(ifp);
		if (ndi && nd6_accepts_rtadv(ndi)) {
			/* refresh default router list */
			nd6_defrouter_select();
		}
	}
	ND6_UNLOCK();

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

void
nd6_assert_purged(struct ifnet *ifp)
{
	struct nd_defrouter *dr;
	struct nd_prefix *pr;
	char ip6buf[INET6_ADDRSTRLEN] __diagused;

	ND6_RLOCK();
	ND_DEFROUTER_LIST_FOREACH(dr) {
		KASSERTMSG(dr->ifp != ifp,
		    "defrouter %s remains on %s",
		    IN6_PRINT(ip6buf, &dr->rtaddr), ifp->if_xname);
	}

	ND_PREFIX_LIST_FOREACH(pr) {
		KASSERTMSG(pr->ndpr_ifp != ifp,
		    "prefix %s/%d remains on %s",
		    IN6_PRINT(ip6buf, &pr->ndpr_prefix.sin6_addr),
		    pr->ndpr_plen, ifp->if_xname);
	}
	ND6_UNLOCK();
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
		ln->ln_state = ND6_LLINFO_NOSTATE;

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
	struct nd_prefix *pr;
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
	 * If the address matches one of our addresses,
	 * it should be a neighbor.
	 * If the address matches one of our on-link prefixes, it should be a
	 * neighbor.
	 */
	ND6_RLOCK();
	ND_PREFIX_LIST_FOREACH(pr) {
		if (pr->ndpr_ifp != ifp)
			continue;

		if (!(pr->ndpr_stateflags & NDPRF_ONLINK)) {
			struct rtentry *rt;

			rt = rtalloc1(sin6tosa(&pr->ndpr_prefix), 0);
			if (rt == NULL)
				continue;
			/*
			 * This is the case where multiple interfaces
			 * have the same prefix, but only one is installed
			 * into the routing table and that prefix entry
			 * is not the one being examined here. In the case
			 * where RADIX_MPATH is enabled, multiple route
			 * entries (of the same rt_key value) will be
			 * installed because the interface addresses all
			 * differ.
			 */
			if (!IN6_ARE_ADDR_EQUAL(&pr->ndpr_prefix.sin6_addr,
			    &satocsin6(rt_getkey(rt))->sin6_addr)) {
				rt_unref(rt);
				continue;
			}
			rt_unref(rt);
		}

		if (IN6_ARE_MASKED_ADDR_EQUAL(&pr->ndpr_prefix.sin6_addr,
		    &addr->sin6_addr, &pr->ndpr_mask)) {
			ND6_UNLOCK();
			return 1;
		}
	}
	ND6_UNLOCK();

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

	/*
	 * If the default router list is empty, all addresses are regarded
	 * as on-link, and thus, as a neighbor.
	 */
	ND6_RLOCK();
	if (ND_IFINFO(ifp)->flags & ND6_IFF_ACCEPT_RTADV &&
	    ND_DEFROUTER_LIST_EMPTY() && nd6_defifindex == ifp->if_index) {
		ND6_UNLOCK();
		return 1;
	}
	ND6_UNLOCK();

	return 0;
}

/*
 * Detect if a given IPv6 address identifies a neighbor on a given link.
 * XXX: should take care of the destination of a p2p link?
 */
int
nd6_is_addr_neighbor(const struct sockaddr_in6 *addr, struct ifnet *ifp)
{
	struct nd_prefix *pr;
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

	/*
	 * If the address matches one of our on-link prefixes, it should be a
	 * neighbor.
	 */
	ND6_RLOCK();
	ND_PREFIX_LIST_FOREACH(pr) {
		if (pr->ndpr_ifp != ifp)
			continue;

		if (!(pr->ndpr_stateflags & NDPRF_ONLINK))
			continue;

		if (IN6_ARE_MASKED_ADDR_EQUAL(&pr->ndpr_prefix.sin6_addr,
		    &addr->sin6_addr, &pr->ndpr_mask)) {
			ND6_UNLOCK();
			return 1;
		}
	}

	/*
	 * If the default router list is empty, all addresses are regarded
	 * as on-link, and thus, as a neighbor.
	 * XXX: we restrict the condition to hosts, because routers usually do
	 * not have the "default router list".
	 */
	if (!ip6_forwarding && ND_DEFROUTER_LIST_EMPTY() &&
	    nd6_defifindex == ifp->if_index) {
		ND6_UNLOCK();
		return 1;
	}
	ND6_UNLOCK();

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
	struct nd_defrouter *dr;
	struct ifnet *ifp;
	struct in6_addr *in6;

	KASSERT(ln != NULL);
	LLE_WLOCK_ASSERT(ln);

	ifp = ln->lle_tbl->llt_ifp;
	in6 = &ln->r_l3addr.addr6;
	/*
	 * we used to have pfctlinput(PRC_HOSTDEAD) here.
	 * even though it is not harmful, it was not really necessary.
	 */

	if (!ip6_forwarding) {
		ND6_WLOCK();
		dr = nd6_defrouter_lookup(in6, ifp);

		if (dr != NULL && dr->expire &&
		    ln->ln_state == ND6_LLINFO_STALE && gc) {
			/*
			 * If the reason for the deletion is just garbage
			 * collection, and the neighbor is an active default
			 * router, do not delete it.  Instead, reset the GC
			 * timer using the router's lifetime.
			 * Simply deleting the entry would affect default
			 * router selection, which is not necessarily a good
			 * thing, especially when we're using router preference
			 * values.
			 * XXX: the check for ln_state would be redundant,
			 *      but we intentionally keep it just in case.
			 */
			if (dr->expire > time_uptime)
				nd6_llinfo_settimer(ln,
				    (dr->expire - time_uptime) * hz);
			else
				nd6_llinfo_settimer(ln, nd6_gctimer * hz);
			ND6_UNLOCK();
			LLE_WUNLOCK(ln);
			return;
		}

		if (ln->ln_router || dr) {
			/*
			 * We need to unlock to avoid a LOR with nd6_rt_flush()
			 * with the rnh and for the calls to
			 * nd6_pfxlist_onlink_check() and nd6_defrouter_select() in the
			 * block further down for calls into nd6_lookup().
			 * We still hold a ref.
			 */
			LLE_WUNLOCK(ln);

			/*
			 * nd6_rt_flush must be called whether or not the neighbor
			 * is in the Default Router List.
			 * See a corresponding comment in nd6_na_input().
			 */
			nd6_rt_flush(in6, ifp);
		}

		if (dr) {
			/*
			 * Unreachablity of a router might affect the default
			 * router selection and on-link detection of advertised
			 * prefixes.
			 */

			/*
			 * Temporarily fake the state to choose a new default
			 * router and to perform on-link determination of
			 * prefixes correctly.
			 * Below the state will be set correctly,
			 * or the entry itself will be deleted.
			 */
			ln->ln_state = ND6_LLINFO_INCOMPLETE;

			/*
			 * Since nd6_defrouter_select() does not affect the
			 * on-link determination and MIP6 needs the check
			 * before the default router selection, we perform
			 * the check now.
			 */
			nd6_pfxlist_onlink_check();

			/*
			 * refresh default router list
			 */
			nd6_defrouter_select();
		}

#ifdef __FreeBSD__
		/*
		 * If this entry was added by an on-link redirect, remove the
		 * corresponding host route.
		 */
		if (ln->la_flags & LLE_REDIRECT)
			nd6_free_redirect(ln);
#endif
		ND6_UNLOCK();

		if (ln->ln_router || dr)
			LLE_WLOCK(ln);
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
	if (ln == NULL)
		return;

	if (ln->ln_state < ND6_LLINFO_REACHABLE)
		goto done;

	/*
	 * if we get upper-layer reachability confirmation many times,
	 * it is possible we have false information.
	 */
	ln->ln_byhint++;
	if (ln->ln_byhint > nd6_maxnudhint)
		goto done;

	ln->ln_state = ND6_LLINFO_REACHABLE;
	if (!ND6_LLINFO_PERMANENT(ln))
		nd6_llinfo_settimer(ln, ND_IFINFO(rt->rt_ifp)->reachable * hz);

done:
	LLE_WUNLOCK(ln);

	return;
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

	if (ND6_LLINFO_PERMANENT(ln))
		return 0;

	if (IN6_ARE_ADDR_EQUAL(&ln->r_l3addr.addr6, skip_in6))
		return 0;

	LLE_WLOCK(ln);
	if (ln->ln_state > ND6_LLINFO_INCOMPLETE)
		ln->ln_state = ND6_LLINFO_STALE;
	else
		ln->ln_state = ND6_LLINFO_PURGE;
	nd6_llinfo_settimer(ln, 0);
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

	if ((rt->rt_flags & RTF_GATEWAY) != 0)
		return;

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
				if (ifa != rt->rt_ifa)
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

int
nd6_ioctl(u_long cmd, void *data, struct ifnet *ifp)
{
	struct in6_drlist *drl = (struct in6_drlist *)data;
	struct in6_oprlist *oprl = (struct in6_oprlist *)data;
	struct in6_ndireq *ndi = (struct in6_ndireq *)data;
	struct in6_nbrinfo *nbi = (struct in6_nbrinfo *)data;
	struct in6_ndifreq *ndif = (struct in6_ndifreq *)data;
	struct nd_defrouter *dr;
	struct nd_prefix *pr;
	int i = 0, error = 0;

	switch (cmd) {
	case SIOCGDRLST_IN6:
		/*
		 * obsolete API, use sysctl under net.inet6.icmp6
		 */
		memset(drl, 0, sizeof(*drl));
		ND6_RLOCK();
		ND_DEFROUTER_LIST_FOREACH(dr) {
			if (i >= DRLSTSIZ)
				break;
			drl->defrouter[i].rtaddr = dr->rtaddr;
			in6_clearscope(&drl->defrouter[i].rtaddr);

			drl->defrouter[i].flags = dr->flags;
			drl->defrouter[i].rtlifetime = dr->rtlifetime;
			drl->defrouter[i].expire = dr->expire ?
			    time_mono_to_wall(dr->expire) : 0;
			drl->defrouter[i].if_index = dr->ifp->if_index;
			i++;
		}
		ND6_UNLOCK();
		break;
	case SIOCGPRLST_IN6:
		/*
		 * obsolete API, use sysctl under net.inet6.icmp6
		 *
		 * XXX the structure in6_prlist was changed in backward-
		 * incompatible manner.  in6_oprlist is used for SIOCGPRLST_IN6,
		 * in6_prlist is used for nd6_sysctl() - fill_prlist().
		 */
		/*
		 * XXX meaning of fields, especialy "raflags", is very
		 * differnet between RA prefix list and RR/static prefix list.
		 * how about separating ioctls into two?
		 */
		memset(oprl, 0, sizeof(*oprl));
		ND6_RLOCK();
		ND_PREFIX_LIST_FOREACH(pr) {
			struct nd_pfxrouter *pfr;
			int j;

			if (i >= PRLSTSIZ)
				break;
			oprl->prefix[i].prefix = pr->ndpr_prefix.sin6_addr;
			oprl->prefix[i].raflags = pr->ndpr_raf;
			oprl->prefix[i].prefixlen = pr->ndpr_plen;
			oprl->prefix[i].vltime = pr->ndpr_vltime;
			oprl->prefix[i].pltime = pr->ndpr_pltime;
			oprl->prefix[i].if_index = pr->ndpr_ifp->if_index;
			if (pr->ndpr_vltime == ND6_INFINITE_LIFETIME)
				oprl->prefix[i].expire = 0;
			else {
				time_t maxexpire;

				/* XXX: we assume time_t is signed. */
				maxexpire = (-1) &
				    ~((time_t)1 <<
				    ((sizeof(maxexpire) * 8) - 1));
				if (pr->ndpr_vltime <
				    maxexpire - pr->ndpr_lastupdate) {
					time_t expire;
					expire = pr->ndpr_lastupdate +
					    pr->ndpr_vltime;
					oprl->prefix[i].expire = expire ?
					    time_mono_to_wall(expire) : 0;
				} else
					oprl->prefix[i].expire = maxexpire;
			}

			j = 0;
			LIST_FOREACH(pfr, &pr->ndpr_advrtrs, pfr_entry) {
				if (j < DRLSTSIZ) {
#define RTRADDR oprl->prefix[i].advrtr[j]
					RTRADDR = pfr->router->rtaddr;
					in6_clearscope(&RTRADDR);
#undef RTRADDR
				}
				j++;
			}
			oprl->prefix[i].advrtrs = j;
			oprl->prefix[i].origin = PR_ORIG_RA;

			i++;
		}
		ND6_UNLOCK();

		break;
	case OSIOCGIFINFO_IN6:
#define ND	ndi->ndi
		/* XXX: old ndp(8) assumes a positive value for linkmtu. */
		memset(&ND, 0, sizeof(ND));
		ND.linkmtu = IN6_LINKMTU(ifp);
		ND.maxmtu = ND_IFINFO(ifp)->maxmtu;
		ND.basereachable = ND_IFINFO(ifp)->basereachable;
		ND.reachable = ND_IFINFO(ifp)->reachable;
		ND.retrans = ND_IFINFO(ifp)->retrans;
		ND.flags = ND_IFINFO(ifp)->flags;
		ND.recalctm = ND_IFINFO(ifp)->recalctm;
		ND.chlim = ND_IFINFO(ifp)->chlim;
		break;
	case SIOCGIFINFO_IN6:
		ND = *ND_IFINFO(ifp);
		break;
	case SIOCSIFINFO_IN6:
		/* 
		 * used to change host variables from userland.
		 * intented for a use on router to reflect RA configurations.
		 */
		/* 0 means 'unspecified' */
		if (ND.linkmtu != 0) {
			if (ND.linkmtu < IPV6_MMTU ||
			    ND.linkmtu > IN6_LINKMTU(ifp)) {
				error = EINVAL;
				break;
			}
			ND_IFINFO(ifp)->linkmtu = ND.linkmtu;
		}

		if (ND.basereachable != 0) {
			int obasereachable = ND_IFINFO(ifp)->basereachable;

			ND_IFINFO(ifp)->basereachable = ND.basereachable;
			if (ND.basereachable != obasereachable)
				ND_IFINFO(ifp)->reachable =
				    ND_COMPUTE_RTIME(ND.basereachable);
		}
		if (ND.retrans != 0)
			ND_IFINFO(ifp)->retrans = ND.retrans;
		if (ND.chlim != 0)
			ND_IFINFO(ifp)->chlim = ND.chlim;
		/* FALLTHROUGH */
	case SIOCSIFINFO_FLAGS:
	{
		struct ifaddr *ifa;
		struct in6_ifaddr *ia;
		int s;

		if ((ND_IFINFO(ifp)->flags & ND6_IFF_IFDISABLED) &&
		    !(ND.flags & ND6_IFF_IFDISABLED))
		{
			/*
			 * If the interface is marked as ND6_IFF_IFDISABLED and
			 * has a link-local address with IN6_IFF_DUPLICATED,
			 * do not clear ND6_IFF_IFDISABLED.
			 * See RFC 4862, section 5.4.5.
			 */
			int duplicated_linklocal = 0;

			s = pserialize_read_enter();
			IFADDR_READER_FOREACH(ifa, ifp) {
				if (ifa->ifa_addr->sa_family != AF_INET6)
					continue;
				ia = (struct in6_ifaddr *)ifa;
				if ((ia->ia6_flags & IN6_IFF_DUPLICATED) &&
				    IN6_IS_ADDR_LINKLOCAL(IA6_IN6(ia)))
				{
					duplicated_linklocal = 1;
					break;
				}
			}
			pserialize_read_exit(s);

			if (duplicated_linklocal) {
				ND.flags |= ND6_IFF_IFDISABLED;
				log(LOG_ERR, "Cannot enable an interface"
				    " with a link-local address marked"
				    " duplicate.\n");
			} else {
				ND_IFINFO(ifp)->flags &= ~ND6_IFF_IFDISABLED;
				if (ifp->if_flags & IFF_UP)
					in6_if_up(ifp);
			}
		} else if (!(ND_IFINFO(ifp)->flags & ND6_IFF_IFDISABLED) &&
		    (ND.flags & ND6_IFF_IFDISABLED)) {
			int bound = curlwp_bind(); 
			/* Mark all IPv6 addresses as tentative. */

			ND_IFINFO(ifp)->flags |= ND6_IFF_IFDISABLED;
			s = pserialize_read_enter();
			IFADDR_READER_FOREACH(ifa, ifp) {
				struct psref psref;
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

		if (ND.flags & ND6_IFF_AUTO_LINKLOCAL) {
			if (!(ND_IFINFO(ifp)->flags & ND6_IFF_AUTO_LINKLOCAL)) {
				/* auto_linklocal 0->1 transition */

				ND_IFINFO(ifp)->flags |= ND6_IFF_AUTO_LINKLOCAL;
				in6_ifattach(ifp, NULL);
			} else if (!(ND.flags & ND6_IFF_IFDISABLED) &&
			    ifp->if_flags & IFF_UP)
			{
				/*
				 * When the IF already has
				 * ND6_IFF_AUTO_LINKLOCAL, no link-local
				 * address is assigned, and IFF_UP, try to
				 * assign one.
				 */
				int haslinklocal = 0;

				s = pserialize_read_enter();
				IFADDR_READER_FOREACH(ifa, ifp) {
					if (ifa->ifa_addr->sa_family !=AF_INET6)
						continue;
					ia = (struct in6_ifaddr *)ifa;
					if (IN6_IS_ADDR_LINKLOCAL(IA6_IN6(ia))){
						haslinklocal = 1;
						break;
					}
				}
				pserialize_read_exit(s);
				if (!haslinklocal)
					in6_ifattach(ifp, NULL);
			}
		}
	}
		ND_IFINFO(ifp)->flags = ND.flags;
		break;
#undef ND
	case SIOCSNDFLUSH_IN6:	/* XXX: the ioctl name is confusing... */
		/* sync kernel routing table with the default router list */
		ND6_WLOCK();
		nd6_defrouter_reset();
		nd6_defrouter_select();
		ND6_UNLOCK();
		break;
	case SIOCSPFXFLUSH_IN6:
	{
		/* flush all the prefix advertised by routers */
		struct nd_prefix *pfx, *next;

	restart:
		ND6_WLOCK();
		ND_PREFIX_LIST_FOREACH_SAFE(pfx, next) {
			struct in6_ifaddr *ia, *ia_next;
			int _s;

			if (IN6_IS_ADDR_LINKLOCAL(&pfx->ndpr_prefix.sin6_addr))
				continue; /* XXX */

			/* do we really have to remove addresses as well? */
			_s = pserialize_read_enter();
			for (ia = IN6_ADDRLIST_READER_FIRST(); ia;
			     ia = ia_next) {
				struct ifnet *ifa_ifp;
				int bound;
				struct psref psref;

				/* ia might be removed.  keep the next ptr. */
				ia_next = IN6_ADDRLIST_READER_NEXT(ia);

				if ((ia->ia6_flags & IN6_IFF_AUTOCONF) == 0)
					continue;

				if (ia->ia6_ndpr != pfx)
					continue;

				bound = curlwp_bind();
				ia6_acquire(ia, &psref);
				pserialize_read_exit(_s);
				ND6_UNLOCK();

				ifa_ifp = ia->ia_ifa.ifa_ifp;
				if (ifa_ifp == ifp) {
					/* Already have IFNET_LOCK(ifp) */
					KASSERT(!if_is_deactivated(ifp));
					ia6_release(ia, &psref);
					in6_purgeaddr(&ia->ia_ifa);
					curlwp_bindx(bound);
					goto restart;
				}
				IFNET_LOCK(ifa_ifp);
				/*
				 * Need to take the lock first to prevent
				 * if_detach from running in6_purgeaddr
				 * concurrently.
				 */
				if (!if_is_deactivated(ifa_ifp)) {
					ia6_release(ia, &psref);
					in6_purgeaddr(&ia->ia_ifa);
				} else {
					/*
					 * ifp is being destroyed, ia will be
					 * destroyed by if_detach.
					 */
					ia6_release(ia, &psref);
					/* XXX may cause busy loop */
				}
				IFNET_UNLOCK(ifa_ifp);
				curlwp_bindx(bound);
				goto restart;
			}
			pserialize_read_exit(_s);

			KASSERT(pfx->ndpr_refcnt == 0);
			nd6_prelist_remove(pfx);
		}
		ND6_UNLOCK();
		break;
	}
	case SIOCSRTRFLUSH_IN6:
	{
		/* flush all the default routers */
		struct nd_defrouter *drtr, *next;

		ND6_WLOCK();
		nd6_defrouter_reset();
		ND_DEFROUTER_LIST_FOREACH_SAFE(drtr, next) {
			nd6_defrtrlist_del(drtr, NULL);
		}
		nd6_defrouter_select();
		ND6_UNLOCK();
		break;
	}
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
	case SIOCGDEFIFACE_IN6:	/* XXX: should be implemented as a sysctl? */
		ndif->ifindex = nd6_defifindex;
		break;
	case SIOCSDEFIFACE_IN6:	/* XXX: should be implemented as a sysctl? */
		return nd6_setdefaultiface(ndif->ifindex);
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
	struct nd_ifinfo *ndi = ND_IFINFO(ifp);
	struct llentry *ln = NULL;
	int is_newentry;
	int do_update;
	int olladdr;
	int llchange;
	int newstate = 0;
	uint16_t router = 0;

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
			newstate = ND6_LLINFO_STALE;
		} else					/* (1-2,4) */
			do_update = 0;
	} else {
		do_update = 1;
		if (lladdr == NULL)			/* (6) */
			newstate = ND6_LLINFO_NOSTATE;
		else					/* (7) */
			newstate = ND6_LLINFO_STALE;
	}

	if (do_update) {
		/*
		 * Update the state of the neighbor cache.
		 */
		ln->ln_state = newstate;

		if (ln->ln_state == ND6_LLINFO_STALE) {
			/*
			 * XXX: since nd6_output() below will cause
			 * state tansition to DELAY and reset the timer,
			 * we must set the timer now, although it is actually
			 * meaningless.
			 */
			nd6_llinfo_settimer(ln, nd6_gctimer * hz);

			nd6_llinfo_release_pkts(ln, ifp);
		} else if (ln->ln_state == ND6_LLINFO_INCOMPLETE) {
			/* probe right away */
			nd6_llinfo_settimer((void *)ln, 0);
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
	 * A quetion arises for (1) case.  (1) case has no lladdr in the
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

#if 0
	/* XXX should we send rtmsg as it used to be? */
	if (do_update)
		rt_newmsg(RTM_CHANGE, rt);  /* tell user process */
#endif

	if (ln != NULL) {
		router = ln->ln_router;
		LLE_WUNLOCK(ln);
	}

	/*
	 * If we have too many cache entries, initiate immediate
	 * purging for some entries.
	 */
	if (is_newentry)
		nd6_gc_neighbors(LLTABLE6(ifp), &ln->r_l3addr.addr6);

	/*
	 * When the link-layer address of a router changes, select the
	 * best router again.  In particular, when the neighbor entry is newly
	 * created, it might affect the selection policy.
	 * Question: can we restrict the first condition to the "is_newentry"
	 * case?
	 * XXX: when we hear an RA from a new router with the link-layer
	 * address option, nd6_defrouter_select() is called twice, since
	 * defrtrlist_update called the function as well.  However, I believe
	 * we can compromise the overhead, since it only happens the first
	 * time.
	 * XXX: although nd6_defrouter_select() should not have a bad effect
	 * for those are not autoconfigured hosts, we explicitly avoid such
	 * cases for safety.
	 */
	if (do_update && router && !ip6_forwarding &&
	    nd6_accepts_rtadv(ndi)) {
		ND6_WLOCK();
		nd6_defrouter_select();
		ND6_UNLOCK();
	}
}

static void
nd6_slowtimo(void *ignored_arg)
{
	struct nd_ifinfo *nd6if;
	struct ifnet *ifp;
	int s;

	SOFTNET_KERNEL_LOCK_UNLESS_NET_MPSAFE();
	callout_reset(&nd6_slowtimo_ch, ND6_SLOWTIMER_INTERVAL * hz,
	    nd6_slowtimo, NULL);

	s = pserialize_read_enter();
	IFNET_READER_FOREACH(ifp) {
		nd6if = ND_IFINFO(ifp);
		if (nd6if->basereachable && /* already initialized */
		    (nd6if->recalctm -= ND6_SLOWTIMER_INTERVAL) <= 0) {
			/*
			 * Since reachable time rarely changes by router
			 * advertisements, we SHOULD insure that a new random
			 * value gets recomputed at least once every few hours.
			 * (RFC 2461, 6.3.4)
			 */
			nd6if->recalctm = nd6_recalc_reachtm_interval;
			nd6if->reachable = ND_COMPUTE_RTIME(nd6if->basereachable);
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

	/* discard the packet if IPv6 operation is disabled on the interface */
	if ((ND_IFINFO(ifp)->flags & ND6_IFF_IFDISABLED)) {
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

	if (ln != NULL && (ln->la_flags & LLE_VALID) != 0) {
		KASSERT(ln->ln_state > ND6_LLINFO_INCOMPLETE);
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
		struct sockaddr_in6 sin6;
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

		sockaddr_in6_init(&sin6, &ln->r_l3addr.addr6, 0, 0, 0);
		rt_clonedmsg(sin6tosa(&sin6), ifp, rt);

		created = true;
	}

	if (ln == NULL) {
		m_freem(m);
		return ENETDOWN; /* better error? */
	}

	LLE_WLOCK_ASSERT(ln);

	/* We don't have to do link-layer address resolution on a p2p link. */
	if ((ifp->if_flags & IFF_POINTOPOINT) != 0 &&
	    ln->ln_state < ND6_LLINFO_REACHABLE) {
		ln->ln_state = ND6_LLINFO_STALE;
		nd6_llinfo_settimer(ln, nd6_gctimer * hz);
	}

	/*
	 * The first time we send a packet to a neighbor whose entry is
	 * STALE, we have to change the state to DELAY and a sets a timer to
	 * expire in DELAY_FIRST_PROBE_TIME seconds to ensure do
	 * neighbor unreachability detection on expiration.
	 * (RFC 2461 7.3.3)
	 */
	if (ln->ln_state == ND6_LLINFO_STALE) {
		ln->ln_asked = 0;
		ln->ln_state = ND6_LLINFO_DELAY;
		nd6_llinfo_settimer(ln, nd6_delay * hz);
	}

	/*
	 * There is a neighbor cache entry, but no ethernet address
	 * response yet.  Append this latest packet to the end of the
	 * packet queue in the mbuf, unless the number of the packet
	 * does not exceed nd6_maxqueuelen.  When it exceeds nd6_maxqueuelen,
	 * the oldest packet in the queue will be removed.
	 */
	if (ln->ln_state == ND6_LLINFO_NOSTATE)
		ln->ln_state = ND6_LLINFO_INCOMPLETE;
	if (ln->ln_hold) {
		struct mbuf *m_hold;
		int i;

		i = 0;
		for (m_hold = ln->ln_hold; m_hold; m_hold = m_hold->m_nextpkt) {
			i++;
			if (m_hold->m_nextpkt == NULL) {
				m_hold->m_nextpkt = m;
				break;
			}
		}
		while (i >= nd6_maxqueuelen) {
			m_hold = ln->ln_hold;
			ln->ln_hold = ln->ln_hold->m_nextpkt;
			m_freem(m_hold);
			i--;
		}
	} else {
		ln->ln_hold = m;
	}

	/*
	 * If there has been no NS for the neighbor after entering the
	 * INCOMPLETE state, send the first solicitation.
	 */
	if (!ND6_LLINFO_PERMANENT(ln) && ln->ln_asked == 0) {
		struct in6_addr src, *psrc;

		ln->ln_asked++;
		nd6_llinfo_settimer(ln, ND_IFINFO(ifp)->retrans * hz / 1000);
		psrc = nd6_llinfo_get_holdsrc(ln, &src);
		LLE_WUNLOCK(ln);
		ln = NULL;
		nd6_ns_output(ifp, NULL, &dst->sin6_addr, psrc, 0);
	} else {
		/* We did the lookup so we need to do the unlock here. */
		LLE_WUNLOCK(ln);
	}

	if (created)
		nd6_gc_neighbors(LLTABLE6(ifp), &dst->sin6_addr);

	return EWOULDBLOCK;
}

int
nd6_need_cache(struct ifnet *ifp)
{
	/*
	 * XXX: we currently do not make neighbor cache on any interface
	 * other than ARCnet, Ethernet, FDDI and GIF.
	 *
	 * RFC2893 says:
	 * - unidirectional tunnels needs no ND
	 */
	switch (ifp->if_type) {
	case IFT_ARCNET:
	case IFT_ETHER:
	case IFT_FDDI:
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

static void 
clear_llinfo_pqueue(struct llentry *ln)
{
	struct mbuf *m_hold, *m_hold_next;

	for (m_hold = ln->ln_hold; m_hold; m_hold = m_hold_next) {
		m_hold_next = m_hold->m_nextpkt;
		m_hold->m_nextpkt = NULL;
		m_freem(m_hold);
	}

	ln->ln_hold = NULL;
	return;
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
	int (*fill_func)(void *, size_t *);

	if (newp)
		return EPERM;

	switch (name) {
	case ICMPV6CTL_ND6_DRLIST:
		fill_func = fill_drlist;
		break;

	case ICMPV6CTL_ND6_PRLIST:
		fill_func = fill_prlist;
		break;

	case ICMPV6CTL_ND6_MAXQLEN:
		return 0;

	default:
		return ENOPROTOOPT;
	}

	if (oldlenp == NULL)
		return EINVAL;

	size_t ol;
	int error = (*fill_func)(NULL, &ol);	/* calc len needed */
	if (error)
		return error;

	if (oldp == NULL) {
		*oldlenp = ol;
		return 0;
	}

	ol = *oldlenp = min(ol, *oldlenp);
	if (ol == 0)
		return 0;

	void *p = kmem_alloc(ol, KM_SLEEP);
	error = (*fill_func)(p, oldlenp);
	if (!error)
		error = copyout(p, oldp, *oldlenp);
	kmem_free(p, ol);

	return error;
}

static int
fill_drlist(void *oldp, size_t *oldlenp)
{
	int error = 0;
	struct in6_defrouter *d = NULL, *de = NULL;
	struct nd_defrouter *dr;
	size_t l;

	if (oldp) {
		d = (struct in6_defrouter *)oldp;
		de = (struct in6_defrouter *)((char *)oldp + *oldlenp);
	}
	l = 0;

	ND6_RLOCK();
	ND_DEFROUTER_LIST_FOREACH(dr) {

		if (oldp && d + 1 <= de) {
			memset(d, 0, sizeof(*d));
			sockaddr_in6_init(&d->rtaddr, &dr->rtaddr, 0, 0, 0);
			if (sa6_recoverscope(&d->rtaddr)) {
				char ip6buf[INET6_ADDRSTRLEN];
				log(LOG_ERR,
				    "scope error in router list (%s)\n",
				    IN6_PRINT(ip6buf, &d->rtaddr.sin6_addr));
				/* XXX: press on... */
			}
			d->flags = dr->flags;
			d->rtlifetime = dr->rtlifetime;
			d->expire = dr->expire ?
			    time_mono_to_wall(dr->expire) : 0;
			d->if_index = dr->ifp->if_index;
		}

		l += sizeof(*d);
		if (d)
			d++;
	}
	ND6_UNLOCK();

	*oldlenp = l;	/* (void *)d - (void *)oldp */

	return error;
}

static int
fill_prlist(void *oldp, size_t *oldlenp)
{
	int error = 0;
	struct nd_prefix *pr;
	uint8_t *p = NULL, *ps = NULL;
	uint8_t *pe = NULL;
	size_t l;
	char ip6buf[INET6_ADDRSTRLEN];

	if (oldp) {
		ps = p = (uint8_t*)oldp;
		pe = (uint8_t*)oldp + *oldlenp;
	}
	l = 0;

	ND6_RLOCK();
	ND_PREFIX_LIST_FOREACH(pr) {
		u_short advrtrs;
		struct sockaddr_in6 sin6;
		struct nd_pfxrouter *pfr;
		struct in6_prefix pfx;

		if (oldp && p + sizeof(struct in6_prefix) <= pe)
		{
			memset(&pfx, 0, sizeof(pfx));
			ps = p;
			pfx.prefix = pr->ndpr_prefix;

			if (sa6_recoverscope(&pfx.prefix)) {
				log(LOG_ERR,
				    "scope error in prefix list (%s)\n",
				    IN6_PRINT(ip6buf, &pfx.prefix.sin6_addr));
				/* XXX: press on... */
			}
			pfx.raflags = pr->ndpr_raf;
			pfx.prefixlen = pr->ndpr_plen;
			pfx.vltime = pr->ndpr_vltime;
			pfx.pltime = pr->ndpr_pltime;
			pfx.if_index = pr->ndpr_ifp->if_index;
			if (pr->ndpr_vltime == ND6_INFINITE_LIFETIME)
				pfx.expire = 0;
			else {
				time_t maxexpire;

				/* XXX: we assume time_t is signed. */
				maxexpire = (-1) &
				    ~((time_t)1 <<
				    ((sizeof(maxexpire) * 8) - 1));
				if (pr->ndpr_vltime <
				    maxexpire - pr->ndpr_lastupdate) {
					pfx.expire = pr->ndpr_lastupdate +
						pr->ndpr_vltime;
				} else
					pfx.expire = maxexpire;
			}
			pfx.refcnt = pr->ndpr_refcnt;
			pfx.flags = pr->ndpr_stateflags;
			pfx.origin = PR_ORIG_RA;

			p += sizeof(pfx); l += sizeof(pfx);

			advrtrs = 0;
			LIST_FOREACH(pfr, &pr->ndpr_advrtrs, pfr_entry) {
				if (p + sizeof(sin6) > pe) {
					advrtrs++;
					continue;
				}

				sockaddr_in6_init(&sin6, &pfr->router->rtaddr,
					    0, 0, 0);
				if (sa6_recoverscope(&sin6)) {
					log(LOG_ERR,
					    "scope error in "
					    "prefix list (%s)\n",
					    IN6_PRINT(ip6buf,
					    &pfr->router->rtaddr));
				}
				advrtrs++;
				memcpy(p, &sin6, sizeof(sin6));
				p += sizeof(sin6);
				l += sizeof(sin6);
			}
			pfx.advrtrs = advrtrs;
			memcpy(ps, &pfx, sizeof(pfx));
		}
		else {
			l += sizeof(pfx);
			advrtrs = 0;
			LIST_FOREACH(pfr, &pr->ndpr_advrtrs, pfr_entry) {
				advrtrs++;
				l += sizeof(sin6);
			}
		}
	}
	ND6_UNLOCK();

	*oldlenp = l;

	return error;
}

static int
nd6_setdefaultiface(int ifindex)
{
	ifnet_t *ifp;
	int error = 0;
	int s;

	s = pserialize_read_enter();
	ifp = if_byindex(ifindex);
	if (ifp == NULL) {
		pserialize_read_exit(s);
		return EINVAL;
	}
	if (nd6_defifindex != ifindex) {
		nd6_defifindex = ifindex;
		nd6_defifp = nd6_defifindex > 0 ? ifp : NULL;

		/*
		 * Our current implementation assumes one-to-one maping between
		 * interfaces and links, so it would be natural to use the
		 * default interface as the default link.
		 */
		scope6_setdefault(nd6_defifp);
	}
	pserialize_read_exit(s);

	return (error);
}
