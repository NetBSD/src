/*	$NetBSD: in_offload.c,v 1.15 2024/07/05 04:31:54 rin Exp $	*/

/*
 * Copyright (c)2005, 2006 YAMAMOTO Takashi,
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
__KERNEL_RCSID(0, "$NetBSD: in_offload.c,v 1.15 2024/07/05 04:31:54 rin Exp $");

#include <sys/param.h>
#include <sys/mbuf.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/in_offload.h>

/*
 * Handle M_CSUM_TSOv4 in software. Split the TCP payload in chunks of
 * size MSS, and return mbuf chain consists of them.
 */
struct mbuf *
tcp4_segment(struct mbuf *m, int off)
{
	int mss;
	int iphlen, thlen;
	int hlen, len;
	struct ip *ip;
	struct tcphdr *th;
	uint16_t ipid, phsum;
	uint32_t tcpseq;
	struct mbuf *hdr = NULL;
	struct mbuf *m0 = NULL;
	struct mbuf *prev = NULL;
	struct mbuf *n, *t;
	int nsegs;

	KASSERT((m->m_flags & M_PKTHDR) != 0);
	KASSERT((m->m_pkthdr.csum_flags & M_CSUM_TSOv4) != 0);

	m->m_pkthdr.csum_flags = 0;

	len = m->m_pkthdr.len;
	KASSERT(len >= off + sizeof(*ip) + sizeof(*th));

	hlen = off + sizeof(*ip);
	if (m->m_len < hlen) {
		m = m_pullup(m, hlen);
		if (m == NULL)
			goto quit;
	}
	ip = (void *)(mtod(m, char *) + off);
	iphlen = ip->ip_hl * 4;
	KASSERT(ip->ip_v == IPVERSION);
	KASSERT(iphlen >= sizeof(*ip));
	KASSERT(ip->ip_p == IPPROTO_TCP);
	ipid = ntohs(ip->ip_id);

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

	ip = (void *)(mtod(hdr, char *) + off);
	ip->ip_len = htons(iphlen + thlen + mss);
	phsum = in_cksum_phdr(ip->ip_src.s_addr, ip->ip_dst.s_addr,
	    htons((uint16_t)(thlen + mss) + IPPROTO_TCP));

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

		ip = (void *)(mtod(n, char *) + off);
		ip->ip_id = htons(ipid);
		ip->ip_sum = 0;
		ip->ip_sum = in4_cksum(n, 0, off, iphlen);

		th = (void *)(mtod(n, char *) + off + iphlen);
		th->th_seq = htonl(tcpseq);
		th->th_sum = phsum;
		th->th_sum = in4_cksum(n, 0, off + iphlen, thlen + mss);

		tcpseq += mss;
		ipid++;
		prev = n;
	}
	return m0;

quit:
	m_freem(hdr);
	m_freem(m);
	for (m = m0; m != NULL; m = n) {
		n = m->m_nextpkt;
		m_freem(m);
	}

	return NULL;
}

int
ip_tso_output(struct ifnet *ifp, struct mbuf *m, const struct sockaddr *sa,
    struct rtentry *rt)
{
	struct mbuf *n;
	int error = 0;

	m = tcp4_segment(m, 0);
	if (m == NULL)
		return ENOMEM;
	do {
		n = m->m_nextpkt;
		if (error == 0)
			error = ip_if_output(ifp, m, sa, rt);
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
in_undefer_cksum(struct mbuf *mh, size_t hdrlen, int csum_flags)
{
	const size_t iphdrlen = M_CSUM_DATA_IPv4_IPHL(mh->m_pkthdr.csum_data);
	uint16_t csum;
	uint16_t ip_len;
	uint16_t *csump;
	struct mbuf *m = mh;

	KASSERT(mh->m_flags & M_PKTHDR);
	KASSERT(mh->m_pkthdr.len > hdrlen);
	KASSERT((mh->m_pkthdr.csum_flags & csum_flags) == csum_flags);

	/*
	 * Deal with prepended frame header as done by e.g. ether_output().
	 * If first mbuf in chain has just the header, use second mbuf
	 * for the actual checksum. in4_csum() expects the passed mbuf
	 * to have the whole (struct ip) area contiguous.
	 */
	if (m->m_len <= hdrlen) {
		hdrlen -= m->m_len;
		m = m->m_next;
		KASSERT(m != NULL);
	}

	if (__predict_true(hdrlen + sizeof(struct ip) <= m->m_len)) {
		struct ip *ip = (struct ip *)(mtod(m, uint8_t *) + hdrlen);

		ip_len = ip->ip_len;
		csump = &ip->ip_sum;
	} else {
		const size_t ip_len_offset =
		    hdrlen + offsetof(struct ip, ip_len);

		m_copydata(m, ip_len_offset, sizeof(ip_len), &ip_len);
		csump = NULL;
	}
	ip_len = ntohs(ip_len);

	if (csum_flags & M_CSUM_IPv4) {
		csum = in4_cksum(m, 0, hdrlen, iphdrlen);
		if (csump != NULL) {
			*csump = csum;
		} else {
			const size_t offset = hdrlen +
			    offsetof(struct ip, ip_sum);

			m_copyback(m, offset, sizeof(uint16_t), &csum);
		}
	}

	if (csum_flags & (M_CSUM_UDPv4|M_CSUM_TCPv4)) {
		size_t l4offset = hdrlen + iphdrlen;

		csum = in4_cksum(m, 0, l4offset, ip_len - iphdrlen);
		if (csum == 0 && (csum_flags & M_CSUM_UDPv4) != 0)
			csum = 0xffff;

		l4offset += M_CSUM_DATA_IPv4_OFFSET(m->m_pkthdr.csum_data);

		if (__predict_true(l4offset + sizeof(uint16_t) <= m->m_len)) {
			*(uint16_t *)(mtod(m, char *) + l4offset) = csum;
		} else {
			m_copyback(m, l4offset, sizeof(csum), (void *)&csum);
		}
	}

	mh->m_pkthdr.csum_flags ^= csum_flags;
}

/*
 * Compute now in software the TCP/UDP checksum. Cancel the hardware
 * offloading.
 */
void
in_undefer_cksum_tcpudp(struct mbuf *m)
{
	struct ip *ip;
	uint16_t csum, offset;

	KASSERT((m->m_flags & M_PKTHDR) != 0);
	KASSERT((m->m_pkthdr.csum_flags & (M_CSUM_TCPv4|M_CSUM_UDPv4)) != 0);
	KASSERT((m->m_pkthdr.csum_flags & (M_CSUM_TCPv6|M_CSUM_UDPv6)) == 0);

	ip = mtod(m, struct ip *);
	offset = ip->ip_hl << 2;

	csum = in4_cksum(m, 0, offset, ntohs(ip->ip_len) - offset);
	if (csum == 0 && (m->m_pkthdr.csum_flags & M_CSUM_UDPv4) != 0)
		csum = 0xffff;

	offset += M_CSUM_DATA_IPv4_OFFSET(m->m_pkthdr.csum_data);

	if ((offset + sizeof(uint16_t)) <= m->m_len) {
		*(uint16_t *)(mtod(m, char *) + offset) = csum;
	} else {
		m_copyback(m, offset, sizeof(csum), (void *)&csum);
	}
}
