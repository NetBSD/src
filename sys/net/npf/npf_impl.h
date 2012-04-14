/*	$NetBSD: npf_impl.h,v 1.13 2012/04/14 19:01:21 rmind Exp $	*/

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
 * Private NPF structures and interfaces.
 * For internal use within NPF core only.
 */

#ifndef _NPF_IMPL_H_
#define _NPF_IMPL_H_

#if !defined(_KERNEL)
#error "Kernel-level header only"
#endif

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/hash.h>
#include <sys/rbtree.h>
#include <sys/rwlock.h>
#include <net/if.h>

#include "npf.h"
#include "npf_ncode.h"

#ifdef _NPF_DEBUG
#define	NPF_PRINTF(x)	printf x
#else
#define	NPF_PRINTF(x)
#endif

/*
 * STRUCTURE DECLARATIONS.
 */

struct npf_ruleset;
struct npf_rule;
struct npf_nat;
struct npf_session;

typedef struct npf_ruleset	npf_ruleset_t;
typedef struct npf_rule		npf_rule_t;
typedef struct npf_nat		npf_nat_t;
typedef struct npf_alg		npf_alg_t;
typedef struct npf_natpolicy	npf_natpolicy_t;
typedef struct npf_session	npf_session_t;

struct npf_sehash;
struct npf_tblent;
struct npf_table;

typedef struct npf_sehash	npf_sehash_t;
typedef struct npf_tblent	npf_tblent_t;
typedef struct npf_table	npf_table_t;

typedef npf_table_t *		npf_tableset_t;

/*
 * DEFINITIONS.
 */

#define	NPF_DECISION_BLOCK	0
#define	NPF_DECISION_PASS	1

typedef bool (*npf_algfunc_t)(npf_cache_t *, nbuf_t *, void *);

#define	NPF_NCODE_LIMIT		1024
#define	NPF_TABLE_SLOTS		32

/*
 * SESSION STATE STRUCTURES
 */

#define	NPF_FLOW_FORW		0
#define	NPF_FLOW_BACK		1

typedef struct {
	uint32_t	nst_end;
	uint32_t	nst_maxend;
	uint32_t	nst_maxwin;
	int		nst_wscale;
} npf_tcpstate_t;

typedef struct {
	kmutex_t	nst_lock;
	int		nst_state;
	npf_tcpstate_t	nst_tcpst[2];
} npf_state_t;

/*
 * INTERFACES.
 */

/* NPF control, statistics, etc. */
void		npf_core_enter(void);
npf_ruleset_t *	npf_core_ruleset(void);
npf_ruleset_t *	npf_core_natset(void);
npf_tableset_t *npf_core_tableset(void);
void		npf_core_exit(void);
bool		npf_core_locked(void);
bool		npf_default_pass(void);
prop_dictionary_t npf_core_dict(void);

void		npf_reload(prop_dictionary_t, npf_ruleset_t *,
		    npf_tableset_t *, npf_ruleset_t *, bool);

void		npflogattach(int);
void		npflogdetach(void);
int		npfctl_switch(void *);
int		npfctl_reload(u_long, void *);
int		npfctl_getconf(u_long, void *);
int		npfctl_sessions_save(u_long, void *);
int		npfctl_sessions_load(u_long, void *);
int		npfctl_update_rule(u_long, void *);
int		npfctl_table(void *);

void		npf_stats_inc(npf_stats_t);
void		npf_stats_dec(npf_stats_t);

/* Packet filter hooks. */
int		npf_pfil_register(void);
void		npf_pfil_unregister(void);
bool		npf_pfil_registered_p(void);
void		npf_log_packet(npf_cache_t *, nbuf_t *, int);

/* Protocol helpers. */
bool		npf_fetch_ip(npf_cache_t *, nbuf_t *, void *);
bool		npf_fetch_tcp(npf_cache_t *, nbuf_t *, void *);
bool		npf_fetch_udp(npf_cache_t *, nbuf_t *, void *);
bool		npf_fetch_icmp(npf_cache_t *, nbuf_t *, void *);
int		npf_cache_all(npf_cache_t *, nbuf_t *);

bool		npf_rwrip(npf_cache_t *, nbuf_t *, void *, const int,
		    npf_addr_t *);
bool		npf_rwrport(npf_cache_t *, nbuf_t *, void *, const int,
		    in_port_t);
bool		npf_rwrcksum(npf_cache_t *, nbuf_t *, void *, const int,
		    npf_addr_t *, in_port_t);

uint16_t	npf_fixup16_cksum(uint16_t, uint16_t, uint16_t);
uint16_t	npf_fixup32_cksum(uint16_t, uint32_t, uint32_t);
uint16_t	npf_addr_cksum(uint16_t, int, npf_addr_t *, npf_addr_t *);
uint32_t	npf_addr_sum(const int, const npf_addr_t *, const npf_addr_t *);
int		npf_tcpsaw(npf_cache_t *, tcp_seq *, tcp_seq *, uint32_t *);
bool		npf_fetch_tcpopts(const npf_cache_t *, nbuf_t *,
		    uint16_t *, int *);
bool		npf_normalize(npf_cache_t *, nbuf_t *, bool, bool, u_int, u_int);
void		npf_return_block(npf_cache_t *, nbuf_t *, const int);

/* Complex instructions. */
int		npf_match_ether(nbuf_t *, int, int, uint16_t, uint32_t *);
int		npf_match_table(npf_cache_t *, nbuf_t *, void *,
		    const int, const u_int);
int		npf_match_ipmask(npf_cache_t *, nbuf_t *, void *,
		    const int, const npf_addr_t *, const npf_netmask_t);
int		npf_match_tcp_ports(npf_cache_t *, nbuf_t *, void *,
		    const int, const uint32_t);
int		npf_match_udp_ports(npf_cache_t *, nbuf_t *, void *,
		    const int, const uint32_t);
int		npf_match_icmp4(npf_cache_t *, nbuf_t *, void *, uint32_t);
int		npf_match_tcpfl(npf_cache_t *, nbuf_t *, void *, uint32_t);

/* Tableset interface. */
void		npf_tableset_sysinit(void);
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
int		npf_table_add_cidr(npf_tableset_t *, u_int,
		    const npf_addr_t *, const npf_netmask_t);
int		npf_table_rem_cidr(npf_tableset_t *, u_int,
		    const npf_addr_t *, const npf_netmask_t);
int		npf_table_match_addr(npf_tableset_t *, u_int,
		    const npf_addr_t *);

/* Ruleset interface. */
npf_ruleset_t *	npf_ruleset_create(void);
void		npf_ruleset_destroy(npf_ruleset_t *);
void		npf_ruleset_insert(npf_ruleset_t *, npf_rule_t *);
void		npf_ruleset_natreload(npf_ruleset_t *, npf_ruleset_t *);
npf_rule_t *	npf_ruleset_matchnat(npf_ruleset_t *, npf_natpolicy_t *);
npf_rule_t *	npf_ruleset_sharepm(npf_ruleset_t *, npf_natpolicy_t *);
npf_rule_t *	npf_ruleset_replace(const char *, npf_ruleset_t *);

npf_rule_t *	npf_ruleset_inspect(npf_cache_t *, nbuf_t *, npf_ruleset_t *,
		    ifnet_t *, const int, const int);
int		npf_rule_apply(npf_cache_t *, nbuf_t *, npf_rule_t *, int *);

/* Rule interface. */
npf_rule_t *	npf_rule_alloc(prop_dictionary_t, npf_rproc_t *, void *, size_t);
void		npf_rule_free(npf_rule_t *);
npf_ruleset_t *	npf_rule_subset(npf_rule_t *);
npf_natpolicy_t *npf_rule_getnat(const npf_rule_t *);
void		npf_rule_setnat(npf_rule_t *, npf_natpolicy_t *);
npf_rproc_t *	npf_rule_getrproc(npf_rule_t *);

npf_rproc_t *	npf_rproc_create(prop_dictionary_t);
void		npf_rproc_acquire(npf_rproc_t *);
void		npf_rproc_release(npf_rproc_t *);
void		npf_rproc_run(npf_cache_t *, nbuf_t *, npf_rproc_t *, int);

/* Session handling interface. */
void		npf_session_sysinit(void);
void		npf_session_sysfini(void);
int		npf_session_tracking(bool);

npf_sehash_t *	sess_htable_create(void);
void		sess_htable_destroy(npf_sehash_t *);
void		sess_htable_reload(npf_sehash_t *);

npf_session_t *	npf_session_inspect(npf_cache_t *, nbuf_t *, const int, int *);
npf_session_t *	npf_session_establish(const npf_cache_t *, nbuf_t *, const int);
void		npf_session_release(npf_session_t *);
void		npf_session_expire(npf_session_t *);
bool		npf_session_pass(const npf_session_t *, npf_rproc_t **);
void		npf_session_setpass(npf_session_t *, npf_rproc_t *);
int		npf_session_setnat(npf_session_t *, npf_nat_t *, const int);
npf_nat_t *	npf_session_retnat(npf_session_t *, const int, bool *);

int		npf_session_save(prop_array_t, prop_array_t);
int		npf_session_restore(npf_sehash_t *, prop_dictionary_t);

/* State handling. */
bool		npf_state_init(const npf_cache_t *, nbuf_t *, npf_state_t *);
bool		npf_state_inspect(const npf_cache_t *, nbuf_t *, npf_state_t *,
		    const bool);
int		npf_state_etime(const npf_state_t *, const int);
void		npf_state_destroy(npf_state_t *);

bool		npf_state_tcp(const npf_cache_t *, nbuf_t *, npf_state_t *, int);
int		npf_state_tcp_timeout(const npf_state_t *);

/* NAT. */
void		npf_nat_sysinit(void);
void		npf_nat_sysfini(void);
npf_natpolicy_t *npf_nat_newpolicy(prop_dictionary_t, npf_ruleset_t *);
void		npf_nat_freepolicy(npf_natpolicy_t *);
bool		npf_nat_matchpolicy(npf_natpolicy_t *, npf_natpolicy_t *);
bool		npf_nat_sharepm(npf_natpolicy_t *, npf_natpolicy_t *);

int		npf_do_nat(npf_cache_t *, npf_session_t *, nbuf_t *,
		    ifnet_t *, const int);
void		npf_nat_expire(npf_nat_t *);
void		npf_nat_getorig(npf_nat_t *, npf_addr_t **, in_port_t *);
void		npf_nat_gettrans(npf_nat_t *, npf_addr_t **, in_port_t *);
void		npf_nat_setalg(npf_nat_t *, npf_alg_t *, uintptr_t);

int		npf_nat_save(prop_dictionary_t, prop_array_t, npf_nat_t *);
npf_nat_t *	npf_nat_restore(prop_dictionary_t, npf_session_t *);

/* ALG interface. */
void		npf_alg_sysinit(void);
void		npf_alg_sysfini(void);
npf_alg_t *	npf_alg_register(npf_algfunc_t, npf_algfunc_t,
		    npf_algfunc_t, npf_algfunc_t);
int		npf_alg_unregister(npf_alg_t *);
bool		npf_alg_match(npf_cache_t *, nbuf_t *, npf_nat_t *);
void		npf_alg_exec(npf_cache_t *, nbuf_t *, npf_nat_t *, const int );
bool		npf_alg_sessionid(npf_cache_t *, nbuf_t *, npf_cache_t *);

/* Debugging routines. */
void		npf_rulenc_dump(npf_rule_t *);
void		npf_sessions_dump(void);
void		npf_state_dump(npf_state_t *);
void		npf_nat_dump(npf_nat_t *);

#endif	/* _NPF_IMPL_H_ */
