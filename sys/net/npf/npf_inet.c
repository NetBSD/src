/*	$NetBSD: npf_inet.c,v 1.6.8.5 2013/01/23 00:06:25 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: npf_inet.c,v 1.6.8.5 2013/01/23 00:06:25 yamt Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <net/pfil.h>
#include <net/if.h>
#include <net/ethertypes.h>
#include <net/if_ether.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
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
npf_addr_cksum(uint16_t cksum, int sz, const npf_addr_t *oaddr,
    const npf_addr_t *naddr)
{
	const uint32_t *oip32 = (const uint32_t *)oaddr;
	const uint32_t *nip32 = (const uint32_t *)naddr;

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

/*
 * npf_addr_mask: apply the mask to a given address and store the result.
 */
void
npf_addr_mask(const npf_addr_t *addr, const npf_netmask_t mask,
    const int alen, npf_addr_t *out)
{
	const int nwords = alen >> 2;
	uint_fast8_t length = mask;

	/* Note: maximum length is 32 for IPv4 and 128 for IPv6. */
	KASSERT(length <= NPF_MAX_NETMASK);

	for (int i = 0; i < nwords; i++) {
		uint32_t wordmask;

		if (length >= 32) {
			wordmask = htonl(0xffffffff);
			length -= 32;
		} else if (length) {
			wordmask = htonl(0xffffffff << (32 - length));
			length = 0;
		} else {
			wordmask = 0;
		}
		out->s6_addr32[i] = addr->s6_addr32[i] & wordmask;
	}
}

/*
 * npf_addr_cmp: compare two addresses, either IPv4 or IPv6.
 *
 * => Return 0 if equal and negative/positive if less/greater accordingly.
 * => Ignore the mask, if NPF_NO_NETMASK is specified.
 */
int
npf_addr_cmp(const npf_addr_t *addr1, const npf_netmask_t mask1,
    const npf_addr_t *addr2, const npf_netmask_t mask2, const int alen)
{
	npf_addr_t realaddr1, realaddr2;

	if (mask1 != NPF_NO_NETMASK) {
		npf_addr_mask(addr1, mask1, alen, &realaddr1);
		addr1 = &realaddr1;
	}
	if (mask2 != NPF_NO_NETMASK) {
		npf_addr_mask(addr2, mask2, alen, &realaddr2);
		addr2 = &realaddr2;
	}
	return memcmp(addr1, addr2, alen);
}

/*
 * npf_tcpsaw: helper to fetch SEQ, ACK, WIN and return TCP data length.
 *
 * => Returns all values in host byte-order.
 */
int
npf_tcpsaw(const npf_cache_t *npc, tcp_seq *seq, tcp_seq *ack, uint32_t *win)
{
	const struct tcphdr *th = npc->npc_l4.tcp;
	u_int thlen;

	KASSERT(npf_iscached(npc, NPC_TCP));

	*seq = ntohl(th->th_seq);
	*ack = ntohl(th->th_ack);
	*win = (uint32_t)ntohs(th->th_win);
	thlen = th->th_off << 2;

	if (npf_iscached(npc, NPC_IP4)) {
		const struct ip *ip = npc->npc_ip.v4;
		return ntohs(ip->ip_len) - npf_cache_hlen(npc) - thlen;
	} else if (npf_iscached(npc, NPC_IP6)) {
		const struct ip6_hdr *ip6 = npc->npc_ip.v6;
		return ntohs(ip6->ip6_plen) - thlen;
	}
	return 0;
}

/*
 * npf_fetch_tcpopts: parse and return TCP options.
 */
bool
npf_fetch_tcpopts(npf_cache_t *npc, nbuf_t *nbuf, uint16_t *mss, int *wscale)
{
	const struct tcphdr *th = npc->npc_l4.tcp;
	int topts_len, step;
	void *nptr;
	uint8_t val;
	bool ok;

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
	nbuf_reset(nbuf);
next:
	if ((nptr = nbuf_advance(nbuf, step, 1)) == NULL) {
		ok = false;
		goto done;
	}
	val = *(uint8_t *)nptr;

	switch (val) {
	case TCPOPT_EOL:
		/* Done. */
		ok = true;
		goto done;
	case TCPOPT_NOP:
		topts_len--;
		step = 1;
		break;
	case TCPOPT_MAXSEG:
		if ((nptr = nbuf_advance(nbuf, 2, 2)) == NULL) {
			ok = false;
			goto done;
		}
		if (mss) {
			if (*mss) {
				memcpy(nptr, mss, sizeof(uint16_t));
			} else {
				memcpy(mss, nptr, sizeof(uint16_t));
			}
		}
		topts_len -= TCPOLEN_MAXSEG;
		step = 2;
		break;
	case TCPOPT_WINDOW:
		/* TCP Window Scaling (RFC 1323). */
		if ((nptr = nbuf_advance(nbuf, 2, 1)) == NULL) {
			ok = false;
			goto done;
		}
		val = *(uint8_t *)nptr;
		*wscale = (val > TCP_MAX_WINSHIFT) ? TCP_MAX_WINSHIFT : val;
		topts_len -= TCPOLEN_WINDOW;
		step = 1;
		break;
	default:
		if ((nptr = nbuf_advance(nbuf, 1, 1)) == NULL) {
			ok = false;
			goto done;
		}
		val = *(uint8_t *)nptr;
		if (val < 2 || val > topts_len) {
			ok = false;
			goto done;
		}
		topts_len -= val;
		step = val - 1;
	}

	/* Any options left? */
	if (__predict_true(topts_len > 0)) {
		goto next;
	}
	ok = true;
done:
	if (nbuf_flag_p(nbuf, NBUF_DATAREF_RESET)) {
		npf_recache(npc, nbuf);
	}
	return ok;
}

static int
npf_cache_ip(npf_cache_t *npc, nbuf_t *nbuf)
{
	const void *nptr = nbuf_dataptr(nbuf);
	const uint8_t ver = *(const uint8_t *)nptr;
	int flags = 0;

	switch (ver >> 4) {
	case IPVERSION: {
		struct ip *ip;

		ip = nbuf_ensure_contig(nbuf, sizeof(struct ip));
		if (ip == NULL) {
			return 0;
		}

		/* Check header length and fragment offset. */
		if ((u_int)(ip->ip_hl << 2) < sizeof(struct ip)) {
			return 0;
		}
		if (ip->ip_off & ~htons(IP_DF | IP_RF)) {
			/* Note fragmentation. */
			flags |= NPC_IPFRAG;
		}

		/* Cache: layer 3 - IPv4. */
		npc->npc_alen = sizeof(struct in_addr);
		npc->npc_srcip = (npf_addr_t *)&ip->ip_src;
		npc->npc_dstip = (npf_addr_t *)&ip->ip_dst;
		npc->npc_hlen = ip->ip_hl << 2;
		npc->npc_proto = ip->ip_p;

		npc->npc_ip.v4 = ip;
		flags |= NPC_IP4;
		break;
	}

	case (IPV6_VERSION >> 4): {
		struct ip6_hdr *ip6;
		struct ip6_ext *ip6e;
		size_t off, hlen;

		ip6 = nbuf_ensure_contig(nbuf, sizeof(struct ip6_hdr));
		if (ip6 == NULL) {
			return 0;
		}

		/* Set initial next-protocol value. */
		hlen = sizeof(struct ip6_hdr);
		npc->npc_proto = ip6->ip6_nxt;
		npc->npc_hlen = hlen;

		/*
		 * Advance by the length of the current header.
		 */
		off = nbuf_offset(nbuf);
		while (nbuf_advance(nbuf, hlen, 0) != NULL) {
			ip6e = nbuf_ensure_contig(nbuf, sizeof(*ip6e));
			if (ip6e == NULL) {
				return 0;
			}

			/*
			 * Determine whether we are going to continue.
			 */
			switch (npc->npc_proto) {
			case IPPROTO_HOPOPTS:
			case IPPROTO_DSTOPTS:
			case IPPROTO_ROUTING:
				hlen = (ip6e->ip6e_len + 1) << 3;
				break;
			case IPPROTO_FRAGMENT:
				hlen = sizeof(struct ip6_frag);
				flags |= NPC_IPFRAG;
				break;
			case IPPROTO_AH:
				hlen = (ip6e->ip6e_len + 2) << 2;
				break;
			default:
				hlen = 0;
				break;
			}

			if (!hlen) {
				break;
			}
			npc->npc_proto = ip6e->ip6e_nxt;
			npc->npc_hlen += hlen;
		}

		/* Restore the offset. */
		nbuf_reset(nbuf);
		if (off) {
			nbuf_advance(nbuf, off, 0);
		}

		/* Cache: layer 3 - IPv6. */
		npc->npc_alen = sizeof(struct in6_addr);
		npc->npc_srcip = (npf_addr_t *)&ip6->ip6_src;
		npc->npc_dstip = (npf_addr_t *)&ip6->ip6_dst;

		npc->npc_ip.v6 = ip6;
		flags |= NPC_IP6;
		break;
	}
	default:
		break;
	}
	return flags;
}

/*
 * npf_cache_all: general routine to cache all relevant IP (v4 or v6)
 * and TCP, UDP or ICMP headers.
 *
 * => nbuf offset shall be set accordingly.
 */
int
npf_cache_all(npf_cache_t *npc, nbuf_t *nbuf)
{
	int flags, l4flags;
	u_int hlen;

	/*
	 * This routine is a main point where the references are cached,
	 * therefore clear the flag as we reset.
	 */
again:
	nbuf_unset_flag(nbuf, NBUF_DATAREF_RESET);

	/*
	 * First, cache the L3 header (IPv4 or IPv6).  If IP packet is
	 * fragmented, then we cannot look into L4.
	 */
	flags = npf_cache_ip(npc, nbuf);
	if ((flags & NPC_IP46) == 0 || (flags & NPC_IPFRAG) != 0) {
		npc->npc_info |= flags;
		return flags;
	}
	hlen = npc->npc_hlen;

	switch (npc->npc_proto) {
	case IPPROTO_TCP:
		/* Cache: layer 4 - TCP. */
		npc->npc_l4.tcp = nbuf_advance(nbuf, hlen,
		    sizeof(struct tcphdr));
		l4flags = NPC_LAYER4 | NPC_TCP;
		break;
	case IPPROTO_UDP:
		/* Cache: layer 4 - UDP. */
		npc->npc_l4.udp = nbuf_advance(nbuf, hlen,
		    sizeof(struct udphdr));
		l4flags = NPC_LAYER4 | NPC_UDP;
		break;
	case IPPROTO_ICMP:
		/* Cache: layer 4 - ICMPv4. */
		npc->npc_l4.icmp = nbuf_advance(nbuf, hlen,
		    offsetof(struct icmp, icmp_void));
		l4flags = NPC_LAYER4 | NPC_ICMP;
		break;
	case IPPROTO_ICMPV6:
		/* Cache: layer 4 - ICMPv6. */
		npc->npc_l4.icmp6 = nbuf_advance(nbuf, hlen,
		    offsetof(struct icmp6_hdr, icmp6_data32));
		l4flags = NPC_LAYER4 | NPC_ICMP;
		break;
	default:
		l4flags = 0;
		break;
	}

	if (nbuf_flag_p(nbuf, NBUF_DATAREF_RESET)) {
		goto again;
	}

	/* Add the L4 flags if nbuf_advance() succeeded. */
	if (l4flags && npc->npc_l4.hdr) {
		flags |= l4flags;
	}
	npc->npc_info |= flags;
	return flags;
}

void
npf_recache(npf_cache_t *npc, nbuf_t *nbuf)
{
	const int mflags __unused = npc->npc_info & (NPC_IP46 | NPC_LAYER4);
	int flags;

	nbuf_reset(nbuf);
	npc->npc_info = 0;
	flags = npf_cache_all(npc, nbuf);
	KASSERT((flags & mflags) == mflags);
	KASSERT(nbuf_flag_p(nbuf, NBUF_DATAREF_RESET) == 0);
}

/*
 * npf_rwrip: rewrite required IP address.
 */
bool
npf_rwrip(const npf_cache_t *npc, int di, const npf_addr_t *addr)
{
	npf_addr_t *oaddr;

	KASSERT(npf_iscached(npc, NPC_IP46));

	/*
	 * Rewrite source address if outgoing and destination if incoming.
	 */
	oaddr = (di == PFIL_OUT) ? npc->npc_srcip : npc->npc_dstip;
	memcpy(oaddr, addr, npc->npc_alen);
	return true;
}

/*
 * npf_rwrport: rewrite required TCP/UDP port.
 */
bool
npf_rwrport(const npf_cache_t *npc, int di, const in_port_t port)
{
	const int proto = npf_cache_ipproto(npc);
	in_port_t *oport;

	KASSERT(npf_iscached(npc, NPC_TCP) || npf_iscached(npc, NPC_UDP));
	KASSERT(proto == IPPROTO_TCP || proto == IPPROTO_UDP);

	/* Get the offset and store the port in it. */
	if (proto == IPPROTO_TCP) {
		struct tcphdr *th = npc->npc_l4.tcp;
		oport = (di == PFIL_OUT) ? &th->th_sport : &th->th_dport;
	} else {
		struct udphdr *uh = npc->npc_l4.udp;
		oport = (di == PFIL_OUT) ? &uh->uh_sport : &uh->uh_dport;
	}
	memcpy(oport, &port, sizeof(in_port_t));
	return true;
}

/*
 * npf_rwrcksum: rewrite IPv4 and/or TCP/UDP checksum.
 */
bool
npf_rwrcksum(const npf_cache_t *npc, const int di,
    const npf_addr_t *addr, const in_port_t port)
{
	const int proto = npf_cache_ipproto(npc);
	const int alen = npc->npc_alen;
	npf_addr_t *oaddr;
	uint16_t *ocksum;
	in_port_t oport;

	KASSERT(npf_iscached(npc, NPC_LAYER4));
	oaddr = (di == PFIL_OUT) ? npc->npc_srcip : npc->npc_dstip;

	if (npf_iscached(npc, NPC_IP4)) {
		struct ip *ip = npc->npc_ip.v4;
		uint16_t ipsum = ip->ip_sum;

		/* Recalculate IPv4 checksum and rewrite. */
		ip->ip_sum = npf_addr_cksum(ipsum, alen, oaddr, addr);
	} else {
		/* No checksum for IPv6. */
		KASSERT(npf_iscached(npc, NPC_IP6));
	}

	/* Nothing else to do for ICMP. */
	if (proto == IPPROTO_ICMP) {
		return true;
	}
	KASSERT(npf_iscached(npc, NPC_TCP) || npf_iscached(npc, NPC_UDP));

	/*
	 * Calculate TCP/UDP checksum:
	 * - Skip if UDP and the current checksum is zero.
	 * - Fixup the IP address change.
	 * - Fixup the port change, if required (non-zero).
	 */
	if (proto == IPPROTO_TCP) {
		struct tcphdr *th = npc->npc_l4.tcp;

		ocksum = &th->th_sum;
		oport = (di == PFIL_OUT) ? th->th_sport : th->th_dport;
	} else {
		struct udphdr *uh = npc->npc_l4.udp;

		KASSERT(proto == IPPROTO_UDP);
		ocksum = &uh->uh_sum;
		if (*ocksum == 0) {
			/* No need to update. */
			return true;
		}
		oport = (di == PFIL_OUT) ? uh->uh_sport : uh->uh_dport;
	}

	uint16_t cksum = npf_addr_cksum(*ocksum, alen, oaddr, addr);
	if (port) {
		cksum = npf_fixup16_cksum(cksum, oport, port);
	}

	/* Rewrite TCP/UDP checksum. */
	memcpy(ocksum, &cksum, sizeof(uint16_t));
	return true;
}

#if defined(DDB) || defined(_NPF_TESTING)

void
npf_addr_dump(const npf_addr_t *addr)
{
	printf("IP[%x:%x:%x:%x]\n",
	    addr->s6_addr32[0], addr->s6_addr32[1],
	    addr->s6_addr32[2], addr->s6_addr32[3]);
}

#endif
