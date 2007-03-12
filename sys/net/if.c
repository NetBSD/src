/*	$NetBSD: if.c,v 1.180.2.2 2007/03/12 05:59:09 rmind Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by William Studenmund and Jason R. Thorpe.
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
 * Copyright (c) 1980, 1986, 1993
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
 *	@(#)if.c	8.5 (Berkeley) 1/9/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if.c,v 1.180.2.2 2007/03/12 05:59:09 rmind Exp $");

#include "opt_inet.h"

#include "opt_compat_linux.h"
#include "opt_compat_svr4.h"
#include "opt_compat_ultrix.h"
#include "opt_compat_43.h"
#include "opt_atalk.h"
#include "opt_natm.h"
#include "opt_pfil_hooks.h"

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/kauth.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net80211/ieee80211.h>
#include <net80211/ieee80211_ioctl.h>
#include <net/if_types.h>
#include <net/radix.h>
#include <net/route.h>
#include <net/netisr.h>
#ifdef NETATALK
#include <netatalk/at_extern.h>
#include <netatalk/at.h>
#endif
#include <net/pfil.h>

#ifdef INET6
#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#endif

#include "carp.h"
#if NCARP > 0
#include <netinet/ip_carp.h>
#endif

#if defined(COMPAT_43) || defined(COMPAT_LINUX) || defined(COMPAT_SVR4) || defined(COMPAT_ULTRIX) || defined(LKM)
#define COMPAT_OSOCK
#include <compat/sys/socket.h>
#endif

MALLOC_DEFINE(M_IFADDR, "ifaddr", "interface address");
MALLOC_DEFINE(M_IFMADDR, "ether_multi", "link-level multicast address");

int	ifqmaxlen = IFQ_MAXLEN;
struct	callout if_slowtimo_ch;

int netisr;			/* scheduling bits for network */

static int	if_rt_walktree(struct radix_node *, void *);

static struct if_clone *if_clone_lookup(const char *, int *);
static int	if_clone_list(struct if_clonereq *);

static LIST_HEAD(, if_clone) if_cloners = LIST_HEAD_INITIALIZER(if_cloners);
static int if_cloners_count;

#ifdef PFIL_HOOKS
struct pfil_head if_pfil;	/* packet filtering hook for interfaces */
#endif

static void if_detach_queues(struct ifnet *, struct ifqueue *);

/*
 * Network interface utility routines.
 *
 * Routines with ifa_ifwith* names take sockaddr *'s as
 * parameters.
 */
void
ifinit(void)
{

	callout_init(&if_slowtimo_ch);
	if_slowtimo(NULL);
#ifdef PFIL_HOOKS
	if_pfil.ph_type = PFIL_TYPE_IFNET;
	if_pfil.ph_ifnet = NULL;
	if (pfil_head_register(&if_pfil) != 0)
		printf("WARNING: unable to register pfil hook\n");
#endif
}

/*
 * Null routines used while an interface is going away.  These routines
 * just return an error.
 */

int
if_nulloutput(struct ifnet *ifp, struct mbuf *m,
    const struct sockaddr *so, struct rtentry *rt)
{

	return (ENXIO);
}

void
if_nullinput(struct ifnet *ifp, struct mbuf *m)
{

	/* Nothing. */
}

void
if_nullstart(struct ifnet *ifp)
{

	/* Nothing. */
}

int
if_nullioctl(struct ifnet *ifp, u_long cmd, void *data)
{

	return (ENXIO);
}

int
if_nullinit(struct ifnet *ifp)
{

	return (ENXIO);
}

void
if_nullstop(struct ifnet *ifp, int disable)
{

	/* Nothing. */
}

void
if_nullwatchdog(struct ifnet *ifp)
{

	/* Nothing. */
}

void
if_nulldrain(struct ifnet *ifp)
{

	/* Nothing. */
}

static u_int if_index = 1;
struct ifnet_head ifnet;
size_t if_indexlim = 0;
struct ifaddr **ifnet_addrs = NULL;
struct ifnet **ifindex2ifnet = NULL;
struct ifnet *lo0ifp;

/*
 * Allocate the link level name for the specified interface.  This
 * is an attachment helper.  It must be called after ifp->if_addrlen
 * is initialized, which may not be the case when if_attach() is
 * called.
 */
void
if_alloc_sadl(struct ifnet *ifp)
{
	unsigned socksize, ifasize;
	int namelen, masklen;
	struct sockaddr_dl *sdl;
	struct ifaddr *ifa;

	/*
	 * If the interface already has a link name, release it
	 * now.  This is useful for interfaces that can change
	 * link types, and thus switch link names often.
	 */
	if (ifp->if_sadl != NULL)
		if_free_sadl(ifp);

	namelen = strlen(ifp->if_xname);
	masklen = offsetof(struct sockaddr_dl, sdl_data[0]) + namelen;
	socksize = masklen + ifp->if_addrlen;
#define ROUNDUP(a) (1 + (((a) - 1) | (sizeof(long) - 1)))
	if (socksize < sizeof(*sdl))
		socksize = sizeof(*sdl);
	socksize = ROUNDUP(socksize);
	ifasize = sizeof(*ifa) + 2 * socksize;
	ifa = (struct ifaddr *)malloc(ifasize, M_IFADDR, M_WAITOK);
	memset((void *)ifa, 0, ifasize);
	sdl = (struct sockaddr_dl *)(ifa + 1);
	sdl->sdl_len = socksize;
	sdl->sdl_family = AF_LINK;
	bcopy(ifp->if_xname, sdl->sdl_data, namelen);
	sdl->sdl_nlen = namelen;
	sdl->sdl_alen = ifp->if_addrlen;
	sdl->sdl_index = ifp->if_index;
	sdl->sdl_type = ifp->if_type;
	ifnet_addrs[ifp->if_index] = ifa;
	IFAREF(ifa);
	ifa->ifa_ifp = ifp;
	ifa->ifa_rtrequest = link_rtrequest;
	TAILQ_INSERT_HEAD(&ifp->if_addrlist, ifa, ifa_list);
	IFAREF(ifa);
	ifa->ifa_addr = (struct sockaddr *)sdl;
	ifp->if_sadl = sdl;
	sdl = (struct sockaddr_dl *)(socksize + (char *)sdl);
	ifa->ifa_netmask = (struct sockaddr *)sdl;
	sdl->sdl_len = masklen;
	while (namelen != 0)
		sdl->sdl_data[--namelen] = 0xff;
}

/*
 * Free the link level name for the specified interface.  This is
 * a detach helper.  This is called from if_detach() or from
 * link layer type specific detach functions.
 */
void
if_free_sadl(struct ifnet *ifp)
{
	struct ifaddr *ifa;
	int s;

	ifa = ifnet_addrs[ifp->if_index];
	if (ifa == NULL) {
		KASSERT(ifp->if_sadl == NULL);
		return;
	}

	KASSERT(ifp->if_sadl != NULL);

	s = splnet();
	rtinit(ifa, RTM_DELETE, 0);
	TAILQ_REMOVE(&ifp->if_addrlist, ifa, ifa_list);
	IFAFREE(ifa);

	ifp->if_sadl = NULL;

	ifnet_addrs[ifp->if_index] = NULL;
	IFAFREE(ifa);
	splx(s);
}

/*
 * Attach an interface to the
 * list of "active" interfaces.
 */
void
if_attach(struct ifnet *ifp)
{
	int indexlim = 0;

	if (if_indexlim == 0) {
		TAILQ_INIT(&ifnet);
		if_indexlim = 8;
	}
	TAILQ_INIT(&ifp->if_addrlist);
	TAILQ_INSERT_TAIL(&ifnet, ifp, if_list);
	ifp->if_index = if_index;
	if (ifindex2ifnet == 0)
		if_index++;
	else
		while (ifp->if_index < if_indexlim &&
		    ifindex2ifnet[ifp->if_index] != NULL) {
			++if_index;
			if (if_index == 0)
				if_index = 1;
			/*
			 * If we hit USHRT_MAX, we skip back to 0 since
			 * there are a number of places where the value
			 * of if_index or if_index itself is compared
			 * to or stored in an unsigned short.  By
			 * jumping back, we won't botch those assignments
			 * or comparisons.
			 */
			else if (if_index == USHRT_MAX) {
				/*
				 * However, if we have to jump back to
				 * zero *twice* without finding an empty
				 * slot in ifindex2ifnet[], then there
				 * there are too many (>65535) interfaces.
				 */
				if (indexlim++)
					panic("too many interfaces");
				else
					if_index = 1;
			}
			ifp->if_index = if_index;
		}

	/*
	 * We have some arrays that should be indexed by if_index.
	 * since if_index will grow dynamically, they should grow too.
	 *	struct ifadd **ifnet_addrs
	 *	struct ifnet **ifindex2ifnet
	 */
	if (ifnet_addrs == 0 || ifindex2ifnet == 0 ||
	    ifp->if_index >= if_indexlim) {
		size_t m, n, oldlim;
		void *q;

		oldlim = if_indexlim;
		while (ifp->if_index >= if_indexlim)
			if_indexlim <<= 1;

		/* grow ifnet_addrs */
		m = oldlim * sizeof(struct ifaddr *);
		n = if_indexlim * sizeof(struct ifaddr *);
		q = (void *)malloc(n, M_IFADDR, M_WAITOK);
		memset(q, 0, n);
		if (ifnet_addrs) {
			bcopy((void *)ifnet_addrs, q, m);
			free((void *)ifnet_addrs, M_IFADDR);
		}
		ifnet_addrs = (struct ifaddr **)q;

		/* grow ifindex2ifnet */
		m = oldlim * sizeof(struct ifnet *);
		n = if_indexlim * sizeof(struct ifnet *);
		q = (void *)malloc(n, M_IFADDR, M_WAITOK);
		memset(q, 0, n);
		if (ifindex2ifnet) {
			bcopy((void *)ifindex2ifnet, q, m);
			free((void *)ifindex2ifnet, M_IFADDR);
		}
		ifindex2ifnet = (struct ifnet **)q;
	}

	ifindex2ifnet[ifp->if_index] = ifp;

	/*
	 * Link level name is allocated later by a separate call to
	 * if_alloc_sadl().
	 */

	if (ifp->if_snd.ifq_maxlen == 0)
		ifp->if_snd.ifq_maxlen = ifqmaxlen;
	ifp->if_broadcastaddr = 0; /* reliably crash if used uninitialized */

	ifp->if_link_state = LINK_STATE_UNKNOWN;

	ifp->if_capenable = 0;
	ifp->if_csum_flags_tx = 0;
	ifp->if_csum_flags_rx = 0;

#ifdef ALTQ
	ifp->if_snd.altq_type = 0;
	ifp->if_snd.altq_disc = NULL;
	ifp->if_snd.altq_flags &= ALTQF_CANTCHANGE;
	ifp->if_snd.altq_tbr  = NULL;
	ifp->if_snd.altq_ifp  = ifp;
#endif

#ifdef PFIL_HOOKS
	ifp->if_pfil.ph_type = PFIL_TYPE_IFNET;
	ifp->if_pfil.ph_ifnet = ifp;
	if (pfil_head_register(&ifp->if_pfil) != 0)
		printf("%s: WARNING: unable to register pfil hook\n",
		    ifp->if_xname);
	(void)pfil_run_hooks(&if_pfil,
	    (struct mbuf **)PFIL_IFNET_ATTACH, ifp, PFIL_IFNET);
#endif

	if (!STAILQ_EMPTY(&domains))
		if_attachdomain1(ifp);

	/* Announce the interface. */
	rt_ifannouncemsg(ifp, IFAN_ARRIVAL);
}

void
if_attachdomain(void)
{
	struct ifnet *ifp;
	int s;

	s = splnet();
	TAILQ_FOREACH(ifp, &ifnet, if_list)
		if_attachdomain1(ifp);
	splx(s);
}

void
if_attachdomain1(struct ifnet *ifp)
{
	struct domain *dp;
	int s;

	s = splnet();

	/* address family dependent data region */
	memset(ifp->if_afdata, 0, sizeof(ifp->if_afdata));
	DOMAIN_FOREACH(dp) {
		if (dp->dom_ifattach)
			ifp->if_afdata[dp->dom_family] =
			    (*dp->dom_ifattach)(ifp);
	}

	splx(s);
}

/*
 * Deactivate an interface.  This points all of the procedure
 * handles at error stubs.  May be called from interrupt context.
 */
void
if_deactivate(struct ifnet *ifp)
{
	int s;

	s = splnet();

	ifp->if_output	 = if_nulloutput;
	ifp->if_input	 = if_nullinput;
	ifp->if_start	 = if_nullstart;
	ifp->if_ioctl	 = if_nullioctl;
	ifp->if_init	 = if_nullinit;
	ifp->if_stop	 = if_nullstop;
	ifp->if_watchdog = if_nullwatchdog;
	ifp->if_drain	 = if_nulldrain;

	/* No more packets may be enqueued. */
	ifp->if_snd.ifq_maxlen = 0;

	splx(s);
}

/*
 * Detach an interface from the list of "active" interfaces,
 * freeing any resources as we go along.
 *
 * NOTE: This routine must be called with a valid thread context,
 * as it may block.
 */
void
if_detach(struct ifnet *ifp)
{
	struct socket so;
	struct ifaddr *ifa;
#ifdef IFAREF_DEBUG
	struct ifaddr *last_ifa = NULL;
#endif
	struct domain *dp;
	const struct protosw *pr;
	struct radix_node_head *rnh;
	int s, i, family, purged;

	/*
	 * XXX It's kind of lame that we have to have the
	 * XXX socket structure...
	 */
	memset(&so, 0, sizeof(so));

	s = splnet();

	/*
	 * Do an if_down() to give protocols a chance to do something.
	 */
	if_down(ifp);

#ifdef ALTQ
	if (ALTQ_IS_ENABLED(&ifp->if_snd))
		altq_disable(&ifp->if_snd);
	if (ALTQ_IS_ATTACHED(&ifp->if_snd))
		altq_detach(&ifp->if_snd);
#endif


#if NCARP > 0
	/* Remove the interface from any carp group it is a part of.  */
	if (ifp->if_carp && ifp->if_type != IFT_CARP)
		carp_ifdetach(ifp);
#endif

#ifdef PFIL_HOOKS
	(void)pfil_run_hooks(&if_pfil,
	    (struct mbuf **)PFIL_IFNET_DETACH, ifp, PFIL_IFNET);
	(void)pfil_head_unregister(&ifp->if_pfil);
#endif

	/*
	 * Rip all the addresses off the interface.  This should make
	 * all of the routes go away.
	 *
	 * pr_usrreq calls can remove an arbitrary number of ifaddrs
	 * from the list, including our "cursor", ifa.  For safety,
	 * and to honor the TAILQ abstraction, I just restart the
	 * loop after each removal.  Note that the loop will exit
	 * when all of the remaining ifaddrs belong to the AF_LINK
	 * family.  I am counting on the historical fact that at
	 * least one pr_usrreq in each address domain removes at
	 * least one ifaddr.
	 */
again:
	TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
		family = ifa->ifa_addr->sa_family;
#ifdef IFAREF_DEBUG
		printf("if_detach: ifaddr %p, family %d, refcnt %d\n",
		    ifa, family, ifa->ifa_refcnt);
		if (last_ifa != NULL && ifa == last_ifa)
			panic("if_detach: loop detected");
		last_ifa = ifa;
#endif
		if (family == AF_LINK)
			continue;
		dp = pffinddomain(family);
#ifdef DIAGNOSTIC
		if (dp == NULL)
			panic("if_detach: no domain for AF %d",
			    family);
#endif
		/*
		 * XXX These PURGEIF calls are redundant with the
		 * purge-all-families calls below, but are left in for
		 * now both to make a smaller change, and to avoid
		 * unplanned interactions with clearing of
		 * ifp->if_addrlist.
		 */
		purged = 0;
		for (pr = dp->dom_protosw;
		     pr < dp->dom_protoswNPROTOSW; pr++) {
			so.so_proto = pr;
			if (pr->pr_usrreq != NULL) {
				(void) (*pr->pr_usrreq)(&so,
				    PRU_PURGEIF, NULL, NULL,
				    (struct mbuf *) ifp, curlwp);
				purged = 1;
			}
		}
		if (purged == 0) {
			/*
			 * XXX What's really the best thing to do
			 * XXX here?  --thorpej@NetBSD.org
			 */
			printf("if_detach: WARNING: AF %d not purged\n",
			    family);
			TAILQ_REMOVE(&ifp->if_addrlist, ifa, ifa_list);
		}
		goto again;
	}

	if_free_sadl(ifp);

	/* Walk the routing table looking for stragglers. */
	for (i = 0; i <= AF_MAX; i++) {
		if ((rnh = rt_tables[i]) != NULL)
			(void) (*rnh->rnh_walktree)(rnh, if_rt_walktree, ifp);
	}

	DOMAIN_FOREACH(dp) {
		if (dp->dom_ifdetach && ifp->if_afdata[dp->dom_family])
			(*dp->dom_ifdetach)(ifp,
			    ifp->if_afdata[dp->dom_family]);

		/*
		 * One would expect multicast memberships (INET and
		 * INET6) on UDP sockets to be purged by the PURGEIF
		 * calls above, but if all addresses were removed from
		 * the interface prior to destruction, the calls will
		 * not be made (e.g. ppp, for which pppd(8) generally
		 * removes addresses before destroying the interface).
		 * Because there is no invariant that multicast
		 * memberships only exist for interfaces with IPv4
		 * addresses, we must call PURGEIF regardless of
		 * addresses.  (Protocols which might store ifnet
		 * pointers are marked with PR_PURGEIF.)
		 */
		for (pr = dp->dom_protosw;
		     pr < dp->dom_protoswNPROTOSW; pr++) {
			so.so_proto = pr;
			if (pr->pr_usrreq != NULL &&
			    pr->pr_flags & PR_PURGEIF)
				(void) (*pr->pr_usrreq)(&so,
				    PRU_PURGEIF, NULL, NULL,
				    (struct mbuf *) ifp, curlwp);
		}
	}

	/* Announce that the interface is gone. */
	rt_ifannouncemsg(ifp, IFAN_DEPARTURE);

	ifindex2ifnet[ifp->if_index] = NULL;

	TAILQ_REMOVE(&ifnet, ifp, if_list);

	/*
	 * remove packets that came from ifp, from software interrupt queues.
	 */
	DOMAIN_FOREACH(dp) {
		for (i = 0; i < __arraycount(dp->dom_ifqueues); i++) {
			if (dp->dom_ifqueues[i] == NULL)
				break;
			if_detach_queues(ifp, dp->dom_ifqueues[i]);
		}
	}

	splx(s);
}

static void
if_detach_queues(struct ifnet *ifp, struct ifqueue *q)
{
	struct mbuf *m, *prev, *next;

	prev = NULL;
	for (m = q->ifq_head; m; m = next) {
		next = m->m_nextpkt;
#ifdef DIAGNOSTIC
		if ((m->m_flags & M_PKTHDR) == 0) {
			prev = m;
			continue;
		}
#endif
		if (m->m_pkthdr.rcvif != ifp) {
			prev = m;
			continue;
		}

		if (prev)
			prev->m_nextpkt = m->m_nextpkt;
		else
			q->ifq_head = m->m_nextpkt;
		if (q->ifq_tail == m)
			q->ifq_tail = prev;
		q->ifq_len--;

		m->m_nextpkt = NULL;
		m_freem(m);
		IF_DROP(q);
	}
}

/*
 * Callback for a radix tree walk to delete all references to an
 * ifnet.
 */
static int
if_rt_walktree(struct radix_node *rn, void *v)
{
	struct ifnet *ifp = (struct ifnet *)v;
	struct rtentry *rt = (struct rtentry *)rn;
	int error;

	if (rt->rt_ifp == ifp) {
		/* Delete the entry. */
		++rt->rt_refcnt;
		error = rtrequest(RTM_DELETE, rt_key(rt), rt->rt_gateway,
		    rt_mask(rt), rt->rt_flags, NULL);
		KASSERT((rt->rt_flags & RTF_UP) == 0);
		rt->rt_ifp = NULL;
		RTFREE(rt);
		if (error)
			printf("%s: warning: unable to delete rtentry @ %p, "
			    "error = %d\n", ifp->if_xname, rt, error);
	}
	return (0);
}

/*
 * Create a clone network interface.
 */
int
if_clone_create(const char *name)
{
	struct if_clone *ifc;
	int unit;

	ifc = if_clone_lookup(name, &unit);
	if (ifc == NULL)
		return (EINVAL);

	if (ifunit(name) != NULL)
		return (EEXIST);

	return ((*ifc->ifc_create)(ifc, unit));
}

/*
 * Destroy a clone network interface.
 */
int
if_clone_destroy(const char *name)
{
	struct if_clone *ifc;
	struct ifnet *ifp;

	ifc = if_clone_lookup(name, NULL);
	if (ifc == NULL)
		return (EINVAL);

	ifp = ifunit(name);
	if (ifp == NULL)
		return (ENXIO);

	if (ifc->ifc_destroy == NULL)
		return (EOPNOTSUPP);

	return ((*ifc->ifc_destroy)(ifp));
}

/*
 * Look up a network interface cloner.
 */
static struct if_clone *
if_clone_lookup(const char *name, int *unitp)
{
	struct if_clone *ifc;
	const char *cp;
	int unit;

	/* separate interface name from unit */
	for (cp = name;
	    cp - name < IFNAMSIZ && *cp && (*cp < '0' || *cp > '9');
	    cp++)
		continue;

	if (cp == name || cp - name == IFNAMSIZ || !*cp)
		return (NULL);	/* No name or unit number */

	LIST_FOREACH(ifc, &if_cloners, ifc_list) {
		if (strlen(ifc->ifc_name) == cp - name &&
		    !strncmp(name, ifc->ifc_name, cp - name))
			break;
	}

	if (ifc == NULL)
		return (NULL);

	unit = 0;
	while (cp - name < IFNAMSIZ && *cp) {
		if (*cp < '0' || *cp > '9' || unit > INT_MAX / 10) {
			/* Bogus unit number. */
			return (NULL);
		}
		unit = (unit * 10) + (*cp++ - '0');
	}

	if (unitp != NULL)
		*unitp = unit;
	return (ifc);
}

/*
 * Register a network interface cloner.
 */
void
if_clone_attach(struct if_clone *ifc)
{

	LIST_INSERT_HEAD(&if_cloners, ifc, ifc_list);
	if_cloners_count++;
}

/*
 * Unregister a network interface cloner.
 */
void
if_clone_detach(struct if_clone *ifc)
{

	LIST_REMOVE(ifc, ifc_list);
	if_cloners_count--;
}

/*
 * Provide list of interface cloners to userspace.
 */
static int
if_clone_list(struct if_clonereq *ifcr)
{
	char outbuf[IFNAMSIZ], *dst;
	struct if_clone *ifc;
	int count, error = 0;

	ifcr->ifcr_total = if_cloners_count;
	if ((dst = ifcr->ifcr_buffer) == NULL) {
		/* Just asking how many there are. */
		return (0);
	}

	if (ifcr->ifcr_count < 0)
		return (EINVAL);

	count = (if_cloners_count < ifcr->ifcr_count) ?
	    if_cloners_count : ifcr->ifcr_count;

	for (ifc = LIST_FIRST(&if_cloners); ifc != NULL && count != 0;
	     ifc = LIST_NEXT(ifc, ifc_list), count--, dst += IFNAMSIZ) {
		(void)strncpy(outbuf, ifc->ifc_name, sizeof(outbuf));
		if (outbuf[sizeof(outbuf) - 1] != '\0')
			return ENAMETOOLONG;
		error = copyout(outbuf, dst, sizeof(outbuf));
		if (error)
			break;
	}

	return (error);
}

/*
 * Locate an interface based on a complete address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithaddr(const struct sockaddr *addr)
{
	struct ifnet *ifp;
	struct ifaddr *ifa;

#define	equal(a1, a2) \
  (bcmp((a1), (a2), ((const struct sockaddr *)(a1))->sa_len) == 0)

	TAILQ_FOREACH(ifp, &ifnet, if_list) {
		if (ifp->if_output == if_nulloutput)
			continue;
		TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
			if (ifa->ifa_addr->sa_family != addr->sa_family)
				continue;
			if (equal(addr, ifa->ifa_addr))
				return (ifa);
			if ((ifp->if_flags & IFF_BROADCAST) &&
			    ifa->ifa_broadaddr &&
			    /* IP6 doesn't have broadcast */
			    ifa->ifa_broadaddr->sa_len != 0 &&
			    equal(ifa->ifa_broadaddr, addr))
				return (ifa);
		}
	}
	return (NULL);
}

/*
 * Locate the point to point interface with a given destination address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithdstaddr(const struct sockaddr *addr)
{
	struct ifnet *ifp;
	struct ifaddr *ifa;

	TAILQ_FOREACH(ifp, &ifnet, if_list) {
		if (ifp->if_output == if_nulloutput)
			continue;
		if (ifp->if_flags & IFF_POINTOPOINT) {
			TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
				if (ifa->ifa_addr->sa_family !=
				      addr->sa_family ||
				    ifa->ifa_dstaddr == NULL)
					continue;
				if (equal(addr, ifa->ifa_dstaddr))
					return (ifa);
			}
		}
	}
	return (NULL);
}

/*
 * Find an interface on a specific network.  If many, choice
 * is most specific found.
 */
struct ifaddr *
ifa_ifwithnet(const struct sockaddr *addr)
{
	struct ifnet *ifp;
	struct ifaddr *ifa;
	const struct sockaddr_dl *sdl;
	struct ifaddr *ifa_maybe = 0;
	u_int af = addr->sa_family;
	const char *addr_data = addr->sa_data, *cplim;

	if (af == AF_LINK) {
		sdl = (const struct sockaddr_dl *)addr;
		if (sdl->sdl_index && sdl->sdl_index < if_indexlim &&
		    ifindex2ifnet[sdl->sdl_index] &&
		    ifindex2ifnet[sdl->sdl_index]->if_output != if_nulloutput)
			return (ifnet_addrs[sdl->sdl_index]);
	}
#ifdef NETATALK
	if (af == AF_APPLETALK) {
		const struct sockaddr_at *sat, *sat2;
		sat = (const struct sockaddr_at *)addr;
		TAILQ_FOREACH(ifp, &ifnet, if_list) {
			if (ifp->if_output == if_nulloutput)
				continue;
			ifa = at_ifawithnet((const struct sockaddr_at *)addr, ifp);
			if (ifa == NULL)
				continue;
			sat2 = (struct sockaddr_at *)ifa->ifa_addr;
			if (sat2->sat_addr.s_net == sat->sat_addr.s_net)
				return (ifa); /* exact match */
			if (ifa_maybe == NULL) {
				/* else keep the if with the right range */
				ifa_maybe = ifa;
			}
		}
		return (ifa_maybe);
	}
#endif
	TAILQ_FOREACH(ifp, &ifnet, if_list) {
		if (ifp->if_output == if_nulloutput)
			continue;
		TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
			const char *cp, *cp2, *cp3;

			if (ifa->ifa_addr->sa_family != af ||
			    ifa->ifa_netmask == 0)
 next:				continue;
			cp = addr_data;
			cp2 = ifa->ifa_addr->sa_data;
			cp3 = ifa->ifa_netmask->sa_data;
			cplim = (const char *)ifa->ifa_netmask +
			    ifa->ifa_netmask->sa_len;
			while (cp3 < cplim) {
				if ((*cp++ ^ *cp2++) & *cp3++) {
					/* want to continue for() loop */
					goto next;
				}
			}
			if (ifa_maybe == 0 ||
			    rn_refines((void *)ifa->ifa_netmask,
			    (void *)ifa_maybe->ifa_netmask))
				ifa_maybe = ifa;
		}
	}
	return (ifa_maybe);
}

/*
 * Find the interface of the addresss.
 */
struct ifaddr *
ifa_ifwithladdr(const struct sockaddr *addr)
{
	struct ifaddr *ia;

	if ((ia = ifa_ifwithaddr(addr)) || (ia = ifa_ifwithdstaddr(addr)) ||
	    (ia = ifa_ifwithnet(addr)))
		return (ia);
	return (NULL);
}

/*
 * Find an interface using a specific address family
 */
struct ifaddr *
ifa_ifwithaf(int af)
{
	struct ifnet *ifp;
	struct ifaddr *ifa;

	TAILQ_FOREACH(ifp, &ifnet, if_list) {
		if (ifp->if_output == if_nulloutput)
			continue;
		TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
			if (ifa->ifa_addr->sa_family == af)
				return ifa;
		}
	}
	return NULL;
}

/*
 * Find an interface address specific to an interface best matching
 * a given address.
 */
struct ifaddr *
ifaof_ifpforaddr(const struct sockaddr *addr, struct ifnet *ifp)
{
	struct ifaddr *ifa;
	const char *cp, *cp2, *cp3;
	const char *cplim;
	struct ifaddr *ifa_maybe = 0;
	u_int af = addr->sa_family;

	if (ifp->if_output == if_nulloutput)
		return (NULL);

	if (af >= AF_MAX)
		return (NULL);

	TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
		if (ifa->ifa_addr->sa_family != af)
			continue;
		ifa_maybe = ifa;
		if (ifa->ifa_netmask == 0) {
			if (equal(addr, ifa->ifa_addr) ||
			    (ifa->ifa_dstaddr &&
			     equal(addr, ifa->ifa_dstaddr)))
				return (ifa);
			continue;
		}
		cp = addr->sa_data;
		cp2 = ifa->ifa_addr->sa_data;
		cp3 = ifa->ifa_netmask->sa_data;
		cplim = ifa->ifa_netmask->sa_len + (char *)ifa->ifa_netmask;
		for (; cp3 < cplim; cp3++) {
			if ((*cp++ ^ *cp2++) & *cp3)
				break;
		}
		if (cp3 == cplim)
			return (ifa);
	}
	return (ifa_maybe);
}

/*
 * Default action when installing a route with a Link Level gateway.
 * Lookup an appropriate real ifa to point to.
 * This should be moved to /sys/net/link.c eventually.
 */
void
link_rtrequest(int cmd, struct rtentry *rt, struct rt_addrinfo *info)
{
	struct ifaddr *ifa;
	struct sockaddr *dst;
	struct ifnet *ifp;

	if (cmd != RTM_ADD || ((ifa = rt->rt_ifa) == 0) ||
	    ((ifp = ifa->ifa_ifp) == 0) || ((dst = rt_key(rt)) == 0))
		return;
	if ((ifa = ifaof_ifpforaddr(dst, ifp)) != NULL) {
		rt_replace_ifa(rt, ifa);
		if (ifa->ifa_rtrequest && ifa->ifa_rtrequest != link_rtrequest)
			ifa->ifa_rtrequest(cmd, rt, info);
	}
}

/*
 * Handle a change in the interface link state.
 */
void
if_link_state_change(struct ifnet *ifp, int link_state)
{
	/* Notify that the link state has changed. */
	if (ifp->if_link_state != link_state) {
		ifp->if_link_state = link_state;
		rt_ifmsg(ifp);
#if NCARP > 0
		if (ifp->if_carp)
			carp_carpdev_state(ifp);
#endif
	}
}

/*
 * Mark an interface down and notify protocols of
 * the transition.
 * NOTE: must be called at splsoftnet or equivalent.
 */
void
if_down(struct ifnet *ifp)
{
	struct ifaddr *ifa;

	ifp->if_flags &= ~IFF_UP;
	microtime(&ifp->if_lastchange);
	TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list)
		pfctlinput(PRC_IFDOWN, ifa->ifa_addr);
	IFQ_PURGE(&ifp->if_snd);
#if NCARP > 0
	if (ifp->if_carp)
		carp_carpdev_state(ifp);
#endif
	rt_ifmsg(ifp);
}

/*
 * Mark an interface up and notify protocols of
 * the transition.
 * NOTE: must be called at splsoftnet or equivalent.
 */
void
if_up(struct ifnet *ifp)
{
#ifdef notyet
	struct ifaddr *ifa;
#endif

	ifp->if_flags |= IFF_UP;
	microtime(&ifp->if_lastchange);
#ifdef notyet
	/* this has no effect on IP, and will kill all ISO connections XXX */
	TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list)
		pfctlinput(PRC_IFUP, ifa->ifa_addr);
#endif
#if NCARP > 0
	if (ifp->if_carp)
		carp_carpdev_state(ifp);
#endif
	rt_ifmsg(ifp);
#ifdef INET6
	in6_if_up(ifp);
#endif
}

/*
 * Handle interface watchdog timer routines.  Called
 * from softclock, we decrement timers (if set) and
 * call the appropriate interface routine on expiration.
 */
void
if_slowtimo(void *arg)
{
	struct ifnet *ifp;
	int s = splnet();

	TAILQ_FOREACH(ifp, &ifnet, if_list) {
		if (ifp->if_timer == 0 || --ifp->if_timer)
			continue;
		if (ifp->if_watchdog)
			(*ifp->if_watchdog)(ifp);
	}
	splx(s);
	callout_reset(&if_slowtimo_ch, hz / IFNET_SLOWHZ,
	    if_slowtimo, NULL);
}

/*
 * Set/clear promiscuous mode on interface ifp based on the truth value
 * of pswitch.  The calls are reference counted so that only the first
 * "on" request actually has an effect, as does the final "off" request.
 * Results are undefined if the "off" and "on" requests are not matched.
 */
int
ifpromisc(struct ifnet *ifp, int pswitch)
{
	int pcount, ret;
	short flags;
	struct ifreq ifr;

	pcount = ifp->if_pcount;
	flags = ifp->if_flags;
	if (pswitch) {
		/*
		 * Allow the device to be "placed" into promiscuous
		 * mode even if it is not configured up.  It will
		 * consult IFF_PROMISC when it is is brought up.
		 */
		if (ifp->if_pcount++ != 0)
			return (0);
		ifp->if_flags |= IFF_PROMISC;
		if ((ifp->if_flags & IFF_UP) == 0)
			return (0);
	} else {
		if (--ifp->if_pcount > 0)
			return (0);
		ifp->if_flags &= ~IFF_PROMISC;
		/*
		 * If the device is not configured up, we should not need to
		 * turn off promiscuous mode (device should have turned it
		 * off when interface went down; and will look at IFF_PROMISC
		 * again next time interface comes up).
		 */
		if ((ifp->if_flags & IFF_UP) == 0)
			return (0);
	}
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = ifp->if_flags;
	ret = (*ifp->if_ioctl)(ifp, SIOCSIFFLAGS, (void *) &ifr);
	/* Restore interface state if not successful. */
	if (ret != 0) {
		ifp->if_pcount = pcount;
		ifp->if_flags = flags;
	}
	return (ret);
}

/*
 * Map interface name to
 * interface structure pointer.
 */
struct ifnet *
ifunit(const char *name)
{
	struct ifnet *ifp;
	const char *cp = name;
	u_int unit = 0;
	u_int i;

	/*
	 * If the entire name is a number, treat it as an ifindex.
	 */
	for (i = 0; i < IFNAMSIZ && *cp >= '0' && *cp <= '9'; i++, cp++) {
		unit = unit * 10 + (*cp - '0');
	}

	/*
	 * If the number took all of the name, then it's a valid ifindex.
	 */
	if (i == IFNAMSIZ || (cp != name && *cp == '\0')) {
		if (unit >= if_indexlim)
			return (NULL);
		ifp = ifindex2ifnet[unit];
		if (ifp == NULL || ifp->if_output == if_nulloutput)
			return (NULL);
		return (ifp);
	}

	TAILQ_FOREACH(ifp, &ifnet, if_list) {
		if (ifp->if_output == if_nulloutput)
			continue;
	 	if (strcmp(ifp->if_xname, name) == 0)
			return (ifp);
	}
	return (NULL);
}

/*
 * Interface ioctls.
 */
int
ifioctl(struct socket *so, u_long cmd, void *data, struct lwp *l)
{
	struct ifnet *ifp;
	struct ifreq *ifr;
	struct ifcapreq *ifcr;
	struct ifdatareq *ifdr;
	int s, error = 0;
	short oif_flags;

	switch (cmd) {

	case SIOCGIFCONF:
	case OSIOCGIFCONF:
		return (ifconf(cmd, data));
	}
	ifr = (struct ifreq *)data;
	ifcr = (struct ifcapreq *)data;
	ifdr = (struct ifdatareq *)data;

	ifp = ifunit(ifr->ifr_name);

	switch (cmd) {
	case SIOCIFCREATE:
	case SIOCIFDESTROY:
		if (l) {
			error = kauth_authorize_network(l->l_cred,
			    KAUTH_NETWORK_INTERFACE,
			    KAUTH_REQ_NETWORK_INTERFACE_SETPRIV, ifp,
			    (void *)cmd, NULL);
			if (error)
				return error;
		}
		return ((cmd == SIOCIFCREATE) ?
			if_clone_create(ifr->ifr_name) :
			if_clone_destroy(ifr->ifr_name));

	case SIOCIFGCLONERS:
		return (if_clone_list((struct if_clonereq *)data));
	}

	if (ifp == 0)
		return (ENXIO);

	switch (cmd) {
	case SIOCSIFFLAGS:
	case SIOCSIFCAP:
	case SIOCSIFMETRIC:
	case SIOCZIFDATA:
	case SIOCSIFMTU:
	case SIOCSIFPHYADDR:
	case SIOCDIFPHYADDR:
#ifdef INET6
	case SIOCSIFPHYADDR_IN6:
#endif
	case SIOCSLIFPHYADDR:
	case SIOCADDMULTI:
	case SIOCDELMULTI:
	case SIOCSIFMEDIA:
	case SIOCSDRVSPEC:
	case SIOCS80211NWID:
	case SIOCS80211NWKEY:
	case SIOCS80211POWER:
	case SIOCS80211BSSID:
	case SIOCS80211CHANNEL:
		if (l) {
			error = kauth_authorize_network(l->l_cred,
			    KAUTH_NETWORK_INTERFACE,
			    KAUTH_REQ_NETWORK_INTERFACE_SETPRIV, ifp,
			    (void *)cmd, NULL);
			if (error)
				return error;
		}
	}

	oif_flags = ifp->if_flags;
	switch (cmd) {

	case SIOCGIFFLAGS:
		ifr->ifr_flags = ifp->if_flags;
		break;

	case SIOCGIFMETRIC:
		ifr->ifr_metric = ifp->if_metric;
		break;

	case SIOCGIFMTU:
		ifr->ifr_mtu = ifp->if_mtu;
		break;

	case SIOCGIFDLT:
		ifr->ifr_dlt = ifp->if_dlt;
		break;

	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP && (ifr->ifr_flags & IFF_UP) == 0) {
			s = splnet();
			if_down(ifp);
			splx(s);
		}
		if (ifr->ifr_flags & IFF_UP && (ifp->if_flags & IFF_UP) == 0) {
			s = splnet();
			if_up(ifp);
			splx(s);
		}
		ifp->if_flags = (ifp->if_flags & IFF_CANTCHANGE) |
			(ifr->ifr_flags &~ IFF_CANTCHANGE);
		if (ifp->if_ioctl)
			(void) (*ifp->if_ioctl)(ifp, cmd, data);
		break;

	case SIOCGIFCAP:
		ifcr->ifcr_capabilities = ifp->if_capabilities;
		ifcr->ifcr_capenable = ifp->if_capenable;
		break;

	case SIOCSIFCAP:
		if ((ifcr->ifcr_capenable & ~ifp->if_capabilities) != 0)
			return (EINVAL);
		if (ifp->if_ioctl == NULL)
			return (EOPNOTSUPP);

		/* Must prevent race with packet reception here. */
		s = splnet();
		if (ifcr->ifcr_capenable != ifp->if_capenable) {
			struct ifreq ifrq;

			ifrq.ifr_flags = ifp->if_flags;
			ifp->if_capenable = ifcr->ifcr_capenable;

			/* Pre-compute the checksum flags mask. */
			ifp->if_csum_flags_tx = 0;
			ifp->if_csum_flags_rx = 0;
			if (ifp->if_capenable & IFCAP_CSUM_IPv4_Tx) {
				ifp->if_csum_flags_tx |= M_CSUM_IPv4;
			}
			if (ifp->if_capenable & IFCAP_CSUM_IPv4_Rx) {
				ifp->if_csum_flags_rx |= M_CSUM_IPv4;
			}

			if (ifp->if_capenable & IFCAP_CSUM_TCPv4_Tx) {
				ifp->if_csum_flags_tx |= M_CSUM_TCPv4;
			}
			if (ifp->if_capenable & IFCAP_CSUM_TCPv4_Rx) {
				ifp->if_csum_flags_rx |= M_CSUM_TCPv4;
			}

			if (ifp->if_capenable & IFCAP_CSUM_UDPv4_Tx) {
				ifp->if_csum_flags_tx |= M_CSUM_UDPv4;
			}
			if (ifp->if_capenable & IFCAP_CSUM_UDPv4_Rx) {
				ifp->if_csum_flags_rx |= M_CSUM_UDPv4;
			}

			if (ifp->if_capenable & IFCAP_CSUM_TCPv6_Tx) {
				ifp->if_csum_flags_tx |= M_CSUM_TCPv6;
			}
			if (ifp->if_capenable & IFCAP_CSUM_TCPv6_Rx) {
				ifp->if_csum_flags_rx |= M_CSUM_TCPv6;
			}

			if (ifp->if_capenable & IFCAP_CSUM_UDPv6_Tx) {
				ifp->if_csum_flags_tx |= M_CSUM_UDPv6;
			}
			if (ifp->if_capenable & IFCAP_CSUM_UDPv6_Rx) {
				ifp->if_csum_flags_rx |= M_CSUM_UDPv6;
			}

			/*
			 * Only kick the interface if it's up.  If it's
			 * not up now, it will notice the cap enables
			 * when it is brought up later.
			 */
			if (ifp->if_flags & IFF_UP)
				(void) (*ifp->if_ioctl)(ifp, SIOCSIFFLAGS,
				    (void *) &ifrq);
		}
		splx(s);
		break;

	case SIOCSIFMETRIC:
		ifp->if_metric = ifr->ifr_metric;
		break;

	case SIOCGIFDATA:
		ifdr->ifdr_data = ifp->if_data;
		break;

	case SIOCZIFDATA:
		ifdr->ifdr_data = ifp->if_data;
		/*
		 * Assumes that the volatile counters that can be
		 * zero'ed are at the end of if_data.
		 */
		memset(&ifp->if_data.ifi_ipackets, 0, sizeof(ifp->if_data) -
		    offsetof(struct if_data, ifi_ipackets));
		break;

	case SIOCSIFMTU:
	{
		u_long oldmtu = ifp->if_mtu;

		if (ifp->if_ioctl == NULL)
			return (EOPNOTSUPP);
		error = (*ifp->if_ioctl)(ifp, cmd, data);

		/*
		 * If the link MTU changed, do network layer specific procedure.
		 */
		if (ifp->if_mtu != oldmtu) {
#ifdef INET6
			nd6_setmtu(ifp);
#endif
		}
		break;
	}
	case SIOCSIFPHYADDR:
	case SIOCDIFPHYADDR:
#ifdef INET6
	case SIOCSIFPHYADDR_IN6:
#endif
	case SIOCSLIFPHYADDR:
	case SIOCADDMULTI:
	case SIOCDELMULTI:
	case SIOCSIFMEDIA:
	case SIOCGIFPSRCADDR:
	case SIOCGIFPDSTADDR:
	case SIOCGLIFPHYADDR:
	case SIOCGIFMEDIA:
		if (ifp->if_ioctl == 0)
			return (EOPNOTSUPP);
		error = (*ifp->if_ioctl)(ifp, cmd, data);
		break;

	case SIOCSDRVSPEC:
	case SIOCS80211NWID:
	case SIOCS80211NWKEY:
	case SIOCS80211POWER:
	case SIOCS80211BSSID:
	case SIOCS80211CHANNEL:
	default:
		if (so->so_proto == 0)
			return (EOPNOTSUPP);
#ifdef COMPAT_OSOCK
		error = compat_ifioctl(so, cmd, data, l);
#else
		error = ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL,
		    (struct mbuf *)cmd, (struct mbuf *)data,
		    (struct mbuf *)ifp, l));
#endif
		break;
	}

	if (((oif_flags ^ ifp->if_flags) & IFF_UP) != 0) {
#ifdef INET6
		if ((ifp->if_flags & IFF_UP) != 0) {
			s = splnet();
			in6_if_up(ifp);
			splx(s);
		}
#endif
	}

	return (error);
}

/*
 * Return interface configuration
 * of system.  List may be used
 * in later ioctl's (above) to get
 * other information.
 */
/*ARGSUSED*/
int
ifconf(u_long cmd, void *data)
{
	struct ifconf *ifc = (struct ifconf *)data;
	struct ifnet *ifp;
	struct ifaddr *ifa;
	struct ifreq ifr, *ifrp;
	int space = ifc->ifc_len, error = 0;
	const int sz = (int)sizeof(ifr);
	int sign;

	if ((ifrp = ifc->ifc_req) == NULL) {
		space = 0;
		sign = -1;
	} else {
		sign = 1;
	}
	IFNET_FOREACH(ifp) {
		(void)strncpy(ifr.ifr_name, ifp->if_xname,
		    sizeof(ifr.ifr_name));
		if (ifr.ifr_name[sizeof(ifr.ifr_name) - 1] != '\0')
			return ENAMETOOLONG;
		if (TAILQ_EMPTY(&ifp->if_addrlist)) {
			memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
			if (ifrp != NULL && space >= sz) {
				error = copyout(&ifr, ifrp, sz);
				if (error)
					break;
				ifrp++;
			}
			space -= sizeof(ifr) * sign;
			continue;
		}

		TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
			struct sockaddr *sa = ifa->ifa_addr;
#ifdef COMPAT_OSOCK
			if (cmd == OSIOCGIFCONF) {
				struct osockaddr *osa =
					 (struct osockaddr *)&ifr.ifr_addr;
				/*
				 * If it does not fit, we don't bother with it
				 */
				if (sa->sa_len > sizeof(*osa))
					continue;
				ifr.ifr_addr = *sa;
				osa->sa_family = sa->sa_family;
				if (ifrp != NULL && space >= sz) {
					error = copyout(&ifr, ifrp, sz);
					ifrp++;
				}
			} else
#endif
			if (sa->sa_len <= sizeof(*sa)) {
				ifr.ifr_addr = *sa;
				if (ifrp != NULL && space >= sz) {
					error = copyout(&ifr, ifrp, sz);
					ifrp++;
				}
			} else {
				space -= (sa->sa_len - sizeof(*sa)) * sign;
				if (ifrp != NULL && space >= sz) {
					error = copyout(&ifr, ifrp,
					    sizeof(ifr.ifr_name));
					if (error == 0) {
						error = copyout(sa,
						    &ifrp->ifr_addr,
						    sa->sa_len);
					}
					ifrp = (struct ifreq *)
						(sa->sa_len +
						 (char *)&ifrp->ifr_addr);
				}
			}
			if (error)
				break;
			space -= sz * sign;
		}
	}
	if (ifrp != NULL)
		ifc->ifc_len -= space;
	else
		ifc->ifc_len = space;
	return (error);
}

/*
 * Queue message on interface, and start output if interface
 * not yet active.
 */
int
ifq_enqueue(struct ifnet *ifp, struct mbuf *m
    ALTQ_COMMA ALTQ_DECL(struct altq_pktattr *pktattr))
{
	int len = m->m_pkthdr.len;
	int mflags = m->m_flags;
	int s = splnet();
	int error;

	IFQ_ENQUEUE(&ifp->if_snd, m, pktattr, error);
	if (error) {
		splx(s);
		return error;
	}
	ifp->if_obytes += len;
	if (mflags & M_MCAST)
		ifp->if_omcasts++;
	if ((ifp->if_flags & IFF_OACTIVE) == 0)
		(*ifp->if_start)(ifp);
	splx(s);
	return error;
}

/*
 * Queue message on interface, possibly using a second fast queue
 */
int
ifq_enqueue2(struct ifnet *ifp, struct ifqueue *ifq, struct mbuf *m
    ALTQ_COMMA ALTQ_DECL(struct altq_pktattr *pktattr))
{
	int error = 0;

	if (ifq != NULL
#ifdef ALTQ
	    && ALTQ_IS_ENABLED(&ifp->if_snd) == 0
#endif
	    ) {
		if (IF_QFULL(ifq)) {
			IF_DROP(&ifp->if_snd);
			m_freem(m);
			if (error == 0)
				error = ENOBUFS;
		}
		else
			IF_ENQUEUE(ifq, m);
	} else
		IFQ_ENQUEUE(&ifp->if_snd, m, pktattr, error);
	if (error != 0) {
		++ifp->if_oerrors;
		return error;
	}

	return 0;
}


#if defined(INET) || defined(INET6)
static void
sysctl_net_ifq_setup(struct sysctllog **clog,
		     int pf, const char *pfname,
		     int ipn, const char *ipname,
		     int qid, struct ifqueue *ifq)
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "net", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, pfname, NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, pf, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, ipname, NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, pf, ipn, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ifq",
		       SYSCTL_DESCR("Protocol input queue controls"),
		       NULL, 0, NULL, 0,
		       CTL_NET, pf, ipn, qid, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "len",
		       SYSCTL_DESCR("Current input queue length"),
		       NULL, 0, &ifq->ifq_len, 0,
		       CTL_NET, pf, ipn, qid, IFQCTL_LEN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "maxlen",
		       SYSCTL_DESCR("Maximum allowed input queue length"),
		       NULL, 0, &ifq->ifq_maxlen, 0,
		       CTL_NET, pf, ipn, qid, IFQCTL_MAXLEN, CTL_EOL);
#ifdef notyet
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "peak",
		       SYSCTL_DESCR("Highest input queue length"),
		       NULL, 0, &ifq->ifq_peak, 0,
		       CTL_NET, pf, ipn, qid, IFQCTL_PEAK, CTL_EOL);
#endif
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "drops",
		       SYSCTL_DESCR("Packets dropped due to full input queue"),
		       NULL, 0, &ifq->ifq_drops, 0,
		       CTL_NET, pf, ipn, qid, IFQCTL_DROPS, CTL_EOL);
}

#ifdef INET
SYSCTL_SETUP(sysctl_net_inet_ip_ifq_setup,
	     "sysctl net.inet.ip.ifq subtree setup")
{
	extern struct ifqueue ipintrq;

	sysctl_net_ifq_setup(clog, PF_INET, "inet", IPPROTO_IP, "ip",
			     IPCTL_IFQ, &ipintrq);
}
#endif /* INET */

#ifdef INET6
SYSCTL_SETUP(sysctl_net_inet6_ip6_ifq_setup,
	     "sysctl net.inet6.ip6.ifq subtree setup")
{
	extern struct ifqueue ip6intrq;

	sysctl_net_ifq_setup(clog, PF_INET6, "inet6", IPPROTO_IPV6, "ip6",
			     IPV6CTL_IFQ, &ip6intrq);
}
#endif /* INET6 */
#endif /* INET || INET6 */
