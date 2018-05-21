/*	$NetBSD: nd6_rtr.c,v 1.138.2.2 2018/05/21 04:36:16 pgoyette Exp $	*/
/*	$KAME: nd6_rtr.c,v 1.95 2001/02/07 08:09:47 itojun Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: nd6_rtr.c,v 1.138.2.2 2018/05/21 04:36:16 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/cprng.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>

#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <netinet6/in6_ifattach.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/nd6.h>
#include <netinet/icmp6.h>
#include <netinet6/icmp6_private.h>
#include <netinet6/scope6_var.h>

static int rtpref(struct nd_defrouter *);
static struct nd_defrouter *defrtrlist_update(struct nd_defrouter *);
static int prelist_update(struct nd_prefixctl *, struct nd_defrouter *,
    struct mbuf *, int);
static struct in6_ifaddr *in6_ifadd(struct nd_prefixctl *, int, struct psref *);
static struct nd_pfxrouter *pfxrtr_lookup(struct nd_prefix *,
	struct nd_defrouter *);
static void pfxrtr_add(struct nd_prefix *, struct nd_defrouter *);
static void pfxrtr_del(struct nd_pfxrouter *);
static struct nd_pfxrouter *find_pfxlist_reachable_router
	(struct nd_prefix *);

static void defrouter_addreq(struct nd_defrouter *);
static void defrouter_delreq(struct nd_defrouter *);

static int in6_init_prefix_ltimes(struct nd_prefix *);
static void in6_init_address_ltimes(struct nd_prefix *,
	struct in6_addrlifetime *);
static void purge_detached(struct ifnet *);

static int rt6_deleteroute_matcher(struct rtentry *, void *);

static int nd6_prelist_add(struct nd_prefixctl *, struct nd_defrouter *,
	struct nd_prefix **);
static int nd6_prefix_onlink(struct nd_prefix *);
static int nd6_prefix_offlink(struct nd_prefix *);
static struct nd_prefix *nd6_prefix_lookup(struct nd_prefixctl *);

extern int nd6_recalc_reachtm_interval;

int ip6_use_tempaddr = 0;

int ip6_desync_factor;
u_int32_t ip6_temp_preferred_lifetime = DEF_TEMP_PREFERRED_LIFETIME;
u_int32_t ip6_temp_valid_lifetime = DEF_TEMP_VALID_LIFETIME;
int ip6_temp_regen_advance = TEMPADDR_REGEN_ADVANCE;

int nd6_numroutes = 0;

/* RTPREF_MEDIUM has to be 0! */
#define RTPREF_HIGH	1
#define RTPREF_MEDIUM	0
#define RTPREF_LOW	(-1)
#define RTPREF_RESERVED	(-2)
#define RTPREF_INVALID	(-3)	/* internal */

static inline bool
nd6_is_llinfo_probreach(struct nd_defrouter *dr)
{
	struct llentry *ln = NULL;

	ln = nd6_lookup(&dr->rtaddr, dr->ifp, false);
	if (ln == NULL)
		return false;

	if (!ND6_IS_LLINFO_PROBREACH(ln)) {
		LLE_RUNLOCK(ln);
		return false;
	}

	LLE_RUNLOCK(ln);
	return true;
}

/*
 * Receive Router Solicitation Message - just for routers.
 * Router solicitation/advertisement is mostly managed by a userland program
 * (rtadvd) so here we have no function like nd6_ra_output().
 *
 * Based on RFC 2461
 */
void
nd6_rs_input(struct mbuf *m, int off, int icmp6len)
{
	struct ifnet *ifp;
	struct nd_ifinfo *ndi;
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
	struct nd_router_solicit *nd_rs;
	struct in6_addr saddr6 = ip6->ip6_src;
	char *lladdr = NULL;
	int lladdrlen = 0;
	union nd_opts ndopts;
	struct psref psref;
	char ip6bufs[INET6_ADDRSTRLEN], ip6bufd[INET6_ADDRSTRLEN];

	ifp = m_get_rcvif_psref(m, &psref);
	if (ifp == NULL)
		goto freeit;

	ndi = ND_IFINFO(ifp);

	/* If I'm not a router, ignore it. */
	if (nd6_accepts_rtadv(ndi) || !ip6_forwarding)
		goto freeit;

	/* Sanity checks */
	if (ip6->ip6_hlim != 255) {
		nd6log(LOG_ERR, "invalid hlim (%d) from %s to %s on %s\n",
		    ip6->ip6_hlim, IN6_PRINT(ip6bufs, &ip6->ip6_src),
		    IN6_PRINT(ip6bufd, &ip6->ip6_dst), if_name(ifp));
		goto bad;
	}

	/*
	 * Don't update the neighbor cache, if src = ::.
	 * This indicates that the src has no IP address assigned yet.
	 */
	if (IN6_IS_ADDR_UNSPECIFIED(&saddr6))
		goto freeit;

	IP6_EXTHDR_GET(nd_rs, struct nd_router_solicit *, m, off, icmp6len);
	if (nd_rs == NULL) {
		ICMP6_STATINC(ICMP6_STAT_TOOSHORT);
		m_put_rcvif_psref(ifp, &psref);
		return;
	}

	icmp6len -= sizeof(*nd_rs);
	nd6_option_init(nd_rs + 1, icmp6len, &ndopts);
	if (nd6_options(&ndopts) < 0) {
		nd6log(LOG_INFO, "invalid ND option, ignored\n");
		/* nd6_options have incremented stats */
		goto freeit;
	}

	if (ndopts.nd_opts_src_lladdr) {
		lladdr = (char *)(ndopts.nd_opts_src_lladdr + 1);
		lladdrlen = ndopts.nd_opts_src_lladdr->nd_opt_len << 3;
	}

	if (lladdr && ((ifp->if_addrlen + 2 + 7) & ~7) != lladdrlen) {
		nd6log(LOG_INFO, "lladdrlen mismatch for %s "
		    "(if %d, RS packet %d)\n",
		    IN6_PRINT(ip6bufs, &saddr6),
		    ifp->if_addrlen, lladdrlen - 2);
		goto bad;
	}

	nd6_cache_lladdr(ifp, &saddr6, lladdr, lladdrlen, ND_ROUTER_SOLICIT, 0);

 freeit:
	m_put_rcvif_psref(ifp, &psref);
	m_freem(m);
	return;

 bad:
	ICMP6_STATINC(ICMP6_STAT_BADRS);
	m_put_rcvif_psref(ifp, &psref);
	m_freem(m);
}

/*
 * Receive Router Advertisement Message.
 *
 * Based on RFC 2461
 * TODO: on-link bit on prefix information
 * TODO: ND_RA_FLAG_{OTHER,MANAGED} processing
 */
void
nd6_ra_input(struct mbuf *m, int off, int icmp6len)
{
	struct ifnet *ifp;
	struct nd_ifinfo *ndi;
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
	struct nd_router_advert *nd_ra;
	struct in6_addr saddr6 = ip6->ip6_src;
	int mcast = 0;
	union nd_opts ndopts;
	struct nd_defrouter *dr;
	struct psref psref;
	char ip6buf[INET6_ADDRSTRLEN], ip6buf2[INET6_ADDRSTRLEN];

	ifp = m_get_rcvif_psref(m, &psref);
	if (ifp == NULL)
		goto freeit;

	ndi = ND_IFINFO(ifp);
	/*
	 * We only accept RAs when the system-wide variable allows the
	 * acceptance, and the per-interface variable allows RAs on the
	 * receiving interface.
	 */
	if (!nd6_accepts_rtadv(ndi))
		goto freeit;

	if (ip6->ip6_hlim != 255) {
		nd6log(LOG_ERR, "invalid hlim (%d) from %s to %s on %s\n",
		    ip6->ip6_hlim, IN6_PRINT(ip6buf, &ip6->ip6_src),
		    IN6_PRINT(ip6buf2, &ip6->ip6_dst), if_name(ifp));
		goto bad;
	}

	if (!IN6_IS_ADDR_LINKLOCAL(&saddr6)) {
		nd6log(LOG_ERR, "src %s is not link-local\n",
		    IN6_PRINT(ip6buf, &saddr6));
		goto bad;
	}

	IP6_EXTHDR_GET(nd_ra, struct nd_router_advert *, m, off, icmp6len);
	if (nd_ra == NULL) {
		ICMP6_STATINC(ICMP6_STAT_TOOSHORT);
		m_put_rcvif_psref(ifp, &psref);
		return;
	}

	icmp6len -= sizeof(*nd_ra);
	nd6_option_init(nd_ra + 1, icmp6len, &ndopts);
	if (nd6_options(&ndopts) < 0) {
		nd6log(LOG_INFO, "invalid ND option, ignored\n");
		/* nd6_options have incremented stats */
		goto freeit;
	}

    {
	struct nd_defrouter drtr;
	u_int32_t advreachable = nd_ra->nd_ra_reachable;

	/* remember if this is a multicasted advertisement */
	if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst))
		mcast = 1;

	memset(&drtr, 0, sizeof(drtr));
	drtr.rtaddr = saddr6;
	drtr.flags  = nd_ra->nd_ra_flags_reserved;
	drtr.rtlifetime = ntohs(nd_ra->nd_ra_router_lifetime);
	drtr.expire = time_uptime + drtr.rtlifetime;
	drtr.ifp = ifp;
	/* unspecified or not? (RFC 2461 6.3.4) */
	if (advreachable) {
		NTOHL(advreachable);
		if (advreachable <= MAX_REACHABLE_TIME &&
		    ndi->basereachable != advreachable) {
			ndi->basereachable = advreachable;
			ndi->reachable = ND_COMPUTE_RTIME(ndi->basereachable);
			ndi->recalctm = nd6_recalc_reachtm_interval; /* reset */
		}
	}
	if (nd_ra->nd_ra_retransmit)
		ndi->retrans = ntohl(nd_ra->nd_ra_retransmit);
	if (nd_ra->nd_ra_curhoplimit) {
		if (ndi->chlim < nd_ra->nd_ra_curhoplimit)
			ndi->chlim = nd_ra->nd_ra_curhoplimit;
		else if (ndi->chlim != nd_ra->nd_ra_curhoplimit)
			log(LOG_ERR, "nd_ra_input: lower CurHopLimit sent from "
			   "%s on %s (current=%d, received=%d), ignored\n",
			   IN6_PRINT(ip6buf, &ip6->ip6_src),
			   if_name(ifp), ndi->chlim, nd_ra->nd_ra_curhoplimit);
	}
	IFNET_LOCK(ifp);
	ND6_WLOCK();
	dr = defrtrlist_update(&drtr);
    }

	/*
	 * prefix
	 */
	if (ndopts.nd_opts_pi) {
		struct nd_opt_hdr *pt;
		struct nd_opt_prefix_info *pi = NULL;
		struct nd_prefixctl prc;

		for (pt = (struct nd_opt_hdr *)ndopts.nd_opts_pi;
		     pt <= (struct nd_opt_hdr *)ndopts.nd_opts_pi_end;
		     pt = (struct nd_opt_hdr *)((char *)pt +
						(pt->nd_opt_len << 3))) {
			if (pt->nd_opt_type != ND_OPT_PREFIX_INFORMATION)
				continue;
			pi = (struct nd_opt_prefix_info *)pt;

			if (pi->nd_opt_pi_len != 4) {
				nd6log(LOG_INFO, "invalid option "
				    "len %d for prefix information option, "
				    "ignored\n", pi->nd_opt_pi_len);
				continue;
			}

			if (128 < pi->nd_opt_pi_prefix_len) {
				nd6log(LOG_INFO, "invalid prefix "
				    "len %d for prefix information option, "
				    "ignored\n", pi->nd_opt_pi_prefix_len);
				continue;
			}

			if (IN6_IS_ADDR_MULTICAST(&pi->nd_opt_pi_prefix)
			 || IN6_IS_ADDR_LINKLOCAL(&pi->nd_opt_pi_prefix)) {
				nd6log(LOG_INFO,
				    "invalid prefix %s, ignored\n",
				    IN6_PRINT(ip6buf, &pi->nd_opt_pi_prefix));
				continue;
			}

			memset(&prc, 0, sizeof(prc));
			sockaddr_in6_init(&prc.ndprc_prefix,
			    &pi->nd_opt_pi_prefix, 0, 0, 0);
			prc.ndprc_ifp = ifp;

			prc.ndprc_raf_onlink = (pi->nd_opt_pi_flags_reserved &
			    ND_OPT_PI_FLAG_ONLINK) ? 1 : 0;
			prc.ndprc_raf_auto = (pi->nd_opt_pi_flags_reserved &
			    ND_OPT_PI_FLAG_AUTO) ? 1 : 0;
			prc.ndprc_plen = pi->nd_opt_pi_prefix_len;
			prc.ndprc_vltime = ntohl(pi->nd_opt_pi_valid_time);
			prc.ndprc_pltime = ntohl(pi->nd_opt_pi_preferred_time);

			(void)prelist_update(&prc, dr, m, mcast);
		}
	}
	ND6_UNLOCK();
	IFNET_UNLOCK(ifp);

	/*
	 * MTU
	 */
	if (ndopts.nd_opts_mtu && ndopts.nd_opts_mtu->nd_opt_mtu_len == 1) {
		u_long mtu;
		u_long maxmtu;

		mtu = ntohl(ndopts.nd_opts_mtu->nd_opt_mtu_mtu);

		/* lower bound */
		if (mtu < IPV6_MMTU) {
			nd6log(LOG_INFO, "bogus mtu option "
			    "mtu=%lu sent from %s, ignoring\n",
			    mtu, IN6_PRINT(ip6buf, &ip6->ip6_src));
			goto skip;
		}

		/* upper bound */
		maxmtu = (ndi->maxmtu && ndi->maxmtu < ifp->if_mtu)
		    ? ndi->maxmtu : ifp->if_mtu;
		if (mtu <= maxmtu) {
			int change = (ndi->linkmtu != mtu);

			ndi->linkmtu = mtu;
			if (change) /* in6_maxmtu may change */
				in6_setmaxmtu();
		} else {
			nd6log(LOG_INFO,
			    "bogus mtu mtu=%lu sent from %s; "
			    "exceeds maxmtu %lu, ignoring\n",
			    mtu, IN6_PRINT(ip6buf, &ip6->ip6_src), maxmtu);
		}
	}

 skip:

	/*
	 * Source link layer address
	 */
    {
	char *lladdr = NULL;
	int lladdrlen = 0;

	if (ndopts.nd_opts_src_lladdr) {
		lladdr = (char *)(ndopts.nd_opts_src_lladdr + 1);
		lladdrlen = ndopts.nd_opts_src_lladdr->nd_opt_len << 3;
	}

	if (lladdr && ((ifp->if_addrlen + 2 + 7) & ~7) != lladdrlen) {
		nd6log(LOG_INFO, "lladdrlen mismatch for %s "
		    "(if %d, RA packet %d)\n", IN6_PRINT(ip6buf, &saddr6),
		    ifp->if_addrlen, lladdrlen - 2);
		goto bad;
	}

	nd6_cache_lladdr(ifp, &saddr6, lladdr, lladdrlen, ND_ROUTER_ADVERT, 0);

	/*
	 * Installing a link-layer address might change the state of the
	 * router's neighbor cache, which might also affect our on-link
	 * detection of adveritsed prefixes.
	 */
	ND6_WLOCK();
	nd6_pfxlist_onlink_check();
	ND6_UNLOCK();
    }

 freeit:
	m_put_rcvif_psref(ifp, &psref);
	m_freem(m);
	return;

 bad:
	ICMP6_STATINC(ICMP6_STAT_BADRA);
	m_put_rcvif_psref(ifp, &psref);
	m_freem(m);
}

/*
 * default router list processing sub routines
 */
static void
defrouter_addreq(struct nd_defrouter *newdr)
{
	union {
		struct sockaddr_in6 sin6;
		struct sockaddr sa;
	} def, mask, gate;
#ifndef NET_MPSAFE
	int s;
#endif
	int error;

	memset(&def, 0, sizeof(def));
	memset(&mask, 0, sizeof(mask));
	memset(&gate, 0,sizeof(gate)); /* for safety */

	def.sin6.sin6_len = mask.sin6.sin6_len = gate.sin6.sin6_len =
	    sizeof(struct sockaddr_in6);
	def.sin6.sin6_family = mask.sin6.sin6_family = gate.sin6.sin6_family = AF_INET6;
	gate.sin6.sin6_addr = newdr->rtaddr;
#ifndef SCOPEDROUTING
	gate.sin6.sin6_scope_id = 0;	/* XXX */
#endif

#ifndef NET_MPSAFE
	s = splsoftnet();
#endif
	error = rtrequest_newmsg(RTM_ADD, &def.sa, &gate.sa, &mask.sa,
	    RTF_GATEWAY);
	if (error == 0) {
		nd6_numroutes++;
		newdr->installed = 1;
	}
#ifndef NET_MPSAFE
	splx(s);
#endif
	return;
}

struct nd_defrouter *
nd6_defrouter_lookup(const struct in6_addr *addr, struct ifnet *ifp)
{
	struct nd_defrouter *dr;

	ND6_ASSERT_LOCK();

	ND_DEFROUTER_LIST_FOREACH(dr) {
		if (dr->ifp == ifp && IN6_ARE_ADDR_EQUAL(addr, &dr->rtaddr))
			break;
	}

	return dr;		/* search failed */
}

void
nd6_defrtrlist_del(struct nd_defrouter *dr, struct in6_ifextra *ext)
{
	struct nd_defrouter *deldr = NULL;
	struct nd_prefix *pr;
	struct nd_ifinfo *ndi;

	ND6_ASSERT_WLOCK();

	if (ext == NULL)
		ext = dr->ifp->if_afdata[AF_INET6];

	/* detach already in progress, can not do anything */
	if (ext == NULL)
		return;

	ndi = ext->nd_ifinfo;

	/*
	 * Flush all the routing table entries that use the router
	 * as a next hop.
	 */
	/* XXX: better condition? */
	if (!ip6_forwarding && nd6_accepts_rtadv(ndi))
		nd6_rt_flush(&dr->rtaddr, dr->ifp);

	if (dr->installed) {
		deldr = dr;
		defrouter_delreq(dr);
	}
	ND_DEFROUTER_LIST_REMOVE(dr);

	/*
	 * Also delete all the pointers to the router in each prefix lists.
	 */
	ND_PREFIX_LIST_FOREACH(pr) {
		struct nd_pfxrouter *pfxrtr;
		if ((pfxrtr = pfxrtr_lookup(pr, dr)) != NULL)
			pfxrtr_del(pfxrtr);
	}
	nd6_pfxlist_onlink_check();

	/*
	 * If the router is the primary one, choose a new one.
	 * Note that nd6_defrouter_select() will remove the current gateway
	 * from the routing table.
	 */
	if (deldr)
		nd6_defrouter_select();

	ext->ndefrouters--;
	if (ext->ndefrouters < 0) {
		log(LOG_WARNING, "nd6_defrtrlist_del: negative count on %s\n",
		    dr->ifp->if_xname);
	}

	free(dr, M_IP6NDP);
}

/*
 * Remove the default route for a given router.
 * This is just a subroutine function for nd6_defrouter_select(), and should
 * not be called from anywhere else.
 */
static void
defrouter_delreq(struct nd_defrouter *dr)
{
	union {
		struct sockaddr_in6 sin6;
		struct sockaddr sa;
	} def, mask, gw;
	int error;

	memset(&def, 0, sizeof(def));
	memset(&mask, 0, sizeof(mask));
	memset(&gw, 0, sizeof(gw));	/* for safety */

	def.sin6.sin6_len = mask.sin6.sin6_len = gw.sin6.sin6_len =
	    sizeof(struct sockaddr_in6);
	def.sin6.sin6_family = mask.sin6.sin6_family = gw.sin6.sin6_family = AF_INET6;
	gw.sin6.sin6_addr = dr->rtaddr;
#ifndef SCOPEDROUTING
	gw.sin6.sin6_scope_id = 0;	/* XXX */
#endif

	error = rtrequest_newmsg(RTM_DELETE, &def.sa, &gw.sa, &mask.sa,
	    RTF_GATEWAY);
	if (error == 0)
		nd6_numroutes--;

	dr->installed = 0;
}

/*
 * remove all default routes from default router list
 */
void
nd6_defrouter_reset(void)
{
	struct nd_defrouter *dr;

	ND6_ASSERT_WLOCK();

	ND_DEFROUTER_LIST_FOREACH(dr)
		defrouter_delreq(dr);

	/*
	 * XXX should we also nuke any default routers in the kernel, by
	 * going through them by rtalloc1()?
	 */
}

/*
 * Default Router Selection according to Section 6.3.6 of RFC 2461 and
 * draft-ietf-ipngwg-router-selection:
 * 1) Routers that are reachable or probably reachable should be preferred.
 *    If we have more than one (probably) reachable router, prefer ones
 *    with the highest router preference.
 * 2) When no routers on the list are known to be reachable or
 *    probably reachable, routers SHOULD be selected in a round-robin
 *    fashion, regardless of router preference values.
 * 3) If the Default Router List is empty, assume that all
 *    destinations are on-link.
 *
 * We assume nd_defrouter is sorted by router preference value.
 * Since the code below covers both with and without router preference cases,
 * we do not need to classify the cases by ifdef.
 *
 * At this moment, we do not try to install more than one default router,
 * even when the multipath routing is available, because we're not sure about
 * the benefits for stub hosts comparing to the risk of making the code
 * complicated and the possibility of introducing bugs.
 */
void
nd6_defrouter_select(void)
{
	struct nd_ifinfo *ndi;
	struct nd_defrouter *dr, *selected_dr = NULL, *installed_dr = NULL;

	ND6_ASSERT_WLOCK();

	/*
	 * This function should be called only when acting as an autoconfigured
	 * host.  Although the remaining part of this function is not effective
	 * if the node is not an autoconfigured host, we explicitly exclude
	 * such cases here for safety.
	 */
	if (ip6_forwarding) {
		nd6log(LOG_WARNING, "called unexpectedly (forwarding=%d, "
		    "accept_rtadv=%d)\n", ip6_forwarding, ip6_accept_rtadv);
		return;
	}

	/*
	 * Let's handle easy case (3) first:
	 * If default router list is empty, there's nothing to be done.
	 */
	if (ND_DEFROUTER_LIST_EMPTY())
		return;

	/*
	 * Search for a (probably) reachable router from the list.
	 * We just pick up the first reachable one (if any), assuming that
	 * the ordering rule of the list described in defrtrlist_update().
	 */
	ND_DEFROUTER_LIST_FOREACH(dr) {
		ndi = ND_IFINFO(dr->ifp);
		if (nd6_accepts_rtadv(ndi))
			continue;

		if (selected_dr == NULL &&
		    nd6_is_llinfo_probreach(dr))
			selected_dr = dr;

		if (dr->installed && !installed_dr)
			installed_dr = dr;
		else if (dr->installed && installed_dr) {
			/* this should not happen.  warn for diagnosis. */
			log(LOG_ERR, "nd6_defrouter_select: more than one router"
			    " is installed\n");
		}
	}
	/*
	 * If none of the default routers was found to be reachable,
	 * round-robin the list regardless of preference.
	 * Otherwise, if we have an installed router, check if the selected
	 * (reachable) router should really be preferred to the installed one.
	 * We only prefer the new router when the old one is not reachable
	 * or when the new one has a really higher preference value.
	 */
	if (selected_dr == NULL) {
		if (installed_dr == NULL ||
		    ND_DEFROUTER_LIST_NEXT(installed_dr) == NULL)
			selected_dr = ND_DEFROUTER_LIST_FIRST();
		else
			selected_dr = ND_DEFROUTER_LIST_NEXT(installed_dr);
	} else if (installed_dr &&
	    nd6_is_llinfo_probreach(installed_dr) &&
	    rtpref(selected_dr) <= rtpref(installed_dr)) {
		selected_dr = installed_dr;
	}

	/*
	 * If the selected router is different than the installed one,
	 * remove the installed router and install the selected one.
	 * Note that the selected router is never NULL here.
	 */
	if (installed_dr != selected_dr) {
		if (installed_dr)
			defrouter_delreq(installed_dr);
		defrouter_addreq(selected_dr);
	}

	return;
}

/*
 * for default router selection
 * regards router-preference field as a 2-bit signed integer
 */
static int
rtpref(struct nd_defrouter *dr)
{
	switch (dr->flags & ND_RA_FLAG_RTPREF_MASK) {
	case ND_RA_FLAG_RTPREF_HIGH:
		return (RTPREF_HIGH);
	case ND_RA_FLAG_RTPREF_MEDIUM:
	case ND_RA_FLAG_RTPREF_RSV:
		return (RTPREF_MEDIUM);
	case ND_RA_FLAG_RTPREF_LOW:
		return (RTPREF_LOW);
	default:
		/*
		 * This case should never happen.  If it did, it would mean a
		 * serious bug of kernel internal.  We thus always bark here.
		 * Or, can we even panic?
		 */
		log(LOG_ERR, "rtpref: impossible RA flag %x\n", dr->flags);
		return (RTPREF_INVALID);
	}
	/* NOTREACHED */
}

static struct nd_defrouter *
defrtrlist_update(struct nd_defrouter *newdr)
{
	struct nd_defrouter *dr, *n, *ret = NULL;
	struct in6_ifextra *ext = newdr->ifp->if_afdata[AF_INET6];

	ND6_ASSERT_WLOCK();

	if ((dr = nd6_defrouter_lookup(&newdr->rtaddr, newdr->ifp)) != NULL) {
		/* entry exists */
		if (newdr->rtlifetime == 0) {
			nd6_defrtrlist_del(dr, ext);
			dr = NULL;
		} else {
			int oldpref = rtpref(dr);

			/* override */
			dr->flags = newdr->flags; /* xxx flag check */
			dr->rtlifetime = newdr->rtlifetime;
			dr->expire = newdr->expire;

			/*
			 * If the preference does not change, there's no need
			 * to sort the entries.
			 */
			if (rtpref(newdr) == oldpref) {
				ret = dr;
				goto out;
			}

			/*
			 * preferred router may be changed, so relocate
			 * this router.
			 * XXX: calling TAILQ_REMOVE directly is a bad manner.
			 * However, since nd6_defrtrlist_del() has many side
			 * effects, we intentionally do so here.
			 * nd6_defrouter_select() below will handle routing
			 * changes later.
			 */
			ND_DEFROUTER_LIST_REMOVE(dr);
			n = dr;
			goto insert;
		}
		ret = dr;
		goto out;
	}

	if (ip6_maxifdefrouters >= 0 && ext->ndefrouters >= ip6_maxifdefrouters)
		goto out;

	/* entry does not exist */
	if (newdr->rtlifetime == 0)
		goto out;

	if (ip6_rtadv_maxroutes <= nd6_numroutes) {
		ICMP6_STATINC(ICMP6_STAT_DROPPED_RAROUTE);
		goto out;
	}

	n = (struct nd_defrouter *)malloc(sizeof(*n), M_IP6NDP, M_NOWAIT);
	if (n == NULL)
		goto out;
	memset(n, 0, sizeof(*n));
	*n = *newdr;

insert:
	/*
	 * Insert the new router in the Default Router List;
	 * The Default Router List should be in the descending order
	 * of router-preferece.  Routers with the same preference are
	 * sorted in the arriving time order.
	 */

	/* insert at the end of the group */
	ND_DEFROUTER_LIST_FOREACH(dr) {
		if (rtpref(n) > rtpref(dr))
			break;
	}
	if (dr)
		ND_DEFROUTER_LIST_INSERT_BEFORE(dr, n);
	else
		ND_DEFROUTER_LIST_INSERT_TAIL(n);

	nd6_defrouter_select();

	ext->ndefrouters++;

	ret = n;
out:
	return ret;
}

static struct nd_pfxrouter *
pfxrtr_lookup(struct nd_prefix *pr, struct nd_defrouter *dr)
{
	struct nd_pfxrouter *search;

	ND6_ASSERT_LOCK();

	LIST_FOREACH(search, &pr->ndpr_advrtrs, pfr_entry) {
		if (search->router == dr)
			break;
	}

	return (search);
}

static void
pfxrtr_add(struct nd_prefix *pr, struct nd_defrouter *dr)
{
	struct nd_pfxrouter *newpfr;

	ND6_ASSERT_WLOCK();

	newpfr = malloc(sizeof(*newpfr), M_IP6NDP, M_NOWAIT|M_ZERO);
	if (newpfr == NULL)
		return;
	newpfr->router = dr;

	LIST_INSERT_HEAD(&pr->ndpr_advrtrs, newpfr, pfr_entry);

	nd6_pfxlist_onlink_check();
}

static void
pfxrtr_del(struct nd_pfxrouter *pfr)
{
	LIST_REMOVE(pfr, pfr_entry);
	free(pfr, M_IP6NDP);
}

static struct nd_prefix *
nd6_prefix_lookup(struct nd_prefixctl *key)
{
	struct nd_prefix *search;

	ND_PREFIX_LIST_FOREACH(search) {
		if (key->ndprc_ifp == search->ndpr_ifp &&
		    key->ndprc_plen == search->ndpr_plen &&
		    in6_are_prefix_equal(&key->ndprc_prefix.sin6_addr,
		    &search->ndpr_prefix.sin6_addr, key->ndprc_plen)) {
			break;
		}
	}

	return (search);
}

static void
purge_detached(struct ifnet *ifp)
{
	struct nd_prefix *pr, *pr_next;
	struct in6_ifaddr *ia;
	struct ifaddr *ifa, *ifa_next;

restart:
	ND6_ASSERT_WLOCK();

	ND_PREFIX_LIST_FOREACH_SAFE(pr, pr_next) {
		int s;

		/*
		 * This function is called when we need to make more room for
		 * new prefixes rather than keeping old, possibly stale ones.
		 * Detached prefixes would be a good candidate; if all routers
		 * that advertised the prefix expired, the prefix is also
		 * probably stale.
		 */
		if (pr->ndpr_ifp != ifp ||
		    IN6_IS_ADDR_LINKLOCAL(&pr->ndpr_prefix.sin6_addr) ||
		    ((pr->ndpr_stateflags & NDPRF_DETACHED) == 0 &&
		    !LIST_EMPTY(&pr->ndpr_advrtrs)))
			continue;

		s = pserialize_read_enter();
		for (ifa = IFADDR_READER_FIRST(ifp); ifa; ifa = ifa_next) {
			ifa_next = IFADDR_READER_NEXT(ifa);
			if (ifa->ifa_addr->sa_family != AF_INET6)
				continue;
			ia = (struct in6_ifaddr *)ifa;
			if ((ia->ia6_flags & IN6_IFF_AUTOCONF) ==
			    IN6_IFF_AUTOCONF && ia->ia6_ndpr == pr) {
				pserialize_read_exit(s);
				ND6_UNLOCK();

				/* in6_purgeaddr may destroy pr. */
				in6_purgeaddr(ifa);

				ND6_WLOCK();
				goto restart;
			}
		}
		pserialize_read_exit(s);

		KASSERT(pr->ndpr_refcnt == 0);
		nd6_prelist_remove(pr);
	}
}

static int
nd6_prelist_add(struct nd_prefixctl *prc, struct nd_defrouter *dr,
	struct nd_prefix **newp)
{
	struct nd_prefix *newpr = NULL;
	int i;
	int error;
	struct in6_ifextra *ext = prc->ndprc_ifp->if_afdata[AF_INET6];

	ND6_ASSERT_WLOCK();

	if (ip6_maxifprefixes >= 0) {
		if (ext->nprefixes >= ip6_maxifprefixes / 2)
			purge_detached(prc->ndprc_ifp);
		if (ext->nprefixes >= ip6_maxifprefixes)
			return ENOMEM;
	}

	error = 0;
	newpr = malloc(sizeof(*newpr), M_IP6NDP, M_NOWAIT|M_ZERO);
	if (newpr == NULL)
		return ENOMEM;
	newpr->ndpr_ifp = prc->ndprc_ifp;
	newpr->ndpr_prefix = prc->ndprc_prefix;
	newpr->ndpr_plen = prc->ndprc_plen;
	newpr->ndpr_vltime = prc->ndprc_vltime;
	newpr->ndpr_pltime = prc->ndprc_pltime;
	newpr->ndpr_flags = prc->ndprc_flags;
	if ((error = in6_init_prefix_ltimes(newpr)) != 0) {
		free(newpr, M_IP6NDP);
		return(error);
	}
	newpr->ndpr_lastupdate = time_uptime;
	if (newp != NULL)
		*newp = newpr;

	/* initialization */
	LIST_INIT(&newpr->ndpr_advrtrs);
	in6_prefixlen2mask(&newpr->ndpr_mask, newpr->ndpr_plen);
	/* make prefix in the canonical form */
	for (i = 0; i < 4; i++) {
		newpr->ndpr_prefix.sin6_addr.s6_addr32[i] &=
		    newpr->ndpr_mask.s6_addr32[i];
	}

	/* link ndpr_entry to nd_prefix list */
	ND_PREFIX_LIST_INSERT_HEAD(newpr);

	/* ND_OPT_PI_FLAG_ONLINK processing */
	if (newpr->ndpr_raf_onlink) {
		int e;

		if ((e = nd6_prefix_onlink(newpr)) != 0) {
			char ip6buf[INET6_ADDRSTRLEN];
			nd6log(LOG_ERR, "failed to make "
			    "the prefix %s/%d on-link on %s (errno=%d)\n",
			    IN6_PRINT(ip6buf, &prc->ndprc_prefix.sin6_addr),
			    prc->ndprc_plen, if_name(prc->ndprc_ifp), e);
			/* proceed anyway. XXX: is it correct? */
		}
	}

	if (dr)
		pfxrtr_add(newpr, dr);

	ext->nprefixes++;

	return 0;
}

void
nd6_prefix_unref(struct nd_prefix *pr)
{

	ND6_WLOCK();
	pr->ndpr_refcnt--;
	if (pr->ndpr_refcnt == 0)
		nd6_prelist_remove(pr);
	ND6_UNLOCK();
}

void
nd6_invalidate_prefix(struct nd_prefix *pr)
{
	int e;

	ND6_ASSERT_WLOCK();

	/* make sure to invalidate the prefix until it is really freed. */
	pr->ndpr_vltime = 0;
	pr->ndpr_pltime = 0;
#if 0
	/*
	 * Though these flags are now meaningless, we'd rather keep the value
	 * not to confuse users when executing "ndp -p".
	 */
	pr->ndpr_raf_onlink = 0;
	pr->ndpr_raf_auto = 0;
#endif
	if ((pr->ndpr_stateflags & NDPRF_ONLINK) != 0 &&
	    (e = nd6_prefix_offlink(pr)) != 0) {
		char ip6buf[INET6_ADDRSTRLEN];
		nd6log(LOG_ERR,
		    "failed to make %s/%d offlink on %s, errno=%d\n",
		    IN6_PRINT(ip6buf, &pr->ndpr_prefix.sin6_addr),
		    pr->ndpr_plen, if_name(pr->ndpr_ifp), e);
		/* what should we do? */
	}
}

void
nd6_prelist_remove(struct nd_prefix *pr)
{
	struct nd_pfxrouter *pfr, *next;
	struct in6_ifextra *ext = pr->ndpr_ifp->if_afdata[AF_INET6];

	ND6_ASSERT_WLOCK();
	KASSERT(pr->ndpr_refcnt == 0);

	nd6_invalidate_prefix(pr);

	/* unlink ndpr_entry from nd_prefix list */
	ND_PREFIX_LIST_REMOVE(pr);

	/* free list of routers that adversed the prefix */
	for (pfr = LIST_FIRST(&pr->ndpr_advrtrs); pfr != NULL; pfr = next) {
		next = LIST_NEXT(pfr, pfr_entry);

		free(pfr, M_IP6NDP);
	}

	if (ext) {
		ext->nprefixes--;
		if (ext->nprefixes < 0) {
			log(LOG_WARNING, "nd6_prelist_remove: negative count on "
			    "%s\n", pr->ndpr_ifp->if_xname);
		}
	}

	free(pr, M_IP6NDP);

	nd6_pfxlist_onlink_check();
}

static int
prelist_update(struct nd_prefixctl *newprc,
	struct nd_defrouter *dr, /* may be NULL */
	struct mbuf *m,
	int mcast)
{
	struct in6_ifaddr *ia6_match = NULL;
	struct ifaddr *ifa;
	struct ifnet *ifp = newprc->ndprc_ifp;
	struct nd_prefix *pr;
	int error = 0;
	int auth;
	struct in6_addrlifetime lt6_tmp;
	int ss;
	char ip6buf[INET6_ADDRSTRLEN];

	KASSERT(m != NULL);
	ND6_ASSERT_WLOCK();

	auth = (m->m_flags & M_AUTHIPHDR) ? 1 : 0;

	if ((pr = nd6_prefix_lookup(newprc)) != NULL) {
		/*
		 * nd6_prefix_lookup() ensures that pr and newprc have the same
		 * prefix on a same interface.
		 */

		/*
		 * Update prefix information.  Note that the on-link (L) bit
		 * and the autonomous (A) bit should NOT be changed from 1
		 * to 0.
		 */
		if (newprc->ndprc_raf_onlink == 1)
			pr->ndpr_raf_onlink = 1;
		if (newprc->ndprc_raf_auto == 1)
			pr->ndpr_raf_auto = 1;
		if (newprc->ndprc_raf_onlink) {
			pr->ndpr_vltime = newprc->ndprc_vltime;
			pr->ndpr_pltime = newprc->ndprc_pltime;
			(void)in6_init_prefix_ltimes(pr); /* XXX error case? */
			pr->ndpr_lastupdate = time_uptime;
		}

		if (newprc->ndprc_raf_onlink &&
		    (pr->ndpr_stateflags & NDPRF_ONLINK) == 0) {
			int e;

			if ((e = nd6_prefix_onlink(pr)) != 0) {
				nd6log(LOG_ERR,
				    "failed to make "
				    "the prefix %s/%d on-link on %s "
				    "(errno=%d)\n",
				    IN6_PRINT(ip6buf,
				    &pr->ndpr_prefix.sin6_addr),
				    pr->ndpr_plen, if_name(pr->ndpr_ifp), e);
				/* proceed anyway. XXX: is it correct? */
			}
		}

		if (dr && pfxrtr_lookup(pr, dr) == NULL)
			pfxrtr_add(pr, dr);
	} else {
		struct nd_prefix *newpr = NULL;

		if (newprc->ndprc_vltime == 0)
			goto end;
		if (newprc->ndprc_raf_onlink == 0 && newprc->ndprc_raf_auto == 0)
			goto end;

		if (ip6_rtadv_maxroutes <= nd6_numroutes) {
			ICMP6_STATINC(ICMP6_STAT_DROPPED_RAROUTE);
			goto end;
		}

		error = nd6_prelist_add(newprc, dr, &newpr);
		if (error != 0 || newpr == NULL) {
			nd6log(LOG_NOTICE,
			    "nd6_prelist_add failed for %s/%d on %s "
			    "errno=%d, returnpr=%p\n",
			    IN6_PRINT(ip6buf, &newprc->ndprc_prefix.sin6_addr),
			    newprc->ndprc_plen, if_name(newprc->ndprc_ifp),
			    error, newpr);
			goto end; /* we should just give up in this case. */
		}

		/*
		 * XXX: from the ND point of view, we can ignore a prefix
		 * with the on-link bit being zero.  However, we need a
		 * prefix structure for references from autoconfigured
		 * addresses.  Thus, we explicitly make sure that the prefix
		 * itself expires now.
		 */
		if (newpr->ndpr_raf_onlink == 0) {
			newpr->ndpr_vltime = 0;
			newpr->ndpr_pltime = 0;
			in6_init_prefix_ltimes(newpr);
		}

		pr = newpr;
	}

	/*
	 * Address autoconfiguration based on Section 5.5.3 of RFC 2462.
	 * Note that pr must be non NULL at this point.
	 */

	/* 5.5.3 (a). Ignore the prefix without the A bit set. */
	if (!newprc->ndprc_raf_auto)
		goto end;

	/*
	 * 5.5.3 (b). the link-local prefix should have been ignored in
	 * nd6_ra_input.
	 */

	/* 5.5.3 (c). Consistency check on lifetimes: pltime <= vltime. */
	if (newprc->ndprc_pltime > newprc->ndprc_vltime) {
		error = EINVAL;	/* XXX: won't be used */
		goto end;
	}

	/*
	 * 5.5.3 (d).  If the prefix advertised is not equal to the prefix of
	 * an address configured by stateless autoconfiguration already in the
	 * list of addresses associated with the interface, and the Valid
	 * Lifetime is not 0, form an address.  We first check if we have
	 * a matching prefix.
	 * Note: we apply a clarification in rfc2462bis-02 here.  We only
	 * consider autoconfigured addresses while RFC2462 simply said
	 * "address".
	 */
	ss = pserialize_read_enter();
	IFADDR_READER_FOREACH(ifa, ifp) {
		struct in6_ifaddr *ia6;
		u_int32_t remaininglifetime;

		if (ifa->ifa_addr->sa_family != AF_INET6)
			continue;

		ia6 = (struct in6_ifaddr *)ifa;

		/*
		 * We only consider autoconfigured addresses as per rfc2462bis.
		 */
		if (!(ia6->ia6_flags & IN6_IFF_AUTOCONF))
			continue;

		/*
		 * Spec is not clear here, but I believe we should concentrate
		 * on unicast (i.e. not anycast) addresses.
		 * XXX: other ia6_flags? detached or duplicated?
		 */
		if ((ia6->ia6_flags & IN6_IFF_ANYCAST) != 0)
			continue;

		/*
		 * Ignore the address if it is not associated with a prefix
		 * or is associated with a prefix that is different from this
		 * one.  (pr is never NULL here)
		 */
		if (ia6->ia6_ndpr != pr)
			continue;

		if (ia6_match == NULL) /* remember the first one */
			ia6_match = ia6;

		/*
		 * An already autoconfigured address matched.  Now that we
		 * are sure there is at least one matched address, we can
		 * proceed to 5.5.3. (e): update the lifetimes according to the
		 * "two hours" rule and the privacy extension.
		 * We apply some clarifications in rfc2462bis:
		 * - use remaininglifetime instead of storedlifetime as a
		 *   variable name
		 * - remove the dead code in the "two-hour" rule
		 */
#define TWOHOUR		(120*60)
		lt6_tmp = ia6->ia6_lifetime;
		if (lt6_tmp.ia6t_vltime == ND6_INFINITE_LIFETIME)
			remaininglifetime = ND6_INFINITE_LIFETIME;
		else if (time_uptime - ia6->ia6_updatetime >
			 lt6_tmp.ia6t_vltime) {
			/*
			 * The case of "invalid" address.  We should usually
			 * not see this case.
			 */
			remaininglifetime = 0;
		} else
			remaininglifetime = lt6_tmp.ia6t_vltime -
			    (time_uptime - ia6->ia6_updatetime);

		/* when not updating, keep the current stored lifetime. */
		lt6_tmp.ia6t_vltime = remaininglifetime;

		if (TWOHOUR < newprc->ndprc_vltime ||
		    remaininglifetime < newprc->ndprc_vltime) {
			lt6_tmp.ia6t_vltime = newprc->ndprc_vltime;
		} else if (remaininglifetime <= TWOHOUR) {
			if (auth)
				lt6_tmp.ia6t_vltime = newprc->ndprc_vltime;
		} else {
			/*
			 * newprc->ndprc_vltime <= TWOHOUR &&
			 * TWOHOUR < remaininglifetime
			 */
			lt6_tmp.ia6t_vltime = TWOHOUR;
		}

		/* The 2 hour rule is not imposed for preferred lifetime. */
		lt6_tmp.ia6t_pltime = newprc->ndprc_pltime;

		in6_init_address_ltimes(pr, &lt6_tmp);

		/*
		 * We need to treat lifetimes for temporary addresses
		 * differently, according to
		 * draft-ietf-ipv6-privacy-addrs-v2-01.txt 3.3 (1);
		 * we only update the lifetimes when they are in the maximum
		 * intervals.
		 */
		if ((ia6->ia6_flags & IN6_IFF_TEMPORARY) != 0) {
			u_int32_t maxvltime, maxpltime;

			if (ip6_temp_valid_lifetime >
			    (u_int32_t)((time_uptime - ia6->ia6_createtime) +
			    ip6_desync_factor)) {
				maxvltime = ip6_temp_valid_lifetime -
				    (time_uptime - ia6->ia6_createtime) -
				    ip6_desync_factor;
			} else
				maxvltime = 0;
			if (ip6_temp_preferred_lifetime >
			    (u_int32_t)((time_uptime - ia6->ia6_createtime) +
			    ip6_desync_factor)) {
				maxpltime = ip6_temp_preferred_lifetime -
				    (time_uptime - ia6->ia6_createtime) -
				    ip6_desync_factor;
			} else
				maxpltime = 0;

			if (lt6_tmp.ia6t_vltime == ND6_INFINITE_LIFETIME ||
			    lt6_tmp.ia6t_vltime > maxvltime) {
				lt6_tmp.ia6t_vltime = maxvltime;
			}
			if (lt6_tmp.ia6t_pltime == ND6_INFINITE_LIFETIME ||
			    lt6_tmp.ia6t_pltime > maxpltime) {
				lt6_tmp.ia6t_pltime = maxpltime;
			}
		}

		ia6->ia6_lifetime = lt6_tmp;
		ia6->ia6_updatetime = time_uptime;
	}
	pserialize_read_exit(ss);

	if (ia6_match == NULL && newprc->ndprc_vltime) {
		int ifidlen;
		struct in6_ifaddr *ia6;
		struct psref psref;

		/*
		 * 5.5.3 (d) (continued)
		 * No address matched and the valid lifetime is non-zero.
		 * Create a new address.
		 */

		/*
		 * Prefix Length check:
		 * If the sum of the prefix length and interface identifier
		 * length does not equal 128 bits, the Prefix Information
		 * option MUST be ignored.  The length of the interface
		 * identifier is defined in a separate link-type specific
		 * document.
		 */
		ifidlen = in6_if2idlen(ifp);
		if (ifidlen < 0) {
			/* this should not happen, so we always log it. */
			log(LOG_ERR, "%s: IFID undefined (%s)\n",
			    __func__, if_name(ifp));
			goto end;
		}
		if (ifidlen + pr->ndpr_plen != 128) {
			nd6log(LOG_INFO,
			    "invalid prefixlen %d for %s, ignored\n",
			    pr->ndpr_plen, if_name(ifp));
			goto end;
		}

		if ((ia6 = in6_ifadd(newprc, mcast, &psref)) != NULL) {
			/*
			 * note that we should use pr (not newprc) for reference.
			 */
			pr->ndpr_refcnt++;
			ia6->ia6_ndpr = pr;

			/* toggle onlink state if the address was assigned
			 * a prefix route. */
			if (ia6->ia_flags & IFA_ROUTE)
				pr->ndpr_stateflags |= NDPRF_ONLINK;

			/*
			 * draft-ietf-ipngwg-temp-addresses-v2-00 3.3 (2).
			 * When a new public address is created as described
			 * in RFC2462, also create a new temporary address.
			 *
			 * draft-ietf-ipngwg-temp-addresses-v2-00 3.5.
			 * When an interface connects to a new link, a new
			 * randomized interface identifier should be generated
			 * immediately together with a new set of temporary
			 * addresses.  Thus, we specifiy 1 as the 2nd arg of
			 * in6_tmpifadd().
			 */
			if (ip6_use_tempaddr) {
				int e;
				if ((e = in6_tmpifadd(ia6, 1, 1)) != 0) {
					nd6log(LOG_NOTICE,
					    "failed to create a temporary "
					    "address, errno=%d\n", e);
				}
			}
			ia6_release(ia6, &psref);

			/*
			 * A newly added address might affect the status
			 * of other addresses, so we check and update it.
			 * XXX: what if address duplication happens?
			 */
			nd6_pfxlist_onlink_check();
		} else {
			/* just set an error. do not bark here. */
			error = EADDRNOTAVAIL; /* XXX: might be unused. */
		}
	}

 end:
	return error;
}

/*
 * A supplement function used in the on-link detection below;
 * detect if a given prefix has a (probably) reachable advertising router.
 * XXX: lengthy function name...
 */
static struct nd_pfxrouter *
find_pfxlist_reachable_router(struct nd_prefix *pr)
{
	struct nd_pfxrouter *pfxrtr;

	for (pfxrtr = LIST_FIRST(&pr->ndpr_advrtrs); pfxrtr;
	     pfxrtr = LIST_NEXT(pfxrtr, pfr_entry)) {
		if (pfxrtr->router->ifp->if_flags & IFF_UP &&
		    pfxrtr->router->ifp->if_link_state != LINK_STATE_DOWN &&
		    nd6_is_llinfo_probreach(pfxrtr->router))
			break;	/* found */
	}

	return (pfxrtr);
}

/*
 * Check if each prefix in the prefix list has at least one available router
 * that advertised the prefix (a router is "available" if its neighbor cache
 * entry is reachable or probably reachable).
 * If the check fails, the prefix may be off-link, because, for example,
 * we have moved from the network but the lifetime of the prefix has not
 * expired yet.  So we should not use the prefix if there is another prefix
 * that has an available router.
 * But, if there is no prefix that has an available router, we still regards
 * all the prefixes as on-link.  This is because we can't tell if all the
 * routers are simply dead or if we really moved from the network and there
 * is no router around us.
 */
void
nd6_pfxlist_onlink_check(void)
{
	struct nd_prefix *pr;
	struct in6_ifaddr *ia;
	struct nd_defrouter *dr;
	struct nd_pfxrouter *pfxrtr = NULL;
	int s;
	char ip6buf[INET6_ADDRSTRLEN];

	ND6_ASSERT_WLOCK();

	/*
	 * Check if there is a prefix that has a reachable advertising
	 * router.
	 */
	ND_PREFIX_LIST_FOREACH(pr) {
		if (pr->ndpr_raf_onlink && find_pfxlist_reachable_router(pr))
			break;
	}
	/*
	 * If we have no such prefix, check whether we still have a router
	 * that does not advertise any prefixes.
	 */
	if (pr == NULL) {
		ND_DEFROUTER_LIST_FOREACH(dr) {
			struct nd_prefix *pr0;

			ND_PREFIX_LIST_FOREACH(pr0) {
				if ((pfxrtr = pfxrtr_lookup(pr0, dr)) != NULL)
					break;
			}
			if (pfxrtr)
				break;
		}
	}
	if (pr != NULL || (!ND_DEFROUTER_LIST_EMPTY() && !pfxrtr)) {
		/*
		 * There is at least one prefix that has a reachable router,
		 * or at least a router which probably does not advertise
		 * any prefixes.  The latter would be the case when we move
		 * to a new link where we have a router that does not provide
		 * prefixes and we configure an address by hand.
		 * Detach prefixes which have no reachable advertising
		 * router, and attach other prefixes.
		 */
		ND_PREFIX_LIST_FOREACH(pr) {
			/* XXX: a link-local prefix should never be detached */
			if (IN6_IS_ADDR_LINKLOCAL(&pr->ndpr_prefix.sin6_addr))
				continue;

			/*
			 * we aren't interested in prefixes without the L bit
			 * set.
			 */
			if (pr->ndpr_raf_onlink == 0)
				continue;

			if ((pr->ndpr_stateflags & NDPRF_DETACHED) == 0 &&
			    find_pfxlist_reachable_router(pr) == NULL)
				pr->ndpr_stateflags |= NDPRF_DETACHED;
			if ((pr->ndpr_stateflags & NDPRF_DETACHED) != 0 &&
			    find_pfxlist_reachable_router(pr) != 0)
				pr->ndpr_stateflags &= ~NDPRF_DETACHED;
		}
	} else {
		/* there is no prefix that has a reachable router */
		ND_PREFIX_LIST_FOREACH(pr) {
			if (IN6_IS_ADDR_LINKLOCAL(&pr->ndpr_prefix.sin6_addr))
				continue;

			if (pr->ndpr_raf_onlink == 0)
				continue;

			if ((pr->ndpr_stateflags & NDPRF_DETACHED) != 0)
				pr->ndpr_stateflags &= ~NDPRF_DETACHED;
		}
	}

	/*
	 * Remove each interface route associated with a (just) detached
	 * prefix, and reinstall the interface route for a (just) attached
	 * prefix.  Note that all attempt of reinstallation does not
	 * necessarily success, when a same prefix is shared among multiple
	 * interfaces.  Such cases will be handled in nd6_prefix_onlink,
	 * so we don't have to care about them.
	 */
	ND_PREFIX_LIST_FOREACH(pr) {
		int e;

		if (IN6_IS_ADDR_LINKLOCAL(&pr->ndpr_prefix.sin6_addr))
			continue;

		if (pr->ndpr_raf_onlink == 0)
			continue;

		if ((pr->ndpr_stateflags & NDPRF_DETACHED) != 0 &&
		    (pr->ndpr_stateflags & NDPRF_ONLINK) != 0) {
			if ((e = nd6_prefix_offlink(pr)) != 0) {
				nd6log(LOG_ERR,
				    "failed to make %s/%d offlink, errno=%d\n",
				    IN6_PRINT(ip6buf,
				    &pr->ndpr_prefix.sin6_addr),
				    pr->ndpr_plen, e);
			}
		}
		if ((pr->ndpr_stateflags & NDPRF_DETACHED) == 0 &&
		    (pr->ndpr_stateflags & NDPRF_ONLINK) == 0 &&
		    pr->ndpr_raf_onlink) {
			if ((e = nd6_prefix_onlink(pr)) != 0) {
				nd6log(LOG_ERR,
				    "failed to make %s/%d onlink, errno=%d\n",
				    IN6_PRINT(ip6buf,
				    &pr->ndpr_prefix.sin6_addr),
				    pr->ndpr_plen, e);
			}
		}
	}

	/*
	 * Changes on the prefix status might affect address status as well.
	 * Make sure that all addresses derived from an attached prefix are
	 * attached, and that all addresses derived from a detached prefix are
	 * detached.  Note, however, that a manually configured address should
	 * always be attached.
	 * The precise detection logic is same as the one for prefixes.
	 */
	s = pserialize_read_enter();
	IN6_ADDRLIST_READER_FOREACH(ia) {
		if (!(ia->ia6_flags & IN6_IFF_AUTOCONF))
			continue;

		if (ia->ia6_ndpr == NULL) {
			/*
			 * This can happen when we first configure the address
			 * (i.e. the address exists, but the prefix does not).
			 * XXX: complicated relationships...
			 */
			continue;
		}

		if (find_pfxlist_reachable_router(ia->ia6_ndpr))
			break;
	}
	pserialize_read_exit(s);

	if (ia) {
		int bound = curlwp_bind();

		s = pserialize_read_enter();
		IN6_ADDRLIST_READER_FOREACH(ia) {
			struct ifaddr *ifa = (struct ifaddr *)ia;
			struct psref psref;

			if ((ia->ia6_flags & IN6_IFF_AUTOCONF) == 0)
				continue;

			if (ia->ia6_ndpr == NULL) /* XXX: see above. */
				continue;

			ia6_acquire(ia, &psref);
			pserialize_read_exit(s);

			if (find_pfxlist_reachable_router(ia->ia6_ndpr)) {
				if (ia->ia6_flags & IN6_IFF_DETACHED) {
					ia->ia6_flags &= ~IN6_IFF_DETACHED;
					ia->ia6_flags |= IN6_IFF_TENTATIVE;
					nd6_dad_start(ifa,
					    0);
					/* We will notify the routing socket
					 * of the DAD result, so no need to
					 * here */
				}
			} else {
				if ((ia->ia6_flags & IN6_IFF_DETACHED) == 0) {
					ia->ia6_flags |= IN6_IFF_DETACHED;
					rt_newaddrmsg(RTM_NEWADDR,
					    ifa, 0, NULL);
				}
			}

			s = pserialize_read_enter();
			ia6_release(ia, &psref);
		}
		pserialize_read_exit(s);
		curlwp_bindx(bound);
	}
	else {
		int bound = curlwp_bind();

		s = pserialize_read_enter();
		IN6_ADDRLIST_READER_FOREACH(ia) {
			if ((ia->ia6_flags & IN6_IFF_AUTOCONF) == 0)
				continue;

			if (ia->ia6_flags & IN6_IFF_DETACHED) {
				struct ifaddr *ifa = (struct ifaddr *)ia;
				struct psref psref;

				ia->ia6_flags &= ~IN6_IFF_DETACHED;
				ia->ia6_flags |= IN6_IFF_TENTATIVE;

				ia6_acquire(ia, &psref);
				pserialize_read_exit(s);

				/* Do we need a delay in this case? */
				nd6_dad_start(ifa, 0);

				s = pserialize_read_enter();
				ia6_release(ia, &psref);
			}
		}
		pserialize_read_exit(s);
		curlwp_bindx(bound);
	}
}

static int
nd6_prefix_onlink(struct nd_prefix *pr)
{
	struct ifaddr *ifa;
	struct ifnet *ifp = pr->ndpr_ifp;
	struct sockaddr_in6 mask6;
	struct nd_prefix *opr;
	u_long rtflags;
	int error = 0;
	struct psref psref;
	int bound;
	char ip6buf[INET6_ADDRSTRLEN];
	char ip6bufp[INET6_ADDRSTRLEN], ip6bufm[INET6_ADDRSTRLEN];

	ND6_ASSERT_WLOCK();

	/* sanity check */
	if ((pr->ndpr_stateflags & NDPRF_ONLINK) != 0) {
		nd6log(LOG_ERR, "%s/%d is already on-link\n",
		    IN6_PRINT(ip6buf, &pr->ndpr_prefix.sin6_addr),
		    pr->ndpr_plen);
		return (EEXIST);
	}

	/*
	 * Add the interface route associated with the prefix.  Before
	 * installing the route, check if there's the same prefix on another
	 * interface, and the prefix has already installed the interface route.
	 * Although such a configuration is expected to be rare, we explicitly
	 * allow it.
	 */
	ND_PREFIX_LIST_FOREACH(opr) {
		if (opr == pr)
			continue;

		if ((opr->ndpr_stateflags & NDPRF_ONLINK) == 0)
			continue;

		if (opr->ndpr_plen == pr->ndpr_plen &&
		    in6_are_prefix_equal(&pr->ndpr_prefix.sin6_addr,
		    &opr->ndpr_prefix.sin6_addr, pr->ndpr_plen))
			return (0);
	}

	/*
	 * We prefer link-local addresses as the associated interface address.
	 */
	/* search for a link-local addr */
	bound = curlwp_bind();
	ifa = (struct ifaddr *)in6ifa_ifpforlinklocal_psref(ifp,
	    IN6_IFF_NOTREADY | IN6_IFF_ANYCAST, &psref);
	if (ifa == NULL) {
		int s = pserialize_read_enter();
		IFADDR_READER_FOREACH(ifa, ifp) {
			if (ifa->ifa_addr->sa_family == AF_INET6)
				break;
		}
		if (ifa != NULL)
			ifa_acquire(ifa, &psref);
		pserialize_read_exit(s);
		/* should we care about ia6_flags? */
	}
	if (ifa == NULL) {
		/*
		 * This can still happen, when, for example, we receive an RA
		 * containing a prefix with the L bit set and the A bit clear,
		 * after removing all IPv6 addresses on the receiving
		 * interface.  This should, of course, be rare though.
		 */
		nd6log(LOG_NOTICE, "failed to find any ifaddr"
		    " to add route for a prefix(%s/%d) on %s\n",
		    IN6_PRINT(ip6buf, &pr->ndpr_prefix.sin6_addr),
		    pr->ndpr_plen, if_name(ifp));
		curlwp_bindx(bound);
		return (0);
	}

	/*
	 * in6_ifinit() sets nd6_rtrequest to ifa_rtrequest for all ifaddrs.
	 * ifa->ifa_rtrequest = nd6_rtrequest;
	 */
	memset(&mask6, 0, sizeof(mask6));
	mask6.sin6_family = AF_INET6;
	mask6.sin6_len = sizeof(mask6);
	mask6.sin6_addr = pr->ndpr_mask;
	/* rtrequest() will probably set RTF_UP, but we're not sure. */
	rtflags = ifa->ifa_flags | RTF_UP;
	if (nd6_need_cache(ifp)) {
		/* explicitly set in case ifa_flags does not set the flag. */
		rtflags |= RTF_CONNECTED;
	} else {
		/*
		 * explicitly clear the cloning bit in case ifa_flags sets it.
		 */
		rtflags &= ~RTF_CONNECTED;
	}
	error = rtrequest_newmsg(RTM_ADD, sin6tosa(&pr->ndpr_prefix),
	    ifa->ifa_addr, sin6tosa(&mask6), rtflags);
	if (error == 0) {
		nd6_numroutes++;
		pr->ndpr_stateflags |= NDPRF_ONLINK;
	} else {
		nd6log(LOG_ERR, "failed to add route for a"
		    " prefix (%s/%d) on %s, gw=%s, mask=%s, flags=%lx "
		    "errno = %d\n",
		    IN6_PRINT(ip6bufp, &pr->ndpr_prefix.sin6_addr),
		    pr->ndpr_plen, if_name(ifp),
		    IN6_PRINT(ip6buf,
		    &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr),
		    IN6_PRINT(ip6bufm, &mask6.sin6_addr), rtflags, error);
	}
	ifa_release(ifa, &psref);
	curlwp_bindx(bound);

	return (error);
}

static int
nd6_prefix_offlink(struct nd_prefix *pr)
{
	int error = 0;
	struct ifnet *ifp = pr->ndpr_ifp;
	struct nd_prefix *opr;
	struct sockaddr_in6 sa6, mask6;
	char ip6buf[INET6_ADDRSTRLEN];

	ND6_ASSERT_WLOCK();

	/* sanity check */
	if ((pr->ndpr_stateflags & NDPRF_ONLINK) == 0) {
		nd6log(LOG_ERR, "%s/%d is already off-link\n",
		    IN6_PRINT(ip6buf, &pr->ndpr_prefix.sin6_addr),
		    pr->ndpr_plen);
		return (EEXIST);
	}

	sockaddr_in6_init(&sa6, &pr->ndpr_prefix.sin6_addr, 0, 0, 0);
	sockaddr_in6_init(&mask6, &pr->ndpr_mask, 0, 0, 0);
	error = rtrequest_newmsg(RTM_DELETE, sin6tosa(&sa6), NULL,
	    sin6tosa(&mask6), 0);
	if (error == 0) {
		pr->ndpr_stateflags &= ~NDPRF_ONLINK;
		nd6_numroutes--;

		/*
		 * There might be the same prefix on another interface,
		 * the prefix which could not be on-link just because we have
		 * the interface route (see comments in nd6_prefix_onlink).
		 * If there's one, try to make the prefix on-link on the
		 * interface.
		 */
		ND_PREFIX_LIST_FOREACH(opr) {
			if (opr == pr)
				continue;

			if ((opr->ndpr_stateflags & NDPRF_ONLINK) != 0)
				continue;

			/*
			 * KAME specific: detached prefixes should not be
			 * on-link.
			 */
			if ((opr->ndpr_stateflags & NDPRF_DETACHED) != 0)
				continue;

			if (opr->ndpr_plen == pr->ndpr_plen &&
			    in6_are_prefix_equal(&pr->ndpr_prefix.sin6_addr,
			    &opr->ndpr_prefix.sin6_addr, pr->ndpr_plen)) {
				int e;

				if ((e = nd6_prefix_onlink(opr)) != 0) {
					nd6log(LOG_ERR, "failed to "
					    "recover a prefix %s/%d from %s "
					    "to %s (errno = %d)\n",
					    IN6_PRINT(ip6buf,
					    &opr->ndpr_prefix.sin6_addr),
					    opr->ndpr_plen, if_name(ifp),
					    if_name(opr->ndpr_ifp), e);
				}
			}
		}
	} else {
		/* XXX: can we still set the NDPRF_ONLINK flag? */
		nd6log(LOG_ERR, "failed to delete route: "
		    "%s/%d on %s (errno = %d)\n",
		    IN6_PRINT(ip6buf, &sa6.sin6_addr), pr->ndpr_plen,
		    if_name(ifp),
		    error);
	}

	return error;
}

static struct in6_ifaddr *
in6_ifadd(struct nd_prefixctl *prc, int mcast, struct psref *psref)
{
	struct ifnet *ifp = prc->ndprc_ifp;
	struct ifaddr *ifa;
	struct in6_aliasreq ifra;
	struct in6_ifaddr *ia, *ib;
	int error, plen0;
	struct in6_addr mask;
	int prefixlen = prc->ndprc_plen;
	int updateflags;
	int s;
	char ip6buf[INET6_ADDRSTRLEN];

	ND6_ASSERT_WLOCK();

	in6_prefixlen2mask(&mask, prefixlen);

	/*
	 * find a link-local address (will be interface ID).
	 * Is it really mandatory? Theoretically, a global or a site-local
	 * address can be configured without a link-local address, if we
	 * have a unique interface identifier...
	 *
	 * it is not mandatory to have a link-local address, we can generate
	 * interface identifier on the fly.  we do this because:
	 * (1) it should be the easiest way to find interface identifier.
	 * (2) RFC2462 5.4 suggesting the use of the same interface identifier
	 * for multiple addresses on a single interface, and possible shortcut
	 * of DAD.  we omitted DAD for this reason in the past.
	 * (3) a user can prevent autoconfiguration of global address
	 * by removing link-local address by hand (this is partly because we
	 * don't have other way to control the use of IPv6 on an interface.
	 * this has been our design choice - cf. NRL's "ifconfig auto").
	 * (4) it is easier to manage when an interface has addresses
	 * with the same interface identifier, than to have multiple addresses
	 * with different interface identifiers.
	 */
	s = pserialize_read_enter();
	ifa = (struct ifaddr *)in6ifa_ifpforlinklocal(ifp, 0); /* 0 is OK? */
	if (ifa)
		ib = (struct in6_ifaddr *)ifa;
	else {
		pserialize_read_exit(s);
		return NULL;
	}

#if 0 /* don't care link local addr state, and always do DAD */
	/* if link-local address is not eligible, do not autoconfigure. */
	if (((struct in6_ifaddr *)ifa)->ia6_flags & IN6_IFF_NOTREADY) {
		printf("in6_ifadd: link-local address not ready\n");
		return NULL;
	}
#endif

	/* prefixlen + ifidlen must be equal to 128 */
	plen0 = in6_mask2len(&ib->ia_prefixmask.sin6_addr, NULL);
	if (prefixlen != plen0) {
		nd6log(LOG_INFO, "wrong prefixlen for %s "
		    "(prefix=%d ifid=%d)\n",
		    if_name(ifp), prefixlen, 128 - plen0);
		pserialize_read_exit(s);
		return NULL;
	}

	/* make ifaddr */

	memset(&ifra, 0, sizeof(ifra));
	/*
	 * in6_update_ifa() does not use ifra_name, but we accurately set it
	 * for safety.
	 */
	strncpy(ifra.ifra_name, if_name(ifp), sizeof(ifra.ifra_name));
	sockaddr_in6_init(&ifra.ifra_addr, &prc->ndprc_prefix.sin6_addr, 0, 0, 0);
	/* prefix */
	ifra.ifra_addr.sin6_addr.s6_addr32[0] &= mask.s6_addr32[0];
	ifra.ifra_addr.sin6_addr.s6_addr32[1] &= mask.s6_addr32[1];
	ifra.ifra_addr.sin6_addr.s6_addr32[2] &= mask.s6_addr32[2];
	ifra.ifra_addr.sin6_addr.s6_addr32[3] &= mask.s6_addr32[3];

	/* interface ID */
	ifra.ifra_addr.sin6_addr.s6_addr32[0] |=
	    (ib->ia_addr.sin6_addr.s6_addr32[0] & ~mask.s6_addr32[0]);
	ifra.ifra_addr.sin6_addr.s6_addr32[1] |=
	    (ib->ia_addr.sin6_addr.s6_addr32[1] & ~mask.s6_addr32[1]);
	ifra.ifra_addr.sin6_addr.s6_addr32[2] |=
	    (ib->ia_addr.sin6_addr.s6_addr32[2] & ~mask.s6_addr32[2]);
	ifra.ifra_addr.sin6_addr.s6_addr32[3] |=
	    (ib->ia_addr.sin6_addr.s6_addr32[3] & ~mask.s6_addr32[3]);
	pserialize_read_exit(s);

	/* new prefix mask. */
	sockaddr_in6_init(&ifra.ifra_prefixmask, &mask, 0, 0, 0);

	/* lifetimes */
	ifra.ifra_lifetime.ia6t_vltime = prc->ndprc_vltime;
	ifra.ifra_lifetime.ia6t_pltime = prc->ndprc_pltime;

	/* XXX: scope zone ID? */

	ifra.ifra_flags |= IN6_IFF_AUTOCONF; /* obey autoconf */

	/*
	 * Make sure that we do not have this address already.  This should
	 * usually not happen, but we can still see this case, e.g., if we
	 * have manually configured the exact address to be configured.
	 */
	s = pserialize_read_enter();
	if (in6ifa_ifpwithaddr(ifp, &ifra.ifra_addr.sin6_addr) != NULL) {
		/* this should be rare enough to make an explicit log */
		log(LOG_INFO, "in6_ifadd: %s is already configured\n",
		    IN6_PRINT(ip6buf, &ifra.ifra_addr.sin6_addr));
		pserialize_read_exit(s);
		return (NULL);
	}
	pserialize_read_exit(s);

	/*
	 * Allocate ifaddr structure, link into chain, etc.
	 * If we are going to create a new address upon receiving a multicasted
	 * RA, we need to impose a random delay before starting DAD.
	 * [draft-ietf-ipv6-rfc2462bis-02.txt, Section 5.4.2]
	 */
	updateflags = 0;
	if (mcast)
		updateflags |= IN6_IFAUPDATE_DADDELAY;
	if ((error = in6_update_ifa(ifp, &ifra, updateflags)) != 0) {
		nd6log(LOG_ERR, "failed to make ifaddr %s on %s (errno=%d)\n",
		    IN6_PRINT(ip6buf, &ifra.ifra_addr.sin6_addr), if_name(ifp),
		    error);
		return (NULL);	/* ifaddr must not have been allocated. */
	}

	ia = in6ifa_ifpwithaddr_psref(ifp, &ifra.ifra_addr.sin6_addr, psref);

	return (ia);		/* this is always non-NULL */
}

int
in6_tmpifadd(
	const struct in6_ifaddr *ia0, /* corresponding public address */
	int forcegen,
	int dad_delay)
{
	struct ifnet *ifp = ia0->ia_ifa.ifa_ifp;
	struct in6_ifaddr *newia, *ia;
	struct in6_aliasreq ifra;
	int i, error;
	int trylimit = 3;	/* XXX: adhoc value */
	int updateflags;
	u_int32_t randid[2];
	u_int32_t vltime0, pltime0;
	int s;

	ND6_ASSERT_WLOCK();

	memset(&ifra, 0, sizeof(ifra));
	strncpy(ifra.ifra_name, if_name(ifp), sizeof(ifra.ifra_name));
	ifra.ifra_addr = ia0->ia_addr;
	/* copy prefix mask */
	ifra.ifra_prefixmask = ia0->ia_prefixmask;
	/* clear the old IFID */
	for (i = 0; i < 4; i++) {
		ifra.ifra_addr.sin6_addr.s6_addr32[i] &=
		    ifra.ifra_prefixmask.sin6_addr.s6_addr32[i];
	}

  again:
	if (in6_get_tmpifid(ifp, (u_int8_t *)randid,
	    (const u_int8_t *)&ia0->ia_addr.sin6_addr.s6_addr[8], forcegen)) {
		nd6log(LOG_NOTICE, "failed to find a good random IFID\n");
		return (EINVAL);
	}
	ifra.ifra_addr.sin6_addr.s6_addr32[2] |=
	    (randid[0] & ~(ifra.ifra_prefixmask.sin6_addr.s6_addr32[2]));
	ifra.ifra_addr.sin6_addr.s6_addr32[3] |=
	    (randid[1] & ~(ifra.ifra_prefixmask.sin6_addr.s6_addr32[3]));

	/*
	 * in6_get_tmpifid() quite likely provided a unique interface ID.
	 * However, we may still have a chance to see collision, because
	 * there may be a time lag between generation of the ID and generation
	 * of the address.  So, we'll do one more sanity check.
	 */
	s = pserialize_read_enter();
	IN6_ADDRLIST_READER_FOREACH(ia) {
		if (IN6_ARE_ADDR_EQUAL(&ia->ia_addr.sin6_addr,
		    &ifra.ifra_addr.sin6_addr)) {
			pserialize_read_exit(s);
			if (trylimit-- == 0) {
				/*
				 * Give up.  Something strange should have
				 * happened.
				 */
				nd6log(LOG_NOTICE,
				    "failed to find a unique random IFID\n");
				return (EEXIST);
			}
			forcegen = 1;
			goto again;
		}
	}
	pserialize_read_exit(s);

	/*
	 * The Valid Lifetime is the lower of the Valid Lifetime of the
         * public address or TEMP_VALID_LIFETIME.
	 * The Preferred Lifetime is the lower of the Preferred Lifetime
         * of the public address or TEMP_PREFERRED_LIFETIME -
         * DESYNC_FACTOR.
	 */
	if (ia0->ia6_lifetime.ia6t_vltime != ND6_INFINITE_LIFETIME) {
		vltime0 = IFA6_IS_INVALID(ia0) ? 0 :
		    (ia0->ia6_lifetime.ia6t_vltime -
		    (time_uptime - ia0->ia6_updatetime));
		if (vltime0 > ip6_temp_valid_lifetime)
			vltime0 = ip6_temp_valid_lifetime;
	} else
		vltime0 = ip6_temp_valid_lifetime;
	if (ia0->ia6_lifetime.ia6t_pltime != ND6_INFINITE_LIFETIME) {
		pltime0 = IFA6_IS_DEPRECATED(ia0) ? 0 :
		    (ia0->ia6_lifetime.ia6t_pltime -
		    (time_uptime - ia0->ia6_updatetime));
		if (pltime0 > ip6_temp_preferred_lifetime - ip6_desync_factor){
			pltime0 = ip6_temp_preferred_lifetime -
			    ip6_desync_factor;
		}
	} else
		pltime0 = ip6_temp_preferred_lifetime - ip6_desync_factor;
	ifra.ifra_lifetime.ia6t_vltime = vltime0;
	ifra.ifra_lifetime.ia6t_pltime = pltime0;

	/*
	 * A temporary address is created only if this calculated Preferred
	 * Lifetime is greater than REGEN_ADVANCE time units.
	 */
	if (ifra.ifra_lifetime.ia6t_pltime <= ip6_temp_regen_advance)
		return (0);

	/* XXX: scope zone ID? */

	ifra.ifra_flags |= (IN6_IFF_AUTOCONF|IN6_IFF_TEMPORARY);

	/* allocate ifaddr structure, link into chain, etc. */
	updateflags = 0;
	if (dad_delay)
		updateflags |= IN6_IFAUPDATE_DADDELAY;
	if ((error = in6_update_ifa(ifp, &ifra, updateflags)) != 0)
		return (error);

	s = pserialize_read_enter();
	newia = in6ifa_ifpwithaddr(ifp, &ifra.ifra_addr.sin6_addr);
	if (newia == NULL) {	/* XXX: can it happen? */
		pserialize_read_exit(s);
		nd6log(LOG_ERR,
		    "ifa update succeeded, but we got no ifaddr\n");
		return (EINVAL); /* XXX */
	}
	newia->ia6_ndpr = ia0->ia6_ndpr;
	newia->ia6_ndpr->ndpr_refcnt++;
	pserialize_read_exit(s);

	/*
	 * A newly added address might affect the status of other addresses.
	 * XXX: when the temporary address is generated with a new public
	 * address, the onlink check is redundant.  However, it would be safe
	 * to do the check explicitly everywhere a new address is generated,
	 * and, in fact, we surely need the check when we create a new
	 * temporary address due to deprecation of an old temporary address.
	 */
	nd6_pfxlist_onlink_check();

	return (0);
}

static int
in6_init_prefix_ltimes(struct nd_prefix *ndpr)
{

	ND6_ASSERT_WLOCK();

	/* check if preferred lifetime > valid lifetime.  RFC2462 5.5.3 (c) */
	if (ndpr->ndpr_pltime > ndpr->ndpr_vltime) {
		nd6log(LOG_INFO, "preferred lifetime"
		    "(%d) is greater than valid lifetime(%d)\n",
		    (u_int)ndpr->ndpr_pltime, (u_int)ndpr->ndpr_vltime);
		return (EINVAL);
	}
	if (ndpr->ndpr_pltime == ND6_INFINITE_LIFETIME)
		ndpr->ndpr_preferred = 0;
	else
		ndpr->ndpr_preferred = time_uptime + ndpr->ndpr_pltime;
	if (ndpr->ndpr_vltime == ND6_INFINITE_LIFETIME)
		ndpr->ndpr_expire = 0;
	else
		ndpr->ndpr_expire = time_uptime + ndpr->ndpr_vltime;

	return 0;
}

static void
in6_init_address_ltimes(struct nd_prefix *newpr,
    struct in6_addrlifetime *lt6)
{

	/* Valid lifetime must not be updated unless explicitly specified. */
	/* init ia6t_expire */
	if (lt6->ia6t_vltime == ND6_INFINITE_LIFETIME)
		lt6->ia6t_expire = 0;
	else {
		lt6->ia6t_expire = time_uptime;
		lt6->ia6t_expire += lt6->ia6t_vltime;
	}

	/* init ia6t_preferred */
	if (lt6->ia6t_pltime == ND6_INFINITE_LIFETIME)
		lt6->ia6t_preferred = 0;
	else {
		lt6->ia6t_preferred = time_uptime;
		lt6->ia6t_preferred += lt6->ia6t_pltime;
	}
}

/*
 * Delete all the routing table entries that use the specified gateway.
 * XXX: this function causes search through all entries of routing table, so
 * it shouldn't be called when acting as a router.
 */
void
nd6_rt_flush(struct in6_addr *gateway, struct ifnet *ifp)
{
#ifndef NET_MPSAFE
	int s = splsoftnet();
#endif

	/* We'll care only link-local addresses */
	if (!IN6_IS_ADDR_LINKLOCAL(gateway))
		goto out;

	rt_delete_matched_entries(AF_INET6, rt6_deleteroute_matcher, gateway);

out:
#ifndef NET_MPSAFE
	splx(s);
#endif
	return; /* XXX gcc */
}

static int
rt6_deleteroute_matcher(struct rtentry *rt, void *arg)
{
	struct in6_addr *gate = (struct in6_addr *)arg;

	if (rt->rt_gateway == NULL || rt->rt_gateway->sa_family != AF_INET6)
		return (0);

	if (!IN6_ARE_ADDR_EQUAL(gate, &satosin6(rt->rt_gateway)->sin6_addr))
		return (0);

	/*
	 * Do not delete a static route.
	 * XXX: this seems to be a bit ad-hoc. Should we consider the
	 * 'cloned' bit instead?
	 */
	if ((rt->rt_flags & RTF_STATIC) != 0)
		return (0);

	/*
	 * We delete only host route. This means, in particular, we don't
	 * delete default route.
	 */
	if ((rt->rt_flags & RTF_HOST) == 0)
		return (0);

	return 1;
}
