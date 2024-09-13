/*	$NetBSD: if_arp.c,v 1.311.2.3 2024/09/13 14:17:26 martin Exp $	*/

/*
 * Copyright (c) 1998, 2000, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Public Access Networks Corporation ("Panix").  It was developed under
 * contract to Panix by Eric Haszlakiewicz and Thor Lancelot Simon.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * Copyright (c) 1982, 1986, 1988, 1993
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
 *	@(#)if_ether.c	8.2 (Berkeley) 9/26/94
 */

/*
 * Ethernet address resolution protocol.
 * TODO:
 *	add "inuse/lock" bit (or ref. count) along with valid bit
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_arp.c,v 1.311.2.3 2024/09/13 14:17:26 martin Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "opt_inet.h"
#include "opt_net_mpsafe.h"
#endif

#ifdef INET

#include "arp.h"
#include "bridge.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/sysctl.h>
#include <sys/socketvar.h>
#include <sys/percpu.h>
#include <sys/cprng.h>
#include <sys/kmem.h>

#include <net/ethertypes.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_ether.h>
#include <net/if_llatbl.h>
#include <net/nd.h>
#include <net/route.h>
#include <net/net_stats.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>

#include "arcnet.h"
#if NARCNET > 0
#include <net/if_arc.h>
#endif
#include "carp.h"
#if NCARP > 0
#include <netinet/ip_carp.h>
#endif

/*
 * ARP trailer negotiation.  Trailer protocol is not IP specific,
 * but ARP request/response use IP addresses.
 */
#define ETHERTYPE_IPTRAILERS ETHERTYPE_TRAIL

/* timers */
static int arp_reachable = REACHABLE_TIME;
static int arp_retrans = RETRANS_TIMER;
static int arp_perform_nud = 1;

static bool arp_nud_enabled(struct ifnet *);
static unsigned int arp_llinfo_reachable(struct ifnet *);
static unsigned int arp_llinfo_retrans(struct ifnet *);
static union l3addr *arp_llinfo_holdsrc(struct llentry *, union l3addr *);
static void arp_llinfo_output(struct ifnet *, const union l3addr *,
    const union l3addr *, const uint8_t *, const union l3addr *);
static void arp_llinfo_missed(struct ifnet *, const union l3addr *,
    int16_t, struct mbuf *);
static void arp_free(struct llentry *, int);

static struct nd_domain arp_nd_domain = {
	.nd_family = AF_INET,
	.nd_delay = 5,		/* delay first probe time 5 second */
	.nd_mmaxtries = 3,	/* maximum broadcast query */
	.nd_umaxtries = 3,	/* maximum unicast query */
	.nd_retransmultiple = BACKOFF_MULTIPLE,
	.nd_maxretrans = MAX_RETRANS_TIMER,
	.nd_maxnudhint = 0,	/* max # of subsequent upper layer hints */
	.nd_maxqueuelen = 1,	/* max # of packets in unresolved ND entries */
	.nd_nud_enabled = arp_nud_enabled,
	.nd_reachable = arp_llinfo_reachable,
	.nd_retrans = arp_llinfo_retrans,
	.nd_holdsrc = arp_llinfo_holdsrc,
	.nd_output = arp_llinfo_output,
	.nd_missed = arp_llinfo_missed,
	.nd_free = arp_free,
};

int ip_dad_count = PROBE_NUM;
#ifdef ARP_DEBUG
int arp_debug = 1;
#else
int arp_debug = 0;
#endif

static void arp_init(void);
static void arp_dad_init(void);

static void arprequest(struct ifnet *,
    const struct in_addr *, const struct in_addr *,
    const uint8_t *, const uint8_t *);
static void arpannounce1(struct ifaddr *);
static struct sockaddr *arp_setgate(struct rtentry *, struct sockaddr *,
    const struct sockaddr *);
static struct llentry *arpcreate(struct ifnet *,
    const struct in_addr *, const struct sockaddr *, int);
static void in_arpinput(struct mbuf *);
static void in_revarpinput(struct mbuf *);
static void revarprequest(struct ifnet *);

static void arp_drainstub(void);

struct dadq;
static void arp_dad_timer(struct dadq *);
static void arp_dad_start(struct ifaddr *);
static void arp_dad_stop(struct ifaddr *);
static void arp_dad_duplicated(struct ifaddr *, const struct sockaddr_dl *);

#define	ARP_MAXQLEN	50
pktqueue_t *		arp_pktq		__read_mostly;

static int useloopback = 1;	/* use loopback interface for local traffic */

static percpu_t *arpstat_percpu;

#define	ARP_STAT_GETREF()	_NET_STAT_GETREF(arpstat_percpu)
#define	ARP_STAT_PUTREF()	_NET_STAT_PUTREF(arpstat_percpu)

#define	ARP_STATINC(x)		_NET_STATINC(arpstat_percpu, x)
#define	ARP_STATADD(x, v)	_NET_STATADD(arpstat_percpu, x, v)

/* revarp state */
static struct in_addr myip, srv_ip;
static int myip_initialized = 0;
static int revarp_in_progress = 0;
static struct ifnet *myip_ifp = NULL;

static int arp_drainwanted;

static int log_movements = 0;
static int log_permanent_modify = 1;
static int log_wrong_iface = 1;

DOMAIN_DEFINE(arpdomain);	/* forward declare and add to link set */

static void
arp_fasttimo(void)
{
	if (arp_drainwanted) {
		arp_drain();
		arp_drainwanted = 0;
	}
}

static const struct protosw arpsw[] = {
	{
		.pr_type = 0,
		.pr_domain = &arpdomain,
		.pr_protocol = 0,
		.pr_flags = 0,
		.pr_input = 0,
		.pr_ctlinput = 0,
		.pr_ctloutput = 0,
		.pr_usrreqs = 0,
		.pr_init = arp_init,
		.pr_fasttimo = arp_fasttimo,
		.pr_slowtimo = 0,
		.pr_drain = arp_drainstub,
	}
};

struct domain arpdomain = {
	.dom_family = PF_ARP,
	.dom_name = "arp",
	.dom_protosw = arpsw,
	.dom_protoswNPROTOSW = &arpsw[__arraycount(arpsw)],
#ifdef MBUFTRACE
	.dom_mowner = MOWNER_INIT("internet", "arp"),
#endif
};

static void sysctl_net_inet_arp_setup(struct sysctllog **);

void
arp_init(void)
{

	arp_pktq = pktq_create(ARP_MAXQLEN, arpintr, NULL);
	KASSERT(arp_pktq != NULL);

	sysctl_net_inet_arp_setup(NULL);
	arpstat_percpu = percpu_alloc(sizeof(uint64_t) * ARP_NSTATS);

#ifdef MBUFTRACE
	MOWNER_ATTACH(&arpdomain.dom_mowner);
#endif

	nd_attach_domain(&arp_nd_domain);
	arp_dad_init();
}

static void
arp_drainstub(void)
{
	arp_drainwanted = 1;
}

/*
 * ARP protocol drain routine.  Called when memory is in short supply.
 * Called at splvm();  don't acquire softnet_lock as can be called from
 * hardware interrupt handlers.
 */
void
arp_drain(void)
{

	lltable_drain(AF_INET);
}

/*
 * We set the gateway for RTF_CLONING routes to a "prototype"
 * link-layer sockaddr whose interface type (if_type) and interface
 * index (if_index) fields are prepared.
 */
static struct sockaddr *
arp_setgate(struct rtentry *rt, struct sockaddr *gate,
    const struct sockaddr *netmask)
{
	const struct ifnet *ifp = rt->rt_ifp;
	uint8_t namelen = strlen(ifp->if_xname);
	uint8_t addrlen = ifp->if_addrlen;

	/*
	 * XXX: If this is a manually added route to interface
	 * such as older version of routed or gated might provide,
	 * restore cloning bit.
	 */
	if ((rt->rt_flags & RTF_HOST) == 0 && netmask != NULL &&
	    satocsin(netmask)->sin_addr.s_addr != 0xffffffff)
		rt->rt_flags |= RTF_CONNECTED;

	if ((rt->rt_flags & (RTF_CONNECTED | RTF_LOCAL))) {
		union {
			struct sockaddr sa;
			struct sockaddr_storage ss;
			struct sockaddr_dl sdl;
		} u;
		/*
		 * Case 1: This route should come from a route to iface.
		 */
		sockaddr_dl_init(&u.sdl, sizeof(u.ss),
		    ifp->if_index, ifp->if_type, NULL, namelen, NULL, addrlen);
		rt_setgate(rt, &u.sa);
		gate = rt->rt_gateway;
	}
	return gate;
}

/*
 * Parallel to llc_rtrequest.
 */
void
arp_rtrequest(int req, struct rtentry *rt, const struct rt_addrinfo *info)
{
	struct sockaddr *gate = rt->rt_gateway;
	struct in_ifaddr *ia;
	struct ifaddr *ifa;
	struct ifnet *ifp = rt->rt_ifp;
	int bound;
	int s;

	if (req == RTM_LLINFO_UPD) {
		if ((ifa = info->rti_ifa) != NULL)
			arpannounce1(ifa);
		return;
	}

	if ((rt->rt_flags & RTF_GATEWAY) != 0) {
		if (req != RTM_ADD)
			return;

		/*
		 * linklayers with particular link MTU limitation.
		 */
		switch(ifp->if_type) {
#if NARCNET > 0
		case IFT_ARCNET:
		    {
			int arcipifmtu;

			if (ifp->if_flags & IFF_LINK0)
				arcipifmtu = arc_ipmtu;
			else
				arcipifmtu = ARCMTU;
			if (ifp->if_mtu > arcipifmtu)
				rt->rt_rmx.rmx_mtu = arcipifmtu;
			break;
		    }
#endif
		}
		return;
	}

	switch (req) {
	case RTM_SETGATE:
		gate = arp_setgate(rt, gate, info->rti_info[RTAX_NETMASK]);
		break;
	case RTM_ADD:
		gate = arp_setgate(rt, gate, info->rti_info[RTAX_NETMASK]);
		if (gate == NULL) {
			log(LOG_ERR, "%s: arp_setgate failed\n", __func__);
			break;
		}
		if ((rt->rt_flags & RTF_CONNECTED) ||
		    (rt->rt_flags & RTF_LOCAL)) {
			/*
			 * linklayers with particular link MTU limitation.
			 */
			switch (ifp->if_type) {
#if NARCNET > 0
			case IFT_ARCNET:
			    {
				int arcipifmtu;
				if (ifp->if_flags & IFF_LINK0)
					arcipifmtu = arc_ipmtu;
				else
					arcipifmtu = ARCMTU;

				if ((rt->rt_rmx.rmx_locks & RTV_MTU) == 0 &&
				    (rt->rt_rmx.rmx_mtu > arcipifmtu ||
				     (rt->rt_rmx.rmx_mtu == 0 &&
				      ifp->if_mtu > arcipifmtu)))
					rt->rt_rmx.rmx_mtu = arcipifmtu;
				break;
			    }
#endif
			}
			if (rt->rt_flags & RTF_CONNECTED)
				break;
		}

		bound = curlwp_bind();
		/* Announce a new entry if requested. */
		if (rt->rt_flags & RTF_ANNOUNCE) {
			struct psref psref;
			ia = in_get_ia_on_iface_psref(
			    satocsin(rt_getkey(rt))->sin_addr, ifp, &psref);
			if (ia != NULL) {
				arpannounce(ifp, &ia->ia_ifa,
				    CLLADDR(satocsdl(gate)));
				ia4_release(ia, &psref);
			}
		}

		if (gate->sa_family != AF_LINK ||
		    gate->sa_len < sockaddr_dl_measure(0, ifp->if_addrlen)) {
			log(LOG_DEBUG, "%s: bad gateway value\n", __func__);
			goto out;
		}

		satosdl(gate)->sdl_type = ifp->if_type;
		satosdl(gate)->sdl_index = ifp->if_index;

		/*
		 * If the route is for a broadcast address mark it as such.
		 * This way we can avoid an expensive call to in_broadcast()
		 * in ip_output() most of the time (because the route passed
		 * to ip_output() is almost always a host route).
		 */
		if (rt->rt_flags & RTF_HOST &&
		    !(rt->rt_flags & RTF_BROADCAST) &&
		    in_broadcast(satocsin(rt_getkey(rt))->sin_addr, rt->rt_ifp))
			rt->rt_flags |= RTF_BROADCAST;
		/* There is little point in resolving the broadcast address */
		if (rt->rt_flags & RTF_BROADCAST)
			goto out;

		/*
		 * When called from rt_ifa_addlocal, we cannot depend on that
		 * the address (rt_getkey(rt)) exits in the address list of the
		 * interface. So check RTF_LOCAL instead.
		 */
		if (rt->rt_flags & RTF_LOCAL) {
			if (useloopback) {
				rt->rt_ifp = lo0ifp;
				rt->rt_rmx.rmx_mtu = 0;
			}
			goto out;
		}

		s = pserialize_read_enter();
		ia = in_get_ia_on_iface(satocsin(rt_getkey(rt))->sin_addr, ifp);
		if (ia == NULL) {
			pserialize_read_exit(s);
			goto out;
		}

		if (useloopback) {
			rt->rt_ifp = lo0ifp;
			rt->rt_rmx.rmx_mtu = 0;
		}
		rt->rt_flags |= RTF_LOCAL;

		if (ISSET(info->rti_flags, RTF_DONTCHANGEIFA)) {
			pserialize_read_exit(s);
			goto out;
		}
		/*
		 * make sure to set rt->rt_ifa to the interface
		 * address we are using, otherwise we will have trouble
		 * with source address selection.
		 */
		ifa = &ia->ia_ifa;
		if (ifa != rt->rt_ifa)
			/* Assume it doesn't sleep */
			rt_replace_ifa(rt, ifa);
		pserialize_read_exit(s);
	out:
		curlwp_bindx(bound);
		break;
	}
}

/*
 * Broadcast an ARP request. Caller specifies:
 *	- arp header source ip address
 *	- arp header target ip address
 *	- arp header source ethernet address
 */
static void
arprequest(struct ifnet *ifp,
    const struct in_addr *sip, const struct in_addr *tip,
    const uint8_t *saddr, const uint8_t *taddr)
{
	struct mbuf *m;
	struct arphdr *ah;
	struct sockaddr sa;
	uint64_t *arps;

	KASSERT(sip != NULL);
	KASSERT(tip != NULL);
	KASSERT(saddr != NULL);

	if ((m = m_gethdr(M_DONTWAIT, MT_DATA)) == NULL)
		return;
	MCLAIM(m, &arpdomain.dom_mowner);
	switch (ifp->if_type) {
	case IFT_IEEE1394:
		m->m_len = sizeof(*ah) + 2 * sizeof(struct in_addr) +
		    ifp->if_addrlen;
		break;
	default:
		m->m_len = sizeof(*ah) + 2 * sizeof(struct in_addr) +
		    2 * ifp->if_addrlen;
		break;
	}
	m->m_pkthdr.len = m->m_len;
	m_align(m, m->m_len);
	ah = mtod(m, struct arphdr *);
	memset(ah, 0, m->m_len);
	switch (ifp->if_type) {
	case IFT_IEEE1394:	/* RFC2734 */
		/* fill it now for ar_tpa computation */
		ah->ar_hrd = htons(ARPHRD_IEEE1394);
		break;
	default:
		/* ifp->if_output will fill ar_hrd */
		break;
	}
	ah->ar_pro = htons(ETHERTYPE_IP);
	ah->ar_hln = ifp->if_addrlen;		/* hardware address length */
	ah->ar_pln = sizeof(struct in_addr);	/* protocol address length */
	ah->ar_op = htons(ARPOP_REQUEST);
	memcpy(ar_sha(ah), saddr, ah->ar_hln);
	if (taddr == NULL)
		m->m_flags |= M_BCAST;
	else
		memcpy(ar_tha(ah), taddr, ah->ar_hln);
	memcpy(ar_spa(ah), sip, ah->ar_pln);
	memcpy(ar_tpa(ah), tip, ah->ar_pln);
	sa.sa_family = AF_ARP;
	sa.sa_len = 2;
	arps = ARP_STAT_GETREF();
	arps[ARP_STAT_SNDTOTAL]++;
	arps[ARP_STAT_SENDREQUEST]++;
	ARP_STAT_PUTREF();
	if_output_lock(ifp, ifp, m, &sa, NULL);
}

void
arpannounce(struct ifnet *ifp, struct ifaddr *ifa, const uint8_t *enaddr)
{
	struct in_ifaddr *ia = ifatoia(ifa);
	struct in_addr *ip = &IA_SIN(ifa)->sin_addr;

	if (ia->ia4_flags & (IN_IFF_NOTREADY | IN_IFF_DETACHED)) {
		ARPLOG(LOG_DEBUG, "%s not ready\n", ARPLOGADDR(ip));
		return;
	}
	arprequest(ifp, ip, ip, enaddr, NULL);
}

static void
arpannounce1(struct ifaddr *ifa)
{

	arpannounce(ifa->ifa_ifp, ifa, CLLADDR(ifa->ifa_ifp->if_sadl));
}

/*
 * Resolve an IP address into an ethernet address.  If success, desten is
 * filled in. If there is no entry in arptab, set one up and broadcast a
 * request for the IP address. Hold onto this mbuf and resend it once the
 * address is finally resolved.
 *
 * A return value of 0 indicates that desten has been filled in and the packet
 * should be sent normally; a return value of EWOULDBLOCK indicates that the
 * packet has been held pending resolution. Any other value indicates an
 * error.
 */
int
arpresolve(struct ifnet *ifp, const struct rtentry *rt, struct mbuf *m,
    const struct sockaddr *dst, void *desten, size_t destlen)
{
	struct llentry *la;
	const char *create_lookup;
	int error;

#if NCARP > 0
	if (rt != NULL && rt->rt_ifp->if_type == IFT_CARP)
		ifp = rt->rt_ifp;
#endif

	KASSERT(m != NULL);

	la = arplookup(ifp, NULL, dst, 0);
	if (la == NULL)
		goto notfound;

	if (la->la_flags & LLE_VALID && la->ln_state == ND_LLINFO_REACHABLE) {
		KASSERT(destlen >= ifp->if_addrlen);
		memcpy(desten, &la->ll_addr, ifp->if_addrlen);
		LLE_RUNLOCK(la);
		return 0;
	}

notfound:
	if (ifp->if_flags & IFF_NOARP) {
		if (la != NULL)
			LLE_RUNLOCK(la);
		error = ENOTSUP;
		goto bad;
	}

	if (la == NULL) {
		struct rtentry *_rt;

		create_lookup = "create";
		_rt = rtalloc1(dst, 0);
		IF_AFDATA_WLOCK(ifp);
		la = lla_create(LLTABLE(ifp), LLE_EXCLUSIVE, dst, _rt);
		IF_AFDATA_WUNLOCK(ifp);
		if (_rt != NULL)
			rt_unref(_rt);
		if (la == NULL)
			ARP_STATINC(ARP_STAT_ALLOCFAIL);
		else
			la->ln_state = ND_LLINFO_NOSTATE;
	} else if (LLE_TRY_UPGRADE(la) == 0) {
		create_lookup = "lookup";
		LLE_RUNLOCK(la);
		IF_AFDATA_RLOCK(ifp);
		la = lla_lookup(LLTABLE(ifp), LLE_EXCLUSIVE, dst);
		IF_AFDATA_RUNLOCK(ifp);
	}

	error = EINVAL;
	if (la == NULL) {
		log(LOG_DEBUG,
		    "%s: failed to %s llentry for %s on %s\n",
		    __func__, create_lookup, inet_ntoa(satocsin(dst)->sin_addr),
		    ifp->if_xname);
		goto bad;
	}

	error = nd_resolve(la, rt, m, desten, destlen);
	return error;

bad:
	m_freem(m);
	return error;
}

/*
 * Common length and type checks are done here,
 * then the protocol-specific routine is called.
 */
void
arpintr(void *arg __unused)
{
	struct mbuf *m;
	struct arphdr *ar;
	int s;
	int arplen;
	struct ifnet *rcvif;
	bool badhrd;

	SOFTNET_KERNEL_LOCK_UNLESS_NET_MPSAFE();
	while ((m = pktq_dequeue(arp_pktq)) != NULL) {
		if ((m->m_flags & M_PKTHDR) == 0)
			panic("arpintr");

		MCLAIM(m, &arpdomain.dom_mowner);
		ARP_STATINC(ARP_STAT_RCVTOTAL);

		if (__predict_false(m->m_len < sizeof(*ar))) {
			if ((m = m_pullup(m, sizeof(*ar))) == NULL)
				goto badlen;
		}
		ar = mtod(m, struct arphdr *);
		KASSERT(ACCESSIBLE_POINTER(ar, struct arphdr));

		rcvif = m_get_rcvif(m, &s);
		if (__predict_false(rcvif == NULL)) {
			ARP_STATINC(ARP_STAT_RCVNOINT);
			goto free;
		}

		/*
		 * We don't want non-IEEE1394 ARP packets on IEEE1394
		 * interfaces, and vice versa. Our life depends on that.
		 */
		if (ntohs(ar->ar_hrd) == ARPHRD_IEEE1394)
			badhrd = rcvif->if_type != IFT_IEEE1394;
		else
			badhrd = rcvif->if_type == IFT_IEEE1394;

		m_put_rcvif(rcvif, &s);

		if (badhrd) {
			ARP_STATINC(ARP_STAT_RCVBADPROTO);
			goto free;
		}

		arplen = sizeof(*ar) + 2 * ar->ar_hln + 2 * ar->ar_pln;
		if (__predict_false(m->m_len < arplen)) {
			if ((m = m_pullup(m, arplen)) == NULL)
				goto badlen;
			ar = mtod(m, struct arphdr *);
			KASSERT(ACCESSIBLE_POINTER(ar, struct arphdr));
		}

		switch (ntohs(ar->ar_pro)) {
		case ETHERTYPE_IP:
		case ETHERTYPE_IPTRAILERS:
			in_arpinput(m);
			continue;
		default:
			ARP_STATINC(ARP_STAT_RCVBADPROTO);
			goto free;
		}

badlen:
		ARP_STATINC(ARP_STAT_RCVBADLEN);
free:
		m_freem(m);
	}
	SOFTNET_KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
	return; /* XXX gcc */
}

/*
 * ARP for Internet protocols on 10 Mb/s Ethernet. Algorithm is that given in
 * RFC 826. In addition, a sanity check is performed on the sender protocol
 * address, to catch impersonators.
 *
 * We no longer handle negotiations for use of trailer protocol: formerly, ARP
 * replied for protocol type ETHERTYPE_TRAIL sent along with IP replies if we
 * wanted trailers sent to us, and also sent them in response to IP replies.
 * This allowed either end to announce the desire to receive trailer packets.
 *
 * We no longer reply to requests for ETHERTYPE_TRAIL protocol either, but
 * formerly didn't normally send requests.
 */
static void
in_arpinput(struct mbuf *m)
{
	struct arphdr *ah;
	struct ifnet *ifp, *rcvif = NULL;
	struct llentry *la = NULL;
	struct in_ifaddr *ia = NULL;
#if NBRIDGE > 0
	struct in_ifaddr *bridge_ia = NULL;
#endif
#if NCARP > 0
	uint32_t count = 0, index = 0;
#endif
	struct sockaddr sa;
	struct in_addr isaddr, itaddr, myaddr;
	int op, rt_cmd, new_state = 0;
	void *tha;
	uint64_t *arps;
	struct psref psref, psref_ia;
	int s;
	char ipbuf[INET_ADDRSTRLEN];
	bool find_source, do_dad;

	if (__predict_false(m_makewritable(&m, 0, m->m_pkthdr.len, M_DONTWAIT)))
		goto out;
	ah = mtod(m, struct arphdr *);
	op = ntohs(ah->ar_op);

	if (ah->ar_pln != sizeof(struct in_addr))
		goto out;

	ifp = if_get_bylla(ar_sha(ah), ah->ar_hln, &psref);
	if (ifp) {
		/* it's from me, ignore it. */
		if_put(ifp, &psref);
		ARP_STATINC(ARP_STAT_RCVLOCALSHA);
		goto out;
	}

	rcvif = ifp = m_get_rcvif_psref(m, &psref);
	if (__predict_false(rcvif == NULL))
		goto out;
	if (rcvif->if_flags & IFF_NOARP)
		goto out;

	memcpy(&isaddr, ar_spa(ah), sizeof(isaddr));
	memcpy(&itaddr, ar_tpa(ah), sizeof(itaddr));

	if (m->m_flags & (M_BCAST|M_MCAST))
		ARP_STATINC(ARP_STAT_RCVMCAST);

	/*
	 * Search for a matching interface address
	 * or any address on the interface to use
	 * as a dummy address in the rest of this function.
	 *
	 * First try and find the source address for early
	 * duplicate address detection.
	 */
	if (in_nullhost(isaddr)) {
		if (in_nullhost(itaddr)) /* very bogus ARP */
			goto out;
		find_source = false;
		myaddr = itaddr;
	} else {
		find_source = true;
		myaddr = isaddr;
	}
	s = pserialize_read_enter();
again:
	IN_ADDRHASH_READER_FOREACH(ia, myaddr.s_addr) {
		if (!in_hosteq(ia->ia_addr.sin_addr, myaddr))
			continue;
#if NCARP > 0
		if (ia->ia_ifp->if_type == IFT_CARP &&
		    ((ia->ia_ifp->if_flags & (IFF_UP|IFF_RUNNING)) ==
		    (IFF_UP|IFF_RUNNING))) {
			index++;
			/* XXX: ar_hln? */
			if (ia->ia_ifp == rcvif && (ah->ar_hln >= 6) &&
			    carp_iamatch(ia, ar_sha(ah),
			    &count, index)) {
				break;
			}
		} else
#endif
		if (ia->ia_ifp == rcvif)
			break;
#if NBRIDGE > 0
		/*
		 * If the interface we received the packet on
		 * is part of a bridge, check to see if we need
		 * to "bridge" the packet to ourselves at this
		 * layer.  Note we still prefer a perfect match,
		 * but allow this weaker match if necessary.
		 */
		if (rcvif->if_bridge != NULL &&
		    rcvif->if_bridge == ia->ia_ifp->if_bridge)
			bridge_ia = ia;
#endif
	}

#if NBRIDGE > 0
	if (ia == NULL && bridge_ia != NULL) {
		ia = bridge_ia;
		m_put_rcvif_psref(rcvif, &psref);
		rcvif = NULL;
		/* FIXME */
		ifp = bridge_ia->ia_ifp;
	}
#endif

	/* If we failed to find the source address then find
	 * the target address. */
	if (ia == NULL && find_source && !in_nullhost(itaddr)) {
		find_source = false;
		myaddr = itaddr;
		goto again;
	}

	if (ia != NULL)
		ia4_acquire(ia, &psref_ia);
	pserialize_read_exit(s);

	if (ah->ar_hln != ifp->if_addrlen) {
		ARP_STATINC(ARP_STAT_RCVBADLEN);
		log(LOG_WARNING,
		    "arp from %s: addr len: new %d, i/f %d (ignored)\n",
		    IN_PRINT(ipbuf, &isaddr), ah->ar_hln, ifp->if_addrlen);
		goto out;
	}

	/* Only do DaD if we have a matching address. */
	do_dad = (ia != NULL);

	if (ia == NULL) {
		ia = in_get_ia_on_iface_psref(isaddr, rcvif, &psref_ia);
		if (ia == NULL) {
			ia = in_get_ia_from_ifp_psref(ifp, &psref_ia);
			if (ia == NULL) {
				ARP_STATINC(ARP_STAT_RCVNOINT);
				goto out;
			}
		}
	}

	myaddr = ia->ia_addr.sin_addr;

	/* XXX checks for bridge case? */
	if (!memcmp(ar_sha(ah), ifp->if_broadcastaddr, ifp->if_addrlen)) {
		ARP_STATINC(ARP_STAT_RCVBCASTSHA);
		log(LOG_ERR,
		    "%s: arp: link address is broadcast for IP address %s!\n",
		    ifp->if_xname, IN_PRINT(ipbuf, &isaddr));
		goto out;
	}

	/*
	 * If the source IP address is zero, this is an RFC 5227 ARP probe
	 */
	if (in_nullhost(isaddr))
		ARP_STATINC(ARP_STAT_RCVZEROSPA);
	else if (in_hosteq(isaddr, myaddr)) {
		ARP_STATINC(ARP_STAT_RCVLOCALSPA);
		/* This is the original behavior prior to supporting IPv4 DAD */
		if (!ip_dad_enabled()) {
			char llabuf[LLA_ADDRSTRLEN];
			log(LOG_ERR,
			    "duplicate IP address %s sent from link address %s\n",
			    IN_PRINT(ipbuf, &isaddr),
			    lla_snprintf(llabuf, sizeof(llabuf), ar_sha(ah),
			                 ah->ar_hln));
			itaddr = myaddr;
			goto reply;
		}
	}

	if (in_nullhost(itaddr))
		ARP_STATINC(ARP_STAT_RCVZEROTPA);

	/*
	 * DAD check, RFC 5227.
	 * ARP sender hardware address must match the interface
	 * address of the interface sending the packet.
	 * Collision on sender address is always a duplicate.
	 * Collision on target address is only a duplicate
	 * IF the sender address is the null host (ie a DAD probe)
	 * AND the message was broadcast
	 * AND our address is either tentative or duplicated
	 * If it was unicast then it's a valid Unicast Poll from RFC 1122.
	 */
	if (ip_dad_enabled() && do_dad &&
	    (in_hosteq(isaddr, myaddr) ||
	    (in_nullhost(isaddr) && in_hosteq(itaddr, myaddr) &&
	     m->m_flags & M_BCAST &&
	     ia->ia4_flags & (IN_IFF_TENTATIVE | IN_IFF_DUPLICATED))))
	{
		struct m_tag *mtag;

		mtag = m_tag_find(m, PACKET_TAG_ETHERNET_SRC);
		if (mtag == NULL || (ah->ar_hln == ETHER_ADDR_LEN &&
		    memcmp(mtag + 1, ar_sha(ah), ah->ar_hln) == 0)) {
			struct sockaddr_dl sdl, *sdlp;

			sdlp = sockaddr_dl_init(&sdl, sizeof(sdl),
			    ifp->if_index, ifp->if_type,
			    NULL, 0, ar_sha(ah), ah->ar_hln);
			arp_dad_duplicated((struct ifaddr *)ia, sdlp);
			goto out;
		}
	}

	/*
	 * If the target IP address is zero, ignore the packet.
	 * This prevents the code below from trying to answer
	 * when we are using IP address zero (booting).
	 */
	if (in_nullhost(itaddr))
		goto out;

	if (in_nullhost(isaddr))
		goto reply;

	if (in_hosteq(itaddr, myaddr))
		la = arpcreate(ifp, &isaddr, NULL, 1);
	else
		la = arplookup(ifp, &isaddr, NULL, 1);
	if (la == NULL)
		goto reply;

	if ((la->la_flags & LLE_VALID) &&
	    memcmp(ar_sha(ah), &la->ll_addr, ifp->if_addrlen))
	{
		char llabuf[LLA_ADDRSTRLEN], *llastr;

		llastr = lla_snprintf(llabuf, sizeof(llabuf),
		    ar_sha(ah), ah->ar_hln);

		if (la->la_flags & LLE_STATIC) {
			ARP_STATINC(ARP_STAT_RCVOVERPERM);
			if (!log_permanent_modify)
				goto out;
			log(LOG_INFO,
			    "%s tried to overwrite permanent arp info"
			    " for %s\n", llastr, IN_PRINT(ipbuf, &isaddr));
			goto out;
		} else if (la->lle_tbl->llt_ifp != ifp) {
			/* XXX should not happen? */
			ARP_STATINC(ARP_STAT_RCVOVERINT);
			if (!log_wrong_iface)
				goto out;
			log(LOG_INFO,
			    "%s on %s tried to overwrite "
			    "arp info for %s on %s\n",
			    llastr,
			    ifp->if_xname, IN_PRINT(ipbuf, &isaddr),
			    la->lle_tbl->llt_ifp->if_xname);
				goto out;
		} else {
			ARP_STATINC(ARP_STAT_RCVOVER);
			if (log_movements)
				log(LOG_INFO, "arp info overwritten "
				    "for %s by %s\n",
				    IN_PRINT(ipbuf, &isaddr), llastr);
		}
		rt_cmd = RTM_CHANGE;
		new_state = ND_LLINFO_STALE;
	} else {
		if (op == ARPOP_REPLY && in_hosteq(itaddr, myaddr)) {
			/* This was a solicited ARP reply. */
			la->ln_byhint = 0;
			new_state = ND_LLINFO_REACHABLE;
		} else if (op == ARPOP_REQUEST &&
		           (la->ln_state == ND_LLINFO_NOSTATE ||
			    la->ln_state == ND_LLINFO_INCOMPLETE)) {
			/*
			 * If an ARP request comes but there is no entry
			 * and a new one has been created or an entry exists
			 * but incomplete, make it stale to allow to send
			 * packets to the requester without an ARP resolution.
			 */
			la->ln_byhint = 0;
			new_state = ND_LLINFO_STALE;
		}
		rt_cmd = la->la_flags & LLE_VALID ? 0 : RTM_ADD;
	}

	KASSERT(ifp->if_sadl->sdl_alen == ifp->if_addrlen);

	KASSERT(sizeof(la->ll_addr) >= ifp->if_addrlen);
	memcpy(&la->ll_addr, ar_sha(ah), ifp->if_addrlen);
	la->la_flags |= LLE_VALID;
	la->ln_asked = 0;
	if (new_state != 0) {
		la->ln_state = new_state;

		if (new_state != ND_LLINFO_REACHABLE ||
		    !(la->la_flags & LLE_STATIC))
		{
			int timer = ND_TIMER_GC;

			if (new_state == ND_LLINFO_REACHABLE)
				timer = ND_TIMER_REACHABLE;
			nd_set_timer(la, timer);
		}
	}

	if (rt_cmd != 0) {
		struct sockaddr_in sin;

		sockaddr_in_init(&sin, &la->r_l3addr.addr4, 0);
		rt_clonedmsg(rt_cmd, NULL, sintosa(&sin), ar_sha(ah), ifp);
	}

	if (la->la_hold != NULL) {
		int n = la->la_numheld;
		struct mbuf *m_hold, *m_hold_next;
		struct sockaddr_in sin;

		sockaddr_in_init(&sin, &la->r_l3addr.addr4, 0);

		m_hold = la->la_hold;
		la->la_hold = NULL;
		la->la_numheld = 0;
		/*
		 * We have to unlock here because if_output would call
		 * arpresolve
		 */
		LLE_WUNLOCK(la);
		ARP_STATADD(ARP_STAT_DFRSENT, n);
		ARP_STATADD(ARP_STAT_DFRTOTAL, n);
		for (; m_hold != NULL; m_hold = m_hold_next) {
			m_hold_next = m_hold->m_nextpkt;
			m_hold->m_nextpkt = NULL;
			if_output_lock(ifp, ifp, m_hold, sintosa(&sin), NULL);
		}
	} else
		LLE_WUNLOCK(la);
	la = NULL;

reply:
	if (la != NULL) {
		LLE_WUNLOCK(la);
		la = NULL;
	}
	if (op != ARPOP_REQUEST) {
		if (op == ARPOP_REPLY)
			ARP_STATINC(ARP_STAT_RCVREPLY);
		goto out;
	}
	ARP_STATINC(ARP_STAT_RCVREQUEST);
	if (in_hosteq(itaddr, myaddr)) {
		/* If our address is unusable, don't reply */
		if (ia->ia4_flags & (IN_IFF_NOTREADY | IN_IFF_DETACHED))
			goto out;
		/* I am the target */
		tha = ar_tha(ah);
		if (tha)
			memcpy(tha, ar_sha(ah), ah->ar_hln);
		memcpy(ar_sha(ah), CLLADDR(ifp->if_sadl), ah->ar_hln);
	} else {
		/* Proxy ARP */
		struct llentry *lle = NULL;
		struct sockaddr_in sin;

#if NCARP > 0
		if (ifp->if_type == IFT_CARP) {
			struct ifnet *_rcvif = m_get_rcvif(m, &s);
			int iftype = 0;
			if (__predict_true(_rcvif != NULL))
				iftype = _rcvif->if_type;
			m_put_rcvif(_rcvif, &s);
			if (iftype != IFT_CARP)
				goto out;
		}
#endif

		tha = ar_tha(ah);

		sockaddr_in_init(&sin, &itaddr, 0);

		IF_AFDATA_RLOCK(ifp);
		lle = lla_lookup(LLTABLE(ifp), 0, (struct sockaddr *)&sin);
		IF_AFDATA_RUNLOCK(ifp);

		if ((lle != NULL) && (lle->la_flags & LLE_PUB)) {
			if (tha)
				memcpy(tha, ar_sha(ah), ah->ar_hln);
			memcpy(ar_sha(ah), &lle->ll_addr, ah->ar_hln);
			LLE_RUNLOCK(lle);
		} else {
			if (lle != NULL)
				LLE_RUNLOCK(lle);
			goto out;
		}
	}
	ia4_release(ia, &psref_ia);

	/*
	 * XXX XXX: Here we're recycling the mbuf. But the mbuf could have
	 * other mbufs in its chain, and just overwriting m->m_pkthdr.len
	 * would be wrong in this case (the length becomes smaller than the
	 * real chain size).
	 *
	 * This can theoretically cause bugs in the lower layers (drivers,
	 * and L2encap), in some corner cases.
	 */
	memcpy(ar_tpa(ah), ar_spa(ah), ah->ar_pln);
	memcpy(ar_spa(ah), &itaddr, ah->ar_pln);
	ah->ar_op = htons(ARPOP_REPLY);
	ah->ar_pro = htons(ETHERTYPE_IP); /* let's be sure! */
	switch (ifp->if_type) {
	case IFT_IEEE1394:
		/* ieee1394 arp reply is broadcast */
		m->m_flags &= ~M_MCAST;
		m->m_flags |= M_BCAST;
		m->m_len = sizeof(*ah) + (2 * ah->ar_pln) + ah->ar_hln;
		break;
	default:
		m->m_flags &= ~(M_BCAST|M_MCAST); /* never reply by broadcast */
		m->m_len = sizeof(*ah) + (2 * ah->ar_pln) + (2 * ah->ar_hln);
		break;
	}
	m->m_pkthdr.len = m->m_len;
	sa.sa_family = AF_ARP;
	sa.sa_len = 2;
	arps = ARP_STAT_GETREF();
	arps[ARP_STAT_SNDTOTAL]++;
	arps[ARP_STAT_SNDREPLY]++;
	ARP_STAT_PUTREF();
	if_output_lock(ifp, ifp, m, &sa, NULL);
	if (rcvif != NULL)
		m_put_rcvif_psref(rcvif, &psref);
	return;

out:
	if (la != NULL)
		LLE_WUNLOCK(la);
	if (ia != NULL)
		ia4_release(ia, &psref_ia);
	if (rcvif != NULL)
		m_put_rcvif_psref(rcvif, &psref);
	m_freem(m);
}

/*
 * Lookup or a new address in arptab.
 */
struct llentry *
arplookup(struct ifnet *ifp, const struct in_addr *addr,
    const struct sockaddr *sa, int wlock)
{
	struct sockaddr_in sin;
	struct llentry *la;
	int flags = wlock ? LLE_EXCLUSIVE : 0;

	if (sa == NULL) {
		KASSERT(addr != NULL);
		sockaddr_in_init(&sin, addr, 0);
		sa = sintocsa(&sin);
	}

	IF_AFDATA_RLOCK(ifp);
	la = lla_lookup(LLTABLE(ifp), flags, sa);
	IF_AFDATA_RUNLOCK(ifp);

	return la;
}

static struct llentry *
arpcreate(struct ifnet *ifp, const struct in_addr *addr,
    const struct sockaddr *sa, int wlock)
{
	struct sockaddr_in sin;
	struct llentry *la;
	int flags = wlock ? LLE_EXCLUSIVE : 0;

	if (sa == NULL) {
		KASSERT(addr != NULL);
		sockaddr_in_init(&sin, addr, 0);
		sa = sintocsa(&sin);
	}

	la = arplookup(ifp, addr, sa, wlock);

	if (la == NULL) {
		struct rtentry *rt;

		rt = rtalloc1(sa, 0);
		IF_AFDATA_WLOCK(ifp);
		la = lla_create(LLTABLE(ifp), flags, sa, rt);
		IF_AFDATA_WUNLOCK(ifp);
		if (rt != NULL)
			rt_unref(rt);

		if (la != NULL)
			la->ln_state = ND_LLINFO_NOSTATE;
	}

	return la;
}

int
arpioctl(u_long cmd, void *data)
{

	return EOPNOTSUPP;
}

void
arp_ifinit(struct ifnet *ifp, struct ifaddr *ifa)
{
	struct in_ifaddr *ia = (struct in_ifaddr *)ifa;

	ifa->ifa_rtrequest = arp_rtrequest;
	ifa->ifa_flags |= RTF_CONNECTED;

	/* ARP will handle DAD for this address. */
	if (in_nullhost(IA_SIN(ifa)->sin_addr)) {
		if (ia->ia_dad_stop != NULL)	/* safety */
			ia->ia_dad_stop(ifa);
		ia->ia_dad_start = NULL;
		ia->ia_dad_stop = NULL;
		ia->ia4_flags &= ~IN_IFF_TENTATIVE;
	} else {
		ia->ia_dad_start = arp_dad_start;
		ia->ia_dad_stop = arp_dad_stop;
		if (ia->ia4_flags & IN_IFF_TRYTENTATIVE && ip_dad_enabled())
			ia->ia4_flags |= IN_IFF_TENTATIVE;
		else
			arpannounce1(ifa);
	}
}

static bool
arp_nud_enabled(__unused struct ifnet *ifp)
{

	return arp_perform_nud != 0;
}

static unsigned int
arp_llinfo_reachable(__unused struct ifnet *ifp)
{

	return arp_reachable;
}

static unsigned int
arp_llinfo_retrans(__unused struct ifnet *ifp)
{

	return arp_retrans;
}

/*
 * Gets source address of the first packet in hold queue
 * and stores it in @src.
 * Returns pointer to @src (if hold queue is not empty) or NULL.
 */
static union l3addr *
arp_llinfo_holdsrc(struct llentry *ln, union l3addr *src)
{
	struct ip *ip;

	if (ln == NULL || ln->ln_hold == NULL)
		return NULL;

	/*
	 * assuming every packet in ln_hold has the same IP header
	 */
	ip = mtod(ln->ln_hold, struct ip *);
	/* XXX pullup? */
	if (sizeof(*ip) < ln->ln_hold->m_len)
		src->addr4 = ip->ip_src;
	else
		src = NULL;

	return src;
}

static void
arp_llinfo_output(struct ifnet *ifp, __unused const union l3addr *daddr,
    const union l3addr *taddr, const uint8_t *tlladdr,
    const union l3addr *hsrc)
{
	struct in_addr tip = taddr->addr4, sip = zeroin_addr;
	const uint8_t *slladdr = CLLADDR(ifp->if_sadl);

	if (hsrc != NULL) {
		struct in_ifaddr *ia;
		struct psref psref;

		ia = in_get_ia_on_iface_psref(hsrc->addr4, ifp, &psref);
		if (ia != NULL) {
			sip = hsrc->addr4;
			ia4_release(ia, &psref);
		}
	}

	if (sip.s_addr == INADDR_ANY) {
		struct sockaddr_in dst;
		struct rtentry *rt;

		sockaddr_in_init(&dst, &tip, 0);
		rt = rtalloc1(sintosa(&dst), 0);
		if (rt != NULL) {
			if (rt->rt_ifp == ifp &&
			    rt->rt_ifa != NULL &&
			    rt->rt_ifa->ifa_addr->sa_family == AF_INET)
				sip = satosin(rt->rt_ifa->ifa_addr)->sin_addr;
			rt_unref(rt);
		}
		if (sip.s_addr == INADDR_ANY) {
			char ipbuf[INET_ADDRSTRLEN];

			log(LOG_DEBUG, "%s: source can't be "
			    "determined: dst=%s\n", __func__,
			    IN_PRINT(ipbuf, &tip));
			return;
		}
	}

	arprequest(ifp, &sip, &tip, slladdr, tlladdr);
}


static void
arp_llinfo_missed(struct ifnet *ifp, const union l3addr *taddr,
    __unused int16_t type, struct mbuf *m)
{
	struct in_addr mdaddr = zeroin_addr;
	struct sockaddr_in dsin, tsin;
	struct sockaddr *sa;

	if (m != NULL) {
		struct ip *ip = mtod(m, struct ip *);

		if (sizeof(*ip) < m->m_len)
			mdaddr = ip->ip_src;

		/* ip_input() will send ICMP_UNREACH_HOST, not us. */
		m_freem(m);
	}

	if (mdaddr.s_addr != INADDR_ANY) {
		sockaddr_in_init(&dsin, &mdaddr, 0);
		sa = sintosa(&dsin);
	} else
		sa = NULL;

	sockaddr_in_init(&tsin, &taddr->addr4, 0);
	rt_clonedmsg(RTM_MISS, sa, sintosa(&tsin), NULL, ifp);
}

static void
arp_free(struct llentry *ln, int gc)
{
	struct ifnet *ifp;

	KASSERT(ln != NULL);
	LLE_WLOCK_ASSERT(ln);

	ifp = ln->lle_tbl->llt_ifp;

	if (ln->la_flags & LLE_VALID || gc) {
		struct sockaddr_in sin;
		const char *lladdr;

		sockaddr_in_init(&sin, &ln->r_l3addr.addr4, 0);
		lladdr = ln->la_flags & LLE_VALID ?
		    (const char *)&ln->ll_addr : NULL;
		rt_clonedmsg(RTM_DELETE, NULL, sintosa(&sin), lladdr, ifp);
	}

	/*
	 * Save to unlock. We still hold an extra reference and will not
	 * free(9) in llentry_free() if someone else holds one as well.
	 */
	LLE_WUNLOCK(ln);
	IF_AFDATA_LOCK(ifp);
	LLE_WLOCK(ln);

	lltable_free_entry(LLTABLE(ifp), ln);

	IF_AFDATA_UNLOCK(ifp);
}

/*
 * Upper-layer reachability hint for Neighbor Unreachability Detection.
 *
 * XXX cost-effective methods?
 */
void
arp_nud_hint(struct rtentry *rt)
{
	struct llentry *ln;
	struct ifnet *ifp;

	if (rt == NULL)
		return;

	ifp = rt->rt_ifp;
	ln = arplookup(ifp, NULL, rt_getkey(rt), 1);
	nd_nud_hint(ln);
}

TAILQ_HEAD(dadq_head, dadq);
struct dadq {
	TAILQ_ENTRY(dadq) dad_list;
	struct ifaddr *dad_ifa;
	int dad_count;		/* max ARP to send */
	int dad_arp_tcount;	/* # of trials to send ARP */
	int dad_arp_ocount;	/* ARP sent so far */
	int dad_arp_announce;	/* max ARP announcements */
	int dad_arp_acount;	/* # of announcements */
	struct callout dad_timer_ch;
};

static struct dadq_head dadq;
static int dad_maxtry = 15;     /* max # of *tries* to transmit DAD packet */
static kmutex_t arp_dad_lock;

static void
arp_dad_init(void)
{

	TAILQ_INIT(&dadq);
	mutex_init(&arp_dad_lock, MUTEX_DEFAULT, IPL_NONE);
}

static struct dadq *
arp_dad_find(struct ifaddr *ifa)
{
	struct dadq *dp;

	KASSERT(mutex_owned(&arp_dad_lock));

	TAILQ_FOREACH(dp, &dadq, dad_list) {
		if (dp->dad_ifa == ifa)
			return dp;
	}
	return NULL;
}

static void
arp_dad_starttimer(struct dadq *dp, int ticks)
{

	callout_reset(&dp->dad_timer_ch, ticks,
	    (void (*)(void *))arp_dad_timer, dp);
}

static void
arp_dad_stoptimer(struct dadq *dp)
{

	KASSERT(mutex_owned(&arp_dad_lock));

	TAILQ_REMOVE(&dadq, dp, dad_list);
	/* Tell the timer that dp is being destroyed. */
	dp->dad_ifa = NULL;
	callout_halt(&dp->dad_timer_ch, &arp_dad_lock);
}

static void
arp_dad_destroytimer(struct dadq *dp)
{

	callout_destroy(&dp->dad_timer_ch);
	KASSERT(dp->dad_ifa == NULL);
	kmem_intr_free(dp, sizeof(*dp));
}

static void
arp_dad_output(struct dadq *dp, struct ifaddr *ifa)
{
	struct in_ifaddr *ia = (struct in_ifaddr *)ifa;
	struct ifnet *ifp = ifa->ifa_ifp;
	struct in_addr sip;

	dp->dad_arp_tcount++;
	if ((ifp->if_flags & IFF_UP) == 0)
		return;
	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;

	dp->dad_arp_tcount = 0;
	dp->dad_arp_ocount++;

	memset(&sip, 0, sizeof(sip));
	arprequest(ifa->ifa_ifp, &sip, &ia->ia_addr.sin_addr,
	    CLLADDR(ifa->ifa_ifp->if_sadl), NULL);
}

/*
 * Start Duplicate Address Detection (DAD) for specified interface address.
 */
static void
arp_dad_start(struct ifaddr *ifa)
{
	struct in_ifaddr *ia = (struct in_ifaddr *)ifa;
	struct dadq *dp;
	char ipbuf[INET_ADDRSTRLEN];

	/*
	 * If we don't need DAD, don't do it.
	 * - DAD is disabled
	 */
	if (!(ia->ia4_flags & IN_IFF_TENTATIVE)) {
		log(LOG_DEBUG,
		    "%s: called with non-tentative address %s(%s)\n", __func__,
		    IN_PRINT(ipbuf, &ia->ia_addr.sin_addr),
		    ifa->ifa_ifp ? if_name(ifa->ifa_ifp) : "???");
		return;
	}
	if (!ip_dad_enabled()) {
		ia->ia4_flags &= ~IN_IFF_TENTATIVE;
		rt_addrmsg(RTM_NEWADDR, ifa);
		arpannounce1(ifa);
		return;
	}
	KASSERT(ifa->ifa_ifp != NULL);
	if (!(ifa->ifa_ifp->if_flags & IFF_UP))
		return;

	dp = kmem_intr_alloc(sizeof(*dp), KM_NOSLEEP);

	mutex_enter(&arp_dad_lock);
	if (arp_dad_find(ifa) != NULL) {
		mutex_exit(&arp_dad_lock);
		/* DAD already in progress */
		if (dp != NULL)
			kmem_intr_free(dp, sizeof(*dp));
		return;
	}

	if (dp == NULL) {
		mutex_exit(&arp_dad_lock);
		log(LOG_ERR, "%s: memory allocation failed for %s(%s)\n",
		    __func__, IN_PRINT(ipbuf, &ia->ia_addr.sin_addr),
		    ifa->ifa_ifp ? if_name(ifa->ifa_ifp) : "???");
		return;
	}

	/*
	 * Send ARP packet for DAD, ip_dad_count times.
	 * Note that we must delay the first transmission.
	 */
	callout_init(&dp->dad_timer_ch, CALLOUT_MPSAFE);
	dp->dad_ifa = ifa;
	ifaref(ifa);	/* just for safety */
	dp->dad_count = ip_dad_count;
	dp->dad_arp_announce = 0; /* Will be set when starting to announce */
	dp->dad_arp_acount = dp->dad_arp_ocount = dp->dad_arp_tcount = 0;
	TAILQ_INSERT_TAIL(&dadq, (struct dadq *)dp, dad_list);

	ARPLOG(LOG_DEBUG, "%s: starting DAD for %s\n", if_name(ifa->ifa_ifp),
	    ARPLOGADDR(&ia->ia_addr.sin_addr));

	arp_dad_starttimer(dp, cprng_fast32() % (PROBE_WAIT * hz));

	mutex_exit(&arp_dad_lock);
}

/*
 * terminate DAD unconditionally.  used for address removals.
 */
static void
arp_dad_stop(struct ifaddr *ifa)
{
	struct dadq *dp;

	mutex_enter(&arp_dad_lock);
	dp = arp_dad_find(ifa);
	if (dp == NULL) {
		mutex_exit(&arp_dad_lock);
		/* DAD wasn't started yet */
		return;
	}

	arp_dad_stoptimer(dp);

	mutex_exit(&arp_dad_lock);

	arp_dad_destroytimer(dp);
	ifafree(ifa);
}

static void
arp_dad_timer(struct dadq *dp)
{
	struct ifaddr *ifa;
	struct in_ifaddr *ia;
	char ipbuf[INET_ADDRSTRLEN];
	bool need_free = false;

	KERNEL_LOCK_UNLESS_NET_MPSAFE();
	mutex_enter(&arp_dad_lock);

	ifa = dp->dad_ifa;
	if (ifa == NULL) {
		/* dp is being destroyed by someone.  Do nothing. */
		goto done;
	}

	ia = (struct in_ifaddr *)ifa;
	if (ia->ia4_flags & IN_IFF_DUPLICATED) {
		log(LOG_ERR, "%s: called with duplicate address %s(%s)\n",
		    __func__, IN_PRINT(ipbuf, &ia->ia_addr.sin_addr),
		    ifa->ifa_ifp ? if_name(ifa->ifa_ifp) : "???");
		goto done;
	}
	if ((ia->ia4_flags & IN_IFF_TENTATIVE) == 0 && dp->dad_arp_acount == 0)
	{
		log(LOG_ERR, "%s: called with non-tentative address %s(%s)\n",
		    __func__, IN_PRINT(ipbuf, &ia->ia_addr.sin_addr),
		    ifa->ifa_ifp ? if_name(ifa->ifa_ifp) : "???");
		goto done;
	}

	/* timeouted with IFF_{RUNNING,UP} check */
	if (dp->dad_arp_tcount > dad_maxtry) {
		ARPLOG(LOG_INFO, "%s: could not run DAD, driver problem?\n",
		    if_name(ifa->ifa_ifp));

		arp_dad_stoptimer(dp);
		need_free = true;
		goto done;
	}

	/* Need more checks? */
	if (dp->dad_arp_ocount < dp->dad_count) {
		int adelay;

		/*
		 * We have more ARP to go.  Send ARP packet for DAD.
		 */
		arp_dad_output(dp, ifa);
		if (dp->dad_arp_ocount < dp->dad_count)
			adelay = (PROBE_MIN * hz) +
			    (cprng_fast32() %
			    ((PROBE_MAX * hz) - (PROBE_MIN * hz)));
		else
			adelay = ANNOUNCE_WAIT * hz;
		arp_dad_starttimer(dp, adelay);
		goto done;
	} else if (dp->dad_arp_acount == 0) {
		/*
		 * We are done with DAD.
		 * No duplicate address found.
		 */
		ia->ia4_flags &= ~IN_IFF_TENTATIVE;
		rt_addrmsg(RTM_NEWADDR, ifa);
		ARPLOG(LOG_DEBUG,
		    "%s: DAD complete for %s - no duplicates found\n",
		    if_name(ifa->ifa_ifp), ARPLOGADDR(&ia->ia_addr.sin_addr));
		dp->dad_arp_announce = ANNOUNCE_NUM;
		goto announce;
	} else if (dp->dad_arp_acount < dp->dad_arp_announce) {
announce:
		/*
		 * Announce the address.
		 */
		arpannounce1(ifa);
		dp->dad_arp_acount++;
		if (dp->dad_arp_acount < dp->dad_arp_announce) {
			arp_dad_starttimer(dp, ANNOUNCE_INTERVAL * hz);
			goto done;
		}
		ARPLOG(LOG_DEBUG,
		    "%s: ARP announcement complete for %s\n",
		    if_name(ifa->ifa_ifp), ARPLOGADDR(&ia->ia_addr.sin_addr));
	}

	arp_dad_stoptimer(dp);
	need_free = true;
done:
	mutex_exit(&arp_dad_lock);

	if (need_free) {
		arp_dad_destroytimer(dp);
		KASSERT(ifa != NULL);
		ifafree(ifa);
	}

	KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
}

static void
arp_dad_duplicated(struct ifaddr *ifa, const struct sockaddr_dl *from)
{
	struct in_ifaddr *ia = ifatoia(ifa);
	struct ifnet *ifp = ifa->ifa_ifp;
	char ipbuf[INET_ADDRSTRLEN], llabuf[LLA_ADDRSTRLEN];
	const char *iastr, *llastr;

	iastr = IN_PRINT(ipbuf, &ia->ia_addr.sin_addr);
	if (__predict_false(from == NULL))
		llastr = NULL;
	else
		llastr = lla_snprintf(llabuf, sizeof(llabuf),
		    CLLADDR(from), from->sdl_alen);

	if (ia->ia4_flags & (IN_IFF_TENTATIVE|IN_IFF_DUPLICATED)) {
		log(LOG_ERR,
		    "%s: DAD duplicate address %s from %s\n",
		    if_name(ifp), iastr, llastr);
	} else if (ia->ia_dad_defended == 0 ||
		   ia->ia_dad_defended < time_uptime - DEFEND_INTERVAL) {
		ia->ia_dad_defended = time_uptime;
		arpannounce1(ifa);
		log(LOG_ERR,
		    "%s: DAD defended address %s from %s\n",
		    if_name(ifp), iastr, llastr);
		return;
	} else {
		/* If DAD is disabled, just report the duplicate. */
		if (!ip_dad_enabled()) {
			log(LOG_ERR,
			    "%s: DAD ignoring duplicate address %s from %s\n",
			    if_name(ifp), iastr, llastr);
			return;
		}
		log(LOG_ERR,
		    "%s: DAD defence failed for %s from %s\n",
		    if_name(ifp), iastr, llastr);
	}

	arp_dad_stop(ifa);

	ia->ia4_flags &= ~IN_IFF_TENTATIVE;
	if ((ia->ia4_flags & IN_IFF_DUPLICATED) == 0) {
		ia->ia4_flags |= IN_IFF_DUPLICATED;
		/* Inform the routing socket of the duplicate address */
		rt_addrmsg_src(RTM_NEWADDR, ifa, (const struct sockaddr *)from);
	}
}

/*
 * Called from 10 Mb/s Ethernet interrupt handlers
 * when ether packet type ETHERTYPE_REVARP
 * is received.  Common length and type checks are done here,
 * then the protocol-specific routine is called.
 */
void
revarpinput(struct mbuf *m)
{
	struct arphdr *ar;
	int arplen;

	arplen = sizeof(struct arphdr);
	if (m->m_len < arplen && (m = m_pullup(m, arplen)) == NULL)
		return;
	ar = mtod(m, struct arphdr *);

	if (ntohs(ar->ar_hrd) == ARPHRD_IEEE1394) {
		goto out;
	}

	arplen = sizeof(struct arphdr) + 2 * (ar->ar_hln + ar->ar_pln);
	if (m->m_len < arplen && (m = m_pullup(m, arplen)) == NULL)
		return;
	ar = mtod(m, struct arphdr *);

	switch (ntohs(ar->ar_pro)) {
	case ETHERTYPE_IP:
	case ETHERTYPE_IPTRAILERS:
		in_revarpinput(m);
		return;

	default:
		break;
	}

out:
	m_freem(m);
}

/*
 * RARP for Internet protocols on 10 Mb/s Ethernet.
 * Algorithm is that given in RFC 903.
 * We are only using for bootstrap purposes to get an ip address for one of
 * our interfaces.  Thus we support no user-interface.
 *
 * Since the contents of the RARP reply are specific to the interface that
 * sent the request, this code must ensure that they are properly associated.
 *
 * Note: also supports ARP via RARP packets, per the RFC.
 */
void
in_revarpinput(struct mbuf *m)
{
	struct arphdr *ah;
	void *tha;
	int op;
	struct ifnet *rcvif;
	int s;

	ah = mtod(m, struct arphdr *);
	op = ntohs(ah->ar_op);

	rcvif = m_get_rcvif(m, &s);
	if (__predict_false(rcvif == NULL))
		goto out;
	if (rcvif->if_flags & IFF_NOARP)
		goto out;

	switch (rcvif->if_type) {
	case IFT_IEEE1394:
		/* ARP without target hardware address is not supported */
		goto out;
	default:
		break;
	}

	switch (op) {
	case ARPOP_REQUEST:
	case ARPOP_REPLY:	/* per RFC */
		m_put_rcvif(rcvif, &s);
		in_arpinput(m);
		return;
	case ARPOP_REVREPLY:
		break;
	case ARPOP_REVREQUEST:	/* handled by rarpd(8) */
	default:
		goto out;
	}
	if (!revarp_in_progress)
		goto out;
	if (rcvif != myip_ifp) /* !same interface */
		goto out;
	if (myip_initialized)
		goto wake;
	tha = ar_tha(ah);
	if (tha == NULL)
		goto out;
	if (ah->ar_pln != sizeof(struct in_addr))
		goto out;
	if (ah->ar_hln != rcvif->if_sadl->sdl_alen)
		goto out;
	if (memcmp(tha, CLLADDR(rcvif->if_sadl), rcvif->if_sadl->sdl_alen))
		goto out;
	memcpy(&srv_ip, ar_spa(ah), sizeof(srv_ip));
	memcpy(&myip, ar_tpa(ah), sizeof(myip));
	myip_initialized = 1;
wake:	/* Do wakeup every time in case it was missed. */
	wakeup((void *)&myip);

out:
	m_put_rcvif(rcvif, &s);
	m_freem(m);
}

/*
 * Send a RARP request for the ip address of the specified interface.
 * The request should be RFC 903-compliant.
 */
static void
revarprequest(struct ifnet *ifp)
{
	struct sockaddr sa;
	struct mbuf *m;
	struct arphdr *ah;
	void *tha;

	if ((m = m_gethdr(M_DONTWAIT, MT_DATA)) == NULL)
		return;
	MCLAIM(m, &arpdomain.dom_mowner);
	m->m_len = sizeof(*ah) + 2*sizeof(struct in_addr) +
	    2*ifp->if_addrlen;
	m->m_pkthdr.len = m->m_len;
	m_align(m, m->m_len);
	ah = mtod(m, struct arphdr *);
	memset(ah, 0, m->m_len);
	ah->ar_pro = htons(ETHERTYPE_IP);
	ah->ar_hln = ifp->if_addrlen;		/* hardware address length */
	ah->ar_pln = sizeof(struct in_addr);	/* protocol address length */
	ah->ar_op = htons(ARPOP_REVREQUEST);

	memcpy(ar_sha(ah), CLLADDR(ifp->if_sadl), ah->ar_hln);
	tha = ar_tha(ah);
	if (tha == NULL) {
		m_free(m);
		return;
	}
	memcpy(tha, CLLADDR(ifp->if_sadl), ah->ar_hln);

	sa.sa_family = AF_ARP;
	sa.sa_len = 2;
	m->m_flags |= M_BCAST;

	if_output_lock(ifp, ifp, m, &sa, NULL);
}

/*
 * RARP for the ip address of the specified interface, but also
 * save the ip address of the server that sent the answer.
 * Timeout if no response is received.
 */
int
revarpwhoarewe(struct ifnet *ifp, struct in_addr *serv_in,
    struct in_addr *clnt_in)
{
	int result, count = 20;

	myip_initialized = 0;
	myip_ifp = ifp;

	revarp_in_progress = 1;
	while (count--) {
		revarprequest(ifp);
		result = tsleep((void *)&myip, PSOCK, "revarp", hz/2);
		if (result != EWOULDBLOCK)
			break;
	}
	revarp_in_progress = 0;

	if (!myip_initialized)
		return ENETUNREACH;

	memcpy(serv_in, &srv_ip, sizeof(*serv_in));
	memcpy(clnt_in, &myip, sizeof(*clnt_in));
	return 0;
}

void
arp_stat_add(int type, uint64_t count)
{
	ARP_STATADD(type, count);
}

static int
sysctl_net_inet_arp_stats(SYSCTLFN_ARGS)
{

	return NETSTAT_SYSCTL(arpstat_percpu, ARP_NSTATS);
}

static void
sysctl_net_inet_arp_setup(struct sysctllog **clog)
{
	const struct sysctlnode *node;

	sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT,
			CTLTYPE_NODE, "inet", NULL,
			NULL, 0, NULL, 0,
			CTL_NET, PF_INET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, &node,
			CTLFLAG_PERMANENT,
			CTLTYPE_NODE, "arp",
			SYSCTL_DESCR("Address Resolution Protocol"),
			NULL, 0, NULL, 0,
			CTL_NET, PF_INET, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "nd_delay",
		       SYSCTL_DESCR("First probe delay time"),
		       NULL, 0, &arp_nd_domain.nd_delay, 0,
		       CTL_NET, PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "nd_bmaxtries",
		       SYSCTL_DESCR("Number of broadcast discovery attempts"),
		       NULL, 0, &arp_nd_domain.nd_mmaxtries, 0,
		       CTL_NET, PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "nd_umaxtries",
		       SYSCTL_DESCR("Number of unicast discovery attempts"),
		       NULL, 0, &arp_nd_domain.nd_umaxtries, 0,
		       CTL_NET, PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "nd_reachable",
		       SYSCTL_DESCR("Reachable time"),
		       NULL, 0, &arp_reachable, 0,
		       CTL_NET, PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "nd_retrans",
		       SYSCTL_DESCR("Retransmission time"),
		       NULL, 0, &arp_retrans, 0,
		       CTL_NET, PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "nd_nud",
		       SYSCTL_DESCR("Perform neighbour unreachability detection"),
		       NULL, 0, &arp_perform_nud, 0,
		       CTL_NET, PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "nd_maxnudhint",
		       SYSCTL_DESCR("Maximum neighbor unreachable hint count"),
		       NULL, 0, &arp_nd_domain.nd_maxnudhint, 0,
		       CTL_NET, PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "maxqueuelen",
		       SYSCTL_DESCR("max packet queue len for a unresolved ARP"),
		       NULL, 1, &arp_nd_domain.nd_maxqueuelen, 0,
		       CTL_NET, PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT,
			CTLTYPE_STRUCT, "stats",
			SYSCTL_DESCR("ARP statistics"),
			sysctl_net_inet_arp_stats, 0, NULL, 0,
			CTL_NET,PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "log_movements",
			SYSCTL_DESCR("log ARP replies from MACs different than"
			    " the one in the cache"),
			NULL, 0, &log_movements, 0,
			CTL_NET,PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "log_permanent_modify",
			SYSCTL_DESCR("log ARP replies from MACs different than"
			    " the one in the permanent arp entry"),
			NULL, 0, &log_permanent_modify, 0,
			CTL_NET,PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "log_wrong_iface",
			SYSCTL_DESCR("log ARP packets arriving on the wrong"
			    " interface"),
			NULL, 0, &log_wrong_iface, 0,
			CTL_NET,PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "debug",
			SYSCTL_DESCR("Enable ARP DAD debug output"),
			NULL, 0, &arp_debug, 0,
			CTL_NET, PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);
}

#endif /* INET */
