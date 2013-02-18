/*	$NetBSD: npf_disassemble.c,v 1.3.2.12 2013/02/18 18:26:14 riz Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 * NPF n-code disassembler.
 *
 * FIXME: config generation should be redesigned..
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: npf_disassemble.c,v 1.3.2.12 2013/02/18 18:26:14 riz Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>

#define NPF_OPCODES_STRINGS
#include <net/npf_ncode.h>

#include "npfctl.h"

enum {
	NPF_SHOW_SRCADDR,
	NPF_SHOW_DSTADDR,
	NPF_SHOW_SRCPORT,
	NPF_SHOW_DSTPORT,
	NPF_SHOW_PROTO,
	NPF_SHOW_FAMILY,
	NPF_SHOW_ICMP,
	NPF_SHOW_TCPF,
	NPF_SHOW_COUNT,
};

struct nc_inf {
	FILE *			ni_fp;
	const uint32_t *	ni_buf;
	size_t			ni_left;
	const uint32_t *	ni_ipc;
	const uint32_t *	ni_pc;

	/* Jump target array, its size and current index. */
	const uint32_t **	ni_targs;
	size_t			ni_targsize;
	size_t			ni_targidx;

	/* Other meta-data. */
	npfvar_t *		ni_vlist[NPF_SHOW_COUNT];
	int			ni_proto;
	bool			ni_srcdst;
};

static size_t
npfctl_ncode_get_target(const nc_inf_t *ni, const uint32_t *pc)
{
	for (size_t i = 0; i < ni->ni_targidx; i++) {
		if (ni->ni_targs[i] == pc)
			return i;
	}
	return (size_t)-1;
}

static size_t
npfctl_ncode_add_target(nc_inf_t *ni, const uint32_t *pc)
{
	size_t i = npfctl_ncode_get_target(ni, pc);

	/* If found, just return the index. */
	if (i != (size_t)-1) {
		return i;
	}

	/* Grow array, if needed, and add a new target. */
	if (ni->ni_targidx == ni->ni_targsize) {
		ni->ni_targsize += 16;
		ni->ni_targs = erealloc(ni->ni_targs,
		    ni->ni_targsize * sizeof(uint32_t));
	}
	assert(ni->ni_targidx < ni->ni_targsize);
	i = ni->ni_targidx++;
	ni->ni_targs[i] = pc;
	return i;
}

static void
npfctl_ncode_add_vp(nc_inf_t *ni, char *buf, unsigned idx)
{
	npfvar_t *vl = ni->ni_vlist[idx];

	if (vl == NULL) {
		vl = npfvar_create(".list");
		ni->ni_vlist[idx] = vl;
	}
	npfvar_t *vp = npfvar_create(".string");
	npfvar_add_element(vp, NPFVAR_STRING, buf, strlen(buf) + 1);
	npfvar_add_elements(vl, vp);
}

static void
npf_tcpflags2str(char *buf, unsigned tfl)
{
	int i = 0;

	if (tfl & TH_FIN)	buf[i++] = 'F';
	if (tfl & TH_SYN)	buf[i++] = 'S';
	if (tfl & TH_RST)	buf[i++] = 'R';
	if (tfl & TH_PUSH)	buf[i++] = 'P';
	if (tfl & TH_ACK)	buf[i++] = 'A';
	if (tfl & TH_URG)	buf[i++] = 'U';
	if (tfl & TH_ECE)	buf[i++] = 'E';
	if (tfl & TH_CWR)	buf[i++] = 'C';
	buf[i] = '\0';
}

static const char *
npfctl_ncode_operand(nc_inf_t *ni, char *buf, size_t bufsiz, uint8_t operand)
{
	const uint32_t op = *ni->ni_pc;
	struct sockaddr_storage ss;
	unsigned advance;

	/* Advance by one is a default for most cases. */
	advance = 1;

	switch (operand) {
	case NPF_OPERAND_NONE:
		abort();

	case NPF_OPERAND_REGISTER:
		if (op & ~0x3) {
			warnx("invalid register operand 0x%x at offset %td",
			    op, ni->ni_pc - ni->ni_buf);
			return NULL;
		}
		snprintf(buf, bufsiz, "R%d", op);
		break;

	case NPF_OPERAND_KEY:
		snprintf(buf, bufsiz, "key=<0x%x>", op);
		break;

	case NPF_OPERAND_VALUE:
		snprintf(buf, bufsiz, "value=<0x%x>", op);
		break;

	case NPF_OPERAND_SD:
		if (op & ~0x1) {
			warnx("invalid src/dst operand 0x%x at offset %td",
			    op, ni->ni_pc - ni->ni_buf);
			return NULL;
		}
		bool srcdst = (op == NPF_OPERAND_SD_SRC);
		if (ni) {
			ni->ni_srcdst = srcdst;
		}
		snprintf(buf, bufsiz, "%s", srcdst ? "SRC" : "DST");
		break;

	case NPF_OPERAND_REL_ADDRESS:
		snprintf(buf, bufsiz, "L%zu",
		    npfctl_ncode_add_target(ni, ni->ni_ipc + op));
		break;

	case NPF_OPERAND_NET_ADDRESS4: {
		struct sockaddr_in *sin = (void *)&ss;
		sin->sin_len = sizeof(*sin);
		sin->sin_family = AF_INET;
		sin->sin_port = 0;
		memcpy(&sin->sin_addr, ni->ni_pc, sizeof(sin->sin_addr));
		sockaddr_snprintf(buf, bufsiz, "%a", (struct sockaddr *)sin);
		if (ni) {
			npfctl_ncode_add_vp(ni, buf, ni->ni_srcdst ?
			    NPF_SHOW_SRCADDR : NPF_SHOW_DSTADDR);
		}
		advance = sizeof(sin->sin_addr) / sizeof(op);
		break;
	}
	case NPF_OPERAND_NET_ADDRESS6: {
		struct sockaddr_in6 *sin6 = (void *)&ss;
		sin6->sin6_len = sizeof(*sin6);
		sin6->sin6_family = AF_INET6;
		sin6->sin6_port = 0;
		sin6->sin6_scope_id = 0;
		memcpy(&sin6->sin6_addr, ni->ni_pc, sizeof(sin6->sin6_addr));
		sockaddr_snprintf(buf, bufsiz, "%a", (struct sockaddr *)sin6);
		if (ni) {
			npfctl_ncode_add_vp(ni, buf, ni->ni_srcdst ?
			    NPF_SHOW_SRCADDR : NPF_SHOW_DSTADDR);
		}
		advance = sizeof(sin6->sin6_addr) / sizeof(op);
		break;
	}
	case NPF_OPERAND_ETHER_TYPE:
		snprintf(buf, bufsiz, "ether=0x%x", op);
		break;

	case NPF_OPERAND_PROTO: {
		uint8_t addrlen = (op >> 8) & 0xff;
		uint8_t proto = op & 0xff;

		snprintf(buf, bufsiz, "addrlen=%u, proto=%u", addrlen, proto);
		if (!ni) {
			break;
		}
		switch (proto) {
		case 0xff:
			/* None. */
			break;
		case IPPROTO_TCP:
			ni->ni_proto |= NC_MATCH_TCP;
			break;
		case IPPROTO_UDP:
			ni->ni_proto |= NC_MATCH_UDP;
			break;
		case IPPROTO_ICMP:
			ni->ni_proto |= NC_MATCH_ICMP;
			/* FALLTHROUGH */
		default:
			snprintf(buf, bufsiz, "proto %d", proto);
			npfctl_ncode_add_vp(ni, buf, NPF_SHOW_PROTO);
			break;
		}
		switch (addrlen) {
		case 4:
		case 16:
			snprintf(buf, bufsiz, "family inet%s",
			    addrlen == 16 ? "6" : "");
			npfctl_ncode_add_vp(ni, buf, NPF_SHOW_FAMILY);
			break;
		}
		break;
	}
	case NPF_OPERAND_SUBNET: {
		snprintf(buf, bufsiz, "/%d", op);
		if (ni && op != NPF_NO_NETMASK) {
			npfctl_ncode_add_vp(ni, buf, ni->ni_srcdst ?
			    NPF_SHOW_SRCADDR : NPF_SHOW_DSTADDR);
		}
		break;
	}
	case NPF_OPERAND_LENGTH:
		snprintf(buf, bufsiz, "length=%d", op);
		break;

	case NPF_OPERAND_TABLE_ID:
		if (ni) {
			snprintf(buf, bufsiz, "<%d>", op);
			npfctl_ncode_add_vp(ni, buf, ni->ni_srcdst ?
			    NPF_SHOW_SRCADDR : NPF_SHOW_DSTADDR);
		}
		snprintf(buf, bufsiz, "id=%d", op);
		break;

	case NPF_OPERAND_ICMP_TYPE_CODE: {
		uint8_t type = (op & 31) ? op >> 8 : 0;
		uint8_t code = (op & 30) ? op & 0xff : 0;

		if (op & ~0xc000ffff) {
			warnx("invalid icmp/type operand 0x%x at offset %td",
			    op, ni->ni_pc - ni->ni_buf);
			return NULL;
		}
		snprintf(buf, bufsiz, "type=%d, code=%d", type, code);
		if (!ni) {
			break;
		}
		ni->ni_proto |= NC_MATCH_ICMP;
		if (*ni->ni_ipc == NPF_OPCODE_ICMP6) {
			snprintf(buf, bufsiz, "proto \"ipv6-icmp\"");
			npfctl_ncode_add_vp(ni, buf, NPF_SHOW_PROTO);
		}
		if (type || code) {
			snprintf(buf, bufsiz,
			    "icmp-type %d code %d", type, code);
			npfctl_ncode_add_vp(ni, buf, NPF_SHOW_ICMP);
		}
		break;
	}
	case NPF_OPERAND_TCP_FLAGS_MASK: {
		uint8_t tf = op >> 8, tf_mask = op & 0xff;
		if (op & ~0xffff) {
			warnx("invalid flags/mask operand 0x%x at offset %td",
			    op, ni->ni_pc - ni->ni_buf);
			return NULL;
		}
		char tf_buf[16], tfm_buf[16];
		npf_tcpflags2str(tf_buf, tf);
		npf_tcpflags2str(tfm_buf, tf_mask);
		snprintf(buf, bufsiz, "flags %s/%s", tf_buf, tfm_buf);
		if (ni) {
			ni->ni_proto |= NC_MATCH_TCP;
			npfctl_ncode_add_vp(ni, buf, NPF_SHOW_TCPF);
		}
		break;
	}
	case NPF_OPERAND_PORT_RANGE: {
		in_port_t p1 = ntohs(op >> 16), p2 = ntohs(op & 0xffff);

		if (p1 == p2) {
			snprintf(buf, bufsiz, "%d", p1);
		} else {
			snprintf(buf, bufsiz, "%d-%d", p1, p2);
		}

		if (!ni) {
			break;
		}
		switch (*ni->ni_ipc) {
		case NPF_OPCODE_TCP_PORTS:
			ni->ni_proto |= NC_MATCH_TCP;
			break;
		case NPF_OPCODE_UDP_PORTS:
			ni->ni_proto |= NC_MATCH_UDP;
			break;
		}
		int sd = ni->ni_srcdst ?  NPF_SHOW_SRCPORT : NPF_SHOW_DSTPORT;
		if (ni->ni_vlist[sd]) {
			break;
		}
		npfctl_ncode_add_vp(ni, buf, sd);
		break;
	}
	default:
		warnx("invalid operand %d at offset %td",
		    operand, ni->ni_pc - ni->ni_buf);
		return NULL;
	}

	if (ni->ni_left < sizeof(op) * advance) {
		warnx("ran out of bytes");
		return NULL;
	}
	ni->ni_pc += advance;
	ni->ni_left -= sizeof(op) * advance;
	return buf;
}

nc_inf_t *
npfctl_ncode_disinf(FILE *fp)
{
	nc_inf_t *ni = ecalloc(1, sizeof(nc_inf_t));

	memset(ni, 0, sizeof(nc_inf_t));
	ni->ni_fp = fp;
	return ni;
}

int
npfctl_ncode_disassemble(nc_inf_t *ni, const void *v, size_t len)
{
	FILE *fp = ni->ni_fp;
	int error = -1;

	ni->ni_buf = v;
	ni->ni_left = len;
	ni->ni_pc = v;

	while (ni->ni_left) {
		const struct npf_instruction *insn;
		const uint32_t opcode = *ni->ni_pc;
		size_t target;

		/* Get the opcode. */
		if (opcode & ~0xff) {
			warnx("invalid opcode 0x%x at offset (%td)",
			    opcode, ni->ni_pc - ni->ni_buf);
			goto out;
		}
		insn = &npf_instructions[opcode];
		if (insn->name == NULL) {
			warnx("invalid opcode 0x%x at offset (%td)",
			    opcode, ni->ni_pc - ni->ni_buf);
			goto out;
		}

		/*
		 * Lookup target array and prefix with the label,
		 * if this opcode is a jump target.
		 */
		ni->ni_ipc = ni->ni_pc;
		target = npfctl_ncode_get_target(ni, ni->ni_pc);
		if (fp) {
			if (target != (size_t)-1) {
				fprintf(fp, "L%zu:", target);
			}
			fprintf(fp, "\t%s", insn->name);
		}
		if (ni->ni_left < sizeof(opcode)) {
			warnx("ran out of bytes");
			return -1;
		}
		ni->ni_left -= sizeof(opcode);
		ni->ni_pc++;
		for (size_t i = 0; i < __arraycount(insn->op); i++) {
			const uint8_t o = insn->op[i];
			const char *op;
			char buf[256];

			if (o == NPF_OPERAND_NONE) {
				break;
			}
			op = npfctl_ncode_operand(ni, buf, sizeof(buf), o);
			if (op == NULL) {
				goto out;
			}
			if (fp) {
				fprintf(fp, "%s%s", i == 0 ? " " : ", ", op);
			}
		}
		if (fp) {
			fprintf(fp, "\n");
		}
	}
	error = 0;
out:
	free(ni->ni_targs);
	return error;
}

static void
npfctl_show_fromto(const char *name, npfvar_t *vl, bool showany)
{
	size_t count = npfvar_get_count(vl);
	char *s;

	switch (count) {
	case 0:
		if (showany) {
			printf("%s any ", name);
		}
		return;
	case 1:
		s = npfvar_get_data(vl, NPFVAR_STRING, 0);
		printf("%s %s ", name, s);
		return;
	}
	printf("%s%s", name, " { ");
	for (size_t i = 0; i < count; i++) {
		s = npfvar_get_data(vl, NPFVAR_STRING, i);
		printf("%s%s", (i && s[0] != '/') ? ", " : "", s);
	}
	printf(" } ");
	npfvar_destroy(vl);
}

static bool
npfctl_show_ncode(const void *nc, size_t len)
{
	nc_inf_t *ni = npfctl_ncode_disinf(NULL);
	bool any, protoshown = false;
	npfvar_t *vl;

	if (npfctl_ncode_disassemble(ni, nc, len) != 0) {
		printf("<< ncode >> ");
		return true;
	}

	if ((vl = ni->ni_vlist[NPF_SHOW_FAMILY]) != NULL) {
		printf("%s ", npfvar_expand_string(vl));
		npfvar_destroy(vl);
	}

	if ((vl = ni->ni_vlist[NPF_SHOW_PROTO]) != NULL) {
		printf("%s ", npfvar_expand_string(vl));
		npfvar_destroy(vl);
		protoshown = true;
	}

	switch (ni->ni_proto) {
	case NC_MATCH_TCP:
		if (!protoshown) {
			printf("proto tcp ");
		}
		if ((vl = ni->ni_vlist[NPF_SHOW_TCPF]) != NULL) {
			printf("%s ", npfvar_expand_string(vl));
			npfvar_destroy(vl);
		}
		break;
	case NC_MATCH_ICMP:
		if (!protoshown) {
			printf("proto icmp ");
		}
		if ((vl = ni->ni_vlist[NPF_SHOW_ICMP]) != NULL) {
			printf("%s ", npfvar_expand_string(vl));
			npfvar_destroy(vl);
		}
		break;
	case NC_MATCH_UDP:
		if (!protoshown) {
			printf("proto udp ");
		}
		break;
	default:
		break;
	}

	any = false;
	if (ni->ni_vlist[NPF_SHOW_SRCADDR] || ni->ni_vlist[NPF_SHOW_SRCPORT]) {
		npfctl_show_fromto("from", ni->ni_vlist[NPF_SHOW_SRCADDR], true);
		npfctl_show_fromto("port", ni->ni_vlist[NPF_SHOW_SRCPORT], false);
		any = true;
	}
	if (ni->ni_vlist[NPF_SHOW_DSTADDR] || ni->ni_vlist[NPF_SHOW_DSTPORT]) {
		npfctl_show_fromto("to", ni->ni_vlist[NPF_SHOW_DSTADDR], true);
		npfctl_show_fromto("port", ni->ni_vlist[NPF_SHOW_DSTPORT], false);
		any = true;
	}

	free(ni);
	return any;
}

#define	NPF_RSTICMP		(NPF_RULE_RETRST | NPF_RULE_RETICMP)

static const struct attr_keyword_mapent {
	uint32_t	mask;
	uint32_t	flags;
	const char *	onmatch;
	const char *	nomatch;
} attr_keyword_map[] = {
	{ NPF_RULE_PASS,	NPF_RULE_PASS,	"pass",		"block"	},
	{ NPF_RSTICMP,		NPF_RSTICMP,	"return",	NULL	},
	{ NPF_RSTICMP,		NPF_RULE_RETRST,"return-rst",	NULL	},
	{ NPF_RSTICMP,		NPF_RULE_RETICMP,"return-icmp",	NULL	},
	{ NPF_RULE_STATEFUL,	NPF_RULE_STATEFUL,"stateful",	NULL	},
	{ NPF_RULE_DIMASK,	NPF_RULE_IN,	"in",		NULL	},
	{ NPF_RULE_DIMASK,	NPF_RULE_OUT,	"out",		NULL	},
	{ NPF_RULE_FINAL,	NPF_RULE_FINAL,	"final",	NULL	},
};

static int rules_seen = 0;

/*
 * FIXME: This mess needs a complete rewrite..
 */

static void
npfctl_show_rule(nl_rule_t *nrl, unsigned nlevel)
{
	static int grouplvl = -1;
	rule_group_t rg;
	const char *rproc;
	const void *nc;
	size_t nclen;
	u_int n;

	memset(&rg, 0, sizeof(rg));
	_npf_rule_getinfo(nrl, &rg.rg_name, &rg.rg_attr, &rg.rg_ifnum);
	rules_seen++;

	/* Get the interface, if any. */
	char ifnamebuf[IFNAMSIZ], *ifname = NULL;
	if (rg.rg_ifnum) {
		ifname = if_indextoname(rg.rg_ifnum, ifnamebuf);
	}

	if (grouplvl >= 0 && (unsigned)grouplvl >= nlevel) {
		for (n = 0; n < nlevel; n++) {
			printf("\t");
		}
		printf("}\n\n");
		grouplvl--;
	}
	for (n = 0; n < nlevel; n++) {
		printf("\t");
	}

	if (rg.rg_attr & NPF_RULE_GROUP) {
		const char *rname = rg.rg_name;

		grouplvl = nlevel;
		if (rg.rg_attr == (NPF_RULE_GROUP| NPF_RULE_IN | NPF_RULE_OUT)
		    && rname == NULL && rg.rg_ifnum == 0) {
			puts("group (default) {");
			return;
		}
		printf("group (name \"%s\"", rname == NULL ? "" : rname);
		if (ifname) {
			printf(", interface %s", ifname);
		}
		if (rg.rg_attr & NPF_RULE_DYNAMIC) {
			printf(", dynamic");
		}
		puts(") {");
		return;
	}

	/*
	 * Rule case.  First, unparse the attributes.
	 */
	for (unsigned i = 0; i < __arraycount(attr_keyword_map); i++) {
		const struct attr_keyword_mapent *ak = &attr_keyword_map[i];

		if ((rg.rg_attr & ak->mask) == ak->flags) {
			printf("%s ", ak->onmatch);
		} else if (ak->nomatch) {
			printf("%s ", ak->nomatch);
		}
	}

	if (ifname) {
		printf("on %s ", ifname);
	}

	nc = _npf_rule_ncode(nrl, &nclen);
	if (!nc || !npfctl_show_ncode(nc, nclen)) {
		printf("all ");
	}

	if ((rproc = _npf_rule_rproc(nrl)) != NULL) {
		printf("apply \"%s\"", rproc);
	}
	puts("");
}

static void
npfctl_show_table(unsigned id, int type)
{
	printf("table <%u> type %s\n", id,
		(type == NPF_TABLE_HASH) ? "hash" :
		(type == NPF_TABLE_TREE) ? "tree" :
		"unknown"
	);
}

static void
npfctl_show_nat(nl_rule_t *nrl, unsigned nlevel)
{
	rule_group_t rg;
	nl_nat_t *nt = nrl;
	npf_addr_t taddr;
	in_port_t port;
	size_t alen;
	u_int flags;
	int type;

	/* TODO: bi-NAT */

	_npf_rule_getinfo(nrl, &rg.rg_name, &rg.rg_attr, &rg.rg_ifnum);

	/* Get the interface, if any. */
	char ifnamebuf[IFNAMSIZ], *ifname = NULL;
	if (rg.rg_ifnum) {
		ifname = if_indextoname(rg.rg_ifnum, ifnamebuf);
	}
	_npf_nat_getinfo(nt, &type, &flags, &taddr, &alen, &port);

	char *taddrbuf, tportbuf[16];

	taddrbuf = npfctl_print_addrmask(alen, &taddr, 0);
	if (port) {
		snprintf(tportbuf, sizeof(tportbuf), " port %d", ntohs(port));
	}

	const char *seg1 = "any", *seg2 = "any", *sp1 = "", *sp2 = "", *mt;
	switch (type) {
	case NPF_NATIN:
		mt = "<-";
		seg1 = taddrbuf;
		sp1 = port ? tportbuf : "";
		break;
	case NPF_NATOUT:
		mt = "->";
		seg2 = taddrbuf;
		sp2 = port ? tportbuf : "";
		break;
	default:
		assert(false);
	}
	printf("map %s dynamic %s%s %s %s%s pass ", ifname,
	    seg1, sp1, mt, seg2, sp2);
	free(taddrbuf);

	const void *nc;
	size_t nclen;

	nc = _npf_rule_ncode(nrl, &nclen);
	printf("%s\n", (!nc || !npfctl_show_ncode(nc, nclen)) ? " any " : "");
}

int
npfctl_config_show(int fd)
{
	nl_config_t *ncf;
	bool active, loaded;
	int error = 0;

	if (fd) {
		ncf = npf_config_retrieve(fd, &active, &loaded);
		if (ncf == NULL) {
			return errno;
		}
		printf("Filtering:\t%s\nConfiguration:\t%s\n\n",
		    active ? "active" : "inactive",
		    loaded ? "loaded" : "empty");
	} else {
		ncf = npfctl_config_ref();
		loaded = true;
	}

	if (loaded) {
		_npf_table_foreach(ncf, npfctl_show_table);
		puts("");
		error = _npf_nat_foreach(ncf, npfctl_show_nat);
		puts("");
		if (!error) {
			error = _npf_rule_foreach(ncf, npfctl_show_rule);
			if (rules_seen)
				puts("}");
		}
	}
	npf_config_destroy(ncf);
	return error;
}

int
npfctl_ruleset_show(int fd, const char *ruleset_name)
{
	nl_config_t *ncf;
	int error;

	ncf = npf_config_create();
	if ((error = _npf_ruleset_list(fd, ruleset_name, ncf)) != 0) {
		return error;
	}
	error = _npf_rule_foreach(ncf, npfctl_show_rule);
	npf_config_destroy(ncf);
	return error;
}
