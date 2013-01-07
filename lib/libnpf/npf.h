/*	$NetBSD: npf.h,v 1.6.2.6 2013/01/07 16:51:08 riz Exp $	*/

/*-
 * Copyright (c) 2011-2012 The NetBSD Foundation, Inc.
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

typedef struct nl_config	nl_config_t;
typedef struct nl_rule		nl_rule_t;
typedef struct nl_rproc		nl_rproc_t;
typedef struct nl_table		nl_table_t;

typedef struct nl_rule		nl_nat_t;

typedef struct nl_ext		nl_ext_t;

typedef int (*npfext_initfunc_t)(void);
typedef nl_ext_t *(*npfext_consfunc_t)(const char *);
typedef int (*npfext_paramfunc_t)(nl_ext_t *, const char *, const char *);

#ifdef _NPF_PRIVATE

typedef struct {
	int		ne_id;
	char *		ne_source_file;
	u_int		ne_source_line;
	int		ne_ncode_error;
	int		ne_ncode_errat;
} nl_error_t;

typedef void (*nl_rule_callback_t)(nl_rule_t *, unsigned);
typedef void (*nl_table_callback_t)(unsigned, int);

#endif

#define	NPF_CODE_NCODE		1
#define	NPF_CODE_BPF		2

#define	NPF_PRI_NEXT		(-1)

#define	NPF_MAX_TABLE_ID	(16)

nl_config_t *	npf_config_create(void);
int		npf_config_submit(nl_config_t *, int);
void		npf_config_destroy(nl_config_t *);
nl_config_t *	npf_config_retrieve(int, bool *, bool *);
int		npf_config_flush(int);

nl_ext_t *	npf_ext_construct(const char *name);
void		npf_ext_param_u32(nl_ext_t *, const char *, uint32_t);
void		npf_ext_param_bool(nl_ext_t *, const char *, bool);

nl_rule_t *	npf_rule_create(const char *, uint32_t, u_int);
int		npf_rule_setcode(nl_rule_t *, int, const void *, size_t);
int		npf_rule_setproc(nl_config_t *, nl_rule_t *, const char *);
bool		npf_rule_exists_p(nl_config_t *, const char *);
int		npf_rule_insert(nl_config_t *, nl_rule_t *, nl_rule_t *, pri_t);
void		npf_rule_destroy(nl_rule_t *);

nl_rproc_t *	npf_rproc_create(const char *);
int		npf_rproc_extcall(nl_rproc_t *, nl_ext_t *);
bool		npf_rproc_exists_p(nl_config_t *, const char *);
int		npf_rproc_insert(nl_config_t *, nl_rproc_t *);

nl_nat_t *	npf_nat_create(int, u_int, u_int, npf_addr_t *, int, in_port_t);
int		npf_nat_insert(nl_config_t *, nl_nat_t *, pri_t);

nl_table_t *	npf_table_create(u_int, int);
int		npf_table_add_entry(nl_table_t *, int,
		    const npf_addr_t *, const npf_netmask_t);
bool		npf_table_exists_p(nl_config_t *, u_int);
int		npf_table_insert(nl_config_t *, nl_table_t *);
void		npf_table_destroy(nl_table_t *);

#ifdef _NPF_PRIVATE

#include <ifaddrs.h>

int		npf_update_rule(int, const char *, nl_rule_t *);
int		npf_sessions_send(int, const char *);
int		npf_sessions_recv(int, const char *);

void		_npf_config_error(nl_config_t *, nl_error_t *);
void		_npf_config_setsubmit(nl_config_t *, const char *);
int		_npf_rule_foreach(nl_config_t *, nl_rule_callback_t);
pri_t		_npf_rule_getinfo(nl_rule_t *, const char **, uint32_t *,
		    u_int *);
const void *	_npf_rule_ncode(nl_rule_t *, size_t *);
const char *	_npf_rule_rproc(nl_rule_t *);
int		_npf_nat_foreach(nl_config_t *, nl_rule_callback_t);
void		_npf_nat_getinfo(nl_nat_t *, int *, u_int *, npf_addr_t *,
		    size_t *, in_port_t *);
void		_npf_table_foreach(nl_config_t *, nl_table_callback_t);

void		_npf_debug_addif(nl_config_t *, struct ifaddrs *, u_int);
#endif

__END_DECLS

#endif	/* _NPF_LIB_H_ */
