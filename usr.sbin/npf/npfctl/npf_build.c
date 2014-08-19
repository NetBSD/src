/*	$NetBSD: npf_build.c,v 1.13.2.4 2014/08/20 00:05:11 tls Exp $	*/

/*-
 * Copyright (c) 2011-2014 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: npf_build.c,v 1.13.2.4 2014/08/20 00:05:11 tls Exp $");

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <netinet/tcp.h>

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

#include <pcap/pcap.h>
#include <cdbw.h>

#include "npfctl.h"

#define	MAX_RULE_NESTING	16

static nl_config_t *		npf_conf = NULL;
static bool			npf_debug = false;
static nl_rule_t *		the_rule = NULL;

static nl_rule_t *		current_group[MAX_RULE_NESTING];
static unsigned			rule_nesting_level = 0;
static nl_rule_t *		defgroup = NULL;

static void			npfctl_dump_bpf(struct bpf_program *);

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
	if (fd) {
		npf_config_destroy(npf_conf);
	}
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

bool
npfctl_debug_addif(const char *ifname)
{
	const char tname[] = "npftest";
	const size_t tnamelen = sizeof(tname) - 1;

	if (npf_debug) {
		_npf_debug_addif(npf_conf, ifname);
		return strncmp(ifname, tname, tnamelen) == 0;
	}
	return 0;
}

unsigned
npfctl_table_getid(const char *name)
{
	unsigned tid = (unsigned)-1;
	nl_table_t *tl;

	/* XXX dynamic ruleset */
	if (!npf_conf) {
		return (unsigned)-1;
	}

	/* XXX: Iterating all as we need to rewind for the next call. */
	while ((tl = npf_table_iterate(npf_conf)) != NULL) {
		const char *tname = npf_table_getname(tl);
		if (strcmp(tname, name) == 0) {
			tid = npf_table_getid(tl);
		}
	}
	return tid;
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
npfctl_build_fam(npf_bpf_t *ctx, sa_family_t family,
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

	family = fam->fam_family;
	if (family != AF_INET && family != AF_INET6) {
		yyerror("family %d is not supported", family);
	}

	/*
	 * Optimise 0.0.0.0/0 case to be NOP.  Otherwise, address with
	 * zero mask would never match and therefore is not valid.
	 */
	if (fam->fam_mask == 0) {
		static const npf_addr_t zero; /* must be static */

		if (memcmp(&fam->fam_addr, &zero, sizeof(npf_addr_t))) {
			yyerror("filter criterion would never match");
		}
		return false;
	}

	npfctl_bpf_cidr(ctx, opts, family, &fam->fam_addr, fam->fam_mask);
	return true;
}

static void
npfctl_build_vars(npf_bpf_t *ctx, sa_family_t family, npfvar_t *vars, int opts)
{
	const int type = npfvar_get_type(vars, 0);
	size_t i;

	npfctl_bpf_group(ctx);
	for (i = 0; i < npfvar_get_count(vars); i++) {
		void *data = npfvar_get_data(vars, type, i);
		assert(data != NULL);

		switch (type) {
		case NPFVAR_FAM: {
			fam_addr_mask_t *fam = data;
			npfctl_build_fam(ctx, family, fam, opts);
			break;
		}
		case NPFVAR_PORT_RANGE: {
			port_range_t *pr = data;
			npfctl_bpf_ports(ctx, opts, pr->pr_start, pr->pr_end);
			break;
		}
		case NPFVAR_TABLE: {
			u_int tid;
			memcpy(&tid, data, sizeof(u_int));
			npfctl_bpf_table(ctx, opts, tid);
			break;
		}
		default:
			assert(false);
		}
	}
	npfctl_bpf_endgroup(ctx);
}

static void
npfctl_build_proto(npf_bpf_t *ctx, sa_family_t family, const opt_proto_t *op)
{
	const npfvar_t *popts = op->op_opts;
	const int proto = op->op_proto;

	/* IP version and/or L4 protocol matching. */
	if (family != AF_UNSPEC || proto != -1) {
		npfctl_bpf_proto(ctx, family, proto);
	}

	switch (proto) {
	case IPPROTO_TCP:
		/* Build TCP flags matching (optional). */
		if (popts) {
			uint8_t *tf, *tf_mask;

			assert(npfvar_get_count(popts) == 2);
			tf = npfvar_get_data(popts, NPFVAR_TCPFLAG, 0);
			tf_mask = npfvar_get_data(popts, NPFVAR_TCPFLAG, 1);
			npfctl_bpf_tcpfl(ctx, *tf, *tf_mask, false);
		}
		break;
	case IPPROTO_ICMP:
	case IPPROTO_ICMPV6:
		/* Build ICMP/ICMPv6 type and/or code matching. */
		if (popts) {
			int *icmp_type, *icmp_code;

			assert(npfvar_get_count(popts) == 2);
			icmp_type = npfvar_get_data(popts, NPFVAR_ICMP, 0);
			icmp_code = npfvar_get_data(popts, NPFVAR_ICMP, 1);
			npfctl_bpf_icmp(ctx, *icmp_type, *icmp_code);
		}
		break;
	default:
		/* No options for other protocols. */
		break;
	}
}

static bool
npfctl_build_code(nl_rule_t *rl, sa_family_t family, const opt_proto_t *op,
    const filt_opts_t *fopts)
{
	bool noproto, noaddrs, noports, need_tcpudp = false;
	const addr_port_t *apfrom = &fopts->fo_from;
	const addr_port_t *apto = &fopts->fo_to;
	const int proto = op->op_proto;
	npf_bpf_t *bc;
	size_t len;

	/* If none specified, then no byte-code. */
	noproto = family == AF_UNSPEC && proto == -1 && !op->op_opts;
	noaddrs = !apfrom->ap_netaddr && !apto->ap_netaddr;
	noports = !apfrom->ap_portrange && !apto->ap_portrange;
	if (noproto && noaddrs && noports) {
		return false;
	}

	/*
	 * Sanity check: ports can only be used with TCP or UDP protocol.
	 * No filter options are supported for other protocols, only the
	 * IP addresses are allowed.
	 */
	if (!noports) {
		switch (proto) {
		case IPPROTO_TCP:
		case IPPROTO_UDP:
			break;
		case -1:
			need_tcpudp = true;
			break;
		default:
			yyerror("invalid filter options for protocol %d", proto);
		}
	}

	bc = npfctl_bpf_create();

	/* Build layer 4 protocol blocks. */
	npfctl_build_proto(bc, family, op);

	/*
	 * If this is a stateful rule and TCP flags are not specified,
	 * then add "flags S/SAFR" filter for TCP protocol case.
	 */
	if ((npf_rule_getattr(rl) & NPF_RULE_STATEFUL) != 0 &&
	    (proto == -1 || (proto == IPPROTO_TCP && !op->op_opts))) {
		npfctl_bpf_tcpfl(bc, TH_SYN,
		    TH_SYN | TH_ACK | TH_FIN | TH_RST, proto == -1);
	}

	/* Build IP address blocks. */
	npfctl_build_vars(bc, family, apfrom->ap_netaddr, MATCH_SRC);
	npfctl_build_vars(bc, family, apto->ap_netaddr, MATCH_DST);

	/* Build port-range blocks. */
	if (need_tcpudp) {
		/* TCP/UDP check for the ports. */
		npfctl_bpf_group(bc);
		npfctl_bpf_proto(bc, AF_UNSPEC, IPPROTO_TCP);
		npfctl_bpf_proto(bc, AF_UNSPEC, IPPROTO_UDP);
		npfctl_bpf_endgroup(bc);
	}
	npfctl_build_vars(bc, family, apfrom->ap_portrange, MATCH_SRC);
	npfctl_build_vars(bc, family, apto->ap_portrange, MATCH_DST);

	/* Set the byte-code marks, if any. */
	const void *bmarks = npfctl_bpf_bmarks(bc, &len);
	if (npf_rule_setinfo(rl, bmarks, len) == -1) {
		errx(EXIT_FAILURE, "npf_rule_setinfo failed");
	}

	/* Complete BPF byte-code and pass to the rule. */
	struct bpf_program *bf = npfctl_bpf_complete(bc);
	len = bf->bf_len * sizeof(struct bpf_insn);

	if (npf_rule_setcode(rl, NPF_CODE_BPF, bf->bf_insns, len) == -1) {
		errx(EXIT_FAILURE, "npf_rule_setcode failed");
	}
	npfctl_dump_bpf(bf);
	npfctl_bpf_destroy(bc);

	return true;
}

static void
npfctl_build_pcap(nl_rule_t *rl, const char *filter)
{
	const size_t maxsnaplen = 64 * 1024;
	struct bpf_program bf;
	size_t len;

	if (pcap_compile_nopcap(maxsnaplen, DLT_RAW, &bf,
	    filter, 1, PCAP_NETMASK_UNKNOWN) == -1) {
		yyerror("invalid pcap-filter(7) syntax");
	}
	len = bf.bf_len * sizeof(struct bpf_insn);

	if (npf_rule_setcode(rl, NPF_CODE_BPF, bf.bf_insns, len) == -1) {
		errx(EXIT_FAILURE, "npf_rule_setcode failed");
	}
	npfctl_dump_bpf(&bf);
	pcap_freecode(&bf);
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
		errx(EXIT_FAILURE, "%s failed", __func__);
	}
	npf_rproc_insert(npf_conf, rp);

	for (i = 0; i < npfvar_get_count(procs); i++) {
		proc_call_t *pc = npfvar_get_data(procs, NPFVAR_PROC, i);
		npfctl_build_rpcall(rp, pc->pc_name, pc->pc_opts);
	}
}

void
npfctl_build_maprset(const char *name, int attr, const char *ifname)
{
	const int attr_di = (NPF_RULE_IN | NPF_RULE_OUT);
	nl_rule_t *rl;

	/* If no direction is not specified, then both. */
	if ((attr & attr_di) == 0) {
		attr |= attr_di;
	}
	/* Allow only "in/out" attributes. */
	attr = NPF_RULE_GROUP | NPF_RULE_GROUP | (attr & attr_di);
	rl = npf_rule_create(name, attr, ifname);
	npf_nat_insert(npf_conf, rl, NPF_PRI_LAST);
}

/*
 * npfctl_build_group: create a group, insert into the global ruleset,
 * update the current group pointer and increase the nesting level.
 */
void
npfctl_build_group(const char *name, int attr, const char *ifname, bool def)
{
	const int attr_di = (NPF_RULE_IN | NPF_RULE_OUT);
	nl_rule_t *rl;

	if (def || (attr & attr_di) == 0) {
		attr |= attr_di;
	}

	rl = npf_rule_create(name, attr | NPF_RULE_GROUP, ifname);
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
 * npfctl_build_rule: create a rule, build byte-code from filter options,
 * if any, and insert into the ruleset of current group, or set the rule.
 */
void
npfctl_build_rule(uint32_t attr, const char *ifname, sa_family_t family,
    const opt_proto_t *op, const filt_opts_t *fopts,
    const char *pcap_filter, const char *rproc)
{
	nl_rule_t *rl;

	attr |= (npf_conf ? 0 : NPF_RULE_DYNAMIC);

	rl = npf_rule_create(NULL, attr, ifname);
	if (pcap_filter) {
		npfctl_build_pcap(rl, pcap_filter);
	} else {
		npfctl_build_code(rl, family, op, fopts);
	}

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
static nl_nat_t *
npfctl_build_nat(int type, const char *ifname, const addr_port_t *ap,
    const filt_opts_t *fopts, u_int flags)
{
	const opt_proto_t op = { .op_proto = -1, .op_opts = NULL };
	fam_addr_mask_t *am = npfctl_get_singlefam(ap->ap_netaddr);
	in_port_t port;
	nl_nat_t *nat;

	if (ap->ap_portrange) {
		port = npfctl_get_singleport(ap->ap_portrange);
		flags &= ~NPF_NAT_PORTMAP;
		flags |= NPF_NAT_PORTS;
	} else {
		port = 0;
	}

	nat = npf_nat_create(type, flags, ifname, am->fam_family,
	    &am->fam_addr, am->fam_mask, port);
	npfctl_build_code(nat, am->fam_family, &op, fopts);
	npf_nat_insert(npf_conf, nat, NPF_PRI_LAST);
	return nat;
}

/*
 * npfctl_build_natseg: validate and create NAT policies.
 */
void
npfctl_build_natseg(int sd, int type, const char *ifname,
    const addr_port_t *ap1, const addr_port_t *ap2,
    const filt_opts_t *fopts, u_int algo)
{
	fam_addr_mask_t *am1 = NULL, *am2 = NULL;
	nl_nat_t *nt1 = NULL, *nt2 = NULL;
	filt_opts_t imfopts;
	uint16_t adj = 0;
	u_int flags;
	bool binat;

	assert(ifname != NULL);

	/*
	 * Bi-directional NAT is a combination of inbound NAT and outbound
	 * NAT policies with the translation segments inverted respectively.
	 */
	binat = (NPF_NATIN | NPF_NATOUT) == type;

	switch (sd) {
	case NPFCTL_NAT_DYNAMIC:
		/*
		 * Dynamic NAT: traditional NAPT is expected.  Unless it
		 * is bi-directional NAT, perform port mapping.
		 */
		flags = !binat ? (NPF_NAT_PORTS | NPF_NAT_PORTMAP) : 0;
		break;
	case NPFCTL_NAT_STATIC:
		/* Static NAT: mechanic translation. */
		flags = NPF_NAT_STATIC;
		break;
	default:
		abort();
	}

	/*
	 * Validate the mappings and their configuration.
	 */

	if ((type & NPF_NATIN) != 0) {
		if (!ap1->ap_netaddr)
			yyerror("inbound network segment is not specified");
		am1 = npfctl_get_singlefam(ap1->ap_netaddr);
	}
	if ((type & NPF_NATOUT) != 0) {
		if (!ap2->ap_netaddr)
			yyerror("outbound network segment is not specified");
		am2 = npfctl_get_singlefam(ap2->ap_netaddr);
	}

	switch (algo) {
	case NPF_ALGO_NPT66:
		if (am1 == NULL || am2 == NULL)
			yyerror("1:1 mapping of two segments must be "
			    "used for NPTv6");
		if (am1->fam_mask != am2->fam_mask)
			yyerror("asymmetric translation is not supported");
		adj = npfctl_npt66_calcadj(am1->fam_mask,
		    &am1->fam_addr, &am2->fam_addr);
		break;
	default:
		if ((am1 && am1->fam_mask != NPF_NO_NETMASK) ||
		    (am2 && am2->fam_mask != NPF_NO_NETMASK))
			yyerror("net-to-net translation is not supported");
		break;
	}

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
		nt1 = npfctl_build_nat(NPF_NATIN, ifname, ap1, fopts, flags);
	}
	if (type & NPF_NATOUT) {
		memset(&imfopts, 0, sizeof(filt_opts_t));
		memcpy(&imfopts.fo_from, ap1, sizeof(addr_port_t));
		nt2 = npfctl_build_nat(NPF_NATOUT, ifname, ap2, fopts, flags);
	}

	if (algo == NPF_ALGO_NPT66) {
		npf_nat_setnpt66(nt1, ~adj);
		npf_nat_setnpt66(nt2, adj);
	}
}

/*
 * npfctl_fill_table: fill NPF table with entries from a specified file.
 */
static void
npfctl_fill_table(nl_table_t *tl, u_int type, const char *fname)
{
	struct cdbw *cdbw = NULL;	/* XXX: gcc */
	char *buf = NULL;
	int l = 0;
	FILE *fp;
	size_t n;

	if (type == NPF_TABLE_CDB && (cdbw = cdbw_open()) == NULL) {
		err(EXIT_FAILURE, "cdbw_open");
	}
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
		if (type != NPF_TABLE_TREE && fam.fam_mask != NPF_NO_NETMASK) {
			errx(EXIT_FAILURE, "%s:%d: mask used with the "
			    "non-tree table", fname, l);
		}

		/*
		 * Create and add a table entry.
		 */
		if (type == NPF_TABLE_CDB) {
			const npf_addr_t *addr = &fam.fam_addr;
			if (cdbw_put(cdbw, addr, alen, addr, alen) == -1) {
				err(EXIT_FAILURE, "cdbw_put");
			}
		} else {
			npf_table_add_entry(tl, fam.fam_family,
			    &fam.fam_addr, fam.fam_mask);
		}
	}
	if (buf != NULL) {
		free(buf);
	}

	if (type == NPF_TABLE_CDB) {
		struct stat sb;
		char sfn[32];
		void *cdb;
		int fd;

		strlcpy(sfn, "/tmp/npfcdb.XXXXXX", sizeof(sfn));
		if ((fd = mkstemp(sfn)) == -1) {
			err(EXIT_FAILURE, "mkstemp");
		}
		unlink(sfn);

		if (cdbw_output(cdbw, fd, "npf-table-cdb", NULL) == -1) {
			err(EXIT_FAILURE, "cdbw_output");
		}
		cdbw_close(cdbw);

		if (fstat(fd, &sb) == -1) {
			err(EXIT_FAILURE, "fstat");
		}
		if ((cdb = mmap(NULL, sb.st_size, PROT_READ,
		    MAP_FILE | MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
			err(EXIT_FAILURE, "mmap");
		}
		npf_table_setdata(tl, cdb, sb.st_size);

		close(fd);
	}
}

/*
 * npfctl_build_table: create an NPF table, add to the configuration and,
 * if required, fill with contents from a file.
 */
void
npfctl_build_table(const char *tname, u_int type, const char *fname)
{
	static unsigned tid = 0;
	nl_table_t *tl;

	tl = npf_table_create(tname, tid++, type);
	assert(tl != NULL);

	if (npf_table_insert(npf_conf, tl)) {
		yyerror("table '%s' is already defined", tname);
	}

	if (fname) {
		npfctl_fill_table(tl, type, fname);
	} else if (type == NPF_TABLE_CDB) {
		errx(EXIT_FAILURE, "tables of cdb type must be static");
	}
}

/*
 * npfctl_build_alg: create an NPF application level gateway and add it
 * to the configuration.
 */
void
npfctl_build_alg(const char *al_name)
{
	if (_npf_alg_load(npf_conf, al_name) != 0) {
		errx(EXIT_FAILURE, "ALG '%s' already loaded", al_name);
	}
}

static void
npfctl_dump_bpf(struct bpf_program *bf)
{
	if (npf_debug) {
		extern char *yytext;
		extern int yylineno;

		int rule_line = yylineno - (int)(*yytext == '\n');
		printf("\nRULE AT LINE %d\n", rule_line);
		bpf_dump(bf, 0);
	}
}
