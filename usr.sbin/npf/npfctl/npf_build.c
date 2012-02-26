/*	$NetBSD: npf_build.c,v 1.6 2012/02/26 21:50:05 christos Exp $	*/

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

/*
 * npfctl(8) building of the configuration.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: npf_build.c,v 1.6 2012/02/26 21:50:05 christos Exp $");

#include <sys/types.h>
#include <sys/ioctl.h>

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <err.h>

#include "npfctl.h"

static nl_config_t *		npf_conf = NULL;
static nl_rule_t *		current_group = NULL;
static bool			npf_debug = false;
static bool			defgroup_set = false;

void
npfctl_config_init(bool debug)
{

	npf_conf = npf_config_create();
	if (npf_conf == NULL) {
		errx(EXIT_FAILURE, "npf_config_create failed");
	}
	npf_debug = debug;
}

int
npfctl_config_send(int fd)
{
	int error;

	if (!fd) {
		_npf_config_setsubmit(npf_conf, "./npf.plist");
	}
	if (!defgroup_set) {
		errx(EXIT_FAILURE, "default group was not defined");
	}
	error = npf_config_submit(npf_conf, fd);
	if (error) {
		nl_error_t ne;
		_npf_config_error(npf_conf, &ne);
		npfctl_print_error(&ne);
	}
	npf_config_destroy(npf_conf);
	return error;
}

bool
npfctl_table_exists_p(const char *id)
{
	return npf_table_exists_p(npf_conf, atoi(id));
}

static in_port_t *
npfctl_get_singleport(const npfvar_t *vp)
{
	port_range_t *pr;

	if (npfvar_get_count(vp) > 1) {
		yyerror("multiple ports are not valid");
	}
	pr = npfvar_get_data(vp, NPFVAR_PORT_RANGE, 0);
	if (pr->pr_start != pr->pr_end) {
		yyerror("port range is not valid");
	}
	return &pr->pr_start;
}

static fam_addr_mask_t *
npfctl_get_singlefam(const npfvar_t *vp)
{
	if (npfvar_get_count(vp) > 1) {
		yyerror("multiple addresses are not valid");
	}
	return npfvar_get_data(vp, NPFVAR_FAM, 0);
}

static void
npfctl_build_fam(nc_ctx_t *nc, sa_family_t family,
    fam_addr_mask_t *fam, int opts)
{
	/*
	 * If family is specified, address does not match it and the
	 * address is extracted from the interface, then simply ignore.
	 * Otherwise, address of invalid family was passed manually.
	 */
	if (family != AF_UNSPEC && family != fam->fam_family) {
		if (!fam->fam_interface) {
			yyerror("specified address is not of the required "
			    "family %d", family);
		}
		return;
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
		return;
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
npfctl_build_proto(nc_ctx_t *nc, const opt_proto_t *op)
{
	const npfvar_t *popts = op->op_opts;
	int pflag = 0;

	switch (op->op_proto) {
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
		break;
	case IPPROTO_UDP:
		pflag = NC_MATCH_UDP;
		break;
	case IPPROTO_ICMP:
		/*
		 * Build ICMP block.
		 */
		assert(npfvar_get_count(popts) == 2);

		int *icmp_type, *icmp_code;
		icmp_type = npfvar_get_data(popts, NPFVAR_ICMP, 0);
		icmp_code = npfvar_get_data(popts, NPFVAR_ICMP, 1);
		npfctl_gennc_icmp(nc, *icmp_type, *icmp_code);
		break;
	case -1:
		pflag = NC_MATCH_TCP | NC_MATCH_UDP;
		break;
	default:
		yyerror("protocol %d is not supported", op->op_proto);
	}
	return pflag;
}

static bool
npfctl_build_ncode(nl_rule_t *rl, sa_family_t family, const opt_proto_t *op,
    const filt_opts_t *fopts, bool invert)
{
	nc_ctx_t *nc;
	void *code;
	size_t len;

	if (family == AF_UNSPEC && op->op_proto == -1 &&
	    op->op_opts == NULL && !fopts->fo_from && !fopts->fo_to &&
	    !fopts->fo_from_port_range && !fopts->fo_to_port_range)
		return false;

	int srcflag = NC_MATCH_SRC;
	int dstflag = NC_MATCH_DST;

	if (invert) {
		srcflag = NC_MATCH_DST;
		dstflag = NC_MATCH_SRC;
	}

	nc = npfctl_ncgen_create();

	/* Build IP address blocks. */
	npfctl_build_vars(nc, family, fopts->fo_from, srcflag);
	npfctl_build_vars(nc, family, fopts->fo_to, dstflag);

	/* Build layer 4 protocol blocks. */
	int pflag = npfctl_build_proto(nc, op);

	/* Build port-range blocks. */
	if (fopts->fo_from_port_range) {
		npfctl_build_vars(nc, family, fopts->fo_from_port_range,
		    srcflag | pflag);
	}
	if (fopts->fo_to_port_range) {
		npfctl_build_vars(nc, family, fopts->fo_to_port_range,
		    dstflag | pflag);
	}

	/*
	 * Complete n-code (destroys the context) and pass to the rule.
	 */
	code = npfctl_ncgen_complete(nc, &len);
	if (npf_debug) {
		extern int yylineno;
		printf("RULE AT LINE %d\n", yylineno - 1);
		npfctl_ncgen_print(code, len);
	}
	if (npf_rule_setcode(rl, NPF_CODE_NCODE, code, len) == -1) {
		errx(EXIT_FAILURE, "npf_rule_setcode failed");
	}
	free(code);
	return true;
}

static void
npfctl_build_rpcall(nl_rproc_t *rp, const char *name, npfvar_t *args)
{
	/*
	 * XXX/TODO: Hardcoded for the first release.  However,
	 * rule procedures will become fully dynamic modules.
	 */

	bool log = false, norm = false;
	bool rnd = false, no_df = false;
	int minttl = 0, maxmss = 0;

	if (strcmp(name, "log") == 0) {
		log = true;
	} else if (strcmp(name, "normalise") == 0) {
		norm = true;
	} else {
		yyerror("unknown rule procedure '%s'", name);
	}

	for (size_t i = 0; i < npfvar_get_count(args); i++) {
		module_arg_t *arg;
		const char *aval;

		arg = npfvar_get_data(args, NPFVAR_MODULE_ARG, i);
		aval = arg->ma_name;

		if (log) {
			u_int if_idx = npfctl_find_ifindex(aval);
			if (!if_idx) {
				yyerror("unknown interface '%s'", aval);
			}
			_npf_rproc_setlog(rp, if_idx);
			return;
		}

		const int type = npfvar_get_type(arg->ma_opts, 0);
		if (type != -1 && type != NPFVAR_NUM) {
			yyerror("option '%s' is not numeric", aval);
		}
		unsigned long *opt;

		if (strcmp(aval, "random-id") == 0) {
			rnd = true;
		} else if (strcmp(aval, "min-ttl") == 0) {
			opt = npfvar_get_data(arg->ma_opts, NPFVAR_NUM, 0);
			minttl = *opt;
		} else if (strcmp(aval, "max-mss") == 0) {
			opt = npfvar_get_data(arg->ma_opts, NPFVAR_NUM, 0);
			maxmss = *opt;
		} else if (strcmp(aval, "no-df") == 0) {
			no_df = true;
		} else {
			yyerror("unknown argument '%s'", aval);
		}
	}
	assert(norm == true);
	_npf_rproc_setnorm(rp, rnd, no_df, minttl, maxmss);
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
		proc_op_t *po = npfvar_get_data(procs, NPFVAR_PROC_OP, i);
		npfctl_build_rpcall(rp, po->po_name, po->po_opts);
	}
}

/*
 * npfctl_build_group: create a group, insert into the global ruleset
 * and update the current group pointer.
 */
void
npfctl_build_group(const char *name, int attr, u_int if_idx)
{
	const int attr_di = (NPF_RULE_IN | NPF_RULE_OUT);
	nl_rule_t *rl;

	if (attr & NPF_RULE_DEFAULT) {
		if (defgroup_set) {
			yyerror("multiple default groups are not valid");
		}
		defgroup_set = true;
		attr |= attr_di;

	} else if ((attr & attr_di) == 0) {
		attr |= attr_di;
	}

	rl = npf_rule_create(name, attr | NPF_RULE_FINAL, if_idx);
	npf_rule_insert(npf_conf, NULL, rl, NPF_PRI_NEXT);
	current_group = rl;
}

/*
 * npfctl_build_rule: create a rule, build n-code from filter options,
 * if any, and insert into the ruleset of current group.
 */
void
npfctl_build_rule(int attr, u_int if_idx, sa_family_t family,
    const opt_proto_t *op, const filt_opts_t *fopts, const char *rproc)
{
	nl_rule_t *rl;

	rl = npf_rule_create(NULL, attr, if_idx);
	npfctl_build_ncode(rl, family, op, fopts, false);
	if (rproc && npf_rule_setproc(npf_conf, rl, rproc) != 0) {
		yyerror("rule procedure '%s' is not defined", rproc);
	}
	assert(current_group != NULL);
	npf_rule_insert(npf_conf, current_group, rl, NPF_PRI_NEXT);
}

/*
 * npfctl_build_nat: create a NAT policy of a specified type with a
 * given filter options.
 */
void
npfctl_build_nat(int type, u_int if_idx, const filt_opts_t *fopts,
    npfvar_t *var1, npfvar_t *var2)
{
	opt_proto_t op = { .op_proto = -1, .op_opts = NULL };
	nl_nat_t *nat;
	fam_addr_mask_t *ai;

	assert(type != 0 && if_idx != 0);
	assert(fopts != NULL && var1 != NULL);

	ai = npfctl_get_singlefam(var1);
	assert(ai != NULL);
	if (ai->fam_family != AF_INET) {
		yyerror("IPv6 NAT is not supported");
	}

	switch (type) {
	case NPFCTL_RDR: {
		/*
		 * Redirection: an inbound NAT with a specific port.
		 */
		in_port_t *port = npfctl_get_singleport(var2);
		nat = npf_nat_create(NPF_NATIN, NPF_NAT_PORTS,
		    if_idx, &ai->fam_addr, ai->fam_family, *port);
		break;
	}
	case NPFCTL_BINAT: {
		/*
		 * Bi-directional NAT: a combination of inbound NAT and
		 * outbound NAT policies.  Note that the translation address
		 * is local IP and filter criteria is inverted accordingly.
		 */
		fam_addr_mask_t *tai = npfctl_get_singlefam(var2);
		assert(tai != NULL);
		if (ai->fam_family != AF_INET) {
			yyerror("IPv6 NAT is not supported");
		}
		nat = npf_nat_create(NPF_NATIN, 0, if_idx,
		    &tai->fam_addr, tai->fam_family, 0);
		npfctl_build_ncode(nat, AF_INET, &op, fopts, true);
		npf_nat_insert(npf_conf, nat, NPF_PRI_NEXT);
		/* FALLTHROUGH */
	}
	case NPFCTL_NAT: {
		/*
		 * Traditional NAPT: an outbound NAT policy with port.
		 * If this is another hald for bi-directional NAT, then
		 * no port translation with mapping.
		 */
		nat = npf_nat_create(NPF_NATOUT, type == NPFCTL_NAT ?
		    (NPF_NAT_PORTS | NPF_NAT_PORTMAP) : 0,
		    if_idx, &ai->fam_addr, ai->fam_family, 0);
		break;
	}
	default:
		assert(false);
	}
	npfctl_build_ncode(nat, AF_INET, &op, fopts, false);
	npf_nat_insert(npf_conf, nat, NPF_PRI_NEXT);
}

/*
 * npfctl_fill_table: fill NPF table with entries from a specified file.
 */
static void
npfctl_fill_table(nl_table_t *tl, const char *fname)
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
		fam_addr_mask_t *fam;

		if (*buf == '\n' || *buf == '#') {
			continue;
		}
		fam = npfctl_parse_cidr(buf);
		if (fam == NULL) {
			errx(EXIT_FAILURE, "%s:%d: invalid table entry",
			    fname, l);
		}

		/* Create and add a table entry. */
		npf_table_add_entry(tl, &fam->fam_addr, fam->fam_mask);
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
		npfctl_fill_table(tl, fname);
	}
}
