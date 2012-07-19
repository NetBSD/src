/*	$NetBSD: npf_alg_icmp.c,v 1.11 2012/07/19 21:52:29 spz Exp $	*/

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
 * NPF ALG for ICMP and traceroute translations.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_alg_icmp.c,v 1.11 2012/07/19 21:52:29 spz Exp $");

#include <sys/param.h>
#include <sys/module.h>
#include <sys/pool.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <net/pfil.h>

#include "npf_impl.h"

MODULE(MODULE_CLASS_MISC, npf_alg_icmp, "npf");

/*
 * Traceroute criteria.
 *
 * IANA assigned base port: 33434.  However, common practice is to increase
 * the port, thus monitor [33434-33484] range.  Additional filter is TTL < 50.
 */

#define	TR_BASE_PORT	33434
#define	TR_PORT_RANGE	33484
#define	TR_MAX_TTL	50

static npf_alg_t *	alg_icmp	__read_mostly;

static bool		npfa_icmp_match(npf_cache_t *, nbuf_t *, void *);
static bool		npfa_icmp_natin(npf_cache_t *, nbuf_t *, void *);
static bool		npfa_icmp_session(npf_cache_t *, nbuf_t *, void *);

/*
 * npf_alg_icmp_{init,fini,modcmd}: ICMP ALG initialization, destruction
 * and module interface.
 */

static int
npf_alg_icmp_init(void)
{

	alg_icmp = npf_alg_register(npfa_icmp_match, NULL,
	    npfa_icmp_natin, npfa_icmp_session);
	KASSERT(alg_icmp != NULL);
	return 0;
}

static int
npf_alg_icmp_fini(void)
{

	KASSERT(alg_icmp != NULL);
	return npf_alg_unregister(alg_icmp);
}

static int
npf_alg_icmp_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return npf_alg_icmp_init();
	case MODULE_CMD_FINI:
		return npf_alg_icmp_fini();
	case MODULE_CMD_AUTOUNLOAD:
		return EBUSY;
	default:
		return ENOTTY;
	}
	return 0;
}

/*
 * npfa_icmp_match: ALG matching inspector - determines ALG case and
 * associates ALG with NAT entry.
 */
static bool
npfa_icmp_match(npf_cache_t *npc, nbuf_t *nbuf, void *ntptr)
{
	const int proto = npf_cache_ipproto(npc);
	struct ip *ip = &npc->npc_ip.v4;
	in_port_t dport;

	KASSERT(npf_iscached(npc, NPC_IP46));
	KASSERT(npf_iscached(npc, NPC_LAYER4));

	/* Check for low TTL. */
	if (ip->ip_ttl > TR_MAX_TTL) {
		return false;
	}

	if (proto == IPPROTO_TCP) {
		struct tcphdr *th = &npc->npc_l4.tcp;
		dport = ntohs(th->th_dport);
	} else if (proto == IPPROTO_UDP) {
		struct udphdr *uh = &npc->npc_l4.udp;
		dport = ntohs(uh->uh_dport);
	} else {
		return false;
	}

	/* Handle TCP/UDP traceroute - check for port range. */
	if (dport < TR_BASE_PORT || dport > TR_PORT_RANGE) {
		return false;
	}

	/* Associate ALG with translation entry. */
	npf_nat_t *nt = ntptr;
	npf_nat_setalg(nt, alg_icmp, 0);
	return true;
}

/*
 * npf_icmp_uniqid: retrieve unique identifiers - either ICMP query ID
 * or TCP/UDP ports of the original packet, which is embedded.
 */
static bool
npf_icmp_uniqid(const int type, npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr)
{
	struct icmp      *ic;
	struct icmp6_hdr *ic6;
	u_int            offby;

	if (npf_iscached(npc, NPC_IP4)) {
		/* Per RFC 792. */
		switch (type) {
		case ICMP_UNREACH:
		case ICMP_SOURCEQUENCH:
		case ICMP_REDIRECT:
		case ICMP_TIMXCEED:
		case ICMP_PARAMPROB:
			/* Should contain original IP header. */
			offby = offsetof(struct icmp, icmp_ip);
			if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL) {
				return false;
			}
			/* Fetch into the cache. */
			if (!npf_fetch_ip(npc, nbuf, n_ptr)) {
				return false;
			}
			switch (npf_cache_ipproto(npc)) {
			case IPPROTO_TCP:
				return npf_fetch_tcp(npc, nbuf, n_ptr);
			case IPPROTO_UDP:
				return npf_fetch_udp(npc, nbuf, n_ptr);
			default:
				return false;
			}
			return true;

		case ICMP_ECHOREPLY:
		case ICMP_ECHO:
		case ICMP_TSTAMP:
		case ICMP_TSTAMPREPLY:
		case ICMP_IREQ:
		case ICMP_IREQREPLY:
			/* Should contain ICMP query ID. */
			ic = &npc->npc_l4.icmp;
			offby = offsetof(struct icmp, icmp_id);
			if (nbuf_advfetch(&nbuf, &n_ptr, offby,
			    sizeof(uint16_t), &ic->icmp_id)) {
				return false;
			}
			npc->npc_info |= NPC_ICMP_ID;
			return true;
		default:
			break;
		}
		/* No unique IDs. */
		return false;
	}
	if (npf_iscached(npc, NPC_IP6)) {
		switch (type) {
		/* Per RFC 4443. */
		case ICMP6_DST_UNREACH:
		case ICMP6_PACKET_TOO_BIG:
		case ICMP6_TIME_EXCEEDED:
		case ICMP6_PARAM_PROB:
			/* Should contain original IP header. */
			offby = sizeof(struct icmp6_hdr);
			if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL) {
				return false;
			}
			/* Fetch into the cache. */
			if (!npf_fetch_ip(npc, nbuf, n_ptr)) {
				return false;
			}
			switch (npf_cache_ipproto(npc)) {
			case IPPROTO_TCP:
				return npf_fetch_tcp(npc, nbuf, n_ptr);
			case IPPROTO_UDP:
				return npf_fetch_udp(npc, nbuf, n_ptr);
			default:
				return false;
			}
			return true;

		case ICMP6_ECHO_REQUEST:
		case ICMP6_ECHO_REPLY:
			/* Should contain ICMP query ID. */
			ic6 = &npc->npc_l4.icmp6;
			offby = offsetof(struct icmp6_hdr, icmp6_id);
			if (nbuf_advfetch(&nbuf, &n_ptr, offby,
			    sizeof(uint16_t), &ic6->icmp6_id)) {
				return false;
			}
			npc->npc_info |= NPC_ICMP_ID;
			return true;
		default:
			break;
		}
		/* No unique IDs. */
		return false;
	}
	/* Whatever protocol that may have been ... */
	return false;
}

static void
npfa_srcdst_invert(npf_cache_t *npc)
{
	const int proto = npf_cache_ipproto(npc);
	npf_addr_t *tmp_ip;

	if (proto == IPPROTO_TCP) {
		struct tcphdr *th = &npc->npc_l4.tcp;
		in_port_t tmp_sport = th->th_sport;
		th->th_sport = th->th_dport;
		th->th_dport = tmp_sport;

	} else if (proto == IPPROTO_UDP) {
		struct udphdr *uh = &npc->npc_l4.udp;
		in_port_t tmp_sport = uh->uh_sport;
		uh->uh_sport = uh->uh_dport;
		uh->uh_dport = tmp_sport;
	}
	tmp_ip = npc->npc_srcip;
	npc->npc_srcip = npc->npc_dstip;
	npc->npc_dstip = tmp_ip;
}

/*
 * npfa_icmp_session: ALG session inspector, returns unique identifiers.
 */
static bool
npfa_icmp_session(npf_cache_t *npc, nbuf_t *nbuf, void *keyptr)
{
	npf_cache_t *key = keyptr;
	KASSERT(key->npc_info == 0);

	/* IP + ICMP?  Get unique identifiers from ICMP packet. */
	if (!npf_iscached(npc, NPC_IP4)) {
		return false;
	}
	if (npf_cache_ipproto(npc) != IPPROTO_ICMP) {
		return false;
	}
	KASSERT(npf_iscached(npc, NPC_ICMP));

	/* Advance to ICMP header. */
	void *n_ptr = nbuf_dataptr(nbuf);
	const u_int hlen = npf_cache_hlen(npc);

	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, hlen)) == NULL) {
		return false;
	}

	/* Fetch relevant data into the separate ("key") cache. */
	struct icmp *ic = &npc->npc_l4.icmp;
	if (!npf_icmp_uniqid(ic->icmp_type, key, nbuf, n_ptr)) {
		return false;
	}

	if (npf_iscached(key, NPC_ICMP_ID)) {
		struct icmp *keyic = &key->npc_l4.icmp;

		/* Copy ICMP ID to the cache and flag it. */
		npc->npc_info |= NPC_ICMP_ID;
		ic->icmp_id = keyic->icmp_id;

		/* Note: return False, since key is the original cache. */
		return false;
	}

	/*
	 * Embedded IP packet is the original of "forwards" stream.
	 * We should imitate the "backwards" stream for inspection.
	 */
	KASSERT(npf_iscached(key, NPC_IP46));
	KASSERT(npf_iscached(key, NPC_LAYER4));
	npfa_srcdst_invert(key);
	key->npc_alen = npc->npc_alen;

	return true;
}

/*
 * npfa_icmp_natin: ALG inbound translation inspector, rewrite IP address
 * in the IP header, which is embedded in ICMP packet.
 */
static bool
npfa_icmp_natin(npf_cache_t *npc, nbuf_t *nbuf, void *ntptr)
{
	npf_cache_t enpc = { .npc_info = 0 };

	/* XXX: Duplicated work (done at session inspection). */
	if (!npfa_icmp_session(npc, nbuf, &enpc)) {
		return false;
	}
	/* XXX: Restore inversion (inefficient). */
	KASSERT(npf_iscached(&enpc, NPC_IP46));
	KASSERT(npf_iscached(&enpc, NPC_LAYER4));
	npfa_srcdst_invert(&enpc);

	/*
	 * Save ICMP and embedded IP with TCP/UDP header checksums, retrieve
	 * the original address and port, and calculate ICMP checksum for
	 * embedded packet changes, while data is not rewritten in the cache.
	 */
	const int proto = npf_cache_ipproto(&enpc);
	const struct ip *eip = &enpc.npc_ip.v4;
	const struct icmp * const ic = &npc->npc_l4.icmp;
	uint16_t cksum = ic->icmp_cksum, ecksum = eip->ip_sum, l4cksum;
	npf_nat_t *nt = ntptr;
	npf_addr_t *addr;
	in_port_t port;

	npf_nat_getorig(nt, &addr, &port);

	if (proto == IPPROTO_TCP) {
		struct tcphdr *th = &enpc.npc_l4.tcp;
		cksum = npf_fixup16_cksum(cksum, th->th_sport, port);
		l4cksum = th->th_sum;
	} else {
		struct udphdr *uh = &enpc.npc_l4.udp;
		cksum = npf_fixup16_cksum(cksum, uh->uh_sport, port);
		l4cksum = uh->uh_sum;
	}
	cksum = npf_addr_cksum(cksum, enpc.npc_alen, enpc.npc_srcip, addr);

	/*
	 * Save the original pointers to the main IP header and then advance
	 * to the embedded IP header after ICMP header.
	 */
	void *n_ptr = nbuf_dataptr(nbuf), *cnbuf = nbuf, *cnptr = n_ptr;
	u_int offby = npf_cache_hlen(npc) + offsetof(struct icmp, icmp_ip);

	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL) {
		return false;
	}

	/*
	 * Rewrite source IP address and port of the embedded IP header,
	 * which represents original packet - therefore passing PFIL_OUT.
	 * Note: checksums are first, since it uses values from the cache.
	 */
	if (!npf_rwrcksum(&enpc, nbuf, n_ptr, PFIL_OUT, addr, port)) {
		return false;
	}
	if (!npf_rwrip(&enpc, nbuf, n_ptr, PFIL_OUT, addr)) {
		return false;
	}
	if (!npf_rwrport(&enpc, nbuf, n_ptr, PFIL_OUT, port)) {
		return false;
	}

	/*
	 * Finish calculation of the ICMP checksum.  Update for embedded IP
	 * and TCP/UDP checksum changes.  Finally, rewrite ICMP checksum.
	 */
	if (proto == IPPROTO_TCP) {
		struct tcphdr *th = &enpc.npc_l4.tcp;
		cksum = npf_fixup16_cksum(cksum, l4cksum, th->th_sum);
	} else if (l4cksum) {
		struct udphdr *uh = &enpc.npc_l4.udp;
		cksum = npf_fixup16_cksum(cksum, l4cksum, uh->uh_sum);
	}
	cksum = npf_fixup16_cksum(cksum, ecksum, eip->ip_sum);

	offby = npf_cache_hlen(npc) + offsetof(struct icmp, icmp_cksum);
	if (nbuf_advstore(&cnbuf, &cnptr, offby, sizeof(uint16_t), &cksum)) {
		return false;
	}
	return true;
}
