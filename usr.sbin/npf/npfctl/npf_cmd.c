/*-
 * Copyright (c) 2009-2020 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: npf_cmd.c,v 1.1 2020/05/30 14:16:56 rmind Exp $");

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

#ifdef __NetBSD__
#include <sha1.h>
#define SHA_DIGEST_LENGTH SHA1_DIGEST_LENGTH
#else
#include <openssl/sha.h>
#endif

#include "npfctl.h"

////////////////////////////////////////////////////////////////////////////
//
// NPFCTL RULE COMMANDS
//

#ifdef __NetBSD__
static unsigned char *
SHA1(const unsigned char *d, size_t l, unsigned char *md)
{
	SHA1_CTX c;

	SHA1Init(&c);
	SHA1Update(&c, d, l);
	SHA1Final(md, &c);
	return md;
}
#endif

static void
npfctl_generate_key(nl_rule_t *rl, void *key)
{
	void *meta;
	size_t len;

	if ((meta = npf_rule_export(rl, &len)) == NULL) {
		errx(EXIT_FAILURE, "error generating rule key");
	}
	__CTASSERT(NPF_RULE_MAXKEYLEN >= SHA_DIGEST_LENGTH);
	memset(key, 0, NPF_RULE_MAXKEYLEN);
	SHA1(meta, len, key);
	free(meta);
}

int
npfctl_nat_ruleset_p(const char *name, bool *natset)
{
	const size_t preflen = sizeof(NPF_RULESET_MAP_PREF) - 1;
	*natset = strncmp(name, NPF_RULESET_MAP_PREF, preflen) == 0;
	return (*natset && strlen(name) <= preflen) ? -1 : 0;
}

static nl_rule_t *
npfctl_parse_rule(int argc, char **argv, parse_entry_t entry)
{
	char rule_string[1024];
	nl_rule_t *rl;

	/* Get the rule string and parse it. */
	if (!join(rule_string, sizeof(rule_string), argc, argv, " ")) {
		errx(EXIT_FAILURE, "command too long");
	}
	npfctl_parse_string(rule_string, entry);
	if ((rl = npfctl_rule_ref()) == NULL) {
		errx(EXIT_FAILURE, "could not parse the rule");
	}
	return rl;
}

void
npfctl_rule(int fd, int argc, char **argv)
{
	static const struct ruleops_s {
		const char *	cmd;
		int		action;
		bool		extra_arg;
	} ruleops[] = {
		{ "add",	NPF_CMD_RULE_ADD,	true	},
		{ "rem",	NPF_CMD_RULE_REMKEY,	true	},
		{ "del",	NPF_CMD_RULE_REMKEY,	true	},
		{ "rem-id",	NPF_CMD_RULE_REMOVE,	true	},
		{ "list",	NPF_CMD_RULE_LIST,	false	},
		{ "flush",	NPF_CMD_RULE_FLUSH,	false	},
		{ NULL,		0,			0	}
	};
	uint8_t key[NPF_RULE_MAXKEYLEN];
	const char *ruleset_name = argv[0];
	const char *cmd = argv[1];
	int error, action = 0;
	bool extra_arg, natset;
	parse_entry_t entry;
	uint64_t rule_id;
	nl_rule_t *rl;

	for (unsigned n = 0; ruleops[n].cmd != NULL; n++) {
		if (strcmp(cmd, ruleops[n].cmd) == 0) {
			action = ruleops[n].action;
			extra_arg = ruleops[n].extra_arg;
			break;
		}
	}
	argc -= 2;
	argv += 2;

	if (!action || (extra_arg && argc == 0)) {
		usage();
	}

	if (npfctl_nat_ruleset_p(ruleset_name, &natset) != 0) {
		errx(EXIT_FAILURE,
		    "invalid NAT ruleset name (note: the name must be "
		    "prefixed with `" NPF_RULESET_MAP_PREF "`)");
	}
	entry = natset ? NPFCTL_PARSE_MAP : NPFCTL_PARSE_RULE;

	switch (action) {
	case NPF_CMD_RULE_ADD:
		rl = npfctl_parse_rule(argc, argv, entry);
		npfctl_generate_key(rl, key);
		npf_rule_setkey(rl, key, sizeof(key));
		error = npf_ruleset_add(fd, ruleset_name, rl, &rule_id);
		break;
	case NPF_CMD_RULE_REMKEY:
		rl = npfctl_parse_rule(argc, argv, entry);
		npfctl_generate_key(rl, key);
		error = npf_ruleset_remkey(fd, ruleset_name, key, sizeof(key));
		break;
	case NPF_CMD_RULE_REMOVE:
		rule_id = strtoull(argv[0], NULL, 16);
		error = npf_ruleset_remove(fd, ruleset_name, rule_id);
		break;
	case NPF_CMD_RULE_LIST:
		error = npfctl_ruleset_show(fd, ruleset_name);
		break;
	case NPF_CMD_RULE_FLUSH:
		error = npf_ruleset_flush(fd, ruleset_name);
		break;
	default:
		abort();
	}

	switch (error) {
	case 0:
		/* Success. */
		break;
	case ESRCH:
		errx(EXIT_FAILURE, "ruleset \"%s\" not found", ruleset_name);
	case ENOENT:
		errx(EXIT_FAILURE, "rule was not found");
	default:
		errx(EXIT_FAILURE, "rule operation: %s", strerror(error));
	}
	if (action == NPF_CMD_RULE_ADD) {
		printf("OK %" PRIx64 "\n", rule_id);
	}
}

////////////////////////////////////////////////////////////////////////////
//
// NPFCTL TABLE COMMANDS
//

static int
npfctl_table_type(const char *typename)
{
	static const struct tbltype_s {
		const char *	name;
		unsigned	type;
	} tbltypes[] = {
		{ "ipset",	NPF_TABLE_IPSET	},
		{ "lpm",	NPF_TABLE_LPM	},
		{ "const",	NPF_TABLE_CONST	},
		{ NULL,		0		}
	};

	for (unsigned i = 0; tbltypes[i].name != NULL; i++) {
		if (strcmp(typename, tbltypes[i].name) == 0) {
			return tbltypes[i].type;
		}
	}
	return 0;
}

void
npfctl_table_replace(int fd, int argc, char **argv)
{
	const char *name, *newname, *path, *typename = NULL;
	nl_config_t *ncf;
	nl_table_t *t;
	unsigned type = 0;
	int c, tid = -1;
	FILE *fp;

	name = newname = argv[0];
	optind = 2;
	while ((c = getopt(argc, argv, "n:t:")) != -1) {
		switch (c) {
		case 't':
			typename = optarg;
			break;
		case 'n':
			newname = optarg;
			break;
		default:
			errx(EXIT_FAILURE,
			    "Usage: %s table \"table-name\" replace "
			    "[-n \"name\"] [-t <type>] <table-file>\n",
			    getprogname());
		}
	}
	argc -= optind;
	argv += optind;

	if (typename && (type = npfctl_table_type(typename)) == 0) {
		errx(EXIT_FAILURE, "unsupported table type '%s'", typename);
	}

	if (argc != 1) {
		usage();
	}

	path = argv[0];
	if (strcmp(path, "-") == 0) {
		path = "stdin";
		fp = stdin;
	} else if ((fp = fopen(path, "r")) == NULL) {
		err(EXIT_FAILURE, "open '%s'", path);
	}

	/* Get existing config to lookup ID of existing table */
	if ((ncf = npf_config_retrieve(fd)) == NULL) {
		err(EXIT_FAILURE, "npf_config_retrieve()");
	}
	if ((t = npfctl_table_getbyname(ncf, name)) == NULL) {
		errx(EXIT_FAILURE,
		    "table '%s' not found in the active configuration", name);
	}
	tid = npf_table_getid(t);
	if (!type) {
		type = npf_table_gettype(t);
	}
	npf_config_destroy(ncf);

	if ((t = npfctl_load_table(newname, tid, type, path, fp)) == NULL) {
		err(EXIT_FAILURE, "table load failed");
	}

	if (npf_table_replace(fd, t, NULL)) {
		err(EXIT_FAILURE, "npf_table_replace(<%s>)", name);
	}
}

void
npfctl_table(int fd, int argc, char **argv)
{
	static const struct tblops_s {
		const char *	cmd;
		int		action;
	} tblops[] = {
		{ "add",	NPF_CMD_TABLE_ADD		},
		{ "rem",	NPF_CMD_TABLE_REMOVE		},
		{ "del",	NPF_CMD_TABLE_REMOVE		},
		{ "test",	NPF_CMD_TABLE_LOOKUP		},
		{ "list",	NPF_CMD_TABLE_LIST		},
		{ "flush",	NPF_CMD_TABLE_FLUSH		},
		{ NULL,		0				}
	};
	npf_ioctl_table_t nct;
	fam_addr_mask_t fam;
	size_t buflen = 512;
	char *cmd, *arg;
	int n, alen;

	/* Default action is list. */
	memset(&nct, 0, sizeof(npf_ioctl_table_t));
	nct.nct_name = argv[0];
	cmd = argv[1];

	for (n = 0; tblops[n].cmd != NULL; n++) {
		if (strcmp(cmd, tblops[n].cmd) != 0) {
			continue;
		}
		nct.nct_cmd = tblops[n].action;
		break;
	}
	if (tblops[n].cmd == NULL) {
		errx(EXIT_FAILURE, "invalid command '%s'", cmd);
	}

	switch (nct.nct_cmd) {
	case NPF_CMD_TABLE_LIST:
	case NPF_CMD_TABLE_FLUSH:
		arg = NULL;
		break;
	default:
		if (argc < 3) {
			usage();
		}
		arg = argv[2];
	}

again:
	switch (nct.nct_cmd) {
	case NPF_CMD_TABLE_LIST:
		nct.nct_data.buf.buf = ecalloc(1, buflen);
		nct.nct_data.buf.len = buflen;
		break;
	case NPF_CMD_TABLE_FLUSH:
		break;
	default:
		if (!npfctl_parse_cidr(arg, &fam, &alen)) {
			errx(EXIT_FAILURE, "invalid CIDR '%s'", arg);
		}
		nct.nct_data.ent.alen = alen;
		memcpy(&nct.nct_data.ent.addr, &fam.fam_addr, alen);
		nct.nct_data.ent.mask = fam.fam_mask;
	}

	if (ioctl(fd, IOC_NPF_TABLE, &nct) != -1) {
		errno = 0;
	}
	switch (errno) {
	case 0:
		break;
	case EEXIST:
		errx(EXIT_FAILURE, "entry already exists or is conflicting");
	case ENOENT:
		errx(EXIT_FAILURE, "not found");
	case EINVAL:
		errx(EXIT_FAILURE, "invalid address, mask or table ID");
	case ENOMEM:
		if (nct.nct_cmd == NPF_CMD_TABLE_LIST) {
			/* XXX */
			free(nct.nct_data.buf.buf);
			buflen <<= 1;
			goto again;
		}
		/* FALLTHROUGH */
	default:
		err(EXIT_FAILURE, "ioctl(IOC_NPF_TABLE)");
	}

	if (nct.nct_cmd == NPF_CMD_TABLE_LIST) {
		npf_ioctl_ent_t *ent = nct.nct_data.buf.buf;
		char *buf;

		while (nct.nct_data.buf.len--) {
			if (!ent->alen)
				break;
			buf = npfctl_print_addrmask(ent->alen, "%a",
			    &ent->addr, ent->mask);
			puts(buf);
			ent++;
		}
		free(nct.nct_data.buf.buf);
	} else {
		printf("%s: %s\n", getprogname(),
		    nct.nct_cmd == NPF_CMD_TABLE_LOOKUP ?
		    "match" : "success");
	}
}

////////////////////////////////////////////////////////////////////////////
//
// NPFCTL CONNECTION COMMANDS
//

typedef struct {
	FILE *		fp;
	unsigned	alen;
	const char *	ifname;
	bool		nat;
	bool		nowide;
	bool		name;

	bool		v4;
	unsigned	pwidth;
} npf_conn_filter_t;

static int
npfctl_conn_print(unsigned alen, const npf_addr_t *a, const in_port_t *p,
    const char *ifname, void *arg)
{
	const npf_conn_filter_t *fil = arg;
	char *addrstr, *src, *dst;
	const char *fmt;
	FILE *fp = fil->fp;
	bool nat_conn;

	/*
	 * Filter connection entries by IP version, interface and/or
	 * applicability of NAT.
	 */
	if (alen != fil->alen) {
		return 0;
	}
	if (fil->ifname && (!ifname || strcmp(ifname, fil->ifname) != 0)) {
		return 0;
	}
	nat_conn = !npfctl_addr_iszero(&a[2]) || p[2] != 0;
	if (fil->nat && !nat_conn) {
		return 0;
	}

	fmt = fil->name ? "%A" : (fil->v4 ? "%a" : "[%a]");

	addrstr = npfctl_print_addrmask(alen, fmt, &a[0], NPF_NO_NETMASK);
	easprintf(&src, "%s:%d", addrstr, p[0]);
	free(addrstr);

	addrstr = npfctl_print_addrmask(alen, fmt, &a[1], NPF_NO_NETMASK);
	easprintf(&dst, "%s:%d", addrstr, p[1]);
	free(addrstr);

	fprintf(fp, "%-*s %-*s ", fil->pwidth, src, fil->pwidth, dst);
	free(src);
	free(dst);

	fprintf(fp, "%-10s ", ifname ? ifname : "-");
	if (nat_conn) {
		addrstr = npfctl_print_addrmask(alen, fmt, &a[2], NPF_NO_NETMASK);
		fprintf(fp, "%s", addrstr);
		free(addrstr);
		if (p[2]) {
			fprintf(fp, ":%d", p[2]);
		}
	}
	fputc('\n', fp);
	return 1;
}

static void
npf_conn_list_v(int fd, unsigned alen, npf_conn_filter_t *f)
{
	f->alen = alen;
	f->v4 = alen == sizeof(struct in_addr);
	f->pwidth = f->nowide ? 0 : ((f->v4 ? 15 : 40) + 1 + 5);
	if (npf_conn_list(fd, npfctl_conn_print, f) != 0) {
		err(EXIT_FAILURE, "npf_conn_list");
	}
}

int
npfctl_conn_list(int fd, int argc, char **argv)
{
	npf_conn_filter_t f;
	bool header = true;
	unsigned alen = 0;
	int c;

	argc--;
	argv++;

	memset(&f, 0, sizeof(f));
	f.fp = stdout;

	while ((c = getopt(argc, argv, "46hi:nNW")) != -1) {
		switch (c) {
		case '4':
			alen = sizeof(struct in_addr);
			break;
		case '6':
			alen = sizeof(struct in6_addr);
			break;
		case 'h':
			header = false;
			break;
		case 'i':
			f.ifname = optarg;
			break;
		case 'n':
			f.nat = true;
			break;
		case 'N':
			f.name = true;
			break;
		case 'W':
			f.nowide = true;
			break;
		default:
			errx(EXIT_FAILURE,
			    "Usage: %s list [-46hnNW] [-i <ifname>]\n",
			    getprogname());
		}
	}

	if (header) {
		fprintf(f.fp, "# %-*s %-*s %-*s %s\n",
		    21 - 2, "src-addr:port",
		    21, "dst-addr:port",
		    10, "interface",
		    "nat-addr:port");
	}

	if (!alen || alen == sizeof(struct in_addr)) {
		npf_conn_list_v(fd, sizeof(struct in_addr), &f);
	}
	if (!alen || alen == sizeof(struct in6_addr)) {
		npf_conn_list_v(fd, sizeof(struct in6_addr), &f);
	}

	return 0;
}
