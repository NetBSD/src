/*	$NetBSD: ether_sw_offload.c,v 1.8 2021/09/03 21:55:00 andvar Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Rin Okuyama.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ether_sw_offload.c,v 1.8 2021/09/03 21:55:00 andvar Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/time.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/ether_sw_offload.h>

#include <netinet/in.h>
#include <netinet/in_offload.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/in6.h>
#include <netinet6/in6_offload.h>
#endif

/*
 * Limit error messages at most once per 10 seconds.
 */
static const struct timeval eso_err_interval = {
	.tv_sec = 10,
	.tv_usec = 0,
};
static struct timeval eso_err_lasttime;

/*
 * Handle TX offload in software. For TSO, split the packet into
 * chanks with payloads of size MSS. For chekcsum offload, update
 * required checksum fields. The results are more than one packet
 * in general. Return a mbuf queue consists of them.
 */

struct mbuf *
ether_sw_offload_tx(struct ifnet *ifp, struct mbuf *m)
{
	struct ether_header *ep;
	int flags, ehlen;
	uint16_t type;
#ifdef INET6
	bool v6;
#else
	bool v6 __diagused;
#endif

	KASSERT(m->m_flags & M_PKTHDR);
	flags = m->m_pkthdr.csum_flags;
	if (flags == 0)
		goto done;

	/* Sanity check */
	if (!TX_OFFLOAD_SUPPORTED(ifp->if_csum_flags_tx, flags))
		goto quit;

	KASSERT(m->m_pkthdr.len >= sizeof(*ep));
	KASSERT(m->m_len >= sizeof(*ep));
	ep = mtod(m, struct ether_header *);
	switch (type = ntohs(ep->ether_type)) {
	case ETHERTYPE_IP:
	case ETHERTYPE_IPV6:
		ehlen = ETHER_HDR_LEN;
		break;
	case ETHERTYPE_VLAN:
		ehlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
		break;
	default:
		if (ratecheck(&eso_err_lasttime, &eso_err_interval))
			log(LOG_ERR, "%s: %s: dropping invalid frame "
			    "type 0x%04hx csum_flags 0x%08x\n",
			    __func__, ifp->if_xname, type, flags);
		goto quit;
	}
	KASSERT(m->m_pkthdr.len >= ehlen);

	v6 = flags & (M_CSUM_TSOv6 | M_CSUM_TCPv6 | M_CSUM_UDPv6);
#ifndef INET6
	KASSERT(!v6);
#endif

	if (flags & (M_CSUM_TSOv4 | M_CSUM_TSOv6)) {
		/*
		 * tcp[46]_segment() assume that size of payloads is
		 * a multiple of MSS. Further, tcp6_segment() assumes
		 * no extension headers.
		 *
		 * XXX Do we need some KASSERT's?
		 */
#ifdef INET6
		if (v6)
			return tcp6_segment(m, ehlen);
		else
#endif
			return tcp4_segment(m, ehlen);
	}

#ifdef INET6
	if (v6)
		in6_undefer_cksum(m, ehlen, flags);
	else
#endif
		in_undefer_cksum(m, ehlen, flags);
done:
	m->m_pkthdr.csum_flags = 0;
	m->m_nextpkt = NULL;
	return m;
quit:
	m_freem(m);
	return NULL;
}

/*
 * Handle RX offload in software.
 *
 * XXX Fragmented packets or packets with IPv6 extension headers
 * are not currently supported.
 */

struct mbuf *
ether_sw_offload_rx(struct ifnet *ifp, struct mbuf *m)
{
	struct ether_header *eh;
	struct ip *ip;
	struct tcphdr *th;
	struct udphdr *uh;
	uint16_t sum, osum;
	uint8_t proto;
	int flags, enabled, len, ehlen, iphlen, l4offset;
	bool v6;

	flags = 0;

	enabled = ifp->if_csum_flags_rx;
	if (!(enabled & (M_CSUM_IPv4 | M_CSUM_TCPv4 | M_CSUM_UDPv4 |
	    M_CSUM_TCPv6 | M_CSUM_UDPv6)))
		goto done;

	KASSERT(m->m_flags & M_PKTHDR);
	len = m->m_pkthdr.len;

	KASSERT(len >= sizeof(*eh));
	if (m->m_len < sizeof(*eh)) {
		m = m_pullup(m, sizeof(*eh));
		if (m == NULL)
			return NULL;
	}
	eh = mtod(m, struct ether_header *);
	switch (htons(eh->ether_type)) {
	case ETHERTYPE_IP:
	case ETHERTYPE_IPV6:
		ehlen = ETHER_HDR_LEN;
		break;
	case ETHERTYPE_VLAN:
		ehlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
		break;
	default:
		goto done;
	}

	KASSERT(len >= ehlen);
	len = m->m_pkthdr.len - ehlen;

	KASSERT(len >= sizeof(*ip));
	if (m->m_len < ehlen + sizeof(*ip)) {
		m = m_pullup(m, ehlen + sizeof(*ip));
		if (m == NULL)
			return NULL;
	}
	ip = (void *)(mtod(m, char *) + ehlen);
	v6 = (ip->ip_v != IPVERSION);

	if (v6) {
#ifdef INET6
		struct ip6_hdr *ip6;

		KASSERT(len >= sizeof(*ip6));
		if (m->m_len < ehlen + sizeof(*ip6)) {
			m = m_pullup(m, ehlen + sizeof(*ip6));
			if (m == NULL)
				return NULL;
		}
		ip6 = (void *)(mtod(m, char *) + ehlen);
		KASSERT((ip6->ip6_vfc & IPV6_VERSION_MASK) == IPV6_VERSION);

		iphlen = sizeof(*ip6);

		len -= iphlen;

		proto = ip6->ip6_nxt;
		switch (proto) {
		case IPPROTO_TCP:
			if (!(enabled & M_CSUM_TCPv6))
				goto done;
			break;
		case IPPROTO_UDP:
			if (!(enabled & M_CSUM_UDPv6))
				goto done;
			break;
		default:
			/* XXX Extension headers are not supported. */
			goto done;
		}

		sum = in6_cksum_phdr(&ip6->ip6_src, &ip6->ip6_dst, htonl(len),
		    htonl(proto));
#else
		goto done;
#endif
	} else {
		if (enabled & M_CSUM_IPv4)
			flags |= M_CSUM_IPv4;

		iphlen = ip->ip_hl << 2;
		KASSERT(iphlen >= sizeof(*ip));

		len -= iphlen;
		KASSERT(len >= 0);

		if (in4_cksum(m, 0, ehlen, iphlen) != 0) {
			if (enabled & M_CSUM_IPv4)
				flags |= M_CSUM_IPv4_BAD;
			/* Broken. Do not check further. */
			goto done;
		}

		/* Check if fragmented. */
		if (ntohs(ip->ip_off) & ~(IP_DF | IP_RF))
			goto done;

		proto = ip->ip_p;
		switch (proto) {
		case IPPROTO_TCP:
			if (!(enabled & M_CSUM_TCPv4))
				goto done;
			break;
		case IPPROTO_UDP:
			if (!(enabled & M_CSUM_UDPv4))
				goto done;
			break;
		default:
			goto done;
		}

		sum = in_cksum_phdr(ip->ip_src.s_addr, ip->ip_dst.s_addr,
		    htons((uint16_t)len + proto));
	}

	l4offset = ehlen + iphlen;
	switch (proto) {
	case IPPROTO_TCP:
		KASSERT(len >= sizeof(*th));
		if (m->m_len < l4offset + sizeof(*th)) {
			m = m_pullup(m, l4offset + sizeof(*th));
			if (m == NULL)
				return NULL;
		}
		th = (void *)(mtod(m, char *) + l4offset);
		osum = th->th_sum;
		th->th_sum = sum;
#ifdef INET6
		if (v6) {
			flags |= M_CSUM_TCPv6;
			sum = in6_cksum(m, 0, l4offset, len);
		} else
#endif
		{
			flags |= M_CSUM_TCPv4;
			sum = in4_cksum(m, 0, l4offset, len);
		}
		if (sum != osum)
			flags |= M_CSUM_TCP_UDP_BAD;
		th->th_sum = osum;
		break;
	case IPPROTO_UDP:
		KASSERT(len >= sizeof(*uh));
		if (m->m_len < l4offset + sizeof(*uh)) {
			m = m_pullup(m, l4offset + sizeof(*uh));
			if (m == NULL)
				return NULL;
		}
		uh = (void *)(mtod(m, char *) + l4offset);
		osum = uh->uh_sum;
		if (osum == 0)
			break;
		uh->uh_sum = sum;
#ifdef INET6
		if (v6) {
			flags |= M_CSUM_UDPv6;
			sum = in6_cksum(m, 0, l4offset, len);
		} else
#endif
		{
			flags |= M_CSUM_UDPv4;
			sum = in4_cksum(m, 0, l4offset, len);
		}
		if (sum == 0)
			sum = 0xffff;
		if (sum != osum)
			flags |= M_CSUM_TCP_UDP_BAD;
		uh->uh_sum = osum;
		break;
	default:
		panic("%s: impossible", __func__);
	}

done:
	m->m_pkthdr.csum_flags = flags;
	return m;
}
