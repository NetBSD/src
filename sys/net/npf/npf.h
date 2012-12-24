/*	$NetBSD: npf.h,v 1.25 2012/12/24 19:05:42 rmind Exp $	*/

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

#define	NPF_VERSION		8

/*
 * Public declarations and definitions.
 */

/* Storage of address (both for IPv4 and IPv6) and netmask */
typedef struct in6_addr		npf_addr_t;
typedef uint8_t			npf_netmask_t;

#define	NPF_MAX_NETMASK		(128)
#define	NPF_NO_NETMASK		((npf_netmask_t)~0)

#if defined(_KERNEL)

#define	NPF_DECISION_BLOCK	0
#define	NPF_DECISION_PASS	1

#define	NPF_EXT_MODULE(name, req)	\
    MODULE(MODULE_CLASS_MISC, name, "npf," req)

/*
 * Packet information cache.
 */
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>

#define	NPC_IP4		0x01	/* Indicates fetched IPv4 header. */
#define	NPC_IP6		0x02	/* Indicates IPv6 header. */
#define	NPC_IPFRAG	0x04	/* IPv4/IPv6 fragment. */
#define	NPC_LAYER4	0x08	/* Layer 4 has been fetched. */

#define	NPC_TCP		0x10	/* TCP header. */
#define	NPC_UDP		0x20	/* UDP header. */
#define	NPC_ICMP	0x40	/* ICMP header. */
#define	NPC_ICMP_ID	0x80	/* ICMP with query ID. */

#define	NPC_ALG_EXEC	0x100	/* ALG execution. */

#define	NPC_IP46	(NPC_IP4|NPC_IP6)

typedef struct {
	/* Information flags. */
	uint32_t		npc_info;
	/* Pointers to the IP v4/v6 addresses. */
	npf_addr_t *		npc_srcip;
	npf_addr_t *		npc_dstip;
	/* Size (v4 or v6) of IP addresses. */
	uint8_t			npc_alen;
	uint8_t			npc_hlen;
	uint16_t		npc_proto;
	/* IPv4, IPv6. */
	union {
		struct ip *		v4;
		struct ip6_hdr *	v6;
	} npc_ip;
	/* TCP, UDP, ICMP. */
	union {
		struct tcphdr *		tcp;
		struct udphdr *		udp;
		struct icmp *		icmp;
		struct icmp6_hdr *	icmp6;
		void *			hdr;
	} npc_l4;
} npf_cache_t;

static inline bool
npf_iscached(const npf_cache_t *npc, const int inf)
{

	return __predict_true((npc->npc_info & inf) != 0);
}

static inline int
npf_cache_ipproto(const npf_cache_t *npc)
{
	KASSERT(npf_iscached(npc, NPC_IP46));
	return npc->npc_proto;
}

static inline u_int
npf_cache_hlen(const npf_cache_t *npc)
{
	KASSERT(npf_iscached(npc, NPC_IP46));
	return npc->npc_hlen;
}

/*
 * Network buffer interface.
 */

#define	NBUF_DATAREF_RESET	0x01

typedef struct {
	struct mbuf *	nb_mbuf0;
	struct mbuf *	nb_mbuf;
	void *		nb_nptr;
	const ifnet_t *	nb_ifp;
	int		nb_flags;
} nbuf_t;

void		nbuf_init(nbuf_t *, struct mbuf *, const ifnet_t *);
void		nbuf_reset(nbuf_t *);
struct mbuf *	nbuf_head_mbuf(nbuf_t *);

bool		nbuf_flag_p(const nbuf_t *, int);
void		nbuf_unset_flag(nbuf_t *, int);

void *		nbuf_dataptr(nbuf_t *);
size_t		nbuf_offset(const nbuf_t *);
void *		nbuf_advance(nbuf_t *, size_t, size_t);

void *		nbuf_ensure_contig(nbuf_t *, size_t);
void *		nbuf_ensure_writable(nbuf_t *, size_t);

bool		nbuf_cksum_barrier(nbuf_t *, int);
int		nbuf_add_tag(nbuf_t *, uint32_t, uint32_t);
int		nbuf_find_tag(nbuf_t *, uint32_t, void **);

/*
 * NPF extensions and rule procedure interface.
 */

struct npf_rproc;
typedef struct npf_rproc	npf_rproc_t;

void		npf_rproc_assign(npf_rproc_t *, void *);

typedef struct {
	unsigned int	version;
	void *		ctx;
	int		(*ctor)(npf_rproc_t *, prop_dictionary_t);
	void		(*dtor)(npf_rproc_t *, void *);
	void		(*proc)(npf_cache_t *, nbuf_t *, void *, int *);
} npf_ext_ops_t;

void *		npf_ext_register(const char *, const npf_ext_ops_t *);
int		npf_ext_unregister(void *);

/*
 * Misc.
 */

bool		npf_autounload_p(void);

#endif	/* _KERNEL */

/* Rule attributes. */
#define	NPF_RULE_PASS			0x0001
#define	NPF_RULE_DEFAULT		0x0002
#define	NPF_RULE_FINAL			0x0004
#define	NPF_RULE_STATEFUL		0x0008
#define	NPF_RULE_RETRST			0x0010
#define	NPF_RULE_RETICMP		0x0020

#define	NPF_RULE_IN			0x10000000
#define	NPF_RULE_OUT			0x20000000
#define	NPF_RULE_DIMASK			(NPF_RULE_IN | NPF_RULE_OUT)

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

#define	NPF_IOCTL_TBLENT_LOOKUP		0
#define	NPF_IOCTL_TBLENT_ADD		1
#define	NPF_IOCTL_TBLENT_REM		2
#define	NPF_IOCTL_TBLENT_LIST		3

typedef struct npf_ioctl_ent {
	int			alen;
	npf_addr_t		addr;
	npf_netmask_t		mask;
} npf_ioctl_ent_t;

typedef struct npf_ioctl_buf {
	void *			buf;
	size_t			len;
} npf_ioctl_buf_t;

typedef struct npf_ioctl_table {
	int			nct_action;
	u_int			nct_tid;
	union {
		npf_ioctl_ent_t	ent;
		npf_ioctl_buf_t	buf;
	} nct_data;
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
	/* Fragments. */
	NPF_STAT_FRAGMENTS,
	NPF_STAT_REASSEMBLY,
	NPF_STAT_REASSFAIL,
	/* Other errors. */
	NPF_STAT_ERROR,
	/* nbuf non-contiguous cases. */
	NPF_STAT_NBUF_NONCONTIG,
	NPF_STAT_NBUF_CONTIG_FAIL,
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
