/*	$NetBSD: if_arp.c,v 1.39 1997/08/04 06:18:49 lukem Exp $	*/

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
 *	@(#)if_ether.c	8.1 (Berkeley) 6/10/93
 */

/*
 * Ethernet address resolution protocol.
 * TODO:
 *	add "inuse/lock" bit (or ref. count) along with valid bit
 */

#ifdef INET

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/proc.h>

#include <net/ethertypes.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>

#include "loop.h"

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
#define	rt_expire rt_rmx.rmx_expire

static	void arprequest __P((struct ifnet *,
	    struct in_addr *, struct in_addr *, u_int8_t *));
static	void arptfree __P((struct llinfo_arp *));
static	void arptimer __P((void *));
static	struct llinfo_arp *arplookup __P((struct in_addr *, int, int));
static	void in_arpinput __P((struct mbuf *));

extern	struct ifnet loif[NLOOP];
LIST_HEAD(, llinfo_arp) llinfo_arp;
struct	ifqueue arpintrq = {0, 0, 0, 50};
int	arp_inuse, arp_allocated, arp_intimer;
int	arp_maxtries = 5;
int	useloopback = 1;	/* use loopback interface for local traffic */
int	arpinit_done = 0;

/* revarp state */
static struct	in_addr myip, srv_ip;
static int	myip_initialized = 0;
static int	revarp_in_progress = 0;
static struct	ifnet *myip_ifp = NULL;

#ifdef DDB
static void db_print_sa __P((struct sockaddr *));
static void db_print_ifa __P((struct ifaddr *));
static void db_print_llinfo __P((caddr_t));
static int db_show_radix_node __P((struct radix_node *, void *));
#endif

/*
 * this should be elsewhere.
 */

static char *
lla_snprintf __P((u_int8_t *, int));

static char *
lla_snprintf(adrp, len)
	u_int8_t *adrp;
	int len;
{
	static char buf[16*3];
	static const char hexdigits[] = {
	    '0','1','2','3','4','5','6','7',
	    '8','9','a','b','c','d','e','f'
	};
		
	int i;
	char *p;

	p = buf;

	*p++ = hexdigits[(*adrp)>>4];
	*p++ = hexdigits[(*adrp++)&0xf];

	for (i=1; i<len && i<16; i++) {
		*p++ = ':';
		*p++ = hexdigits[(*adrp)>>4];
		*p++ = hexdigits[(*adrp++)&0xf];
	}

	*p = 0;
	return buf;
}

/*
 * Timeout routine.  Age arp_tab entries periodically.
 */
/* ARGSUSED */
static void
arptimer(arg)
	void *arg;
{
	int s;
	register struct llinfo_arp *la, *nla;

	s = splsoftnet();
	timeout(arptimer, NULL, arpt_prune * hz);
	for (la = llinfo_arp.lh_first; la != 0; la = nla) {
		register struct rtentry *rt = la->la_rt;

		nla = la->la_list.le_next;
		if (rt->rt_expire && rt->rt_expire <= time.tv_sec)
			arptfree(la); /* timer has expired; clear */
	}
	splx(s);
}

/*
 * Parallel to llc_rtrequest.
 */
void
arp_rtrequest(req, rt, sa)
	int req;
	register struct rtentry *rt;
	struct sockaddr *sa;
{
	register struct sockaddr *gate = rt->rt_gateway;
	register struct llinfo_arp *la = (struct llinfo_arp *)rt->rt_llinfo;
	static struct sockaddr_dl null_sdl = {sizeof(null_sdl), AF_LINK};

	if (!arpinit_done) {
		arpinit_done = 1;
		/*
		 * We generate expiration times from time.tv_sec
		 * so avoid accidently creating permanent routes.
		 */
		if (time.tv_sec == 0) {
			time.tv_sec++;
		}
		timeout(arptimer, (caddr_t)0, hz);
	}
	if (rt->rt_flags & RTF_GATEWAY)
		return;
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
		R_Malloc(la, struct llinfo_arp *, sizeof(*la));
		rt->rt_llinfo = (caddr_t)la;
		if (la == 0) {
			log(LOG_DEBUG, "arp_rtrequest: malloc failed\n");
			break;
		}
		arp_inuse++, arp_allocated++;
		Bzero(la, sizeof(*la));
		la->la_rt = rt;
		rt->rt_flags |= RTF_LLINFO;
		LIST_INSERT_HEAD(&llinfo_arp, la, la_list);
		if (in_hosteq(SIN(rt_key(rt))->sin_addr,
		    (IA_SIN(rt->rt_ifa))->sin_addr)) {
			/*
			 * This test used to be
			 *	if (loif.if_flags & IFF_UP)
			 * It allowed local traffic to be forced through
			 * the hardware by configuring the loopback down.
			 * However, it causes problems during network
			 * configuration for boards that can't receive
			 * packets they send.  It is now necessary to clear
			 * "useloopback" and remove the route to force
			 * traffic out to the hardware.
			 */
			rt->rt_expire = 0;
			Bcopy(LLADDR(rt->rt_ifp->if_sadl),
			    LLADDR(SDL(gate)),
			    SDL(gate)->sdl_alen = 
			    rt->rt_ifp->if_data.ifi_addrlen);
			if (useloopback)
				rt->rt_ifp = &loif[0];
		}
		break;

	case RTM_DELETE:
		if (la == 0)
			break;
		arp_inuse--;
		LIST_REMOVE(la, la_list);
		rt->rt_llinfo = 0;
		rt->rt_flags &= ~RTF_LLINFO;
		if (la->la_hold)
			m_freem(la->la_hold);
		Free((caddr_t)la);
	}
}

/*
 * Broadcast an ARP request. Caller specifies:
 *	- arp header source ip address
 *	- arp header target ip address
 *	- arp header source ethernet address
 */
static void
arprequest(ifp, sip, tip, enaddr)
	register struct ifnet *ifp;
	register struct in_addr *sip, *tip;
	register u_int8_t *enaddr;
{
	register struct mbuf *m;
	struct arphdr *ah;
	struct sockaddr sa;

	if ((m = m_gethdr(M_DONTWAIT, MT_DATA)) == NULL)
		return;
	m->m_len = sizeof(*ah) + 2*sizeof(struct in_addr) +
	    2*ifp->if_data.ifi_addrlen;
	m->m_pkthdr.len = m->m_len;
	MH_ALIGN(m, m->m_len);
	ah = mtod(m, struct arphdr *);
	bzero((caddr_t)ah, m->m_len);
	ah->ar_pro = htons(ETHERTYPE_IP);
	ah->ar_hln = ifp->if_data.ifi_addrlen;	/* hardware address length */
	ah->ar_pln = sizeof(struct in_addr);	/* protocol address length */
	ah->ar_op = htons(ARPOP_REQUEST);
	bcopy((caddr_t)enaddr, (caddr_t)ar_sha(ah), ah->ar_hln);
	bcopy((caddr_t)sip, (caddr_t)ar_spa(ah), ah->ar_pln);
	bcopy((caddr_t)tip, (caddr_t)ar_tpa(ah), ah->ar_pln);
	sa.sa_family = AF_ARP;
	sa.sa_len = 2;
	m->m_flags |= M_BCAST;
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
arpresolve(ifp, rt, m, dst, desten)
	register struct ifnet *ifp;
	register struct rtentry *rt;
	struct mbuf *m;
	register struct sockaddr *dst;
	register u_char *desten;
{
	register struct llinfo_arp *la;
	struct sockaddr_dl *sdl;

	if (rt)
		la = (struct llinfo_arp *)rt->rt_llinfo;
	else {
		if ((la = arplookup(&SIN(dst)->sin_addr, 1, 0)) != NULL)
			rt = la->la_rt;
	}
	if (la == 0 || rt == 0) {
		log(LOG_DEBUG, "arpresolve: can't allocate llinfo\n");
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
		    min(sdl->sdl_alen, ifp->if_data.ifi_addrlen));
		return 1;
	}
	/*
	 * There is an arptab entry, but no ethernet address
	 * response yet.  Replace the held mbuf with this
	 * latest one.
	 */
	if (la->la_hold)
		m_freem(la->la_hold);
	la->la_hold = m;
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
arpintr()
{
	register struct mbuf *m;
	register struct arphdr *ar;
	int s;

	while (arpintrq.ifq_head) {
		s = splimp();
		IF_DEQUEUE(&arpintrq, m);
		splx(s);
		if (m == 0 || (m->m_flags & M_PKTHDR) == 0)
			panic("arpintr");

		if (m->m_len >= sizeof(struct arphdr) &&
		    (ar = mtod(m, struct arphdr *)) &&
		    /* XXX ntohs(ar->ar_hrd) == ARPHRD_ETHER && */
		    m->m_len >=
		      sizeof(struct arphdr) + 2 * (ar->ar_hln + ar->ar_pln))
			switch (ntohs(ar->ar_pro)) {

			case ETHERTYPE_IP:
			case ETHERTYPE_IPTRAILERS:
				in_arpinput(m);
				continue;
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
in_arpinput(m)
	struct mbuf *m;
{
	struct arphdr *ah;
	register struct ifnet *ifp = m->m_pkthdr.rcvif;
	register struct llinfo_arp *la = 0;
	register struct rtentry *rt;
	struct in_ifaddr *ia, *maybe_ia = 0;
	struct sockaddr_dl *sdl;
	struct sockaddr sa;
	struct in_addr isaddr, itaddr, myaddr;
	int op;

	ah = mtod(m, struct arphdr *);
	op = ntohs(ah->ar_op);
	bcopy((caddr_t)ar_spa(ah), (caddr_t)&isaddr, sizeof (isaddr));
	bcopy((caddr_t)ar_tpa(ah), (caddr_t)&itaddr, sizeof (itaddr));
	for (ia = in_ifaddr.tqh_first; ia != 0; ia = ia->ia_list.tqe_next)
		if (ia->ia_ifp == ifp) {
			maybe_ia = ia;
			if (in_hosteq(itaddr, ia->ia_addr.sin_addr) ||
			    in_hosteq(isaddr, ia->ia_addr.sin_addr))
				break;
		}
	if (maybe_ia == 0)
		goto out;

	myaddr = ia ? ia->ia_addr.sin_addr : maybe_ia->ia_addr.sin_addr;
	if (!bcmp((caddr_t)ar_sha(ah), LLADDR(ifp->if_sadl),
	    ifp->if_data.ifi_addrlen))
		goto out;	/* it's from me, ignore it. */
/*
 *  XXX
 *	if (!bcmp((caddr_t)ar_sha(ah), (caddr_t)etherbroadcastaddr,
 *	    sizeof (ea->arp_sha))) {
 *		log(LOG_ERR,
 *		    "arp: ether address is broadcast for IP address %x!\n",
 *		    ntohl(isaddr.s_addr));
 *		goto out;
 *	}
 */
	if (in_hosteq(isaddr, myaddr)) {
		log(LOG_ERR,
		   "duplicate IP address %08x sent from link address %s\n",
		   ntohl(isaddr.s_addr), lla_snprintf(ar_sha(ah), ah->ar_hln));
		itaddr = myaddr;
		goto reply;
	}
	la = arplookup(&isaddr, in_hosteq(itaddr, myaddr), 0);
	if (la && (rt = la->la_rt) && (sdl = SDL(rt->rt_gateway))) {
		if (sdl->sdl_alen &&
		    bcmp((caddr_t)ar_sha(ah), LLADDR(sdl), sdl->sdl_alen))
			log(LOG_INFO, "arp info overwritten for %08x by %s\n",
			    ntohl(isaddr.s_addr),
			    lla_snprintf(ar_sha(ah), ah->ar_hln));
		/* 
		 * sanity check for the address length.
		 * XXX this does not work for protocols with variable address
		 * length. -is
		 */
		if (sdl->sdl_alen &&
		    sdl->sdl_alen != ah->ar_hln) {
			log(LOG_WARNING, 
			    "arp from %08x: new addr len %d, was %d",
			    ntohl(isaddr.s_addr), ah->ar_hln, sdl->sdl_alen);
		}
		if (ifp->if_data.ifi_addrlen != ah->ar_hln) {
			log(LOG_WARNING, 
			    "arp from %08x: addr len: new %d, i/f %d (ignored)",
			    ntohl(isaddr.s_addr), ah->ar_hln,
			    ifp->if_data.ifi_addrlen);
			goto reply;
		}
		bcopy((caddr_t)ar_sha(ah), LLADDR(sdl),
		    sdl->sdl_alen = ah->ar_hln);
		if (rt->rt_expire)
			rt->rt_expire = time.tv_sec + arpt_keep;
		rt->rt_flags &= ~RTF_REJECT;
		la->la_asked = 0;
		if (la->la_hold) {
			(*ifp->if_output)(ifp, la->la_hold,
				rt_key(rt), rt);
			la->la_hold = 0;
		}
	}
reply:
	if (op != ARPOP_REQUEST) {
	out:
		m_freem(m);
		return;
	}
	if (in_hosteq(itaddr, myaddr)) {
		/* I am the target */
		bcopy((caddr_t)ar_sha(ah), (caddr_t)ar_tha(ah), ah->ar_hln);
		bcopy(LLADDR(ifp->if_sadl), (caddr_t)ar_sha(ah), ah->ar_hln);
	} else {
		la = arplookup(&itaddr, 0, SIN_PROXY);
		if (la == 0)
			goto out;
		rt = la->la_rt;
		bcopy((caddr_t)ar_sha(ah), (caddr_t)ar_tha(ah), ah->ar_hln);
		sdl = SDL(rt->rt_gateway);
		bcopy(LLADDR(sdl), (caddr_t)ar_sha(ah), ah->ar_hln);
	}

	bcopy((caddr_t)ar_spa(ah), (caddr_t)ar_tpa(ah), ah->ar_pln);
	bcopy((caddr_t)&itaddr, (caddr_t)ar_spa(ah), ah->ar_pln);
	ah->ar_op = htons(ARPOP_REPLY);
	ah->ar_pro = htons(ETHERTYPE_IP); /* let's be sure! */
	m->m_flags &= ~(M_BCAST|M_MCAST); /* never reply by broadcast */
	sa.sa_family = AF_ARP;
	sa.sa_len = 2;
	(*ifp->if_output)(ifp, m, &sa, (struct rtentry *)0);
	return;
}

/*
 * Free an arp entry.
 */
static void
arptfree(la)
	register struct llinfo_arp *la;
{
	register struct rtentry *rt = la->la_rt;
	register struct sockaddr_dl *sdl;

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
arplookup(addr, create, proxy)
	struct in_addr *addr;
	int create, proxy;
{
	register struct rtentry *rt;
	static struct sockaddr_inarp sin;

	sin.sin_len = sizeof(sin);
	sin.sin_family = AF_INET;
	sin.sin_addr = *addr;
	sin.sin_other = proxy ? SIN_PROXY : 0;
	rt = rtalloc1(sintosa(&sin), create);
	if (rt == 0)
		return (0);
	rt->rt_refcnt--;
	if ((rt->rt_flags & RTF_GATEWAY) || (rt->rt_flags & RTF_LLINFO) == 0 ||
	    rt->rt_gateway->sa_family != AF_LINK) {
		if (create)
			log(LOG_DEBUG, "arplookup: unable to enter address for %x\n",
			    ntohl(addr->s_addr));
		return (0);
	}
	return ((struct llinfo_arp *)rt->rt_llinfo);
}

int
arpioctl(cmd, data)
	u_long cmd;
	caddr_t data;
{

	return (EOPNOTSUPP);
}

void
arp_ifinit(ifp, ifa)
	struct ifnet *ifp;
	struct ifaddr *ifa;
{

	/* Warn the user if another station has this IP address. */
	arprequest(ifp,
	    &IA_SIN(ifa)->sin_addr,
	    &IA_SIN(ifa)->sin_addr,
	    LLADDR(ifp->if_sadl));
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
revarpinput(m)
	struct mbuf *m;
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
in_revarpinput(m)
	struct mbuf *m;
{
	struct ifnet *ifp;
	struct arphdr *ah;
	int op;

	ah = mtod(m, struct arphdr *);
	op = ntohs(ah->ar_op);
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
revarprequest(ifp)
	struct ifnet *ifp;
{
	struct sockaddr sa;
	struct mbuf *m;
	struct arphdr *ah;

	if ((m = m_gethdr(M_DONTWAIT, MT_DATA)) == NULL)
		return;
	m->m_len = sizeof(*ah) + 2*sizeof(struct in_addr) +
	    2*ifp->if_data.ifi_addrlen;
	m->m_pkthdr.len = m->m_len;
	MH_ALIGN(m, m->m_len);
	ah = mtod(m, struct arphdr *);
	bzero((caddr_t)ah, m->m_len);
	ah->ar_pro = htons(ETHERTYPE_IP);
	ah->ar_hln = ifp->if_data.ifi_addrlen;	/* hardware address length */
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
revarpwhoarewe(ifp, serv_in, clnt_in)
	struct ifnet *ifp;
	struct in_addr *serv_in;
	struct in_addr *clnt_in;
{
	int result, count = 20;
	
	if (!myip_initialized) {
		myip_ifp = ifp;
		revarp_in_progress = 1;
		while (count--) {
			revarprequest(ifp);
			result = tsleep((caddr_t)&myip, PSOCK, "revarp", hz/2);
			if (result != EWOULDBLOCK)
				break;
		}
		revarp_in_progress = 0;
	}
	if (!myip_initialized)
		return ENETUNREACH;
	
	bcopy((caddr_t)&srv_ip, serv_in, sizeof(*serv_in));
	bcopy((caddr_t)&myip, clnt_in, sizeof(*clnt_in));
	return 0;
}

/* For compatibility: only saves interface address. */
int
revarpwhoami(in, ifp)
	struct in_addr *in;
	struct ifnet *ifp;
{
	struct in_addr server;
	return (revarpwhoarewe(ifp, &server, in));
}


#ifdef DDB

#include <machine/db_machdep.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>
static void
db_print_sa(sa)
	struct sockaddr *sa;
{
	int len;
	u_char *p;

	if (sa == 0) {
		db_printf("[NULL]");
		return;
	}

	p = (u_char*)sa;
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
db_print_ifa(ifa)
	struct ifaddr *ifa;
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
db_print_llinfo(li)
	caddr_t li;
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
db_show_radix_node(rn, w)
	struct radix_node *rn;
	void *w;
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
 * Use this from ddb:  "call db_show_arptab"
 */
int
db_show_arptab()
{
	struct radix_node_head *rnh;
	rnh = rt_tables[AF_INET];
	db_printf("Route tree for AF_INET\n");
	if (rnh == NULL) {
		db_printf(" (not initialized)\n");
		return (0);
	}
	rn_walktree(rnh, db_show_radix_node, NULL);
	return (0);
}
#endif
#endif /* INET */
