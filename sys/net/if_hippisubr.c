/*	$NetBSD: if_hippisubr.c,v 1.5 2000/03/30 09:45:35 augustss Exp $	*/

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
 */

#include "opt_inet.h"

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

#include <net/if_hippi.h>

#include <netinet/in.h>
#ifdef INET
#include <netinet/in_var.h>
#endif

#define senderr(e) { error = (e); goto bad;}

#define SIN(x) ((struct sockaddr_in *)x)
#define SDL(x) ((struct sockaddr_dl *)x)

#ifndef llc_snap
#define	llc_snap	llc_un.type_snap
#endif

static	int hippi_output __P((struct ifnet *, struct mbuf *,
	    struct sockaddr *, struct rtentry *)); 
static	void hippi_input __P((struct ifnet *, struct mbuf *));

/*
 * HIPPI output routine.
 * Encapsulate a packet of type family for the local net.
 * I don't know anything about the mapping of AppleTalk or OSI
 * protocols to HIPPI, so I don't include any code for them.
 */

static int
hippi_output(ifp, m0, dst, rt0)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
	struct rtentry *rt0;
{
	u_int16_t htype;
	u_int32_t ifield = 0;
	int s, error = 0;
	struct mbuf *m = m0;
	struct rtentry *rt;
	struct hippi_header *hh;
	u_int32_t *cci;
	u_int32_t d2_len;

	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
		senderr(ENETDOWN);
	ifp->if_lastchange = time;

	/* HIPPI doesn't really do broadcast or multicast right now */
	if (m->m_flags & (M_BCAST | M_MCAST))
		senderr(EOPNOTSUPP);  /* XXX: some other error? */

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
			if (rt->rt_rmx.rmx_expire == 0 ||   /* XXX:  no ARP */
			    time.tv_sec < rt->rt_rmx.rmx_expire)
				senderr(rt == rt0 ? EHOSTDOWN : EHOSTUNREACH);
	}
	switch (dst->sa_family) {

#ifdef INET
#ifdef INET6
	case AF_INET6:
#endif
	case AF_INET:
		if (rt) {
			struct sockaddr_dl *sdl = 
				(struct sockaddr_dl *) SDL(rt->rt_gateway);
			if (sdl->sdl_family == AF_LINK && sdl->sdl_alen != 0)
				bcopy(LLADDR(sdl), &ifield, sizeof(ifield));
		}
		if (!ifield)  /* XXX:  bogus check, but helps us get going */
			senderr(EHOSTUNREACH);
		htype = htons(ETHERTYPE_IP);
		break;
#endif

	default:
		printf("%s: can't handle af%d\n", ifp->if_xname,
		       dst->sa_family);
		senderr(EAFNOSUPPORT);
	}

	if (htype != 0) {
		struct llc *l;
		M_PREPEND(m, sizeof (struct llc), M_DONTWAIT);
		if (m == 0)
			senderr(ENOBUFS);
		l = mtod(m, struct llc *);
		l->llc_control = LLC_UI;
		l->llc_dsap = l->llc_ssap = LLC_SNAP_LSAP;
		l->llc_snap.org_code[0] = l->llc_snap.org_code[1] = 
			l->llc_snap.org_code[2] = 0;
		bcopy((caddr_t) &htype, (caddr_t) &l->llc_snap.ether_type,
		      sizeof(u_int16_t));
	}

	d2_len = m->m_pkthdr.len;

	/*
	 * Add local net header.  If no space in first mbuf,
	 * allocate another.
	 */

	M_PREPEND(m, sizeof (struct hippi_header) + 8, M_DONTWAIT);
	if (m == 0)
		senderr(ENOBUFS);
	cci = mtod(m, u_int32_t *);
	bzero(cci, sizeof(struct hippi_header) + 8);
	cci[0] = 0;
	cci[1] = ifield;
	hh = (struct hippi_header *) &cci[2];
	hh->hi_fp.fp_ulp = HIPPI_ULP_802;
	hh->hi_fp.fp_flags = HIPPI_FP_D1_PRESENT;
	hh->hi_fp.fp_offsets = htons(sizeof(struct hippi_le));
	hh->hi_fp.fp_d2_len = htonl(d2_len);

	/* Pad out the D2 area to end on a quadword (64-bit) boundry. */
    
	if (d2_len % 8 != 0) {
		static u_int32_t buffer[2] = {0, 0};
		m_copyback(m, m->m_pkthdr.len, 8 - d2_len % 8, (caddr_t) buffer);
	}

	s = splimp();
	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		splx(s);
		senderr(ENOBUFS);
	}
	ifp->if_obytes += m->m_pkthdr.len;
	IF_ENQUEUE(&ifp->if_snd, m);
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
 * Process a received HIPPI packet;
 * the packet is in the mbuf chain m with
 * the HIPPI header.
 */

static void
hippi_input(ifp, m)
	struct ifnet *ifp;
	struct mbuf *m;
{
	struct ifqueue *inq;
	struct llc *l;
	u_int16_t htype;
	struct hippi_header *hh;
	int s;

	if ((ifp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		return;
	}

	/* XXX:  need to check flags and drop if bogus! */

	hh = mtod(m, struct hippi_header *);

	ifp->if_lastchange = time;
	ifp->if_ibytes += m->m_pkthdr.len;
	if (hh->hi_le.le_dest_addr[0] & 1) {
		if (bcmp((caddr_t)etherbroadcastaddr, 
			 (caddr_t)hh->hi_le.le_dest_addr,
			 sizeof(etherbroadcastaddr)) == 0)
			m->m_flags |= M_BCAST;
		else
			m->m_flags |= M_MCAST;
	}
	if (m->m_flags & (M_BCAST|M_MCAST))
		ifp->if_imcasts++;

	/* Skip past the HIPPI header. */
	m_adj(m, sizeof(struct hippi_header));

	l = mtod(m, struct llc *);
	if (l->llc_dsap != LLC_SNAP_LSAP) {
		m_freem(m);
		return;
	}
	htype = ntohs(l->llc_snap.ether_type);
	m_adj(m, 8);
	switch (htype) {
#ifdef INET
#ifdef INET6
	case ETHERTYPE_IPV6:
#endif
	case ETHERTYPE_IP:
		schednetisr(NETISR_IP);
		inq = &ipintrq;
		break;
#endif
	default:
		m_freem(m);
		return;
	}

	s = splimp();
	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		m_freem(m);
	} else
		IF_ENQUEUE(inq, m);
	splx(s);
}

/*
 * Handle packet from HIPPI that has no MAC header
 */

void
hippi_ip_input(ifp, m)
	struct ifnet *ifp;
	struct mbuf *m;
{
	struct ifqueue *inq;
	int s;
	u_int32_t *ip;
    
	ip = mtod(m, u_int32_t *);

	schednetisr(NETISR_IP);
	inq = &ipintrq;

	s = splimp();
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
hippi_ifattach(ifp, lla)
	struct ifnet *ifp;
	caddr_t lla;
{
	struct sockaddr_dl *sdl;

	ifp->if_type = IFT_HIPPI;
	ifp->if_addrlen = 6;  /* regular 802.3 MAC address */
	ifp->if_hdrlen = sizeof(struct hippi_header) + 8; /* add CCI */
	ifp->if_mtu = HIPPIMTU;
	ifp->if_output = hippi_output;
	ifp->if_input = hippi_input;
	ifp->if_baudrate = IF_Mbps(800);	/* XXX double-check */
	if ((sdl = ifp->if_sadl) &&
	    sdl->sdl_family == AF_LINK) {
		sdl->sdl_type = IFT_HIPPI;
		sdl->sdl_alen = ifp->if_addrlen;
		bcopy((caddr_t)lla, LLADDR(sdl), ifp->if_addrlen);
	}
}
