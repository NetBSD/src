/*	$NetBSD: if_gif.c,v 1.20 2001/01/17 00:30:51 thorpej Exp $	*/
/*	$KAME: if_gif.c,v 1.34 2000/10/07 03:58:53 itojun Exp $	*/

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

#include "opt_inet.h"

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
#include <machine/cpu.h>

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
#include <netinet/in_gif.h>
#endif	/* INET */

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

#include <netinet/ip_encap.h>
#include <net/if_gif.h>

#include "gif.h"
#include "bpfilter.h"

#include <net/net_osdep.h>

#if NGIF > 0

void gifattach __P((int));
static int gif_encapcheck __P((const struct mbuf *, int, int, void *));
#ifdef INET
extern struct protosw in_gif_protosw;
#endif
#ifdef INET6
extern struct ip6protosw in6_gif_protosw;
#endif

/*
 * gif global variable definitions
 */
LIST_HEAD(, gif_softc) gif_softc_list;

int	gif_clone_create __P((struct if_clone *, int));
void	gif_clone_destroy __P((struct ifnet *));

struct if_clone gif_cloner =
    IF_CLONE_INITIALIZER("gif", gif_clone_create, gif_clone_destroy);

void	gif_delete_tunnel __P((struct gif_softc *));

#ifndef MAX_GIF_NEST
/*
 * This macro controls the upper limitation on nesting of gif tunnels.
 * Since, setting a large value to this macro with a careless configuration
 * may introduce system crash, we don't allow any nestings by default.
 * If you need to configure nested gif tunnels, you can define this macro
 * in your kernel configuration file. However, if you do so, please be
 * careful to configure the tunnels so that it won't make a loop.
 */
#define MAX_GIF_NEST 1
#endif
static int max_gif_nesting = MAX_GIF_NEST;

/* ARGSUSED */
void
gifattach(count)
	int count;
{

	LIST_INIT(&gif_softc_list);
	if_clone_attach(&gif_cloner);
}

int
gif_clone_create(ifc, unit)
	struct if_clone *ifc;
	int unit;
{
	struct gif_softc *sc;

	sc = malloc(sizeof(struct gif_softc), M_DEVBUF, M_WAIT);
	bzero(sc, sizeof(struct gif_softc));

	sprintf(sc->gif_if.if_xname, "%s%d", ifc->ifc_name, unit);

	sc->encap_cookie4 = sc->encap_cookie6 = NULL;
#ifdef INET
	sc->encap_cookie4 = encap_attach_func(AF_INET, -1,
	    gif_encapcheck, &in_gif_protosw, sc);
	if (sc->encap_cookie4 == NULL) {
		printf("%s: unable to attach encap4\n", if_name(&sc->gif_if));
		free(sc, M_DEVBUF);
		return (EIO);	/* XXX */
	}
#endif
#ifdef INET6
	sc->encap_cookie6 = encap_attach_func(AF_INET6, -1,
	    gif_encapcheck, (struct protosw *)&in6_gif_protosw, sc);
	if (sc->encap_cookie6 == NULL) {
		if (sc->encap_cookie4) {
			encap_detach(sc->encap_cookie4);
			sc->encap_cookie4 = NULL;
		}
		printf("%s: unable to attach encap6\n", if_name(&sc->gif_if));
		free(sc, M_DEVBUF);
		return (EIO);	/* XXX */
	}
#endif

	sc->gif_if.if_mtu    = GIF_MTU;
	sc->gif_if.if_flags  = IFF_POINTOPOINT | IFF_MULTICAST;
	sc->gif_if.if_ioctl  = gif_ioctl;
	sc->gif_if.if_output = gif_output;
	sc->gif_if.if_type   = IFT_GIF;
	sc->gif_if.if_dlt    = DLT_NULL;
	if_attach(&sc->gif_if);
	if_alloc_sadl(&sc->gif_if);
#if NBPFILTER > 0
	bpfattach(&sc->gif_if, DLT_NULL, sizeof(u_int));
#endif
	LIST_INSERT_HEAD(&gif_softc_list, sc, gif_list);
	return (0);
}

void
gif_clone_destroy(ifp)
	struct ifnet *ifp;
{
	struct gif_softc *sc = (void *) ifp;

	gif_delete_tunnel(sc);
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
}

static int
gif_encapcheck(m, off, proto, arg)
	const struct mbuf *m;
	int off;
	int proto;
	void *arg;
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
	default:
		return 0;
	}

	/* LINTED const cast */
	m_copydata((struct mbuf *)m, 0, sizeof(ip), (caddr_t)&ip);

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
		if (sc->gif_psrc->sa_family != AF_INET6 ||
		    sc->gif_pdst->sa_family != AF_INET6)
			return 0;
		return gif_encapcheck6(m, off, proto, arg);
#endif
	default:
		return 0;
	}
}

int
gif_output(ifp, m, dst, rt)
	struct ifnet *ifp;
	struct mbuf *m;
	struct sockaddr *dst;
	struct rtentry *rt;	/* added in net2 */
{
	register struct gif_softc *sc = (struct gif_softc*)ifp;
	int error = 0;
	static int called = 0;	/* XXX: MUTEX */

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

	ifp->if_lastchange = time;	
	m->m_flags &= ~(M_BCAST|M_MCAST);
	if (!(ifp->if_flags & IFF_UP) ||
	    sc->gif_psrc == NULL || sc->gif_pdst == NULL) {
		m_freem(m);
		error = ENETDOWN;
		goto end;
	}

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
		u_int af = dst->sa_family;

		m0.m_next = m;
		m0.m_len = 4;
		m0.m_data = (char *)&af;
		
#ifdef HAVE_OLD_BPF
		bpf_mtap(ifp, &m0);
#else
		bpf_mtap(ifp->if_bpf, &m0);
#endif
	}
#endif
	ifp->if_opackets++;	
	ifp->if_obytes += m->m_pkthdr.len;

	/* XXX should we check if our outer source is legal? */

	switch (sc->gif_psrc->sa_family) {
#ifdef INET
	case AF_INET:
		error = in_gif_output(ifp, dst->sa_family, m, rt);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		error = in6_gif_output(ifp, dst->sa_family, m, rt);
		break;
#endif
	default:
		m_freem(m);		
		error = ENETDOWN;
	}

  end:
	called = 0;		/* reset recursion counter */
	if (error) ifp->if_oerrors++;
	return error;
}

void
gif_input(m, af, gifp)
	struct mbuf *m;
	int af;
	struct ifnet *gifp;
{
	int s, isr;
	register struct ifqueue *ifq = 0;

	if (gifp == NULL) {
		/* just in case */
		m_freem(m);
		return;
	}

	m->m_pkthdr.rcvif = gifp;
	
#if NBPFILTER > 0
	if (gifp->if_bpf) {
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
		bpf_mtap(gifp, &m0);
#else
		bpf_mtap(gifp->if_bpf, &m0);
#endif
	}
#endif /*NBPFILTER > 0*/

	/*
	 * Put the packet to the network layer input queue according to the
	 * specified address family.
	 * Note: older versions of gif_input directly called network layer
	 * input functions, e.g. ip6_input, here. We changed the policy to
	 * prevent too many recursive calls of such input functions, which
	 * might cause kernel panic. But the change may introduce another
	 * problem; if the input queue is full, packets are discarded.
	 * We believed it rarely occurs and changed the policy. If we find
	 * it occurs more times than we thought, we may change the policy
	 * again.
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
	default:
		m_freem(m);
		return;
	}

	s = splimp();
	if (IF_QFULL(ifq)) {
		IF_DROP(ifq);	/* update statistics */
		m_freem(m);
		splx(s);
		return;
	}
	IF_ENQUEUE(ifq, m);
	/* we need schednetisr since the address family may change */
	schednetisr(isr);
	gifp->if_ipackets++;
	gifp->if_ibytes += m->m_pkthdr.len;
	splx(s);

	return;
}

/* XXX how should we handle IPv6 scope on SIOC[GS]IFPHYADDR? */
int
gif_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct proc *p = curproc;	/* XXX */
	struct gif_softc *sc  = (struct gif_softc*)ifp;
	struct ifreq     *ifr = (struct ifreq*)data;
	int error = 0, size;
	struct sockaddr *dst, *src;
	struct sockaddr *sa;
	struct gif_softc *sc2;
		
	switch (cmd) {
	case SIOCSIFADDR:
		break;
		
	case SIOCSIFDSTADDR:
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			break;
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
		{
			u_long mtu;
			if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
				break;
			mtu = ifr->ifr_mtu;
			if (mtu < GIF_MTU_MIN || mtu > GIF_MTU_MAX) {
				return (EINVAL);
			}
			ifp->if_mtu = mtu;
		}
		break;
#endif /* SIOCSIFMTU */

	case SIOCSIFPHYADDR:
#ifdef INET6
	case SIOCSIFPHYADDR_IN6:
#endif /* INET6 */
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			break;
		switch (cmd) {
#ifdef INET
		case SIOCSIFPHYADDR:
			src = (struct sockaddr *)
				&(((struct in_aliasreq *)data)->ifra_addr);
			dst = (struct sockaddr *)
				&(((struct in_aliasreq *)data)->ifra_dstaddr);
			if (src->sa_len != sizeof(struct sockaddr_in) ||
			    dst->sa_len != sizeof(struct sockaddr_in))
				return EINVAL;
			if (src->sa_family != AF_INET ||
			    dst->sa_family != AF_INET)
				return EAFNOSUPPORT;
			break;
#endif
#ifdef INET6
		case SIOCSIFPHYADDR_IN6:
			src = (struct sockaddr *)
				&(((struct in6_aliasreq *)data)->ifra_addr);
			dst = (struct sockaddr *)
				&(((struct in6_aliasreq *)data)->ifra_dstaddr);
			if (src->sa_len != sizeof(struct sockaddr_in6) ||
			    dst->sa_len != sizeof(struct sockaddr_in6))
				return EINVAL;
			if (src->sa_family != AF_INET6 ||
			    dst->sa_family != AF_INET6)
				return EAFNOSUPPORT;
			break;
#endif
		}

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

			/* can't configure multiple multi-dest interfaces */
#define multidest(x) \
	(((struct sockaddr_in *)(x))->sin_addr.s_addr == INADDR_ANY)
#ifdef INET6
#define multidest6(x) \
	(IN6_IS_ADDR_UNSPECIFIED(&((struct sockaddr_in6 *)(x))->sin6_addr))
#endif
			if (dst->sa_family == AF_INET &&
			    multidest(dst) && multidest(sc2->gif_pdst)) {
				error = EADDRNOTAVAIL;
				goto bad;
			}
#ifdef INET6
			if (dst->sa_family == AF_INET6 &&
			    multidest6(dst) && multidest6(sc2->gif_pdst)) {
				error = EADDRNOTAVAIL;
				goto bad;
			}
#endif
		}

		if (src->sa_family != dst->sa_family ||
		    src->sa_len != dst->sa_len) {
			error = EINVAL;
			break;
		}
		switch (src->sa_family) {
#ifdef INET
		case AF_INET:
			size = sizeof(struct sockaddr_in);
			break;
#endif
#ifdef INET6
		case AF_INET6:
			size = sizeof(struct sockaddr_in6);
			break;
#endif
		default:
			error = EAFNOSUPPORT;
			goto bad;
		}
		if (src->sa_len != size) {
			error = EINVAL;
			break;
		}

		if (sc->gif_psrc)
			free((caddr_t)sc->gif_psrc, M_IFADDR);
		sa = (struct sockaddr *)malloc(size, M_IFADDR, M_WAITOK);
		bcopy((caddr_t)src, (caddr_t)sa, size);
		sc->gif_psrc = sa;

		if (sc->gif_pdst)
			free((caddr_t)sc->gif_pdst, M_IFADDR);
		sa = (struct sockaddr *)malloc(size, M_IFADDR, M_WAITOK);
		bcopy((caddr_t)dst, (caddr_t)sa, size);
		sc->gif_pdst = sa;

		ifp->if_flags |= IFF_UP;
		if_up(ifp);		/* send up RTM_IFINFO */

		error = 0;
		break;

#ifdef SIOCDIFPHYADDR
	case SIOCDIFPHYADDR:
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			break;
		gif_delete_tunnel(sc);
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

void
gif_delete_tunnel(sc)
	struct gif_softc *sc;
{
	int s;

	s = splsoftnet();

	if (sc->gif_psrc) {
		free((caddr_t)sc->gif_psrc, M_IFADDR);
		sc->gif_psrc = NULL;
	}
	if (sc->gif_pdst) {
		free((caddr_t)sc->gif_pdst, M_IFADDR);
		sc->gif_pdst = NULL;
	}
	/* change the IFF_UP flag as well? */

	splx(s);
}
#endif /*NGIF > 0*/
