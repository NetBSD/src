/*-
 * Copyright (c) 2011-2019 The NetBSD Foundation, Inc.
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

#ifndef _NPF_LIB_H_
#define _NPF_LIB_H_

#include <sys/types.h>
#include <net/npf.h>

__BEGIN_DECLS

struct nl_config;
struct nl_rule;
struct nl_rproc;
struct nl_table;
struct nl_ext;

typedef struct nl_config	nl_config_t;
typedef struct nl_rule		nl_rule_t;
typedef struct nl_rproc		nl_rproc_t;
typedef struct nl_table		nl_table_t;
typedef struct nl_rule		nl_nat_t;
typedef struct nl_ext		nl_ext_t;

/*
 * Iterator.
 */
#define	NPF_ITER_BEGIN		0

typedef signed long		nl_iter_t;

/*
 * Ruleset prefix(es).
 */

#define	NPF_RULESET_MAP_PREF	"map:"

/*
 * Extensions API types.
 */
typedef int (*npfext_initfunc_t)(void);
typedef nl_ext_t *(*npfext_consfunc_t)(const char *);
typedef int (*npfext_paramfunc_t)(nl_ext_t *, const char *, const char *);

typedef int (*npf_conn_func_t)(unsigned, const npf_addr_t *,
    const in_port_t *, const char *, void *);

/*
 * API functions.
 */

nl_config_t *	npf_config_create(void);
void		npf_config_destroy(nl_config_t *);
int		npf_config_submit(nl_config_t *, int, npf_error_t *);
nl_config_t *	npf_config_retrieve(int);
int		npf_config_flush(int);
nl_config_t *	npf_config_import(const void *, size_t);
void *		npf_config_export(nl_config_t *, size_t *);
bool		npf_config_active_p(nl_config_t *);
bool		npf_config_loaded_p(nl_config_t *);
const void *	npf_config_build(nl_config_t *);

int		npf_alg_load(nl_config_t *, const char *);

int		npf_param_get(nl_config_t *, const char *, int *);
int		npf_param_set(nl_config_t *, const char *, int);
const char *	npf_param_iterate(nl_config_t *, nl_iter_t *, int *, int *);

int		npf_ruleset_add(int, const char *, nl_rule_t *, uint64_t *);
int		npf_ruleset_remove(int, const char *, uint64_t);
int		npf_ruleset_remkey(int, const char *, const void *, size_t);
int		npf_ruleset_flush(int, const char *);

nl_ext_t *	npf_ext_construct(const char *);
void		npf_ext_param_u32(nl_ext_t *, const char *, uint32_t);
void		npf_ext_param_bool(nl_ext_t *, const char *, bool);
void		npf_ext_param_string(nl_ext_t *, const char *, const char *);

nl_rule_t *	npf_rule_create(const char *, uint32_t, const char *);
int		npf_rule_setcode(nl_rule_t *, int, const void *, size_t);
int		npf_rule_setprio(nl_rule_t *, int);
int		npf_rule_setproc(nl_rule_t *, const char *);
int		npf_rule_setkey(nl_rule_t *, const void *, size_t);
int		npf_rule_setinfo(nl_rule_t *, const void *, size_t);
const char *	npf_rule_getname(nl_rule_t *);
uint32_t	npf_rule_getattr(nl_rule_t *);
const char *	npf_rule_getinterface(nl_rule_t *);
const void *	npf_rule_getinfo(nl_rule_t *, size_t *);
const char *	npf_rule_getproc(nl_rule_t *);
uint64_t	npf_rule_getid(nl_rule_t *);
const void *	npf_rule_getcode(nl_rule_t *, int *, size_t *);
bool		npf_rule_exists_p(nl_config_t *, const char *);
int		npf_rule_insert(nl_config_t *, nl_rule_t *, nl_rule_t *);
void *		npf_rule_export(nl_rule_t *, size_t *);
void		npf_rule_destroy(nl_rule_t *);

nl_rproc_t *	npf_rproc_create(const char *);
int		npf_rproc_extcall(nl_rproc_t *, nl_ext_t *);
bool		npf_rproc_exists_p(nl_config_t *, const char *);
int		npf_rproc_insert(nl_config_t *, nl_rproc_t *);
const char *	npf_rproc_getname(nl_rproc_t *);

nl_nat_t *	npf_nat_create(int, unsigned, const char *);
int		npf_nat_setaddr(nl_nat_t *, int, npf_addr_t *, npf_netmask_t);
int		npf_nat_setport(nl_nat_t *, in_port_t);
int		npf_nat_settable(nl_nat_t *, unsigned);
int		npf_nat_settablefilter(nl_nat_t *, int, npf_addr_t *, npf_netmask_t);
int		npf_nat_setalgo(nl_nat_t *, unsigned);
int		npf_nat_setnpt66(nl_nat_t *, uint16_t);
int		npf_nat_gettype(nl_nat_t *);
unsigned	npf_nat_getflags(nl_nat_t *);
const npf_addr_t *npf_nat_getaddr(nl_nat_t *, size_t *, npf_netmask_t *);
in_port_t	npf_nat_getport(nl_nat_t *);
unsigned	npf_nat_gettable(nl_nat_t *);
unsigned	npf_nat_getalgo(nl_nat_t *);
int		npf_nat_insert(nl_config_t *, nl_nat_t *);
int		npf_nat_lookup(int, int, npf_addr_t *[2], in_port_t [2], int, int);

int		npf_conn_list(int, npf_conn_func_t, void *);

nl_table_t *	npf_table_create(const char *, unsigned, int);
const char *	npf_table_getname(nl_table_t *);
unsigned	npf_table_getid(nl_table_t *);
int		npf_table_gettype(nl_table_t *);
int		npf_table_add_entry(nl_table_t *, int,
		    const npf_addr_t *, const npf_netmask_t);
int		npf_table_insert(nl_config_t *, nl_table_t *);
void		npf_table_destroy(nl_table_t *);

int		npf_table_replace(int, nl_table_t *, npf_error_t *);

#ifdef _NPF_PRIVATE

#include <ifaddrs.h>

nl_rule_t *	npf_rule_iterate(nl_config_t *, nl_iter_t *, unsigned *);
nl_nat_t *	npf_nat_iterate(nl_config_t *, nl_iter_t *);
nl_rproc_t *	npf_rproc_iterate(nl_config_t *, nl_iter_t *);
nl_table_t *	npf_table_iterate(nl_config_t *, nl_iter_t *);

int		_npf_ruleset_list(int, const char *, nl_config_t *);
void		_npf_debug_addif(nl_config_t *, const char *);
void		_npf_config_dump(nl_config_t *, int);

#endif

__END_DECLS

#endif	/* _NPF_LIB_H_ */
