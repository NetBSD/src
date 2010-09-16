/*	$NetBSD: npf_sendpkt.c,v 1.1 2010/09/16 04:53:27 rmind Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: npf_sendpkt.c,v 1.1 2010/09/16 04:53:27 rmind Exp $");

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
 * npf_fetch_seqack: fetch TCP data length, SEQ and ACK numbers.
 *
 * NOTE: Returns in host byte-order.
 */
static inline bool
npf_fetch_seqack(nbuf_t *nbuf, npf_cache_t *npc,
    tcp_seq *seq, tcp_seq *ack, size_t *tcpdlen)
{
	void *n_ptr = nbuf_dataptr(nbuf);
	u_int offby;
	tcp_seq seqack[2];
	uint16_t iplen;
	uint8_t toff;

	/* Fetch total length of IP. */
	offby = offsetof(struct ip, ip_len);
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL)
		return false;
	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(uint16_t), &iplen))
		return false;

	/* Fetch SEQ and ACK numbers. */
	offby = (npc->npc_hlen - offby) + offsetof(struct tcphdr, th_seq);
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL)
		return false;
	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(seqack), seqack))
		return false;

	/* Fetch TCP data offset (header length) value. */
	offby = sizeof(seqack);
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL)
		return false;
	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(uint8_t), &toff))
		return false;
	toff >>= 4;

	*seq = ntohl(seqack[0]);
	*ack = ntohl(seqack[1]);
	*tcpdlen = ntohs(iplen) - npc->npc_hlen - (toff << 2);
	return true;
}

/*
 * npf_return_tcp: return a TCP reset (RST) packet.
 */
static int
npf_return_tcp(npf_cache_t *npc, nbuf_t *nbuf)
{
	struct mbuf *m;
	struct ip *ip;
	struct tcphdr *th;
	tcp_seq seq, ack;
	size_t tcpdlen, len;

	/* Fetch relevant data. */
	if (!npf_iscached(npc, NPC_IP46 | NPC_ADDRS | NPC_PORTS) ||
	    !npf_fetch_seqack(nbuf, npc, &seq, &ack, &tcpdlen)) {
		return EBADMSG;
	}
	if (npc->npc_tcp_flags & TH_RST) {
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
	ip->ip_src.s_addr = npc->npc_dstip;
	ip->ip_dst.s_addr = npc->npc_srcip;
	ip->ip_len = htons(sizeof(struct tcphdr));

	/* Construct TCP header and compute the checksum. */
	th = (struct tcphdr *)(ip + 1);
	th->th_sport = npc->npc_dport;
	th->th_dport = npc->npc_sport;
	th->th_seq = htonl(ack);
	if (npc->npc_tcp_flags & TH_SYN) {
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
	ip->ip_off = htons(IP_DF);
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
	const int proto = npc->npc_proto;

	if (!npf_iscached(npc, NPC_IP46) && !npf_ip4_proto(npc, nbuf, n_ptr))
		return;
	if ((proto == IPPROTO_TCP && (retfl & NPF_RULE_RETRST) == 0) ||
	    (proto == IPPROTO_UDP && (retfl & NPF_RULE_RETICMP) == 0)) {
		return;
	}
	switch (proto) {
	case IPPROTO_TCP:
		(void)npf_return_tcp(npc, nbuf);
		break;
	case IPPROTO_UDP:
		(void)npf_return_icmp(nbuf);
		break;
	}
}
