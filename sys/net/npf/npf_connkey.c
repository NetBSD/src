/*-
 * Copyright (c) 2014-2020 Mindaugas Rasiukevicius <rmind at netbsd org>
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
 * Connection key -- is an n-tuple structure encoding the address length,
 * layer 3 protocol, source and destination addresses and ports (or other
 * protocol IDs) and some configurable elements (see below).
 *
 * Key layout
 *
 *	The single key is formed out of 32-bit integers.  The layout is
 *	as follows (first row -- fields, second row -- number of bits):
 *
 *	| alen | proto |  ckey  | src-id | dst-id | src-addr | dst-addr |
 *	+------+-------+--------+--------+--------+----------+----------+
 *	|   4  |   8   |   20   |   16   |   16   |  32-128  |  32-128  |
 *
 *	The source and destination are inverted if the key is for the
 *	backwards stream (NPF_FLOW_BACK).  The address length depends on
 *	the 'alen' field.  The length is in words and is either 1 or 4,
 *	meaning 4 or 16 in bytes.
 *
 *	The 20-bit configurable key area ('ckey') is for the optional
 *	elements which may be included or excluded by the user.  It has
 *	the following layout:
 *
 *	| direction | interface-id |
 *	+-----------+--------------+
 *	|     2     |      18      |
 *
 *	Note: neither direction nor interface ID cannot be zero; we rely
 *	on this by reserving the zero 'ckey' value to for the case when
 *	these checks are not applicable.
 *
 * Embedding in the connection structure (npf_conn_t)
 *
 *	Two keys are stored in the npf_conn_t::c_keys[] array, which is
 *	variable-length, depending on whether the keys store IPv4 or IPv6
 *	addresses.  The length of the first key determines the position
 *	of the second key.
 *
 * WARNING: the keys must be immutable while they are in conndb.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_connkey.c,v 1.2 2020/05/30 14:16:56 rmind Exp $");

#include <sys/param.h>
#include <sys/types.h>
#endif

#define __NPF_CONN_PRIVATE
#include "npf_conn.h"
#include "npf_impl.h"

unsigned
npf_connkey_setkey(npf_connkey_t *key, unsigned alen, unsigned proto,
    const void *ipv, const uint16_t *id, const npf_flow_t flow)
{
	const npf_addr_t * const *ips = ipv;
	uint32_t *k = key->ck_key;
	unsigned isrc, idst;

	if (__predict_true(flow == NPF_FLOW_FORW)) {
		isrc = NPF_SRC, idst = NPF_DST;
	} else {
		isrc = NPF_DST, idst = NPF_SRC;
	}

	/*
	 * See the key layout explanation above.
	 */
	KASSERT((alen >> 2) <= 0xf && proto <= 0xff);
	k[0] = ((uint32_t)(alen >> 2) << 28) | (proto << 20);
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

void
npf_connkey_getkey(const npf_connkey_t *key, unsigned *alen, unsigned *proto,
    npf_addr_t *ips, uint16_t *id)
{
	const uint32_t *k = key->ck_key;

	/*
	 * See the key layout explanation above.
	 */

	*alen = (k[0] >> 28) << 2;
	*proto = (k[0] >> 16) & 0xff;
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

static inline void
npf_connkey_setckey(npf_connkey_t *key, unsigned ifid, unsigned di)
{
	if (ifid) {
		/*
		 * Interface ID: the lower 18 bits of the 20-bit 'ckey'.
		 * Note: the interface ID cannot be zero.
		 */
		CTASSERT(NPF_MAX_IFMAP < (1U << 18));
		key->ck_key[0] |= ifid;
	}
	if (di) {
		/*
		 * Direction: The highest 2 bits of the 20-bit 'ckey'.
		 * Note: we rely on PFIL_IN and PFIL_OUT definitions.
		 */
		CTASSERT(PFIL_IN == 0x1 || PFIL_OUT == 0x2);
		KASSERT((di & ~PFIL_ALL) == 0);
		key->ck_key[0] |= ((uint32_t)di << 18);
	}
}

static void
npf_connkey_getckey(const npf_connkey_t *key, unsigned *ifid, unsigned *di)
{
	const uint32_t * const k = key->ck_key;

	*ifid = k[0] & ((1U << 20) - 1);
	*di = (k[0] >> 18) & PFIL_ALL;
}

/*
 * npf_conn_adjkey: adjust the connection key by setting the address/port.
 *
 * => The 'which' must either be NPF_SRC or NPF_DST.
 */
void
npf_conn_adjkey(npf_connkey_t *key, const npf_addr_t *naddr,
    const uint16_t id, const unsigned which)
{
	const unsigned alen = NPF_CONNKEY_ALEN(key);
	uint32_t * const k = key->ck_key;
	uint32_t *addr = &k[2 + ((alen >> 2) * which)];

	KASSERT(which == NPF_SRC || which == NPF_DST);
	KASSERT(alen > 0);
	memcpy(addr, naddr, alen);

	if (id) {
		const uint32_t oid = k[1];
		const unsigned shift = 16 * !which;
		const uint32_t mask = 0xffff0000 >> shift;
		k[1] = ((uint32_t)id << shift) | (oid & mask);
	}
}

static unsigned
npf_connkey_copy(const npf_connkey_t *skey, npf_connkey_t *dkey, bool invert)
{
	const unsigned klen = NPF_CONNKEY_LEN(skey);
	const uint32_t *sk = skey->ck_key;
	uint32_t *dk = dkey->ck_key;

	if (invert) {
		const unsigned alen = NPF_CONNKEY_ALEN(skey);
		const unsigned nwords = alen >> 2;

		dk[0] = sk[1];
		dk[1] = (sk[1] >> 16) | (sk[1] << 16);
		memcpy(&dk[2], &sk[2 + nwords], alen);
		memcpy(&dk[2 + nwords], &sk[2], alen);
	} else {
		memcpy(dk, sk, klen);
	}
	return klen;
}

/*
 * npf_conn_conkey: construct a key for the connection lookup.
 *
 * => Returns the key length in bytes or zero on failure.
 */
unsigned
npf_conn_conkey(const npf_cache_t *npc, npf_connkey_t *key,
    const unsigned di, const npf_flow_t flow)
{
	const npf_conn_params_t *params = npc->npc_ctx->params[NPF_PARAMS_CONN];
	const nbuf_t *nbuf = npc->npc_nbuf;
	const unsigned proto = npc->npc_proto;
	const unsigned alen = npc->npc_alen;
	const struct tcphdr *th;
	const struct udphdr *uh;
	uint16_t id[2] = { 0, 0 };
	unsigned ret;

	if (npc->npc_ckey) {
		/*
		 * Request to override the connection key.
		 */
		const bool invert = flow != NPF_FLOW_FORW;
		return npf_connkey_copy(npc->npc_ckey, key, invert);
	}

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

	ret = npf_connkey_setkey(key, alen, proto, npc->npc_ips, id, flow);
	npf_connkey_setckey(key,
	    params->connkey_interface ? nbuf->nb_ifid : 0,
	    params->connkey_direction ? (di & PFIL_ALL) : 0);
	return ret;
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
npf_connkey_export(npf_t *npf, const npf_connkey_t *key)
{
	unsigned alen, proto, ifid, di;
	npf_addr_t ips[2];
	uint16_t ids[2];
	nvlist_t *key_nv;

	key_nv = nvlist_create(0);

	npf_connkey_getkey(key, &alen, &proto, ips, ids);
	nvlist_add_number(key_nv, "proto", proto);
	nvlist_add_number(key_nv, "sport", ids[NPF_SRC]);
	nvlist_add_number(key_nv, "dport", ids[NPF_DST]);
	nvlist_add_binary(key_nv, "saddr", &ips[NPF_SRC], alen);
	nvlist_add_binary(key_nv, "daddr", &ips[NPF_DST], alen);

	npf_connkey_getckey(key, &ifid, &di);
	if (ifid) {
		char ifname[IFNAMSIZ];
		npf_ifmap_copyname(npf, ifid, ifname, sizeof(ifname));
		nvlist_add_string(key_nv, "ifname", ifname);
	}
	if (di) {
		nvlist_add_number(key_nv, "di", di);
	}

	return key_nv;
}

unsigned
npf_connkey_import(npf_t *npf, const nvlist_t *key_nv, npf_connkey_t *key)
{
	npf_addr_t const * ips[2];
	size_t alen1, alen2, proto;
	unsigned ret, di, ifid = 0;
	const char *ifname;
	uint16_t ids[2];

	proto = dnvlist_get_number(key_nv, "proto", 0);
	if (proto >= IPPROTO_MAX) {
		return 0;
	}
	ids[NPF_SRC] = dnvlist_get_number(key_nv, "sport", 0);
	ids[NPF_DST] = dnvlist_get_number(key_nv, "dport", 0);
	ips[NPF_SRC] = dnvlist_get_binary(key_nv, "saddr", &alen1, NULL, 0);
	ips[NPF_DST] = dnvlist_get_binary(key_nv, "daddr", &alen2, NULL, 0);
	if (alen1 == 0 || alen1 > sizeof(npf_addr_t) || alen1 != alen2) {
		return 0;
	}
	ret = npf_connkey_setkey(key, alen1, proto, ips, ids, NPF_FLOW_FORW);
	if (ret == 0) {
		return 0;
	}

	ifname = dnvlist_get_string(key_nv, "ifname", NULL);
	if (ifname && (ifid = npf_ifmap_register(npf, ifname)) == 0) {
		return 0;
	}
	di = dnvlist_get_number(key_nv, "di", 0) & PFIL_ALL;
	npf_connkey_setckey(key, ifid, di);

	return ret;
}

#if defined(DDB) || defined(_NPF_TESTING)

void
npf_connkey_print(const npf_connkey_t *key)
{
	unsigned alen, proto, ifid, di;
	npf_addr_t ips[2];
	uint16_t ids[2];

	npf_connkey_getkey(key, &alen, &proto, ips, ids);
	npf_connkey_getckey(key, &ifid, &di);
	printf("\tkey (ifid %u, di %x)\t", ifid, di);
	printf("%s:%u", npf_addr_dump(&ips[0], alen), ids[0]);
	printf("-> %s:%u\n", npf_addr_dump(&ips[1], alen), ids[1]);
}

#endif
