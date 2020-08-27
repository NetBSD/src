/*-
 * Copyright (c) 2009-2019 The NetBSD Foundation, Inc.
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

#if !defined(_KERNEL) && !defined(_NPF_STANDALONE)
#error "Kernel-level header only"
#endif

#ifdef _KERNEL_OPT
/* For INET/INET6 definitions. */
#include "opt_inet.h"
#include "opt_inet6.h"
#endif

#ifdef _KERNEL
#include <sys/types.h>
#include <sys/queue.h>

#include <net/bpf.h>
#include <net/bpfjit.h>
#include <net/if.h>
#endif
#include <dnv.h>
#include <nv.h>

#include "npf.h"
#include "npfkern.h"

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
struct npf_rprocset;
struct npf_portmap;
struct npf_nat;
struct npf_conn;

typedef struct npf_ruleset	npf_ruleset_t;
typedef struct npf_rule		npf_rule_t;
typedef struct npf_portmap	npf_portmap_t;
typedef struct npf_nat		npf_nat_t;
typedef struct npf_rprocset	npf_rprocset_t;
typedef struct npf_alg		npf_alg_t;
typedef struct npf_natpolicy	npf_natpolicy_t;
typedef struct npf_conn		npf_conn_t;

struct npf_conndb;
struct npf_table;
struct npf_tableset;
struct npf_algset;
struct npf_ifmap;

typedef struct npf_conndb	npf_conndb_t;
typedef struct npf_table	npf_table_t;
typedef struct npf_tableset	npf_tableset_t;
typedef struct npf_algset	npf_algset_t;

#ifdef __NetBSD__
typedef void			ebr_t;
#endif

/*
 * DEFINITIONS.
 */

typedef struct {
	npf_ruleset_t *		ruleset;
	npf_ruleset_t *		nat_ruleset;
	npf_rprocset_t *	rule_procs;
	npf_tableset_t *	tableset;
	bool			default_pass;
} npf_config_t;

typedef void (*npf_workfunc_t)(npf_t *);

typedef struct {
	uint64_t	mi_rid;
	unsigned	mi_retfl;
	unsigned	mi_di;
} npf_match_info_t;

/*
 * Some artificial limits.
 * Note: very unlikely to have many ALGs.
 */
#define	NPF_MAX_RULES		(1024 * 1024)
#define	NPF_MAX_TABLES		128
#define	NPF_MAX_RPROCS		128
#define	NPF_MAX_IFMAP		64
#define	NPF_MAX_ALGS		4
#define	NPF_MAX_WORKS		4

/*
 * CONNECTION STATE STRUCTURES
 */

typedef enum {
	NPF_FLOW_FORW = 0,
	NPF_FLOW_BACK = 1,
} npf_flow_t;

typedef struct {
	uint32_t	nst_end;
	uint32_t	nst_maxend;
	uint32_t	nst_maxwin;
	int		nst_wscale;
} npf_tcpstate_t;

typedef struct {
	unsigned 	nst_state;
	npf_tcpstate_t	nst_tcpst[2];
} npf_state_t;

/*
 * ALG FUNCTIONS.
 */

typedef struct {
	bool		(*match)(npf_cache_t *, npf_nat_t *, int);
	bool		(*translate)(npf_cache_t *, npf_nat_t *, npf_flow_t);
	npf_conn_t *	(*inspect)(npf_cache_t *, int);
	void		(*destroy)(npf_t *, npf_nat_t *, npf_conn_t *);
} npfa_funcs_t;

/*
 * NBUF STRUCTURE.
 */

struct nbuf {
	struct mbuf *	nb_mbuf0;
	struct mbuf *	nb_mbuf;
	void *		nb_nptr;
	const ifnet_t *	nb_ifp;
	unsigned	nb_ifid;
	int		nb_flags;
	const npf_mbufops_t *nb_mops;
};

/*
 * PARAMS.
 */

typedef struct npf_paraminfo npf_paraminfo_t;

typedef struct {
	const char *	name;
	int *		valp;
	int		default_val;
	/*
	 * Minimum and maximum allowed values (inclusive).
	 */
	int		min;
	int		max;
} npf_param_t;

typedef enum {
	NPF_PARAMS_CONN = 0,
	NPF_PARAMS_CONNDB,
	NPF_PARAMS_GENERIC_STATE,
	NPF_PARAMS_TCP_STATE,
	NPF_PARAMS_COUNT
} npf_paramgroup_t;

/*
 * NPF INSTANCE (CONTEXT) STRUCTURE AND AUXILIARY OPERATIONS.
 */

struct npf {
	/* Active NPF configuration. */
	kmutex_t		config_lock;
	ebr_t *			ebr;
	npf_config_t *		config;

	/*
	 * BPF byte-code context, mbuf operations an arbitrary user argument.
	 */
	bpf_ctx_t *		bpfctx;
	const npf_mbufops_t *	mbufops;
	void *			arg;

	/* Parameters. */
	npf_paraminfo_t *	paraminfo;
	void *			params[NPF_PARAMS_COUNT];
	int			ip4_reassembly;
	int			ip6_reassembly;

	/*
	 * Connection tracking state: disabled (off) or enabled (on).
	 * Connection tracking database, connection cache and the lock.
	 * There are two caches (pools): for IPv4 and IPv6.
	 */
	volatile int		conn_tracking;
	kmutex_t		conn_lock;
	npf_conndb_t *		conn_db;
	pool_cache_t		conn_cache[2];

	/* NAT and ALGs. */
	npf_portmap_t *		portmap;
	npf_algset_t *		algset;

	/* Interface mapping. */
	const npf_ifops_t *	ifops;
	struct npf_ifmap *	ifmap;
	unsigned		ifmap_cnt;
	unsigned		ifmap_off;
	kmutex_t		ifmap_lock;

	/* List of extensions and its lock. */
	LIST_HEAD(, npf_ext)	ext_list;
	kmutex_t		ext_lock;

	/* Associated worker information. */
	unsigned		worker_flags;
	LIST_ENTRY(npf)		worker_entry;
	unsigned		worker_wait_time;
	npf_workfunc_t		worker_funcs[NPF_MAX_WORKS];

	/* Statistics. */
	percpu_t *		stats_percpu;
};

/*
 * NPF extensions and rule procedure interface.
 */

struct npf_rproc;
typedef struct npf_rproc npf_rproc_t;

typedef struct {
	u_int	version;
	void *	ctx;
	int	(*ctor)(npf_rproc_t *, const nvlist_t *);
	void	(*dtor)(npf_rproc_t *, void *);
	bool	(*proc)(npf_cache_t *, void *, const npf_match_info_t *, int *);
} npf_ext_ops_t;

void *		npf_ext_register(npf_t *, const char *, const npf_ext_ops_t *);
int		npf_ext_unregister(npf_t *, void *);
void		npf_rproc_assign(npf_rproc_t *, void *);

/*
 * INTERFACES.
 */

/* NPF config, statistics, etc. */
void		npf_config_init(npf_t *);
void		npf_config_fini(npf_t *);

npf_config_t *	npf_config_enter(npf_t *);
void		npf_config_exit(npf_t *);
void		npf_config_sync(npf_t *);
bool		npf_config_locked_p(npf_t *);
int		npf_config_read_enter(npf_t *);
void		npf_config_read_exit(npf_t *, int);

npf_config_t *	npf_config_create(void);
void		npf_config_destroy(npf_config_t *);
void		npf_config_load(npf_t *, npf_config_t *, npf_conndb_t *, bool);
npf_ruleset_t *	npf_config_ruleset(npf_t *npf);
npf_ruleset_t *	npf_config_natset(npf_t *npf);
npf_tableset_t *npf_config_tableset(npf_t *npf);
bool		npf_default_pass(npf_t *);
bool		npf_active_p(void);

int		npf_worker_sysinit(unsigned);
void		npf_worker_sysfini(void);
int		npf_worker_addfunc(npf_t *, npf_workfunc_t);
void		npf_worker_enlist(npf_t *);
void		npf_worker_discharge(npf_t *);
void		npf_worker_signal(npf_t *);

int		npfctl_run_op(npf_t *, unsigned, const nvlist_t *, nvlist_t *);
int		npfctl_table(npf_t *, void *);

void		npf_stats_inc(npf_t *, npf_stats_t);
void		npf_stats_dec(npf_t *, npf_stats_t);

void		npf_param_init(npf_t *);
void		npf_param_fini(npf_t *);
void		npf_param_register(npf_t *, npf_param_t *, unsigned);
void *		npf_param_allocgroup(npf_t *, npf_paramgroup_t, size_t);
void		npf_param_freegroup(npf_t *, npf_paramgroup_t, size_t);
int		npf_param_check(npf_t *, const char *, int);
int		npf_params_export(const npf_t *, nvlist_t *);

void		npf_ifmap_init(npf_t *, const npf_ifops_t *);
void		npf_ifmap_fini(npf_t *);
u_int		npf_ifmap_register(npf_t *, const char *);
void		npf_ifmap_flush(npf_t *);
u_int		npf_ifmap_getid(npf_t *, const ifnet_t *);
void		npf_ifmap_copylogname(npf_t *, unsigned, char *, size_t);
void		npf_ifmap_copyname(npf_t *, unsigned, char *, size_t);

void		npf_ifaddr_sync(npf_t *, ifnet_t *);
void		npf_ifaddr_flush(npf_t *, ifnet_t *);
void		npf_ifaddr_syncall(npf_t *);

/* Protocol helpers. */
int		npf_cache_all(npf_cache_t *);
void		npf_recache(npf_cache_t *);

bool		npf_rwrip(const npf_cache_t *, u_int, const npf_addr_t *);
bool		npf_rwrport(const npf_cache_t *, u_int, const in_port_t);
bool		npf_rwrcksum(const npf_cache_t *, u_int,
		    const npf_addr_t *, const in_port_t);
int		npf_napt_rwr(const npf_cache_t *, u_int, const npf_addr_t *,
		    const in_addr_t);
int		npf_npt66_rwr(const npf_cache_t *, u_int, const npf_addr_t *,
		    npf_netmask_t, uint16_t);

uint16_t	npf_fixup16_cksum(uint16_t, uint16_t, uint16_t);
uint16_t	npf_fixup32_cksum(uint16_t, uint32_t, uint32_t);
uint16_t	npf_addr_cksum(uint16_t, int, const npf_addr_t *,
		    const npf_addr_t *);
uint32_t	npf_addr_mix(const int, const npf_addr_t *, const npf_addr_t *);
int		npf_addr_cmp(const npf_addr_t *, const npf_netmask_t,
		    const npf_addr_t *, const npf_netmask_t, const int);
void		npf_addr_mask(const npf_addr_t *, const npf_netmask_t,
		    const int, npf_addr_t *);
void		npf_addr_bitor(const npf_addr_t *, const npf_netmask_t,
		    const int, npf_addr_t *);
int		npf_netmask_check(const int, npf_netmask_t);

int		npf_tcpsaw(const npf_cache_t *, tcp_seq *, tcp_seq *,
		    uint32_t *);
bool		npf_fetch_tcpopts(npf_cache_t *, uint16_t *, int *);
bool		npf_set_mss(npf_cache_t *, uint16_t, uint16_t *, uint16_t *,
		    bool *);
bool		npf_return_block(npf_cache_t *, const int);

/* BPF interface. */
void		npf_bpf_sysinit(void);
void		npf_bpf_sysfini(void);
void		npf_bpf_prepare(npf_cache_t *, bpf_args_t *, uint32_t *);
int		npf_bpf_filter(bpf_args_t *, const void *, bpfjit_func_t);
void *		npf_bpf_compile(void *, size_t);
bool		npf_bpf_validate(const void *, size_t);

/* Tableset interface. */
void		npf_tableset_sysinit(void);
void		npf_tableset_sysfini(void);

npf_tableset_t *npf_tableset_create(u_int);
void		npf_tableset_destroy(npf_tableset_t *);
int		npf_tableset_insert(npf_tableset_t *, npf_table_t *);
npf_table_t *	npf_tableset_getbyname(npf_tableset_t *, const char *);
npf_table_t *	npf_tableset_getbyid(npf_tableset_t *, u_int);
npf_table_t *	npf_tableset_swap(npf_tableset_t *, npf_table_t *);
void		npf_tableset_reload(npf_t *, npf_tableset_t *, npf_tableset_t *);
int		npf_tableset_export(npf_t *, const npf_tableset_t *, nvlist_t *);

npf_table_t *	npf_table_create(const char *, u_int, int, const void *, size_t);
void		npf_table_destroy(npf_table_t *);

u_int		npf_table_getid(npf_table_t *);
int		npf_table_check(npf_tableset_t *, const char *, uint64_t, uint64_t, bool);
int		npf_table_insert(npf_table_t *, const int,
		    const npf_addr_t *, const npf_netmask_t);
int		npf_table_remove(npf_table_t *, const int,
		    const npf_addr_t *, const npf_netmask_t);
int		npf_table_lookup(npf_table_t *, const int, const npf_addr_t *);
npf_addr_t *	npf_table_getsome(npf_table_t *, const int, unsigned);
int		npf_table_list(npf_table_t *, void *, size_t);
int		npf_table_flush(npf_table_t *);
void		npf_table_gc(npf_t *, npf_table_t *);

/* Ruleset interface. */
npf_ruleset_t *	npf_ruleset_create(size_t);
void		npf_ruleset_destroy(npf_ruleset_t *);
void		npf_ruleset_insert(npf_ruleset_t *, npf_rule_t *);
void		npf_ruleset_reload(npf_t *, npf_ruleset_t *,
		    npf_ruleset_t *, bool);
npf_natpolicy_t *npf_ruleset_findnat(npf_ruleset_t *, uint64_t);
void		npf_ruleset_freealg(npf_ruleset_t *, npf_alg_t *);
int		npf_ruleset_export(npf_t *, const npf_ruleset_t *,
		    const char *, nvlist_t *);

npf_rule_t *	npf_ruleset_lookup(npf_ruleset_t *, const char *);
int		npf_ruleset_add(npf_ruleset_t *, const char *, npf_rule_t *);
int		npf_ruleset_remove(npf_ruleset_t *, const char *, uint64_t);
int		npf_ruleset_remkey(npf_ruleset_t *, const char *,
		    const void *, size_t);
int		npf_ruleset_list(npf_t *, npf_ruleset_t *, const char *, nvlist_t *);
int		npf_ruleset_flush(npf_ruleset_t *, const char *);
void		npf_ruleset_gc(npf_ruleset_t *);

npf_rule_t *	npf_ruleset_inspect(npf_cache_t *, const npf_ruleset_t *,
		    const int, const int);
int		npf_rule_conclude(const npf_rule_t *, npf_match_info_t *);

/* Rule interface. */
npf_rule_t *	npf_rule_alloc(npf_t *, const nvlist_t *);
void		npf_rule_setcode(npf_rule_t *, int, void *, size_t);
void		npf_rule_setrproc(npf_rule_t *, npf_rproc_t *);
void		npf_rule_free(npf_rule_t *);
uint64_t	npf_rule_getid(const npf_rule_t *);
npf_natpolicy_t *npf_rule_getnat(const npf_rule_t *);
void		npf_rule_setnat(npf_rule_t *, npf_natpolicy_t *);
npf_rproc_t *	npf_rule_getrproc(const npf_rule_t *);

void		npf_ext_init(npf_t *);
void		npf_ext_fini(npf_t *);
int		npf_ext_construct(npf_t *, const char *,
		    npf_rproc_t *, const nvlist_t *);

npf_rprocset_t *npf_rprocset_create(void);
void		npf_rprocset_destroy(npf_rprocset_t *);
npf_rproc_t *	npf_rprocset_lookup(npf_rprocset_t *, const char *);
void		npf_rprocset_insert(npf_rprocset_t *, npf_rproc_t *);
int		npf_rprocset_export(const npf_rprocset_t *, nvlist_t *);

npf_rproc_t *	npf_rproc_create(const nvlist_t *);
void		npf_rproc_acquire(npf_rproc_t *);
void		npf_rproc_release(npf_rproc_t *);
const char *	npf_rproc_getname(const npf_rproc_t *);
bool		npf_rproc_run(npf_cache_t *, npf_rproc_t *,
		    const npf_match_info_t *, int *);

/* State handling. */
void		npf_state_sysinit(npf_t *);
void		npf_state_sysfini(npf_t *);

bool		npf_state_init(npf_cache_t *, npf_state_t *);
bool		npf_state_inspect(npf_cache_t *, npf_state_t *, npf_flow_t);
int		npf_state_etime(npf_t *, const npf_state_t *, const int);
void		npf_state_destroy(npf_state_t *);

void		npf_state_tcp_sysinit(npf_t *);
void		npf_state_tcp_sysfini(npf_t *);
bool		npf_state_tcp(npf_cache_t *, npf_state_t *, npf_flow_t);
int		npf_state_tcp_timeout(npf_t *, const npf_state_t *);

/* Portmap. */
void		npf_portmap_sysinit(void);
void		npf_portmap_sysfini(void);

void		npf_portmap_init(npf_t *);
void		npf_portmap_fini(npf_t *);

npf_portmap_t *	npf_portmap_create(int, int);
void		npf_portmap_destroy(npf_portmap_t *);

in_port_t	npf_portmap_get(npf_portmap_t *, int, const npf_addr_t *);
bool		npf_portmap_take(npf_portmap_t *, int, const npf_addr_t *, in_port_t);
void		npf_portmap_put(npf_portmap_t *, int, const npf_addr_t *, in_port_t);
void		npf_portmap_flush(npf_portmap_t *);

/* NAT. */
void		npf_nat_sysinit(void);
void		npf_nat_sysfini(void);
npf_natpolicy_t *npf_natpolicy_create(npf_t *, const nvlist_t *, npf_ruleset_t *);
int		npf_natpolicy_export(const npf_natpolicy_t *, nvlist_t *);
void		npf_natpolicy_destroy(npf_natpolicy_t *);
bool		npf_natpolicy_cmp(npf_natpolicy_t *, npf_natpolicy_t *);
void		npf_nat_setid(npf_natpolicy_t *, uint64_t);
uint64_t	npf_nat_getid(const npf_natpolicy_t *);
void		npf_nat_freealg(npf_natpolicy_t *, npf_alg_t *);

int		npf_do_nat(npf_cache_t *, npf_conn_t *, const unsigned);
npf_nat_t *	npf_nat_share_policy(npf_cache_t *, npf_conn_t *, npf_nat_t *);
void		npf_nat_destroy(npf_conn_t *, npf_nat_t *);
void		npf_nat_getorig(npf_nat_t *, npf_addr_t **, in_port_t *);
void		npf_nat_gettrans(npf_nat_t *, npf_addr_t **, in_port_t *);
void		npf_nat_setalg(npf_nat_t *, npf_alg_t *, uintptr_t);
npf_alg_t *	npf_nat_getalg(const npf_nat_t *);
uintptr_t	npf_nat_getalgarg(const npf_nat_t *);

void		npf_nat_export(npf_t *, const npf_nat_t *, nvlist_t *);
npf_nat_t *	npf_nat_import(npf_t *, const nvlist_t *, npf_ruleset_t *,
		    npf_conn_t *);

/* ALG interface. */
void		npf_alg_sysinit(void);
void		npf_alg_sysfini(void);
void		npf_alg_init(npf_t *);
void		npf_alg_fini(npf_t *);
npf_alg_t *	npf_alg_register(npf_t *, const char *, const npfa_funcs_t *);
int		npf_alg_unregister(npf_t *, npf_alg_t *);
npf_alg_t *	npf_alg_construct(npf_t *, const char *);
bool		npf_alg_match(npf_cache_t *, npf_nat_t *, int);
void		npf_alg_exec(npf_cache_t *, npf_nat_t *, const npf_flow_t);
npf_conn_t *	npf_alg_conn(npf_cache_t *, int);
int		npf_alg_export(npf_t *, nvlist_t *);
void		npf_alg_destroy(npf_t *, npf_alg_t *, npf_nat_t *, npf_conn_t *);

/* Wrappers for the reclamation mechanism. */
ebr_t *		npf_ebr_create(void);
void		npf_ebr_destroy(ebr_t *);
void		npf_ebr_register(ebr_t *);
void		npf_ebr_unregister(ebr_t *);
int		npf_ebr_enter(ebr_t *);
void		npf_ebr_exit(ebr_t *, int);
void		npf_ebr_full_sync(ebr_t *);
bool		npf_ebr_incrit_p(ebr_t *);

/* Debugging routines. */
const char *	npf_addr_dump(const npf_addr_t *, int);
void		npf_state_dump(const npf_state_t *);
void		npf_nat_dump(const npf_nat_t *);
void		npf_ruleset_dump(npf_t *, const char *);
void		npf_state_setsampler(void (*)(npf_state_t *, bool));

/* In-kernel routines. */
void		npf_setkernctx(npf_t *);
npf_t *		npf_getkernctx(void);

#endif	/* _NPF_IMPL_H_ */
