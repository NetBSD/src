/*-
 * Copyright (c) 2009-2017 The NetBSD Foundation, Inc.
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

#define	NPF_BPFCOP
#include <net/npf.h>

#define	_NPF_PRIVATE
#include <npf.h>

#include "npf_var.h"

#define	NPF_DEV_PATH	"/dev/npf"
#define	NPF_CONF_PATH	"/etc/npf.conf"
#define	NPF_DB_PATH	"/var/db/npf.db"

typedef struct fam_addr_mask {
	sa_family_t	fam_family;
	npf_addr_t	fam_addr;
	npf_netmask_t	fam_mask;
	unsigned long	fam_ifindex;
} fam_addr_mask_t;

typedef struct ifnet_addr {
	char *		ifna_name;
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
	bool		fo_finvert;
	bool		fo_tinvert;
} filt_opts_t;

typedef struct opt_proto {
	int		op_proto;
	npfvar_t *	op_opts;
} opt_proto_t;

typedef struct rule_group {
	const char *	rg_name;
	uint32_t	rg_attr;
	const char *	rg_ifname;
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

typedef enum {
	NPFCTL_PARSE_DEFAULT,
	NPFCTL_PARSE_RULE,
	NPFCTL_PARSE_MAP
} parse_entry_t;

#define	NPF_IFNET_TABLE_PREF		".ifnet-"
#define	NPF_IFNET_TABLE_PREFLEN		(sizeof(NPF_IFNET_TABLE_PREF) - 1)

void		yyerror(const char *, ...) __printflike(1, 2) __dead;
void		npfctl_bpfjit(bool);
void		npfctl_parse_file(const char *);
void		npfctl_parse_string(const char *, parse_entry_t);

bool		join(char *, size_t, int, char **, const char *);
bool		npfctl_addr_iszero(const npf_addr_t *);

void		npfctl_print_error(const npf_error_t *);
char *		npfctl_print_addrmask(int, const char *, const npf_addr_t *,
		    npf_netmask_t);
void		npfctl_note_interface(const char *);
nl_table_t *	npfctl_table_getbyname(nl_config_t *, const char *);
unsigned	npfctl_table_getid(const char *);
const char *	npfctl_table_getname(nl_config_t *, unsigned, bool *);
int		npfctl_protono(const char *);
in_port_t	npfctl_portno(const char *);
uint8_t		npfctl_icmpcode(int, uint8_t, const char *);
uint8_t		npfctl_icmptype(int, const char *);
npfvar_t *	npfctl_ifnet_table(const char *);
npfvar_t *	npfctl_parse_ifnet(const char *, const int);
npfvar_t *	npfctl_parse_tcpflag(const char *);
npfvar_t *	npfctl_parse_table_id(const char *);
npfvar_t * 	npfctl_parse_icmp(int, int, int);
npfvar_t *	npfctl_parse_port_range(in_port_t, in_port_t);
npfvar_t *	npfctl_parse_port_range_variable(const char *, npfvar_t *);
npfvar_t *	npfctl_parse_fam_addr_mask(const char *, const char *,
		    unsigned long *);
bool		npfctl_parse_cidr(char *, fam_addr_mask_t *, int *);
uint16_t	npfctl_npt66_calcadj(npf_netmask_t, const npf_addr_t *,
		    const npf_addr_t *);
int		npfctl_nat_ruleset_p(const char *, bool *);

void		usage(void);
void		npfctl_rule(int, int, char **);
void		npfctl_table_replace(int, int, char **);
void		npfctl_table(int, int, char **);
int		npfctl_conn_list(int, int, char **);

/*
 * NPF extension loading.
 */

typedef struct npf_extmod npf_extmod_t;

npf_extmod_t *	npf_extmod_get(const char *, nl_ext_t **);
int		npf_extmod_param(npf_extmod_t *, nl_ext_t *,
		    const char *, const char *);

/*
 * BFF byte-code generation interface.
 */

typedef struct npf_bpf npf_bpf_t;

#define	MATCH_DST	0x01
#define	MATCH_SRC	0x02
#define	MATCH_INVERT	0x04

enum {
	BM_IPVER, BM_PROTO, BM_SRC_CIDR, BM_SRC_TABLE, BM_DST_CIDR,
	BM_DST_TABLE, BM_SRC_PORTS, BM_DST_PORTS, BM_TCPFL, BM_ICMP_TYPE,
	BM_ICMP_CODE, BM_SRC_NEG, BM_DST_NEG,

	BM_COUNT // total number of the marks
};

npf_bpf_t *	npfctl_bpf_create(void);
struct bpf_program *npfctl_bpf_complete(npf_bpf_t *);
const void *	npfctl_bpf_bmarks(npf_bpf_t *, size_t *);
void		npfctl_bpf_destroy(npf_bpf_t *);

void		npfctl_bpf_group_enter(npf_bpf_t *, bool);
void		npfctl_bpf_group_exit(npf_bpf_t *);

void		npfctl_bpf_ipver(npf_bpf_t *, sa_family_t);
void		npfctl_bpf_proto(npf_bpf_t *, unsigned);
void		npfctl_bpf_cidr(npf_bpf_t *, u_int, sa_family_t,
		    const npf_addr_t *, const npf_netmask_t);
void		npfctl_bpf_ports(npf_bpf_t *, u_int, in_port_t, in_port_t);
void		npfctl_bpf_tcpfl(npf_bpf_t *, uint8_t, uint8_t);
void		npfctl_bpf_icmp(npf_bpf_t *, int, int);
void		npfctl_bpf_table(npf_bpf_t *, u_int, u_int);

/*
 * Configuration building interface.
 */

#define	NPFCTL_NAT_DYNAMIC	1
#define	NPFCTL_NAT_STATIC	2

void		npfctl_config_init(bool);
void		npfctl_config_build(void);
int		npfctl_config_send(int);
nl_config_t *	npfctl_config_ref(void);
int		npfctl_config_show(int);
void		npfctl_config_save(nl_config_t *, const char *);
int		npfctl_ruleset_show(int, const char *);

nl_rule_t *	npfctl_rule_ref(void);
nl_table_t *	npfctl_table_ref(void);
bool		npfctl_debug_addif(const char *);

nl_table_t *	npfctl_load_table(const char *, int, u_int, const char *, FILE *);

void		npfctl_build_alg(const char *);
void		npfctl_build_rproc(const char *, npfvar_t *);
void		npfctl_build_group(const char *, int, const char *, bool);
void		npfctl_build_group_end(void);
void		npfctl_build_rule(uint32_t, const char *, sa_family_t,
		    const npfvar_t *, const filt_opts_t *,
		    const char *, const char *);
void		npfctl_build_natseg(int, int, unsigned, const char *,
		    const addr_port_t *, const addr_port_t *,
		    const npfvar_t *, const filt_opts_t *, unsigned);
void		npfctl_build_maprset(const char *, int, const char *);
void		npfctl_build_table(const char *, u_int, const char *);

void		npfctl_setparam(const char *, int);

/*
 * For the systems which do not define TH_ECE and TW_CRW.
 */
#ifndef	TH_ECE
#define	TH_ECE		0x40
#endif
#ifndef	TH_CWR
#define	TH_CWR		0x80
#endif

#endif
