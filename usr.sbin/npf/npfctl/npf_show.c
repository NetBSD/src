/*	$NetBSD: npf_show.c,v 1.13.2.1 2014/08/10 07:00:01 tls Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
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
 * NPF configuration printing.
 *
 * Each rule having BPF byte-code has a binary description.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: npf_show.c,v 1.13.2.1 2014/08/10 07:00:01 tls Exp $");

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>
#include <err.h>

#include "npfctl.h"

typedef struct {
	nl_config_t *	conf;
	FILE *		fp;
	long		fpos;
} npf_conf_info_t;

static npf_conf_info_t	stdout_ctx = { .fp = stdout, .fpos = 0 };

static void	print_indent(npf_conf_info_t *, u_int);
static void	print_linesep(npf_conf_info_t *);

/*
 * Helper routines to print various pieces of information.
 */

static void
print_indent(npf_conf_info_t *ctx, u_int level)
{
	if (level == 0) { /* XXX */
		print_linesep(ctx);
	}
	while (level--)
		fprintf(ctx->fp, "\t");
}

static void
print_linesep(npf_conf_info_t *ctx)
{
	if (ftell(ctx->fp) != ctx->fpos) {
		fputs("\n", ctx->fp);
		ctx->fpos = ftell(ctx->fp);
	}
}

static size_t
tcpflags2string(char *buf, u_int tfl)
{
	u_int i = 0;

	if (tfl & TH_FIN)	buf[i++] = 'F';
	if (tfl & TH_SYN)	buf[i++] = 'S';
	if (tfl & TH_RST)	buf[i++] = 'R';
	if (tfl & TH_PUSH)	buf[i++] = 'P';
	if (tfl & TH_ACK)	buf[i++] = 'A';
	if (tfl & TH_URG)	buf[i++] = 'U';
	if (tfl & TH_ECE)	buf[i++] = 'E';
	if (tfl & TH_CWR)	buf[i++] = 'C';
	buf[i] = '\0';
	return i;
}

static char *
print_family(npf_conf_info_t *ctx, const uint32_t *words)
{
	const int af = words[0];

	switch (af) {
	case AF_INET:
		return estrdup("inet4");
	case AF_INET6:
		return estrdup("inet6");
	default:
		errx(EXIT_FAILURE, "invalid byte-code mark (family)");
	}
	return NULL;
}

static char *
print_address(npf_conf_info_t *ctx, const uint32_t *words)
{
	const int af = *words++;
	const u_int mask = *words++;
	const npf_addr_t *addr;
	int alen = 0;

	switch (af) {
	case AF_INET:
		alen = 4;
		break;
	case AF_INET6:
		alen = 16;
		break;
	default:
		errx(EXIT_FAILURE, "invalid byte-code mark (address)");
	}
	addr = (const npf_addr_t *)words;
	return npfctl_print_addrmask(alen, addr, mask);
}

static char *
print_number(npf_conf_info_t *ctx, const uint32_t *words)
{
	char *p;
	easprintf(&p, "%u", words[0]);
	return p;
}

static char *
print_table(npf_conf_info_t *ctx, const uint32_t *words)
{
	unsigned tid = words[0];
	nl_table_t *tl;
	char *p = NULL;

	/* XXX: Iterating all as we need to rewind for the next call. */
	while ((tl = npf_table_iterate(ctx->conf)) != NULL) {
		if (!p && npf_table_getid(tl) == tid) {
			easprintf(&p, "%s", npf_table_getname(tl));
		}
	}
	assert(p != NULL);
	return p;
}

static char *
print_proto(npf_conf_info_t *ctx, const uint32_t *words)
{
	switch (words[0]) {
	case IPPROTO_TCP:
		return estrdup("tcp");
	case IPPROTO_UDP:
		return estrdup("udp");
	case IPPROTO_ICMP:
		return estrdup("icmp");
	case IPPROTO_ICMPV6:
		return estrdup("ipv6-icmp");
	}
	return print_number(ctx, words);
}

static char *
print_tcpflags(npf_conf_info_t *ctx, const uint32_t *words)
{
	const u_int tf = words[0], tf_mask = words[1];
	char buf[16];

	size_t n = tcpflags2string(buf, tf);
	if (tf != tf_mask) {
		buf[n++] = '/';
		tcpflags2string(buf + n, tf_mask);
	}
	return estrdup(buf);
}

static char *
print_portrange(npf_conf_info_t *ctx, const uint32_t *words)
{
	u_int fport = words[0], tport = words[1];
	char *p;

	if (fport != tport) {
		easprintf(&p, "%u:%u", fport, tport);
	} else {
		easprintf(&p, "%u", fport);
	}
	return p;
}

/*
 * The main keyword mapping tables defining the syntax:
 * - Mapping of rule attributes (flags) to the keywords.
 * - Mapping of the byte-code marks to the keywords.
 */

#define	F(name)		__CONCAT(NPF_RULE_, name)
#define	STATEFUL_ENDS	(NPF_RULE_STATEFUL | NPF_RULE_MULTIENDS)
#define	NAME_AT		2

static const struct attr_keyword_mapent {
	uint32_t	mask;
	uint32_t	flags;
	const char *	val;
} attr_keyword_map[] = {
	{ F(GROUP)|F(DYNAMIC),	F(GROUP),		"group"		},
	{ F(DYNAMIC),		F(DYNAMIC),		"ruleset"	},
	{ F(GROUP)|F(PASS),	0,			"block"		},
	{ F(GROUP)|F(PASS),	F(PASS),		"pass"		},
	{ F(RETRST)|F(RETICMP),	F(RETRST)|F(RETICMP),	"return"	},
	{ F(RETRST)|F(RETICMP),	F(RETRST),		"return-rst"	},
	{ F(RETRST)|F(RETICMP),	F(RETICMP),		"return-icmp"	},
	{ STATEFUL_ENDS,	F(STATEFUL),		"stateful"	},
	{ STATEFUL_ENDS,	STATEFUL_ENDS,		"stateful-ends"	},
	{ F(DIMASK),		F(IN),			"in"		},
	{ F(DIMASK),		F(OUT),			"out"		},
	{ F(FINAL),		F(FINAL),		"final"		},
};

static const struct mark_keyword_mapent {
	u_int		mark;
	const char *	token;
	const char *	sep;
	char *		(*printfn)(npf_conf_info_t *, const uint32_t *);
	u_int		fwords;
} mark_keyword_map[] = {
	{ BM_IPVER,	"family %s",	NULL,		print_family,	1 },
	{ BM_PROTO,	"proto %s",	", ",		print_proto,	1 },
	{ BM_TCPFL,	"flags %s",	NULL,		print_tcpflags,	2 },
	{ BM_ICMP_TYPE,	"icmp-type %s",	NULL,		print_number,	1 },
	{ BM_ICMP_CODE,	"code %s",	NULL,		print_number,	1 },

	{ BM_SRC_CIDR,	"from %s",	", ",		print_address,	6 },
	{ BM_SRC_TABLE,	"from <%s>",	NULL,		print_table,	1 },
	{ BM_SRC_PORTS,	"port %s",	", ",		print_portrange,2 },

	{ BM_DST_CIDR,	"to %s",	", ",		print_address,	6 },
	{ BM_DST_TABLE,	"to <%s>",	NULL,		print_table,	1 },
	{ BM_DST_PORTS,	"port %s",	", ",		print_portrange,2 },
};

static const char * __attribute__((format_arg(2)))
verified_fmt(const char *fmt, const char *t __unused)
{
	return fmt;
}

static char *
scan_marks(npf_conf_info_t *ctx, const struct mark_keyword_mapent *mk,
    const uint32_t *marks, size_t mlen)
{
	char buf[2048], *vals[256], *p;
	size_t nvals = 0;

	/* Scan for the marks and extract the values. */
	mlen /= sizeof(uint32_t);
	while (mlen > 2) {
		const uint32_t m = *marks++;
		const u_int nwords = *marks++;

		if ((mlen -= 2) < nwords) {
			errx(EXIT_FAILURE, "byte-code marking inconsistency");
		}
		if (m == mk->mark) {
			/* Value is processed by the print function. */
			assert(mk->fwords == nwords);
			vals[nvals++] = mk->printfn(ctx, marks);
		}
		marks += nwords;
		mlen -= nwords;
	}
	if (nvals == 0) {
		return NULL;
	}
	assert(nvals == 1 || mk->sep != NULL);

	/*
	 * Join all the values and print.  Add curly brackets if there
	 * is more than value and it can be a set.
	 */
	if (!join(buf, sizeof(buf), nvals, vals, mk->sep ? mk->sep : "")) {
		errx(EXIT_FAILURE, "out of memory while parsing the rule");
	}
	easprintf(&p, nvals > 1 ? "{ %s }" : "%s", buf);

	for (u_int i = 0; i < nvals; i++) {
		free(vals[i]);
	}
	return p;
}

static void
npfctl_print_filter(npf_conf_info_t *ctx, nl_rule_t *rl)
{
	const void *marks;
	size_t mlen;

	/* BPF filter criteria described by the byte-code marks. */
	marks = npf_rule_getinfo(rl, &mlen);
	for (u_int i = 0; i < __arraycount(mark_keyword_map); i++) {
		const struct mark_keyword_mapent *mk = &mark_keyword_map[i];
		char *val;

		if ((val = scan_marks(ctx, mk, marks, mlen)) != NULL) {
			fprintf(ctx->fp, verified_fmt(mk->token, "%s"), val);
			fputs(" ", ctx->fp);
			free(val);
		}
	}
	if (!mlen) {
		fputs("all ", ctx->fp);
	}
}

static void
npfctl_print_rule(npf_conf_info_t *ctx, nl_rule_t *rl)
{
	const uint32_t attr = npf_rule_getattr(rl);
	const char *rproc, *ifname, *name;

	/* Rule attributes/flags. */
	for (u_int i = 0; i < __arraycount(attr_keyword_map); i++) {
		const struct attr_keyword_mapent *ak = &attr_keyword_map[i];

		if (i == NAME_AT && (name = npf_rule_getname(rl)) != NULL) {
			fprintf(ctx->fp, "\"%s\" ", name);
		}
		if ((attr & ak->mask) == ak->flags) {
			fprintf(ctx->fp, "%s ", ak->val);
		}
	}
	if ((ifname = npf_rule_getinterface(rl)) != NULL) {
		fprintf(ctx->fp, "on %s ", ifname);
	}

	if ((attr & (NPF_RULE_GROUP | NPF_RULE_DYNAMIC)) == NPF_RULE_GROUP) {
		/* Group; done. */
		fputs("\n", ctx->fp);
		return;
	}

	/* Print filter criteria. */
	npfctl_print_filter(ctx, rl);

	/* Rule procedure. */
	if ((rproc = npf_rule_getproc(rl)) != NULL) {
		fprintf(ctx->fp, "apply \"%s\"", rproc);
	}
	fputs("\n", ctx->fp);
}

static void
npfctl_print_nat(npf_conf_info_t *ctx, nl_nat_t *nt)
{
	nl_rule_t *rl = (nl_nat_t *)nt;
	const char *ifname, *seg1, *seg2, *arrow;
	npf_addr_t addr;
	in_port_t port;
	size_t alen;
	u_int flags;
	char *seg;

	/* Get the interface. */
	ifname = npf_rule_getinterface(rl);
	assert(ifname != NULL);

	/* Get the translation address (and port, if used). */
	npf_nat_getmap(nt, &addr, &alen, &port);
	seg = npfctl_print_addrmask(alen, &addr, NPF_NO_NETMASK);
	if (port) {
		char *p;
		easprintf(&p, "%s port %u", seg, ntohs(port));
		free(seg), seg = p;
	}
	seg1 = seg2 = "any";

	/* Get the NAT type and determine the translation segment. */
	switch (npf_nat_gettype(nt)) {
	case NPF_NATIN:
		arrow = "<-";
		seg1 = seg;
		break;
	case NPF_NATOUT:
		arrow = "->";
		seg2 = seg;
		break;
	default:
		abort();
	}
	flags = npf_nat_getflags(nt);

	/* Print out the NAT policy with the filter criteria. */
	fprintf(ctx->fp, "map %s %s %s %s %s pass ",
	    ifname, (flags & NPF_NAT_STATIC) ? "static" : "dynamic",
	    seg1, arrow, seg2);
	npfctl_print_filter(ctx, rl);
	fputs("\n", ctx->fp);
	free(seg);
}

static void
npfctl_print_table(npf_conf_info_t *ctx, nl_table_t *tl)
{
	const char *name = npf_table_getname(tl);
	const unsigned type = npf_table_gettype(tl);
	const char *table_types[] = {
		[NPF_TABLE_HASH] = "hash",
		[NPF_TABLE_TREE] = "tree",
		[NPF_TABLE_CDB]  = "cdb",
	};

	if (name[0] == '.') {
		/* Internal tables use dot and are hidden. */
		return;
	}
	assert(type < __arraycount(table_types));
	fprintf(ctx->fp, "table <%s> type %s\n", name, table_types[type]);
}

int
npfctl_config_show(int fd)
{
	npf_conf_info_t *ctx = &stdout_ctx;
	nl_config_t *ncf;
	bool active, loaded;

	if (fd) {
		ncf = npf_config_retrieve(fd, &active, &loaded);
		if (ncf == NULL) {
			return errno;
		}
		fprintf(ctx->fp, "# filtering:\t%s\n# config:\t%s\n",
		    active ? "active" : "inactive",
		    loaded ? "loaded" : "empty");
		print_linesep(ctx);
	} else {
		npfctl_config_send(0, NULL);
		ncf = npfctl_config_ref();
		loaded = true;
	}
	ctx->conf = ncf;

	if (loaded) {
		nl_rule_t *rl;
		nl_rproc_t *rp;
		nl_nat_t *nt;
		nl_table_t *tl;
		u_int level;

		while ((tl = npf_table_iterate(ncf)) != NULL) {
			npfctl_print_table(ctx, tl);
		}
		print_linesep(ctx);

		while ((rp = npf_rproc_iterate(ncf)) != NULL) {
			const char *rpname = npf_rproc_getname(rp);
			fprintf(ctx->fp, "procedure \"%s\"\n", rpname);
		}
		print_linesep(ctx);

		while ((nt = npf_nat_iterate(ncf)) != NULL) {
			npfctl_print_nat(ctx, nt);
		}
		print_linesep(ctx);

		while ((rl = npf_rule_iterate(ncf, &level)) != NULL) {
			print_indent(ctx, level);
			npfctl_print_rule(ctx, rl);
		}
		print_linesep(ctx);
	}
	npf_config_destroy(ncf);
	return 0;
}

int
npfctl_ruleset_show(int fd, const char *ruleset_name)
{
	npf_conf_info_t *ctx = &stdout_ctx;
	nl_config_t *ncf;
	nl_rule_t *rl;
	u_int level;
	int error;

	ncf = npf_config_create();
	ctx->conf = ncf;

	if ((error = _npf_ruleset_list(fd, ruleset_name, ncf)) != 0) {
		return error;
	}
	while ((rl = npf_rule_iterate(ncf, &level)) != NULL) {
		npfctl_print_rule(ctx, rl);
	}
	npf_config_destroy(ncf);
	return error;
}
