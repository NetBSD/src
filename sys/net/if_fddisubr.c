/*	$NetBSD: if_fddisubr.c,v 1.38.2.2 2001/08/24 00:12:10 nathanw Exp $	*/

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
 * Copyright (c) 1995, 1996
 *	Matt Thomas <matt@3am-software.com>.  All rights reserved.
 * Copyright (c) 1982, 1989, 1993
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
 *	@(#)if_fddisubr.c	8.1 (Berkeley) 6/10/93
 *
 * Id: if_fddisubr.c,v 1.15 1997/03/21 22:35:50 thomas Exp
 */
#include "opt_inet.h"
#include "opt_atalk.h"
#include "opt_ccitt.h"
#include "opt_llc.h"
#include "opt_iso.h"
#include "opt_ns.h"

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>

#include <machine/cpu.h>

#include <net/if.h>
#include <net/netisr.h>
#include <net/route.h>
#include <net/if_llc.h>
#include <net/if_dl.h>
#include <net/if_types.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_var.h>
#if defined(__NetBSD__)
#include <netinet/if_inarp.h>
#include "opt_gateway.h"
#else
#include <netinet/if_ether.h>
#endif
#endif
#if defined(__FreeBSD__)
#include <netinet/if_fddi.h>
#else
#include <net/if_fddi.h>
#endif

#ifdef IPX
#include <netipx/ipx.h> 
#include <netipx/ipx_if.h>
#endif

#ifdef INET6
#ifndef INET
#include <netinet/in.h>
#include <netinet/in_var.h>
#endif
#include <netinet6/nd6.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#ifdef DECNET
#include <netdnet/dn.h>
#endif

#ifdef ISO
#include <netiso/argo_debug.h>
#include <netiso/iso.h>
#include <netiso/iso_var.h>
#include <netiso/iso_snpac.h>
#endif

#ifdef LLC
#include <netccitt/dll.h>
#include <netccitt/llc_var.h>
#endif

#ifdef NETATALK
#include <netatalk/at.h>
#include <netatalk/at_var.h>
#include <netatalk/at_extern.h>

#define llc_snap_org_code llc_un.type_snap.org_code
#define llc_snap_ether_type llc_un.type_snap.ether_type

extern u_char	at_org_code[ 3 ];
extern u_char	aarp_org_code[ 3 ];
#endif /* NETATALK */

#if defined(LLC) && defined(CCITT)
extern struct ifqueue pkintrq;
#endif

#include "bpfilter.h"

#define senderr(e) { error = (e); goto bad;}

/*
 * This really should be defined in if_llc.h but in case it isn't.
 */
#ifndef llc_snap
#define	llc_snap	llc_un.type_snap
#endif

#if defined(__bsdi__) || defined(__NetBSD__)
#define	RTALLOC1(a, b)			rtalloc1(a, b)
#define	ARPRESOLVE(a, b, c, d, e, f)	arpresolve(a, b, c, d, e)
#define	ETYPEHTONS(t)			(t)
#elif defined(__FreeBSD__)
#define	RTALLOC1(a, b)			rtalloc1(a, b, 0UL)
#define	ARPRESOLVE(a, b, c, d, e, f)	arpresolve(a, b, c, d, e, f)
#define	ETYPEHTONS(t)			(t)
#endif

#if defined(__NetBSD__)
#define	FDDIADDR(ifp)		LLADDR((ifp)->if_sadl)
#else
#define	FDDICOM(ifp)		((struct arpcom *)(ifp))
#define	FDDIADDR(ifp)		(FDDICOM(ifp)->ac_enaddr)
#endif

static	int fddi_output __P((struct ifnet *, struct mbuf *,
	    struct sockaddr *, struct rtentry *)); 
static	void fddi_input __P((struct ifnet *, struct mbuf *));

/*
 * FDDI output routine.
 * Encapsulate a packet of type family for the local net.
 * Assumes that ifp is actually pointer to ethercom structure.
 */
static int
fddi_output(ifp, m0, dst, rt0)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
	struct rtentry *rt0;
{
	u_int16_t etype;
	int s, len, error = 0, hdrcmplt = 0;
 	u_char esrc[6], edst[6];
	struct mbuf *m = m0;
	struct rtentry *rt;
	struct fddi_header *fh;
	struct mbuf *mcopy = (struct mbuf *)0;
	ALTQ_DECL(struct altq_pktattr pktattr;)
	short mflags;

	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
		senderr(ENETDOWN);
#if !defined(__bsdi__) || _BSDI_VERSION >= 199401
	if ((rt = rt0) != NULL) {
		if ((rt->rt_flags & RTF_UP) == 0) {
			if ((rt0 = rt = RTALLOC1(dst, 1)) != NULL)
				rt->rt_refcnt--;
			else 
				senderr(EHOSTUNREACH);
		}
		if (rt->rt_flags & RTF_GATEWAY) {
			if (rt->rt_gwroute == 0)
				goto lookup;
			if (((rt = rt->rt_gwroute)->rt_flags & RTF_UP) == 0) {
				rtfree(rt); rt = rt0;
			lookup: rt->rt_gwroute = RTALLOC1(rt->rt_gateway, 1);
				if ((rt = rt->rt_gwroute) == 0)
					senderr(EHOSTUNREACH);
			}
		}
		if (rt->rt_flags & RTF_REJECT)
			if (rt->rt_rmx.rmx_expire == 0 ||
			    time.tv_sec < rt->rt_rmx.rmx_expire)
				senderr(rt == rt0 ? EHOSTDOWN : EHOSTUNREACH);
	}
#endif

	/*
	 * If the queueing discipline needs packet classification,
	 * do it before prepending link headers.
	 */
	IFQ_CLASSIFY(&ifp->if_snd, m, dst->sa_family, &pktattr);

	switch (dst->sa_family) {

#ifdef INET
	case AF_INET: {
#if !defined(__bsdi__) || _BSDI_VERSION >= 199401
#if defined(__NetBSD__)
#define SIN(x) ((struct sockaddr_in *)(x))
		if (m->m_flags & M_BCAST)
                	bcopy((caddr_t)fddibroadcastaddr, (caddr_t)edst,
				sizeof(edst));

		else if (m->m_flags & M_MCAST) {
			ETHER_MAP_IP_MULTICAST(&SIN(dst)->sin_addr,
			    (caddr_t)edst)
		} else if (!arpresolve(ifp, rt, m, dst, edst))
#else
		if (!ARPRESOLVE(FDDICOM(ifp), rt, m, dst, edst, rt0))
#endif
			return (0);	/* if not yet resolved */
#else
		int usetrailers;
		if (!arpresolve(FDDICOM(ifp), m, &((struct sockaddr_in *)dst)->sin_addr, edst, &usetrailers))
			return (0);	/* if not yet resolved */
#endif
		/* If broadcasting on a simplex interface, loopback a copy */
		if ((m->m_flags & M_BCAST) && (ifp->if_flags & IFF_SIMPLEX))
			mcopy = m_copy(m, 0, (int)M_COPYALL);
		etype = htons(ETHERTYPE_IP);
		break;
	}
#endif
#ifdef INET6
	case AF_INET6:
#ifdef OLDIP6OUTPUT
		if (!nd6_resolve(ifp, rt, m, dst, edst))
			return (0);	/* if not yet resolved */
#else
		if (!nd6_storelladdr(ifp, rt, m, dst, (u_char *)edst)){
			/* something bad happened */
			return(0);
		}
#endif /* OLDIP6OUTPUT */
		etype = htons(ETHERTYPE_IPV6);
		break;
#endif
#ifdef AF_ARP
	case AF_ARP: {
		struct arphdr *ah = mtod(m, struct arphdr *);
		if (m->m_flags & M_BCAST)
                	bcopy((caddr_t)etherbroadcastaddr, (caddr_t)edst,
				sizeof(edst));
		else
			bcopy((caddr_t)ar_tha(ah),
				(caddr_t)edst, sizeof(edst));
		
		ah->ar_hrd = htons(ARPHRD_ETHER);

		switch (ntohs(ah->ar_op)) {
		case ARPOP_REVREQUEST:
		case ARPOP_REVREPLY:
			etype = htons(ETHERTYPE_REVARP);
			break;

		case ARPOP_REQUEST:
		case ARPOP_REPLY:
		default:
			etype = htons(ETHERTYPE_ARP);
		}

		break;
	}
#endif /* AF_ARP */
#ifdef IPX
	case AF_IPX:
		etype = htons(ETHERTYPE_IPX);
 		bcopy((caddr_t)&(((struct sockaddr_ipx *)dst)->sipx_addr.x_host),
		    (caddr_t)edst, sizeof (edst));
		/* If broadcasting on a simplex interface, loopback a copy */
		if ((m->m_flags & M_BCAST) && (ifp->if_flags & IFF_SIMPLEX))
			mcopy = m_copy(m, 0, (int)M_COPYALL);
		break;
#endif
#ifdef NETATALK
	case AF_APPLETALK: {
		struct at_ifaddr *aa;
		if (!aarpresolve(ifp, m, (struct sockaddr_at *)dst, edst)) {
#ifdef NETATALKDEBUG
			printf("aarpresolv: failed\n");
#endif
			return (0);
		}
		/*
		 * ifaddr is the first thing in at_ifaddr
		 */
		if ((aa = (struct at_ifaddr *)at_ifawithnet(
		    (struct sockaddr_at *)dst, ifp)) == NULL)
			goto bad;
	    
		/*
		 * In the phase 2 case, we need to prepend an mbuf for the llc
		 * header. Since we must preserve the value of m, which is
		 * passed to us by value, we m_copy() the first mbuf, and use
		 * it for our llc header.
		 */
		if (aa->aa_flags & AFA_PHASE2) {
			struct llc llc;

			M_PREPEND(m, sizeof(struct llc), M_NOWAIT);
			if (m == 0)
				senderr(ENOBUFS);
			llc.llc_dsap = llc.llc_ssap = LLC_SNAP_LSAP;
			llc.llc_control = LLC_UI;
			bcopy(at_org_code, llc.llc_snap_org_code,
			    sizeof(at_org_code));
			llc.llc_snap_ether_type = htons(ETHERTYPE_ATALK);
			bcopy(&llc, mtod(m, caddr_t), sizeof(struct llc));
			etype = 0;
		} else {
			etype = htons(ETHERTYPE_ATALK);
		}
		break;
	}
#endif /* NETATALK */
#ifdef NS
	case AF_NS:
		etype = htons(ETHERTYPE_NS);
 		bcopy((caddr_t)&(((struct sockaddr_ns *)dst)->sns_addr.x_host),
		    (caddr_t)edst, sizeof (edst));
		if (!bcmp((caddr_t)edst, (caddr_t)&ns_thishost, sizeof(edst)))
			return (looutput(ifp, m, dst, rt));
		/* If broadcasting on a simplex interface, loopback a copy */
		if ((m->m_flags & M_BCAST) && (ifp->if_flags & IFF_SIMPLEX))
			mcopy = m_copy(m, 0, (int)M_COPYALL);
		break;
#endif
#ifdef	ISO
	case AF_ISO: {
		int	snpalen;
		struct	llc *l;
		struct sockaddr_dl *sdl;

		if (rt && (sdl = (struct sockaddr_dl *)rt->rt_gateway) &&
		    sdl->sdl_family == AF_LINK && sdl->sdl_alen > 0) {
			bcopy(LLADDR(sdl), (caddr_t)edst, sizeof(edst));
		} else if ((error =
			    iso_snparesolve(ifp, (struct sockaddr_iso *)dst,
					    (char *)edst, &snpalen)) != 0)
			goto bad; /* Not Resolved */
		/* If broadcasting on a simplex interface, loopback a copy */
		if (*edst & 1)
			m->m_flags |= (M_BCAST|M_MCAST);
		if ((m->m_flags & M_BCAST) && (ifp->if_flags & IFF_SIMPLEX) &&
		    (mcopy = m_copy(m, 0, (int)M_COPYALL))) {
			M_PREPEND(mcopy, sizeof (*fh), M_DONTWAIT);
			if (mcopy) {
				fh = mtod(mcopy, struct fddi_header *);
				bcopy((caddr_t)edst,
				      (caddr_t)fh->fddi_dhost, sizeof (edst));
				bcopy((caddr_t)FDDIADDR(ifp),
				      (caddr_t)fh->fddi_shost, sizeof (edst));
			}
		}
		M_PREPEND(m, 3, M_DONTWAIT);
		if (m == NULL)
			return (0);
		etype = 0;
		l = mtod(m, struct llc *);
		l->llc_dsap = l->llc_ssap = LLC_ISO_LSAP;
		l->llc_control = LLC_UI;
#if defined(__FreeBSD__)
		IFDEBUG(D_ETHER)
			int i;
			printf("unoutput: sending pkt to: ");
			for (i=0; i<6; i++)
				printf("%x ", edst[i] & 0xff);
			printf("\n");
		ENDDEBUG
#endif
		} break;
#endif /* ISO */
#ifdef	LLC
/*	case AF_NSAP: */
	case AF_CCITT: {
		struct sockaddr_dl *sdl = 
			(struct sockaddr_dl *) rt->rt_gateway;

		if (sdl && sdl->sdl_family == AF_LINK
		    && sdl->sdl_alen > 0) {
			bcopy(LLADDR(sdl), (char *)edst, sizeof(edst));
		} else goto bad; /* Not a link interface ? Funny ... */

		if ((ifp->if_flags & IFF_SIMPLEX) && (*edst & 1) &&
		    (mcopy = m_copy(m, 0, (int)M_COPYALL))) {
			M_PREPEND(mcopy, sizeof (*fh), M_DONTWAIT);
			if (mcopy) {
				fh = mtod(mcopy, struct fddi_header *);
				bcopy((caddr_t)edst,
				      (caddr_t)fh->fddi_dhost, sizeof (edst));
				bcopy((caddr_t)FDDIADDR(ifp),
				      (caddr_t)fh->fddi_shost, sizeof (edst));
				fh->fddi_fc = FDDIFC_LLC_ASYNC|FDDIFC_LLC_PRIO4;
			}
		}
		etype = 0;
#ifdef LLC_DEBUG
		{
			int i;
			struct llc *l = mtod(m, struct llc *);

			printf("fddi_output: sending LLC2 pkt to: ");
			for (i=0; i<6; i++)
				printf("%x ", edst[i] & 0xff);
			printf(" len 0x%x dsap 0x%x ssap 0x%x control 0x%x\n", 
			       m->m_pkthdr.len, l->llc_dsap & 0xff, l->llc_ssap &0xff,
			       l->llc_control & 0xff);

		}
#endif /* LLC_DEBUG */
		} break;
#endif /* LLC */	

	case pseudo_AF_HDRCMPLT:
	{
		struct fddi_header *fh = (struct fddi_header *)dst->sa_data;
		hdrcmplt = 1;
		bcopy((caddr_t)fh->fddi_shost, (caddr_t)esrc, sizeof (esrc));
		/*FALLTHROUGH*/
	}

	case AF_LINK:
	{
		struct fddi_header *fh = (struct fddi_header *)dst->sa_data;
 		bcopy((caddr_t)fh->fddi_dhost, (caddr_t)edst, sizeof (edst));
		if (*edst & 1)
			m->m_flags |= (M_BCAST|M_MCAST);
		etype = 0;
		break;
	}

	case AF_UNSPEC:
	{
		struct ether_header *eh;
		eh = (struct ether_header *)dst->sa_data;
 		bcopy((caddr_t)eh->ether_dhost, (caddr_t)edst, sizeof (edst));
		if (*edst & 1)
			m->m_flags |= (M_BCAST|M_MCAST);
		etype = ETYPEHTONS(eh->ether_type);
		break;
	}

#if NBPFILTER > 0
	case AF_IMPLINK:
	{
		fh = mtod(m, struct fddi_header *);
		error = EPROTONOSUPPORT;
		switch (fh->fddi_fc & (FDDIFC_C|FDDIFC_L|FDDIFC_F)) {
			case FDDIFC_LLC_ASYNC: {
				/* legal priorities are 0 through 7 */
				if ((fh->fddi_fc & FDDIFC_Z) > 7)
			        	goto bad;
				break;
			}
			case FDDIFC_LLC_SYNC: {
				/* FDDIFC_Z bits reserved, must be zero */
				if (fh->fddi_fc & FDDIFC_Z)
					goto bad;
				break;
			}
			case FDDIFC_SMT: {
				/* FDDIFC_Z bits must be non zero */
				if ((fh->fddi_fc & FDDIFC_Z) == 0)
					goto bad;
				break;
			}
			default: {
				/* anything else is too dangerous */
               	 		goto bad;
			}
		}
		error = 0;
		if (fh->fddi_dhost[0] & 1)
			m->m_flags |= (M_BCAST|M_MCAST);
		goto queue_it;
	}
#endif
	default:
#if defined(__NetBSD__)
		printf("%s: can't handle af%d\n", ifp->if_xname,
		       dst->sa_family);
#else
		printf("%s%d: can't handle af%d\n", ifp->if_name, ifp->if_unit,
		       dst->sa_family);
#endif
		senderr(EAFNOSUPPORT);
	}


	if (mcopy)
		(void) looutput(ifp, mcopy, dst, rt);
	if (etype != 0) {
		struct llc *l;
		M_PREPEND(m, sizeof (struct llc), M_DONTWAIT);
		if (m == 0)
			senderr(ENOBUFS);
		l = mtod(m, struct llc *);
		l->llc_control = LLC_UI;
		l->llc_dsap = l->llc_ssap = LLC_SNAP_LSAP;
		l->llc_snap.org_code[0] = l->llc_snap.org_code[1] = l->llc_snap.org_code[2] = 0;
		bcopy((caddr_t) &etype, (caddr_t) &l->llc_snap.ether_type,
			sizeof(u_int16_t));
	}
	/*
	 * Add local net header.  If no space in first mbuf,
	 * allocate another.
	 */
	M_PREPEND(m, sizeof (struct fddi_header), M_DONTWAIT);
	if (m == 0)
		senderr(ENOBUFS);
	fh = mtod(m, struct fddi_header *);
	fh->fddi_fc = FDDIFC_LLC_ASYNC|FDDIFC_LLC_PRIO4;
 	bcopy((caddr_t)edst, (caddr_t)fh->fddi_dhost, sizeof (edst));
#if NBPFILTER > 0
  queue_it:
#endif
	if (hdrcmplt)
		bcopy((caddr_t)esrc, (caddr_t)fh->fddi_shost,
		    sizeof(fh->fddi_shost));
	else
		bcopy((caddr_t)FDDIADDR(ifp), (caddr_t)fh->fddi_shost,
		    sizeof(fh->fddi_shost));
	mflags = m->m_flags;
	len = m->m_pkthdr.len;
	s = splnet();
	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	IFQ_ENQUEUE(&ifp->if_snd, m, &pktattr, error);
	if (error) {
		/* mbuf is already freed */
		splx(s);
		return (error);
	}
	ifp->if_obytes += len;
	if (mflags & M_MCAST)
		ifp->if_omcasts++;
	if ((ifp->if_flags & IFF_OACTIVE) == 0)
		(*ifp->if_start)(ifp);
	splx(s);
	return (error);

bad:
	if (m)
		m_freem(m);
	return (error);
}

/*
 * Process a received FDDI packet;
 * the packet is in the mbuf chain m with
 * the fddi header.
 */
static void
fddi_input(ifp, m)
	struct ifnet *ifp;
	struct mbuf *m;
{
	struct ifqueue *inq;
	struct llc *l;
	struct fddi_header *fh;
	int s;

	if ((ifp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		return;
	}

	fh = mtod(m, struct fddi_header *);

	ifp->if_ibytes += m->m_pkthdr.len;
	if (fh->fddi_dhost[0] & 1) {
		if (bcmp((caddr_t)fddibroadcastaddr, (caddr_t)fh->fddi_dhost,
		    sizeof(fddibroadcastaddr)) == 0)
			m->m_flags |= M_BCAST;
		else
			m->m_flags |= M_MCAST;
		ifp->if_imcasts++;
	} else if ((ifp->if_flags & IFF_PROMISC)
	    && bcmp(FDDIADDR(ifp), (caddr_t)fh->fddi_dhost,
		    sizeof(fh->fddi_dhost)) != 0) {
		m_freem(m);
		return;
	}

#ifdef M_LINK0
	/*
	 * If this has a LLC priority of 0, then mark it so upper
	 * layers have a hint that it really came via a FDDI/Ethernet
	 * bridge.
	 */
	if ((fh->fddi_fc & FDDIFC_LLC_PRIO7) == FDDIFC_LLC_PRIO0)
		m->m_flags |= M_LINK0;
#endif

	/* Strip off the FDDI header. */
	m_adj(m, sizeof(struct fddi_header));

	l = mtod(m, struct llc *);
	switch (l->llc_dsap) {
#if defined(INET) || defined(INET6) || defined(NS) || defined(DECNET) || defined(IPX) || defined(NETATALK)
	case LLC_SNAP_LSAP:
	{
		u_int16_t etype;
		if (l->llc_control != LLC_UI || l->llc_ssap != LLC_SNAP_LSAP)
			goto dropanyway;
#ifdef NETATALK
		if (Bcmp(&(l->llc_snap_org_code)[0], at_org_code,
			 sizeof(at_org_code)) == 0 &&
		 	ntohs(l->llc_snap_ether_type) == ETHERTYPE_ATALK) {
		    inq = &atintrq2;
		    m_adj( m, sizeof( struct llc ));
		    schednetisr(NETISR_ATALK);
		    break;
		}

		if (Bcmp(&(l->llc_snap_org_code)[0], aarp_org_code,
			 sizeof(aarp_org_code)) == 0 &&
			ntohs(l->llc_snap_ether_type) == ETHERTYPE_AARP) {
		    m_adj( m, sizeof( struct llc ));
		    aarpinput(ifp, m); /* XXX */
		    return;
		}
#endif /* NETATALK */
		if (l->llc_snap.org_code[0] != 0 || l->llc_snap.org_code[1] != 0|| l->llc_snap.org_code[2] != 0)
			goto dropanyway;
		etype = ntohs(l->llc_snap.ether_type);
		m_adj(m, 8);
		switch (etype) {
#ifdef INET
		case ETHERTYPE_IP:
#ifdef GATEWAY
			if (ipflow_fastforward(m))
				return;
#endif
			schednetisr(NETISR_IP);
			inq = &ipintrq;
			break;

		case ETHERTYPE_ARP:
#if !defined(__bsdi__) || _BSDI_VERSION >= 199401
			schednetisr(NETISR_ARP);
			inq = &arpintrq;
			break;
#else
			arpinput(ifp, m);
			return;
#endif
#endif
#ifdef IPX      
		case ETHERTYPE_IPX: 
			schednetisr(NETISR_IPX);
			inq = &ipxintrq;
			break;  
#endif   
#ifdef NS
		case ETHERTYPE_NS:
			schednetisr(NETISR_NS);
			inq = &nsintrq;
			break;
#endif
#ifdef INET6
		case ETHERTYPE_IPV6:
			schednetisr(NETISR_IPV6);
			inq = &ip6intrq;
			break;

#endif
#ifdef DECNET
		case ETHERTYPE_DECNET:
			schednetisr(NETISR_DECNET);
			inq = &decnetintrq;
			break;
#endif
#ifdef NETATALK 
		case ETHERTYPE_ATALK:
	                schednetisr(NETISR_ATALK);
			inq = &atintrq1;
			break;
	        case ETHERTYPE_AARP:
			/* probably this should be done with a NETISR as well */
			aarpinput(ifp, m); /* XXX */
			return;
#endif /* NETATALK */
		default:
			ifp->if_noproto++;
			goto dropanyway;
		}
		break;
	}
#endif /* INET || NS */
#ifdef	ISO
	case LLC_ISO_LSAP: 
		switch (l->llc_control) {
		case LLC_UI:
			/* LLC_UI_P forbidden in class 1 service */
			if ((l->llc_dsap == LLC_ISO_LSAP) &&
			    (l->llc_ssap == LLC_ISO_LSAP)) {
				/* LSAP for ISO */
				m->m_data += 3;		/* XXX */
				m->m_len -= 3;		/* XXX */
				m->m_pkthdr.len -= 3;	/* XXX */
				M_PREPEND(m, sizeof *fh, M_DONTWAIT);
				if (m == 0)
					return;
				*mtod(m, struct fddi_header *) = *fh;
#if defined(__FreeBSD__)
				IFDEBUG(D_ETHER)
					printf("clnp packet");
				ENDDEBUG
#endif
				schednetisr(NETISR_ISO);
				inq = &clnlintrq;
				break;
			}
			goto dropanyway;
			
		case LLC_XID:
		case LLC_XID_P:
			if(m->m_len < 6)
				goto dropanyway;
			l->llc_window = 0;
			l->llc_fid = 9;
			l->llc_class = 1;
			l->llc_dsap = l->llc_ssap = 0;
			/* Fall through to */
		case LLC_TEST:
		case LLC_TEST_P:
		{
			struct sockaddr sa;
			struct ether_header *eh;
			int i;
			u_char c = l->llc_dsap;

			l->llc_dsap = l->llc_ssap;
			l->llc_ssap = c;
			eh = (struct ether_header *)sa.sa_data;
			if (m->m_flags & (M_BCAST | M_MCAST))
				bcopy((caddr_t)FDDIADDR(ifp),
				      (caddr_t)eh->ether_dhost, 6);
			sa.sa_family = AF_UNSPEC;
			sa.sa_len = sizeof(sa);
			for (i = 0; i < 6; i++) {
				eh->ether_shost[i] = fh->fddi_dhost[i];
				eh->ether_dhost[i] = fh->fddi_shost[i];
			}
			eh->ether_type = 0;
			ifp->if_output(ifp, m, &sa, NULL);
			return;
		}
		default:
			m_freem(m);
			return;
		}
		break;
#endif /* ISO */
#ifdef LLC
	case LLC_X25_LSAP:
	{
		M_PREPEND(m, sizeof(struct sdl_hdr) , M_DONTWAIT);
		if (m == 0)
			return;
		if ( !sdl_sethdrif(ifp, fh->fddi_shost, LLC_X25_LSAP,
				    fh->fddi_dhost, LLC_X25_LSAP, 6, 
				    mtod(m, struct sdl_hdr *)))
			panic("ETHER cons addr failure");
		mtod(m, struct sdl_hdr *)->sdlhdr_len = m->m_pkthdr.len - sizeof(struct sdl_hdr);
#ifdef LLC_DEBUG
		printf("llc packet\n");
#endif /* LLC_DEBUG */
		schednetisr(NETISR_CCITT);
		inq = &llcintrq;
		break;
	}
#endif /* LLC */
		
	default:
		ifp->if_noproto++;
	dropanyway:
		m_freem(m);
		return;
	}

	s = splnet();
	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		m_freem(m);
	} else
		IF_ENQUEUE(inq, m);
	splx(s);
}
/*
 * Perform common duties while attaching to interface list
 */
void
fddi_ifattach(ifp, lla)
	struct ifnet *ifp;
	caddr_t lla;
{

	ifp->if_type = IFT_FDDI;
	ifp->if_addrlen = 6;
	ifp->if_hdrlen = 21;
	ifp->if_dlt = DLT_FDDI;
	ifp->if_mtu = FDDIMTU;
	ifp->if_output = fddi_output;
	ifp->if_input = fddi_input;
	ifp->if_baudrate = IF_Mbps(100);
#ifdef IFF_NOTRAILERS
	ifp->if_flags |= IFF_NOTRAILERS;
#endif

	/*
	 * Update the max_linkhdr
	 */
	if (ALIGN(ifp->if_hdrlen) > max_linkhdr)
		max_linkhdr = ALIGN(ifp->if_hdrlen);

	if_alloc_sadl(ifp);
	memcpy(LLADDR(ifp->if_sadl), lla, ifp->if_addrlen);

	ifp->if_broadcastaddr = fddibroadcastaddr;
#if NBPFILTER > 0
	bpfattach(ifp, DLT_FDDI, sizeof(struct fddi_header));
#endif /* NBPFILTER > 0 */
}
