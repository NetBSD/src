/*	$NetBSD: npfctl.h,v 1.15 2012/06/15 23:24:08 rmind Exp $	*/

/*-
 * Copyright (c) 2009-2012 The NetBSD Foundation, Inc.
 * All rights reserved.
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
	npfvar_t *	fam_interface;
} fam_addr_mask_t;

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
} rule_group_t;

typedef struct proc_op {
	const char *	po_name;
	npfvar_t *	po_opts;
} proc_op_t;

typedef struct module_arg {
	const char *	ma_name;
	npfvar_t *	ma_opts;
} module_arg_t;

void		yyerror(const char *, ...) __printflike(1, 2) __dead;
void *		zalloc(size_t);
void *		xrealloc(void *, size_t);
char *		xstrdup(const char *);
char *		xstrndup(const char *, size_t);

void		npfctl_print_error(const nl_error_t *);
bool		npfctl_table_exists_p(const char *);
in_port_t	npfctl_portno(const char *);
uint8_t		npfctl_icmpcode(uint8_t, const char *);
uint8_t		npfctl_icmptype(const char *);
unsigned long   npfctl_find_ifindex(const char *);
npfvar_t *	npfctl_parse_tcpflag(const char *);
npfvar_t *	npfctl_parse_table_id(const char *);
npfvar_t * 	npfctl_parse_icmpcode(const char *);
npfvar_t * 	npfctl_parse_icmp(int, int);
npfvar_t *	npfctl_parse_iface(const char *);
npfvar_t *	npfctl_parse_port_range(in_port_t, in_port_t);
npfvar_t *	npfctl_parse_port_range_variable(const char *);
npfvar_t *	npfctl_parse_fam_addr_mask(const char *, const char *,
		    unsigned long *);
fam_addr_mask_t *npfctl_parse_cidr(char *);

/*
 * N-code generation interface.
 */

typedef struct nc_ctx nc_ctx_t;

#define	NC_MATCH_DST		0x01
#define	NC_MATCH_SRC		0x02

#define	NC_MATCH_TCP		0x04
#define	NC_MATCH_UDP		0x08
#define	NC_MATCH_ICMP		0x10

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
void		npfctl_gennc_tbl(nc_ctx_t *, int, u_int);
void		npfctl_gennc_tcpfl(nc_ctx_t *, uint8_t, uint8_t);

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
int		npfctl_config_send(int);
int		npfctl_config_show(int);

void		npfctl_build_rproc(const char *, npfvar_t *);
void		npfctl_build_group(const char *, int, u_int);
void		npfctl_build_rule(int, u_int, sa_family_t,
		    const opt_proto_t *, const filt_opts_t *, const char *);
void		npfctl_build_nat(int, int, u_int, const addr_port_t *,
		    const addr_port_t *, const filt_opts_t *);
void		npfctl_build_table(const char *, u_int, const char *);

#endif
