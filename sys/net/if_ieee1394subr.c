/*	$NetBSD: if_ieee1394subr.c,v 1.1 2000/11/05 17:17:15 onoe Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Atsushi Onoe.
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

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ieee1394.h>
#include <net/if_types.h>
#include <net/if_media.h>
#include <net/ethertypes.h>
#include <net/netisr.h>
#include <net/route.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/if_ieee1394arp.h>
#endif /* INET */
#ifdef INET6
#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#endif /* INET6 */

#define	senderr(e)	do { error = (e); goto bad; } while(0/*CONSTCOND*/)

static int  ieee1394_output(struct ifnet *, struct mbuf *, struct sockaddr *,
		struct rtentry *);
static void ieee1394_input(struct ifnet *, struct mbuf *);

static int
ieee1394_output(struct ifnet *ifp, struct mbuf *m0, struct sockaddr *dst,
    struct rtentry *rt0)
{
	u_int16_t etype = 0;
	int s, hdrlen, maxrec, len, off, tlen, error = 0;
	struct mbuf *m = m0, **mp;
	struct rtentry *rt;
	struct mbuf *mcopy = NULL;
	struct ieee1394com *ic = (struct ieee1394com *)ifp;
#ifdef INET
	struct ieee1394_arphdr *ah;
#endif /* INET */
	struct ieee1394_unfraghdr *iuh;
	struct ieee1394_fraghdr *ifh;
	struct ieee1394_hwaddr hwdst;

	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
		senderr(ENETDOWN);
	ifp->if_lastchange = time;
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
		if (rt->rt_flags & RTF_GATEWAY) {
			if (rt->rt_gwroute == 0)
				goto lookup;
			if (((rt = rt->rt_gwroute)->rt_flags & RTF_UP) == 0) {
				rtfree(rt);
				rt = rt0;
  lookup:
				rt->rt_gwroute = rtalloc1(rt->rt_gateway, 1);
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
			    time.tv_sec < rt->rt_rmx.rmx_expire)
				senderr(rt == rt0 ? EHOSTDOWN : EHOSTUNREACH);
	}
	switch (dst->sa_family) {
#ifdef INET
	case AF_INET:
		if (m->m_flags & (M_BCAST|M_MCAST))
			memcpy(&hwdst, ifp->if_broadcastaddr, sizeof(hwdst));
		else if (!ieee1394arpresolve(ifp, rt, m, dst, &hwdst))
			return 0;	/* if not yet resolved */
		/* if broadcasting on a simplex interface, loopback a copy */
		if ((m->m_flags & M_BCAST) && (ifp->if_flags & IFF_SIMPLEX))
			mcopy = m_copy(m, 0, (int)M_COPYALL);
		etype = htons(ETHERTYPE_IP);
		break;

	case AF_ARP:
		ah = mtod(m, struct ieee1394_arphdr *);
		memcpy(&hwdst, ifp->if_broadcastaddr, sizeof(hwdst));
		etype = htons(ETHERTYPE_ARP);
		break;
#endif /* INET */
#ifdef INET6
	case AF_INET6:
		if (m->m_flags & M_MCAST)
			memcpy(&hwdst, ifp->if_broadcastaddr, sizeof(hwdst));
		else if (!nd6_storelladdr(ifp, rt, m, dst, (u_char *)&hwdst)) {
			/* this must be impossible, so we bark */
			printf("ieee1394_output: nd6_storelladdr failed\n");
			return 0;
		}
		etype = htons(ETHERTYPE_IPV6);
		break;
#endif /* INET6 */

	case pseudo_AF_HDRCMPLT:
	case AF_UNSPEC:
		/* TODO? */
	default:
		printf("%s: can't handle af%d\n", ifp->if_xname,
		    dst->sa_family);
		senderr(EAFNOSUPPORT);
		break;
	}

	if (mcopy)
		looutput(ifp, mcopy, dst, rt);
#if NBPFILTER > 0
	if (ifp->if_bpf) {
		struct mbuf mb;

		mb.m_next = m;
		mb.m_len = 14;
		mb.m_data = mb.m_dat;
		((u_int32_t *)mb.m_data)[0] = 0;
		((u_int32_t *)mb.m_data)[1] = 0;
		((u_int32_t *)mb.m_data)[2] = 0;
		((u_int16_t *)mb.m_data)[6] = etype;
		bpf_mtap(ifp->if_bpf, &mb);
	}
#endif

	/* determine fragmented or not */
	maxrec = 1 << (hwdst.iha_maxrec + 1);
	if (m->m_flags & (M_BCAST|M_MCAST))
		hdrlen = IEEE1394_STRHDRLEN + sizeof(*iuh);
	else
		hdrlen = IEEE1394_AWBHDRLEN + sizeof(*iuh);
	if (m->m_pkthdr.len < maxrec - hdrlen) {
		M_PREPEND(m, sizeof(hwdst) + sizeof(*iuh), M_DONTWAIT);
		if (m == 0)
			senderr(ENOBUFS);
		memcpy(mtod(m, caddr_t), &hwdst, sizeof(hwdst));
		iuh = (struct ieee1394_unfraghdr *)
		    (mtod(m, caddr_t) + sizeof(hwdst));
		iuh->iuh_ft = 0;
		iuh->iuh_etype = etype;

		s = splimp();
		if (IF_QFULL(&ifp->if_snd)) {
			IF_DROP(&ifp->if_snd);
			splx(s);
			senderr(ENOBUFS);
		}
		IF_ENQUEUE(&ifp->if_snd, m);
		ifp->if_obytes += m0->m_pkthdr.len;
		goto done;
	}
	printf("ieee1304_output: fragment packet\n");
	hdrlen += sizeof(*ifh) - sizeof(*iuh);
	len = maxrec - hdrlen;
	off = len;
	mp = &m0->m_nextpkt;
	while (off < m0->m_pkthdr.len) {
		MGETHDR(m, M_DONTWAIT, MT_HEADER);
		if (m == NULL)
			senderr(ENOBUFS);
		m->m_flags |= m0->m_flags & (M_BCAST|M_MCAST);
		tlen = len;
		if (tlen > m0->m_pkthdr.len - off)
			tlen = m0->m_pkthdr.len - off;
		m->m_data += hdrlen - sizeof(hwdst);
		m->m_len = sizeof(hwdst) + sizeof(*ifh);
		memcpy(mtod(m, caddr_t), &hwdst, sizeof(hwdst));
		ifh = (struct ieee1394_fraghdr *)
		    (mtod(m, caddr_t) + sizeof(hwdst));
		ifh->ifh_ft_size = htons(0xc000 | tlen);
		ifh->ifh_etype_off = htons(off);
		ifh->ifh_dgl = htons(ic->ic_dgl);
		ifh->ifh_reserved = 0;
		m->m_next = m_copy(m0, off, tlen);
		if (m->m_next == NULL)
			senderr(ENOBUFS);
		off += tlen;
		*mp = m;
		mp = &m->m_nextpkt;
	}
	ifh->ifh_ft_size &= ~htons(0x4000);	/* note last fragment */

	/* first fragment */
	m = m0;
	M_PREPEND(m, sizeof(hwdst) + sizeof(*ifh), M_DONTWAIT);
	memcpy(mtod(m, caddr_t), &hwdst, sizeof(hwdst));
	ifh = (struct ieee1394_fraghdr *)(mtod(m, caddr_t) + sizeof(hwdst));
	ifh->ifh_ft_size = htons(0x4000 | (maxrec - hdrlen));
	ifh->ifh_etype_off = etype;
	ifh->ifh_dgl = htons(ic->ic_dgl++);
	ifh->ifh_reserved = 0;

	/* send off */

	s = splimp();
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		splx(s);
		senderr(ENOBUFS);
	}
	while (m0 != NULL) {
		m = m0->m_nextpkt;
		m0->m_nextpkt = NULL;
		IF_ENQUEUE(&ifp->if_snd, m0);
		ifp->if_obytes += m0->m_pkthdr.len;
		m0 = m;
	}

  done:
	if (m0->m_flags & M_MCAST)
		ifp->if_omcasts++;
	if ((ifp->if_flags & IFF_OACTIVE) == 0)
		(*ifp->if_start)(ifp);
	splx(s);
	return error;

  bad:
	while (m0 != NULL) {
		m = m0->m_nextpkt;
		m_freem(m0);
		m0 = m;
	}

	return error;
}

static void
ieee1394_input(struct ifnet *ifp, struct mbuf *m)
{
	struct ifqueue *inq;
	u_int16_t etype;
	int s;
	struct ieee1394_unfraghdr *iuh;

	if ((ifp->if_flags & IFF_UP) == 0) {
		m_freem(m);
		return;
	}

	iuh = mtod(m, struct ieee1394_unfraghdr *);
	if (iuh->iuh_ft != 0) {
		printf("%s: ieee1394_input: fragment not implemented yet\n",
		    ifp->if_xname);
		m_freem(m);
		return;
	}
	etype = ntohs(iuh->iuh_etype);

	/* strip off the ieee1394 header */
	m_adj(m, sizeof(struct ieee1394_unfraghdr));
#if NBPFILTER > 0
	if (ifp->if_bpf) {
		struct mbuf mb;

		mb.m_next = m;
		mb.m_len = 14;
		mb.m_data = mb.m_dat;
		((u_int32_t *)mb.m_data)[0] = 0;
		((u_int32_t *)mb.m_data)[1] = 0;
		((u_int32_t *)mb.m_data)[2] = 0;
		((u_int16_t *)mb.m_data)[6] = iuh->iuh_etype;
		bpf_mtap(ifp->if_bpf, &mb);
	}
#endif

	switch (etype) {
#ifdef INET
	case ETHERTYPE_IP:
		schednetisr(NETISR_IP);
		inq = &ipintrq;
		break;

	case ETHERTYPE_ARP:
		in_ieee1394arpinput(m);
		return;
#endif /* INET */

#ifdef INET6
	case ETHERTYPE_IPV6:
		schednetisr(NETISR_IPV6);
		inq = &ip6intrq;
		break;
#endif /* INET6 */

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

const char *
ieee1394_sprintf(const u_int8_t *laddr)
{
	static char buf[3*8];

	snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
	    laddr[0], laddr[1], laddr[2], laddr[3],
	    laddr[4], laddr[5], laddr[6], laddr[7]);
	return buf;
}

void
ieee1394_ifattach(struct ifnet *ifp, const struct ieee1394_hwaddr *hwaddr)
{
	struct sockaddr_dl *sdl;

	ifp->if_type = IFT_IEEE1394;
	ifp->if_addrlen = sizeof(struct ieee1394_hwaddr);
	ifp->if_hdrlen = sizeof(struct ieee1394_header);
	ifp->if_mtu = IEEE1394MTU;
	ifp->if_output = ieee1394_output;
	ifp->if_input = ieee1394_input;
	if (ifp->if_baudrate == 0)
		ifp->if_baudrate = IF_Mbps(100);
	if ((sdl = ifp->if_sadl) && sdl->sdl_family == AF_LINK) {
		sdl->sdl_type = ifp->if_type;
		sdl->sdl_alen = ifp->if_addrlen;
		memcpy(LLADDR(sdl), hwaddr, ifp->if_addrlen);
	}
	ifp->if_broadcastaddr = malloc(ifp->if_addrlen, M_DEVBUF, M_WAITOK);
	memcpy(ifp->if_broadcastaddr, hwaddr, ifp->if_addrlen);
	memset(ifp->if_broadcastaddr, 0xff, IEEE1394_ADDR_LEN);
}

void
ieee1394_ifdetach(struct ifnet *ifp)
{
	struct sockaddr_dl *sdl = ifp->if_sadl;

	free(ifp->if_broadcastaddr, M_DEVBUF);
	ifp->if_broadcastaddr = NULL;
	memset(LLADDR(sdl), 0, sizeof(struct ieee1394_hwaddr));
	sdl->sdl_alen = 0;
	sdl->sdl_type = 0;
}

int
ieee1394_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int error = 0;
#if __NetBSD_Version < 105080000
	int fw_init(struct ifnet *);
	void fw_stop(struct ifnet *, int);
#endif

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
#if __NetBSD_Version >= 105080000
			if ((error = (*ifp->if_init)(ifp)) != 0)
#else
			if ((error = fw_init(ifp)) != 0)
#endif
				break;
			ieee1394arp_ifinit(ifp, ifa);
			break;
#endif /* INET */
		default:
#if __NetBSD_Version >= 105080000
			error = (*ifp->if_init)(ifp);
#else
			error = fw_init(ifp);
#endif
			break;
		}
		break;

	case SIOCGIFADDR:
		memcpy(((struct sockaddr *)&ifr->ifr_data)->sa_data,
		    LLADDR(ifp->if_sadl), IEEE1394_ADDR_LEN);
		    break;

	case SIOCSIFMTU:
		if (ifr->ifr_mtu > IEEE1394MTU)
			error = EINVAL;
		else
			ifp->if_mtu = ifr->ifr_mtu;
		break;

	case SIOCSIFFLAGS:
#if __NetBSD_Version >= 105080000
		if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_RUNNING)
			(*ifp->if_stop)(ifp, 1);
		else if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_UP)
			error = (*ifp->if_init)(ifp);
		else if ((ifp->if_flags & IFF_UP) != 0)
			error = (*ifp->if_init)(ifp);
#else
		if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_RUNNING)
			fw_stop(ifp, 1);
		else if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_UP)
			error = fw_init(ifp);
		else if ((ifp->if_flags & IFF_UP) != 0)
			error = fw_init(ifp);
#endif
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		/* nothing to do */
		break;

	default:
		error = ENOTTY;
		break;
	}

	return error;
}
