/*	$NetBSD: if_gre.c,v 1.8.4.1 1999/11/15 00:42:12 fvdl Exp $ */

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Heiko W.Rupp <hwr@pilhuhn.de>
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * Encapsulate L3 protocols into IP
 * See RFC 1701 and 1702 for more details.
 * If_gre is compatible with Cisco GRE tunnels, so you can
 * have a NetBSD box as the other end of a tunnel interface of a Cisco
 * router. See gre(4) for more details.
 * Also supported:  IP in IP encaps (proto 55) as of RFC 2004
 */

#include "gre.h"
#if NGRE > 0

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/dkstat.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>
#include <sys/file.h>
#include <sys/tty.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#if __NetBSD__
#include <sys/systm.h>
#endif

#include <machine/cpu.h>

#include <net/ethertypes.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/netisr.h>
#include <net/route.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#else
#error "Huh? if_gre without inet?"
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#ifdef NETATALK
#include <netatalk/at.h>
#include <netatalk/at_var.h>
#include <netatalk/at_extern.h>
#endif

#if NBPFILTER > 0
#include <sys/time.h>
#include <net/bpf.h>
#endif

#include <net/if_gre.h>

#define GREMTU 1450	/* XXX this is below the standard MTU of
                         1500 Bytes, allowing for headers, 
                         but we should possibly do path mtu discovery
                         before changing if state to up to find the 
                         correct value */
#define LINK_MASK (IFF_LINK0|IFF_LINK1|IFF_LINK2)

struct gre_softc gre_softc[NGRE];


void gre_compute_route(struct gre_softc *sc);
#ifdef DIAGNOSTIC
void gre_inet_ntoa(struct in_addr in);
#endif

void
greattach(void)
{
	struct gre_softc *sc;
	int i;

	i = 0 ;
	for (sc = gre_softc ; i < NGRE ; sc++ ) {
		sprintf(sc->sc_if.if_xname, "gre%d", i++);
		sc->sc_if.if_softc = sc;
		sc->sc_if.if_type =  IFT_OTHER;
		sc->sc_if.if_addrlen = 4;
		sc->sc_if.if_hdrlen = 24; /* IP + GRE */
		sc->sc_if.if_mtu = GREMTU; 
		sc->sc_if.if_flags = IFF_POINTOPOINT|IFF_MULTICAST;
		sc->sc_if.if_output = gre_output;
		sc->sc_if.if_ioctl = gre_ioctl;
		sc->sc_if.if_collisions = 0;
		sc->sc_if.if_ierrors = 0;
		sc->sc_if.if_oerrors = 0;
		sc->sc_if.if_ipackets = 0;
		sc->sc_if.if_opackets = 0;
		sc->g_dst.s_addr = sc->g_src.s_addr=INADDR_ANY;
		sc->g_proto = IPPROTO_GRE;
		if_attach(&sc->sc_if);
#if 0
#if NBPFILTER > 0
		bpfattach(&sc->gre_bpf, &sc->sc_if, DLT_RAW, sizeof(u_int32_t) );
#endif
#endif

	}
}


/* 
 * The output routine. Takes a packet and encapsulates it in the protocol
 * given by sc->g_proto. See also RFC 1701 and RFC 2004
 */

#if 0
struct ip ip_h;
#endif
struct mobile_h mob_h;

int
gre_output(struct ifnet *ifp, struct mbuf *m, struct sockaddr *dst,
	   struct rtentry *rt)
{
	int error = 0;
	struct gre_softc *sc = (struct gre_softc *)(ifp->if_softc);
	struct greip *gh;
	struct ip *inp;
	u_char ttl, osrc;
	u_short etype = 0;
	

	gh = NULL;
	inp = NULL;
	osrc = 0;

#if 0
#if NBPFILTER >0

	if (sc->gre_bpf) {
		/* see comment of other if_foo.c files */
		struct mbuf m0;
		u_int af = dst->sa_family;

		m0.m_next = m;
		m0.m_len = 4;
		m0.m_data = (char *)&af;
		
		bpf_mtap(ifp->if_bpf, &m0);
	}
#endif
#endif

	ttl = 255;

	if (sc->g_proto == IPPROTO_MOBILE) {
		if (dst->sa_family == AF_INET) {
			struct mbuf *m0;
			int msiz;

			inp = mtod(m, struct ip *);

			memset(&mob_h, 0, MOB_H_SIZ_L);
			mob_h.proto = (inp->ip_p) << 8;
			mob_h.odst = inp->ip_dst.s_addr;
			inp->ip_dst.s_addr = sc->g_dst.s_addr;

			/*
			 * If the packet comes from our host, we only change
			 * the destination address in the IP header.
			 * Else we also need to save and change the source
			 */
			if (in_hosteq(inp->ip_src, sc->g_src)) {
				msiz = MOB_H_SIZ_S;
			} else {
				mob_h.proto |= MOB_H_SBIT;
				mob_h.osrc = inp->ip_src.s_addr;
				inp->ip_src.s_addr = sc->g_src.s_addr;
				msiz = MOB_H_SIZ_L;
			}
			HTONS(mob_h.proto);
			mob_h.hcrc = gre_in_cksum((u_short *)&mob_h, msiz);

			if ((m->m_data - msiz) < m->m_pktdat) {
				/* need new mbuf */
				MGETHDR(m0, M_DONTWAIT, MT_HEADER);
				if (m0 == NULL) {
					IF_DROP(&ifp->if_snd);
					m_freem(m);
					return (ENOBUFS);
				}
				m0->m_next = m;
				m->m_data += sizeof(struct ip);
				m->m_len -= sizeof(struct ip);
				m0->m_pkthdr.len = m->m_pkthdr.len + msiz;
				m0->m_len = msiz + sizeof(struct ip);
				m0->m_data += max_linkhdr;
				memcpy(mtod(m0, caddr_t), (caddr_t)inp,
				       sizeof(struct ip));
				m = m0;
			} else {  /* we have some spave left in the old one */
				m->m_data -= msiz;
				m->m_len += msiz;
				m->m_pkthdr.len += msiz;
				memmove(mtod(m, caddr_t), inp,
					sizeof(struct ip));
			}
			inp=mtod(m, struct ip *);
			memcpy((caddr_t)(inp + 1), &mob_h, (unsigned)msiz);
			NTOHS(inp->ip_len);
			inp->ip_len += msiz;
		} else {  /* AF_INET */
			IF_DROP(&ifp->if_snd);
			m_freem(m);
			return (EINVAL);
		}
	} else if (sc->g_proto == IPPROTO_GRE) {
		switch(dst->sa_family) {
		case AF_INET:
			inp = mtod(m, struct ip *);
			ttl = inp->ip_ttl;
			etype = ETHERTYPE_IP;
			break;
#ifdef NETATALK
		case AF_APPLETALK:
			etype = ETHERTYPE_ATALK;
			break;
#endif
#ifdef NS
		case AF_NS:
			etype = ETHERTYPE_NS;
			break;
#endif
		default:
			IF_DROP(&ifp->if_snd);
			m_freem(m);
			return (EAFNOSUPPORT);
		}
		M_PREPEND(m, sizeof(struct greip), M_DONTWAIT);
	} else {
		error = EINVAL;
		IF_DROP(&ifp->if_snd);
		m_freem(m);
		return (error);
	}
			

	if (m == NULL) {
		IF_DROP(&ifp->if_snd);
		return (ENOBUFS);
	}

	gh = mtod(m, struct greip *);
	if (sc->g_proto == IPPROTO_GRE) {
		/* we don't have any GRE flags for now */

		memset((void *)&gh->gi_g, 0, sizeof(struct gre_h));
		gh->gi_ptype = htons(etype);
	}

	gh->gi_pr = sc->g_proto;
	if (sc->g_proto != IPPROTO_MOBILE) {
		gh->gi_src = sc->g_src;
		gh->gi_dst = sc->g_dst;
		((struct ip*)gh)->ip_hl = (sizeof(struct ip)) >> 2; 
		((struct ip*)gh)->ip_ttl = ttl;
		((struct ip*)gh)->ip_tos = inp->ip_tos;
	    gh->gi_len = m->m_pkthdr.len;
	}

	ifp->if_opackets++;
	ifp->if_obytes += m->m_pkthdr.len;
	/* send it off */
	error = ip_output(m, NULL, &sc->route, 0, NULL);
	if (error)
		ifp->if_oerrors++;
	return (error);

}

int
gre_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{

	struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data;
	struct in_ifaddr *ia = (struct in_ifaddr *)data;
	struct gre_softc *sc = ifp->if_softc;
	int s;
	struct sockaddr_in si;
	struct sockaddr *sa = NULL;
	int error;

	error = 0;

	s = splimp();
	switch(cmd) {
	case SIOCSIFADDR:		
	case SIOCSIFDSTADDR: 	
		/* 
                 * set tunnel endpoints in case that we "only"
                 * have ip over ip encapsulation. This allows to
                 * set tunnel endpoints with ifconfig.
                 */
		if (ifa->ifa_addr->sa_family == AF_INET) {
			sa = ifa->ifa_addr;
			sc->g_src = (satosin(sa))->sin_addr;
			sc->g_dst = ia->ia_dstaddr.sin_addr;
			if ((sc->g_src.s_addr != INADDR_ANY) &&
			    (sc->g_dst.s_addr != INADDR_ANY)) {
				if (sc->route.ro_rt != 0) /* free old route */
					RTFREE(sc->route.ro_rt);
				gre_compute_route(sc);
				ifp->if_flags |= IFF_UP;
			}
		}
		break;
	case SIOCSIFFLAGS:
		if ((sc->g_dst.s_addr == INADDR_ANY) || 
		    (sc->g_src.s_addr == INADDR_ANY))
			ifp->if_flags &= ~IFF_UP;

		switch(ifr->ifr_flags & LINK_MASK) {
			case IFF_LINK0:
				sc->g_proto = IPPROTO_GRE;
				ifp->if_flags |= IFF_LINK0;
				ifp->if_flags &= ~(IFF_LINK1|IFF_LINK2);
				break;
			case IFF_LINK2:
				sc->g_proto = IPPROTO_MOBILE;
				ifp->if_flags |= IFF_LINK2;
				ifp->if_flags &= ~(IFF_LINK0|IFF_LINK1);
				break;
		}
		break;
	case SIOCSIFMTU: 
		if (ifr->ifr_mtu > GREMTU || ifr->ifr_mtu < 576) {
			error = EINVAL;
			break;
		}
		ifp->if_mtu = ifr->ifr_mtu;
		break;
	case SIOCGIFMTU:
		ifr->ifr_mtu = sc->sc_if.if_mtu;
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if (ifr == 0) {
			error = EAFNOSUPPORT;
			break;
		}
		switch (ifr->ifr_addr.sa_family) {
#ifdef INET
		case AF_INET:
			break;
#endif
		default:
			error = EAFNOSUPPORT;
			break;
		}
		break;
	case GRESPROTO:
		sc->g_proto = ifr->ifr_flags;
		switch (sc->g_proto) {
		case IPPROTO_GRE :
			ifp->if_flags |= IFF_LINK0;
			ifp->if_flags &= ~(IFF_LINK1|IFF_LINK2);
			break;
		case IPPROTO_MOBILE :
			ifp->if_flags |= IFF_LINK2;
			ifp->if_flags &= ~(IFF_LINK1|IFF_LINK2);
			break;
		default:
			ifp->if_flags &= ~(IFF_LINK0|IFF_LINK1|IFF_LINK2);
		}
		break;
	case GREGPROTO:
		ifr->ifr_flags = sc->g_proto;
		break;
	case GRESADDRS:
	case GRESADDRD:
		/*
	         * set tunnel endpoints, compute a less specific route
	         * to the remote end and mark if as up
                 */
		sa = &ifr->ifr_addr;
		if (cmd == GRESADDRS )
			sc->g_src = (satosin(sa))->sin_addr;
		if (cmd == GRESADDRD )
			sc->g_dst = (satosin(sa))->sin_addr;
		if ((sc->g_src.s_addr != INADDR_ANY) &&
		    (sc->g_dst.s_addr != INADDR_ANY)) {
			if (sc->route.ro_rt != 0) /* free old route */
				RTFREE(sc->route.ro_rt);
			gre_compute_route(sc);
			ifp->if_flags |= IFF_UP;
		}
		break;
	case GREGADDRS:
		si.sin_addr.s_addr = sc->g_src.s_addr;
		sa = sintosa(&si);
		ifr->ifr_addr = *sa;
		break;
	case GREGADDRD:
		si.sin_addr.s_addr = sc->g_dst.s_addr;
		sa = sintosa(&si);
		ifr->ifr_addr = *sa;
		break;
	default:
		error = EINVAL;
	}

	splx(s);
	return (error);
}

/* 
 * computes a route to our destination that is not the one
 * which would be taken by ip_output(), as this one will loop back to
 * us. If the interface is p2p as  a--->b, then a routing entry exists
 * If we now send a packet to b (e.g. ping b), this will come down here
 * gets src=a, dst=b tacked on and would from ip_ouput() sent back to 
 * if_gre.
 * Goal here is to compute a route to b that is less specific than
 * a-->b. We know that this one exists as in normal operation we have
 * at least a default route which matches.
 */

void
gre_compute_route(struct gre_softc *sc)
{
	struct route *ro;
	u_int32_t a, b, c;

	ro = &sc->route;
	
	memset(ro, 0, sizeof(struct route));
	((struct sockaddr_in *)&ro->ro_dst)->sin_addr = sc->g_dst;
	ro->ro_dst.sa_family = AF_INET;
	ro->ro_dst.sa_len = sizeof(ro->ro_dst);

	/*
	 * toggle last bit, so our interface is not found, but a less
         * specific route. I'd rather like to specify a shorter mask,
 	 * but this is not possible. Should work though. XXX
	 * there is a simpler way ...
         */
	if ((sc->sc_if.if_flags & IFF_LINK1) == 0) {
		a = ntohl(sc->g_dst.s_addr);
		b = a & 0x01;
		c = a & 0xfffffffe;
		b = b ^ 0x01;
		a = b | c;
		((struct sockaddr_in *)&ro->ro_dst)->sin_addr.s_addr
			= htonl(a);
	}

#ifdef DIAGNOSTIC
	printf("%s: searching a route to ", sc->sc_if.if_xname);
	gre_inet_ntoa(((struct sockaddr_in *)&ro->ro_dst)->sin_addr);
#endif

	rtalloc(ro);

	/*
	 * now change it back - else ip_output will just drop 
         * the route and search one to this interface ...
         */
	if ((sc->sc_if.if_flags & IFF_LINK1) == 0)
		((struct sockaddr_in *)&ro->ro_dst)->sin_addr = sc->g_dst;

#ifdef DIAGNOSTIC
	printf(", choosing %s with gateway ",ro->ro_rt->rt_ifp->if_xname);
	gre_inet_ntoa(((struct sockaddr_in *)(ro->ro_rt->rt_gateway))->sin_addr);
	printf("\n");
#endif
}

/*
 * do a checksum of a buffer - much like in_cksum, which operates on  
 * mbufs. 
 */

u_short
gre_in_cksum(u_short *p, u_int len)
{
	u_int sum = 0; 
	int nwords = len >> 1;
  
	while (nwords-- != 0)
		sum += *p++;
  
		if (len & 1) {
			union {
				u_short w;
				u_char c[2]; 
			} u;
			u.c[0] = *(u_char *)p;
			u.c[1] = 0;
			sum += u.w;
		} 
 
		/* end-around-carry */
		sum = (sum >> 16) + (sum & 0xffff);
		sum += (sum >> 16);
		return (~sum);
}


/* while testing ... */
#ifdef DIAGNOSTIC
void
gre_inet_ntoa(struct in_addr in)
{
	char *p;

	p = (char *)&in;
#define UC(b)   (((int)b)&0xff)
	printf("%d.%d.%d.%d", UC(p[0]), UC(p[1]), UC(p[2]), UC(p[3]));
}

#endif
#endif

