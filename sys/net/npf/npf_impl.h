/*	$NetBSD: npf_impl.h,v 1.3.4.2 2010/10/22 09:23:14 uebayasi Exp $	*/

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
 * Private NPF structures and interfaces.
 * For internal use within NPF core only.
 */

#ifndef _NPF_IMPL_H_
#define _NPF_IMPL_H_

#include <sys/rbtree.h>
#include <sys/hash.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/rwlock.h>

#include "npf.h"
#include "npf_ncode.h"

#ifdef _NPF_TESTING
#include "testing.h"
#endif

/*
 * STRUCTURE DECLARATIONS.
 *
 * Note: ruleset interface declarations are public.
 */

struct npf_nat;
struct npf_session;

typedef struct npf_nat		npf_nat_t;
typedef struct npf_alg		npf_alg_t;
typedef struct npf_natpolicy	npf_natpolicy_t;
typedef struct npf_session	npf_session_t;

struct npf_tblent;
struct npf_table;

typedef struct npf_tblent	npf_tblent_t;
typedef struct npf_table	npf_table_t;

typedef npf_table_t *		npf_tableset_t;

/*
 * DEFINITIONS.
 */

typedef bool	(*npf_algfunc_t)(npf_cache_t *, void *, void *);

#define	NPF_NCODE_LIMIT		1024
#define	NPF_TABLE_SLOTS		32

/*
 * INTERFACES.
 */

/* NPF control. */
int		npfctl_switch(void *);
int		npfctl_reload(u_long, void *);
int		npfctl_table(void *);

/* Packet filter hooks. */
int		npf_register_pfil(void);
void		npf_unregister_pfil(void);

/* Protocol helpers. */
bool		npf_ip4_proto(npf_cache_t *, nbuf_t *, void *);
bool		npf_fetch_ip4addrs(npf_cache_t *, nbuf_t *, void *);
bool		npf_fetch_ports(npf_cache_t *, nbuf_t *, void *, const int);
bool		npf_fetch_tcpfl(npf_cache_t *, nbuf_t *, void *);
bool		npf_fetch_icmp(npf_cache_t *, nbuf_t *, void *);
bool		npf_cache_all(npf_cache_t *, nbuf_t *);

bool		npf_rwrport(npf_cache_t *, nbuf_t *, void *, const int,
		    in_port_t, in_addr_t);
bool		npf_rwrip(npf_cache_t *, nbuf_t *, void *, const int, in_addr_t);

uint16_t	npf_fixup16_cksum(uint16_t, uint16_t, uint16_t);
uint16_t	npf_fixup32_cksum(uint16_t, uint32_t, uint32_t);

void		npf_return_block(npf_cache_t *, nbuf_t *, const int);

/* Complex instructions. */
int		npf_match_ether(nbuf_t *, int, int, uint16_t, uint32_t *);
int		npf_match_ip4table(npf_cache_t *, nbuf_t *, void *,
		    const int, const u_int);
int		npf_match_ip4mask(npf_cache_t *, nbuf_t *, void *,
		    const int, in_addr_t, in_addr_t);
int		npf_match_tcp_ports(npf_cache_t *, nbuf_t *, void *,
		    const int, const uint32_t);
int		npf_match_udp_ports(npf_cache_t *, nbuf_t *, void *,
		    const int, const uint32_t);
int		npf_match_icmp4(npf_cache_t *, nbuf_t *, void *, const uint32_t);
int		npf_match_tcpfl(npf_cache_t *, nbuf_t *, void *, const uint32_t);

/* Tableset interface. */
int		npf_tableset_sysinit(void);
void		npf_tableset_sysfini(void);

npf_tableset_t *npf_tableset_create(void);
void		npf_tableset_destroy(npf_tableset_t *);
int		npf_tableset_insert(npf_tableset_t *, npf_table_t *);
npf_tableset_t *npf_tableset_reload(npf_tableset_t *);

npf_table_t *	npf_table_create(u_int, int, size_t);
void		npf_table_destroy(npf_table_t *);
void		npf_table_ref(npf_table_t *);
void		npf_table_unref(npf_table_t *);

npf_table_t *	npf_table_get(npf_tableset_t *, u_int);
void		npf_table_put(npf_table_t *);
int		npf_table_check(npf_tableset_t *, u_int, int);
int		npf_table_add_v4cidr(npf_tableset_t *, u_int,
		    in_addr_t, in_addr_t);
int		npf_table_rem_v4cidr(npf_tableset_t *, u_int,
		    in_addr_t, in_addr_t);
int		npf_table_match_v4addr(u_int, in_addr_t);

/* Ruleset interface. */
int		npf_ruleset_sysinit(void);
void		npf_ruleset_sysfini(void);

npf_ruleset_t *	npf_ruleset_create(void);
void		npf_ruleset_destroy(npf_ruleset_t *);
void		npf_ruleset_insert(npf_ruleset_t *, npf_rule_t *);
void		npf_ruleset_reload(npf_ruleset_t *, npf_tableset_t *);

npf_rule_t *	npf_ruleset_match(npf_ruleset_t *, npf_cache_t *, nbuf_t *,
		    struct ifnet *, const int, const int);
npf_rule_t *	npf_ruleset_inspect(npf_cache_t *, nbuf_t *,
		    struct ifnet *, const int, const int);
int		npf_rule_apply(const npf_cache_t *, npf_rule_t *, bool *, int *);
npf_ruleset_t *	npf_rule_subset(npf_rule_t *);

npf_natpolicy_t *npf_rule_getnat(const npf_rule_t *);
void		npf_rule_setnat(npf_rule_t *, npf_natpolicy_t *);

/* State handling interface. */
int		npf_session_sysinit(void);
void		npf_session_sysfini(void);
int		npf_session_tracking(bool);

npf_session_t *	npf_session_inspect(npf_cache_t *, nbuf_t *,
		    struct ifnet *, const int);
npf_session_t *	npf_session_establish(const npf_cache_t *,
		    npf_nat_t *, const int);
void		npf_session_release(npf_session_t *);
bool		npf_session_pass(const npf_session_t *);
void		npf_session_setpass(npf_session_t *);
void		npf_session_link(npf_session_t *, npf_session_t *);
npf_nat_t *	npf_session_retnat(npf_session_t *, const int, bool *);

/* NAT. */
void		npf_nat_sysinit(void);
void		npf_nat_sysfini(void);
npf_natpolicy_t *npf_nat_newpolicy(int, int, in_addr_t, in_port_t);
void		npf_nat_freepolicy(npf_natpolicy_t *);
void		npf_nat_flush(void);
void		npf_nat_reload(npf_ruleset_t *);

int		npf_do_nat(npf_cache_t *, npf_session_t *, nbuf_t *,
		    struct ifnet *, const int);
void		npf_nat_expire(npf_nat_t *);
void		npf_nat_getorig(npf_nat_t *, in_addr_t *, in_port_t *);
void		npf_nat_setalg(npf_nat_t *, npf_alg_t *, uintptr_t);

/* ALG interface. */
void		npf_alg_sysinit(void);
void		npf_alg_sysfini(void);
npf_alg_t *	npf_alg_register(npf_algfunc_t, npf_algfunc_t,
		    npf_algfunc_t, npf_algfunc_t);
int		npf_alg_unregister(npf_alg_t *);
void		npf_alg_match(npf_cache_t *, nbuf_t *, npf_nat_t *);
void		npf_alg_exec(npf_cache_t *, nbuf_t *, npf_nat_t *, const int );
bool		npf_alg_sessionid(npf_cache_t *, nbuf_t *, npf_cache_t *);

/* Debugging routines. */
void		npf_rulenc_dump(npf_rule_t *);
void		npf_sessions_dump(void);
void		npf_nat_dump(npf_nat_t *);

#endif
