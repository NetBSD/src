/*	$NetBSD: if_stf.c,v 1.5.2.3 2000/12/13 15:50:32 bouyer Exp $	*/
/*	$KAME: if_stf.c,v 1.39 2000/06/07 23:35:18 itojun Exp $	*/

/*
 * Copyright (C) 2000 WIDE Project.
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
 * 6to4 interface, based on draft-ietf-ngtrans-6to4-06.txt.
 *
 * 6to4 interface is NOT capable of link-layer (I mean, IPv4) multicasting.
 * There is no address mapping defined from IPv6 multicast address to IPv4
 * address.  Therefore, we do not have IFF_MULTICAST on the interface.
 *
 * Due to the lack of address mapping for link-local addresses, we cannot
 * throw packets toward link-local addresses (fe80::x).  Also, we cannot throw
 * packets to link-local multicast addresses (ff02::x).
 *
 * Here are interesting symptoms due to the lack of link-local address:
 *
 * Unicast routing exchange:
 * - RIPng: Impossible.  Uses link-local multicast packet toward ff02::9,
 *   and link-local addresses as nexthop.
 * - OSPFv6: Impossible.  OSPFv6 assumes that there's link-local address
 *   assigned to the link, and makes use of them.  Also, HELLO packets use
 *   link-local multicast addresses (ff02::5 and ff02::6).
 * - BGP4+: Maybe.  You can only use global address as nexthop, and global
 *   address as TCP endpoint address.
 *
 * Multicast routing protocols:
 * - PIM: Hello packet cannot be used to discover adjacent PIM routers.
 *   Adjacent PIM routers must be configured manually (is it really spec-wise
 *   correct thing to do?).
 *
 * ICMPv6:
 * - Redirects cannot be used due to the lack of link-local address.
 *
 * Starting from 04 draft, the specification suggests how to construct
 * link-local address for 6to4 interface.
 * However, it seems to have no real use and does not help the above symptom
 * much.  Even if we assign link-locals to interface, we cannot really
 * use link-local unicast/multicast on top of 6to4 cloud, and the above
 * analysis does not change.
 *
 * 6to4 interface has security issues.  Refer to
 * http://playground.iijlab.net/i-d/draft-itojun-ipv6-transition-abuse-00.txt
 * for details.  The code tries to filter out some of malicious packets.
 * Note that there is no way to be 100% secure.
 */

#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include "opt_inet.h"
#include "opt_inet6.h"
#endif
#ifdef __NetBSD__
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#if !(defined(__FreeBSD__) && __FreeBSD__ >= 3)
#include <sys/ioctl.h>
#endif
#include <sys/protosw.h>
#ifdef __FreeBSD__
#include <sys/kernel.h>
#endif
#include <sys/queue.h>

#include <machine/cpu.h>

#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <sys/malloc.h>
#endif

#include <net/if.h>
#include <net/route.h>
#include <net/netisr.h>
#include <net/if_types.h>
#include <net/if_stf.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_var.h>

#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_gif.h>
#include <netinet6/in6_var.h>
#include <netinet/ip_ecn.h>

#include <netinet/ip_encap.h>

#include <machine/stdarg.h>

#include <net/net_osdep.h>

#if defined(__FreeBSD__) && __FreeBSD__ >= 4
#include "bpf.h"
#define NBPFILTER	NBPF
#else
#include "bpfilter.h"
#endif
#include "stf.h"
#include "gif.h"	/*XXX*/

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#if NGIF > 0
#include <net/if_gif.h>
#endif

#if NSTF > 0

#define IN6_IS_ADDR_6TO4(x)	(ntohs((x)->s6_addr16[0]) == 0x2002)
#define GET_V4(x)	((struct in_addr *)(&(x)->s6_addr16[1]))

struct stf_softc {
	struct ifnet	sc_if;	   /* common area */
	union {
		struct route  __sc_ro4;
		struct route_in6 __sc_ro6; /* just for safety */
	} __sc_ro46;
#define sc_ro	__sc_ro46.__sc_ro4
	const struct encaptab *encap_cookie;
	LIST_ENTRY(stf_softc) sc_list;
};

LIST_HEAD(, stf_softc) stf_softc_list;

int	stf_clone_create __P((struct if_clone *, int));
void	stf_clone_destroy __P((struct ifnet *));

struct if_clone stf_cloner =
    IF_CLONE_INITIALIZER("stf", stf_clone_create, stf_clone_destroy);

#if NGIF > 0
extern int ip_gif_ttl;	/*XXX*/
#else
static int ip_gif_ttl = 40;	/*XXX*/
#endif

extern struct protosw in_stf_protosw;

#ifdef __FreeBSD__
void stfattach __P((void *));
#else
void stfattach __P((int));
#endif
static int stf_encapcheck __P((const struct mbuf *, int, int, void *));
static struct in6_ifaddr *stf_getsrcifa6 __P((struct ifnet *));
static int stf_output __P((struct ifnet *, struct mbuf *, struct sockaddr *,
	struct rtentry *));
static int stf_checkaddr4 __P((struct in_addr *, struct ifnet *));
static int stf_checkaddr6 __P((struct in6_addr *, struct ifnet *));
#if defined(__bsdi__) && _BSDI_VERSION >= 199802
static void stf_rtrequest __P((int, struct rtentry *, struct rt_addrinfo *));
#else
static void stf_rtrequest __P((int, struct rtentry *, struct sockaddr *));
#endif
#if defined(__FreeBSD__) && __FreeBSD__ < 3
static int stf_ioctl __P((struct ifnet *, int, caddr_t));
#else
static int stf_ioctl __P((struct ifnet *, u_long, caddr_t));
#endif

/* ARGSUSED */
void
stfattach(count)
	int count;
{

	LIST_INIT(&stf_softc_list);
	if_clone_attach(&stf_cloner);
}

int
stf_clone_create(ifc, unit)
	struct if_clone *ifc;
	int unit;
{
	struct stf_softc *sc;

	if (LIST_FIRST(&stf_softc_list) != NULL) {
		/* Only one stf interface is allowed. */
		return (EEXIST);
	}

	sc = malloc(sizeof(struct stf_softc), M_DEVBUF, M_WAIT);
	bzero(sc, sizeof(struct stf_softc));

	sprintf(sc->sc_if.if_xname, "%s%d", ifc->ifc_name, unit);

	sc->encap_cookie = encap_attach_func(AF_INET, IPPROTO_IPV6,
	    stf_encapcheck, &in_stf_protosw, sc);
	if (sc->encap_cookie == NULL) {
		printf("%s: unable to attach encap\n", if_name(&sc->sc_if));
		free(sc, M_DEVBUF);
		return (EIO);	/* XXX */
	}

	sc->sc_if.if_mtu    = IPV6_MMTU;
	sc->sc_if.if_flags  = 0;
	sc->sc_if.if_ioctl  = stf_ioctl;
	sc->sc_if.if_output = stf_output;
	sc->sc_if.if_type   = IFT_STF;
#if defined(__FreeBSD__) && __FreeBSD__ >= 4
	sc->sc_if.if_snd.ifq_maxlen = IFQ_MAXLEN;
#endif
	if_attach(&sc->sc_if);
#if NBPFILTER > 0
	bpfattach(&sc->sc_if, DLT_NULL, sizeof(u_int));
#endif
	LIST_INSERT_HEAD(&stf_softc_list, sc, sc_list);
	return (0);
}

void
stf_clone_destroy(ifp)
	struct ifnet *ifp;
{
	struct stf_softc *sc = (void *) ifp;

	LIST_REMOVE(sc, sc_list);
	encap_detach(sc->encap_cookie);
#if NBPFILTER > 0
	bpfdetach(ifp);
#endif
	if_detach(ifp);
	free(sc, M_DEVBUF);
}

#ifdef __FreeBSD__
PSEUDO_SET(stfattach, if_stf);
#endif

static int
stf_encapcheck(m, off, proto, arg)
	const struct mbuf *m;
	int off;
	int proto;
	void *arg;
{
	struct ip ip;
	struct in6_ifaddr *ia6;
	struct stf_softc *sc;
	struct in_addr a, b;

	sc = (struct stf_softc *)arg;
	if (sc == NULL)
		return 0;

	if ((sc->sc_if.if_flags & IFF_UP) == 0)
		return 0;

	if (proto != IPPROTO_IPV6)
		return 0;

	/* LINTED const cast */
	m_copydata((struct mbuf *)m, 0, sizeof(ip), (caddr_t)&ip);

	if (ip.ip_v != 4)
		return 0;

	ia6 = stf_getsrcifa6(&sc->sc_if);
	if (ia6 == NULL)
		return 0;

	/*
	 * check if IPv4 dst matches the IPv4 address derived from the
	 * local 6to4 address.
	 * success on: dst = 10.1.1.1, ia6->ia_addr = 2002:0a01:0101:...
	 */
	if (bcmp(GET_V4(&ia6->ia_addr.sin6_addr), &ip.ip_dst,
	    sizeof(ip.ip_dst)) != 0)
		return 0;

	/*
	 * check if IPv4 src matches the IPv4 address derived from the
	 * local 6to4 address masked by prefixmask.
	 * success on: src = 10.1.1.1, ia6->ia_addr = 2002:0a00:.../24
	 * fail on: src = 10.1.1.1, ia6->ia_addr = 2002:0b00:.../24
	 */
	bzero(&a, sizeof(a));
	a.s_addr = GET_V4(&ia6->ia_addr.sin6_addr)->s_addr;
	a.s_addr &= GET_V4(&ia6->ia_prefixmask.sin6_addr)->s_addr;
	b = ip.ip_src;
	b.s_addr &= GET_V4(&ia6->ia_prefixmask.sin6_addr)->s_addr;
	if (a.s_addr != b.s_addr)
		return 0;

	/* stf interface makes single side match only */
	return 32;
}

static struct in6_ifaddr *
stf_getsrcifa6(ifp)
	struct ifnet *ifp;
{
	struct ifaddr *ia;
	struct in_ifaddr *ia4;
	struct sockaddr_in6 *sin6;
	struct in_addr in;

#if defined(__bsdi__) || (defined(__FreeBSD__) && __FreeBSD__ < 3)
	for (ia = ifp->if_addrlist; ia; ia = ia->ifa_next)
#else
	for (ia = ifp->if_addrlist.tqh_first;
	     ia;
	     ia = ia->ifa_list.tqe_next)
#endif
	{
		if (ia->ifa_addr == NULL)
			continue;
		if (ia->ifa_addr->sa_family != AF_INET6)
			continue;
		sin6 = (struct sockaddr_in6 *)ia->ifa_addr;
		if (!IN6_IS_ADDR_6TO4(&sin6->sin6_addr))
			continue;

		bcopy(GET_V4(&sin6->sin6_addr), &in, sizeof(in));
#ifdef __NetBSD__
		INADDR_TO_IA(in, ia4);
#else
#ifdef __OpenBSD__
		for (ia4 = in_ifaddr.tqh_first;
		     ia4;
		     ia4 = ia4->ia_list.tqe_next)
#elif defined(__FreeBSD__) && __FreeBSD__ >= 3
		for (ia4 = TAILQ_FIRST(&in_ifaddrhead);
		     ia4;
		     ia4 = TAILQ_NEXT(ia4, ia_link))
#else
		for (ia4 = in_ifaddr; ia4 != NULL; ia4 = ia4->ia_next)
#endif
		{
			if (ia4->ia_addr.sin_addr.s_addr == in.s_addr)
				break;
		}
#endif
		if (ia4 == NULL)
			continue;

		return (struct in6_ifaddr *)ia;
	}

	return NULL;
}

static int
stf_output(ifp, m, dst, rt)
	struct ifnet *ifp;
	struct mbuf *m;
	struct sockaddr *dst;
	struct rtentry *rt;
{
	struct stf_softc *sc;
	struct sockaddr_in6 *dst6;
	struct sockaddr_in *dst4;
	u_int8_t tos;
	struct ip *ip;
	struct ip6_hdr *ip6;
	struct in6_ifaddr *ia6;

	sc = (struct stf_softc*)ifp;
	dst6 = (struct sockaddr_in6 *)dst;

	/* just in case */
	if ((ifp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		return ENETDOWN;
	}

	/*
	 * If we don't have an ip4 address that match my inner ip6 address,
	 * we shouldn't generate output.  Without this check, we'll end up
	 * using wrong IPv4 source.
	 */
	ia6 = stf_getsrcifa6(ifp);
	if (ia6 == NULL) {
		m_freem(m);
		return ENETDOWN;
	}

	if (m->m_len < sizeof(*ip6)) {
		m = m_pullup(m, sizeof(*ip6));
		if (!m)
			return ENOBUFS;
	}
	ip6 = mtod(m, struct ip6_hdr *);
	tos = (ntohl(ip6->ip6_flow) >> 20) & 0xff;

	M_PREPEND(m, sizeof(struct ip), M_DONTWAIT);
	if (m && m->m_len < sizeof(struct ip))
		m = m_pullup(m, sizeof(struct ip));
	if (m == NULL)
		return ENOBUFS;
	ip = mtod(m, struct ip *);

	bzero(ip, sizeof(*ip));

	bcopy(GET_V4(&((struct sockaddr_in6 *)&ia6->ia_addr)->sin6_addr),
	    &ip->ip_src, sizeof(ip->ip_src));
	bcopy(GET_V4(&dst6->sin6_addr), &ip->ip_dst, sizeof(ip->ip_dst));
	ip->ip_p = IPPROTO_IPV6;
	ip->ip_ttl = ip_gif_ttl;	/*XXX*/
	ip->ip_len = m->m_pkthdr.len;	/*host order*/
	if (ifp->if_flags & IFF_LINK1)
		ip_ecn_ingress(ECN_ALLOWED, &ip->ip_tos, &tos);

	dst4 = (struct sockaddr_in *)&sc->sc_ro.ro_dst;
	if (dst4->sin_family != AF_INET ||
	    bcmp(&dst4->sin_addr, &ip->ip_dst, sizeof(ip->ip_dst)) != 0) {
		/* cache route doesn't match */
		dst4->sin_family = AF_INET;
		dst4->sin_len = sizeof(struct sockaddr_in);
		bcopy(&ip->ip_dst, &dst4->sin_addr, sizeof(dst4->sin_addr));
		if (sc->sc_ro.ro_rt) {
			RTFREE(sc->sc_ro.ro_rt);
			sc->sc_ro.ro_rt = NULL;
		}
	}

	if (sc->sc_ro.ro_rt == NULL) {
		rtalloc(&sc->sc_ro);
		if (sc->sc_ro.ro_rt == NULL) {
			m_freem(m);
			return ENETUNREACH;
		}
	}

#ifndef __OpenBSD__
	return ip_output(m, NULL, &sc->sc_ro, 0, NULL);
#else
	return ip_output(m, NULL, &sc->sc_ro, 0, NULL, NULL);
#endif
}

static int
stf_checkaddr4(in, ifp)
	struct in_addr *in;
	struct ifnet *ifp;	/* incoming interface */
{
	struct in_ifaddr *ia4;

	/*
	 * reject packets with the following address:
	 * 224.0.0.0/4 0.0.0.0/8 127.0.0.0/8 255.0.0.0/8
	 */
	if (IN_MULTICAST(in->s_addr))
		return -1;
	switch ((ntohl(in->s_addr) & 0xff000000) >> 24) {
	case 0: case 127: case 255:
		return -1;
	}

	/*
	 * reject packets with broadcast
	 */
#if defined(__OpenBSD__) || defined(__NetBSD__)
	for (ia4 = in_ifaddr.tqh_first; ia4; ia4 = ia4->ia_list.tqe_next)
#elif defined(__FreeBSD__) && __FreeBSD__ >= 3
	for (ia4 = TAILQ_FIRST(&in_ifaddrhead);
	     ia4;
	     ia4 = TAILQ_NEXT(ia4, ia_link))
#else
	for (ia4 = in_ifaddr; ia4 != NULL; ia4 = ia4->ia_next)
#endif
	{
		if ((ia4->ia_ifa.ifa_ifp->if_flags & IFF_BROADCAST) == 0)
			continue;
		if (in->s_addr == ia4->ia_broadaddr.sin_addr.s_addr)
			return -1;
	}

	/*
	 * perform ingress filter
	 */
	if (ifp) {
		struct sockaddr_in sin;
		struct rtentry *rt;

		bzero(&sin, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_len = sizeof(struct sockaddr_in);
		sin.sin_addr = *in;
#ifdef __FreeBSD__
		rt = rtalloc1((struct sockaddr *)&sin, 0, 0UL);
#else
		rt = rtalloc1((struct sockaddr *)&sin, 0);
#endif
		if (!rt)
			return -1;
		if (rt->rt_ifp != ifp) {
			rtfree(rt);
			return -1;
		}
		rtfree(rt);
	}

	return 0;
}

static int
stf_checkaddr6(in6, ifp)
	struct in6_addr *in6;
	struct ifnet *ifp;	/* incoming interface */
{
	/*
	 * check 6to4 addresses
	 */
	if (IN6_IS_ADDR_6TO4(in6))
		return stf_checkaddr4(GET_V4(in6), ifp);

	/*
	 * reject anything that look suspicious.  the test is implemented
	 * in ip6_input too, but we check here as well to
	 * (1) reject bad packets earlier, and
	 * (2) to be safe against future ip6_input change.
	 */
	if (IN6_IS_ADDR_V4COMPAT(in6) || IN6_IS_ADDR_V4MAPPED(in6))
		return -1;

	return 0;
}

void
#if __STDC__
in_stf_input(struct mbuf *m, ...)
#else
in_stf_input(m, va_alist)
	register struct mbuf *m;
#endif
{
	int off, proto;
	struct stf_softc *sc;
	struct ip *ip;
	struct ip6_hdr *ip6;
	u_int8_t otos, itos;
	int s, isr;
	struct ifqueue *ifq = NULL;
	struct ifnet *ifp;
	va_list ap;

	va_start(ap, m);
	off = va_arg(ap, int);
	proto = va_arg(ap, int);
	va_end(ap);

	if (proto != IPPROTO_IPV6) {
		m_freem(m);
		return;
	}

	ip = mtod(m, struct ip *);

	sc = (struct stf_softc *)encap_getarg(m);

	if (sc == NULL || (sc->sc_if.if_flags & IFF_UP) == 0) {
		m_freem(m);
		return;
	}

	ifp = &sc->sc_if;

	/*
	 * perform sanity check against outer src/dst.
	 * for source, perform ingress filter as well.
	 */
	if (stf_checkaddr4(&ip->ip_dst, NULL) < 0 ||
	    stf_checkaddr4(&ip->ip_src, m->m_pkthdr.rcvif) < 0) {
		m_freem(m);
		return;
	}

	otos = ip->ip_tos;
	m_adj(m, off);

	if (m->m_len < sizeof(*ip6)) {
		m = m_pullup(m, sizeof(*ip6));
		if (!m)
			return;
	}
	ip6 = mtod(m, struct ip6_hdr *);

	/*
	 * perform sanity check against inner src/dst.
	 * for source, perform ingress filter as well.
	 */
	if (stf_checkaddr6(&ip6->ip6_dst, NULL) < 0 ||
	    stf_checkaddr6(&ip6->ip6_src, m->m_pkthdr.rcvif) < 0) {
		m_freem(m);
		return;
	}

	itos = (ntohl(ip6->ip6_flow) >> 20) & 0xff;
	if ((ifp->if_flags & IFF_LINK1) != 0)
		ip_ecn_egress(ECN_ALLOWED, &otos, &itos);
	ip6->ip6_flow &= ~htonl(0xff << 20);
	ip6->ip6_flow |= htonl((u_int32_t)itos << 20);

	m->m_pkthdr.rcvif = ifp;
	
#if NBPFILTER > 0
	if (ifp->if_bpf) {
		/*
		 * We need to prepend the address family as
		 * a four byte field.  Cons up a dummy header
		 * to pacify bpf.  This is safe because bpf
		 * will only read from the mbuf (i.e., it won't
		 * try to free it or keep a pointer a to it).
		 */
		struct mbuf m0;
		u_int af = AF_INET6;
		
		m0.m_next = m;
		m0.m_len = 4;
		m0.m_data = (char *)&af;
		
#ifdef HAVE_OLD_BPF
		bpf_mtap(ifp, &m0);
#else
		bpf_mtap(ifp->if_bpf, &m0);
#endif
	}
#endif /*NBPFILTER > 0*/

	/*
	 * Put the packet to the network layer input queue according to the
	 * specified address family.
	 * See net/if_gif.c for possible issues with packet processing
	 * reorder due to extra queueing.
	 */
	ifq = &ip6intrq;
	isr = NETISR_IPV6;

	s = splimp();
	if (IF_QFULL(ifq)) {
		IF_DROP(ifq);	/* update statistics */
		m_freem(m);
		splx(s);
		return;
	}
	IF_ENQUEUE(ifq, m);
	schednetisr(isr);
	ifp->if_ipackets++;
	ifp->if_ibytes += m->m_pkthdr.len;
	splx(s);
}

/* ARGSUSED */
static void
stf_rtrequest(cmd, rt, sa)
	int cmd;
	struct rtentry *rt;
#if defined(__bsdi__) && _BSDI_VERSION >= 199802
	struct rt_addrinfo *sa;
#else
	struct sockaddr *sa;
#endif
{

	if (rt)
		rt->rt_rmx.rmx_mtu = IPV6_MMTU;
}

static int
stf_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
#if defined(__FreeBSD__) && __FreeBSD__ < 3
	int cmd;
#else
	u_long cmd;
#endif
	caddr_t data;
{
	struct ifaddr *ifa;
	struct ifreq *ifr;
	struct sockaddr_in6 *sin6;
	int error;

	error = 0;
	switch (cmd) {
	case SIOCSIFADDR:
		ifa = (struct ifaddr *)data;
		if (ifa == NULL || ifa->ifa_addr->sa_family != AF_INET6) {
			error = EAFNOSUPPORT;
			break;
		}
		sin6 = (struct sockaddr_in6 *)ifa->ifa_addr;
		if (IN6_IS_ADDR_6TO4(&sin6->sin6_addr)) {
			ifa->ifa_rtrequest = stf_rtrequest;
			ifp->if_flags |= IFF_UP;
		} else
			error = EINVAL;
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		ifr = (struct ifreq *)data;
		if (ifr && ifr->ifr_addr.sa_family == AF_INET6)
			;
		else
			error = EAFNOSUPPORT;
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}

#endif /* NSTF > 0 */
