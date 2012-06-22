/*	$NetBSD: npf_inet.c,v 1.12 2012/06/22 13:43:17 rmind Exp $	*/

/*-
 * Copyright (c) 2009-2012 The NetBSD Foundation, Inc.
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
 * Various procotol related helper routines.
 *
 * This layer manipulates npf_cache_t structure i.e. caches requested headers
 * and stores which information was cached in the information bit field.
 * It is also responsibility of this layer to update or invalidate the cache
 * on rewrites (e.g. by translation routines).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_inet.c,v 1.12 2012/06/22 13:43:17 rmind Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <net/pfil.h>
#include <net/if.h>
#include <net/ethertypes.h>
#include <net/if_ether.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

#include "npf_impl.h"

/*
 * npf_fixup{16,32}_cksum: update IPv4 checksum.
 */

uint16_t
npf_fixup16_cksum(uint16_t cksum, uint16_t odatum, uint16_t ndatum)
{
	uint32_t sum;

	/*
	 * RFC 1624:
	 *	HC' = ~(~HC + ~m + m')
	 */
	sum = ~ntohs(cksum) & 0xffff;
	sum += (~ntohs(odatum) & 0xffff) + ntohs(ndatum);
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	return htons(~sum & 0xffff);
}

uint16_t
npf_fixup32_cksum(uint16_t cksum, uint32_t odatum, uint32_t ndatum)
{

	cksum = npf_fixup16_cksum(cksum, odatum & 0xffff, ndatum & 0xffff);
	cksum = npf_fixup16_cksum(cksum, odatum >> 16, ndatum >> 16);
	return cksum;
}

/*
 * npf_addr_cksum: calculate checksum of the address, either IPv4 or IPv6.
 */
uint16_t
npf_addr_cksum(uint16_t cksum, int sz, npf_addr_t *oaddr, npf_addr_t *naddr)
{
	uint32_t *oip32 = (uint32_t *)oaddr, *nip32 = (uint32_t *)naddr;

	KASSERT(sz % sizeof(uint32_t) == 0);
	do {
		cksum = npf_fixup32_cksum(cksum, *oip32++, *nip32++);
		sz -= sizeof(uint32_t);
	} while (sz);

	return cksum;
}

/*
 * npf_addr_sum: provide IP address as a summed (if needed) 32-bit integer.
 * Note: used for hash function.
 */
uint32_t
npf_addr_sum(const int sz, const npf_addr_t *a1, const npf_addr_t *a2)
{
	uint32_t mix = 0;
	int i;

	KASSERT(sz > 0 && a1 != NULL && a2 != NULL);

	for (i = 0; i < (sz >> 2); i++) {
		mix += a1->s6_addr32[i];
		mix += a2->s6_addr32[i];
	}
	return mix;
}

static inline void
npf_generate_mask(npf_addr_t *out, const npf_netmask_t mask)
{
	uint_fast8_t length = mask;

	/* Note: maximum length is 32 for IPv4 and 128 for IPv6. */
	KASSERT(length <= NPF_MAX_NETMASK);

	for (int i = 0; i < 4; i++) {
		if (length >= 32) {
			out->s6_addr32[i] = htonl(0xffffffff);
			length -= 32;
		} else {
			out->s6_addr32[i] = htonl(0xffffffff << (32 - length));
			length = 0;
		}
	}
}

/*
 * npf_addr_mask: apply the mask to a given address and store the result.
 */
void
npf_addr_mask(const npf_addr_t *addr, const npf_netmask_t mask, npf_addr_t *out)
{
	npf_addr_t realmask;

	npf_generate_mask(&realmask, mask);

	for (int i = 0; i < 4; i++) {
		out->s6_addr32[i] = addr->s6_addr32[i] & realmask.s6_addr32[i];
	}
}

/*
 * npf_addr_cmp: compare two addresses, either IPv4 or IPv6.
 *
 * => Ignore the mask, if NPF_NO_NETMASK is specified.
 * => Return 0 if equal and -1 or 1 if less or greater accordingly.
 */
int
npf_addr_cmp(const npf_addr_t *addr1, const npf_netmask_t mask1,
    const npf_addr_t *addr2, const npf_netmask_t mask2)
{
	npf_addr_t realmask1, realmask2;

	if (mask1 != NPF_NO_NETMASK) {
		npf_generate_mask(&realmask1, mask1);
	}
	if (mask2 != NPF_NO_NETMASK) {
		npf_generate_mask(&realmask2, mask2);
	}

	for (int i = 0; i < 4; i++) {
		const uint32_t x = mask1 != NPF_NO_NETMASK ?
		    addr1->s6_addr32[i] & realmask1.s6_addr32[i] :
		    addr1->s6_addr32[i];
		const uint32_t y = mask2 != NPF_NO_NETMASK ?
		    addr2->s6_addr32[i] & realmask2.s6_addr32[i] :
		    addr2->s6_addr32[i];
		if (x < y) {
			return -1;
		}
		if (x > y) {
			return 1;
		}
	}

	return 0;
}

/*
 * npf_tcpsaw: helper to fetch SEQ, ACK, WIN and return TCP data length.
 *
 * => Returns all values in host byte-order.
 */
int
npf_tcpsaw(const npf_cache_t *npc, tcp_seq *seq, tcp_seq *ack, uint32_t *win)
{
	const struct tcphdr *th = &npc->npc_l4.tcp;
	u_int thlen;

	KASSERT(npf_iscached(npc, NPC_TCP));

	*seq = ntohl(th->th_seq);
	*ack = ntohl(th->th_ack);
	*win = (uint32_t)ntohs(th->th_win);
	thlen = th->th_off << 2;

	if (npf_iscached(npc, NPC_IP4)) {
		const struct ip *ip = &npc->npc_ip.v4;
		return ntohs(ip->ip_len) - npf_cache_hlen(npc) - thlen;
	} else if (npf_iscached(npc, NPC_IP6)) {
		const struct ip6_hdr *ip6 = &npc->npc_ip.v6;
		return ntohs(ip6->ip6_plen) - thlen;
	}
	return 0;
}

/*
 * npf_fetch_tcpopts: parse and return TCP options.
 */
bool
npf_fetch_tcpopts(const npf_cache_t *npc, nbuf_t *nbuf,
    uint16_t *mss, int *wscale)
{
	void *n_ptr = nbuf_dataptr(nbuf);
	const struct tcphdr *th = &npc->npc_l4.tcp;
	int topts_len, step;
	uint16_t val16;
	uint8_t val;

	KASSERT(npf_iscached(npc, NPC_IP46));
	KASSERT(npf_iscached(npc, NPC_TCP));

	/* Determine if there are any TCP options, get their length. */
	topts_len = (th->th_off << 2) - sizeof(struct tcphdr);
	if (topts_len <= 0) {
		/* No options. */
		return false;
	}
	KASSERT(topts_len <= MAX_TCPOPTLEN);

	/* First step: IP and TCP header up to options. */
	step = npf_cache_hlen(npc) + sizeof(struct tcphdr);
next:
	if (nbuf_advfetch(&nbuf, &n_ptr, step, sizeof(val), &val)) {
		return false;
	}

	switch (val) {
	case TCPOPT_EOL:
		/* Done. */
		return true;
	case TCPOPT_NOP:
		topts_len--;
		step = 1;
		break;
	case TCPOPT_MAXSEG:
		/*
		 * XXX: clean this mess.
		 */
		if (mss && *mss) {
			val16 = *mss;
			if (nbuf_advstore(&nbuf, &n_ptr, 2,
			    sizeof(val16), &val16))
				return false;
		} else if (nbuf_advfetch(&nbuf, &n_ptr, 2,
		    sizeof(val16), &val16)) {
			return false;
		}
		if (mss) {
			*mss = val16;
		}
		topts_len -= TCPOLEN_MAXSEG;
		step = sizeof(val16);
		break;
	case TCPOPT_WINDOW:
		/* TCP Window Scaling (RFC 1323). */
		if (nbuf_advfetch(&nbuf, &n_ptr, 2, sizeof(val), &val)) {
			return false;
		}
		*wscale = (val > TCP_MAX_WINSHIFT) ? TCP_MAX_WINSHIFT : val;
		topts_len -= TCPOLEN_WINDOW;
		step = sizeof(val);
		break;
	default:
		if (nbuf_advfetch(&nbuf, &n_ptr, 1, sizeof(val), &val)) {
			return false;
		}
		if (val < 2 || val >= topts_len) {
			return false;
		}
		topts_len -= val;
		step = val - 1;
	}

	/* Any options left? */
	if (__predict_true(topts_len > 0)) {
		goto next;
	}
	return true;
}

/*
 * npf_fetch_ip: fetch, check and cache IP header.
 */
bool
npf_fetch_ip(npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr)
{
	uint8_t ver;

	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(uint8_t), &ver)) {
		return false;
	}

	switch (ver >> 4) {
	case IPVERSION: {
		struct ip *ip = &npc->npc_ip.v4;

		/* Fetch IPv4 header. */
		if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(struct ip), ip)) {
			return false;
		}

		/* Check header length and fragment offset. */
		if ((u_int)(ip->ip_hl << 2) < sizeof(struct ip)) {
			return false;
		}
		if (ip->ip_off & ~htons(IP_DF | IP_RF)) {
			/* Note fragmentation. */
			npc->npc_info |= NPC_IPFRAG;
		}

		/* Cache: layer 3 - IPv4. */
		npc->npc_ipsz = sizeof(struct in_addr);
		npc->npc_srcip = (npf_addr_t *)&ip->ip_src;
		npc->npc_dstip = (npf_addr_t *)&ip->ip_dst;
		npc->npc_info |= NPC_IP4;
		npc->npc_hlen = ip->ip_hl << 2;
		npc->npc_next_proto = npc->npc_ip.v4.ip_p;
		break;
	}

	case (IPV6_VERSION >> 4): {
		struct ip6_hdr *ip6 = &npc->npc_ip.v6;
		size_t toskip;
		bool done;

		/* Fetch IPv6 header. */
		if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(struct ip6_hdr), ip6)) {
			return false;
		}

		/* Initial next-protocol value. */
		npc->npc_next_proto = ip6->ip6_nxt;
		toskip = sizeof(struct ip6_hdr);
		npc->npc_hlen = 0;
		done = false;

		/*
		 * Advance by the length of the previous known header and
		 * fetch by the lengh of next extension header.
		 */
		do {
			struct ip6_ext ip6e;

			if (nbuf_advfetch(&nbuf, &n_ptr, toskip,
			    sizeof(struct ip6_ext), &ip6e)) {
				return false;
			}
			switch (npc->npc_next_proto) {
			case IPPROTO_DSTOPTS:
			case IPPROTO_ROUTING:
				toskip = (ip6e.ip6e_len + 1) << 3;
				break;
			case IPPROTO_FRAGMENT:
				npc->npc_info |= NPC_IPFRAG;
				toskip = sizeof(struct ip6_frag);
				break;
			case IPPROTO_AH:
				toskip = (ip6e.ip6e_len + 2) << 2;
				break;
			default:
				done = true;
				break;
			}
			npc->npc_hlen += toskip;
			npc->npc_next_proto = ip6e.ip6e_nxt;

		} while (!done);

		/* Cache: layer 3 - IPv6. */
		npc->npc_ipsz = sizeof(struct in6_addr);
		npc->npc_srcip = (npf_addr_t *)&ip6->ip6_src;
		npc->npc_dstip = (npf_addr_t *)&ip6->ip6_dst;
		npc->npc_info |= NPC_IP6;
		break;
	}
	default:
		return false;
	}

	return true;
}

/*
 * npf_fetch_tcp: fetch, check and cache TCP header.  If necessary,
 * fetch and cache layer 3 as well.
 */
bool
npf_fetch_tcp(npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr)
{
	struct tcphdr *th;

	/* Must have IP header processed for its length and protocol. */
	if (!npf_iscached(npc, NPC_IP46) && !npf_fetch_ip(npc, nbuf, n_ptr)) {
		return false;
	}
	if (npf_cache_ipproto(npc) != IPPROTO_TCP) {
		return false;
	}
	th = &npc->npc_l4.tcp;

	/* Fetch TCP header. */
	if (nbuf_advfetch(&nbuf, &n_ptr, npf_cache_hlen(npc),
	    sizeof(struct tcphdr), th)) {
		return false;
	}

	/* Cache: layer 4 - TCP. */
	npc->npc_info |= (NPC_LAYER4 | NPC_TCP);
	return true;
}

/*
 * npf_fetch_udp: fetch, check and cache UDP header.  If necessary,
 * fetch and cache layer 3 as well.
 */
bool
npf_fetch_udp(npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr)
{
	struct ip *ip = &npc->npc_ip.v4;
	struct udphdr *uh;
	u_int hlen;

	/* Must have IP header processed for its length and protocol. */
	if (!npf_iscached(npc, NPC_IP46) && !npf_fetch_ip(npc, nbuf, n_ptr)) {
		return false;
	}
	if (ip->ip_p != IPPROTO_UDP) {
		return false;
	}
	uh = &npc->npc_l4.udp;
	hlen = npf_cache_hlen(npc);

	/* Fetch UDP header. */
	if (nbuf_advfetch(&nbuf, &n_ptr, hlen, sizeof(struct udphdr), uh)) {
		return false;
	}

	/* Cache: layer 4 - UDP. */
	npc->npc_info |= (NPC_LAYER4 | NPC_UDP);
	return true;
}

/*
 * npf_fetch_icmp: fetch ICMP code, type and possible query ID.
 */
bool
npf_fetch_icmp(npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr)
{
	struct ip *ip = &npc->npc_ip.v4;
	struct icmp *ic;
	u_int hlen, iclen;

	/* Must have IP header processed for its length and protocol. */
	if (!npf_iscached(npc, NPC_IP46) && !npf_fetch_ip(npc, nbuf, n_ptr)) {
		return false;
	}
	if (ip->ip_p != IPPROTO_ICMP) {
		return false;
	}
	ic = &npc->npc_l4.icmp;
	hlen = npf_cache_hlen(npc);

	/* Fetch basic ICMP header, up to the "data" point. */
	iclen = offsetof(struct icmp, icmp_data);
	if (nbuf_advfetch(&nbuf, &n_ptr, hlen, iclen, ic)) {
		return false;
	}

	/* Cache: layer 4 - ICMP. */
	npc->npc_info |= (NPC_LAYER4 | NPC_ICMP);
	return true;
}

/*
 * npf_cache_all: general routine to cache all relevant IP (v4 or v6)
 * and TCP, UDP or ICMP headers.
 */
int
npf_cache_all(npf_cache_t *npc, nbuf_t *nbuf)
{
	void *n_ptr = nbuf_dataptr(nbuf);

	if (!npf_iscached(npc, NPC_IP46) && !npf_fetch_ip(npc, nbuf, n_ptr)) {
		return npc->npc_info;
	}
	if (npf_iscached(npc, NPC_IPFRAG)) {
		return npc->npc_info;
	}
	switch (npf_cache_ipproto(npc)) {
	case IPPROTO_TCP:
		(void)npf_fetch_tcp(npc, nbuf, n_ptr);
		break;
	case IPPROTO_UDP:
		(void)npf_fetch_udp(npc, nbuf, n_ptr);
		break;
	case IPPROTO_ICMP:
		(void)npf_fetch_icmp(npc, nbuf, n_ptr);
		break;
	}
	return npc->npc_info;
}

/*
 * npf_rwrip: rewrite required IP address, update the cache.
 */
bool
npf_rwrip(npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr, const int di,
    npf_addr_t *addr)
{
	npf_addr_t *oaddr;
	u_int offby;

	KASSERT(npf_iscached(npc, NPC_IP46));

	if (di == PFIL_OUT) {
		/* Rewrite source address, if outgoing. */
		offby = offsetof(struct ip, ip_src);
		oaddr = npc->npc_srcip;
	} else {
		/* Rewrite destination, if incoming. */
		offby = offsetof(struct ip, ip_dst);
		oaddr = npc->npc_dstip;
	}

	/* Advance to the address and rewrite it. */
	if (nbuf_advstore(&nbuf, &n_ptr, offby, npc->npc_ipsz, addr))
		return false;

	/* Cache: IP address. */
	memcpy(oaddr, addr, npc->npc_ipsz);
	return true;
}

/*
 * npf_rwrport: rewrite required TCP/UDP port, update the cache.
 */
bool
npf_rwrport(npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr, const int di,
    in_port_t port)
{
	const int proto = npf_cache_ipproto(npc);
	u_int offby = npf_cache_hlen(npc);
	in_port_t *oport;

	KASSERT(npf_iscached(npc, NPC_TCP) || npf_iscached(npc, NPC_UDP));
	KASSERT(proto == IPPROTO_TCP || proto == IPPROTO_UDP);

	/* Offset to the port and pointer in the cache. */
	if (proto == IPPROTO_TCP) {
		struct tcphdr *th = &npc->npc_l4.tcp;
		if (di == PFIL_OUT) {
			CTASSERT(offsetof(struct tcphdr, th_sport) == 0);
			oport = &th->th_sport;
		} else {
			offby += offsetof(struct tcphdr, th_dport);
			oport = &th->th_dport;
		}
	} else {
		struct udphdr *uh = &npc->npc_l4.udp;
		if (di == PFIL_OUT) {
			CTASSERT(offsetof(struct udphdr, uh_sport) == 0);
			oport = &uh->uh_sport;
		} else {
			offby += offsetof(struct udphdr, uh_dport);
			oport = &uh->uh_dport;
		}
	}

	/* Advance and rewrite the port. */
	if (nbuf_advstore(&nbuf, &n_ptr, offby, sizeof(in_port_t), &port))
		return false;

	/* Cache: TCP/UDP port. */
	*oport = port;
	return true;
}

/*
 * npf_rwrcksum: rewrite IPv4 and/or TCP/UDP checksum, update the cache.
 */
bool
npf_rwrcksum(npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr, const int di,
    npf_addr_t *addr, in_port_t port)
{
	const int proto = npf_cache_ipproto(npc);
	npf_addr_t *oaddr;
	in_port_t *oport;
	uint16_t *cksum;
	u_int offby;

	/* Checksum update for IPv4 header. */
	if (npf_iscached(npc, NPC_IP4)) {
		struct ip *ip = &npc->npc_ip.v4;
		uint16_t ipsum;

		oaddr = (di == PFIL_OUT) ? npc->npc_srcip : npc->npc_dstip;
		ipsum = npf_addr_cksum(ip->ip_sum, npc->npc_ipsz, oaddr, addr);

		/* Advance to the IPv4 checksum and rewrite it. */
		offby = offsetof(struct ip, ip_sum);
		if (nbuf_advstore(&nbuf, &n_ptr, offby, sizeof(ipsum), &ipsum))
			return false;

		ip->ip_sum = ipsum;
		offby = npf_cache_hlen(npc) - offby;
	} else {
		/* No checksum for IPv6. */
		KASSERT(npf_iscached(npc, NPC_IP6));
		oaddr = NULL;
		offby = 0;
		return false;	/* XXX: Not yet supported. */
	}

	/* Determine whether TCP/UDP checksum update is needed. */
	if (proto == IPPROTO_ICMP || port == 0) {
		return true;
	}
	KASSERT(npf_iscached(npc, NPC_TCP) || npf_iscached(npc, NPC_UDP));

	/* Calculate TCP/UDP checksum. */
	if (proto == IPPROTO_TCP) {
		struct tcphdr *th = &npc->npc_l4.tcp;

		cksum = &th->th_sum;
		offby += offsetof(struct tcphdr, th_sum);
		oport = (di == PFIL_OUT) ? &th->th_sport : &th->th_dport;
	} else {
		struct udphdr *uh = &npc->npc_l4.udp;

		KASSERT(proto == IPPROTO_UDP);
		cksum = &uh->uh_sum;
		if (*cksum == 0) {
			/* No need to update. */
			return true;
		}
		offby += offsetof(struct udphdr, uh_sum);
		oport = (di == PFIL_OUT) ? &uh->uh_sport : &uh->uh_dport;
	}
	*cksum = npf_addr_cksum(*cksum, npc->npc_ipsz, oaddr, addr);
	*cksum = npf_fixup16_cksum(*cksum, *oport, port);

	/* Advance to TCP/UDP checksum and rewrite it. */
	if (nbuf_advstore(&nbuf, &n_ptr, offby, sizeof(uint16_t), cksum)) {
		return false;
	}
	return true;
}

static inline bool
npf_normalize_ip4(npf_cache_t *npc, nbuf_t *nbuf,
    bool rnd, bool no_df, int minttl)
{
	void *n_ptr = nbuf_dataptr(nbuf);
	struct ip *ip = &npc->npc_ip.v4;
	uint16_t cksum = ip->ip_sum;
	uint16_t ip_off = ip->ip_off;
	uint8_t ttl = ip->ip_ttl;
	u_int offby = 0;

	KASSERT(rnd || minttl || no_df);

	/* Randomize IPv4 ID. */
	if (rnd) {
		uint16_t oid = ip->ip_id, nid;

		nid = htons(ip_randomid(ip_ids, 0));
		offby = offsetof(struct ip, ip_id);
		if (nbuf_advstore(&nbuf, &n_ptr, offby, sizeof(nid), &nid)) {
			return false;
		}
		cksum = npf_fixup16_cksum(cksum, oid, nid);
		ip->ip_id = nid;
	}

	/* IP_DF flag cleansing. */
	if (no_df && (ip_off & htons(IP_DF)) != 0) {
		uint16_t nip_off = ip_off & ~htons(IP_DF);

		if (nbuf_advstore(&nbuf, &n_ptr,
		    offsetof(struct ip, ip_off) - offby,
		    sizeof(uint16_t), &nip_off)) {
			return false;
		}
		cksum = npf_fixup16_cksum(cksum, ip_off, nip_off);
		ip->ip_off = nip_off;
		offby = offsetof(struct ip, ip_off);
	}

	/* Enforce minimum TTL. */
	if (minttl && ttl < minttl) {
		if (nbuf_advstore(&nbuf, &n_ptr,
		    offsetof(struct ip, ip_ttl) - offby,
		    sizeof(uint8_t), &minttl)) {
			return false;
		}
		cksum = npf_fixup16_cksum(cksum, ttl, minttl);
		ip->ip_ttl = minttl;
		offby = offsetof(struct ip, ip_ttl);
	}

	/* Update IP checksum. */
	offby = offsetof(struct ip, ip_sum) - offby;
	if (nbuf_advstore(&nbuf, &n_ptr, offby, sizeof(cksum), &cksum)) {
		return false;
	}
	ip->ip_sum = cksum;
	return true;
}

bool
npf_normalize(npf_cache_t *npc, nbuf_t *nbuf,
    bool no_df, bool rnd, u_int minttl, u_int maxmss)
{
	void *n_ptr = nbuf_dataptr(nbuf);
	struct tcphdr *th = &npc->npc_l4.tcp;
	uint16_t cksum, mss;
	u_int offby;
	int wscale;

	/* Normalize IPv4. */
	if (npf_iscached(npc, NPC_IP4) && (rnd || minttl)) {
		if (!npf_normalize_ip4(npc, nbuf, rnd, no_df, minttl)) {
			return false;
		}
	} else if (!npf_iscached(npc, NPC_IP4)) {
		/* XXX: no IPv6 */
		return false;
	}

	/*
	 * TCP Maximum Segment Size (MSS) "clamping".  Only if SYN packet.
	 * Fetch MSS and check whether rewrite to lower is needed.
	 */
	if (maxmss == 0 || !npf_iscached(npc, NPC_TCP) ||
	    (th->th_flags & TH_SYN) == 0) {
		/* Not required; done. */
		return true;
	}
	mss = 0;
	if (!npf_fetch_tcpopts(npc, nbuf, &mss, &wscale)) {
		return false;
	}
	if (ntohs(mss) <= maxmss) {
		return true;
	}

	/* Calculate TCP checksum, then rewrite MSS and the checksum. */
	maxmss = htons(maxmss);
	cksum = npf_fixup16_cksum(th->th_sum, mss, maxmss);
	th->th_sum = cksum;
	mss = maxmss;
	if (!npf_fetch_tcpopts(npc, nbuf, &mss, &wscale)) {
		return false;
	}
	offby = npf_cache_hlen(npc) + offsetof(struct tcphdr, th_sum);
	if (nbuf_advstore(&nbuf, &n_ptr, offby, sizeof(cksum), &cksum)) {
		return false;
	}
	return true;
}
