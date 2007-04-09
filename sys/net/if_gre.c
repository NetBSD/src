/*	$NetBSD: if_gre.c,v 1.88.2.1 2007/04/09 22:10:04 ad Exp $ */

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Heiko W.Rupp <hwr@pilhuhn.de>
 *
 * IPv6-over-GRE contributed by Gert Doering <gert@greenie.muc.de>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_gre.c,v 1.88.2.1 2007/04/09 22:10:04 ad Exp $");

#include "opt_gre.h"
#include "opt_inet.h"
#include "bpfilter.h"

#ifdef INET
#include <sys/param.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#if __NetBSD__
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/kauth.h>
#endif

#include <sys/kthread.h>

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

/*
 * It is not easy to calculate the right value for a GRE MTU.
 * We leave this task to the admin and use the same default that
 * other vendors use.
 */
#define GREMTU 1476

#ifdef GRE_DEBUG
#define	GRE_DPRINTF(__sc, __fmt, ...)				\
	do {							\
		if (((__sc)->sc_if.if_flags & IFF_DEBUG) != 0)	\
			printf(__fmt, __VA_ARGS__);		\
	} while (/*CONSTCOND*/0)
#else
#define	GRE_DPRINTF(__sc, __fmt, ...)	do { } while (/*CONSTCOND*/0)
#endif /* GRE_DEBUG */

struct gre_softc_head gre_softc_list;
int ip_gre_ttl = GRE_TTL;

static int	gre_clone_create(struct if_clone *, int);
static int	gre_clone_destroy(struct ifnet *);

static struct if_clone gre_cloner =
    IF_CLONE_INITIALIZER("gre", gre_clone_create, gre_clone_destroy);

static int	gre_output(struct ifnet *, struct mbuf *,
			   const struct sockaddr *, struct rtentry *);
static int	gre_ioctl(struct ifnet *, u_long, void *);

static int	gre_compute_route(struct gre_softc *sc);

static int gre_getsockname(struct socket *, struct mbuf *, struct lwp *);
static int gre_getpeername(struct socket *, struct mbuf *, struct lwp *);
static int gre_getnames(struct socket *, struct lwp *, struct sockaddr_in *,
    struct sockaddr_in *);

static void
gre_stop(volatile int *running)
{
	*running = 0;
	wakeup(running);
}

static void
gre_join(volatile int *running)
{
	int s;

	s = splnet();
	while (*running != 0) {
		splx(s);
		tsleep(running, PSOCK, "grejoin", 0);
		s = splnet();
	}
	splx(s);
}

static void
gre_wakeup(struct gre_softc *sc)
{
	GRE_DPRINTF(sc, "%s: enter\n", __func__);
	sc->sc_waitchan = 1;
	wakeup(&sc->sc_waitchan);
}

static int
gre_clone_create(struct if_clone *ifc, int unit)
{
	struct gre_softc *sc;

	sc = malloc(sizeof(struct gre_softc), M_DEVBUF, M_WAITOK);
	memset(sc, 0, sizeof(struct gre_softc));

	snprintf(sc->sc_if.if_xname, sizeof(sc->sc_if.if_xname), "%s%d",
	    ifc->ifc_name, unit);
	sc->sc_if.if_softc = sc;
	sc->sc_if.if_type = IFT_TUNNEL;
	sc->sc_if.if_addrlen = 0;
	sc->sc_if.if_hdrlen = 24; /* IP + GRE */
	sc->sc_if.if_dlt = DLT_NULL;
	sc->sc_if.if_mtu = GREMTU;
	sc->sc_if.if_flags = IFF_POINTOPOINT|IFF_MULTICAST;
	sc->sc_if.if_output = gre_output;
	sc->sc_if.if_ioctl = gre_ioctl;
	sc->g_dst.s_addr = sc->g_src.s_addr = INADDR_ANY;
	sc->g_dstport = sc->g_srcport = 0;
	sc->sc_proto = IPPROTO_GRE;
	sc->sc_snd.ifq_maxlen = 256;
	sc->sc_if.if_flags |= IFF_LINK0;
	if_attach(&sc->sc_if);
	if_alloc_sadl(&sc->sc_if);
#if NBPFILTER > 0
	bpfattach(&sc->sc_if, DLT_NULL, sizeof(u_int32_t));
#endif
	LIST_INSERT_HEAD(&gre_softc_list, sc, sc_list);
	return 0;
}

static int
gre_clone_destroy(struct ifnet *ifp)
{
	int s;
	struct gre_softc *sc = ifp->if_softc;

	LIST_REMOVE(sc, sc_list);
#if NBPFILTER > 0
	bpfdetach(ifp);
#endif
	s = splnet();
	ifp->if_flags &= ~IFF_UP;
	gre_wakeup(sc);
	splx(s);
	gre_join(&sc->sc_thread);
	s = splnet();
	rtcache_free(&sc->route);
	if_detach(ifp);
	splx(s);
	if (sc->sc_fp != NULL) {
		closef(sc->sc_fp, curlwp);
		sc->sc_fp = NULL;
	}
	free(sc, M_DEVBUF);

	return 0;
}

static void
gre_receive(struct socket *so, void *arg, int waitflag)
{
	struct gre_softc *sc = (struct gre_softc *)arg;

	GRE_DPRINTF(sc, "%s: enter\n", __func__);

	gre_wakeup(sc);
}

static void
gre_upcall_add(struct socket *so, void *arg)
{
	/* XXX What if the kernel already set an upcall? */
	so->so_upcallarg = arg;
	so->so_upcall = gre_receive;
	so->so_rcv.sb_flags |= SB_UPCALL;
}

static void
gre_upcall_remove(struct socket *so)
{
	/* XXX What if the kernel already set an upcall? */
	so->so_rcv.sb_flags &= ~SB_UPCALL;
	so->so_upcallarg = NULL;
	so->so_upcall = NULL;
}

static void
gre_sodestroy(struct socket **sop)
{
	gre_upcall_remove(*sop);
	soshutdown(*sop, SHUT_RDWR);
	soclose(*sop);
	*sop = NULL;
}

static struct mbuf *
gre_getsockmbuf(struct socket *so)
{
	struct mbuf *m;

	m = m_get(M_WAIT, MT_SONAME);
	if (m != NULL)
		MCLAIM(m, so->so_mowner);
	return m;
}

static int
gre_socreate1(struct gre_softc *sc, struct lwp *l, struct gre_soparm *sp,
    struct socket **sop)
{
	int rc;
	struct mbuf *m;
	struct sockaddr_in *sin;
	struct socket *so;

	GRE_DPRINTF(sc, "%s: enter\n", __func__);
	rc = socreate(AF_INET, sop, SOCK_DGRAM, IPPROTO_UDP, l);
	if (rc != 0) {
		GRE_DPRINTF(sc, "%s: socreate failed\n", __func__);
		return rc;
	}

	so = *sop;

	gre_upcall_add(so, (void *)sc);
	if ((m = gre_getsockmbuf(so)) == NULL) {
		rc = ENOBUFS;
		goto out;
	}
	sin = mtod(m, struct sockaddr_in *);
	sin->sin_len = m->m_len = sizeof(struct sockaddr_in);
	sin->sin_family = AF_INET;
	sin->sin_addr = sc->g_src;
	sin->sin_port = sc->g_srcport;

	GRE_DPRINTF(sc, "%s: bind 0x%08" PRIx32 " port %d\n", __func__,
	    sin->sin_addr.s_addr, ntohs(sin->sin_port));
	if ((rc = sobind(so, m, l)) != 0) {
		GRE_DPRINTF(sc, "%s: sobind failed\n", __func__);
		goto out;
	}

	if (sc->g_srcport == 0) {
		if ((rc = gre_getsockname(so, m, l)) != 0) {
			GRE_DPRINTF(sc, "%s: gre_getsockname failed\n",
			    __func__);
			goto out;
		}
		sc->g_srcport = sin->sin_port;
	}

	sin->sin_addr = sc->g_dst;
	sin->sin_port = sc->g_dstport;

	if ((rc = soconnect(so, m, l)) != 0) {
		GRE_DPRINTF(sc, "%s: soconnect failed\n", __func__);
		goto out;
	}

	*mtod(m, int *) = ip_gre_ttl;
	m->m_len = sizeof(int);
	rc = (*so->so_proto->pr_ctloutput)(PRCO_SETOPT, so, IPPROTO_IP, IP_TTL,
	    &m);
	m = NULL;
	if (rc != 0) {
		printf("%s: setopt ttl failed\n", __func__);
		rc = 0;
	}
out:
	m_freem(m);

	if (rc != 0)
		gre_sodestroy(sop);
	else
		*sp = sc->sc_soparm;

	return rc;
}

static void
gre_thread1(struct gre_softc *sc, struct lwp *l)
{
	int flags, rc, s;
	const struct gre_h *gh;
	struct ifnet *ifp = &sc->sc_if;
	struct mbuf *m;
	struct socket *so = NULL;
	struct uio uio;
	struct gre_soparm sp;

	GRE_DPRINTF(sc, "%s: enter\n", __func__);
	s = splnet();

	sc->sc_waitchan = 1;

	memset(&sp, 0, sizeof(sp));
	memset(&uio, 0, sizeof(uio));

	ifp->if_flags |= IFF_RUNNING;

	for (;;) {
		while (sc->sc_waitchan == 0) {
			splx(s);
			GRE_DPRINTF(sc, "%s: sleeping\n", __func__);
			tsleep(&sc->sc_waitchan, PSOCK, "grewait", 0);
			s = splnet();
		}
		sc->sc_waitchan = 0;
		GRE_DPRINTF(sc, "%s: awake\n", __func__);
		if ((ifp->if_flags & IFF_UP) != IFF_UP) {
			GRE_DPRINTF(sc, "%s: not up & running; exiting\n",
			    __func__);
			break;
		}
		if (sc->sc_proto != IPPROTO_UDP) {
			GRE_DPRINTF(sc, "%s: not udp; exiting\n", __func__);
			break;
		}
		/* XXX optimize */ 
		if (so == NULL || memcmp(&sp, &sc->sc_soparm, sizeof(sp)) != 0){
			GRE_DPRINTF(sc, "%s: parameters changed\n", __func__);

			if (sp.sp_fp != NULL) {
				FILE_UNUSE(sp.sp_fp, NULL);
				sp.sp_fp = NULL;
				so = NULL;
			} else if (so != NULL)
				gre_sodestroy(&so);

			if (sc->sc_fp != NULL) {
				so = (struct socket *)sc->sc_fp->f_data;
				gre_upcall_add(so, (void *)sc);
				sp = sc->sc_soparm;
				FILE_USE(sp.sp_fp);
			} else if (gre_socreate1(sc, l, &sp, &so) != 0)
				goto out;
		}
		for (;;) {
			flags = MSG_DONTWAIT;
			uio.uio_resid = 1000000;
			rc = (*so->so_receive)(so, NULL, &uio, &m, NULL,
			    &flags);
			/* TBD Back off if ECONNREFUSED (indicates
			 * ICMP Port Unreachable)?
			 */
			if (rc == EWOULDBLOCK) {
				GRE_DPRINTF(sc, "%s: so_receive EWOULDBLOCK\n",
				    __func__);
				break;
			} else if (rc != 0 || m == NULL) {
				GRE_DPRINTF(sc, "%s: rc %d m %p\n",
				    ifp->if_xname, rc, (void *)m);
				continue;
			} else
				GRE_DPRINTF(sc, "%s: so_receive ok\n",
				    __func__);
			if (m->m_len < sizeof(*gh) &&
			    (m = m_pullup(m, sizeof(*gh))) == NULL) {
				GRE_DPRINTF(sc, "%s: m_pullup failed\n",
				    __func__);
				continue;
			}
			gh = mtod(m, const struct gre_h *);

			if (gre_input3(sc, m, 0, IPPROTO_GRE, gh) == 0) {
				GRE_DPRINTF(sc, "%s: dropping unsupported\n",
				    __func__);
				ifp->if_ierrors++;
				m_freem(m);
			}
		}
		for (;;) {
			IF_DEQUEUE(&sc->sc_snd, m);
			if (m == NULL)
				break;
			GRE_DPRINTF(sc, "%s: dequeue\n", __func__);
			if ((so->so_state & SS_ISCONNECTED) == 0) {
				GRE_DPRINTF(sc, "%s: not connected\n",
				    __func__);
				m_freem(m);
				continue;
			}
			rc = (*so->so_send)(so, NULL, NULL, m, NULL, 0, l);
			/* XXX handle ENOBUFS? */
			if (rc != 0)
				GRE_DPRINTF(sc, "%s: so_send failed\n",
				    __func__);
		}
		/* Give the software interrupt queues a chance to
		 * run, or else when I send a ping from gre0 to gre1 on
		 * the same host, gre0 will not wake for the reply.
		 */
		splx(s);
		s = splnet();
	}
	if (sp.sp_fp != NULL) {
		GRE_DPRINTF(sc, "%s: removing upcall\n", __func__);
		gre_upcall_remove(so);
		FILE_UNUSE(sp.sp_fp, NULL);
		sp.sp_fp = NULL;
	} else if (so != NULL)
		gre_sodestroy(&so);
out:
	GRE_DPRINTF(sc, "%s: stopping\n", __func__);
	if (sc->sc_proto == IPPROTO_UDP)
		ifp->if_flags &= ~IFF_RUNNING;
	while (!IF_IS_EMPTY(&sc->sc_snd)) {
		IF_DEQUEUE(&sc->sc_snd, m);
		m_freem(m);
	}
	gre_stop(&sc->sc_thread);
	/* must not touch sc after this! */
	GRE_DPRINTF(sc, "%s: restore ipl\n", __func__);
	splx(s);
}

static void
gre_thread(void *arg)
{
	struct gre_softc *sc = (struct gre_softc *)arg;

	gre_thread1(sc, curlwp);
	/* must not touch sc after this! */
	kthread_exit(0);
}

int
gre_input3(struct gre_softc *sc, struct mbuf *m, int hlen, u_char proto,
    const struct gre_h *gh)
{
	u_int16_t flags;
#if NBPFILTER > 0
	u_int32_t af = AF_INET;		/* af passed to BPF tap */
#endif
	int s, isr;
	struct ifqueue *ifq;

	sc->sc_if.if_ipackets++;
	sc->sc_if.if_ibytes += m->m_pkthdr.len;

	switch (proto) {
	case IPPROTO_GRE:
		hlen += sizeof(struct gre_h);

		/* process GRE flags as packet can be of variable len */
		flags = ntohs(gh->flags);

		/* Checksum & Offset are present */
		if ((flags & GRE_CP) | (flags & GRE_RP))
			hlen += 4;
		/* We don't support routing fields (variable length) */
		if (flags & GRE_RP)
			return 0;
		if (flags & GRE_KP)
			hlen += 4;
		if (flags & GRE_SP)
			hlen += 4;

		switch (ntohs(gh->ptype)) { /* ethertypes */
		case ETHERTYPE_IP: /* shouldn't need a schednetisr(), as */
			ifq = &ipintrq;          /* we are in ip_input */
			isr = NETISR_IP;
			break;
#ifdef NETATALK
		case ETHERTYPE_ATALK:
			ifq = &atintrq1;
			isr = NETISR_ATALK;
#if NBPFILTER > 0
			af = AF_APPLETALK;
#endif
			break;
#endif
#ifdef INET6
		case ETHERTYPE_IPV6:
			GRE_DPRINTF(sc, "%s: IPv6 packet\n", __func__);
			ifq = &ip6intrq;
			isr = NETISR_IPV6;
#if NBPFILTER > 0
			af = AF_INET6;
#endif
			break;
#endif
		default:	   /* others not yet supported */
			printf("%s: unhandled ethertype 0x%04x\n", __func__,
			    ntohs(gh->ptype));
			return 0;
		}
		break;
	default:
		/* others not yet supported */
		return 0;
	}

	if (hlen > m->m_pkthdr.len) {
		m_freem(m);
		sc->sc_if.if_ierrors++;
		return EINVAL;
	}
	m_adj(m, hlen);

#if NBPFILTER > 0
	if (sc->sc_if.if_bpf != NULL)
		bpf_mtap_af(sc->sc_if.if_bpf, af, m);
#endif /*NBPFILTER > 0*/

	m->m_pkthdr.rcvif = &sc->sc_if;

	s = splnet();		/* possible */
	if (IF_QFULL(ifq)) {
		IF_DROP(ifq);
		m_freem(m);
	} else {
		IF_ENQUEUE(ifq, m);
	}
	/* we need schednetisr since the address family may change */
	schednetisr(isr);
	splx(s);

	return 1;	/* packet is done, no further processing needed */
}

/*
 * The output routine. Takes a packet and encapsulates it in the protocol
 * given by sc->sc_proto. See also RFC 1701 and RFC 2004
 */
static int
gre_output(struct ifnet *ifp, struct mbuf *m, const struct sockaddr *dst,
	   struct rtentry *rt)
{
	int error = 0, hlen;
	struct gre_softc *sc = ifp->if_softc;
	struct greip *gi;
	struct gre_h *gh;
	struct ip *eip, *ip;
	u_int8_t ip_tos = 0;
	u_int16_t etype = 0;
	struct mobile_h mob_h;

	if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) == 0 ||
	    sc->g_src.s_addr == INADDR_ANY || sc->g_dst.s_addr == INADDR_ANY) {
		m_freem(m);
		error = ENETDOWN;
		goto end;
	}

	gi = NULL;
	ip = NULL;

#if NBPFILTER >0
	if (ifp->if_bpf)
		bpf_mtap_af(ifp->if_bpf, dst->sa_family, m);
#endif

	m->m_flags &= ~(M_BCAST|M_MCAST);

	switch (sc->sc_proto) {
	case IPPROTO_MOBILE:
		if (dst->sa_family == AF_INET) {
			int msiz;

			if (M_UNWRITABLE(m, sizeof(*ip)) &&
			    (m = m_pullup(m, sizeof(*ip))) == NULL) {
				error = ENOBUFS;
				goto end;
			}
			ip = mtod(m, struct ip *);

			memset(&mob_h, 0, MOB_H_SIZ_L);
			mob_h.proto = (ip->ip_p) << 8;
			mob_h.odst = ip->ip_dst.s_addr;
			ip->ip_dst.s_addr = sc->g_dst.s_addr;

			/*
			 * If the packet comes from our host, we only change
			 * the destination address in the IP header.
			 * Else we also need to save and change the source
			 */
			if (in_hosteq(ip->ip_src, sc->g_src)) {
				msiz = MOB_H_SIZ_S;
			} else {
				mob_h.proto |= MOB_H_SBIT;
				mob_h.osrc = ip->ip_src.s_addr;
				ip->ip_src.s_addr = sc->g_src.s_addr;
				msiz = MOB_H_SIZ_L;
			}
			HTONS(mob_h.proto);
			mob_h.hcrc = gre_in_cksum((u_int16_t *)&mob_h, msiz);

			M_PREPEND(m, msiz, M_DONTWAIT);
			if (m == NULL) {
				error = ENOBUFS;
				goto end;
			}
			/* XXX Assuming that ip does not dangle after
			 * M_PREPEND.  In practice, that's true, but
			 * that's in M_PREPEND's contract.
			 */
			memmove(mtod(m, void *), ip, sizeof(*ip));
			ip = mtod(m, struct ip *);
			memcpy((void *)(ip + 1), &mob_h, (unsigned)msiz);
			ip->ip_len = htons(ntohs(ip->ip_len) + msiz);
		} else {  /* AF_INET */
			IF_DROP(&ifp->if_snd);
			m_freem(m);
			error = EINVAL;
			goto end;
		}
		break;
	case IPPROTO_UDP:
	case IPPROTO_GRE:
		GRE_DPRINTF(sc, "%s: dst->sa_family=%d\n", __func__,
		    dst->sa_family);
		switch (dst->sa_family) {
		case AF_INET:
			ip = mtod(m, struct ip *);
			ip_tos = ip->ip_tos;
			etype = ETHERTYPE_IP;
			break;
#ifdef NETATALK
		case AF_APPLETALK:
			etype = ETHERTYPE_ATALK;
			break;
#endif
#ifdef INET6
		case AF_INET6:
			etype = ETHERTYPE_IPV6;
			break;
#endif
		default:
			IF_DROP(&ifp->if_snd);
			m_freem(m);
			error = EAFNOSUPPORT;
			goto end;
		}
		break;
	default:
		IF_DROP(&ifp->if_snd);
		m_freem(m);
		error = EINVAL;
		goto end;
	}

	switch (sc->sc_proto) {
	case IPPROTO_GRE:
		hlen = sizeof(struct greip);
		break;
	case IPPROTO_UDP:
		hlen = sizeof(struct gre_h);
		break;
	default:
		hlen = 0;
		break;
	}

	M_PREPEND(m, hlen, M_DONTWAIT);

	if (m == NULL) {
		IF_DROP(&ifp->if_snd);
		error = ENOBUFS;
		goto end;
	}

	switch (sc->sc_proto) {
	case IPPROTO_UDP:
		gh = mtod(m, struct gre_h *);
		memset(gh, 0, sizeof(*gh));
		gh->ptype = htons(etype);
		/* XXX Need to handle IP ToS.  Look at how I handle IP TTL. */
		break;
	case IPPROTO_GRE:
		gi = mtod(m, struct greip *);
		gh = &gi->gi_g;
		eip = &gi->gi_i;
		/* we don't have any GRE flags for now */
		memset(gh, 0, sizeof(*gh));
		gh->ptype = htons(etype);
		eip->ip_src = sc->g_src;
		eip->ip_dst = sc->g_dst;
		eip->ip_hl = (sizeof(struct ip)) >> 2;
		eip->ip_ttl = ip_gre_ttl;
		eip->ip_tos = ip_tos;
		eip->ip_len = htons(m->m_pkthdr.len);
		eip->ip_p = sc->sc_proto;
		break;
	case IPPROTO_MOBILE:
		eip = mtod(m, struct ip *);
		eip->ip_p = sc->sc_proto;
		break;
	default:
		error = EPROTONOSUPPORT;
		m_freem(m);
		goto end;
	}

	ifp->if_opackets++;
	ifp->if_obytes += m->m_pkthdr.len;

	/* send it off */
	if (sc->sc_proto == IPPROTO_UDP) {
		if (IF_QFULL(&sc->sc_snd)) {
			IF_DROP(&sc->sc_snd);
			error = ENOBUFS;
			m_freem(m);
		} else {
			IF_ENQUEUE(&sc->sc_snd, m);
			gre_wakeup(sc);
			error = 0;
		}
		goto end;
	}
	if (sc->route.ro_rt == NULL)
		rtcache_init(&sc->route);
	else
		rtcache_check(&sc->route);
	if (sc->route.ro_rt == NULL)
		goto end;
	if (sc->route.ro_rt->rt_ifp->if_softc == sc)
		rtcache_free(&sc->route);
	else
		error = ip_output(m, NULL, &sc->route, 0,
		    (struct ip_moptions *)NULL, (struct socket *)NULL);
  end:
	if (error)
		ifp->if_oerrors++;
	return error;
}

/* gre_kick must be synchronized with network interrupts in order
 * to synchronize access to gre_softc members, so call it with
 * interrupt priority level set to IPL_NET or greater.
 */
static int
gre_kick(struct gre_softc *sc)
{
	int rc;
	struct ifnet *ifp = &sc->sc_if;

	if (sc->sc_proto == IPPROTO_UDP && (ifp->if_flags & IFF_UP) == IFF_UP &&
	    !sc->sc_thread) {
		sc->sc_thread = 1;
		rc = kthread_create1(PRI_NONE, false, gre_thread, (void *)sc,
		    NULL, ifp->if_xname);
		if (rc != 0)
			gre_stop(&sc->sc_thread);
		return rc;
	} else {
		gre_wakeup(sc);
		return 0;
	}
}

static int
gre_getname(struct socket *so, int req, struct mbuf *nam, struct lwp *l)
{
	int s, error;

	s = splsoftnet();
	error = (*so->so_proto->pr_usrreq)(so, req, (struct mbuf *)0,
	    nam, (struct mbuf *)0, l);
	splx(s);
	return error;
}

static int
gre_getsockname(struct socket *so, struct mbuf *nam, struct lwp *l)
{
	return gre_getname(so, PRU_SOCKADDR, nam, l);
}

static int
gre_getpeername(struct socket *so, struct mbuf *nam, struct lwp *l)
{
	return gre_getname(so, PRU_PEERADDR, nam, l);
}

static int
gre_getnames(struct socket *so, struct lwp *l, struct sockaddr_in *src,
    struct sockaddr_in *dst)
{
	struct mbuf *m;
	struct sockaddr_in *sin;
	int rc;

	if ((m = gre_getsockmbuf(so)) == NULL)
		return ENOBUFS;

	sin = mtod(m, struct sockaddr_in *);

	if ((rc = gre_getsockname(so, m, l)) != 0)
		goto out;
	if (sin->sin_family != AF_INET) {
		rc = EAFNOSUPPORT;
		goto out;
	}
	*src = *sin;

	if ((rc = gre_getpeername(so, m, l)) != 0)
		goto out;
	if (sin->sin_family != AF_INET) {
		rc = EAFNOSUPPORT;
		goto out;
	}
	*dst = *sin;

out:
	m_freem(m);
	return rc;
}

static int
gre_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	u_char oproto;
	struct file *fp, *ofp;
	struct socket *so;
	struct sockaddr_in dst, src;
	struct proc *p = curproc;	/* XXX */
	struct lwp *l = curlwp;	/* XXX */
	struct ifreq *ifr = (struct ifreq *)data;
	struct if_laddrreq *lifr = (struct if_laddrreq *)data;
	struct gre_softc *sc = ifp->if_softc;
	int s;
	struct sockaddr_in si;
	struct sockaddr *sa = NULL;
	int error = 0;

	switch (cmd) {
	case SIOCSIFFLAGS:
	case SIOCSIFMTU:
	case GRESPROTO:
	case GRESADDRD:
	case GRESADDRS:
	case GRESSOCK:
	case GREDSOCK:
	case SIOCSLIFPHYADDR:
	case SIOCDIFPHYADDR:
		if (kauth_authorize_network(l->l_cred, KAUTH_NETWORK_INTERFACE,
		    KAUTH_REQ_NETWORK_INTERFACE_SETPRIV, ifp, (void *)cmd,
		    NULL) != 0)
			return EPERM;
		break;
	default:
		break;
	}

	s = splnet();
	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		if ((error = gre_kick(sc)) != 0)
			ifp->if_flags &= ~IFF_UP;
		break;
	case SIOCSIFDSTADDR:
		break;
	case SIOCSIFFLAGS:
		oproto = sc->sc_proto;
		switch (ifr->ifr_flags & (IFF_LINK0|IFF_LINK2)) {
		case IFF_LINK0|IFF_LINK2:
			sc->sc_proto = IPPROTO_UDP;
			if (oproto != IPPROTO_UDP)
				ifp->if_flags &= ~IFF_RUNNING;
			error = gre_kick(sc);
			break;
		case IFF_LINK0:
			sc->sc_proto = IPPROTO_GRE;
			gre_wakeup(sc);
			goto recompute;
		case 0:
			sc->sc_proto = IPPROTO_MOBILE;
			gre_wakeup(sc);
			goto recompute;
		}
		break;
	case SIOCSIFMTU:
		if (ifr->ifr_mtu < 576) {
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
#ifdef INET6
		case AF_INET6:
			break;
#endif
		default:
			error = EAFNOSUPPORT;
			break;
		}
		break;
	case GRESPROTO:
		oproto = sc->sc_proto;
		sc->sc_proto = ifr->ifr_flags;
		switch (sc->sc_proto) {
		case IPPROTO_UDP:
			ifp->if_flags |= IFF_LINK0|IFF_LINK2;
			if (oproto != IPPROTO_UDP)
				ifp->if_flags &= ~IFF_RUNNING;
			error = gre_kick(sc);
			break;
		case IPPROTO_GRE:
			ifp->if_flags |= IFF_LINK0;
			ifp->if_flags &= ~IFF_LINK2;
			goto recompute;
		case IPPROTO_MOBILE:
			ifp->if_flags &= ~(IFF_LINK0|IFF_LINK2);
			goto recompute;
		default:
			error = EPROTONOSUPPORT;
			break;
		}
		break;
	case GREGPROTO:
		ifr->ifr_flags = sc->sc_proto;
		break;
	case GRESADDRS:
	case GRESADDRD:
		/*
		 * set tunnel endpoints, compute a less specific route
		 * to the remote end and mark if as up
		 */
		sa = &ifr->ifr_addr;
		if (cmd == GRESADDRS) {
			sc->g_src = (satosin(sa))->sin_addr;
			sc->g_srcport = satosin(sa)->sin_port;
		}
		if (cmd == GRESADDRD) {
			if (sc->sc_proto == IPPROTO_UDP &&
			    satosin(sa)->sin_port == 0) {
				error = EINVAL;
				break;
			}
			sc->g_dst = (satosin(sa))->sin_addr;
			sc->g_dstport = satosin(sa)->sin_port;
		}
	recompute:
		if (sc->sc_proto == IPPROTO_UDP ||
		    (sc->g_src.s_addr != INADDR_ANY &&
		     sc->g_dst.s_addr != INADDR_ANY)) {
			if (sc->sc_fp != NULL) {
				closef(sc->sc_fp, l);
				sc->sc_fp = NULL;
			}
			rtcache_free(&sc->route);
			if (sc->sc_proto == IPPROTO_UDP)
				error = gre_kick(sc);
			else if (gre_compute_route(sc) == 0)
				ifp->if_flags |= IFF_RUNNING;
			else
				ifp->if_flags &= ~IFF_RUNNING;
		}
		break;
	case GREGADDRS:
		memset(&si, 0, sizeof(si));
		si.sin_family = AF_INET;
		si.sin_len = sizeof(struct sockaddr_in);
		si.sin_addr.s_addr = sc->g_src.s_addr;
		sa = sintosa(&si);
		ifr->ifr_addr = *sa;
		break;
	case GREGADDRD:
		memset(&si, 0, sizeof(si));
		si.sin_family = AF_INET;
		si.sin_len = sizeof(struct sockaddr_in);
		si.sin_addr.s_addr = sc->g_dst.s_addr;
		sa = sintosa(&si);
		ifr->ifr_addr = *sa;
		break;
	case GREDSOCK:
		if (sc->sc_proto != IPPROTO_UDP)
			return EINVAL;
		if (sc->sc_fp != NULL) {
			closef(sc->sc_fp, l);
			sc->sc_fp = NULL;
			error = gre_kick(sc);
		}
		break;
	case GRESSOCK:
		if (sc->sc_proto != IPPROTO_UDP)
			return EINVAL;
		/* getsock() will FILE_USE() the descriptor for us */
		if ((error = getsock(p->p_fd, (int)ifr->ifr_value, &fp)) != 0)
			break;
		so = (struct socket *)fp->f_data;
		if (so->so_type != SOCK_DGRAM) {
			FILE_UNUSE(fp, NULL);
			error = EINVAL;
			break;
		}
		/* check address */
		if ((error = gre_getnames(so, curlwp, &src, &dst)) != 0) {
			FILE_UNUSE(fp, NULL);
			break;
		}

		fp->f_count++;

		ofp = sc->sc_fp;
		sc->sc_fp = fp;
		if ((error = gre_kick(sc)) != 0) {
			closef(fp, l);
			sc->sc_fp = ofp;
			break;
		}
		sc->g_src = src.sin_addr;
		sc->g_srcport = src.sin_port;
		sc->g_dst = dst.sin_addr;
		sc->g_dstport = dst.sin_port;
		if (ofp != NULL)
			closef(ofp, l);
		break;
	case SIOCSLIFPHYADDR:
		if (lifr->addr.ss_family != AF_INET ||
		    lifr->dstaddr.ss_family != AF_INET) {
			error = EAFNOSUPPORT;
			break;
		}
		if (lifr->addr.ss_len != sizeof(si) ||
		    lifr->dstaddr.ss_len != sizeof(si)) {
			error = EINVAL;
			break;
		}
		sc->g_src = satosin(&lifr->addr)->sin_addr;
		sc->g_dst = satosin(&lifr->dstaddr)->sin_addr;
		sc->g_srcport = satosin(&lifr->addr)->sin_port;
		sc->g_dstport = satosin(&lifr->dstaddr)->sin_port;
		goto recompute;
	case SIOCDIFPHYADDR:
		sc->g_src.s_addr = INADDR_ANY;
		sc->g_dst.s_addr = INADDR_ANY;
		sc->g_srcport = 0;
		sc->g_dstport = 0;
		goto recompute;
	case SIOCGLIFPHYADDR:
		if (sc->g_src.s_addr == INADDR_ANY ||
		    sc->g_dst.s_addr == INADDR_ANY) {
			error = EADDRNOTAVAIL;
			break;
		}
		memset(&si, 0, sizeof(si));
		si.sin_family = AF_INET;
		si.sin_len = sizeof(struct sockaddr_in);
		si.sin_addr = sc->g_src;
		if (sc->sc_proto == IPPROTO_UDP)
			si.sin_port = sc->g_srcport;
		memcpy(&lifr->addr, &si, sizeof(si));
		si.sin_addr = sc->g_dst;
		if (sc->sc_proto == IPPROTO_UDP)
			si.sin_port = sc->g_dstport;
		memcpy(&lifr->dstaddr, &si, sizeof(si));
		break;
	default:
		error = EINVAL;
		break;
	}
	splx(s);
	return error;
}

/*
 * Compute a route to our destination.
 */
static int
gre_compute_route(struct gre_softc *sc)
{
	struct route *ro;

	ro = &sc->route;

	memset(ro, 0, sizeof(struct route));
	satosin(&ro->ro_dst)->sin_addr = sc->g_dst;
	ro->ro_dst.sa_family = AF_INET;
	ro->ro_dst.sa_len = sizeof(ro->ro_dst);

#ifdef DIAGNOSTIC
	printf("%s: searching for a route to %s", sc->sc_if.if_xname,
	    inet_ntoa(satocsin(rtcache_getdst(ro))->sin_addr));
#endif

	rtcache_init(ro);

	if (ro->ro_rt == NULL || ro->ro_rt->rt_ifp->if_softc == sc) {
#ifdef DIAGNOSTIC
		if (ro->ro_rt == NULL)
			printf(" - no route found!\n");
		else
			printf(" - route loops back to ourself!\n");
#endif
		rtcache_free(ro);
		return EADDRNOTAVAIL;
	}

	return 0;
}

/*
 * do a checksum of a buffer - much like in_cksum, which operates on
 * mbufs.
 */
u_int16_t
gre_in_cksum(u_int16_t *p, u_int len)
{
	u_int32_t sum = 0;
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
	return ~sum;
}
#endif

void	greattach(int);

/* ARGSUSED */
void
greattach(int count)
{
#ifdef INET
	LIST_INIT(&gre_softc_list);
	if_clone_attach(&gre_cloner);
#endif
}
