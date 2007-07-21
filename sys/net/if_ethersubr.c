/*	$NetBSD: if_ethersubr.c,v 1.151 2007/07/21 02:24:11 dyoung Exp $	*/

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
 *	@(#)if_ethersubr.c	8.2 (Berkeley) 4/4/96
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ethersubr.c,v 1.151 2007/07/21 02:24:11 dyoung Exp $");

#include "opt_inet.h"
#include "opt_atalk.h"
#include "opt_iso.h"
#include "opt_ipx.h"
#include "opt_mbuftrace.h"
#include "opt_gateway.h"
#include "opt_pfil_hooks.h"
#include "opt_pppoe.h"
#include "vlan.h"
#include "pppoe.h"
#include "bridge.h"
#include "bpfilter.h"
#include "arp.h"
#include "agr.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/kauth.h>

#include <machine/cpu.h>

#include <net/if.h>
#include <net/netisr.h>
#include <net/route.h>
#include <net/if_llc.h>
#include <net/if_dl.h>
#include <net/if_types.h>

#if NARP == 0
/*
 * XXX there should really be a way to issue this warning from within config(8)
 */
#error You have included NETATALK or a pseudo-device in your configuration that depends on the presence of ethernet interfaces, but have no such interfaces configured. Check if you really need pseudo-device bridge, pppoe, vlan or options NETATALK.
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <net/if_ether.h>
#if NVLAN > 0
#include <net/if_vlanvar.h>
#endif

#if NPPPOE > 0
#include <net/if_pppoe.h>
#endif

#if NAGR > 0
#include <net/agr/ieee8023_slowprotocols.h>	/* XXX */
#include <net/agr/ieee8023ad.h>
#include <net/agr/if_agrvar.h>
#endif

#if NBRIDGE > 0
#include <net/if_bridgevar.h>
#endif

#include <netinet/in.h>
#ifdef INET
#include <netinet/in_var.h>
#endif
#include <netinet/if_inarp.h>

#ifdef INET6
#ifndef INET
#include <netinet/in.h>
#endif
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#endif


#include "carp.h"
#if NCARP > 0
#include <netinet/ip_carp.h>
#endif

#ifdef IPX
#include <netipx/ipx.h>
#include <netipx/ipx_if.h>
#endif

#ifdef ISO
#include <netiso/argo_debug.h>
#include <netiso/iso.h>
#include <netiso/iso_var.h>
#include <netiso/iso_snpac.h>
#endif



#ifdef NETATALK
#include <netatalk/at.h>
#include <netatalk/at_var.h>
#include <netatalk/at_extern.h>

#define llc_snap_org_code llc_un.type_snap.org_code
#define llc_snap_ether_type llc_un.type_snap.ether_type

extern u_char	at_org_code[3];
extern u_char	aarp_org_code[3];
#endif /* NETATALK */

static struct timeval bigpktppslim_last;
static int bigpktppslim = 2;	/* XXX */
static int bigpktpps_count;


const uint8_t etherbroadcastaddr[ETHER_ADDR_LEN] =
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
const uint8_t ethermulticastaddr_slowprotocols[ETHER_ADDR_LEN] =
    { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x02 };
#define senderr(e) { error = (e); goto bad;}

static	int ether_output(struct ifnet *, struct mbuf *,
	    const struct sockaddr *, struct rtentry *);

/*
 * Ethernet output routine.
 * Encapsulate a packet of type family for the local net.
 * Assumes that ifp is actually pointer to ethercom structure.
 */
static int
ether_output(struct ifnet *ifp0, struct mbuf *m0, const struct sockaddr *dst,
	struct rtentry *rt0)
{
	u_int16_t etype = 0;
	int error = 0, hdrcmplt = 0;
 	uint8_t esrc[6], edst[6];
	struct mbuf *m = m0;
	struct rtentry *rt;
	struct mbuf *mcopy = NULL;
	struct ether_header *eh;
	struct ifnet *ifp = ifp0;
	ALTQ_DECL(struct altq_pktattr pktattr;)
#ifdef INET
	struct arphdr *ah;
#endif /* INET */
#ifdef NETATALK
	struct at_ifaddr *aa;
#endif /* NETATALK */

#ifdef MBUFTRACE
	m_claimm(m, ifp->if_mowner);
#endif

#if NCARP > 0
	if (ifp->if_type == IFT_CARP) {
		struct ifaddr *ifa;

		/* loop back if this is going to the carp interface */
		if (dst != NULL && ifp0->if_link_state == LINK_STATE_UP &&
		    (ifa = ifa_ifwithaddr(dst)) != NULL &&
		    ifa->ifa_ifp == ifp0)
			return looutput(ifp0, m, dst, rt0);

		ifp = ifp->if_carpdev;
		/* ac = (struct arpcom *)ifp; */

		if ((ifp0->if_flags & (IFF_UP|IFF_RUNNING)) !=
		    (IFF_UP|IFF_RUNNING))
			senderr(ENETDOWN);
	}
#endif /* NCARP > 0 */

	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
		senderr(ENETDOWN);
	if ((rt = rt0) != NULL) {
		if ((rt->rt_flags & RTF_UP) == 0) {
			if ((rt0 = rt = rtalloc1(dst, 1)) != NULL) {
				rt->rt_refcnt--;
				if (rt->rt_ifp != ifp)
					return (*rt->rt_ifp->if_output)
							(ifp, m0, dst, rt);
			} else
				senderr(EHOSTUNREACH);
		}
		if ((rt->rt_flags & RTF_GATEWAY) && dst->sa_family != AF_NS) {
			if (rt->rt_gwroute == 0)
				goto lookup;
			if (((rt = rt->rt_gwroute)->rt_flags & RTF_UP) == 0) {
				rtfree(rt); rt = rt0;
			lookup: rt->rt_gwroute = rtalloc1(rt->rt_gateway, 1);
				if ((rt = rt->rt_gwroute) == 0)
					senderr(EHOSTUNREACH);
				/* the "G" test below also prevents rt == rt0 */
				if ((rt->rt_flags & RTF_GATEWAY) ||
				    (rt->rt_ifp != ifp)) {
					rt->rt_refcnt--;
					rt0->rt_gwroute = 0;
					senderr(EHOSTUNREACH);
				}
			}
		}
		if (rt->rt_flags & RTF_REJECT)
			if (rt->rt_rmx.rmx_expire == 0 ||
			    (u_long) time_second < rt->rt_rmx.rmx_expire)
				senderr(rt == rt0 ? EHOSTDOWN : EHOSTUNREACH);
	}

	switch (dst->sa_family) {

#ifdef INET
	case AF_INET:
		if (m->m_flags & M_BCAST)
                	(void)memcpy(edst, etherbroadcastaddr, sizeof(edst));
		else if (m->m_flags & M_MCAST)
			ETHER_MAP_IP_MULTICAST(&satocsin(dst)->sin_addr, edst);
		else if (!arpresolve(ifp, rt, m, dst, edst))
			return (0);	/* if not yet resolved */
		/* If broadcasting on a simplex interface, loopback a copy */
		if ((m->m_flags & M_BCAST) && (ifp->if_flags & IFF_SIMPLEX))
			mcopy = m_copy(m, 0, (int)M_COPYALL);
		etype = htons(ETHERTYPE_IP);
		break;

	case AF_ARP:
		ah = mtod(m, struct arphdr *);
		if (m->m_flags & M_BCAST)
                	(void)memcpy(edst, etherbroadcastaddr, sizeof(edst));
		else {
			void *tha = ar_tha(ah);

			KASSERT(tha);
			memcpy(edst, tha, sizeof(edst));
		}

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
#endif
#ifdef INET6
	case AF_INET6:
		if (!nd6_storelladdr(ifp, rt, m, dst, (u_char *)edst, sizeof(edst))){
			/* something bad happened */
			return (0);
		}
		etype = htons(ETHERTYPE_IPV6);
		break;
#endif
#ifdef NETATALK
    case AF_APPLETALK:
		if (!aarpresolve(ifp, m, (const struct sockaddr_at *)dst, edst)) {
#ifdef NETATALKDEBUG
			printf("aarpresolv failed\n");
#endif /* NETATALKDEBUG */
			return (0);
		}
		/*
		 * ifaddr is the first thing in at_ifaddr
		 */
		aa = (struct at_ifaddr *) at_ifawithnet(
		    (const struct sockaddr_at *)dst, ifp);
		if (aa == NULL)
		    goto bad;

		/*
		 * In the phase 2 case, we need to prepend an mbuf for the
		 * llc header.  Since we must preserve the value of m,
		 * which is passed to us by value, we m_copy() the first
		 * mbuf, and use it for our llc header.
		 */
		if (aa->aa_flags & AFA_PHASE2) {
			struct llc llc;

			M_PREPEND(m, sizeof(struct llc), M_DONTWAIT);
			llc.llc_dsap = llc.llc_ssap = LLC_SNAP_LSAP;
			llc.llc_control = LLC_UI;
			memcpy(llc.llc_snap_org_code, at_org_code,
			    sizeof(llc.llc_snap_org_code));
			llc.llc_snap_ether_type = htons(ETHERTYPE_ATALK);
			memcpy(mtod(m, void *), &llc, sizeof(struct llc));
		} else {
			etype = htons(ETHERTYPE_ATALK);
		}
		break;
#endif /* NETATALK */
#ifdef IPX
	case AF_IPX:
		etype = htons(ETHERTYPE_IPX);
 		memcpy(edst,
		    &(((const struct sockaddr_ipx *)dst)->sipx_addr.x_host),
		    sizeof(edst));
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
			memcpy(edst, LLADDR(sdl), sizeof(edst));
		} else {
			error = iso_snparesolve(ifp,
			    (const struct sockaddr_iso *)dst,
						(char *)edst, &snpalen);
			if (error)
				goto bad; /* Not Resolved */
		}
		/* If broadcasting on a simplex interface, loopback a copy */
		if (*edst & 1)
			m->m_flags |= (M_BCAST|M_MCAST);
		if ((m->m_flags & M_BCAST) && (ifp->if_flags & IFF_SIMPLEX) &&
		    (mcopy = m_copy(m, 0, (int)M_COPYALL))) {
			M_PREPEND(mcopy, sizeof (*eh), M_DONTWAIT);
			if (mcopy) {
				eh = mtod(mcopy, struct ether_header *);
				memcpy(eh->ether_dhost, edst, sizeof(edst));
				memcpy(eh->ether_shost, LLADDR(ifp->if_sadl),
				    sizeof(edst));
			}
		}
		M_PREPEND(m, 3, M_DONTWAIT);
		if (m == NULL)
			return (0);
		l = mtod(m, struct llc *);
		l->llc_dsap = l->llc_ssap = LLC_ISO_LSAP;
		l->llc_control = LLC_UI;
#ifdef ARGO_DEBUG
		if (argo_debug[D_ETHER]) {
			int i;
			printf("unoutput: sending pkt to: ");
			for (i=0; i<6; i++)
				printf("%x ", edst[i] & 0xff);
			printf("\n");
		}
#endif
		} break;
#endif /* ISO */

	case pseudo_AF_HDRCMPLT:
		hdrcmplt = 1;
		memcpy(esrc,
		    ((const struct ether_header *)dst->sa_data)->ether_shost,
		    sizeof(esrc));
		/* FALLTHROUGH */

	case AF_UNSPEC:
 		memcpy(edst,
		    ((const struct ether_header *)dst->sa_data)->ether_dhost,
		    sizeof(edst));
		/* AF_UNSPEC doesn't swap the byte order of the ether_type. */
		etype = ((const struct ether_header *)dst->sa_data)->ether_type;
		break;

	default:
		printf("%s: can't handle af%d\n", ifp->if_xname,
			dst->sa_family);
		senderr(EAFNOSUPPORT);
	}

	if (mcopy)
		(void)looutput(ifp, mcopy, dst, rt);

	/* If no ether type is set, this must be a 802.2 formatted packet.
	 */
	if (etype == 0)
		etype = htons(m->m_pkthdr.len);
	/*
	 * Add local net header.  If no space in first mbuf,
	 * allocate another.
	 */
	M_PREPEND(m, sizeof (struct ether_header), M_DONTWAIT);
	if (m == 0)
		senderr(ENOBUFS);
	eh = mtod(m, struct ether_header *);
	/* Note: etype is already in network byte order. */
	(void)memcpy(&eh->ether_type, &etype, sizeof(eh->ether_type));
 	memcpy(eh->ether_dhost, edst, sizeof(edst));
	if (hdrcmplt)
		memcpy(eh->ether_shost, esrc, sizeof(eh->ether_shost));
	else
	 	memcpy(eh->ether_shost, LLADDR(ifp->if_sadl),
		    sizeof(eh->ether_shost));

#if NCARP > 0
	if (ifp0 != ifp && ifp0->if_type == IFT_CARP) {
	 	memcpy(eh->ether_shost, LLADDR(ifp0->if_sadl),
		    sizeof(eh->ether_shost));
	}
#endif /* NCARP > 0 */

#ifdef PFIL_HOOKS
	if ((error = pfil_run_hooks(&ifp->if_pfil, &m, ifp, PFIL_OUT)) != 0)
		return (error);
	if (m == NULL)
		return (0);
#endif

#if NBRIDGE > 0
	/*
	 * Bridges require special output handling.
	 */
	if (ifp->if_bridge)
		return (bridge_output(ifp, m, NULL, NULL));
#endif

#if NCARP > 0
	if (ifp != ifp0)
		ifp0->if_obytes += m->m_pkthdr.len + ETHER_HDR_LEN;
#endif /* NCARP > 0 */

#ifdef ALTQ
	/*
	 * If ALTQ is enabled on the parent interface, do
	 * classification; the queueing discipline might not
	 * require classification, but might require the
	 * address family/header pointer in the pktattr.
	 */
	if (ALTQ_IS_ENABLED(&ifp->if_snd))
		altq_etherclassify(&ifp->if_snd, m, &pktattr);
#endif

	return ifq_enqueue(ifp, m ALTQ_COMMA ALTQ_DECL(&pktattr));

bad:
	if (m)
		m_freem(m);
	return (error);
}

#ifdef ALTQ
/*
 * This routine is a slight hack to allow a packet to be classified
 * if the Ethernet headers are present.  It will go away when ALTQ's
 * classification engine understands link headers.
 */
void
altq_etherclassify(struct ifaltq *ifq, struct mbuf *m,
    struct altq_pktattr *pktattr)
{
	struct ether_header *eh;
	u_int16_t ether_type;
	int hlen, af, hdrsize;
	void *hdr;

	hlen = ETHER_HDR_LEN;
	eh = mtod(m, struct ether_header *);

	ether_type = htons(eh->ether_type);

	if (ether_type < ETHERMTU) {
		/* LLC/SNAP */
		struct llc *llc = (struct llc *)(eh + 1);
		hlen += 8;

		if (m->m_len < hlen ||
		    llc->llc_dsap != LLC_SNAP_LSAP ||
		    llc->llc_ssap != LLC_SNAP_LSAP ||
		    llc->llc_control != LLC_UI) {
			/* Not SNAP. */
			goto bad;
		}

		ether_type = htons(llc->llc_un.type_snap.ether_type);
	}

	switch (ether_type) {
	case ETHERTYPE_IP:
		af = AF_INET;
		hdrsize = 20;		/* sizeof(struct ip) */
		break;

	case ETHERTYPE_IPV6:
		af = AF_INET6;
		hdrsize = 40;		/* sizeof(struct ip6_hdr) */
		break;

	default:
		af = AF_UNSPEC;
		hdrsize = 0;
		break;
	}

	while (m->m_len <= hlen) {
		hlen -= m->m_len;
		m = m->m_next;
	}
	if (m->m_len < (hlen + hdrsize)) {
		/*
		 * protocol header not in a single mbuf.
		 * We can't cope with this situation right
		 * now (but it shouldn't ever happen, really, anyhow).
		 */
#ifdef DEBUG
		printf("altq_etherclassify: headers span multiple mbufs: "
		    "%d < %d\n", m->m_len, (hlen + hdrsize));
#endif
		goto bad;
	}

	m->m_data += hlen;
	m->m_len -= hlen;

	hdr = mtod(m, void *);

	if (ALTQ_NEEDS_CLASSIFY(ifq))
		pktattr->pattr_class =
		    (*ifq->altq_classify)(ifq->altq_clfier, m, af);
	pktattr->pattr_af = af;
	pktattr->pattr_hdr = hdr;

	m->m_data -= hlen;
	m->m_len += hlen;

	return;

 bad:
	pktattr->pattr_class = NULL;
	pktattr->pattr_hdr = NULL;
	pktattr->pattr_af = AF_UNSPEC;
}
#endif /* ALTQ */

/*
 * Process a received Ethernet packet;
 * the packet is in the mbuf chain m with
 * the ether header.
 */
void
ether_input(struct ifnet *ifp, struct mbuf *m)
{
	struct ethercom *ec = (struct ethercom *) ifp;
	struct ifqueue *inq;
	u_int16_t etype;
	struct ether_header *eh;
#if defined (ISO) || defined (LLC) || defined(NETATALK)
	struct llc *l;
#endif

	if ((ifp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		return;
	}

#ifdef MBUFTRACE
	m_claimm(m, &ec->ec_rx_mowner);
#endif
	eh = mtod(m, struct ether_header *);
	etype = ntohs(eh->ether_type);

	/*
	 * Determine if the packet is within its size limits.
	 */
	if (m->m_pkthdr.len >
	    ETHER_MAX_FRAME(ifp, etype, m->m_flags & M_HASFCS)) {
		if (ppsratecheck(&bigpktppslim_last, &bigpktpps_count,
			    bigpktppslim)) {
			printf("%s: discarding oversize frame (len=%d)\n",
			    ifp->if_xname, m->m_pkthdr.len);
		}
		m_freem(m);
		return;
	}

	if (ETHER_IS_MULTICAST(eh->ether_dhost)) {
		/*
		 * If this is not a simplex interface, drop the packet
		 * if it came from us.
		 */
		if ((ifp->if_flags & IFF_SIMPLEX) == 0 &&
		    memcmp(LLADDR(ifp->if_sadl), eh->ether_shost,
		    ETHER_ADDR_LEN) == 0) {
			m_freem(m);
			return;
		}

		if (memcmp(etherbroadcastaddr,
		    eh->ether_dhost, ETHER_ADDR_LEN) == 0)
			m->m_flags |= M_BCAST;
		else
			m->m_flags |= M_MCAST;
		ifp->if_imcasts++;
	}

	/* If the CRC is still on the packet, trim it off. */
	if (m->m_flags & M_HASFCS) {
		m_adj(m, -ETHER_CRC_LEN);
		m->m_flags &= ~M_HASFCS;
	}

	ifp->if_ibytes += m->m_pkthdr.len;

#if NBRIDGE > 0
	/*
	 * Tap the packet off here for a bridge.  bridge_input()
	 * will return NULL if it has consumed the packet, otherwise
	 * it gets processed as normal.  Note that bridge_input()
	 * will always return the original packet if we need to
	 * process it locally.
	 */
	if (ifp->if_bridge) {
		/* clear M_PROMISC, in case the packets comes from a vlan */
		m->m_flags &= ~M_PROMISC;
		m = bridge_input(ifp, m);
		if (m == NULL)
			return;

		/*
		 * Bridge has determined that the packet is for us.
		 * Update our interface pointer -- we may have had
		 * to "bridge" the packet locally.
		 */
		ifp = m->m_pkthdr.rcvif;
	} else
#endif /* NBRIDGE > 0 */
	{

#if NCARP > 0
		if (ifp->if_carp && ifp->if_type != IFT_CARP) {
			/*
			 * clear M_PROMISC, in case the packets comes from a
			 * vlan
			 */
			m->m_flags &= ~M_PROMISC;
			if (carp_input(m, (u_int8_t *)&eh->ether_shost,
			    (u_int8_t *)&eh->ether_dhost, eh->ether_type) == 0)
				return;
		}
#endif /* NCARP > 0 */
		if ((m->m_flags & (M_BCAST|M_MCAST)) == 0 &&
		    (ifp->if_flags & IFF_PROMISC) != 0 &&
		    memcmp(LLADDR(ifp->if_sadl), eh->ether_dhost,
			   ETHER_ADDR_LEN) != 0) {
			m->m_flags |= M_PROMISC;
		}
	}

#ifdef PFIL_HOOKS
	if ((m->m_flags & M_PROMISC) == 0) {
		if (pfil_run_hooks(&ifp->if_pfil, &m, ifp, PFIL_IN) != 0)
			return;
		if (m == NULL)
			return;

		eh = mtod(m, struct ether_header *);
		etype = ntohs(eh->ether_type);
	}
#endif

	/*
	 * If VLANs are configured on the interface, check to
	 * see if the device performed the decapsulation and
	 * provided us with the tag.
	 */
	if (ec->ec_nvlans && m_tag_find(m, PACKET_TAG_VLAN, NULL) != NULL) {
#if NVLAN > 0
		/*
		 * vlan_input() will either recursively call ether_input()
		 * or drop the packet.
		 */
		vlan_input(ifp, m);
#else
		m_freem(m);
#endif
		return;
	}

#if NAGR > 0
	if (ifp->if_agrprivate &&
	    __predict_true(etype != ETHERTYPE_SLOWPROTOCOLS)) {
		m->m_flags &= ~M_PROMISC;
		agr_input(ifp, m);
		return;
	}
#endif /* NAGR > 0 */

	/*
	 * Handle protocols that expect to have the Ethernet header
	 * (and possibly FCS) intact.
	 */
	switch (etype) {
#if NVLAN > 0
	case ETHERTYPE_VLAN:
		/*
		 * vlan_input() will either recursively call ether_input()
		 * or drop the packet.
		 */
		if (((struct ethercom *)ifp)->ec_nvlans != 0)
			vlan_input(ifp, m);
		else
			m_freem(m);
		return;
#endif /* NVLAN > 0 */
#if NPPPOE > 0
	case ETHERTYPE_PPPOEDISC:
	case ETHERTYPE_PPPOE:
		if (m->m_flags & M_PROMISC) {
			m_freem(m);
			return;
		}
#ifndef PPPOE_SERVER
		if (m->m_flags & (M_MCAST | M_BCAST)) {
			m_freem(m);
			return;
		}
#endif

		if (etype == ETHERTYPE_PPPOEDISC)
			inq = &ppoediscinq;
		else
			inq = &ppoeinq;
		if (IF_QFULL(inq)) {
			IF_DROP(inq);
			m_freem(m);
		} else
			IF_ENQUEUE(inq, m);
		softintr_schedule(pppoe_softintr);
		return;
#endif /* NPPPOE > 0 */
	case ETHERTYPE_SLOWPROTOCOLS: {
		uint8_t subtype;

#if defined(DIAGNOSTIC)
		if (m->m_pkthdr.len < sizeof(*eh) + sizeof(subtype)) {
			panic("ether_input: too short slow protocol packet");
		}
#endif
		m_copydata(m, sizeof(*eh), sizeof(subtype), &subtype);
		switch (subtype) {
#if NAGR > 0
		case SLOWPROTOCOLS_SUBTYPE_LACP:
			if (ifp->if_agrprivate) {
				ieee8023ad_lacp_input(ifp, m);
				return;
			}
			break;

		case SLOWPROTOCOLS_SUBTYPE_MARKER:
			if (ifp->if_agrprivate) {
				ieee8023ad_marker_input(ifp, m);
				return;
			}
			break;
#endif /* NAGR > 0 */
		default:
			if (subtype == 0 || subtype > 10) {
				/* illegal value */
				m_freem(m);
				return;
			}
			/* unknown subtype */
			break;
		}
		/* FALLTHROUGH */
	}
	default:
		if (m->m_flags & M_PROMISC) {
			m_freem(m);
			return;
		}
	}

	/* If the CRC is still on the packet, trim it off. */
	if (m->m_flags & M_HASFCS) {
		m_adj(m, -ETHER_CRC_LEN);
		m->m_flags &= ~M_HASFCS;
	}

	if (etype > ETHERMTU + sizeof (struct ether_header)) {
		/* Strip off the Ethernet header. */
		m_adj(m, sizeof(struct ether_header));

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
			schednetisr(NETISR_ARP);
			inq = &arpintrq;
			break;

		case ETHERTYPE_REVARP:
			revarpinput(m);	/* XXX queue? */
			return;
#endif
#ifdef INET6
		case ETHERTYPE_IPV6:
#ifdef GATEWAY  
			if (ip6flow_fastforward(m))
				return;
#endif
			schednetisr(NETISR_IPV6);
			inq = &ip6intrq;
			break;
#endif
#ifdef IPX
		case ETHERTYPE_IPX:
			schednetisr(NETISR_IPX);
			inq = &ipxintrq;
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
			m_freem(m);
			return;
		}
	} else {
#if defined (ISO) || defined (LLC) || defined (NETATALK)
		l = (struct llc *)(eh+1);
		switch (l->llc_dsap) {
#ifdef NETATALK
		case LLC_SNAP_LSAP:
			switch (l->llc_control) {
			case LLC_UI:
				if (l->llc_ssap != LLC_SNAP_LSAP) {
					goto dropanyway;
				}

				if (Bcmp(&(l->llc_snap_org_code)[0],
				    at_org_code, sizeof(at_org_code)) == 0 &&
				    ntohs(l->llc_snap_ether_type) ==
				    ETHERTYPE_ATALK) {
					inq = &atintrq2;
					m_adj(m, sizeof(struct ether_header)
					    + sizeof(struct llc));
					schednetisr(NETISR_ATALK);
					break;
				}

				if (Bcmp(&(l->llc_snap_org_code)[0],
				    aarp_org_code,
				    sizeof(aarp_org_code)) == 0 &&
				    ntohs(l->llc_snap_ether_type) ==
				    ETHERTYPE_AARP) {
					m_adj( m, sizeof(struct ether_header)
					    + sizeof(struct llc));
					aarpinput(ifp, m); /* XXX */
				    return;
				}

			default:
				goto dropanyway;
			}
			break;
#endif /* NETATALK */
#ifdef	ISO
		case LLC_ISO_LSAP:
			switch (l->llc_control) {
			case LLC_UI:
				/* LLC_UI_P forbidden in class 1 service */
				if ((l->llc_dsap == LLC_ISO_LSAP) &&	/* XXX? case tested */
				    (l->llc_ssap == LLC_ISO_LSAP)) {
					/* LSAP for ISO */
					/* XXX length computation?? */
					if (m->m_pkthdr.len > etype + sizeof(struct ether_header))
						m_adj(m, etype - m->m_pkthdr.len);
					
#ifdef ARGO_DEBUG
					if (argo_debug[D_ETHER])
						printf("clnp packet");
#endif
					schednetisr(NETISR_ISO);
					inq = &clnlintrq;
					break;
				}
				goto dropanyway;

			case LLC_XID:
			case LLC_XID_P:
				if(m->m_len < LLC_XID_BASIC_MINLEN + sizeof(struct ether_header))
					/* XXX m_pullup? */
					goto dropanyway;
				l->llc_window = 0;
				l->llc_fid = LLC_XID_FORMAT_BASIC;
				l->llc_class = LLC_XID_CLASS_I;
				l->llc_dsap = l->llc_ssap = 0;
				/* Fall through to */
			case LLC_TEST:
			case LLC_TEST_P:
			{
				struct sockaddr sa;
				struct ether_header *eh2;
				int i;
				u_char c = l->llc_dsap;

				l->llc_dsap = l->llc_ssap;
				l->llc_ssap = c;
				m_adj(m, sizeof(struct ether_header));
				/* XXX we can optimize here? */
				if (m->m_flags & (M_BCAST | M_MCAST))
					memcpy(eh->ether_dhost,
					    LLADDR(ifp->if_sadl),
					    ETHER_ADDR_LEN);
				sa.sa_family = AF_UNSPEC;
				sa.sa_len = sizeof(sa);
				eh2 = (struct ether_header *)sa.sa_data;
				for (i = 0; i < 6; i++) {
					eh2->ether_shost[i] = c =
					    eh->ether_dhost[i];
					eh2->ether_dhost[i] =
					    eh->ether_dhost[i] =
					    eh->ether_shost[i];
					eh->ether_shost[i] = c;
				}
				ifp->if_output(ifp, m, &sa, NULL);
				return;
			}
			default:
				m_freem(m);
				return;
			}
			break;
#endif /* ISO */
#if defined (ISO) || defined (NETATALK)
		dropanyway:
#endif
		default:
			m_freem(m);
			return;
		}
#else /* ISO || LLC || NETATALK*/
		m_freem(m);
		return;
#endif /* ISO || LLC || NETATALK*/
	}

	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		m_freem(m);
	} else
		IF_ENQUEUE(inq, m);
}

/*
 * Convert Ethernet address to printable (loggable) representation.
 */
char *
ether_sprintf(const u_char *ap)
{
	static char etherbuf[3 * ETHER_ADDR_LEN];
	return ether_snprintf(etherbuf, sizeof(etherbuf), ap);
	return etherbuf;
}

char *
ether_snprintf(char *buf, size_t len, const u_char *ap)
{
	char *cp = buf;
	size_t i;

	for (i = 0; i < len / 3; i++) {
		*cp++ = hexdigits[*ap >> 4];
		*cp++ = hexdigits[*ap++ & 0xf];
		*cp++ = ':';
	}
	*--cp = '\0';
	return buf;
}

/*
 * Perform common duties while attaching to interface list
 */
void
ether_ifattach(struct ifnet *ifp, const u_int8_t *lla)
{
	struct ethercom *ec = (struct ethercom *)ifp;

	ifp->if_type = IFT_ETHER;
	ifp->if_addrlen = ETHER_ADDR_LEN;
	ifp->if_hdrlen = ETHER_HDR_LEN;
	ifp->if_dlt = DLT_EN10MB;
	ifp->if_mtu = ETHERMTU;
	ifp->if_output = ether_output;
	ifp->if_input = ether_input;
	if (ifp->if_baudrate == 0)
		ifp->if_baudrate = IF_Mbps(10);		/* just a default */

	if_alloc_sadl(ifp);
	memcpy(LLADDR(ifp->if_sadl), lla, ifp->if_addrlen);

	LIST_INIT(&ec->ec_multiaddrs);
	ifp->if_broadcastaddr = etherbroadcastaddr;
#if NBPFILTER > 0
	bpfattach(ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif
#ifdef MBUFTRACE
	strlcpy(ec->ec_tx_mowner.mo_name, ifp->if_xname,
	    sizeof(ec->ec_tx_mowner.mo_name));
	strlcpy(ec->ec_tx_mowner.mo_descr, "tx",
	    sizeof(ec->ec_tx_mowner.mo_descr));
	strlcpy(ec->ec_rx_mowner.mo_name, ifp->if_xname,
	    sizeof(ec->ec_rx_mowner.mo_name));
	strlcpy(ec->ec_rx_mowner.mo_descr, "rx",
	    sizeof(ec->ec_rx_mowner.mo_descr));
	MOWNER_ATTACH(&ec->ec_tx_mowner);
	MOWNER_ATTACH(&ec->ec_rx_mowner);
	ifp->if_mowner = &ec->ec_tx_mowner;
#endif
}

void
ether_ifdetach(struct ifnet *ifp)
{
	struct ethercom *ec = (void *) ifp;
	struct ether_multi *enm;
	int s;

#if NBRIDGE > 0
	if (ifp->if_bridge)
		bridge_ifdetach(ifp);
#endif

#if NBPFILTER > 0
	bpfdetach(ifp);
#endif

#if NVLAN > 0
	if (ec->ec_nvlans)
		vlan_ifdetach(ifp);
#endif

	s = splnet();
	while ((enm = LIST_FIRST(&ec->ec_multiaddrs)) != NULL) {
		LIST_REMOVE(enm, enm_list);
		free(enm, M_IFMADDR);
		ec->ec_multicnt--;
	}
	splx(s);

#if 0	/* done in if_detach() */
	if_free_sadl(ifp);
#endif

	MOWNER_DETACH(&ec->ec_rx_mowner);
	MOWNER_DETACH(&ec->ec_tx_mowner);
}

#if 0
/*
 * This is for reference.  We have a table-driven version
 * of the little-endian crc32 generator, which is faster
 * than the double-loop.
 */
u_int32_t
ether_crc32_le(const u_int8_t *buf, size_t len)
{
	u_int32_t c, crc, carry;
	size_t i, j;

	crc = 0xffffffffU;	/* initial value */

	for (i = 0; i < len; i++) {
		c = buf[i];
		for (j = 0; j < 8; j++) {
			carry = ((crc & 0x01) ? 1 : 0) ^ (c & 0x01);
			crc >>= 1;
			c >>= 1;
			if (carry)
				crc = (crc ^ ETHER_CRC_POLY_LE);
		}
	}

	return (crc);
}
#else
u_int32_t
ether_crc32_le(const u_int8_t *buf, size_t len)
{
	static const u_int32_t crctab[] = {
		0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
		0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
		0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
		0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
	};
	u_int32_t crc;
	size_t i;

	crc = 0xffffffffU;	/* initial value */

	for (i = 0; i < len; i++) {
		crc ^= buf[i];
		crc = (crc >> 4) ^ crctab[crc & 0xf];
		crc = (crc >> 4) ^ crctab[crc & 0xf];
	}

	return (crc);
}
#endif

u_int32_t
ether_crc32_be(const u_int8_t *buf, size_t len)
{
	u_int32_t c, crc, carry;
	size_t i, j;

	crc = 0xffffffffU;	/* initial value */

	for (i = 0; i < len; i++) {
		c = buf[i];
		for (j = 0; j < 8; j++) {
			carry = ((crc & 0x80000000U) ? 1 : 0) ^ (c & 0x01);
			crc <<= 1;
			c >>= 1;
			if (carry)
				crc = (crc ^ ETHER_CRC_POLY_BE) | carry;
		}
	}

	return (crc);
}

#ifdef INET
const uint8_t ether_ipmulticast_min[ETHER_ADDR_LEN] =
    { 0x01, 0x00, 0x5e, 0x00, 0x00, 0x00 };
const uint8_t ether_ipmulticast_max[ETHER_ADDR_LEN] =
    { 0x01, 0x00, 0x5e, 0x7f, 0xff, 0xff };
#endif
#ifdef INET6
const uint8_t ether_ip6multicast_min[ETHER_ADDR_LEN] =
    { 0x33, 0x33, 0x00, 0x00, 0x00, 0x00 };
const uint8_t ether_ip6multicast_max[ETHER_ADDR_LEN] =
    { 0x33, 0x33, 0xff, 0xff, 0xff, 0xff };
#endif

/*
 * ether_aton implementation, not using a static buffer.
 */
int
ether_nonstatic_aton(u_char *dest, char *str)
{
        int i;
        char *cp = str;
        u_char val[6];

#define set_value                       \
        if (*cp > '9' && *cp < 'a')     \
                *cp -= 'A' - 10;        \
        else if (*cp > '9')             \
                *cp -= 'a' - 10;        \
        else                            \
                *cp -= '0'

        for (i = 0; i < 6; i++, cp++) {
                if (!isxdigit(*cp))
                        return (1);
                set_value;
                val[i] = *cp++;
                if (isxdigit(*cp)) {
                        set_value;
                        val[i] *= 16;
                        val[i] += *cp++;
                }
                if (*cp == ':' || i == 5)
                        continue;
                else
                        return 1;
        }
        memcpy(dest, val, 6);

        return 0;
}


/*
 * Convert a sockaddr into an Ethernet address or range of Ethernet
 * addresses.
 */
int
ether_multiaddr(struct sockaddr *sa, u_int8_t addrlo[ETHER_ADDR_LEN],
    u_int8_t addrhi[ETHER_ADDR_LEN])
{
#ifdef INET
	struct sockaddr_in *sin;
#endif /* INET */
#ifdef INET6
	struct sockaddr_in6 *sin6;
#endif /* INET6 */

	switch (sa->sa_family) {

	case AF_UNSPEC:
		memcpy(addrlo, sa->sa_data, ETHER_ADDR_LEN);
		memcpy(addrhi, addrlo, ETHER_ADDR_LEN);
		break;

#ifdef INET
	case AF_INET:
		sin = satosin(sa);
		if (sin->sin_addr.s_addr == INADDR_ANY) {
			/*
			 * An IP address of INADDR_ANY means listen to
			 * or stop listening to all of the Ethernet
			 * multicast addresses used for IP.
			 * (This is for the sake of IP multicast routers.)
			 */
			memcpy(addrlo, ether_ipmulticast_min, ETHER_ADDR_LEN);
			memcpy(addrhi, ether_ipmulticast_max, ETHER_ADDR_LEN);
		}
		else {
			ETHER_MAP_IP_MULTICAST(&sin->sin_addr, addrlo);
			memcpy(addrhi, addrlo, ETHER_ADDR_LEN);
		}
		break;
#endif
#ifdef INET6
	case AF_INET6:
		sin6 = satosin6(sa);
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
			/*
			 * An IP6 address of 0 means listen to or stop
			 * listening to all of the Ethernet multicast
			 * address used for IP6.
			 * (This is used for multicast routers.)
			 */
			memcpy(addrlo, ether_ip6multicast_min, ETHER_ADDR_LEN);
			memcpy(addrhi, ether_ip6multicast_max, ETHER_ADDR_LEN);
		} else {
			ETHER_MAP_IPV6_MULTICAST(&sin6->sin6_addr, addrlo);
			memcpy(addrhi, addrlo, ETHER_ADDR_LEN);
		}
		break;
#endif

	default:
		return EAFNOSUPPORT;
	}
	return 0;
}

/*
 * Add an Ethernet multicast address or range of addresses to the list for a
 * given interface.
 */
int
ether_addmulti(struct ifreq *ifr, struct ethercom *ec)
{
	struct ether_multi *enm;
	u_char addrlo[ETHER_ADDR_LEN];
	u_char addrhi[ETHER_ADDR_LEN];
	int s = splnet(), error;

	error = ether_multiaddr(&ifr->ifr_addr, addrlo,
	    addrhi);
	if (error != 0) {
		splx(s);
		return error;
	}

	/*
	 * Verify that we have valid Ethernet multicast addresses.
	 */
	if ((addrlo[0] & 0x01) != 1 || (addrhi[0] & 0x01) != 1) {
		splx(s);
		return EINVAL;
	}
	/*
	 * See if the address range is already in the list.
	 */
	ETHER_LOOKUP_MULTI(addrlo, addrhi, ec, enm);
	if (enm != NULL) {
		/*
		 * Found it; just increment the reference count.
		 */
		++enm->enm_refcount;
		splx(s);
		return 0;
	}
	/*
	 * New address or range; malloc a new multicast record
	 * and link it into the interface's multicast list.
	 */
	enm = (struct ether_multi *)malloc(sizeof(*enm), M_IFMADDR, M_NOWAIT);
	if (enm == NULL) {
		splx(s);
		return ENOBUFS;
	}
	memcpy(enm->enm_addrlo, addrlo, 6);
	memcpy(enm->enm_addrhi, addrhi, 6);
	enm->enm_refcount = 1;
	LIST_INSERT_HEAD(&ec->ec_multiaddrs, enm, enm_list);
	ec->ec_multicnt++;
	splx(s);
	/*
	 * Return ENETRESET to inform the driver that the list has changed
	 * and its reception filter should be adjusted accordingly.
	 */
	return ENETRESET;
}

/*
 * Delete a multicast address record.
 */
int
ether_delmulti(struct ifreq *ifr, struct ethercom *ec)
{
	struct ether_multi *enm;
	u_char addrlo[ETHER_ADDR_LEN];
	u_char addrhi[ETHER_ADDR_LEN];
	int s = splnet(), error;

	error = ether_multiaddr(&ifr->ifr_addr, addrlo,
	    addrhi);
	if (error != 0) {
		splx(s);
		return (error);
	}

	/*
	 * Look ur the address in our list.
	 */
	ETHER_LOOKUP_MULTI(addrlo, addrhi, ec, enm);
	if (enm == NULL) {
		splx(s);
		return (ENXIO);
	}
	if (--enm->enm_refcount != 0) {
		/*
		 * Still some claims to this record.
		 */
		splx(s);
		return (0);
	}
	/*
	 * No remaining claims to this record; unlink and free it.
	 */
	LIST_REMOVE(enm, enm_list);
	free(enm, M_IFMADDR);
	ec->ec_multicnt--;
	splx(s);
	/*
	 * Return ENETRESET to inform the driver that the list has changed
	 * and its reception filter should be adjusted accordingly.
	 */
	return (ENETRESET);
}

/*
 * Common ioctls for Ethernet interfaces.  Note, we must be
 * called at splnet().
 */
int
ether_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct ethercom *ec = (void *) ifp;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int error = 0;

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {
		case AF_LINK:
		    {
			struct sockaddr_dl *sdl =
			    (struct sockaddr_dl *) ifa->ifa_addr;

			if (sdl->sdl_type != IFT_ETHER ||
			    sdl->sdl_alen != ifp->if_addrlen) {
				error = EINVAL;
				break;
			}

			memcpy(LLADDR(ifp->if_sadl), LLADDR(sdl),
			    ifp->if_addrlen);

			/* Set new address. */
			error = (*ifp->if_init)(ifp);
			break;
		    }
#ifdef INET
		case AF_INET:
			if ((ifp->if_flags & IFF_RUNNING) == 0 &&
			    (error = (*ifp->if_init)(ifp)) != 0)
				break;
			arp_ifinit(ifp, ifa);
			break;
#endif /* INET */
		default:
			if ((ifp->if_flags & IFF_RUNNING) == 0)
				error = (*ifp->if_init)(ifp);
			break;
		}
		break;

	case SIOCGIFADDR:
		memcpy(((struct sockaddr *)&ifr->ifr_data)->sa_data,
		    LLADDR(ifp->if_sadl), ETHER_ADDR_LEN);
		break;

	case SIOCSIFMTU:
	    {
		int maxmtu;

		if (ec->ec_capabilities & ETHERCAP_JUMBO_MTU)
			maxmtu = ETHERMTU_JUMBO;
		else
			maxmtu = ETHERMTU;

		if (ifr->ifr_mtu < ETHERMIN || ifr->ifr_mtu > maxmtu)
			error = EINVAL;
		else {
			ifp->if_mtu = ifr->ifr_mtu;

			/* Make sure the device notices the MTU change. */
			if (ifp->if_flags & IFF_UP)
				error = (*ifp->if_init)(ifp);
		}
		break;
	    }

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_RUNNING) {
			/*
			 * If interface is marked down and it is running,
			 * then stop and disable it.
			 */
			(*ifp->if_stop)(ifp, 1);
		} else if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_UP) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			error = (*ifp->if_init)(ifp);
		} else if ((ifp->if_flags & IFF_UP) != 0) {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect the hardware state.
			 */
			error = (*ifp->if_init)(ifp);
		}
		break;

	case SIOCADDMULTI:
		error = ether_addmulti(ifr, ec);
		break;

	case SIOCDELMULTI:
		error = ether_delmulti(ifr, ec);
		break;

	default:
		error = ENOTTY;
	}

	return (error);
}
