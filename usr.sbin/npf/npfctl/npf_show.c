/*-
 * Copyright (c) 2013-2020 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD");

#include <sys/socket.h>
#define	__FAVOR_BSD
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

#define	SEEN_PROTO	0x01

typedef struct {
	char **		values;
	unsigned	count;
} elem_list_t;

enum {
	LIST_PROTO = 0, LIST_SADDR, LIST_DADDR, LIST_SPORT, LIST_DPORT,
	LIST_COUNT,
};

typedef struct {
	nl_config_t *	conf;
	bool		validating;

	FILE *		fp;
	long		fpos;
	long		fposln;
	int		glevel;

	unsigned	flags;
	uint32_t	curmark;
	uint64_t	seen_marks;
	elem_list_t	list[LIST_COUNT];

} npf_conf_info_t;

static void	print_linesep(npf_conf_info_t *);

static npf_conf_info_t *
npfctl_show_init(void)
{
	static npf_conf_info_t stdout_ctx;
	memset(&stdout_ctx, 0, sizeof(npf_conf_info_t));
	stdout_ctx.glevel = -1;
	stdout_ctx.fp = stdout;
	return &stdout_ctx;
}

static void
list_push(elem_list_t *list, char *val)
{
	const unsigned n = list->count;
	char **values;

	if ((values = calloc(n + 1, sizeof(char *))) == NULL) {
		err(EXIT_FAILURE, "calloc");
	}
	for (unsigned i = 0; i < n; i++) {
		values[i] = list->values[i];
	}
	values[n] = val;
	free(list->values);
	list->values = values;
	list->count++;
}

static char *
list_join_free(elem_list_t *list, const bool use_br, const char *sep)
{
	char *s, buf[2048];

	if (!join(buf, sizeof(buf), list->count, list->values, sep)) {
		errx(EXIT_FAILURE, "out of memory while parsing the rule");
	}
	easprintf(&s, (use_br && list->count > 1) ? "{ %s }" : "%s", buf);
	for (unsigned i = 0; i < list->count; i++) {
		free(list->values[i]);
	}
	free(list->values);
	list->values = NULL;
	list->count = 0;
	return s;
}

/*
 * Helper routines to print various pieces of information.
 */

static void
print_indent(npf_conf_info_t *ctx, unsigned level)
{
	if (ctx->glevel >= 0 && level <= (unsigned)ctx->glevel) {
		/*
		 * Level decrease -- end of the group.
		 * Print the group closing curly bracket.
		 */
		ctx->fpos += fprintf(ctx->fp, "}\n\n");
		ctx->glevel = -1;
	}
	while (level--) {
		ctx->fpos += fprintf(ctx->fp, "\t");
	}
}

static void
print_linesep(npf_conf_info_t *ctx)
{
	if (ctx->fpos != ctx->fposln) {
		ctx->fpos += fprintf(ctx->fp, "\n");
		ctx->fposln = ctx->fpos;
	}
}

static size_t
tcpflags2string(char *buf, unsigned tfl)
{
	unsigned i = 0;

	if (tfl & TH_FIN)	buf[i++] = 'F';
	if (tfl & TH_SYN)	buf[i++] = 'S';
	if (tfl & TH_RST)	buf[i++] = 'R';
	if (tfl & TH_PUSH)	buf[i++] = 'P';
	if (tfl & TH_ACK)	buf[i++] = 'A';
	if (tfl & TH_URG)	buf[i++] = 'U';
	if (tfl & TH_ECE)	buf[i++] = 'E';
	if (tfl & TH_CWR)	buf[i++] = 'W';
	buf[i] = '\0';
	return i;
}

static char *
print_family(npf_conf_info_t *ctx __unused, const uint32_t *words)
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
print_address(npf_conf_info_t *ctx __unused, const uint32_t *words)
{
	const int af = *words++;
	const unsigned mask = *words++;
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
	return npfctl_print_addrmask(alen, "%a", addr, mask);
}

static char *
print_number(npf_conf_info_t *ctx __unused, const uint32_t *words)
{
	char *p;
	easprintf(&p, "%u", words[0]);
	return p;
}

static char *
print_table(npf_conf_info_t *ctx, const uint32_t *words)
{
	const unsigned tid = words[0];
	const char *tname;
	char *s = NULL;
	bool ifaddr;

	tname = npfctl_table_getname(ctx->conf, tid, &ifaddr);
	easprintf(&s, ifaddr ? "ifaddrs(%s)" : "<%s>", tname);
	return s;
}

static char *
print_proto(npf_conf_info_t *ctx, const uint32_t *words)
{
	ctx->flags |= SEEN_PROTO;
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
print_tcpflags(npf_conf_info_t *ctx __unused, const uint32_t *words)
{
	const unsigned tf = words[0], tf_mask = words[1];
	char buf[32];
	size_t n;

	if ((ctx->flags & SEEN_PROTO) == 0) {
		/*
		 * Note: the TCP flag matching might be without 'proto tcp'
		 * when using a plain 'stateful' rule.  In such case, just
		 * skip showing of the flags as they are implicit.
		 */
		return NULL;
	}
	n = tcpflags2string(buf, tf);
	if (tf != tf_mask) {
		buf[n++] = '/';
		tcpflags2string(buf + n, tf_mask);
	}
	return estrdup(buf);
}

static char *
print_portrange(npf_conf_info_t *ctx __unused, const uint32_t *words)
{
	unsigned fport = words[0], tport = words[1];
	char *p;

	if (fport != tport) {
		easprintf(&p, "%u-%u", fport, tport);
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
#define	STATEFUL_ALL	(NPF_RULE_STATEFUL | NPF_RULE_GSTATEFUL)
#define	NAME_AT		2

static const struct attr_keyword_mapent {
	uint32_t	mask;
	uint32_t	flags;
	const char *	val;
} attr_keyword_map[] = {
	{ F(GROUP)|F(DYNAMIC),	F(GROUP),		"group"		},
	{ F(GROUP)|F(DYNAMIC),	F(GROUP)|F(DYNAMIC),	"ruleset"	},
	{ F(GROUP)|F(PASS),	0,			"block"		},
	{ F(GROUP)|F(PASS),	F(PASS),		"pass"		},
	{ F(RETRST)|F(RETICMP),	F(RETRST)|F(RETICMP),	"return"	},
	{ F(RETRST)|F(RETICMP),	F(RETRST),		"return-rst"	},
	{ F(RETRST)|F(RETICMP),	F(RETICMP),		"return-icmp"	},
	{ STATEFUL_ALL,		F(STATEFUL),		"stateful"	},
	{ STATEFUL_ALL,		STATEFUL_ALL,		"stateful-all"	},
	{ F(DIMASK),		F(IN),			"in"		},
	{ F(DIMASK),		F(OUT),			"out"		},
	{ F(FINAL),		F(FINAL),		"final"		},
};

static const struct mark_keyword_mapent {
	unsigned	mark;
	const char *	format;
	int		list_id;
	char *		(*printfn)(npf_conf_info_t *, const uint32_t *);
	unsigned	fwords;
} mark_keyword_map[] = {
	{ BM_IPVER,	"family %s",	LIST_PROTO,	print_family,	1 },
	{ BM_PROTO,	"proto %s",	LIST_PROTO,	print_proto,	1 },
	{ BM_TCPFL,	"flags %s",	LIST_PROTO,	print_tcpflags,	2 },
	{ BM_ICMP_TYPE,	"icmp-type %s",	LIST_PROTO,	print_number,	1 },
	{ BM_ICMP_CODE,	"code %s",	LIST_PROTO,	print_number,	1 },

	{ BM_SRC_NEG,	NULL,		-1,		NULL,		0 },
	{ BM_SRC_CIDR,	NULL,		LIST_SADDR,	print_address,	6 },
	{ BM_SRC_TABLE,	NULL,		LIST_SADDR,	print_table,	1 },
	{ BM_SRC_PORTS,	NULL,		LIST_SPORT,	print_portrange,2 },

	{ BM_DST_NEG,	NULL,		-1,		NULL,		0 },
	{ BM_DST_CIDR,	NULL,		LIST_DADDR,	print_address,	6 },
	{ BM_DST_TABLE,	NULL,		LIST_DADDR,	print_table,	1 },
	{ BM_DST_PORTS,	NULL,		LIST_DPORT,	print_portrange,2 },
};

static const char * __attribute__((format_arg(2)))
verified_fmt(const char *fmt, const char *t __unused)
{
	return fmt;
}

static void
scan_marks(npf_conf_info_t *ctx, const struct mark_keyword_mapent *mk,
    const uint32_t *marks, size_t mlen)
{
	elem_list_t sublist, *target_list;

	/*
	 * If format is used for this mark, then collect multiple elements
	 * in into the list, merge and re-push the set into the target list.
	 *
	 * Currently, this is applicable only for 'proto { tcp, udp }'.
	 */
	memset(&sublist, 0, sizeof(elem_list_t));
	target_list = mk->format ? &sublist : &ctx->list[mk->list_id];

	/* Scan for the marks and extract the values. */
	mlen /= sizeof(uint32_t);
	while (mlen > 2) {
		const uint32_t m = *marks++;
		const unsigned nwords = *marks++;

		if ((mlen -= 2) < nwords) {
			errx(EXIT_FAILURE, "byte-code marking inconsistency");
		}
		if (m == mk->mark) {
			/*
			 * Set the current mark and note it as seen.
			 * Value is processed by the print function,
			 * otherwise we just need to note the mark.
			 */
			ctx->curmark = m;
			assert(BM_COUNT < (sizeof(uint64_t) * CHAR_BIT));
			ctx->seen_marks = UINT64_C(1) << m;
			assert(mk->fwords == nwords);

			if (mk->printfn) {
				char *val;

				if ((val = mk->printfn(ctx, marks)) != NULL) {
					list_push(target_list, val);
				}
			}
		}
		marks += nwords;
		mlen -= nwords;
	}

	if (sublist.count) {
		char *val, *elements;

		elements = list_join_free(&sublist, true, ", ");
		easprintf(&val, verified_fmt(mk->format, "%s"), elements );
		list_push(&ctx->list[mk->list_id], val);
		free(elements);
	}
}

static void
npfctl_print_id(npf_conf_info_t *ctx, nl_rule_t *rl)
{
	const uint64_t id = npf_rule_getid(rl);

	if (id) {
		ctx->fpos += fprintf(ctx->fp, "# id=\"%" PRIx64 "\" ", id);
	}
}

static void
npfctl_print_filter_generic(npf_conf_info_t *ctx)
{
	elem_list_t *list = &ctx->list[LIST_PROTO];

	if (list->count) {
		char *elements = list_join_free(list, false, " ");
		ctx->fpos += fprintf(ctx->fp, "%s ", elements);
		free(elements);
	}
}

static bool
npfctl_print_filter_seg(npf_conf_info_t *ctx, unsigned which)
{
	static const struct {
		const char *	keyword;
		unsigned	alist;
		unsigned	plist;
		unsigned	negbm;
	} refs[] = {
		[NPF_SRC] = {
			.keyword	= "from",
			.alist		= LIST_SADDR,
			.plist		= LIST_SPORT,
			.negbm		= UINT64_C(1) << BM_SRC_NEG,
		},
		[NPF_DST] = {
			.keyword	= "to",
			.alist		= LIST_DADDR,
			.plist		= LIST_DPORT,
			.negbm		= UINT64_C(1) << BM_DST_NEG,
		}
	};
	const char *neg = !!(ctx->seen_marks & refs[which].negbm) ? "! " : "";
	const char *kwd = refs[which].keyword;
	bool seen_filter = false;
	elem_list_t *list;
	char *elements;

	list = &ctx->list[refs[which].alist];
	if (list->count != 0) {
		seen_filter = true;
		elements = list_join_free(list, true, ", ");
		ctx->fpos += fprintf(ctx->fp, "%s %s%s ", kwd, neg, elements);
		free(elements);
	}

	list = &ctx->list[refs[which].plist];
	if (list->count != 0) {
		if (!seen_filter) {
			ctx->fpos += fprintf(ctx->fp, "%s any ", kwd);
			seen_filter = true;
		}
		elements = list_join_free(list, true, ", ");
		ctx->fpos += fprintf(ctx->fp, "port %s ", elements);
		free(elements);
	}
	return seen_filter;
}

static bool
npfctl_print_filter(npf_conf_info_t *ctx, nl_rule_t *rl)
{
	const void *marks;
	size_t mlen, len;
	const void *code;
	bool seenf = false;
	int type;

	marks = npf_rule_getinfo(rl, &mlen);
	if (!marks && (code = npf_rule_getcode(rl, &type, &len)) != NULL) {
		/*
		 * No marks, but the byte-code is present.  This must
		 * have been filled by libpcap(3) or possibly an unknown
		 * to us byte-code.
		 */
		ctx->fpos += fprintf(ctx->fp, "%s ", type == NPF_CODE_BPF ?
		    "pcap-filter \"...\"" : "unrecognized-bytecode");
		return true;
	}
	ctx->flags = 0;

	/*
	 * BPF filter criteria described by the byte-code marks.
	 */
	for (unsigned i = 0; i < __arraycount(mark_keyword_map); i++) {
		const struct mark_keyword_mapent *mk = &mark_keyword_map[i];
		scan_marks(ctx, mk, marks, mlen);
	}
	npfctl_print_filter_generic(ctx);
	seenf |= npfctl_print_filter_seg(ctx, NPF_SRC);
	seenf |= npfctl_print_filter_seg(ctx, NPF_DST);
	return seenf;
}

static void
npfctl_print_rule(npf_conf_info_t *ctx, nl_rule_t *rl, unsigned level)
{
	const uint32_t attr = npf_rule_getattr(rl);
	const char *rproc, *ifname, *name;
	bool dyn_ruleset;

	/* Rule attributes/flags. */
	for (unsigned i = 0; i < __arraycount(attr_keyword_map); i++) {
		const struct attr_keyword_mapent *ak = &attr_keyword_map[i];

		if (i == NAME_AT && (name = npf_rule_getname(rl)) != NULL) {
			ctx->fpos += fprintf(ctx->fp, "\"%s\" ", name);
		}
		if ((attr & ak->mask) == ak->flags) {
			ctx->fpos += fprintf(ctx->fp, "%s ", ak->val);
		}
	}
	if ((ifname = npf_rule_getinterface(rl)) != NULL) {
		ctx->fpos += fprintf(ctx->fp, "on %s ", ifname);
	}
	if (attr == (NPF_RULE_GROUP | NPF_RULE_IN | NPF_RULE_OUT) && !ifname) {
		/* The default group is a special case. */
		ctx->fpos += fprintf(ctx->fp, "default ");
	}
	if ((attr & NPF_DYNAMIC_GROUP) == NPF_RULE_GROUP) {
		/* Group; done. */
		ctx->fpos += fprintf(ctx->fp, "{ ");
		ctx->glevel = level;
		goto out;
	}

	/* Print filter criteria. */
	dyn_ruleset = (attr & NPF_DYNAMIC_GROUP) == NPF_DYNAMIC_GROUP;
	if (!npfctl_print_filter(ctx, rl) && !dyn_ruleset) {
		ctx->fpos += fprintf(ctx->fp, "all ");
	}

	/* Rule procedure. */
	if ((rproc = npf_rule_getproc(rl)) != NULL) {
		ctx->fpos += fprintf(ctx->fp, "apply \"%s\" ", rproc);
	}
out:
	npfctl_print_id(ctx, rl);
	ctx->fpos += fprintf(ctx->fp, "\n");
}

static void
npfctl_print_nat(npf_conf_info_t *ctx, nl_nat_t *nt)
{
	const unsigned dynamic_natset = NPF_RULE_GROUP | NPF_RULE_DYNAMIC;
	nl_rule_t *rl = (nl_nat_t *)nt;
	const char *ifname, *algo, *seg1, *seg2, *arrow;
	const npf_addr_t *addr;
	npf_netmask_t mask;
	in_port_t port;
	size_t alen;
	unsigned flags;
	char *seg;

	/* Get flags and the interface. */
	flags = npf_nat_getflags(nt);
	ifname = npf_rule_getinterface(rl);
	assert(ifname != NULL);

	if ((npf_rule_getattr(rl) & dynamic_natset) == dynamic_natset) {
		const char *name = npf_rule_getname(rl);
		ctx->fpos += fprintf(ctx->fp,
		    "map ruleset \"%s\" on %s\n", name, ifname);
		return;
	}

	/* Get the translation address or table (and port, if used). */
	addr = npf_nat_getaddr(nt, &alen, &mask);
	if (addr) {
		seg = npfctl_print_addrmask(alen, "%a", addr, mask);
	} else {
		const unsigned tid = npf_nat_gettable(nt);
		const char *tname;
		bool ifaddr;

		tname = npfctl_table_getname(ctx->conf, tid, &ifaddr);
		easprintf(&seg, ifaddr ? "ifaddrs(%s)" : "<%s>", tname);
	}

	if ((port = npf_nat_getport(nt)) != 0) {
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

	/* NAT algorithm. */
	switch (npf_nat_getalgo(nt)) {
	case NPF_ALGO_NETMAP:
		algo = "algo netmap ";
		break;
	case NPF_ALGO_IPHASH:
		algo = "algo ip-hash ";
		break;
	case NPF_ALGO_RR:
		algo = "algo round-robin ";
		break;
	case NPF_ALGO_NPT66:
		algo = "algo npt66 ";
		break;
	default:
		algo = "";
		break;
	}

	/* XXX also handle "any" */

	/* Print out the NAT policy with the filter criteria. */
	ctx->fpos += fprintf(ctx->fp, "map %s %s %s%s%s %s %s pass ",
	    ifname, (flags & NPF_NAT_STATIC) ? "static" : "dynamic",
	    algo, (flags & NPF_NAT_PORTS) ? "" : "no-ports ",
	    seg1, arrow, seg2);
	npfctl_print_filter(ctx, rl);
	npfctl_print_id(ctx, rl);
	ctx->fpos += fprintf(ctx->fp, "\n");
	free(seg);
}

static void
npfctl_print_table(npf_conf_info_t *ctx, nl_table_t *tl)
{
	const char *name = npf_table_getname(tl);
	const unsigned type = npf_table_gettype(tl);
	const char *table_types[] = {
		[NPF_TABLE_IPSET]	= "ipset",
		[NPF_TABLE_LPM]		= "lpm",
		[NPF_TABLE_CONST]	= "const",
	};

	if (name[0] == '.') {
		/* Internal tables use dot and are hidden. */
		return;
	}
	assert(type < __arraycount(table_types));
	ctx->fpos += fprintf(ctx->fp,
	    "table <%s> type %s\n", name, table_types[type]);
}

static void
npfctl_print_params(npf_conf_info_t *ctx, nl_config_t *ncf)
{
	nl_iter_t i = NPF_ITER_BEGIN;
	int val, defval, *dval;
	const char *name;

	dval = ctx->validating ? NULL : &defval;
	while ((name = npf_param_iterate(ncf, &i, &val, dval)) != NULL) {
		if (dval && val == *dval) {
			continue;
		}
		ctx->fpos += fprintf(ctx->fp, "set %s %d\n", name, val);
	}
	print_linesep(ctx);
}

int
npfctl_config_show(int fd)
{
	npf_conf_info_t *ctx = npfctl_show_init();
	nl_config_t *ncf;
	bool loaded;

	if (fd) {
		ncf = npf_config_retrieve(fd);
		if (ncf == NULL) {
			return errno;
		}
		loaded = npf_config_loaded_p(ncf);
		ctx->validating = false;
		ctx->fpos += fprintf(ctx->fp,
		    "# filtering:\t%s\n# config:\t%s\n",
		    npf_config_active_p(ncf) ? "active" : "inactive",
		    loaded ? "loaded" : "empty");
		print_linesep(ctx);
	} else {
		ncf = npfctl_config_ref();
		npfctl_config_build();
		ctx->validating = true;
		loaded = true;
	}
	ctx->conf = ncf;

	if (loaded) {
		nl_rule_t *rl;
		nl_rproc_t *rp;
		nl_nat_t *nt;
		nl_table_t *tl;
		nl_iter_t i;
		unsigned level;

		npfctl_print_params(ctx, ncf);

		i = NPF_ITER_BEGIN;
		while ((tl = npf_table_iterate(ncf, &i)) != NULL) {
			npfctl_print_table(ctx, tl);
		}
		print_linesep(ctx);

		i = NPF_ITER_BEGIN;
		while ((rp = npf_rproc_iterate(ncf, &i)) != NULL) {
			const char *rpname = npf_rproc_getname(rp);
			ctx->fpos += fprintf(ctx->fp,
			    "procedure \"%s\"\n", rpname);
		}
		print_linesep(ctx);

		i = NPF_ITER_BEGIN;
		while ((nt = npf_nat_iterate(ncf, &i)) != NULL) {
			npfctl_print_nat(ctx, nt);
		}
		print_linesep(ctx);

		i = NPF_ITER_BEGIN;
		while ((rl = npf_rule_iterate(ncf, &i, &level)) != NULL) {
			print_indent(ctx, level);
			npfctl_print_rule(ctx, rl, level);
		}
		print_indent(ctx, 0);
	}
	npf_config_destroy(ncf);
	return 0;
}

int
npfctl_ruleset_show(int fd, const char *ruleset_name)
{
	npf_conf_info_t *ctx = npfctl_show_init();
	nl_config_t *ncf;
	nl_rule_t *rl;
	unsigned level;
	nl_iter_t i;
	int error;

	ncf = npf_config_create();
	ctx->conf = ncf;

	if ((error = _npf_ruleset_list(fd, ruleset_name, ncf)) != 0) {
		return error;
	}
	i = NPF_ITER_BEGIN;
	while ((rl = npf_rule_iterate(ncf, &i, &level)) != NULL) {
		npfctl_print_rule(ctx, rl, 0);
	}
	npf_config_destroy(ncf);
	return error;
}
