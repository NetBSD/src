/*	$NetBSD: npfctl.h,v 1.11.2.13 2013/02/18 18:26:14 riz Exp $	*/

/*-
 * Copyright (c) 2009-2013 The NetBSD Foundation, Inc.
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

#ifndef _NPFCTL_H_
#define _NPFCTL_H_

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <assert.h>
#include <util.h>
#undef ECHO /* XXX util.h */

#include <net/npf_ncode.h>
#include <net/npf.h>

#define	_NPF_PRIVATE
#include <npf.h>

#include "npf_var.h"

#define	NPF_DEV_PATH	"/dev/npf"
#define	NPF_CONF_PATH	"/etc/npf.conf"
#define	NPF_SESSDB_PATH	"/var/db/npf_sessions.db"

typedef struct fam_addr_mask {
	sa_family_t	fam_family;
	npf_addr_t	fam_addr;
	npf_netmask_t	fam_mask;
	unsigned long	fam_ifindex;
} fam_addr_mask_t;

typedef struct ifnet_addr {
	unsigned long	ifna_index;
	sa_family_t	ifna_family;
	npfvar_t *	ifna_filter;
	npfvar_t *	ifna_addrs;
} ifnet_addr_t;

typedef struct port_range {
	in_port_t	pr_start;
	in_port_t	pr_end;
} port_range_t;

typedef struct addr_port {
	npfvar_t *	ap_netaddr;
	npfvar_t *	ap_portrange;
} addr_port_t;

typedef struct filt_opts {
	addr_port_t	fo_from;
	addr_port_t	fo_to;
} filt_opts_t;

typedef struct opt_proto {
	int		op_proto;
	npfvar_t *	op_opts;
} opt_proto_t;

typedef struct rule_group {
	const char *	rg_name;
	uint32_t	rg_attr;
	u_int		rg_ifnum;
	bool		rg_default;
} rule_group_t;

typedef struct proc_call {
	const char *	pc_name;
	npfvar_t *	pc_opts;
} proc_call_t;

typedef struct proc_param {
	const char *	pp_param;
	const char *	pp_value;
} proc_param_t;

enum { NPFCTL_PARSE_FILE, NPFCTL_PARSE_STRING };

void		yyerror(const char *, ...) __printflike(1, 2) __dead;
void		npfctl_parse_file(const char *);
void		npfctl_parse_string(const char *);

void		npfctl_print_error(const nl_error_t *);
char *		npfctl_print_addrmask(int, npf_addr_t *, npf_netmask_t);
bool		npfctl_table_exists_p(const char *);
int		npfctl_protono(const char *);
in_port_t	npfctl_portno(const char *);
uint8_t		npfctl_icmpcode(int, uint8_t, const char *);
uint8_t		npfctl_icmptype(int, const char *);
unsigned long   npfctl_find_ifindex(const char *);
npfvar_t *	npfctl_parse_ifnet(const char *, const int);
npfvar_t *	npfctl_parse_tcpflag(const char *);
npfvar_t *	npfctl_parse_table_id(const char *);
npfvar_t * 	npfctl_parse_icmp(int, int, int);
npfvar_t *	npfctl_parse_port_range(in_port_t, in_port_t);
npfvar_t *	npfctl_parse_port_range_variable(const char *);
npfvar_t *	npfctl_parse_fam_addr_mask(const char *, const char *,
		    unsigned long *);
bool		npfctl_parse_cidr(char *, fam_addr_mask_t *, int *);

/*
 * NPF extension loading.
 */

typedef struct npf_extmod npf_extmod_t;

npf_extmod_t *	npf_extmod_get(const char *, nl_ext_t **);
int		npf_extmod_param(npf_extmod_t *, nl_ext_t *,
		    const char *, const char *);

/*
 * N-code generation interface.
 */

typedef struct nc_ctx nc_ctx_t;

#define	NC_MATCH_DST		0x01
#define	NC_MATCH_SRC		0x02

#define	NC_MATCH_TCP		0x04
#define	NC_MATCH_UDP		0x08
#define	NC_MATCH_ICMP		0x10
#define	NC_MATCH_ICMP6		0x20

nc_ctx_t *	npfctl_ncgen_create(void);
void *		npfctl_ncgen_complete(nc_ctx_t *, size_t *);
void		npfctl_ncgen_print(const void *, size_t);

void		npfctl_ncgen_group(nc_ctx_t *);
void		npfctl_ncgen_endgroup(nc_ctx_t *);

void		npfctl_gennc_v4cidr(nc_ctx_t *, int, const npf_addr_t *,
		    const npf_netmask_t);
void		npfctl_gennc_v6cidr(nc_ctx_t *, int, const npf_addr_t *,
		    const npf_netmask_t);
void		npfctl_gennc_ports(nc_ctx_t *, int, in_port_t, in_port_t);
void		npfctl_gennc_icmp(nc_ctx_t *, int, int);
void		npfctl_gennc_icmp6(nc_ctx_t *, int, int);
void		npfctl_gennc_tbl(nc_ctx_t *, int, u_int);
void		npfctl_gennc_tcpfl(nc_ctx_t *, uint8_t, uint8_t);
void		npfctl_gennc_proto(nc_ctx_t *ctx, uint8_t, uint8_t);

/*
 * N-code disassembler.
 */

typedef struct nc_inf nc_inf_t;

nc_inf_t *	npfctl_ncode_disinf(FILE *);
int		npfctl_ncode_disassemble(nc_inf_t *, const void *, size_t);

/*
 * Configuration building interface.
 */

#define	NPFCTL_NAT_DYNAMIC	1
#define	NPFCTL_NAT_STATIC	2

void		npfctl_config_init(bool);
int		npfctl_config_send(int, const char *);
nl_config_t *	npfctl_config_ref(void);
int		npfctl_config_show(int);
int		npfctl_ruleset_show(int, const char *);

nl_rule_t *	npfctl_rule_ref(void);
unsigned long	npfctl_debug_addif(const char *);

void		npfctl_build_rproc(const char *, npfvar_t *);
void		npfctl_build_group(const char *, int, u_int, bool);
void		npfctl_build_group_end(void);
void		npfctl_build_rule(uint32_t, u_int, sa_family_t,
		    const opt_proto_t *, const filt_opts_t *, const char *);
void		npfctl_build_natseg(int, int, u_int, const addr_port_t *,
		    const addr_port_t *, const filt_opts_t *);
void		npfctl_build_table(const char *, u_int, const char *);

#endif
