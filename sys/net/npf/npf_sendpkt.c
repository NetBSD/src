/*	$NetBSD: npf_sendpkt.c,v 1.3 2010/11/11 06:30:39 rmind Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
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

/*
 * NPF module for packet construction routines.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_sendpkt.c,v 1.3 2010/11/11 06:30:39 rmind Exp $");

#include <sys/param.h>
#include <sys/kernel.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#endif
#include <sys/mbuf.h>

#include "npf_impl.h"

#define	DEFAULT_IP_TTL		(ip_defttl)

/*
 * npf_return_tcp: return a TCP reset (RST) packet.
 */
static int
npf_return_tcp(npf_cache_t *npc, nbuf_t *nbuf)
{
	struct mbuf *m;
	struct ip *oip, *ip;
	struct tcphdr *oth, *th;
	tcp_seq seq, ack;
	int tcpdlen, len;
	uint32_t win;

	/* Fetch relevant data. */
	KASSERT(npf_iscached(npc, NPC_IP46 | NPC_LAYER4));
	tcpdlen = npf_tcpsaw(npc, &seq, &ack, &win);
	oip = &npc->npc_ip.v4;
	oth = &npc->npc_l4.tcp;

	if (oth->th_flags & TH_RST) {
		return 0;
	}

	/* Create and setup a network buffer. */
	len = sizeof(struct ip) + sizeof(struct tcphdr);
	m = m_gethdr(M_DONTWAIT, MT_HEADER);
	if (m == NULL) {
		return ENOMEM;
	}
	m->m_data += max_linkhdr;
	m->m_len = len;
	m->m_pkthdr.len = len;

	ip = mtod(m, struct ip *);
	memset(ip, 0, len);

	/*
	 * First fill of IPv4 header, for TCP checksum.
	 * Note: IP length contains TCP header length.
	 */
	ip->ip_p = IPPROTO_TCP;
	ip->ip_src.s_addr = oip->ip_dst.s_addr;
	ip->ip_dst.s_addr = oip->ip_src.s_addr;
	ip->ip_len = htons(sizeof(struct tcphdr));

	/* Construct TCP header and compute the checksum. */
	th = (struct tcphdr *)(ip + 1);
	th->th_sport = oth->th_dport;
	th->th_dport = oth->th_sport;
	th->th_seq = htonl(ack);
	if (oth->th_flags & TH_SYN) {
		tcpdlen++;
	}
	th->th_ack = htonl(seq + tcpdlen);
	th->th_off = sizeof(struct tcphdr) >> 2;
	th->th_flags = TH_ACK | TH_RST;
	th->th_sum = in_cksum(m, len);

	/* Second fill of IPv4 header, fill correct IP length. */
	ip->ip_v = IPVERSION;
	ip->ip_hl = sizeof(struct ip) >> 2;
	ip->ip_tos = IPTOS_LOWDELAY;
	ip->ip_len = htons(len);
	ip->ip_ttl = DEFAULT_IP_TTL;

	/* Pass to IP layer. */
	return ip_output(m, NULL, NULL, IP_FORWARDING, NULL, NULL);
}

/*
 * npf_return_icmp: return an ICMP error.
 */
static int
npf_return_icmp(nbuf_t *nbuf)
{
	struct mbuf *m = nbuf;

	icmp_error(m, ICMP_UNREACH, ICMP_UNREACH_ADMIN_PROHIBIT, 0, 0);
	return 0;
}

/*
 * npf_return_block: return TCP reset or ICMP host unreachable packet.
 */
void
npf_return_block(npf_cache_t *npc, nbuf_t *nbuf, const int retfl)
{
	void *n_ptr = nbuf_dataptr(nbuf);

	if (!npf_iscached(npc, NPC_IP46) && !npf_fetch_ip(npc, nbuf, n_ptr)) {
		return;
	}
	switch (npf_cache_ipproto(npc)) {
	case IPPROTO_TCP:
		if (retfl & NPF_RULE_RETRST) {
			if (!npf_fetch_tcp(npc, nbuf, n_ptr)) {
				return;
			}
			(void)npf_return_tcp(npc, nbuf);
		}
		break;
	case IPPROTO_UDP:
		if (retfl & NPF_RULE_RETICMP) {
			(void)npf_return_icmp(nbuf);
		}
		break;
	}
}
