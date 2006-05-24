/*	$NetBSD: if_gif.c,v 1.57.8.2 2006/05/24 10:58:56 yamt Exp $	*/
/*	$KAME: if_gif.c,v 1.76 2001/08/20 02:01:02 kjc Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_gif.c,v 1.57.8.2 2006/05/24 10:58:56 yamt Exp $");

#include "opt_inet.h"
#include "opt_iso.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/kauth.h>

#include <machine/cpu.h>
#include <machine/intr.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/netisr.h>
#include <net/route.h>
#include <net/bpf.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifdef	INET
#include <netinet/in_var.h>
#endif	/* INET */
#include <netinet/in_gif.h>

#ifdef INET6
#ifndef INET
#include <netinet/in.h>
#endif
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_gif.h>
#include <netinet6/ip6protosw.h>
#endif /* INET6 */

#ifdef ISO
#include <netiso/iso.h>
#include <netiso/iso_var.h>
#endif

#include <netinet/ip_encap.h>
#include <net/if_ether.h>
#include <net/if_bridgevar.h>
#include <net/if_gif.h>

#include "bpfilter.h"
#include "bridge.h"

#include <net/net_osdep.h>

void	gifattach(int);
#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
static void	gifnetisr(void);
#endif
static void	gifintr(void *);
static void	gif_start(struct ifnet *);
#ifdef ISO
static struct mbuf *gif_eon_encap(struct mbuf *);
static struct mbuf *gif_eon_decap(struct ifnet *, struct mbuf *);
#endif

/*
 * gif global variable definitions
 */
LIST_HEAD(, gif_softc) gif_softc_list;	/* XXX should be static */

static int	gif_clone_create(struct if_clone *, int);
static int	gif_clone_destroy(struct ifnet *);

static struct if_clone gif_cloner =
    IF_CLONE_INITIALIZER("gif", gif_clone_create, gif_clone_destroy);

#ifndef MAX_GIF_NEST
/*
 * This macro controls the upper limitation on nesting of gif tunnels.
 * Since, setting a large value to this macro with a careless configuration
 * may introduce system crash, we don't allow any nestings by default.
 * If you need to configure nested gif tunnels, you can define this macro
 * in your kernel configuration file.  However, if you do so, please be
 * careful to configure the tunnels so that it won't make a loop.
 */
#define MAX_GIF_NEST 1
#endif
static int max_gif_nesting = MAX_GIF_NEST;

/* ARGSUSED */
void
gifattach(int count)
{

	LIST_INIT(&gif_softc_list);
	if_clone_attach(&gif_cloner);
}

static int
gif_clone_create(struct if_clone *ifc, int unit)
{
	struct gif_softc *sc;

	sc = malloc(sizeof(struct gif_softc), M_DEVBUF, M_WAIT);
	memset(sc, 0, sizeof(struct gif_softc));

	snprintf(sc->gif_if.if_xname, sizeof(sc->gif_if.if_xname), "%s%d",
	    ifc->ifc_name, unit);

	gifattach0(sc);

	LIST_INSERT_HEAD(&gif_softc_list, sc, gif_list);
	return (0);
}

void
gifattach0(struct gif_softc *sc)
{

	sc->encap_cookie4 = sc->encap_cookie6 = NULL;

	sc->gif_if.if_addrlen = 0;
	sc->gif_if.if_mtu    = GIF_MTU;
	sc->gif_if.if_flags  = IFF_POINTOPOINT | IFF_MULTICAST;
	sc->gif_if.if_ioctl  = gif_ioctl;
	sc->gif_if.if_output = gif_output;
	sc->gif_if.if_start  = gif_start;
	sc->gif_if.if_type   = IFT_GIF;
	sc->gif_if.if_dlt    = DLT_NULL;
	IFQ_SET_READY(&sc->gif_if.if_snd);
	if_attach(&sc->gif_if);
	if_alloc_sadl(&sc->gif_if);
#if NBPFILTER > 0
	bpfattach(&sc->gif_if, DLT_NULL, sizeof(u_int));
#endif
}

static int
gif_clone_destroy(struct ifnet *ifp)
{
	struct gif_softc *sc = (void *) ifp;

	gif_delete_tunnel(&sc->gif_if);
	LIST_REMOVE(sc, gif_list);
#ifdef INET6
	encap_detach(sc->encap_cookie6);
#endif
#ifdef INET
	encap_detach(sc->encap_cookie4);
#endif

#if NBPFILTER > 0
	bpfdetach(ifp);
#endif
	if_detach(ifp);

	free(sc, M_DEVBUF);

	return (0);
}

static void
gif_start(struct ifnet *ifp)
{
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	softintr_schedule(((struct gif_softc*)ifp)->gif_si);
#else
	/* XXX bad spl level? */
	gifnetisr();
#endif
}

#ifdef GIF_ENCAPCHECK
int
gif_encapcheck(struct mbuf *m, int off, int proto, void *arg)
{
	struct ip ip;
	struct gif_softc *sc;

	sc = (struct gif_softc *)arg;
	if (sc == NULL)
		return 0;

	if ((sc->gif_if.if_flags & IFF_UP) == 0)
		return 0;

	/* no physical address */
	if (!sc->gif_psrc || !sc->gif_pdst)
		return 0;

	switch (proto) {
#ifdef INET
	case IPPROTO_IPV4:
		break;
#endif
#ifdef INET6
	case IPPROTO_IPV6:
		break;
#endif
#ifdef ISO
	case IPPROTO_EON:
		break;
#endif
#if NBRIDGE > 0
	case IPPROTO_ETHERIP:
		break;
#endif
	default:
		return 0;
	}

	/* Bail on short packets */
	KASSERT(m->m_flags & M_PKTHDR);
	if (m->m_pkthdr.len < sizeof(ip))
		return 0;

	m_copydata(m, 0, sizeof(ip), (caddr_t)&ip);

	switch (ip.ip_v) {
#ifdef INET
	case 4:
		if (sc->gif_psrc->sa_family != AF_INET ||
		    sc->gif_pdst->sa_family != AF_INET)
			return 0;
		return gif_encapcheck4(m, off, proto, arg);
#endif
#ifdef INET6
	case 6:
		if (m->m_pkthdr.len < sizeof(struct ip6_hdr))
			return 0;
		if (sc->gif_psrc->sa_family != AF_INET6 ||
		    sc->gif_pdst->sa_family != AF_INET6)
			return 0;
		return gif_encapcheck6(m, off, proto, arg);
#endif
	default:
		return 0;
	}
}
#endif

int
gif_output(struct ifnet *ifp, struct mbuf *m, struct sockaddr *dst,
    struct rtentry *rt)
{
	struct gif_softc *sc = (struct gif_softc*)ifp;
	int error = 0;
	static int called = 0;	/* XXX: MUTEX */
	ALTQ_DECL(struct altq_pktattr pktattr;)
	int s;

	IFQ_CLASSIFY(&ifp->if_snd, m, dst->sa_family, &pktattr);

	/*
	 * gif may cause infinite recursion calls when misconfigured.
	 * We'll prevent this by introducing upper limit.
	 * XXX: this mechanism may introduce another problem about
	 *      mutual exclusion of the variable CALLED, especially if we
	 *      use kernel thread.
	 */
	if (++called > max_gif_nesting) {
		log(LOG_NOTICE,
		    "gif_output: recursively called too many times(%d)\n",
		    called);
		m_freem(m);
		error = EIO;	/* is there better errno? */
		goto end;
	}

	m->m_flags &= ~(M_BCAST|M_MCAST);
	if (!(ifp->if_flags & IFF_UP) ||
	    sc->gif_psrc == NULL || sc->gif_pdst == NULL) {
		m_freem(m);
		error = ENETDOWN;
		goto end;
	}

	/* inner AF-specific encapsulation */
	switch (dst->sa_family) {
#ifdef ISO
	case AF_ISO:
		m = gif_eon_encap(m);
		if (!m) {
			error = ENOBUFS;
			goto end;
		}
		break;
#endif
	default:
		break;
	}

	/* XXX should we check if our outer source is legal? */

	/* use DLT_NULL encapsulation here to pass inner af type */
	M_PREPEND(m, sizeof(int), M_DONTWAIT);
	if (!m) {
		error = ENOBUFS;
		goto end;
	}
	*mtod(m, int *) = dst->sa_family;

	s = splnet();
	IFQ_ENQUEUE(&ifp->if_snd, m, &pktattr, error);
	if (error) {
		splx(s);
		goto end;
	}
	splx(s);

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	softintr_schedule(sc->gif_si);
#else
	/* XXX bad spl level? */
	gifnetisr();
#endif
	error = 0;

  end:
	called = 0;		/* reset recursion counter */
	if (error)
		ifp->if_oerrors++;
	return error;
}

#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
static void
gifnetisr(void)
{
	struct gif_softc *sc;

	for (sc = LIST_FIRST(&gif_softc_list); sc != NULL;
	     sc = LIST_NEXT(sc, gif_list)) {
		gifintr(sc);
	}
}
#endif

static void
gifintr(void *arg)
{
	struct gif_softc *sc;
	struct ifnet *ifp;
	struct mbuf *m;
	int family;
	int len;
	int s;
	int error;

	sc = (struct gif_softc *)arg;
	ifp = &sc->gif_if;

	/* output processing */
	while (1) {
		s = splnet();
		IFQ_DEQUEUE(&sc->gif_if.if_snd, m);
		splx(s);
		if (m == NULL)
			break;

#if NBRIDGE > 0
		if(m->m_flags & M_PROTO1) {
			M_PREPEND(m, sizeof(int), M_DONTWAIT);
			if (!m) {
				ifp->if_oerrors++;
				continue;
			}
			*mtod(m, int *) = AF_LINK;
		}

#endif
		/* grab and chop off inner af type */
		if (sizeof(int) > m->m_len) {
			m = m_pullup(m, sizeof(int));
			if (!m) {
				ifp->if_oerrors++;
				continue;
			}
		}
		family = *mtod(m, int *);
#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif
		m_adj(m, sizeof(int));

		len = m->m_pkthdr.len;

		/* dispatch to output logic based on outer AF */
		switch (sc->gif_psrc->sa_family) {
#ifdef INET
		case AF_INET:
			error = in_gif_output(ifp, family, m);
			break;
#endif
#ifdef INET6
		case AF_INET6:
			error = in6_gif_output(ifp, family, m);
			break;
#endif
		default:
			m_freem(m);
			error = ENETDOWN;
			break;
		}

		if (error)
			ifp->if_oerrors++;
		else {
			ifp->if_opackets++;
			ifp->if_obytes += len;
		}
	}
}

void
gif_input(struct mbuf *m, int af, struct ifnet *ifp)
{
	int s, isr;
	struct ifqueue *ifq = NULL;
#if NBRIDGE > 0
	struct ether_header *eh;
#endif

	if (ifp == NULL) {
		/* just in case */
		m_freem(m);
		return;
	}

	m->m_pkthdr.rcvif = ifp;

#if NBPFILTER > 0
	if (ifp->if_bpf)
		bpf_mtap_af(ifp->if_bpf, af, m);
#endif /*NBPFILTER > 0*/

	/*
	 * Put the packet to the network layer input queue according to the
	 * specified address family.
	 * Note: older versions of gif_input directly called network layer
	 * input functions, e.g. ip6_input, here.  We changed the policy to
	 * prevent too many recursive calls of such input functions, which
	 * might cause kernel panic.  But the change may introduce another
	 * problem; if the input queue is full, packets are discarded.
	 * The kernel stack overflow really happened, and we believed
	 * queue-full rarely occurs, so we changed the policy.
	 */
	switch (af) {
#ifdef INET
	case AF_INET:
		ifq = &ipintrq;
		isr = NETISR_IP;
		break;
#endif
#ifdef INET6
	case AF_INET6:
		ifq = &ip6intrq;
		isr = NETISR_IPV6;
		break;
#endif
#ifdef ISO
	case AF_ISO:
		m = gif_eon_decap(ifp, m);
		if (!m)
			return;
		ifq = &clnlintrq;
		isr = NETISR_ISO;
		break;
#endif
#if NBRIDGE > 0
	case AF_LINK:
		m_adj(m, sizeof(struct etherip_header));
		if (sizeof(struct ether_header) > m->m_len) {
			m = m_pullup(m, sizeof(struct ether_header));
			if (!m) {
				ifp->if_ierrors++;
				return;
			}
		}
		eh = mtod(m, struct ether_header *);
		m->m_flags &= ~(M_BCAST|M_MCAST);
		if (eh->ether_dhost[0] & 1) {
			if (memcmp(etherbroadcastaddr,
			    eh->ether_dhost, sizeof(etherbroadcastaddr)) == 0)
				m->m_flags |= M_BCAST;
			else
				m->m_flags |= M_MCAST;
		}
		m->m_pkthdr.rcvif = ifp;
		if(ifp->if_bridge) {
			if (m->m_flags & (M_BCAST|M_MCAST))
				ifp->if_imcasts++;

			s = splnet();
			m = bridge_input(ifp, m);
			splx(s);
			if (m == NULL)
				return;
		}
#endif
	default:
		m_freem(m);
		return;
	}

	s = splnet();
	if (IF_QFULL(ifq)) {
		IF_DROP(ifq);	/* update statistics */
		m_freem(m);
		splx(s);
		return;
	}
	ifp->if_ipackets++;
	ifp->if_ibytes += m->m_pkthdr.len;
	IF_ENQUEUE(ifq, m);
	/* we need schednetisr since the address family may change */
	schednetisr(isr);
	splx(s);
}

/* XXX how should we handle IPv6 scope on SIOC[GS]IFPHYADDR? */
int
gif_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct proc *p = curproc;	/* XXX */
	struct gif_softc *sc  = (struct gif_softc*)ifp;
	struct ifreq     *ifr = (struct ifreq*)data;
	int error = 0, size;
	struct sockaddr *dst, *src;
#ifdef SIOCSIFMTU
	u_long mtu;
#endif

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		break;

	case SIOCSIFDSTADDR:
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		switch (ifr->ifr_addr.sa_family) {
#ifdef INET
		case AF_INET:	/* IP supports Multicast */
			break;
#endif /* INET */
#ifdef INET6
		case AF_INET6:	/* IP6 supports Multicast */
			break;
#endif /* INET6 */
		default:  /* Other protocols doesn't support Multicast */
			error = EAFNOSUPPORT;
			break;
		}
		break;

#ifdef	SIOCSIFMTU /* xxx */
	case SIOCGIFMTU:
		break;

	case SIOCSIFMTU:
		if ((error = kauth_authorize_generic(p->p_cred,
					       KAUTH_GENERIC_ISSUSER,
					        &p->p_acflag)) != 0)
			break;
		mtu = ifr->ifr_mtu;
		if (mtu < GIF_MTU_MIN || mtu > GIF_MTU_MAX)
			return (EINVAL);
		ifp->if_mtu = mtu;
		break;
#endif /* SIOCSIFMTU */

#ifdef INET
	case SIOCSIFPHYADDR:
#endif
#ifdef INET6
	case SIOCSIFPHYADDR_IN6:
#endif /* INET6 */
	case SIOCSLIFPHYADDR:
		if ((error = kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER, &p->p_acflag)) != 0)
			break;
		switch (cmd) {
#ifdef INET
		case SIOCSIFPHYADDR:
			src = (struct sockaddr *)
				&(((struct in_aliasreq *)data)->ifra_addr);
			dst = (struct sockaddr *)
				&(((struct in_aliasreq *)data)->ifra_dstaddr);
			break;
#endif
#ifdef INET6
		case SIOCSIFPHYADDR_IN6:
			src = (struct sockaddr *)
				&(((struct in6_aliasreq *)data)->ifra_addr);
			dst = (struct sockaddr *)
				&(((struct in6_aliasreq *)data)->ifra_dstaddr);
			break;
#endif
		case SIOCSLIFPHYADDR:
			src = (struct sockaddr *)
				&(((struct if_laddrreq *)data)->addr);
			dst = (struct sockaddr *)
				&(((struct if_laddrreq *)data)->dstaddr);
			break;
		default:
			return EINVAL;
		}

		/* sa_family must be equal */
		if (src->sa_family != dst->sa_family)
			return EINVAL;

		/* validate sa_len */
		switch (src->sa_family) {
#ifdef INET
		case AF_INET:
			if (src->sa_len != sizeof(struct sockaddr_in))
				return EINVAL;
			break;
#endif
#ifdef INET6
		case AF_INET6:
			if (src->sa_len != sizeof(struct sockaddr_in6))
				return EINVAL;
			break;
#endif
		default:
			return EAFNOSUPPORT;
		}
		switch (dst->sa_family) {
#ifdef INET
		case AF_INET:
			if (dst->sa_len != sizeof(struct sockaddr_in))
				return EINVAL;
			break;
#endif
#ifdef INET6
		case AF_INET6:
			if (dst->sa_len != sizeof(struct sockaddr_in6))
				return EINVAL;
			break;
#endif
		default:
			return EAFNOSUPPORT;
		}

		/* check sa_family looks sane for the cmd */
		switch (cmd) {
		case SIOCSIFPHYADDR:
			if (src->sa_family == AF_INET)
				break;
			return EAFNOSUPPORT;
#ifdef INET6
		case SIOCSIFPHYADDR_IN6:
			if (src->sa_family == AF_INET6)
				break;
			return EAFNOSUPPORT;
#endif /* INET6 */
		case SIOCSLIFPHYADDR:
			/* checks done in the above */
			break;
		}

		error = gif_set_tunnel(&sc->gif_if, src, dst);
		break;

#ifdef SIOCDIFPHYADDR
	case SIOCDIFPHYADDR:
		if ((error = kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER, &p->p_acflag)) != 0)
			break;
		gif_delete_tunnel(&sc->gif_if);
		break;
#endif

	case SIOCGIFPSRCADDR:
#ifdef INET6
	case SIOCGIFPSRCADDR_IN6:
#endif /* INET6 */
		if (sc->gif_psrc == NULL) {
			error = EADDRNOTAVAIL;
			goto bad;
		}
		src = sc->gif_psrc;
		switch (cmd) {
#ifdef INET
		case SIOCGIFPSRCADDR:
			dst = &ifr->ifr_addr;
			size = sizeof(ifr->ifr_addr);
			break;
#endif /* INET */
#ifdef INET6
		case SIOCGIFPSRCADDR_IN6:
			dst = (struct sockaddr *)
				&(((struct in6_ifreq *)data)->ifr_addr);
			size = sizeof(((struct in6_ifreq *)data)->ifr_addr);
			break;
#endif /* INET6 */
		default:
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if (src->sa_len > size)
			return EINVAL;
		bcopy((caddr_t)src, (caddr_t)dst, src->sa_len);
		break;

	case SIOCGIFPDSTADDR:
#ifdef INET6
	case SIOCGIFPDSTADDR_IN6:
#endif /* INET6 */
		if (sc->gif_pdst == NULL) {
			error = EADDRNOTAVAIL;
			goto bad;
		}
		src = sc->gif_pdst;
		switch (cmd) {
#ifdef INET
		case SIOCGIFPDSTADDR:
			dst = &ifr->ifr_addr;
			size = sizeof(ifr->ifr_addr);
			break;
#endif /* INET */
#ifdef INET6
		case SIOCGIFPDSTADDR_IN6:
			dst = (struct sockaddr *)
				&(((struct in6_ifreq *)data)->ifr_addr);
			size = sizeof(((struct in6_ifreq *)data)->ifr_addr);
			break;
#endif /* INET6 */
		default:
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if (src->sa_len > size)
			return EINVAL;
		bcopy((caddr_t)src, (caddr_t)dst, src->sa_len);
		break;

	case SIOCGLIFPHYADDR:
		if (sc->gif_psrc == NULL || sc->gif_pdst == NULL) {
			error = EADDRNOTAVAIL;
			goto bad;
		}

		/* copy src */
		src = sc->gif_psrc;
		dst = (struct sockaddr *)
			&(((struct if_laddrreq *)data)->addr);
		size = sizeof(((struct if_laddrreq *)data)->addr);
		if (src->sa_len > size)
			return EINVAL;
		bcopy((caddr_t)src, (caddr_t)dst, src->sa_len);

		/* copy dst */
		src = sc->gif_pdst;
		dst = (struct sockaddr *)
			&(((struct if_laddrreq *)data)->dstaddr);
		size = sizeof(((struct if_laddrreq *)data)->dstaddr);
		if (src->sa_len > size)
			return EINVAL;
		bcopy((caddr_t)src, (caddr_t)dst, src->sa_len);
		break;

	case SIOCSIFFLAGS:
		/* if_ioctl() takes care of it */
		break;

	default:
		error = EINVAL;
		break;
	}
 bad:
	return error;
}

int
gif_set_tunnel(struct ifnet *ifp, struct sockaddr *src, struct sockaddr *dst)
{
	struct gif_softc *sc = (struct gif_softc *)ifp;
	struct gif_softc *sc2;
	struct sockaddr *osrc, *odst, *sa;
	int s;
	int error;

	s = splsoftnet();

	for (sc2 = LIST_FIRST(&gif_softc_list); sc2 != NULL;
	     sc2 = LIST_NEXT(sc2, gif_list)) {
		if (sc2 == sc)
			continue;
		if (!sc2->gif_pdst || !sc2->gif_psrc)
			continue;
		if (sc2->gif_pdst->sa_family != dst->sa_family ||
		    sc2->gif_pdst->sa_len != dst->sa_len ||
		    sc2->gif_psrc->sa_family != src->sa_family ||
		    sc2->gif_psrc->sa_len != src->sa_len)
			continue;
		/* can't configure same pair of address onto two gifs */
		if (bcmp(sc2->gif_pdst, dst, dst->sa_len) == 0 &&
		    bcmp(sc2->gif_psrc, src, src->sa_len) == 0) {
			error = EADDRNOTAVAIL;
			goto bad;
		}

		/* XXX both end must be valid? (I mean, not 0.0.0.0) */
	}

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	if (sc->gif_si) {
		softintr_disestablish(sc->gif_si);
		sc->gif_si = NULL;
	}
#endif

	/* XXX we can detach from both, but be polite just in case */
	if (sc->gif_psrc)
		switch (sc->gif_psrc->sa_family) {
#ifdef INET
		case AF_INET:
			(void)in_gif_detach(sc);
			break;
#endif
#ifdef INET6
		case AF_INET6:
			(void)in6_gif_detach(sc);
			break;
#endif
		}

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	sc->gif_si = softintr_establish(IPL_SOFTNET, gifintr, sc);
	if (sc->gif_si == NULL) {
		error = ENOMEM;
		goto bad;
	}
#endif

	osrc = sc->gif_psrc;
	sa = (struct sockaddr *)malloc(src->sa_len, M_IFADDR, M_WAITOK);
	bcopy((caddr_t)src, (caddr_t)sa, src->sa_len);
	sc->gif_psrc = sa;

	odst = sc->gif_pdst;
	sa = (struct sockaddr *)malloc(dst->sa_len, M_IFADDR, M_WAITOK);
	bcopy((caddr_t)dst, (caddr_t)sa, dst->sa_len);
	sc->gif_pdst = sa;

	switch (sc->gif_psrc->sa_family) {
#ifdef INET
	case AF_INET:
		error = in_gif_attach(sc);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		error = in6_gif_attach(sc);
		break;
#endif
	default:
		error = EINVAL;
		break;
	}
	if (error) {
		/* rollback */
		free((caddr_t)sc->gif_psrc, M_IFADDR);
		free((caddr_t)sc->gif_pdst, M_IFADDR);
		sc->gif_psrc = osrc;
		sc->gif_pdst = odst;
		goto bad;
	}

	if (osrc)
		free((caddr_t)osrc, M_IFADDR);
	if (odst)
		free((caddr_t)odst, M_IFADDR);

	if (sc->gif_psrc && sc->gif_pdst)
		ifp->if_flags |= IFF_RUNNING;
	else
		ifp->if_flags &= ~IFF_RUNNING;
	splx(s);

	return 0;

 bad:
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	if (sc->gif_si) {
		softintr_disestablish(sc->gif_si);
		sc->gif_si = NULL;
	}
#endif
	if (sc->gif_psrc && sc->gif_pdst)
		ifp->if_flags |= IFF_RUNNING;
	else
		ifp->if_flags &= ~IFF_RUNNING;
	splx(s);

	return error;
}

void
gif_delete_tunnel(struct ifnet *ifp)
{
	struct gif_softc *sc = (struct gif_softc *)ifp;
	int s;

	s = splsoftnet();

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	if (sc->gif_si) {
		softintr_disestablish(sc->gif_si);
		sc->gif_si = NULL;
	}
#endif
	if (sc->gif_psrc) {
		free((caddr_t)sc->gif_psrc, M_IFADDR);
		sc->gif_psrc = NULL;
	}
	if (sc->gif_pdst) {
		free((caddr_t)sc->gif_pdst, M_IFADDR);
		sc->gif_pdst = NULL;
	}
	/* it is safe to detach from both */
#ifdef INET
	(void)in_gif_detach(sc);
#endif
#ifdef INET6
	(void)in6_gif_detach(sc);
#endif

	if (sc->gif_psrc && sc->gif_pdst)
		ifp->if_flags |= IFF_RUNNING;
	else
		ifp->if_flags &= ~IFF_RUNNING;
	splx(s);
}

#ifdef ISO
struct eonhdr {
	u_int8_t version;
	u_int8_t class;
	u_int16_t cksum;
};

/*
 * prepend EON header to ISO PDU
 */
static struct mbuf *
gif_eon_encap(struct mbuf *m)
{
	struct eonhdr *ehdr;

	M_PREPEND(m, sizeof(*ehdr), M_DONTWAIT);
	if (m && m->m_len < sizeof(*ehdr))
		m = m_pullup(m, sizeof(*ehdr));
	if (m == NULL)
		return NULL;
	ehdr = mtod(m, struct eonhdr *);
	ehdr->version = 1;
	ehdr->class = 0;		/* always unicast */
#if 0
	/* calculate the checksum of the eonhdr */
	{
		struct mbuf mhead;
		memset(&mhead, 0, sizeof(mhead));
		ehdr->cksum = 0;
		mhead.m_data = (caddr_t)ehdr;
		mhead.m_len = sizeof(*ehdr);
		mhead.m_next = 0;
		iso_gen_csum(&mhead, offsetof(struct eonhdr, cksum),
		    mhead.m_len);
	}
#else
	/* since the data is always constant we'll just plug the value in */
	ehdr->cksum = htons(0xfc02);
#endif
	return m;
}

/*
 * remove EON header and check checksum
 */
static struct mbuf *
gif_eon_decap(struct ifnet *ifp, struct mbuf *m)
{
	struct eonhdr *ehdr;

	if (m->m_len < sizeof(*ehdr) &&
	    (m = m_pullup(m, sizeof(*ehdr))) == NULL) {
		ifp->if_ierrors++;
		return NULL;
	}
	if (iso_check_csum(m, sizeof(struct eonhdr))) {
		m_freem(m);
		return NULL;
	}
	m_adj(m, sizeof(*ehdr));
	return m;
}
#endif /*ISO*/
