/*	$NetBSD: if_gre.c,v 1.1 1998/09/13 20:27:47 hwr Exp $ */
/*
 * (c) 1998 The NetBSD Foundation, Inc.
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
 */

#include "gre.h"
#if NGRE > 0

#include "opt_inet.h"
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

struct gre_softc gre_softc[NGRE];


void gre_compute_route(struct gre_softc *sc);
#ifdef DIAGNOSTIC
void my_inet_ntoa(struct in_addr in);
#endif

void greattach(void)
{

	register struct gre_softc *sc;
	register int i;

	i = 0 ;
	for (sc=gre_softc ; i < NGRE; sc++ ) {
		sprintf(sc->sc_if.if_xname, "gre%d", i++);
		sc->sc_if.if_softc = sc;
		sc->sc_if.if_type =  IFT_OTHER;
		sc->sc_if.if_addrlen = 4;
		sc->sc_if.if_hdrlen = 24; /* IP + GRE */
		sc->sc_if.if_mtu = GREMTU; 
		sc->sc_if.if_flags = IFF_POINTOPOINT|IFF_MULTICAST;
		sc->sc_if.if_output = gre_output;
		sc->sc_if.if_ioctl = gre_ioctl;
		sc->sc_if.if_collisions=0;
		sc->sc_if.if_ierrors=0;	
		sc->sc_if.if_oerrors=0;
		sc->sc_if.if_ipackets=0;
		sc->sc_if.if_opackets=0;
		sc->g_dst.s_addr=sc->g_src.s_addr=INADDR_ANY;
		sc->g_proto=IPPROTO_GRE;
		if_attach(&sc->sc_if);
#if 0
#if NBPFILTER > 0
		bpfattach(&sc->gre_bpf, &sc->sc_if, DLT_NULL, sizeof(u_int32_t) );
#endif
#endif

	}
}


/* 
 * The output routine. Takes a packet and encapsulates it in the protocol
 * given by sc->g_proto. See also RFC 1701 and RFC 2003
 */

struct ip ip_h;

int
gre_output(ifp, m, dst, rt)
        struct ifnet *ifp;
        register struct mbuf *m;
        struct sockaddr *dst;
        register struct rtentry *rt;
{
	int error=0;
	struct gre_softc *sc=(struct gre_softc *)(ifp->if_softc);
	struct greip *gh;
	struct ip *inp;
	u_char ttl;
	u_short etype;

	gh=NULL;
	inp=NULL;

#if 0
#if NBPFILTER >0

	if (sc->gre_bpf) {
		/* see comment of other if_foo.c files */
		struct mbuf m0;
		u_int af = dst->sa_family;

		m0.m_next =m;
		m0.m_len =4;
		m0.m_data = (char *)&af;
		
		bpf_mtap(ifp->if_bpf, &m0);
	}
#endif
#endif

	ttl=255; 

	if (sc->g_proto == IPPROTO_IPIP ) {
		if (dst->sa_family == AF_INET) {
			inp=mtod(m,struct ip *);
			ttl=inp->ip_ttl;
			etype=ETHERTYPE_IP;
			M_PREPEND(m,sizeof(struct ip),M_DONTWAIT);
		} else {
			IF_DROP(&ifp->if_snd);
			m_freem(m);
			return(EINVAL);
		}
	} else if (sc->g_proto == IPPROTO_GRE) {
		switch(dst->sa_family) {
		case AF_INET:
			inp=mtod(m,struct ip *);
			ttl=inp->ip_ttl;
			etype=ETHERTYPE_IP;
			break;
#ifdef NETATALK
		case AF_APPLETALK:
			etype=ETHERTYPE_ATALK;
			break;
#endif
#ifdef NS
		case AF_NS:
			etype=ETHERTYPE_NS;
			break;
#endif
		default:
			IF_DROP(&ifp->if_snd);
			m_freem(m);
			return(EAFNOSUPPORT);
		}
		M_PREPEND(m,sizeof(struct greip),M_DONTWAIT);
	} else {
		error= EINVAL;
		IF_DROP(&ifp->if_snd);
		m_freem(m);
		return(error);
	}
			

	if (m == NULL) {
		IF_DROP(&ifp->if_snd);
		return(ENOBUFS);
	}

	gh=mtod(m,struct greip *);
	if (sc->g_proto == IPPROTO_GRE){
		/* we don't have any GRE flags for now */

#if 0 		/* the world is not yet ready for this  as of 1.3.2 */
		memset((void*)&gh->gi_g,0, sizeof(struct gre_h));
#else
		bzero((void*)&gh->gi_g, sizeof(struct gre_h));
#endif
		gh->gi_ptype=htons(etype);
	}

	/* rest is same for GRE and IPIP and all inner protos */

	gh->gi_pr = sc->g_proto;
	gh->gi_src = sc->g_src;
	gh->gi_dst = sc->g_dst;
	((struct ip*)gh)->ip_hl = (sizeof(struct ip)) >> 2; 
	/* m->m_pkthdr.len is already augmented by
                         sizeof(struct greip) */
	gh->gi_len = m->m_pkthdr.len;
	((struct ip*)gh)->ip_ttl=ttl;
	((struct ip*)gh)->ip_tos=inp->ip_tos;

	ifp->if_opackets++;
	ifp->if_obytes+=m->m_pkthdr.len;
	/* send it off */
	error=ip_output(m,NULL,&sc->route,0,NULL);
	if (error) {
		ifp->if_oerrors++;
	}
	return(error);

}

int
gre_ioctl(ifp, cmd, data)
		struct ifnet *ifp;
		u_long cmd;
		caddr_t data;
{

	register struct ifaddr *ifa = (struct ifaddr *)data;
	register struct ifreq *ifr = (struct ifreq *)data;
	register struct in_ifaddr *ia = (struct in_ifaddr *)data;
	register struct gre_softc *sc = ifp->if_softc;
	int s;
	struct sockaddr_in si;
	struct sockaddr *sa =NULL;
	int error;

	error= 0;

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
		if ((sc->g_dst.s_addr== INADDR_ANY) || 
		    (sc->g_src.s_addr== INADDR_ANY))
			ifp->if_flags &= ~IFF_UP;

		if ((ifr->ifr_flags & IFF_LINK0) == IFF_LINK0) {  /* IPIP */
			sc->g_proto = IPPROTO_IPIP;
			ifp->if_flags |= IFF_LINK0;
			break;
			
		} else {					/* GRE */
			sc->g_proto = IPPROTO_GRE;
			ifp->if_flags &= ~IFF_LINK0;
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
		if (ifr == 0 ) {
			error = EAFNOSUPPORT;
			break;
		}
		switch(ifr->ifr_addr.sa_family) {
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
		case IPPROTO_IPIP :
			ifp->if_flags |= IFF_LINK1;
			ifp->if_flags &= ~IFF_LINK0;
			break;
		case IPPROTO_GRE :
			ifp->if_flags |= IFF_LINK0;
			ifp->if_flags &= ~IFF_LINK1;
			break;
		default:
			ifp->if_flags &= ~IFF_LINK0;
			ifp->if_flags &= ~IFF_LINK1;
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
		sa=sintosa(&si);
		ifr->ifr_addr = *sa;
		break;
	case GREGADDRD:
		si.sin_addr.s_addr = sc->g_dst.s_addr;
		sa=sintosa(&si);
		ifr->ifr_addr = *sa;
		break;
	default:
		error = EINVAL;
	}

	splx(s);
	return(error);
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

void gre_compute_route(struct gre_softc *sc)
{
	struct route *ro;
	u_int32_t a,b,c;

	ro=&sc->route;
	
#if 0	/* the world is not yet ready for this as of 1.3.2 */
	memset(ro,0,sizeof(struct route));
#else
	bzero(ro,sizeof(struct route));
#endif
	((struct sockaddr_in *)&ro->ro_dst)->sin_addr=sc->g_dst;
	ro->ro_dst.sa_family=AF_INET;
	ro->ro_dst.sa_len=sizeof(ro->ro_dst);
	/*
	 * toggle last bit, so our interface is not found, but a less
         * specific route. I'd rather like to specify a shorter mask,
 	 * but this is not possible. Should work though. XXX
	 * there is a simpler way ...
         */
	a= ntohl(sc->g_dst.s_addr);
	b= a & 0x01;
	c= a & 0xfffffffe;
	b = b ^ 0x01;
	a = b | c;
	((struct sockaddr_in *)&ro->ro_dst)->sin_addr.s_addr=htonl(a);

#ifdef DIAGNOSTIC
printf("%s: searching a route to ",sc->sc_if.if_xname);
my_inet_ntoa(((struct sockaddr_in *)&ro->ro_dst)->sin_addr);
#endif

	rtalloc(ro);

	/*
	 * now change it back - else ip_output will just drop 
         * the route and search one to this interface ...
         */
	((struct sockaddr_in *)&ro->ro_dst)->sin_addr=sc->g_dst;

#ifdef DIAGNOSTIC
printf(", choosing %s with gateway ",ro->ro_rt->rt_ifp->if_xname);
my_inet_ntoa(((struct sockaddr_in *)(ro->ro_rt->rt_gateway))->sin_addr);
printf("\n");
#endif
}

/* while testing ... */
#ifdef DIAGNOSTIC
void
my_inet_ntoa(in)
        struct in_addr in;
{
        register char *p;

        p = (char *)&in;
#define UC(b)   (((int)b)&0xff)
        printf("%d.%d.%d.%d", UC(p[0]), UC(p[1]), UC(p[2]), UC(p[3]));
}

#endif
#endif

