/*	$NetBSD: ip_ipip.c,v 1.7.2.4 2001/01/18 09:23:56 bouyer Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center; Heiko W.Rupp <hwr@pilhuhn.de>.
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
 * Encapsulate for and decapsulate packets received on an IP-in-IP tunnel.
 *
 * See: RFC 2003.
 */

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_types.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_ipip.h>
#include <netinet/ip_encap.h>
#else
#error IPIP without INET?
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <machine/stdarg.h>

struct ipip_softc *ipip_softc;
int ipip_nsoftc;

static int ipip_encapcheck __P((const struct mbuf *, int, int, void *));
void	ipipattach __P((int));
int	ipip_output __P((struct ifnet *, struct mbuf *, struct sockaddr *,
	    struct rtentry *));
int	ipip_ioctl __P((struct ifnet *, u_long, caddr_t));
void	ipip_compute_route __P((struct ipip_softc *));

extern struct protosw ipip_protosw;

/*
 * Note this is a common IP-in-IP input for both ipip `interfaces'
 * and the multicast routing code's IP-in-IP needs.
 */
void
#if __STDC__
ipip_input(struct mbuf *m, ...)
#else
ipip_input(m, va_alist)
	sruct mbuf *m;
	va_dcl
#endif
{
	struct ipip_softc *sc;
	int hlen, proto;
	va_list ap;

	va_start(ap, m);
	hlen = va_arg(ap, int);
	proto = va_arg(ap, int);
	va_end(ap);

	sc = (struct ipip_softc *)encap_getarg(m);
	if (sc != NULL) {
		int s;

		sc->sc_if.if_ipackets++;
		sc->sc_if.if_ibytes += m->m_pkthdr.len;

		m_adj(m, hlen);
		m->m_pkthdr.rcvif = &sc->sc_if;

		s = splimp();
		if (IF_QFULL(&ipintrq)) {
			IF_DROP(&ipintrq);
			m_freem(m);
		} else {
			IF_ENQUEUE(&ipintrq, m);
			/*
			 * No need to reschedule an IP netisr, since
			 * ipintr() will simply continue looping and
			 * see the packet we just enqueued.
			 */
		}
		splx(s);

		/* No more processing at this level. */
		return;
	}

	m_freem(m);
}

/*
 * Find the ipip interface associated with our src/dst.
 */
static int
ipip_encapcheck(m, off, proto, arg)
	const struct mbuf *m;
	int off;
	int proto;
	void *arg;
{
	struct ipip_softc *sc;
	struct ip ip;

	sc = (struct ipip_softc *)arg;
	if (sc == NULL)
		return 0;
	if ((sc->sc_if.if_flags & IFF_UP) == 0)
		return 0;

	/* LINTED const cast */
	m_copydata((struct mbuf *)m, 0, sizeof(ip), (caddr_t)&ip);

	if (!in_hosteq(sc->sc_dst, ip.ip_src) ||
	    !in_hosteq(sc->sc_src, ip.ip_dst))
		return 0;

	/* both address matched */
	return 32 * 2;
}

/*
 * Attach the IPIP pseudo-devices.
 */
/* ARGSUSED */
void
ipipattach(count)
	int count;
{
	struct ipip_softc *sc;
	size_t size;
	int i;

	ipip_nsoftc = count;
	size = sizeof(struct ipip_softc) * count;
	ipip_softc = malloc(size, M_DEVBUF, M_WAITOK);
	memset(ipip_softc, 0, size);

	for (i = 0; i < ipip_nsoftc; i++) {
		sc = &ipip_softc[i];

		sprintf(sc->sc_if.if_xname, "ipip%d", i);

		sc->sc_cookie = encap_attach_func(AF_INET, IPPROTO_IPIP,
		    ipip_encapcheck, &ipip_protosw, sc);
		if (sc->sc_cookie == NULL) {
			printf("%s: attach failed\n", sc->sc_if.if_xname);
			continue;
		}

		sc->sc_if.if_softc = sc;
		sc->sc_if.if_type = IFT_OTHER;
		sc->sc_if.if_dlt = DLT_NULL;
		sc->sc_if.if_addrlen = sizeof(struct in_addr);
		sc->sc_if.if_hdrlen = sizeof(struct ip);
		sc->sc_if.if_mtu = 0;	/* filled in later */
		sc->sc_if.if_flags = IFF_POINTOPOINT | IFF_MULTICAST;
		sc->sc_if.if_output = ipip_output;
		sc->sc_if.if_ioctl = ipip_ioctl;

		sc->sc_src.s_addr = INADDR_ANY;
		sc->sc_dst.s_addr = INADDR_ANY;

		if_attach(&sc->sc_if);
		if_alloc_sadl(&sc->sc_if);
#if NBPFILTER > 0
		bpfattach(&sc->sc_if, DLT_NULL, sc->sc_if.if_hdrlen);
#endif
	}
}

int
ipip_output(ifp, m0, dst, rt)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
	struct rtentry *rt;
{
	struct ipip_softc *sc = ifp->if_softc;
	struct ip *nip, *ip;
	int error;

	if (dst->sa_family != AF_INET) {
		IF_DROP(&ifp->if_snd);
		m_freem(m0);
		return (EAFNOSUPPORT);
	}

	ip = mtod(m0, struct ip *);

	/* Add the new IP header. */
	M_PREPEND(m0, sizeof(struct ip), M_DONTWAIT);
	if (m0 == NULL)
		return (ENOBUFS);

	nip = mtod(m0, struct ip *);
	memset(nip, 0, sizeof(*nip));

	nip->ip_hl = sizeof(*nip) >> 2;
	nip->ip_tos = ip->ip_tos;
	nip->ip_len = m0->m_pkthdr.len;
	nip->ip_ttl = ip->ip_ttl;
	nip->ip_p = IPPROTO_IPIP;
	nip->ip_src = sc->sc_src;
	nip->ip_dst = sc->sc_dst;

#if NBPFILTER > 0
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m0);
#endif

	ifp->if_opackets++;
	ifp->if_obytes += m0->m_pkthdr.len;

	error = ip_output(m0, NULL, &sc->sc_route, 0, NULL);
	if (error)
		ifp->if_oerrors++;

	return (error);
}

int
ipip_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct ipip_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct in_ifaddr *ia = (struct in_ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFADDR:
	case SIOCSIFDSTADDR:
		if (ifa->ifa_addr->sa_family != AF_INET) {
			error = EAFNOSUPPORT;
			break;
		}

		sc->sc_src = (satosin(ifa->ifa_addr))->sin_addr;
		sc->sc_dst = ia->ia_dstaddr.sin_addr;
		ifp->if_mtu = 0;
		ifp->if_flags &= ~IFF_UP;

		if (!in_nullhost(sc->sc_src) && !in_nullhost(sc->sc_dst)) {
			struct rtentry *rt;

			ipip_compute_route(sc);
			/*
			 * Now that we know the route to use, fill in the
			 * MTU.
			 */
			rt = sc->sc_route.ro_rt;
			if (rt != NULL) {
				ifp->if_mtu = rt->rt_ifp->if_mtu -
					      ifp->if_hdrlen;
				ifp->if_flags |= IFF_UP;
			}
		}
		break;

	case SIOCSIFFLAGS:
		if (in_nullhost(sc->sc_src) || in_nullhost(sc->sc_dst))
			ifp->if_flags &= ~IFF_UP;

		if (ifp->if_flags & IFF_UP) {
			if (sc->sc_route.ro_rt == NULL) {
				struct rtentry *rt;

				ipip_compute_route(sc);
				rt = sc->sc_route.ro_rt;
				if (rt != NULL)
					ifp->if_mtu = rt->rt_ifp->if_mtu -
						      ifp->if_hdrlen;
				else
					ifp->if_flags &= ~IFF_UP;
			}
		}
		else {
			if (sc->sc_route.ro_rt != NULL) {
				RTFREE(sc->sc_route.ro_rt);
				sc->sc_route.ro_rt = NULL;
			}
			ifp->if_mtu = 0;
		}
		break;

	case SIOCSIFMTU:
		if (ifr->ifr_mtu < (IP_MSS - ifp->if_hdrlen)) {
			error = EINVAL;
			break;
		}
		if (sc->sc_route.ro_rt != NULL &&
		    ifr->ifr_mtu > (sc->sc_route.ro_rt->rt_ifp->if_mtu -
				    ifp->if_hdrlen)) {
			error = EINVAL;
			break;
		}
		ifp->if_mtu = ifr->ifr_mtu;
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if (ifr == NULL || ifr->ifr_addr.sa_family != AF_INET) {
			error = EAFNOSUPPORT;
			break;
		}
		break;

	default:
		error = EINVAL;
	}

	splx(s);
	return (error);
}

/*
 * Compute a less-specific route to the destination such that ip_output()
 * will not loop the packet back to ipip_output().
 */
void
ipip_compute_route(sc)
	struct ipip_softc *sc;
{
	struct route *ro;

	ro = &sc->sc_route;

	if (ro->ro_rt != NULL)
		RTFREE(ro->ro_rt);
	ro->ro_rt = NULL;
	memset(&ro->ro_dst, 0, sizeof(ro->ro_dst));

	ro->ro_dst.sa_family = AF_INET;
	ro->ro_dst.sa_len = sizeof(ro->ro_dst);

	/*
	 * Toggle the last bit on the address so our interface won't
	 * match.
	 *
	 * XXX What we want here is a flag to exclude picking routes
	 * XXX with this interface.
	 */
	((struct sockaddr_in *)&ro->ro_dst)->sin_addr.s_addr =
	    htonl(ntohl(sc->sc_dst.s_addr) ^ 1);

	rtalloc(ro);

	/*
	 * Now change it back, or else ip_ouput() will just drop
	 * the route and find the one for the ipip interface.
	 */
	((struct sockaddr_in *)&ro->ro_dst)->sin_addr = sc->sc_dst;
}
