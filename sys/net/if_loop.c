/*	$NetBSD: if_loop.c,v 1.39 2001/06/14 05:44:24 itojun Exp $	*/

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
 * Copyright (c) 1982, 1986, 1993
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
 *	@(#)if_loop.c	8.2 (Berkeley) 1/9/95
 */

/*
 * Loopback interface driver for protocol testing and timing.
 */

#include "opt_inet.h"
#include "opt_atalk.h"
#include "opt_iso.h"
#include "opt_ns.h"

#include "bpfilter.h"
#include "loop.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <machine/cpu.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/netisr.h>
#include <net/route.h>

#ifdef	INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#endif

#ifdef INET6
#ifndef INET
#include <netinet/in.h>
#endif
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#ifdef IPX
#include <netipx/ipx.h>
#include <netipx/ipx_if.h>
#endif

#ifdef ISO
#include <netiso/iso.h>
#include <netiso/iso_var.h>
#endif

#ifdef NETATALK
#include <netatalk/at.h>
#include <netatalk/at_var.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#if defined(LARGE_LOMTU)
#define LOMTU	(131072 +  MHLEN + MLEN)
#else
#define	LOMTU	(32768 +  MHLEN + MLEN)
#endif

struct	ifnet loif[NLOOP];

#ifdef ALTQ
void	lostart(struct ifnet *);
#endif

void
loopattach(n)
	int n;
{
	int i;
	struct ifnet *ifp;

	for (i = 0; i < NLOOP; i++) {
		ifp = &loif[i];
		sprintf(ifp->if_xname, "lo%d", i);
		ifp->if_softc = NULL;
		ifp->if_mtu = LOMTU;
		ifp->if_flags = IFF_LOOPBACK | IFF_MULTICAST;
		ifp->if_ioctl = loioctl;
		ifp->if_output = looutput;
#ifdef ALTQ
		ifp->if_start = lostart;
#endif
		ifp->if_type = IFT_LOOP;
		ifp->if_hdrlen = 0;
		ifp->if_addrlen = 0;
		ifp->if_dlt = DLT_NULL;
		IFQ_SET_READY(&ifp->if_snd);
		if_attach(ifp);
		if_alloc_sadl(ifp);
#if NBPFILTER > 0
		bpfattach(ifp, DLT_NULL, sizeof(u_int));
#endif
	}
}

int
looutput(ifp, m, dst, rt)
	struct ifnet *ifp;
	struct mbuf *m;
	struct sockaddr *dst;
	struct rtentry *rt;
{
	int s, isr;
	struct ifqueue *ifq = 0;

	if ((m->m_flags & M_PKTHDR) == 0)
		panic("looutput: no header mbuf");
#if NBPFILTER > 0
	if (ifp->if_bpf && (ifp->if_flags & IFF_LOOPBACK)) {
		/*
		 * We need to prepend the address family as
		 * a four byte field.  Cons up a dummy header
		 * to pacify bpf.  This is safe because bpf
		 * will only read from the mbuf (i.e., it won't
		 * try to free it or keep a pointer to it).
		 */
		struct mbuf m0;
		u_int32_t af = dst->sa_family;

		m0.m_next = m;
		m0.m_len = 4;
		m0.m_data = (char *)&af;

		bpf_mtap(ifp->if_bpf, &m0);
	}
#endif
	m->m_pkthdr.rcvif = ifp;

	if (rt && rt->rt_flags & (RTF_REJECT|RTF_BLACKHOLE)) {
		m_freem(m);
		return (rt->rt_flags & RTF_BLACKHOLE ? 0 :
			rt->rt_flags & RTF_HOST ? EHOSTUNREACH : ENETUNREACH);
	}

#ifndef PULLDOWN_TEST
	/*
	 * KAME requires that the packet to be contiguous on the
	 * mbuf.  We need to make that sure.
	 * this kind of code should be avoided.
	 * XXX other conditions to avoid running this part?
	 */
	if (m->m_len != m->m_pkthdr.len) {
		struct mbuf *n = NULL;
		int maxlen;

		MGETHDR(n, M_DONTWAIT, MT_HEADER);
		maxlen = MHLEN;
		if (n)
			M_COPY_PKTHDR(n, m);
		if (n && m->m_pkthdr.len > maxlen) {
			MCLGET(n, M_DONTWAIT);
			maxlen = MCLBYTES;
			if ((n->m_flags & M_EXT) == 0) {
				m_free(n);
				n = NULL;
			}
		}
		if (!n) {
			printf("looutput: mbuf allocation failed\n");
			m_freem(m);
			return ENOBUFS;
		}

		if (m->m_pkthdr.len <= maxlen) {
			m_copydata(m, 0, m->m_pkthdr.len, mtod(n, caddr_t));
			n->m_len = m->m_pkthdr.len;
			n->m_next = NULL;
			m_freem(m);
		} else {
			m_copydata(m, 0, maxlen, mtod(n, caddr_t));
			m_adj(m, maxlen);
			n->m_len = maxlen;
			n->m_next = m;
			m->m_flags &= ~M_PKTHDR;
		}
		m = n;
	}
#if 0
	if (m && m->m_next != NULL) {
		printf("loop: not contiguous...\n");
		m_freem(m);
		return ENOBUFS;
	}
#endif
#endif

	ifp->if_opackets++;
	ifp->if_obytes += m->m_pkthdr.len;

#ifdef ALTQ
	/*
	 * ALTQ on the loopback interface is just for debugging.  It's
	 * used only for loopback interfaces, not for a simplex interface.
	 */
	if ((ALTQ_IS_ENABLED(&ifp->if_snd) || TBR_IS_ENABLED(&ifp->if_snd)) &&
	    ifp->if_start == lostart) {
		struct altq_pktattr pktattr;
		int error;

		/*
		 * If the queueing discipline needs packet classification,
		 * do it before prepending the link headers.
		 */
		IFQ_CLASSIFY(&ifp->if_snd, m, dst->sa_family, &pktattr);

		M_PREPEND(m, sizeof(uint32_t), M_DONTWAIT);
		if (m == NULL)
			return (ENOBUFS);
		*(mtod(m, uint32_t *)) = dst->sa_family;

		s = splnet();
		IFQ_ENQUEUE(&ifp->if_snd, m, &pktattr, error);
		(*ifp->if_start)(ifp);
		splx(s);
		return (error);
	}
#endif /* ALTQ */

	switch (dst->sa_family) {

#ifdef INET
	case AF_INET:
		ifq = &ipintrq;
		isr = NETISR_IP;
		break;
#endif
#ifdef INET6
	case AF_INET6:
		m->m_flags |= M_LOOP;
		ifq = &ip6intrq;
		isr = NETISR_IPV6;
		break;
#endif
#ifdef NS
	case AF_NS:
		ifq = &nsintrq;
		isr = NETISR_NS;
		break;
#endif
#ifdef ISO
	case AF_ISO:
		ifq = &clnlintrq;
		isr = NETISR_ISO;
		break;
#endif
#ifdef IPX
	case AF_IPX:
		ifq = &ipxintrq;
		isr = NETISR_IPX;
		break;
#endif
#ifdef NETATALK
	case AF_APPLETALK:
	        ifq = &atintrq2;
		isr = NETISR_ATALK;
		break;
#endif
	default:
		printf("%s: can't handle af%d\n", ifp->if_xname,
		    dst->sa_family);
		m_freem(m);
		return (EAFNOSUPPORT);
	}
	s = splnet();
	if (IF_QFULL(ifq)) {
		IF_DROP(ifq);
		m_freem(m);
		splx(s);
		return (ENOBUFS);
	}
	IF_ENQUEUE(ifq, m);
	schednetisr(isr);
	ifp->if_ipackets++;
	ifp->if_ibytes += m->m_pkthdr.len;
	splx(s);
	return (0);
}

#ifdef ALTQ
void
lostart(struct ifnet *ifp)
{
	struct ifqueue *ifq;
	struct mbuf *m;
	uint32_t af;
	int s, isr;

	for (;;) {
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			return;

		af = *(mtod(m, uint32_t *));
		m_adj(m, sizeof(uint32_t));

		switch (af) {
#ifdef INET
		case AF_INET:
			ifq = &ipintrq;
			isr = NETISR_IP;
			break;
#endif
#ifdef INET6
		case AF_INET6:
			m->m_flags |= M_LOOP;
			ifq = &ip6intrq;
			isr = NETISR_IPV6;
			break;
#endif
#ifdef IPX
		case AF_IPX:
			ifq = &ipxintrq;
			isr = NETISR_IPX;
			break;
#endif
#ifdef NS
		case AF_NS:
			ifq = &nsintrq;
			isr = NETISR_NS;
			break;
#endif
#ifdef ISO
		case AF_ISO:
			ifq = &clnlintrq;
			isr = NETISR_ISO;
			break;
#endif
#ifdef NETATALK
		case AF_APPLETALK:
			ifq = &atintrq2;
			isr = NETISR_ATALK;
			break;
#endif
		default:
			printf("%s: can't handle af%d\n", ifp->if_xname, af);
			m_freem(m);
			return;
		}

		s = splnet();
		if (IF_QFULL(ifq)) {
			IF_DROP(ifq);
			splx(s);
			m_freem(m);
			return;
		}
		IF_ENQUEUE(ifq, m);
		schednetisr(isr);
		ifp->if_ipackets++;
		ifp->if_ibytes += m->m_pkthdr.len;
		splx(s);
	}
}
#endif /* ALTQ */

/* ARGSUSED */
void
lortrequest(cmd, rt, info)
	int cmd;
	struct rtentry *rt;
	struct rt_addrinfo *info;
{

	if (rt)
		rt->rt_rmx.rmx_mtu = LOMTU;
}

/*
 * Process an ioctl request.
 */
/* ARGSUSED */
int
loioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct ifaddr *ifa;
	struct ifreq *ifr;
	int error = 0;

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		ifa = (struct ifaddr *)data;
		if (ifa != 0 /*&& ifa->ifa_addr->sa_family == AF_ISO*/)
			ifa->ifa_rtrequest = lortrequest;
		/*
		 * Everything else is done at a higher level.
		 */
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		ifr = (struct ifreq *)data;
		if (ifr == 0) {
			error = EAFNOSUPPORT;		/* XXX */
			break;
		}
		switch (ifr->ifr_addr.sa_family) {

#ifdef INET
		case AF_INET:
			break;
#endif
#ifdef INET6
		case AF_INET6:
			break;
#endif

		default:
			error = EAFNOSUPPORT;
			break;
		}
		break;

	default:
		error = EINVAL;
	}
	return (error);
}
