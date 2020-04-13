/*-
 * Copyright (c) 2014-2019 Mindaugas Rasiukevicius <rmind at netbsd org>
 * Copyright (c) 2010-2014 The NetBSD Foundation, Inc.
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
 * Connection key -- is a structure encoding the 5-tuple (protocol, address
 * length, source/destination address and source/destination port/ID).
 *
 * Key layout
 *
 *	The single key is formed out of 32-bit integers.  The layout is:
 *
 *	Field: | proto  |  alen  | src-id | dst-id | src-addr | dst-addr |
 *	       +--------+--------+--------+--------+----------+----------+
 *	Bits:  |   16   |   16   |   16   |   16   |  32-128  |  32-128  |
 *
 *	The source and destination are inverted if the key is for the
 *	backwards stream (forw == false).  The address length depends on
 *	the 'alen' field.  The length is in bytes and is either 4 or 16.
 *
 *	Warning: the keys must be immutable while they are in conndb.
 *
 * Embedding in the connection structure (npf_conn_t)
 *
 *	Two keys are stored in the npf_conn_t::c_keys[] array, which is
 *	variable-length, depending on whether the keys store IPv4 or IPv6
 *	addresses.  The length of the first key determines the position
 *	of the second key.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_connkey.c,v 1.1.10.2 2020/04/13 08:05:15 martin Exp $");

#include <sys/param.h>
#include <sys/types.h>
#endif

#define __NPF_CONN_PRIVATE
#include "npf_conn.h"
#include "npf_impl.h"

static inline unsigned
connkey_setkey(npf_connkey_t *key, uint16_t proto, const void *ipv,
    const uint16_t *id, unsigned alen, bool forw)
{
	const npf_addr_t * const *ips = ipv;
	uint32_t *k = key->ck_key;
	unsigned isrc, idst;

	if (__predict_true(forw)) {
		isrc = NPF_SRC, idst = NPF_DST;
	} else {
		isrc = NPF_DST, idst = NPF_SRC;
	}

	/*
	 * See the key layout explanation above.
	 */

	k[0] = ((uint32_t)proto << 16) | (alen & 0xffff);
	k[1] = ((uint32_t)id[isrc] << 16) | id[idst];

	if (__predict_true(alen == sizeof(in_addr_t))) {
		k[2] = ips[isrc]->word32[0];
		k[3] = ips[idst]->word32[0];
		return 4 * sizeof(uint32_t);
	} else {
		const unsigned nwords = alen >> 2;
		memcpy(&k[2], ips[isrc], alen);
		memcpy(&k[2 + nwords], ips[idst], alen);
		return (2 + (nwords * 2)) * sizeof(uint32_t);
	}
}

static inline void
connkey_getkey(const npf_connkey_t *key, uint16_t *proto, npf_addr_t *ips,
    uint16_t *id, uint16_t *alen)
{
	const uint32_t *k = key->ck_key;

	/*
	 * See the key layout explanation above.
	 */

	*proto = k[0] >> 16;
	*alen = k[0] & 0xffff;
	id[NPF_SRC] = k[1] >> 16;
	id[NPF_DST] = k[1] & 0xffff;

	switch (*alen) {
	case sizeof(struct in6_addr):
	case sizeof(struct in_addr):
		memcpy(&ips[NPF_SRC], &k[2], *alen);
		memcpy(&ips[NPF_DST], &k[2 + ((unsigned)*alen >> 2)], *alen);
		return;
	default:
		KASSERT(0);
	}
}

/*
 * npf_conn_adjkey: adjust the connection key by resetting the address/port.
 */
void
npf_conn_adjkey(npf_connkey_t *key, const npf_addr_t *naddr,
    const uint16_t id, const int di)
{
	uint32_t * const k = key->ck_key;
	const unsigned alen = k[0] & 0xffff;
	uint32_t *addr = &k[2 + ((alen >> 2) * di)];

	KASSERT(alen > 0);
	memcpy(addr, naddr, alen);

	if (id) {
		const uint32_t oid = k[1];
		const unsigned shift = 16 * !di;
		const uint32_t mask = 0xffff0000 >> shift;
		k[1] = ((uint32_t)id << shift) | (oid & mask);
	}
}

/*
 * npf_conn_conkey: construct a key for the connection lookup.
 *
 * => Returns the key length in bytes or zero on failure.
 */
unsigned
npf_conn_conkey(const npf_cache_t *npc, npf_connkey_t *key, const bool forw)
{
	const unsigned proto = npc->npc_proto;
	const unsigned alen = npc->npc_alen;
	const struct tcphdr *th;
	const struct udphdr *uh;
	uint16_t id[2] = { 0, 0 };

	switch (proto) {
	case IPPROTO_TCP:
		KASSERT(npf_iscached(npc, NPC_TCP));
		th = npc->npc_l4.tcp;
		id[NPF_SRC] = th->th_sport;
		id[NPF_DST] = th->th_dport;
		break;
	case IPPROTO_UDP:
		KASSERT(npf_iscached(npc, NPC_UDP));
		uh = npc->npc_l4.udp;
		id[NPF_SRC] = uh->uh_sport;
		id[NPF_DST] = uh->uh_dport;
		break;
	case IPPROTO_ICMP:
		if (npf_iscached(npc, NPC_ICMP_ID)) {
			const struct icmp *ic = npc->npc_l4.icmp;
			id[NPF_SRC] = ic->icmp_id;
			id[NPF_DST] = ic->icmp_id;
			break;
		}
		return 0;
	case IPPROTO_ICMPV6:
		if (npf_iscached(npc, NPC_ICMP_ID)) {
			const struct icmp6_hdr *ic6 = npc->npc_l4.icmp6;
			id[NPF_SRC] = ic6->icmp6_id;
			id[NPF_DST] = ic6->icmp6_id;
			break;
		}
		return 0;
	default:
		/* Unsupported protocol. */
		return 0;
	}
	return connkey_setkey(key, proto, npc->npc_ips, id, alen, forw);
}

/*
 * npf_conn_getforwkey: get the address to the "forwards" key.
 */
npf_connkey_t *
npf_conn_getforwkey(npf_conn_t *conn)
{
	return (void *)&conn->c_keys[0];
}

/*
 * npf_conn_getbackkey: get the address to the "backwards" key.
 *
 * => It depends on the address length.
 */
npf_connkey_t *
npf_conn_getbackkey(npf_conn_t *conn, unsigned alen)
{
	const unsigned off = 2 + ((alen * 2) >> 2);
	KASSERT(off == NPF_CONNKEY_V4WORDS || off == NPF_CONNKEY_V6WORDS);
	return (void *)&conn->c_keys[off];
}

/*
 * Connection key exporting/importing.
 */

nvlist_t *
npf_connkey_export(const npf_connkey_t *key)
{
	uint16_t ids[2], alen, proto;
	npf_addr_t ips[2];
	nvlist_t *kdict;

	kdict = nvlist_create(0);
	connkey_getkey(key, &proto, ips, ids, &alen);
	nvlist_add_number(kdict, "proto", proto);
	nvlist_add_number(kdict, "sport", ids[NPF_SRC]);
	nvlist_add_number(kdict, "dport", ids[NPF_DST]);
	nvlist_add_binary(kdict, "saddr", &ips[NPF_SRC], alen);
	nvlist_add_binary(kdict, "daddr", &ips[NPF_DST], alen);
	return kdict;
}

unsigned
npf_connkey_import(const nvlist_t *kdict, npf_connkey_t *key)
{
	npf_addr_t const * ips[2];
	uint16_t proto, ids[2];
	size_t alen1, alen2;

	proto = dnvlist_get_number(kdict, "proto", 0);
	ids[NPF_SRC] = dnvlist_get_number(kdict, "sport", 0);
	ids[NPF_DST] = dnvlist_get_number(kdict, "dport", 0);
	ips[NPF_SRC] = dnvlist_get_binary(kdict, "saddr", &alen1, NULL, 0);
	ips[NPF_DST] = dnvlist_get_binary(kdict, "daddr", &alen2, NULL, 0);
	if (alen1 == 0 || alen1 > sizeof(npf_addr_t) || alen1 != alen2) {
		return 0;
	}
	return connkey_setkey(key, proto, ips, ids, alen1, true);
}

#if defined(DDB) || defined(_NPF_TESTING)

void
npf_connkey_print(const npf_connkey_t *key)
{
	uint16_t proto, ids[2], alen;
	npf_addr_t ips[2];

	connkey_getkey(key, &proto, ips, ids, &alen);
	printf("\tforw %s:%d", npf_addr_dump(&ips[0], alen), ids[0]);
	printf("-> %s:%d\n", npf_addr_dump(&ips[1], alen), ids[1]);
}

#endif
