/*	$NetBSD: npf_inet.c,v 1.2 2010/09/16 04:53:27 rmind Exp $	*/

/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
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
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_inet.c,v 1.2 2010/09/16 04:53:27 rmind Exp $");

#include <sys/param.h>
#include <sys/kernel.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

#include <net/if.h>
#include <net/ethertypes.h>
#include <net/if_ether.h>
#endif
#include <net/pfil.h>

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
 * npf_ip4_proto: check IPv4 header length and match protocol number.
 *
 * => Returns pointer to protocol header or NULL on failure.
 * => Stores protocol number in the cache.
 * => Updates nbuf pointer to header's nbuf.
 */
bool
npf_ip4_proto(npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr)
{
	u_int hlen, offby;
	uint8_t val8;
	int error;

	/* IPv4 header: check IP version and header length. */
	error = nbuf_fetch_datum(nbuf, n_ptr, sizeof(uint8_t), &val8);
	if (error || (val8 >> 4) != IPVERSION)
		return false;
	hlen = (val8 & 0xf) << 2;
	if (hlen < sizeof(struct ip))
		return false;
	offby = offsetof(struct ip, ip_off);
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL)
		return false;

	/* IPv4 header: check fragment offset. */
	error = nbuf_fetch_datum(nbuf, n_ptr, sizeof(uint8_t), &val8);
	if (error || (val8 & ~htons(IP_DF | IP_RF)))
		return false;

	/* Get and match protocol. */
	KASSERT(offsetof(struct ip, ip_p) > offby);
	offby = offsetof(struct ip, ip_p) - offby;
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL)
		return false;
	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(uint8_t), &val8))
		return false;

	/* IP checksum. */
	offby = offsetof(struct ip, ip_sum) - offsetof(struct ip, ip_p);
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL)
		return false;
	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(uint16_t), &npc->npc_ipsum))
		return false;

	/* Cache: IPv4, protocol, header length. */
	npc->npc_info |= NPC_IP46;
	npc->npc_proto = val8;
	npc->npc_hlen = hlen;
	return true;
}

/*
 * npf_fetch_ip4addrs: fetch source and destination address from IPv4 header.
 *
 * => Stores both source and destination addresses into the cache.
 */
bool
npf_fetch_ip4addrs(npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr)
{
	u_int offby;

	/* Source address. */
	offby = offsetof(struct ip, ip_src);
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL)
		return false;
	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(in_addr_t), &npc->npc_srcip))
		return false;

	/* Destination address. */
	offby = offsetof(struct ip, ip_dst) - offby;
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL)
		return false;
	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(in_addr_t), &npc->npc_dstip))
		return false;

	/* Both addresses are cached. */
	npc->npc_info |= NPC_ADDRS;
	return true;
}

/*
 * npf_fetch_ports: fetch ports from either TCP or UDP header.
 *
 * => Stores both source and destination ports into the cache.
 */
bool
npf_fetch_ports(npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr, const int proto)
{
	u_int dst_off;

	/* Perform checks, advance to TCP/UDP header. */
	if (!npf_iscached(npc, NPC_IP46) && !npf_ip4_proto(npc, nbuf, n_ptr))
		return false;
	n_ptr = nbuf_advance(&nbuf, n_ptr, npc->npc_hlen);
	if (n_ptr == NULL || npc->npc_proto != proto)
		return false;

	/*
	 * TCP/UDP header: fetch source and destination ports.  For both
	 * protocols offset of the source port offset is 0.
	 */
	CTASSERT(offsetof(struct tcphdr, th_sport) == 0);
	CTASSERT(offsetof(struct udphdr, uh_sport) == 0);
	if (proto == IPPROTO_TCP) {
		dst_off = offsetof(struct tcphdr, th_dport);
	} else {
		KASSERT(proto == IPPROTO_UDP);
		dst_off = offsetof(struct udphdr, uh_dport);
	}

	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(in_port_t), &npc->npc_sport))
		return false;
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, dst_off)) == NULL)
		return false;
	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(in_port_t), &npc->npc_dport))
		return false;

	/* Both ports are cached. */
	npc->npc_info |= NPC_PORTS;
	return true;
}

/*
 * npf_fetch_icmp: fetch ICMP code, type and possible query ID.
 *
 * => Stores both all fetched items into the cache.
 */
bool
npf_fetch_icmp(npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr)
{
	u_int offby;
	uint8_t type;

	KASSERT(npf_iscached(npc, NPC_IP46));

	/* ICMP type. */
	offby = npc->npc_hlen;
	CTASSERT(offsetof(struct icmp, icmp_type) == 0);
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL)
		return false;
	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(uint8_t), &type))
		return false;

	/* ICMP code. */
	offby = offsetof(struct icmp, icmp_code);
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL)
		return false;
	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(uint8_t), &npc->npc_icmp_code))
		return false;

	/* Mark as cached. */
	npc->npc_icmp_type = type;
	npc->npc_info |= NPC_ICMP;
	return true;
}

/*
 * npf_fetch_tcpfl: fetch TCP flags and store into the cache.
 */
bool
npf_fetch_tcpfl(npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr)
{
	u_int offby;

	/* Get TCP flags. */
	offby = npc->npc_hlen + offsetof(struct tcphdr, th_flags);
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL)
		return false;
	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(uint8_t), &npc->npc_tcp_flags))
		return false;
	return true;
}

/*
 * npf_cache_all: general routine to cache all relevant IPv4 and
 * TCP, UDP or ICMP data.
 */
bool
npf_cache_all(npf_cache_t *npc, nbuf_t *nbuf)
{
	void *n_ptr = nbuf_dataptr(nbuf);

	/* IPv4: get protocol, source and destination addresses. */
	if (!npf_iscached(npc, NPC_IP46) && !npf_ip4_proto(npc, nbuf, n_ptr)) {
		return false;
	}
	if (!npf_iscached(npc, NPC_ADDRS) &&
	    !npf_fetch_ip4addrs(npc, nbuf, n_ptr)) {
		return false;
	}
	switch (npc->npc_proto) {
	case IPPROTO_TCP:
		/* TCP flags. */
		if (!npf_fetch_tcpfl(npc, nbuf, n_ptr)) {
			return false;
		}
		/* FALLTHROUGH */

	case IPPROTO_UDP:
		/* Fetch TCP/UDP ports. */
		return npf_fetch_ports(npc, nbuf, n_ptr, npc->npc_proto);

	case IPPROTO_ICMP:
		/* Fetch ICMP data. */
		return npf_fetch_icmp(npc, nbuf, n_ptr);
	}
	return false;
}

/*
 * npf_rwrport: rewrite required TCP/UDP port and update checksum.
 */
bool
npf_rwrport(npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr, const int di,
    in_port_t port, in_addr_t naddr)
{
	const int proto = npc->npc_proto;
	u_int offby, toff;
	in_addr_t oaddr;
	in_port_t oport;
	uint16_t cksum;

	KASSERT(npf_iscached(npc, NPC_PORTS));
	KASSERT(proto == IPPROTO_TCP || proto == IPPROTO_UDP);

	offby = npc->npc_hlen;

	if (di == PFIL_OUT) {
		/* Offset to the source port is zero. */
		CTASSERT(offsetof(struct tcphdr, th_sport) == 0);
		CTASSERT(offsetof(struct udphdr, uh_sport) == 0);
		if (proto == IPPROTO_TCP) {
			toff = offsetof(struct tcphdr, th_sum);
		} else {
			toff = offsetof(struct udphdr, uh_sum);
		}
		oaddr = npc->npc_srcip;
		oport = npc->npc_sport;
	} else {
		/* Calculate offset to destination port and checksum. */
		u_int poff;
		if (proto == IPPROTO_TCP) {
			poff = offsetof(struct tcphdr, th_dport);
			toff = offsetof(struct tcphdr, th_sum) - poff;
		} else {
			poff = offsetof(struct udphdr, uh_dport);
			toff = offsetof(struct udphdr, uh_sum) - poff;
		}
		oaddr = npc->npc_dstip;
		oport = npc->npc_dport;
		offby += poff;
	}

	/* Advance and rewrite port. */
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL)
		return false;
	if (nbuf_store_datum(nbuf, n_ptr, sizeof(in_port_t), &port))
		return false;

	/* Advance and update TCP/UDP checksum. */
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, toff)) == NULL)
		return false;
	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(uint16_t), &cksum))
		return false;
	if (__predict_true(cksum || proto == IPPROTO_TCP)) {
		cksum = npf_fixup32_cksum(cksum, oaddr, naddr);
		cksum = npf_fixup16_cksum(cksum, oport, port);
		if (nbuf_store_datum(nbuf, n_ptr, sizeof(uint16_t), &cksum))
			return false;
	}
	return true;
}

/*
 * npf_rwrip: rewrite required IP address and update checksum.
 */
bool
npf_rwrip(npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr, const int di,
    in_addr_t addr)
{
	u_int offby;
	in_addr_t oaddr;

	KASSERT(npf_iscached(npc, NPC_IP46 | NPC_ADDRS));

	/* Advance to the checksum in IP header and fetch it. */
	offby = offsetof(struct ip, ip_sum);
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL)
		return false;

	if (di == PFIL_OUT) {
		/* Rewrite source address, if outgoing. */
		offby = offsetof(struct ip, ip_src) - offby;
		oaddr = npc->npc_srcip;
	} else {
		/* Rewrite destination, if incoming. */
		offby = offsetof(struct ip, ip_dst) - offby;
		oaddr = npc->npc_dstip;
	}

	/* Write new IP checksum (it is acceptable to do this earlier). */
	uint16_t cksum = npf_fixup32_cksum(npc->npc_ipsum, oaddr, addr);
	if (nbuf_store_datum(nbuf, n_ptr, sizeof(uint16_t), &cksum))
		return false;

	/* Advance to address and rewrite it. */
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL)
		return false;
	if (nbuf_store_datum(nbuf, n_ptr, sizeof(in_addr_t), &addr))
		return false;

	npc->npc_ipsum = cksum;
	return true;
}
