/*	$NetBSD: if_loop.c,v 1.62.4.1 2007/02/27 16:54:43 yamt Exp $	*/

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
 *	@(#)if_loop.c	8.2 (Berkeley) 1/9/95
 */

/*
 * Loopback interface driver for protocol testing and timing.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_loop.c,v 1.62.4.1 2007/02/27 16:54:43 yamt Exp $");

#include "opt_inet.h"
#include "opt_atalk.h"
#include "opt_iso.h"
#include "opt_ipx.h"
#include "opt_mbuftrace.h"

#include "bpfilter.h"

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
#define LOMTU_MAX LOMTU
#else
#define	LOMTU	(32768 +  MHLEN + MLEN)
#define	LOMTU_MAX	(65536 +  MHLEN + MLEN)
#endif

#ifdef ALTQ
static void	lostart(struct ifnet *);
#endif

static int	loop_clone_create(struct if_clone *, int);
static int	loop_clone_destroy(struct ifnet *);

static struct if_clone loop_cloner =
    IF_CLONE_INITIALIZER("lo", loop_clone_create, loop_clone_destroy);

void
loopattach(int n)
{

	(void)loop_clone_create(&loop_cloner, 0);	/* lo0 always exists */
	if_clone_attach(&loop_cloner);
}

static int
loop_clone_create(struct if_clone *ifc, int unit)
{
	struct ifnet *ifp;

	ifp = malloc(sizeof(*ifp), M_DEVBUF, M_WAITOK | M_ZERO);

	snprintf(ifp->if_xname, sizeof(ifp->if_xname), "%s%d",
	    ifc->ifc_name, unit);

	ifp->if_mtu = LOMTU;
	ifp->if_flags = IFF_LOOPBACK | IFF_MULTICAST | IFF_RUNNING;
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
	if (unit == 0)
		lo0ifp = ifp;
	if_attach(ifp);
	if_alloc_sadl(ifp);
#if NBPFILTER > 0
	bpfattach(ifp, DLT_NULL, sizeof(u_int));
#endif
#ifdef MBUFTRACE
	ifp->if_mowner = malloc(sizeof(struct mowner), M_DEVBUF,
	    M_WAITOK | M_ZERO);
	strlcpy(ifp->if_mowner->mo_name, ifp->if_xname,
	    sizeof(ifp->if_mowner->mo_name));
	MOWNER_ATTACH(ifp->if_mowner);
#endif

	return (0);
}

static int
loop_clone_destroy(struct ifnet *ifp)
{

	if (ifp == lo0ifp)
		return (EPERM);

#ifdef MBUFTRACE
	MOWNER_DETACH(ifp->if_mowner);
	free(ifp->if_mowner, M_DEVBUF);
#endif

#if NBPFILTER > 0
	bpfdetach(ifp);
#endif
	if_detach(ifp);

	free(ifp, M_DEVBUF);

	return (0);
}

int
looutput(struct ifnet *ifp, struct mbuf *m, const struct sockaddr *dst,
    struct rtentry *rt)
{
	int s, isr;
	struct ifqueue *ifq = NULL;

	MCLAIM(m, ifp->if_mowner);
	if ((m->m_flags & M_PKTHDR) == 0)
		panic("looutput: no header mbuf");
#if NBPFILTER > 0
	if (ifp->if_bpf && (ifp->if_flags & IFF_LOOPBACK))
		bpf_mtap_af(ifp->if_bpf, dst->sa_family, m);
#endif
	m->m_pkthdr.rcvif = ifp;

	if (rt && rt->rt_flags & (RTF_REJECT|RTF_BLACKHOLE)) {
		m_freem(m);
		return (rt->rt_flags & RTF_BLACKHOLE ? 0 :
			rt->rt_flags & RTF_HOST ? EHOSTUNREACH : ENETUNREACH);
	}

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

	m_tag_delete_nonpersistent(m);

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
static void
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
lortrequest(int cmd, struct rtentry *rt,
    struct rt_addrinfo *info)
{

	if (rt)
		rt->rt_rmx.rmx_mtu = lo0ifp->if_mtu;
}

/*
 * Process an ioctl request.
 */
/* ARGSUSED */
int
loioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct ifaddr *ifa;
	struct ifreq *ifr;
	int error = 0;

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		ifa = (struct ifaddr *)data;
		if (ifa != NULL /*&& ifa->ifa_addr->sa_family == AF_ISO*/)
			ifa->ifa_rtrequest = lortrequest;
		/*
		 * Everything else is done at a higher level.
		 */
		break;

	case SIOCSIFMTU:
		ifr = (struct ifreq *)data;
		if ((unsigned)ifr->ifr_mtu > LOMTU_MAX)
			error = EINVAL;
		else {
			/* XXX update rt mtu for AF_ISO? */
			ifp->if_mtu = ifr->ifr_mtu;
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		ifr = (struct ifreq *)data;
		if (ifr == NULL) {
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
