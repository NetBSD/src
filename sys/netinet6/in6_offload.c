/*	$NetBSD: in6_offload.c,v 1.8.2.1 2019/06/10 22:09:48 christos Exp $	*/

/*
 * Copyright (c)2006 YAMAMOTO Takashi,
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: in6_offload.c,v 1.8.2.1 2019/06/10 22:09:48 christos Exp $");

#include <sys/param.h>
#include <sys/mbuf.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet6/in6_var.h>
#include <netinet6/ip6_var.h>
#include <netinet6/nd6.h>
#include <netinet6/in6_offload.h>

/*
 * Handle M_CSUM_TSOv6 in software. Split the TCP payload in chunks of
 * size MSS, and return mbuf chain consists of them.
 */
struct mbuf *
tcp6_segment(struct mbuf *m, int off)
{
	int mss;
	int iphlen;
	int thlen;
	int hlen;
	int len;
	struct ip6_hdr *iph;
	struct tcphdr *th;
	uint32_t tcpseq;
	uint16_t phsum;
	struct mbuf *hdr = NULL;
	struct mbuf *m0 = NULL;
	struct mbuf *prev = NULL;
	struct mbuf *n, *t;
	int nsegs;

	KASSERT((m->m_flags & M_PKTHDR) != 0);
	KASSERT((m->m_pkthdr.csum_flags & M_CSUM_TSOv6) != 0);

	m->m_pkthdr.csum_flags = 0;

	len = m->m_pkthdr.len;
	KASSERT(len >= off + sizeof(*iph) + sizeof(*th));

	hlen = off + sizeof(*iph);
	if (m->m_len < hlen) {
		m = m_pullup(m, hlen);
		if (m == NULL)
			goto quit;
	}
	iph = (void *)(mtod(m, char *) + off);
	iphlen = sizeof(*iph);
	KASSERT((iph->ip6_vfc & IPV6_VERSION_MASK) == IPV6_VERSION);
	KASSERT(iph->ip6_nxt == IPPROTO_TCP);

	hlen = off + iphlen + sizeof(*th);
	if (m->m_len < hlen) {
		m = m_pullup(m, hlen);
		if (m == NULL)
			goto quit;
	}
	th = (void *)(mtod(m, char *) + off + iphlen);
	tcpseq = ntohl(th->th_seq);
	thlen = th->th_off * 4;
	hlen = off + iphlen + thlen;

	mss = m->m_pkthdr.segsz;
	KASSERT(mss != 0);
	KASSERT(len > hlen);

	t = m_split(m, hlen, M_NOWAIT);
	if (t == NULL)
		goto quit;
	hdr = m;
	m = t;

	len -= hlen;
	KASSERT(len % mss == 0);

	iph = (void *)(mtod(hdr, char *) + off);
	iph->ip6_plen = htons(thlen + mss);
	phsum = in6_cksum_phdr(&iph->ip6_src, &iph->ip6_dst, htonl(thlen + mss),
	    htonl(IPPROTO_TCP));

	for (nsegs = len / mss; nsegs > 0; nsegs--) {
		if (nsegs > 1) {
			n = m_dup(hdr, 0, hlen, M_NOWAIT);
			if (n == NULL)
				goto quit;
		} else
			n = hdr;
		KASSERT(n->m_len == hlen); /* XXX */

		if (nsegs > 1) {
			t = m_split(m, mss, M_NOWAIT);
			if (t == NULL) {
				m_freem(n);
				goto quit;
			}
		} else
			t = m;
		m_cat(n, m);
		m = t;

		KASSERT(n->m_len >= hlen); /* XXX */

		if (m0 == NULL)
			m0 = n;

		if (prev != NULL)
			prev->m_nextpkt = n;

		n->m_pkthdr.len = hlen + mss;
		n->m_nextpkt = NULL;	/* XXX */

		th = (void *)(mtod(n, char *) + off + iphlen);
		th->th_seq = htonl(tcpseq);
		th->th_sum = phsum;
		th->th_sum = in6_cksum(n, 0, off + iphlen, thlen + mss);

		tcpseq += mss;
		prev = n;
	}
	return m0;

quit:
	if (hdr != NULL)
		m_freem(hdr);
	if (m != NULL)
		m_freem(m);
	for (m = m0; m != NULL; m = n) {
		n = m->m_nextpkt;
		m_freem(m);
	}

	return NULL;
}

int
ip6_tso_output(struct ifnet *ifp, struct ifnet *origifp, struct mbuf *m,
    const struct sockaddr_in6 *dst, struct rtentry *rt)
{
	struct mbuf *n;
	int error = 0;

	m = tcp6_segment(m, 0);
	if (m == NULL)
		return ENOMEM;
	do {
		n = m->m_nextpkt;
		if (error == 0)
			error = ip6_if_output(ifp, origifp, m, dst, rt);
		else
			m_freem(m);
		m = n;
	} while (m != NULL);
	return error;
}

/*
 * Compute now in software the IP and TCP/UDP checksums. Cancel the
 * hardware offloading.
 */
void
in6_undefer_cksum(struct mbuf *m, size_t hdrlen, int csum_flags)
{
	const size_t ip6_plen_offset =
	    hdrlen + offsetof(struct ip6_hdr, ip6_plen);
	size_t l4hdroff;
	size_t l4offset;
	uint16_t plen;
	uint16_t csum;

	KASSERT(m->m_flags & M_PKTHDR);
	KASSERT((m->m_pkthdr.csum_flags & csum_flags) == csum_flags);
	KASSERT(csum_flags == M_CSUM_UDPv6 || csum_flags == M_CSUM_TCPv6);

	if (__predict_true(hdrlen + sizeof(struct ip6_hdr) <= m->m_len)) {
		plen = *(uint16_t *)(mtod(m, char *) + ip6_plen_offset);
	} else {
		m_copydata(m, ip6_plen_offset, sizeof(plen), &plen);
	}
	plen = ntohs(plen);

	l4hdroff = M_CSUM_DATA_IPv6_IPHL(m->m_pkthdr.csum_data);
	l4offset = hdrlen + l4hdroff;
	csum = in6_cksum(m, 0, l4offset,
	    plen - (l4hdroff - sizeof(struct ip6_hdr)));

	if (csum == 0 && (csum_flags & M_CSUM_UDPv6) != 0)
		csum = 0xffff;

	l4offset += M_CSUM_DATA_IPv6_OFFSET(m->m_pkthdr.csum_data);

	if (__predict_true((l4offset + sizeof(uint16_t)) <= m->m_len)) {
		*(uint16_t *)(mtod(m, char *) + l4offset) = csum;
	} else {
		m_copyback(m, l4offset, sizeof(csum), (void *) &csum);
	}

	m->m_pkthdr.csum_flags ^= csum_flags;
}

/*
 * Compute now in software the TCP/UDP checksum. Cancel the hardware
 * offloading.
 */
void
in6_undefer_cksum_tcpudp(struct mbuf *m)
{
	uint16_t csum, offset;

	KASSERT((m->m_pkthdr.csum_flags & (M_CSUM_UDPv6|M_CSUM_TCPv6)) != 0);
	KASSERT((~m->m_pkthdr.csum_flags & (M_CSUM_UDPv6|M_CSUM_TCPv6)) != 0);
	KASSERT((m->m_pkthdr.csum_flags
	    & (M_CSUM_UDPv4|M_CSUM_TCPv4|M_CSUM_TSOv4)) == 0);

	offset = M_CSUM_DATA_IPv6_IPHL(m->m_pkthdr.csum_data);
	csum = in6_cksum(m, 0, offset, m->m_pkthdr.len - offset);
	if (csum == 0 && (m->m_pkthdr.csum_flags & M_CSUM_UDPv6) != 0) {
		csum = 0xffff;
	}

	offset += M_CSUM_DATA_IPv6_OFFSET(m->m_pkthdr.csum_data);
	if ((offset + sizeof(csum)) > m->m_len) {
		m_copyback(m, offset, sizeof(csum), &csum);
	} else {
		*(uint16_t *)(mtod(m, char *) + offset) = csum;
	}
}
