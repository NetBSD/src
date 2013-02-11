/*	$NetBSD: npf_build.c,v 1.4.2.11 2013/02/11 21:49:48 riz Exp $	*/

/*-
 * Copyright (c) 2011-2013 The NetBSD Foundation, Inc.
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
 * npfctl(8) building of the configuration.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: npf_build.c,v 1.4.2.11 2013/02/11 21:49:48 riz Exp $");

#include <sys/types.h>
#include <sys/ioctl.h>

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <err.h>

#include "npfctl.h"

#define	MAX_RULE_NESTING	16

static nl_config_t *		npf_conf = NULL;
static bool			npf_debug = false;
static nl_rule_t *		the_rule = NULL;

static nl_rule_t *		current_group[MAX_RULE_NESTING];
static unsigned			rule_nesting_level = 0;
static nl_rule_t *		defgroup = NULL;

void
npfctl_config_init(bool debug)
{
	npf_conf = npf_config_create();
	if (npf_conf == NULL) {
		errx(EXIT_FAILURE, "npf_config_create failed");
	}
	npf_debug = debug;
	memset(current_group, 0, sizeof(current_group));
}

int
npfctl_config_send(int fd, const char *out)
{
	int error;

	if (out) {
		_npf_config_setsubmit(npf_conf, out);
		printf("\nSaving to %s\n", out);
	}
	if (!defgroup) {
		errx(EXIT_FAILURE, "default group was not defined");
	}
	npf_rule_insert(npf_conf, NULL, defgroup);
	error = npf_config_submit(npf_conf, fd);
	if (error) {
		nl_error_t ne;
		_npf_config_error(npf_conf, &ne);
		npfctl_print_error(&ne);
	}
	npf_config_destroy(npf_conf);
	return error;
}

nl_config_t *
npfctl_config_ref(void)
{
	return npf_conf;
}

nl_rule_t *
npfctl_rule_ref(void)
{
	return the_rule;
}

unsigned long
npfctl_debug_addif(const char *ifname)
{
	char tname[] = "npftest";
	const size_t tnamelen = sizeof(tname) - 1;

	if (!npf_debug || strncmp(ifname, tname, tnamelen) != 0) {
		return 0;
	}
	struct ifaddrs ifa = {
		.ifa_name = __UNCONST(ifname),
		.ifa_flags = 0
	};
	unsigned long if_idx = atol(ifname + tnamelen) + 1;
	_npf_debug_addif(npf_conf, &ifa, if_idx);
	return if_idx;
}

bool
npfctl_table_exists_p(const char *id)
{
	return npf_table_exists_p(npf_conf, atoi(id));
}

static in_port_t
npfctl_get_singleport(const npfvar_t *vp)
{
	port_range_t *pr;
	in_port_t *port;

	if (npfvar_get_count(vp) > 1) {
		yyerror("multiple ports are not valid");
	}
	pr = npfvar_get_data(vp, NPFVAR_PORT_RANGE, 0);
	if (pr->pr_start != pr->pr_end) {
		yyerror("port range is not valid");
	}
	port = &pr->pr_start;
	return *port;
}

static fam_addr_mask_t *
npfctl_get_singlefam(const npfvar_t *vp)
{
	if (npfvar_get_count(vp) > 1) {
		yyerror("multiple addresses are not valid");
	}
	return npfvar_get_data(vp, NPFVAR_FAM, 0);
}

static bool
npfctl_build_fam(nc_ctx_t *nc, sa_family_t family,
    fam_addr_mask_t *fam, int opts)
{
	/*
	 * If family is specified, address does not match it and the
	 * address is extracted from the interface, then simply ignore.
	 * Otherwise, address of invalid family was passed manually.
	 */
	if (family != AF_UNSPEC && family != fam->fam_family) {
		if (!fam->fam_ifindex) {
			yyerror("specified address is not of the required "
			    "family %d", family);
		}
		return false;
	}

	/*
	 * Optimise 0.0.0.0/0 case to be NOP.  Otherwise, address with
	 * zero mask would never match and therefore is not valid.
	 */
	if (fam->fam_mask == 0) {
		npf_addr_t zero;

		memset(&zero, 0, sizeof(npf_addr_t));
		if (memcmp(&fam->fam_addr, &zero, sizeof(npf_addr_t))) {
			yyerror("filter criterion would never match");
		}
		return false;
	}

	switch (fam->fam_family) {
	case AF_INET:
		npfctl_gennc_v4cidr(nc, opts,
		    &fam->fam_addr, fam->fam_mask);
		break;
	case AF_INET6:
		npfctl_gennc_v6cidr(nc, opts,
		    &fam->fam_addr, fam->fam_mask);
		break;
	default:
		yyerror("family %d is not supported", fam->fam_family);
	}
	return true;
}

static void
npfctl_build_vars(nc_ctx_t *nc, sa_family_t family, npfvar_t *vars, int opts)
{
	const int type = npfvar_get_type(vars, 0);
	size_t i;

	npfctl_ncgen_group(nc);
	for (i = 0; i < npfvar_get_count(vars); i++) {
		void *data = npfvar_get_data(vars, type, i);
		assert(data != NULL);

		switch (type) {
		case NPFVAR_FAM: {
			fam_addr_mask_t *fam = data;
			npfctl_build_fam(nc, family, fam, opts);
			break;
		}
		case NPFVAR_PORT_RANGE: {
			port_range_t *pr = data;
			if (opts & NC_MATCH_TCP) {
				npfctl_gennc_ports(nc, opts & ~NC_MATCH_UDP,
				    pr->pr_start, pr->pr_end);
			}
			if (opts & NC_MATCH_UDP) {
				npfctl_gennc_ports(nc, opts & ~NC_MATCH_TCP,
				    pr->pr_start, pr->pr_end);
			}
			break;
		}
		case NPFVAR_TABLE: {
			u_int tid = atoi(data);
			npfctl_gennc_tbl(nc, opts, tid);
			break;
		}
		default:
			assert(false);
		}
	}
	npfctl_ncgen_endgroup(nc);
}

static int
npfctl_build_proto(nc_ctx_t *nc, sa_family_t family,
    const opt_proto_t *op, bool noaddrs, bool noports)
{
	const npfvar_t *popts = op->op_opts;
	const int proto = op->op_proto;
	int pflag = 0;

	switch (proto) {
	case IPPROTO_TCP:
		pflag = NC_MATCH_TCP;
		if (!popts) {
			break;
		}
		assert(npfvar_get_count(popts) == 2);

		/* Build TCP flags block (optional). */
		uint8_t *tf, *tf_mask;

		tf = npfvar_get_data(popts, NPFVAR_TCPFLAG, 0);
		tf_mask = npfvar_get_data(popts, NPFVAR_TCPFLAG, 1);
		npfctl_gennc_tcpfl(nc, *tf, *tf_mask);
		noports = false;
		break;
	case IPPROTO_UDP:
		pflag = NC_MATCH_UDP;
		break;
	case IPPROTO_ICMP:
		/*
		 * Build ICMP block.
		 */
		if (!noports) {
			goto invop;
		}
		assert(npfvar_get_count(popts) == 2);

		int *icmp_type, *icmp_code;
		icmp_type = npfvar_get_data(popts, NPFVAR_ICMP, 0);
		icmp_code = npfvar_get_data(popts, NPFVAR_ICMP, 1);
		npfctl_gennc_icmp(nc, *icmp_type, *icmp_code);
		noports = false;
		break;
	case IPPROTO_ICMPV6:
		/*
		 * Build ICMP block.
		 */
		if (!noports) {
			goto invop;
		}
		assert(npfvar_get_count(popts) == 2);

		int *icmp6_type, *icmp6_code;
		icmp6_type = npfvar_get_data(popts, NPFVAR_ICMP6, 0);
		icmp6_code = npfvar_get_data(popts, NPFVAR_ICMP6, 1);
		npfctl_gennc_icmp6(nc, *icmp6_type, *icmp6_code);
		noports = false;
		break;
	case -1:
		pflag = NC_MATCH_TCP | NC_MATCH_UDP;
		noports = false;
		break;
	default:
		/*
		 * No filter options are supported for other protocols,
		 * only the IP addresses are allowed.
		 */
		if (noports) {
			break;
		}
invop:
		yyerror("invalid filter options for protocol %d", proto);
	}

	/*
	 * Build the protocol block, unless other blocks will implicitly
	 * perform the family/protocol checks for us.
	 */
	if ((family != AF_UNSPEC && noaddrs) || (proto != -1 && noports)) {
		uint8_t addrlen;

		switch (family) {
		case AF_INET:
			addrlen = sizeof(struct in_addr);
			break;
		case AF_INET6:
			addrlen = sizeof(struct in6_addr);
			break;
		default:
			addrlen = 0;
		}
		npfctl_gennc_proto(nc,
		    noaddrs ? addrlen : 0,
		    noports ? proto : 0xff);
	}
	return pflag;
}

static bool
npfctl_build_ncode(nl_rule_t *rl, sa_family_t family, const opt_proto_t *op,
    const filt_opts_t *fopts, bool invert)
{
	const addr_port_t *apfrom = &fopts->fo_from;
	const addr_port_t *apto = &fopts->fo_to;
	const int proto = op->op_proto;
	bool noaddrs, noports;
	nc_ctx_t *nc;
	void *code;
	size_t len;

	/*
	 * If none specified, no n-code.
	 */
	noaddrs = !apfrom->ap_netaddr && !apto->ap_netaddr;
	noports = !apfrom->ap_portrange && !apto->ap_portrange;
	if (family == AF_UNSPEC && proto == -1 && !op->op_opts &&
	    noaddrs && noports)
		return false;

	int srcflag = NC_MATCH_SRC;
	int dstflag = NC_MATCH_DST;

	if (invert) {
		srcflag = NC_MATCH_DST;
		dstflag = NC_MATCH_SRC;
	}

	nc = npfctl_ncgen_create();

	/* Build layer 4 protocol blocks. */
	int pflag = npfctl_build_proto(nc, family, op, noaddrs, noports);

	/* Build IP address blocks. */
	npfctl_build_vars(nc, family, apfrom->ap_netaddr, srcflag);
	npfctl_build_vars(nc, family, apto->ap_netaddr, dstflag);

	/* Build port-range blocks. */
	npfctl_build_vars(nc, family, apfrom->ap_portrange, srcflag | pflag);
	npfctl_build_vars(nc, family, apto->ap_portrange, dstflag | pflag);

	/*
	 * Complete n-code (destroys the context) and pass to the rule.
	 */
	code = npfctl_ncgen_complete(nc, &len);
	if (npf_debug) {
		extern int yylineno;
		printf("RULE AT LINE %d\n", yylineno);
		npfctl_ncgen_print(code, len);
	}
	assert(code && len > 0);

	if (npf_rule_setcode(rl, NPF_CODE_NC, code, len) == -1) {
		errx(EXIT_FAILURE, "npf_rule_setcode failed");
	}
	free(code);
	return true;
}

static void
npfctl_build_rpcall(nl_rproc_t *rp, const char *name, npfvar_t *args)
{
	npf_extmod_t *extmod;
	nl_ext_t *extcall;
	int error;

	extmod = npf_extmod_get(name, &extcall);
	if (extmod == NULL) {
		yyerror("unknown rule procedure '%s'", name);
	}

	for (size_t i = 0; i < npfvar_get_count(args); i++) {
		const char *param, *value;
		proc_param_t *p;

		p = npfvar_get_data(args, NPFVAR_PROC_PARAM, i);
		param = p->pp_param;
		value = p->pp_value;

		error = npf_extmod_param(extmod, extcall, param, value);
		switch (error) {
		case EINVAL:
			yyerror("invalid parameter '%s'", param);
		default:
			break;
		}
	}
	error = npf_rproc_extcall(rp, extcall);
	if (error) {
		yyerror(error == EEXIST ?
		    "duplicate procedure call" : "unexpected error");
	}
}

/*
 * npfctl_build_rproc: create and insert a rule procedure.
 */
void
npfctl_build_rproc(const char *name, npfvar_t *procs)
{
	nl_rproc_t *rp;
	size_t i;

	rp = npf_rproc_create(name);
	if (rp == NULL) {
		errx(EXIT_FAILURE, "npf_rproc_create failed");
	}
	npf_rproc_insert(npf_conf, rp);

	for (i = 0; i < npfvar_get_count(procs); i++) {
		proc_call_t *pc = npfvar_get_data(procs, NPFVAR_PROC, i);
		npfctl_build_rpcall(rp, pc->pc_name, pc->pc_opts);
	}
}

/*
 * npfctl_build_group: create a group, insert into the global ruleset,
 * update the current group pointer and increase the nesting level.
 */
void
npfctl_build_group(const char *name, int attr, u_int if_idx, bool def)
{
	const int attr_di = (NPF_RULE_IN | NPF_RULE_OUT);
	nl_rule_t *rl;

	if (def || (attr & attr_di) == 0) {
		attr |= attr_di;
	}

	rl = npf_rule_create(name, attr | NPF_RULE_GROUP, if_idx);
	npf_rule_setprio(rl, NPF_PRI_LAST);
	if (def) {
		if (defgroup) {
			yyerror("multiple default groups are not valid");
		}
		if (rule_nesting_level) {
			yyerror("default group can only be at the top level");
		}
		defgroup = rl;
	} else {
		nl_rule_t *cg = current_group[rule_nesting_level];
		npf_rule_insert(npf_conf, cg, rl);
	}

	/* Set the current group and increase the nesting level. */
	if (rule_nesting_level >= MAX_RULE_NESTING) {
		yyerror("rule nesting limit reached");
	}
	current_group[++rule_nesting_level] = rl;
}

void
npfctl_build_group_end(void)
{
	assert(rule_nesting_level > 0);
	current_group[rule_nesting_level--] = NULL;
}

/*
 * npfctl_build_rule: create a rule, build n-code from filter options,
 * if any, and insert into the ruleset of current group, or set the rule.
 */
void
npfctl_build_rule(int attr, u_int if_idx, sa_family_t family,
    const opt_proto_t *op, const filt_opts_t *fopts, const char *rproc)
{
	nl_rule_t *rl;

	attr |= (npf_conf ? 0 : NPF_RULE_DYNAMIC);
	rl = npf_rule_create(NULL, attr, if_idx);
	npfctl_build_ncode(rl, family, op, fopts, false);
	if (rproc) {
		npf_rule_setproc(rl, rproc);
	}

	if (npf_conf) {
		nl_rule_t *cg = current_group[rule_nesting_level];

		if (rproc && !npf_rproc_exists_p(npf_conf, rproc)) {
			yyerror("rule procedure '%s' is not defined", rproc);
		}
		assert(cg != NULL);
		npf_rule_setprio(rl, NPF_PRI_LAST);
		npf_rule_insert(npf_conf, cg, rl);
	} else {
		/* We have parsed a single rule - set it. */
		the_rule = rl;
	}
}

/*
 * npfctl_build_nat: create a single NAT policy of a specified
 * type with a given filter options.
 */
static void
npfctl_build_nat(int type, u_int if_idx, sa_family_t family,
    const addr_port_t *ap, const filt_opts_t *fopts, bool binat)
{
	const opt_proto_t op = { .op_proto = -1, .op_opts = NULL };
	fam_addr_mask_t *am;
	in_port_t port;
	nl_nat_t *nat;

	if (!ap->ap_netaddr) {
		yyerror("%s network segment is not specified",
		    type == NPF_NATIN ? "inbound" : "outbound");
	}
	am = npfctl_get_singlefam(ap->ap_netaddr);
	if (am->fam_family != family) {
		yyerror("IPv6 NAT is not supported");
	}

	switch (type) {
	case NPF_NATOUT:
		/*
		 * Outbound NAT (or source NAT) policy, usually used for the
		 * traditional NAPT.  If it is a half for bi-directional NAT,
		 * then no port translation with mapping.
		 */
		nat = npf_nat_create(NPF_NATOUT, !binat ?
		    (NPF_NAT_PORTS | NPF_NAT_PORTMAP) : 0,
		    if_idx, &am->fam_addr, am->fam_family, 0);
		break;
	case NPF_NATIN:
		/*
		 * Inbound NAT (or destination NAT).  Unless bi-NAT, a port
		 * must be specified, since it has to be redirection.
		 */
		port = 0;
		if (!binat) {
			if (!ap->ap_portrange) {
				yyerror("inbound port is not specified");
			}
			port = npfctl_get_singleport(ap->ap_portrange);
		}
		nat = npf_nat_create(NPF_NATIN, !binat ? NPF_NAT_PORTS : 0,
		    if_idx, &am->fam_addr, am->fam_family, port);
		break;
	default:
		assert(false);
	}

	npfctl_build_ncode(nat, family, &op, fopts, false);
	npf_nat_insert(npf_conf, nat, NPF_PRI_LAST);
}

/*
 * npfctl_build_natseg: validate and create NAT policies.
 */
void
npfctl_build_natseg(int sd, int type, u_int if_idx, const addr_port_t *ap1,
    const addr_port_t *ap2, const filt_opts_t *fopts)
{
	sa_family_t af = AF_INET;
	filt_opts_t imfopts;
	bool binat;

	if (sd == NPFCTL_NAT_STATIC) {
		yyerror("static NAT is not yet supported");
	}
	assert(sd == NPFCTL_NAT_DYNAMIC);
	assert(if_idx != 0);

	/*
	 * Bi-directional NAT is a combination of inbound NAT and outbound
	 * NAT policies.  Note that the translation address is local IP and
	 * the filter criteria is inverted accordingly.
	 */
	binat = (NPF_NATIN | NPF_NATOUT) == type;

	/*
	 * If the filter criteria is not specified explicitly, apply implicit
	 * filtering according to the given network segments.
	 *
	 * Note: filled below, depending on the type.
	 */
	if (__predict_true(!fopts)) {
		fopts = &imfopts;
	}

	if (type & NPF_NATIN) {
		memset(&imfopts, 0, sizeof(filt_opts_t));
		memcpy(&imfopts.fo_to, ap2, sizeof(addr_port_t));
		npfctl_build_nat(NPF_NATIN, if_idx, af, ap1, fopts, binat);
	}
	if (type & NPF_NATOUT) {
		memset(&imfopts, 0, sizeof(filt_opts_t));
		memcpy(&imfopts.fo_from, ap1, sizeof(addr_port_t));
		npfctl_build_nat(NPF_NATOUT, if_idx, af, ap2, fopts, binat);
	}
}

/*
 * npfctl_fill_table: fill NPF table with entries from a specified file.
 */
static void
npfctl_fill_table(nl_table_t *tl, u_int type, const char *fname)
{
	char *buf = NULL;
	int l = 0;
	FILE *fp;
	size_t n;

	fp = fopen(fname, "r");
	if (fp == NULL) {
		err(EXIT_FAILURE, "open '%s'", fname);
	}
	while (l++, getline(&buf, &n, fp) != -1) {
		fam_addr_mask_t fam;
		int alen;

		if (*buf == '\n' || *buf == '#') {
			continue;
		}

		if (!npfctl_parse_cidr(buf, &fam, &alen)) {
			errx(EXIT_FAILURE,
			    "%s:%d: invalid table entry", fname, l);
		}
		if (type == NPF_TABLE_HASH && fam.fam_mask != NPF_NO_NETMASK) {
			errx(EXIT_FAILURE,
			    "%s:%d: mask used with the hash table", fname, l);
		}

		/* Create and add a table entry. */
		npf_table_add_entry(tl, fam.fam_family,
		    &fam.fam_addr, fam.fam_mask);
	}
	if (buf != NULL) {
		free(buf);
	}
}

/*
 * npfctl_build_table: create an NPF table, add to the configuration and,
 * if required, fill with contents from a file.
 */
void
npfctl_build_table(const char *tid, u_int type, const char *fname)
{
	nl_table_t *tl;
	u_int id;

	id = atoi(tid);
	tl = npf_table_create(id, type);
	assert(tl != NULL);

	if (npf_table_insert(npf_conf, tl)) {
		errx(EXIT_FAILURE, "table '%d' is already defined\n", id);
	}

	if (fname) {
		npfctl_fill_table(tl, type, fname);
	}
}
