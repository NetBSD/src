/*	$NetBSD: in_offload.c,v 1.2 2007/04/24 23:43:50 dyoung Exp $	*/

/*-
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
__KERNEL_RCSID(0, "$NetBSD: in_offload.c,v 1.2 2007/04/24 23:43:50 dyoung Exp $");

#include <sys/param.h>
#include <sys/mbuf.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/in_offload.h>

struct ip_tso_output_args {
	struct ifnet *ifp;
	const struct sockaddr *sa;
	struct rtentry *rt;
};

static int ip_tso_output_callback(void *, struct mbuf *);

static int
ip_tso_output_callback(void *vp, struct mbuf *m)
{
	struct ip_tso_output_args *args = vp;
	struct ifnet *ifp = args->ifp;

	return (*ifp->if_output)(ifp, m, args->sa, args->rt);
}

int
ip_tso_output(struct ifnet *ifp, struct mbuf *m, const struct sockaddr *sa,
    struct rtentry *rt)
{
	struct ip_tso_output_args args;

	args.ifp = ifp;
	args.sa = sa;
	args.rt = rt;

	return tcp4_segment(m, ip_tso_output_callback, &args);
}

/*
 * tcp4_segment: handle M_CSUM_TSOv4 by software.
 *
 * => always consume m.
 * => call output_func with output_arg for each segments.
 */

int
tcp4_segment(struct mbuf *m, int (*output_func)(void *, struct mbuf *),
    void *output_arg)
{
	int mss;
	int iphlen;
	int thlen;
	int hlen;
	int len;
	struct ip *iph;
	struct tcphdr *th;
	uint16_t ipid;
	uint32_t tcpseq;
	struct mbuf *hdr = NULL;
	struct mbuf *t;
	int error = 0;

	KASSERT((m->m_flags & M_PKTHDR) != 0);
	KASSERT((m->m_pkthdr.csum_flags & M_CSUM_TSOv4) != 0);

	m->m_pkthdr.csum_flags = 0;

	len = m->m_pkthdr.len;
	KASSERT(len >= sizeof(*iph) + sizeof(*th));

	if (m->m_len < sizeof(*iph)) {
		m = m_pullup(m, sizeof(*iph));
		if (m == NULL) {
			error = ENOMEM;
			goto quit;
		}
	}
	iph = mtod(m, struct ip *);
	iphlen = iph->ip_hl * 4;
	KASSERT(iph->ip_v == IPVERSION);
	KASSERT(iphlen >= sizeof(*iph));
	KASSERT(iph->ip_p == IPPROTO_TCP);
	ipid = ntohs(iph->ip_id);

	hlen = iphlen + sizeof(*th);
	if (m->m_len < hlen) {
		m = m_pullup(m, hlen);
		if (m == NULL) {
			error = ENOMEM;
			goto quit;
		}
	}
	th = (void *)(mtod(m, char *) + iphlen);
	tcpseq = ntohl(th->th_seq);
	thlen = th->th_off * 4;
	hlen = iphlen + thlen;

	mss = m->m_pkthdr.segsz;
	KASSERT(mss != 0);
	KASSERT(len > hlen);

	t = m_split(m, hlen, M_NOWAIT);
	if (t == NULL) {
		error = ENOMEM;
		goto quit;
	}
	hdr = m;
	m = t;
	len -= hlen;
	KASSERT(len % mss == 0);
	while (len > 0) {
		struct mbuf *n;

		n = m_dup(hdr, 0, hlen, M_NOWAIT);
		if (n == NULL) {
			error = ENOMEM;
			goto quit;
		}
		KASSERT(n->m_len == hlen); /* XXX */

		t = m_split(m, mss, M_NOWAIT);
		if (t == NULL) {
			m_freem(n);
			error = ENOMEM;
			goto quit;
		}
		m_cat(n, m);
		m = t;

		KASSERT(n->m_len >= hlen); /* XXX */

		n->m_pkthdr.len = hlen + mss;
		iph = mtod(n, struct ip *);
		KASSERT(iph->ip_v == IPVERSION);
		iph->ip_len = htons(n->m_pkthdr.len);
		iph->ip_id = htons(ipid);
		th = (void *)(mtod(n, char *) + iphlen);
		th->th_seq = htonl(tcpseq);
		iph->ip_sum = 0;
		iph->ip_sum = in_cksum(n, iphlen);
		th->th_sum = 0;
		th->th_sum = in4_cksum(n, IPPROTO_TCP, iphlen, thlen + mss);

		error = (*output_func)(output_arg, n);
		if (error) {
			goto quit;
		}

		tcpseq += mss;
		ipid++;
		len -= mss;
	}

quit:
	if (hdr != NULL) {
		m_freem(hdr);
	}
	if (m != NULL) {
		m_freem(m);
	}

	return error;
}
