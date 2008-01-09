/*	$NetBSD: xennet_checksum.c,v 1.2.30.1 2008/01/09 01:50:23 matt Exp $	*/

/*-
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
__KERNEL_RCSID(0, "$NetBSD: xennet_checksum.c,v 1.2.30.1 2008/01/09 01:50:23 matt Exp $");

#include <sys/types.h>
#include <sys/param.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_vlanvar.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <xen/xennet_checksum.h>

static const void *m_extract(struct mbuf *, int, int, void *);
static void *m_extract_write(struct mbuf *, int, int, void *);

static void *m_extract1(struct mbuf *, int, int, void *, int);
#define	MBUF_EXTRACT_WRITE	1

static void *
m_extract1(struct mbuf *m, int off, int len, void *buf, int flags)
{
	void *result;

	KASSERT((m->m_flags & M_PKTHDR) != 0);

	if (m->m_pkthdr.len < off + len) {
		result = NULL;
	} else if (m->m_len >= off + len &&
	    ((flags & MBUF_EXTRACT_WRITE) != 0 || !M_READONLY(m))) {
		result = mtod(m, char *) + off;
	} else {
		m_copydata(m, off, len, buf);
		result = buf;
	}

	return result;
}

static const void *
m_extract(struct mbuf *m, int off, int len, void *buf)
{

	return m_extract1(m, off, len, buf, 0);
}

static void *
m_extract_write(struct mbuf *m, int off, int len, void *buf)
{

	return m_extract1(m, off, len, buf, MBUF_EXTRACT_WRITE);
}

/*
 * xennet_checksum_fill: fill TCP/UDP checksum
 */

int
xennet_checksum_fill(struct mbuf **mp)
{
	struct mbuf *m = *mp;
	struct ether_header eh_store;
	const struct ether_header *eh;
	struct ip iph_store;
	const struct ip *iph;
	int ehlen;
	int iphlen;
	int iplen;
	uint16_t etype;
	uint8_t nxt;
	int error = 0;

	eh = m_extract(m, 0, sizeof(*eh), &eh_store);
	if (eh == NULL) {
		return EINVAL;
	}
	etype = eh->ether_type;
	if (etype == htobe16(ETHERTYPE_VLAN)) {
		ehlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
	} else if (etype == htobe16(ETHERTYPE_IP)) {
		ehlen = ETHER_HDR_LEN;
	} else {
		return EINVAL;
	}

	iph = m_extract(m, ehlen, sizeof(*iph), &iph_store);
	if (iph == NULL) {
		return EINVAL;
	}
	nxt = iph->ip_p;
	iphlen = iph->ip_hl * 4;
	iplen = ntohs(iph->ip_len);
	if (ehlen + iplen != m->m_pkthdr.len) {
		return EINVAL;
	}
	if (nxt == IPPROTO_UDP) {
		struct udphdr uh_store;
		struct udphdr *uh;
		int ulen;

		uh = m_extract_write(m, ehlen + iphlen, sizeof(*uh), &uh_store);
		ulen = ntohs(uh->uh_ulen);
		if (ehlen + iphlen + ulen > m->m_pkthdr.len) {
			return EINVAL;
		}
		m->m_len -= ehlen;
		m->m_data += ehlen;
		uh->uh_sum = 0;
		uh->uh_sum = in4_cksum(m, nxt, iphlen, iplen - iphlen);
		m->m_len += ehlen;
		m->m_data -= ehlen;
		if (uh != &uh_store) {
			m = m_copyback_cow(m, ehlen + iphlen, sizeof(*uh), uh,
			    M_DONTWAIT);
			if (m == NULL) {
				error = ENOMEM;
			}
		}
	} else if (nxt == IPPROTO_TCP) {
		struct tcphdr th_store;
		struct tcphdr *th;
		int thlen;

		th = m_extract_write(m, ehlen + iphlen, sizeof(*th), &th_store);
		thlen = th->th_off * 4;
		if (ehlen + iphlen + thlen > m->m_pkthdr.len) {
			return EINVAL;
		}
		m->m_len -= ehlen;
		m->m_data += ehlen;
		th->th_sum = 0;
		th->th_sum = in4_cksum(m, nxt, iphlen, iplen - iphlen);
		m->m_len += ehlen;
		m->m_data -= ehlen;
		if (th != &th_store) {
			m = m_copyback_cow(m, ehlen + iphlen, sizeof(*th), th,
			    M_DONTWAIT);
			if (m == NULL) {
				error = ENOMEM;
			}
		}
	} else {
		error = EINVAL;
	}

	*mp = m;
	return error;
}
