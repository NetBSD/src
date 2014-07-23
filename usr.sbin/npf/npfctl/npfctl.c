/*	$NetBSD: npfctl.c,v 1.42 2014/07/23 05:00:38 htodd Exp $	*/

/*-
 * Copyright (c) 2009-2014 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: npfctl.c,v 1.42 2014/07/23 05:00:38 htodd Exp $");

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <openssl/sha.h>

#include "npfctl.h"

extern void		npf_yyparse_string(const char *);

enum {
	NPFCTL_START,
	NPFCTL_STOP,
	NPFCTL_RELOAD,
	NPFCTL_SHOWCONF,
	NPFCTL_FLUSH,
	NPFCTL_VALIDATE,
	NPFCTL_TABLE,
	NPFCTL_RULE,
	NPFCTL_STATS,
	NPFCTL_SAVE,
	NPFCTL_LOAD,
};

static const struct operations_s {
	const char *		cmd;
	int			action;
} operations[] = {
	/* Start, stop, reload */
	{	"start",	NPFCTL_START		},
	{	"stop",		NPFCTL_STOP		},
	{	"reload",	NPFCTL_RELOAD		},
	{	"show",		NPFCTL_SHOWCONF,	},
	{	"flush",	NPFCTL_FLUSH		},
	{	"valid",	NPFCTL_VALIDATE		},
	/* Table */
	{	"table",	NPFCTL_TABLE		},
	/* Rule */
	{	"rule",		NPFCTL_RULE		},
	/* Stats */
	{	"stats",	NPFCTL_STATS		},
	/* Full state save/load */
	{	"save",		NPFCTL_SAVE		},
	{	"load",		NPFCTL_LOAD		},
	/* --- */
	{	NULL,		0			}
};

bool
join(char *buf, size_t buflen, int count, char **args, const char *sep)
{
	const u_int seplen = strlen(sep);
	char *s = buf, *p = NULL;

	for (int i = 0; i < count; i++) {
		size_t len;

		p = stpncpy(s, args[i], buflen);
		len = p - s + seplen;
		if (len >= buflen) {
			return false;
		}
		buflen -= len;
		strcpy(p, sep);
		s = p + seplen;
	}
	*p = '\0';
	return true;
}

__dead static void
usage(void)
{
	const char *progname = getprogname();

	fprintf(stderr,
	    "Usage:\t%s start | stop | flush | show | stats\n",
	    progname);
	fprintf(stderr,
	    "\t%s validate | reload [<rule-file>]\n",
	    progname);
	fprintf(stderr,
	    "\t%s rule \"rule-name\" { add | rem } <rule-syntax>\n",
	    progname);
	fprintf(stderr,
	    "\t%s rule \"rule-name\" rem-id <rule-id>\n",
	    progname);
	fprintf(stderr,
	    "\t%s rule \"rule-name\" { list | flush }\n",
	    progname);
	fprintf(stderr,
	    "\t%s table <tid> { add | rem | test } <address/mask>\n",
	    progname);
	fprintf(stderr,
	    "\t%s table <tid> { list | flush }\n",
	    progname);
	fprintf(stderr,
	    "\t%s save | load\n",
	    progname);
	exit(EXIT_FAILURE);
}

static int
npfctl_print_stats(int fd)
{
	static const struct stats_s {
		/* Note: -1 indicates a new section. */
		int		index;
		const char *	name;
	} stats[] = {
		{ -1, "Packets passed"					},
		{ NPF_STAT_PASS_DEFAULT,	"default pass"		},
		{ NPF_STAT_PASS_RULESET,	"ruleset pass"		},
		{ NPF_STAT_PASS_CONN,		"state pass"		},

		{ -1, "Packets blocked"					},
		{ NPF_STAT_BLOCK_DEFAULT,	"default block"		},
		{ NPF_STAT_BLOCK_RULESET,	"ruleset block"		},

		{ -1, "State and NAT entries"				},
		{ NPF_STAT_CONN_CREATE,		"state allocations"},
		{ NPF_STAT_CONN_DESTROY,	"state destructions"},
		{ NPF_STAT_NAT_CREATE,		"NAT entry allocations"	},
		{ NPF_STAT_NAT_DESTROY,		"NAT entry destructions"},

		{ -1, "Network buffers"					},
		{ NPF_STAT_NBUF_NONCONTIG,	"non-contiguous cases"	},
		{ NPF_STAT_NBUF_CONTIG_FAIL,	"contig alloc failures"	},

		{ -1, "Invalid packet state cases"			},
		{ NPF_STAT_INVALID_STATE,	"cases in total"	},
		{ NPF_STAT_INVALID_STATE_TCP1,	"TCP case I"		},
		{ NPF_STAT_INVALID_STATE_TCP2,	"TCP case II"		},
		{ NPF_STAT_INVALID_STATE_TCP3,	"TCP case III"		},

		{ -1, "Packet race cases"				},
		{ NPF_STAT_RACE_NAT,		"NAT association race"	},
		{ NPF_STAT_RACE_CONN,		"duplicate state race"	},

		{ -1, "Fragmentation"					},
		{ NPF_STAT_FRAGMENTS,		"fragments"		},
		{ NPF_STAT_REASSEMBLY,		"reassembled"		},
		{ NPF_STAT_REASSFAIL,		"failed reassembly"	},

		{ -1, "Other"						},
		{ NPF_STAT_ERROR,		"unexpected errors"	},
	};
	uint64_t *st = ecalloc(1, NPF_STATS_SIZE);

	if (ioctl(fd, IOC_NPF_STATS, &st) != 0) {
		err(EXIT_FAILURE, "ioctl(IOC_NPF_STATS)");
	}

	for (unsigned i = 0; i < __arraycount(stats); i++) {
		const char *sname = stats[i].name;
		int sidx = stats[i].index;

		if (sidx == -1) {
			printf("%s:\n", sname);
		} else {
			printf("\t%"PRIu64" %s\n", st[sidx], sname);
		}
	}

	free(st);
	return 0;
}

void
npfctl_print_error(const nl_error_t *ne)
{
	const char *srcfile = ne->ne_source_file;

	if (srcfile) {
		warnx("source %s line %d", srcfile, ne->ne_source_line);
	}
	if (ne->ne_id) {
		warnx("object: %d", ne->ne_id);
	}
}

char *
npfctl_print_addrmask(int alen, const npf_addr_t *addr, npf_netmask_t mask)
{
	struct sockaddr_storage ss;
	char *buf = ecalloc(1, 64);
	int len;

	switch (alen) {
	case 4: {
		struct sockaddr_in *sin = (void *)&ss;
		sin->sin_len = sizeof(*sin);
		sin->sin_family = AF_INET;
		sin->sin_port = 0;
		memcpy(&sin->sin_addr, addr, sizeof(sin->sin_addr));
		break;
	}
	case 16: {
		struct sockaddr_in6 *sin6 = (void *)&ss;
		sin6->sin6_len = sizeof(*sin6);
		sin6->sin6_family = AF_INET6;
		sin6->sin6_port = 0;
		sin6->sin6_scope_id = 0;
		memcpy(&sin6->sin6_addr, addr, sizeof(sin6->sin6_addr));
		break;
	}
	default:
		assert(false);
	}
	len = sockaddr_snprintf(buf, 64, "%a", (struct sockaddr *)&ss);
	if (mask && mask != NPF_NO_NETMASK) {
		snprintf(&buf[len], 64 - len, "/%u", mask);
	}
	return buf;
}

__dead static void
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
		errx(EXIT_FAILURE, "no matching entry was not found");
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
			buf = npfctl_print_addrmask(ent->alen,
			    &ent->addr, ent->mask);
			puts(buf);
			ent++;
		}
		free(nct.nct_data.buf.buf);
	} else {
		printf("%s: %s\n", getprogname(),
		    nct.nct_cmd == NPF_CMD_TABLE_LOOKUP ?
		    "matching entry found" : "success");
	}
	exit(EXIT_SUCCESS);
}

static nl_rule_t *
npfctl_parse_rule(int argc, char **argv)
{
	char rule_string[1024];
	nl_rule_t *rl;

	/* Get the rule string and parse it. */
	if (!join(rule_string, sizeof(rule_string), argc, argv, " ")) {
		errx(EXIT_FAILURE, "command too long");
	}
	npfctl_parse_string(rule_string);
	if ((rl = npfctl_rule_ref()) == NULL) {
		errx(EXIT_FAILURE, "could not parse the rule");
	}
	return rl;
}

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

__dead static void
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
	uint64_t rule_id;
	bool extra_arg;
	nl_rule_t *rl;

	for (int n = 0; ruleops[n].cmd != NULL; n++) {
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

	switch (action) {
	case NPF_CMD_RULE_ADD:
		rl = npfctl_parse_rule(argc, argv);
		npfctl_generate_key(rl, key);
		npf_rule_setkey(rl, key, sizeof(key));
		error = npf_ruleset_add(fd, ruleset_name, rl, &rule_id);
		break;
	case NPF_CMD_RULE_REMKEY:
		rl = npfctl_parse_rule(argc, argv);
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
		assert(false);
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
	exit(EXIT_SUCCESS);
}

static int
npfctl_save(int fd)
{
	nl_config_t *ncf;
	bool active, loaded;
	int error;

	ncf = npf_config_retrieve(fd, &active, &loaded);
	if (ncf == NULL) {
		return errno;
	}
	error = npf_config_export(ncf, NPF_DB_PATH);
	npf_config_destroy(ncf);
	return error;
}

static int
npfctl_load(int fd)
{
	nl_config_t *ncf;
	int error;

	ncf = npf_config_import(NPF_DB_PATH);
	if (ncf == NULL) {
		return errno;
	}
	error = npf_config_submit(ncf, fd);
	npf_config_destroy(ncf);
	return error;
}

static void
npfctl(int action, int argc, char **argv)
{
	int fd, ver, boolval, ret = 0;

	fd = open(NPF_DEV_PATH, O_RDONLY);
	if (fd == -1) {
		err(EXIT_FAILURE, "cannot open '%s'", NPF_DEV_PATH);
	}
	if (ioctl(fd, IOC_NPF_VERSION, &ver) == -1) {
		err(EXIT_FAILURE, "ioctl(IOC_NPF_VERSION)");
	}
	if (ver != NPF_VERSION) {
		errx(EXIT_FAILURE,
		    "incompatible NPF interface version (%d, kernel %d)\n"
		    "Hint: update userland?", NPF_VERSION, ver);
	}

	const char *fun = "";
	switch (action) {
	case NPFCTL_START:
		boolval = true;
		ret = ioctl(fd, IOC_NPF_SWITCH, &boolval);
		fun = "ioctl(IOC_NPF_SWITCH)";
		break;
	case NPFCTL_STOP:
		boolval = false;
		ret = ioctl(fd, IOC_NPF_SWITCH, &boolval);
		fun = "ioctl(IOC_NPF_SWITCH)";
		break;
	case NPFCTL_RELOAD:
		npfctl_config_init(false);
		npfctl_parse_file(argc < 3 ? NPF_CONF_PATH : argv[2]);
		errno = ret = npfctl_config_send(fd, NULL);
		fun = "npfctl_config_send";
		break;
	case NPFCTL_SHOWCONF:
		ret = npfctl_config_show(fd);
		fun = "npfctl_config_show";
		break;
	case NPFCTL_FLUSH:
		ret = npf_config_flush(fd);
		fun = "npf_config_flush";
		break;
	case NPFCTL_VALIDATE:
		npfctl_config_init(false);
		npfctl_parse_file(argc < 3 ? NPF_CONF_PATH : argv[2]);
		ret = npfctl_config_show(0);
		fun = "npfctl_config_show";
		break;
	case NPFCTL_TABLE:
		if ((argc -= 2) < 2) {
			usage();
		}
		argv += 2;
		npfctl_table(fd, argc, argv);
		break;
	case NPFCTL_RULE:
		if ((argc -= 2) < 2) {
			usage();
		}
		argv += 2;
		npfctl_rule(fd, argc, argv);
		break;
	case NPFCTL_LOAD:
		ret = npfctl_load(fd);
		fun = "npfctl_config_load";
		break;
	case NPFCTL_SAVE:
		fd = npfctl_save(fd);
		fun = "npfctl_config_save";
		break;
	case NPFCTL_STATS:
		ret = npfctl_print_stats(fd);
		fun = "npfctl_print_stats";
		break;
	}
	if (ret) {
		err(EXIT_FAILURE, "%s", fun);
	}
	close(fd);
}

int
main(int argc, char **argv)
{
	char *cmd;

	if (argc < 2) {
		usage();
	}
	cmd = argv[1];

	if (strcmp(cmd, "debug") == 0) {
		const char *cfg = argc > 2 ? argv[2] : "/etc/npf.conf";
		const char *out = argc > 3 ? argv[3] : "/tmp/npf.plist";

		npfctl_config_init(true);
		npfctl_parse_file(cfg);
		npfctl_config_send(0, out);
		return EXIT_SUCCESS;
	}

	/* Find and call the subroutine. */
	for (int n = 0; operations[n].cmd != NULL; n++) {
		const char *opcmd = operations[n].cmd;
		if (strncmp(cmd, opcmd, strlen(opcmd)) != 0)
			continue;
		npfctl(operations[n].action, argc, argv);
		return EXIT_SUCCESS;
	}
	usage();
}
