/*	$NetBSD: if_arp.c,v 1.106 2005/06/20 02:49:18 atatat Exp $	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: if_arp.c,v 1.106 2005/06/20 02:49:18 atatat Exp $");

#include "opt_ddb.h"
#include "opt_inet.h"

#ifdef INET

#include "bridge.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/sysctl.h>

#include <net/ethertypes.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_token.h>
#include <net/if_types.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>

#include "arc.h"
#if NARC > 0
#include <net/if_arc.h>
#endif
#include "fddi.h"
#if NFDDI > 0
#include <net/if_fddi.h>
#endif
#include "token.h"

#define SIN(s) ((struct sockaddr_in *)s)
#define SDL(s) ((struct sockaddr_dl *)s)
#define SRP(s) ((struct sockaddr_inarp *)s)

/*
 * ARP trailer negotiation.  Trailer protocol is not IP specific,
 * but ARP request/response use IP addresses.
 */
#define ETHERTYPE_IPTRAILERS ETHERTYPE_TRAIL

/* timer values */
int	arpt_prune = (5*60*1);	/* walk list every 5 minutes */
int	arpt_keep = (20*60);	/* once resolved, good for 20 more minutes */
int	arpt_down = 20;		/* once declared down, don't send for 20 secs */
int	arpt_refresh = (5*60);	/* time left before refreshing */
#define	rt_expire rt_rmx.rmx_expire
#define	rt_pksent rt_rmx.rmx_pksent

static	void arprequest(struct ifnet *,
	    struct in_addr *, struct in_addr *, u_int8_t *);
static	void arptfree(struct llinfo_arp *);
static	void arptimer(void *);
static	struct llinfo_arp *arplookup(struct mbuf *, struct in_addr *,
					  int, int);
static	void in_arpinput(struct mbuf *);

LIST_HEAD(, llinfo_arp) llinfo_arp;
struct	ifqueue arpintrq = {0, 0, 0, 50};
int	arp_inuse, arp_allocated, arp_intimer;
int	arp_maxtries = 5;
int	useloopback = 1;	/* use loopback interface for local traffic */
int	arpinit_done = 0;

struct	arpstat arpstat;
struct	callout arptimer_ch;


/* revarp state */
static struct	in_addr myip, srv_ip;
static int	myip_initialized = 0;
static int	revarp_in_progress = 0;
static struct	ifnet *myip_ifp = NULL;

#ifdef DDB
static void db_print_sa(const struct sockaddr *);
static void db_print_ifa(struct ifaddr *);
static void db_print_llinfo(caddr_t);
static int db_show_radix_node(struct radix_node *, void *);
#endif

/*
 * this should be elsewhere.
 */

static char *
lla_snprintf(u_int8_t *, int);

static char *
lla_snprintf(u_int8_t *adrp, int len)
{
#define NUMBUFS 3
	static char buf[NUMBUFS][16*3];
	static int bnum = 0;

	int i;
	char *p;

	p = buf[bnum];

	*p++ = hexdigits[(*adrp)>>4];
	*p++ = hexdigits[(*adrp++)&0xf];

	for (i=1; i<len && i<16; i++) {
		*p++ = ':';
		*p++ = hexdigits[(*adrp)>>4];
		*p++ = hexdigits[(*adrp++)&0xf];
	}

	*p = 0;
	p = buf[bnum];
	bnum = (bnum + 1) % NUMBUFS;
	return p;
}

DOMAIN_DEFINE(arpdomain);	/* forward declare and add to link set */

const struct protosw arpsw[] = {
	{ 0, &arpdomain, 0, 0,
	  0, 0, 0, 0,
	  0,
	  0, 0, 0, arp_drain,
	}
};


struct domain arpdomain =
{ 	PF_ARP,  "arp", 0, 0, 0,
	arpsw, &arpsw[sizeof(arpsw)/sizeof(arpsw[0])]
};

/*
 * ARP table locking.
 *
 * to prevent lossage vs. the arp_drain routine (which may be called at
 * any time, including in a device driver context), we do two things:
 *
 * 1) manipulation of la->la_hold is done at splnet() (for all of
 * about two instructions).
 *
 * 2) manipulation of the arp table's linked list is done under the
 * protection of the ARP_LOCK; if arp_drain() or arptimer is called
 * while the arp table is locked, we punt and try again later.
 */

static int	arp_locked;
static __inline int arp_lock_try(int);
static __inline void arp_unlock(void);

static __inline int
arp_lock_try(int recurse)
{
	int s;

	/*
	 * Use splvm() -- we're blocking things that would cause
	 * mbuf allocation.
	 */
	s = splvm();
	if (!recurse && arp_locked) {
		splx(s);
		return (0);
	}
	arp_locked++;
	splx(s);
	return (1);
}

static __inline void
arp_unlock(void)
{
	int s;

	s = splvm();
	arp_locked--;
	splx(s);
}

#ifdef DIAGNOSTIC
#define	ARP_LOCK(recurse)						\
do {									\
	if (arp_lock_try(recurse) == 0) {				\
		printf("%s:%d: arp already locked\n", __FILE__, __LINE__); \
		panic("arp_lock");					\
	}								\
} while (/*CONSTCOND*/ 0)
#define	ARP_LOCK_CHECK()						\
do {									\
	if (arp_locked == 0) {						\
		printf("%s:%d: arp lock not held\n", __FILE__, __LINE__); \
		panic("arp lock check");				\
	}								\
} while (/*CONSTCOND*/ 0)
#else
#define	ARP_LOCK(x)		(void) arp_lock_try(x)
#define	ARP_LOCK_CHECK()	/* nothing */
#endif

#define	ARP_UNLOCK()		arp_unlock()

/*
 * ARP protocol drain routine.  Called when memory is in short supply.
 * Called at splvm();
 */

void
arp_drain(void)
{
	struct llinfo_arp *la, *nla;
	int count = 0;
	struct mbuf *mold;

	if (arp_lock_try(0) == 0) {
		printf("arp_drain: locked; punting\n");
		return;
	}

	for (la = LIST_FIRST(&llinfo_arp); la != 0; la = nla) {
		nla = LIST_NEXT(la, la_list);

		mold = la->la_hold;
		la->la_hold = 0;

		if (mold) {
			m_freem(mold);
			count++;
		}
	}
	ARP_UNLOCK();
	arpstat.as_dfrdropped += count;
}


/*
 * Timeout routine.  Age arp_tab entries periodically.
 */
/* ARGSUSED */
static void
arptimer(void *arg)
{
	int s;
	struct llinfo_arp *la, *nla;

	s = splsoftnet();

	if (arp_lock_try(0) == 0) {
		/* get it later.. */
		splx(s);
		return;
	}

	callout_reset(&arptimer_ch, arpt_prune * hz, arptimer, NULL);
	for (la = LIST_FIRST(&llinfo_arp); la != 0; la = nla) {
		struct rtentry *rt = la->la_rt;

		nla = LIST_NEXT(la, la_list);
		if (rt->rt_expire == 0)
			continue;
		if ((rt->rt_expire - time.tv_sec) < arpt_refresh &&
		    rt->rt_pksent > (time.tv_sec - arpt_keep)) {
			/*
			 * If the entry has been used during since last
			 * refresh, try to renew it before deleting.
			 */
			arprequest(rt->rt_ifp,
			    &SIN(rt->rt_ifa->ifa_addr)->sin_addr,
			    &SIN(rt_key(rt))->sin_addr,
			    LLADDR(rt->rt_ifp->if_sadl));
		} else if (rt->rt_expire <= time.tv_sec)
			arptfree(la); /* timer has expired; clear */
	}

	ARP_UNLOCK();

	splx(s);
}

/*
 * Parallel to llc_rtrequest.
 */
void
arp_rtrequest(int req, struct rtentry *rt, struct rt_addrinfo *info)
{
	struct sockaddr *gate = rt->rt_gateway;
	struct llinfo_arp *la = (struct llinfo_arp *)rt->rt_llinfo;
	static struct sockaddr_dl null_sdl = {sizeof(null_sdl), AF_LINK};
	size_t allocsize;
	struct mbuf *mold;
	int s;
	struct in_ifaddr *ia;
	struct ifaddr *ifa;

	if (!arpinit_done) {
		arpinit_done = 1;
		/*
		 * We generate expiration times from time.tv_sec
		 * so avoid accidently creating permanent routes.
		 */
		if (time.tv_sec == 0) {
			time.tv_sec++;
		}
		callout_init(&arptimer_ch);
		callout_reset(&arptimer_ch, hz, arptimer, NULL);
	}

	if ((rt->rt_flags & RTF_GATEWAY) != 0) {
		if (req != RTM_ADD)
			return;

		/*
		 * linklayers with particular link MTU limitation.
		 */
		switch(rt->rt_ifp->if_type) {
#if NFDDI > 0
		case IFT_FDDI:
			if (rt->rt_ifp->if_mtu > FDDIIPMTU)
				rt->rt_rmx.rmx_mtu = FDDIIPMTU;
			break;
#endif
#if NARC > 0
		case IFT_ARCNET:
		    {
			int arcipifmtu;

			if (rt->rt_ifp->if_flags & IFF_LINK0)
				arcipifmtu = arc_ipmtu;
			else
				arcipifmtu = ARCMTU;
			if (rt->rt_ifp->if_mtu > arcipifmtu)
				rt->rt_rmx.rmx_mtu = arcipifmtu;
			break;
		    }
#endif
		}
		return;
	}

	ARP_LOCK(1);		/* we may already be locked here. */

	switch (req) {

	case RTM_ADD:
		/*
		 * XXX: If this is a manually added route to interface
		 * such as older version of routed or gated might provide,
		 * restore cloning bit.
		 */
		if ((rt->rt_flags & RTF_HOST) == 0 &&
		    SIN(rt_mask(rt))->sin_addr.s_addr != 0xffffffff)
			rt->rt_flags |= RTF_CLONING;
		if (rt->rt_flags & RTF_CLONING) {
			/*
			 * Case 1: This route should come from a route to iface.
			 */
			rt_setgate(rt, rt_key(rt),
					(struct sockaddr *)&null_sdl);
			gate = rt->rt_gateway;
			SDL(gate)->sdl_type = rt->rt_ifp->if_type;
			SDL(gate)->sdl_index = rt->rt_ifp->if_index;
			/*
			 * Give this route an expiration time, even though
			 * it's a "permanent" route, so that routes cloned
			 * from it do not need their expiration time set.
			 */
			rt->rt_expire = time.tv_sec;
			/*
			 * linklayers with particular link MTU limitation.
			 */
			switch (rt->rt_ifp->if_type) {
#if NFDDI > 0
			case IFT_FDDI:
				if ((rt->rt_rmx.rmx_locks & RTV_MTU) == 0 &&
				    (rt->rt_rmx.rmx_mtu > FDDIIPMTU ||
				     (rt->rt_rmx.rmx_mtu == 0 &&
				      rt->rt_ifp->if_mtu > FDDIIPMTU)))
					rt->rt_rmx.rmx_mtu = FDDIIPMTU;
				break;
#endif
#if NARC > 0
			case IFT_ARCNET:
			    {
				int arcipifmtu;
				if (rt->rt_ifp->if_flags & IFF_LINK0)
					arcipifmtu = arc_ipmtu;
				else
					arcipifmtu = ARCMTU;

				if ((rt->rt_rmx.rmx_locks & RTV_MTU) == 0 &&
				    (rt->rt_rmx.rmx_mtu > arcipifmtu ||
				     (rt->rt_rmx.rmx_mtu == 0 &&
				      rt->rt_ifp->if_mtu > arcipifmtu)))
					rt->rt_rmx.rmx_mtu = arcipifmtu;
				break;
			    }
#endif
			}
			break;
		}
		/* Announce a new entry if requested. */
		if (rt->rt_flags & RTF_ANNOUNCE)
			arprequest(rt->rt_ifp,
			    &SIN(rt_key(rt))->sin_addr,
			    &SIN(rt_key(rt))->sin_addr,
			    (u_char *)LLADDR(SDL(gate)));
		/*FALLTHROUGH*/
	case RTM_RESOLVE:
		if (gate->sa_family != AF_LINK ||
		    gate->sa_len < sizeof(null_sdl)) {
			log(LOG_DEBUG, "arp_rtrequest: bad gateway value\n");
			break;
		}
		SDL(gate)->sdl_type = rt->rt_ifp->if_type;
		SDL(gate)->sdl_index = rt->rt_ifp->if_index;
		if (la != 0)
			break; /* This happens on a route change */
		/*
		 * Case 2:  This route may come from cloning, or a manual route
		 * add with a LL address.
		 */
		switch (SDL(gate)->sdl_type) {
#if NTOKEN > 0
		case IFT_ISO88025:
			allocsize = sizeof(*la) + sizeof(struct token_rif);
			break;
#endif /* NTOKEN > 0 */
		default:
			allocsize = sizeof(*la);
		}
		R_Malloc(la, struct llinfo_arp *, allocsize);
		rt->rt_llinfo = (caddr_t)la;
		if (la == 0) {
			log(LOG_DEBUG, "arp_rtrequest: malloc failed\n");
			break;
		}
		arp_inuse++, arp_allocated++;
		Bzero(la, allocsize);
		la->la_rt = rt;
		rt->rt_flags |= RTF_LLINFO;
		LIST_INSERT_HEAD(&llinfo_arp, la, la_list);

		INADDR_TO_IA(SIN(rt_key(rt))->sin_addr, ia);
		while (ia && ia->ia_ifp != rt->rt_ifp)
			NEXT_IA_WITH_SAME_ADDR(ia);
		if (ia) {
			/*
			 * This test used to be
			 *	if (lo0ifp->if_flags & IFF_UP)
			 * It allowed local traffic to be forced through
			 * the hardware by configuring the loopback down.
			 * However, it causes problems during network
			 * configuration for boards that can't receive
			 * packets they send.  It is now necessary to clear
			 * "useloopback" and remove the route to force
			 * traffic out to the hardware.
			 *
			 * In 4.4BSD, the above "if" statement checked
			 * rt->rt_ifa against rt_key(rt).  It was changed
			 * to the current form so that we can provide a
			 * better support for multiple IPv4 addresses on a
			 * interface.
			 */
			rt->rt_expire = 0;
			Bcopy(LLADDR(rt->rt_ifp->if_sadl),
			    LLADDR(SDL(gate)),
			    SDL(gate)->sdl_alen = rt->rt_ifp->if_addrlen);
			if (useloopback)
				rt->rt_ifp = lo0ifp;
			/*
			 * make sure to set rt->rt_ifa to the interface
			 * address we are using, otherwise we will have trouble
			 * with source address selection.
			 */
			ifa = &ia->ia_ifa;
			if (ifa != rt->rt_ifa) {
				IFAFREE(rt->rt_ifa);
				IFAREF(ifa);
				rt->rt_ifa = ifa;
			}
		}
		break;

	case RTM_DELETE:
		if (la == 0)
			break;
		arp_inuse--;
		LIST_REMOVE(la, la_list);
		rt->rt_llinfo = 0;
		rt->rt_flags &= ~RTF_LLINFO;

		s = splnet();
		mold = la->la_hold;
		la->la_hold = 0;
		splx(s);

		if (mold)
			m_freem(mold);

		Free((caddr_t)la);
	}
	ARP_UNLOCK();
}

/*
 * Broadcast an ARP request. Caller specifies:
 *	- arp header source ip address
 *	- arp header target ip address
 *	- arp header source ethernet address
 */
static void
arprequest(struct ifnet *ifp,
    struct in_addr *sip, struct in_addr *tip, u_int8_t *enaddr)
{
	struct mbuf *m;
	struct arphdr *ah;
	struct sockaddr sa;

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
	MH_ALIGN(m, m->m_len);
	ah = mtod(m, struct arphdr *);
	bzero((caddr_t)ah, m->m_len);
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
	bcopy((caddr_t)enaddr, (caddr_t)ar_sha(ah), ah->ar_hln);
	bcopy((caddr_t)sip, (caddr_t)ar_spa(ah), ah->ar_pln);
	bcopy((caddr_t)tip, (caddr_t)ar_tpa(ah), ah->ar_pln);
	sa.sa_family = AF_ARP;
	sa.sa_len = 2;
	m->m_flags |= M_BCAST;
	arpstat.as_sndtotal++;
	arpstat.as_sndrequest++;
	(*ifp->if_output)(ifp, m, &sa, (struct rtentry *)0);
}

/*
 * Resolve an IP address into an ethernet address.  If success,
 * desten is filled in.  If there is no entry in arptab,
 * set one up and broadcast a request for the IP address.
 * Hold onto this mbuf and resend it once the address
 * is finally resolved.  A return value of 1 indicates
 * that desten has been filled in and the packet should be sent
 * normally; a 0 return indicates that the packet has been
 * taken over here, either now or for later transmission.
 */
int
arpresolve(struct ifnet *ifp, struct rtentry *rt, struct mbuf *m,
    struct sockaddr *dst, u_char *desten)
{
	struct llinfo_arp *la;
	struct sockaddr_dl *sdl;
	struct mbuf *mold;
	int s;

	if (rt)
		la = (struct llinfo_arp *)rt->rt_llinfo;
	else {
		if ((la = arplookup(m, &SIN(dst)->sin_addr, 1, 0)) != NULL)
			rt = la->la_rt;
	}
	if (la == 0 || rt == 0) {
		arpstat.as_allocfail++;
		log(LOG_DEBUG,
		    "arpresolve: can't allocate llinfo on %s for %s\n",
		    ifp->if_xname, in_fmtaddr(SIN(dst)->sin_addr));
		m_freem(m);
		return (0);
	}
	sdl = SDL(rt->rt_gateway);
	/*
	 * Check the address family and length is valid, the address
	 * is resolved; otherwise, try to resolve.
	 */
	if ((rt->rt_expire == 0 || rt->rt_expire > time.tv_sec) &&
	    sdl->sdl_family == AF_LINK && sdl->sdl_alen != 0) {
		bcopy(LLADDR(sdl), desten,
		    min(sdl->sdl_alen, ifp->if_addrlen));
		rt->rt_pksent = time.tv_sec; /* Time for last pkt sent */
		return 1;
	}
	/*
	 * There is an arptab entry, but no ethernet address
	 * response yet.  Replace the held mbuf with this
	 * latest one.
	 */

	arpstat.as_dfrtotal++;
	s = splnet();
	mold = la->la_hold;
	la->la_hold = m;
	splx(s);

	if (mold) {
		arpstat.as_dfrdropped++;
		m_freem(mold);
	}

	/*
	 * Re-send the ARP request when appropriate.
	 */
#ifdef	DIAGNOSTIC
	if (rt->rt_expire == 0) {
		/* This should never happen. (Should it? -gwr) */
		printf("arpresolve: unresolved and rt_expire == 0\n");
		/* Set expiration time to now (expired). */
		rt->rt_expire = time.tv_sec;
	}
#endif
	if (rt->rt_expire) {
		rt->rt_flags &= ~RTF_REJECT;
		if (la->la_asked == 0 || rt->rt_expire != time.tv_sec) {
			rt->rt_expire = time.tv_sec;
			if (la->la_asked++ < arp_maxtries)
				arprequest(ifp,
				    &SIN(rt->rt_ifa->ifa_addr)->sin_addr,
				    &SIN(dst)->sin_addr,
				    LLADDR(ifp->if_sadl));
			else {
				rt->rt_flags |= RTF_REJECT;
				rt->rt_expire += arpt_down;
				la->la_asked = 0;
			}
		}
	}
	return (0);
}

/*
 * Common length and type checks are done here,
 * then the protocol-specific routine is called.
 */
void
arpintr(void)
{
	struct mbuf *m;
	struct arphdr *ar;
	int s;
	int arplen;

	while (arpintrq.ifq_head) {
		s = splnet();
		IF_DEQUEUE(&arpintrq, m);
		splx(s);
		if (m == 0 || (m->m_flags & M_PKTHDR) == 0)
			panic("arpintr");

		MCLAIM(m, &arpdomain.dom_mowner);
		arpstat.as_rcvtotal++;

		/*
		 * First, make sure we have at least struct arphdr.
		 */
		if (m->m_len < sizeof(struct arphdr) ||
		    (ar = mtod(m, struct arphdr *)) == NULL)
			goto badlen;

		switch (m->m_pkthdr.rcvif->if_type) {
		case IFT_IEEE1394:
			arplen = sizeof(struct arphdr) +
			    ar->ar_hln + 2 * ar->ar_pln;
			break;
		default:
			arplen = sizeof(struct arphdr) +
			    2 * ar->ar_hln + 2 * ar->ar_pln;
			break;
		}

		if (/* XXX ntohs(ar->ar_hrd) == ARPHRD_ETHER && */
		    m->m_len >= arplen)
			switch (ntohs(ar->ar_pro)) {
			case ETHERTYPE_IP:
			case ETHERTYPE_IPTRAILERS:
				in_arpinput(m);
				continue;
			default:
				arpstat.as_rcvbadproto++;
			}
		else {
badlen:
			arpstat.as_rcvbadlen++;
		}
		m_freem(m);
	}
}

/*
 * ARP for Internet protocols on 10 Mb/s Ethernet.
 * Algorithm is that given in RFC 826.
 * In addition, a sanity check is performed on the sender
 * protocol address, to catch impersonators.
 * We no longer handle negotiations for use of trailer protocol:
 * Formerly, ARP replied for protocol type ETHERTYPE_TRAIL sent
 * along with IP replies if we wanted trailers sent to us,
 * and also sent them in response to IP replies.
 * This allowed either end to announce the desire to receive
 * trailer packets.
 * We no longer reply to requests for ETHERTYPE_TRAIL protocol either,
 * but formerly didn't normally send requests.
 */
static void
in_arpinput(struct mbuf *m)
{
	struct arphdr *ah;
	struct ifnet *ifp = m->m_pkthdr.rcvif;
	struct llinfo_arp *la = 0;
	struct rtentry  *rt;
	struct in_ifaddr *ia;
#if NBRIDGE > 0
	struct in_ifaddr *bridge_ia = NULL;
#endif
	struct sockaddr_dl *sdl;
	struct sockaddr sa;
	struct in_addr isaddr, itaddr, myaddr;
	int op;
	struct mbuf *mold;
	int s;

	ah = mtod(m, struct arphdr *);
	op = ntohs(ah->ar_op);

	/*
	 * Fix up ah->ar_hrd if necessary, before using ar_tha() or
	 * ar_tpa().
	 */
	switch (ifp->if_type) {
	case IFT_IEEE1394:
		if (ntohs(ah->ar_hrd) == ARPHRD_IEEE1394)
			;
		else {
			/* XXX this is to make sure we compute ar_tha right */
			/* XXX check ar_hrd more strictly? */
			ah->ar_hrd = htons(ARPHRD_IEEE1394);
		}
		break;
	default:
		/* XXX check ar_hrd? */
		break;
	}

	bcopy((caddr_t)ar_spa(ah), (caddr_t)&isaddr, sizeof (isaddr));
	bcopy((caddr_t)ar_tpa(ah), (caddr_t)&itaddr, sizeof (itaddr));

	if (m->m_flags & (M_BCAST|M_MCAST))
		arpstat.as_rcvmcast++;

	/*
	 * If the target IP address is zero, ignore the packet.
	 * This prevents the code below from tring to answer
	 * when we are using IP address zero (booting).
	 */
	if (in_nullhost(itaddr)) {
		arpstat.as_rcvzerotpa++;
		goto out;
	}

	/*
	 * If the source IP address is zero, this is most likely a
	 * confused host trying to use IP address zero. (Windoze?)
	 * XXX: Should we bother trying to reply to these?
	 */
	if (in_nullhost(isaddr)) {
		arpstat.as_rcvzerospa++;
		goto out;
	}

	/*
	 * Search for a matching interface address
	 * or any address on the interface to use
	 * as a dummy address in the rest of this function
	 */
	INADDR_TO_IA(itaddr, ia);
	while (ia != NULL) {
		if (ia->ia_ifp == m->m_pkthdr.rcvif)
			break;

#if NBRIDGE > 0
		/*
		 * If the interface we received the packet on
		 * is part of a bridge, check to see if we need
		 * to "bridge" the packet to ourselves at this
		 * layer.  Note we still prefer a perfect match,
		 * but allow this weaker match if necessary.
		 */
		if (m->m_pkthdr.rcvif->if_bridge != NULL &&
		    m->m_pkthdr.rcvif->if_bridge == ia->ia_ifp->if_bridge)
			bridge_ia = ia;
#endif /* NBRIDGE > 0 */

		NEXT_IA_WITH_SAME_ADDR(ia);
	}

#if NBRIDGE > 0
	if (ia == NULL && bridge_ia != NULL) {
		ia = bridge_ia;
		ifp = bridge_ia->ia_ifp;
	}
#endif

	if (ia == NULL) {
		INADDR_TO_IA(isaddr, ia);
		while ((ia != NULL) && ia->ia_ifp != m->m_pkthdr.rcvif)
			NEXT_IA_WITH_SAME_ADDR(ia);

		if (ia == NULL) {
			IFP_TO_IA(ifp, ia);
			if (ia == NULL) {
				arpstat.as_rcvnoint++;
				goto out;
			}
		}
	}

	myaddr = ia->ia_addr.sin_addr;

	/* XXX checks for bridge case? */
	if (!bcmp((caddr_t)ar_sha(ah), LLADDR(ifp->if_sadl),
	    ifp->if_addrlen)) {
		arpstat.as_rcvlocalsha++;
		goto out;	/* it's from me, ignore it. */
	}

	/* XXX checks for bridge case? */
	if (!memcmp(ar_sha(ah), ifp->if_broadcastaddr, ifp->if_addrlen)) {
		arpstat.as_rcvbcastsha++;
		log(LOG_ERR,
		    "%s: arp: link address is broadcast for IP address %s!\n",
		    ifp->if_xname, in_fmtaddr(isaddr));
		goto out;
	}

	if (in_hosteq(isaddr, myaddr)) {
		arpstat.as_rcvlocalspa++;
		log(LOG_ERR,
		   "duplicate IP address %s sent from link address %s\n",
		   in_fmtaddr(isaddr), lla_snprintf(ar_sha(ah), ah->ar_hln));
		itaddr = myaddr;
		goto reply;
	}
	la = arplookup(m, &isaddr, in_hosteq(itaddr, myaddr), 0);
	if (la && (rt = la->la_rt) && (sdl = SDL(rt->rt_gateway))) {
		if (sdl->sdl_alen &&
		    bcmp((caddr_t)ar_sha(ah), LLADDR(sdl), sdl->sdl_alen)) {
			if (rt->rt_flags & RTF_STATIC) {
				arpstat.as_rcvoverperm++;
				log(LOG_INFO,
				    "%s tried to overwrite permanent arp info"
				    " for %s\n",
				    lla_snprintf(ar_sha(ah), ah->ar_hln),
				    in_fmtaddr(isaddr));
				goto out;
			} else if (rt->rt_ifp != ifp) {
				arpstat.as_rcvoverint++;
				log(LOG_INFO,
				    "%s on %s tried to overwrite "
				    "arp info for %s on %s\n",
				    lla_snprintf(ar_sha(ah), ah->ar_hln),
				    ifp->if_xname, in_fmtaddr(isaddr),
				    rt->rt_ifp->if_xname);
				    goto out;
			} else {
				arpstat.as_rcvover++;
				log(LOG_INFO,
				    "arp info overwritten for %s by %s\n",
				    in_fmtaddr(isaddr),
				    lla_snprintf(ar_sha(ah), ah->ar_hln));
			}
		}
		/*
		 * sanity check for the address length.
		 * XXX this does not work for protocols with variable address
		 * length. -is
		 */
		if (sdl->sdl_alen &&
		    sdl->sdl_alen != ah->ar_hln) {
			arpstat.as_rcvlenchg++;
			log(LOG_WARNING,
			    "arp from %s: new addr len %d, was %d",
			    in_fmtaddr(isaddr), ah->ar_hln, sdl->sdl_alen);
		}
		if (ifp->if_addrlen != ah->ar_hln) {
			arpstat.as_rcvbadlen++;
			log(LOG_WARNING,
			    "arp from %s: addr len: new %d, i/f %d (ignored)",
			    in_fmtaddr(isaddr), ah->ar_hln,
			    ifp->if_addrlen);
			goto reply;
		}
#if NTOKEN > 0
		/*
		 * XXX uses m_data and assumes the complete answer including
		 * XXX token-ring headers is in the same buf
		 */
		if (ifp->if_type == IFT_ISO88025) {
			struct token_header *trh;

			trh = (struct token_header *)M_TRHSTART(m);
			if (trh->token_shost[0] & TOKEN_RI_PRESENT) {
				struct token_rif	*rif;
				size_t	riflen;

				rif = TOKEN_RIF(trh);
				riflen = (ntohs(rif->tr_rcf) &
				    TOKEN_RCF_LEN_MASK) >> 8;

				if (riflen > 2 &&
				    riflen < sizeof(struct token_rif) &&
				    (riflen & 1) == 0) {
					rif->tr_rcf ^= htons(TOKEN_RCF_DIRECTION);
					rif->tr_rcf &= htons(~TOKEN_RCF_BROADCAST_MASK);
					bcopy(rif, TOKEN_RIF(la), riflen);
				}
			}
		}
#endif /* NTOKEN > 0 */
		bcopy((caddr_t)ar_sha(ah), LLADDR(sdl),
		    sdl->sdl_alen = ah->ar_hln);
		if (rt->rt_expire)
			rt->rt_expire = time.tv_sec + arpt_keep;
		rt->rt_flags &= ~RTF_REJECT;
		la->la_asked = 0;

		s = splnet();
		mold = la->la_hold;
		la->la_hold = 0;
		splx(s);

		if (mold) {
			arpstat.as_dfrsent++;
			(*ifp->if_output)(ifp, mold, rt_key(rt), rt);
		}
	}
reply:
	if (op != ARPOP_REQUEST) {
		if (op == ARPOP_REPLY)
			arpstat.as_rcvreply++;
	out:
		m_freem(m);
		return;
	}
	arpstat.as_rcvrequest++;
	if (in_hosteq(itaddr, myaddr)) {
		/* I am the target */
		if (ar_tha(ah))
			bcopy((caddr_t)ar_sha(ah), (caddr_t)ar_tha(ah),
			    ah->ar_hln);
		bcopy(LLADDR(ifp->if_sadl), (caddr_t)ar_sha(ah), ah->ar_hln);
	} else {
		la = arplookup(m, &itaddr, 0, SIN_PROXY);
		if (la == 0)
			goto out;
		rt = la->la_rt;
		if (ar_tha(ah))
			bcopy((caddr_t)ar_sha(ah), (caddr_t)ar_tha(ah),
			    ah->ar_hln);
		sdl = SDL(rt->rt_gateway);
		bcopy(LLADDR(sdl), (caddr_t)ar_sha(ah), ah->ar_hln);
	}

	bcopy((caddr_t)ar_spa(ah), (caddr_t)ar_tpa(ah), ah->ar_pln);
	bcopy((caddr_t)&itaddr, (caddr_t)ar_spa(ah), ah->ar_pln);
	ah->ar_op = htons(ARPOP_REPLY);
	ah->ar_pro = htons(ETHERTYPE_IP); /* let's be sure! */
	switch (ifp->if_type) {
	case IFT_IEEE1394:
		/*
		 * ieee1394 arp reply is broadcast
		 */
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
	arpstat.as_sndtotal++;
	arpstat.as_sndreply++;
	(*ifp->if_output)(ifp, m, &sa, (struct rtentry *)0);
	return;
}

/*
 * Free an arp entry.
 */
static void arptfree(struct llinfo_arp *la)
{
	struct rtentry *rt = la->la_rt;
	struct sockaddr_dl *sdl;

	ARP_LOCK_CHECK();

	if (rt == 0)
		panic("arptfree");
	if (rt->rt_refcnt > 0 && (sdl = SDL(rt->rt_gateway)) &&
	    sdl->sdl_family == AF_LINK) {
		sdl->sdl_alen = 0;
		la->la_asked = 0;
		rt->rt_flags &= ~RTF_REJECT;
		return;
	}
	rtrequest(RTM_DELETE, rt_key(rt), (struct sockaddr *)0, rt_mask(rt),
	    0, (struct rtentry **)0);
}

/*
 * Lookup or enter a new address in arptab.
 */
static struct llinfo_arp *
arplookup(struct mbuf *m, struct in_addr *addr, int create, int proxy)
{
	struct arphdr *ah;
	struct ifnet *ifp = m->m_pkthdr.rcvif;
	struct rtentry *rt;
	static struct sockaddr_inarp sin;
	const char *why = 0;

	ah = mtod(m, struct arphdr *);
	sin.sin_len = sizeof(sin);
	sin.sin_family = AF_INET;
	sin.sin_addr = *addr;
	sin.sin_other = proxy ? SIN_PROXY : 0;
	rt = rtalloc1(sintosa(&sin), create);
	if (rt == 0)
		return (0);
	rt->rt_refcnt--;

	if ((rt->rt_flags & (RTF_GATEWAY | RTF_LLINFO)) == RTF_LLINFO &&
	    rt->rt_gateway->sa_family == AF_LINK)
		return ((struct llinfo_arp *)rt->rt_llinfo);



	if (create) {
		if (rt->rt_flags & RTF_GATEWAY)
			why = "host is not on local network";
		else if ((rt->rt_flags & RTF_LLINFO) == 0) {
			arpstat.as_allocfail++;
			why = "could not allocate llinfo";
		} else
			why = "gateway route is not ours";
		log(LOG_DEBUG, "arplookup: unable to enter address"
		    " for %s@%s on %s (%s)\n",
		    in_fmtaddr(*addr), lla_snprintf(ar_sha(ah), ah->ar_hln),
		    (ifp) ? ifp->if_xname : 0, why);
		if (rt->rt_refcnt <= 0 && (rt->rt_flags & RTF_CLONED) != 0) {
			rtrequest(RTM_DELETE, (struct sockaddr *)rt_key(rt),
		    	    rt->rt_gateway, rt_mask(rt), rt->rt_flags, 0);
		}
	}
	return (0);
}

int
arpioctl(u_long cmd, caddr_t data)
{

	return (EOPNOTSUPP);
}

void
arp_ifinit(struct ifnet *ifp, struct ifaddr *ifa)
{
	struct in_addr *ip;

	/*
	 * Warn the user if another station has this IP address,
	 * but only if the interface IP address is not zero.
	 */
	ip = &IA_SIN(ifa)->sin_addr;
	if (!in_nullhost(*ip))
		arprequest(ifp, ip, ip, LLADDR(ifp->if_sadl));

	ifa->ifa_rtrequest = arp_rtrequest;
	ifa->ifa_flags |= RTF_CLONING;
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

	if (m->m_len < sizeof(struct arphdr))
		goto out;
	ar = mtod(m, struct arphdr *);
#if 0 /* XXX I don't think we need this... and it will prevent other LL */
	if (ntohs(ar->ar_hrd) != ARPHRD_ETHER)
		goto out;
#endif
	if (m->m_len < sizeof(struct arphdr) + 2 * (ar->ar_hln + ar->ar_pln))
		goto out;
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
	struct ifnet *ifp;
	struct arphdr *ah;
	int op;

	ah = mtod(m, struct arphdr *);
	op = ntohs(ah->ar_op);

	switch (m->m_pkthdr.rcvif->if_type) {
	case IFT_IEEE1394:
		/* ARP without target hardware address is not supported */
		goto out;
	default:
		break;
	}

	switch (op) {
	case ARPOP_REQUEST:
	case ARPOP_REPLY:	/* per RFC */
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
	ifp = m->m_pkthdr.rcvif;
	if (ifp != myip_ifp) /* !same interface */
		goto out;
	if (myip_initialized)
		goto wake;
	if (bcmp(ar_tha(ah), LLADDR(ifp->if_sadl), ifp->if_sadl->sdl_alen))
		goto out;
	bcopy((caddr_t)ar_spa(ah), (caddr_t)&srv_ip, sizeof(srv_ip));
	bcopy((caddr_t)ar_tpa(ah), (caddr_t)&myip, sizeof(myip));
	myip_initialized = 1;
wake:	/* Do wakeup every time in case it was missed. */
	wakeup((caddr_t)&myip);

out:
	m_freem(m);
}

/*
 * Send a RARP request for the ip address of the specified interface.
 * The request should be RFC 903-compliant.
 */
void
revarprequest(struct ifnet *ifp)
{
	struct sockaddr sa;
	struct mbuf *m;
	struct arphdr *ah;

	if ((m = m_gethdr(M_DONTWAIT, MT_DATA)) == NULL)
		return;
	MCLAIM(m, &arpdomain.dom_mowner);
	m->m_len = sizeof(*ah) + 2*sizeof(struct in_addr) +
	    2*ifp->if_addrlen;
	m->m_pkthdr.len = m->m_len;
	MH_ALIGN(m, m->m_len);
	ah = mtod(m, struct arphdr *);
	bzero((caddr_t)ah, m->m_len);
	ah->ar_pro = htons(ETHERTYPE_IP);
	ah->ar_hln = ifp->if_addrlen;		/* hardware address length */
	ah->ar_pln = sizeof(struct in_addr);	/* protocol address length */
	ah->ar_op = htons(ARPOP_REVREQUEST);

	bcopy(LLADDR(ifp->if_sadl), (caddr_t)ar_sha(ah), ah->ar_hln);
	bcopy(LLADDR(ifp->if_sadl), (caddr_t)ar_tha(ah), ah->ar_hln);

	sa.sa_family = AF_ARP;
	sa.sa_len = 2;
	m->m_flags |= M_BCAST;
	(*ifp->if_output)(ifp, m, &sa, (struct rtentry *)0);

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
		result = tsleep((caddr_t)&myip, PSOCK, "revarp", hz/2);
		if (result != EWOULDBLOCK)
			break;
	}
	revarp_in_progress = 0;

	if (!myip_initialized)
		return ENETUNREACH;

	bcopy((caddr_t)&srv_ip, serv_in, sizeof(*serv_in));
	bcopy((caddr_t)&myip, clnt_in, sizeof(*clnt_in));
	return 0;
}



#ifdef DDB

#include <machine/db_machdep.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>

static void
db_print_sa(const struct sockaddr *sa)
{
	int len;
	const u_char *p;

	if (sa == 0) {
		db_printf("[NULL]");
		return;
	}

	p = (const u_char *)sa;
	len = sa->sa_len;
	db_printf("[");
	while (len > 0) {
		db_printf("%d", *p);
		p++; len--;
		if (len) db_printf(",");
	}
	db_printf("]\n");
}

static void
db_print_ifa(struct ifaddr *ifa)
{
	if (ifa == 0)
		return;
	db_printf("  ifa_addr=");
	db_print_sa(ifa->ifa_addr);
	db_printf("  ifa_dsta=");
	db_print_sa(ifa->ifa_dstaddr);
	db_printf("  ifa_mask=");
	db_print_sa(ifa->ifa_netmask);
	db_printf("  flags=0x%x,refcnt=%d,metric=%d\n",
			  ifa->ifa_flags,
			  ifa->ifa_refcnt,
			  ifa->ifa_metric);
}

static void
db_print_llinfo(caddr_t li)
{
	struct llinfo_arp *la;

	if (li == 0)
		return;
	la = (struct llinfo_arp *)li;
	db_printf("  la_rt=%p la_hold=%p, la_asked=0x%lx\n",
			  la->la_rt, la->la_hold, la->la_asked);
}

/*
 * Function to pass to rn_walktree().
 * Return non-zero error to abort walk.
 */
static int
db_show_radix_node(struct radix_node *rn, void *w)
{
	struct rtentry *rt = (struct rtentry *)rn;

	db_printf("rtentry=%p", rt);

	db_printf(" flags=0x%x refcnt=%d use=%ld expire=%ld\n",
			  rt->rt_flags, rt->rt_refcnt,
			  rt->rt_use, rt->rt_expire);

	db_printf(" key="); db_print_sa(rt_key(rt));
	db_printf(" mask="); db_print_sa(rt_mask(rt));
	db_printf(" gw="); db_print_sa(rt->rt_gateway);

	db_printf(" ifp=%p ", rt->rt_ifp);
	if (rt->rt_ifp)
		db_printf("(%s)", rt->rt_ifp->if_xname);
	else
		db_printf("(NULL)");

	db_printf(" ifa=%p\n", rt->rt_ifa);
	db_print_ifa(rt->rt_ifa);

	db_printf(" genmask="); db_print_sa(rt->rt_genmask);

	db_printf(" gwroute=%p llinfo=%p\n",
			  rt->rt_gwroute, rt->rt_llinfo);
	db_print_llinfo(rt->rt_llinfo);

	return (0);
}

/*
 * Function to print all the route trees.
 * Use this from ddb:  "show arptab"
 */
void
db_show_arptab(db_expr_t addr, int have_addr, db_expr_t count, const char *modif)
{
	struct radix_node_head *rnh;
	rnh = rt_tables[AF_INET];
	db_printf("Route tree for AF_INET\n");
	if (rnh == NULL) {
		db_printf(" (not initialized)\n");
		return;
	}
	rn_walktree(rnh, db_show_radix_node, NULL);
	return;
}
#endif

SYSCTL_SETUP(sysctl_net_inet_arp_setup, "sysctl net.inet.arp subtree setup")
{
	const struct sysctlnode *node;

	sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT,
			CTLTYPE_NODE, "net", NULL,
			NULL, 0, NULL, 0,
			CTL_NET, CTL_EOL);
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
			CTLTYPE_INT, "prune",
			SYSCTL_DESCR("ARP cache pruning interval"),
			NULL, 0, &arpt_prune, 0,
			CTL_NET,PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "keep",
			SYSCTL_DESCR("Valid ARP entry lifetime"),
			NULL, 0, &arpt_keep, 0,
			CTL_NET,PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "down",
			SYSCTL_DESCR("Failed ARP entry lifetime"),
			NULL, 0, &arpt_down, 0,
			CTL_NET,PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "refresh",
			SYSCTL_DESCR("ARP entry refresh interval"),
			NULL, 0, &arpt_refresh, 0,
			CTL_NET,PF_INET, node->sysctl_num, CTL_CREATE, CTL_EOL);
}

#endif /* INET */
