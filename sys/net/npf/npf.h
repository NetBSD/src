/*	$NetBSD: npf.h,v 1.16 2012/04/14 19:01:21 rmind Exp $	*/

/*-
 * Copyright (c) 2009-2011 The NetBSD Foundation, Inc.
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
 * Public NPF interfaces.
 */

#ifndef _NPF_NET_H_
#define _NPF_NET_H_

#include <sys/param.h>
#include <sys/types.h>

#include <sys/ioctl.h>
#include <prop/proplib.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>

#define	NPF_VERSION		4

/*
 * Public declarations and definitions.
 */

/* Storage of address (both for IPv4 and IPv6) and netmask */
typedef struct in6_addr		npf_addr_t;
typedef uint8_t			npf_netmask_t;

#define	NPF_MAX_NETMASK		(128)
#define	NPF_NO_NETMASK		((npf_netmask_t)~0)

#if defined(_KERNEL)

/* Network buffer. */
typedef void			nbuf_t;

struct npf_rproc;
typedef struct npf_rproc	npf_rproc_t;

/*
 * Packet information cache.
 */
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

#define	NPC_IP4		0x01	/* Indicates fetched IPv4 header. */
#define	NPC_IP6		0x02	/* Indicates IPv6 header. */
#define	NPC_IPFRAG	0x04	/* IPv4/IPv6 fragment. */
#define	NPC_LAYER4	0x08	/* Layer 4 has been fetched. */

#define	NPC_TCP		0x10	/* TCP header. */
#define	NPC_UDP		0x20	/* UDP header. */
#define	NPC_ICMP	0x40	/* ICMP header. */
#define	NPC_ICMP_ID	0x80	/* ICMP with query ID. */

#define	NPC_IP46	(NPC_IP4|NPC_IP6)

typedef struct {
	/* Information flags. */
	uint32_t		npc_info;
	/* Pointers to the IP v4/v6 addresses. */
	npf_addr_t *		npc_srcip;
	npf_addr_t *		npc_dstip;
	/* Size (v4 or v6) of IP addresses. */
	int			npc_ipsz;
	u_int			npc_hlen;
	int			npc_next_proto;
	/* IPv4, IPv6. */
	union {
		struct ip	v4;
		struct ip6_hdr	v6;
	} npc_ip;
	/* TCP, UDP, ICMP. */
	union {
		struct tcphdr	tcp;
		struct udphdr	udp;
		struct icmp	icmp;
	} npc_l4;
} npf_cache_t;

static inline void
npf_generate_mask(npf_addr_t *dst, const npf_netmask_t omask)
{
	uint_fast8_t length = omask;

	/* Note: maximum length is 32 for IPv4 and 128 for IPv6. */
	KASSERT(length <= NPF_MAX_NETMASK);

	for (int i = 0; i < 4; i++) {
		if (length >= 32) {
			dst->s6_addr32[i] = htonl(0xffffffff);
			length -= 32;
		} else {
			dst->s6_addr32[i] = htonl(0xffffffff << (32 - length));
			length = 0;
		}
	}
}

static inline void
npf_calculate_masked_addr(npf_addr_t *dst, const npf_addr_t *src,
    const npf_netmask_t omask)
{
	npf_addr_t mask;

	npf_generate_mask(&mask, omask);
	for (int i = 0; i < 4; i++) {
		dst->s6_addr32[i] = src->s6_addr32[i] & mask.s6_addr32[i];
	}
}

/*
 * npf_compare_cidr: compare two addresses, either IPv4 or IPv6.
 *
 * => If the mask is NULL, ignore it.
 */
static inline int
npf_compare_cidr(const npf_addr_t *addr1, const npf_netmask_t mask1,
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

static inline bool
npf_iscached(const npf_cache_t *npc, const int inf)
{

	return __predict_true((npc->npc_info & inf) != 0);
}

static inline int
npf_cache_ipproto(const npf_cache_t *npc)
{
	KASSERT(npf_iscached(npc, NPC_IP46));
	return npc->npc_next_proto;
}

static inline u_int
npf_cache_hlen(const npf_cache_t *npc)
{
	KASSERT(npf_iscached(npc, NPC_IP46));
	return npc->npc_hlen;
}

/* Network buffer interface. */
void *		nbuf_dataptr(void *);
void *		nbuf_advance(nbuf_t **, void *, u_int);
int		nbuf_advfetch(nbuf_t **, void **, u_int, size_t, void *);
int		nbuf_advstore(nbuf_t **, void **, u_int, size_t, void *);
int		nbuf_fetch_datum(nbuf_t *, void *, size_t, void *);
int		nbuf_store_datum(nbuf_t *, void *, size_t, void *);

int		nbuf_add_tag(nbuf_t *, uint32_t, uint32_t);
int		nbuf_find_tag(nbuf_t *, uint32_t, void **);

#endif	/* _KERNEL */

/* Rule attributes. */
#define	NPF_RULE_PASS			0x0001
#define	NPF_RULE_DEFAULT		0x0002
#define	NPF_RULE_FINAL			0x0004
#define	NPF_RULE_KEEPSTATE		0x0008
#define	NPF_RULE_RETRST			0x0010
#define	NPF_RULE_RETICMP		0x0020

#define	NPF_RULE_IN			0x10000000
#define	NPF_RULE_OUT			0x20000000
#define	NPF_RULE_DIMASK			(NPF_RULE_IN | NPF_RULE_OUT)

/* Rule procedure flags. */
#define	NPF_RPROC_LOG			0x0001
#define	NPF_RPROC_NORMALIZE		0x0002

/* Address translation types and flags. */
#define	NPF_NATIN			1
#define	NPF_NATOUT			2

#define	NPF_NAT_PORTS			0x01
#define	NPF_NAT_PORTMAP			0x02

/* Table types. */
#define	NPF_TABLE_HASH			1
#define	NPF_TABLE_TREE			2

/* Layers. */
#define	NPF_LAYER_2			2
#define	NPF_LAYER_3			3

/* XXX mbuf.h: just for now. */
#define	PACKET_TAG_NPF			10

/*
 * IOCTL structures.
 */

#define	NPF_IOCTL_TBLENT_ADD		1
#define	NPF_IOCTL_TBLENT_REM		2

typedef struct npf_ioctl_table {
	int			nct_action;
	u_int			nct_tid;
	npf_addr_t		nct_addr;
	npf_netmask_t		nct_mask;
} npf_ioctl_table_t;

typedef enum {
	/* Packets passed. */
	NPF_STAT_PASS_DEFAULT,
	NPF_STAT_PASS_RULESET,
	NPF_STAT_PASS_SESSION,
	/* Packets blocked. */
	NPF_STAT_BLOCK_DEFAULT,
	NPF_STAT_BLOCK_RULESET,
	/* Session and NAT entries. */
	NPF_STAT_SESSION_CREATE,
	NPF_STAT_SESSION_DESTROY,
	NPF_STAT_NAT_CREATE,
	NPF_STAT_NAT_DESTROY,
	/* Invalid state cases. */
	NPF_STAT_INVALID_STATE,
	NPF_STAT_INVALID_STATE_TCP1,
	NPF_STAT_INVALID_STATE_TCP2,
	NPF_STAT_INVALID_STATE_TCP3,
	/* Raced packets. */
	NPF_STAT_RACE_SESSION,
	NPF_STAT_RACE_NAT,
	/* Rule procedure cases. */
	NPF_STAT_RPROC_LOG,
	NPF_STAT_RPROC_NORM,
	/* Other errors. */
	NPF_STAT_ERROR,
	/* Count (last). */
	NPF_STATS_COUNT
} npf_stats_t;

#define	NPF_STATS_SIZE		(sizeof(uint64_t) * NPF_STATS_COUNT)

/*
 * IOCTL operations.
 */

#define	IOC_NPF_VERSION		_IOR('N', 100, int)
#define	IOC_NPF_SWITCH		_IOW('N', 101, int)
#define	IOC_NPF_RELOAD		_IOWR('N', 102, struct plistref)
#define	IOC_NPF_TABLE		_IOW('N', 103, struct npf_ioctl_table)
#define	IOC_NPF_STATS		_IOW('N', 104, void *)
#define	IOC_NPF_SESSIONS_SAVE	_IOR('N', 105, struct plistref)
#define	IOC_NPF_SESSIONS_LOAD	_IOW('N', 106, struct plistref)
#define	IOC_NPF_UPDATE_RULE	_IOWR('N', 107, struct plistref)
#define	IOC_NPF_GETCONF		_IOR('N', 108, struct plistref)

#endif	/* _NPF_NET_H_ */
