/*	$NetBSD: npf_alg_icmp.c,v 1.2 2010/09/16 04:53:27 rmind Exp $	*/

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

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_alg_icmp.c,v 1.2 2010/09/16 04:53:27 rmind Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#endif
#include <sys/module.h>
#include <sys/pool.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
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

static npf_alg_t *	alg_icmp;

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
	default:
		return ENOTTY;
	}
	return 0;
}

/*
 * npfa_icmp_match: ALG matching inspector, determines ALG case and
 * establishes a session for "backwards" stream.
 */
static bool
npfa_icmp_match(npf_cache_t *npc, nbuf_t *nbuf, void *ntptr)
{
	const int proto = npc->npc_proto;
	void *n_ptr = nbuf_dataptr(nbuf);

	/* Handle TCP/UDP traceroute - check for port range. */
	if (proto != IPPROTO_TCP && proto != IPPROTO_UDP) {
		return false;
	}
	KASSERT(npf_iscached(npc, NPC_PORTS));
	in_port_t dport = ntohs(npc->npc_dport);
	if (dport < TR_BASE_PORT || dport > TR_PORT_RANGE) {
		return false;
	}

	/* Check for low TTL. */
	const u_int offby = offsetof(struct ip, ip_ttl);
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL) {
		return false;
	}
	uint8_t ttl;
	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(uint8_t), &ttl)) {
		return false;
	}
	if (ttl > TR_MAX_TTL) {
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
static inline bool
npf_icmp_uniqid(const int type, npf_cache_t *npc, nbuf_t *nbuf, void *n_ptr)
{
	u_int offby;

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
		if (!npf_ip4_proto(npc, nbuf, n_ptr)) {
			return false;
		}
		const int proto = npc->npc_proto;
		if (proto != IPPROTO_TCP && proto != IPPROTO_UDP) {
			return false;
		}
		if (!npf_fetch_ip4addrs(npc, nbuf, n_ptr)) {
			return false;
		}
		if (!npf_fetch_ports(npc, nbuf, n_ptr, proto)) {
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
		offby = offsetof(struct icmp, icmp_id);
		if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL) {
			return false;
		}
		if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(uint16_t),
		    &npc->npc_icmp_id)) {
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

/*
 * npfa_icmp_session: ALG session inspector, determines unique identifiers.
 */
static bool
npfa_icmp_session(npf_cache_t *npc, nbuf_t *nbuf, void *keyptr)
{
	npf_cache_t *key = keyptr;
	void *n_ptr;

	/* ICMP? Get unique identifiers from ICMP packet. */
	if (npc->npc_proto != IPPROTO_ICMP) {
		return false;
	}
	KASSERT(npf_iscached(npc, NPC_IP46 | NPC_ICMP));
	key->npc_info = NPC_ICMP;

	/* Advance to ICMP header. */
	n_ptr = nbuf_dataptr(nbuf);
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, npc->npc_hlen)) == NULL) {
		return false;
	}

	/* Fetch into the separate (key) cache. */
	if (!npf_icmp_uniqid(npc->npc_icmp_type, key, nbuf, n_ptr)) {
		return false;
	}

	if (npf_iscached(key, NPC_ICMP_ID)) {
		/* Construct the key. */
		key->npc_proto = npc->npc_proto;
		key->npc_dir = npc->npc_dir;
		/* Save IP addresses. */
		key->npc_srcip = npc->npc_srcip;
		key->npc_dstip = npc->npc_dstip;
		key->npc_info |= NPC_IP46 | NPC_ADDRS | NPC_PORTS;
		/* Fake ports with ICMP query IDs. */
		key->npc_sport = key->npc_icmp_id;
		key->npc_dport = key->npc_icmp_id;
	} else {
		in_addr_t addr;
		in_port_t port;
		/*
		 * Embedded IP packet is the original of "forwards" stream.
		 * We should imitate the "backwards" stream for inspection.
		 */
		KASSERT(npf_iscached(key, NPC_IP46 | NPC_ADDRS | NPC_PORTS));
		addr = key->npc_srcip;
		port = key->npc_sport;
		key->npc_srcip = key->npc_dstip;
		key->npc_dstip = addr;
		key->npc_sport = key->npc_dport;
		key->npc_dport = port;
	}
	return true;
}

/*
 * npfa_icmp_natin: ALG inbound translation inspector, rewrite IP address
 * in the IP header, which is embedded in ICMP packet.
 */
static bool
npfa_icmp_natin(npf_cache_t *npc, nbuf_t *nbuf, void *ntptr)
{
	void *n_ptr = nbuf_dataptr(nbuf);
	npf_cache_t enpc;
	u_int offby;
	uint16_t cksum;

	/* XXX: Duplicated work. */
	if (!npfa_icmp_session(npc, nbuf, &enpc)) {
		return false;
	}
	KASSERT(npf_iscached(&enpc, NPC_IP46 | NPC_ADDRS | NPC_PORTS));

	/* Advance to ICMP checksum and fetch it. */
	offby = npc->npc_hlen + offsetof(struct icmp, icmp_cksum);
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL) {
		return false;
	}
	if (nbuf_fetch_datum(nbuf, n_ptr, sizeof(uint16_t), &cksum)) {
		return false;
	}

	/* Save the data for checksum update later. */
	void *cnbuf = nbuf, *cnptr = n_ptr;
	uint16_t ecksum = enpc.npc_ipsum;

	/* Advance to the original IP header, which is embedded after ICMP. */
	offby = offsetof(struct icmp, icmp_ip) -
	    offsetof(struct icmp, icmp_cksum);
	if ((n_ptr = nbuf_advance(&nbuf, n_ptr, offby)) == NULL) {
		return false;
	}

	/*
	 * Rewrite source IP address and port of the embedded IP header,
	 * which represents original packet - therefore passing PFIL_OUT.
	 */
	npf_nat_t *nt = ntptr;
	in_addr_t addr;
	in_port_t port;

	npf_nat_getorig(nt, &addr, &port);

	if (!npf_rwrip(&enpc, nbuf, n_ptr, PFIL_OUT, addr)) {
		return false;
	}
	if (!npf_rwrport(&enpc, nbuf, n_ptr, PFIL_OUT, port, addr)) {
		return false;
	}

	/*
	 * Fixup and update ICMP checksum.
	 * Note: npf_rwrip() has updated the IP checksum.
	 */
	cksum = npf_fixup32_cksum(cksum, enpc.npc_srcip, addr);
	cksum = npf_fixup16_cksum(cksum, enpc.npc_sport, port);
	cksum = npf_fixup16_cksum(cksum, ecksum, enpc.npc_ipsum);
	/* FIXME: Updated UDP/TCP checksum joins-in too., when != 0, sigh. */
	if (nbuf_store_datum(cnbuf, cnptr, sizeof(uint16_t), &cksum)){
		return false;
	}
	return true;
}
