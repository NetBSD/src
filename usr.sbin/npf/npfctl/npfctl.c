/*	$NetBSD: npfctl.c,v 1.28 2013/02/01 05:40:07 spz Exp $	*/

/*-
 * Copyright (c) 2009-2012 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: npfctl.c,v 1.28 2013/02/01 05:40:07 spz Exp $");

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

#include "npfctl.h"

extern int		yylineno, yycolumn;
extern const char *	yyfilename;
extern int		yyparse(void);
extern void		yyrestart(FILE *);

enum {
	NPFCTL_START,
	NPFCTL_STOP,
	NPFCTL_RELOAD,
	NPFCTL_SHOWCONF,
	NPFCTL_FLUSH,
	NPFCTL_VALIDATE,
	NPFCTL_TABLE,
	NPFCTL_STATS,
	NPFCTL_SESSIONS_SAVE,
	NPFCTL_SESSIONS_LOAD,
};

static const struct operations_s {
	const char *		cmd;
	int			action;
} operations[] = {
	/* Start, stop, reload */
	{	"start",		NPFCTL_START		},
	{	"stop",			NPFCTL_STOP		},
	{	"reload",		NPFCTL_RELOAD		},
	{	"show",			NPFCTL_SHOWCONF,	},
	{	"flush",		NPFCTL_FLUSH		},
	{	"valid",		NPFCTL_VALIDATE		},
	/* Table */
	{	"table",		NPFCTL_TABLE		},
	/* Stats */
	{	"stats",		NPFCTL_STATS		},
	/* Sessions */
	{	"sess-save",		NPFCTL_SESSIONS_SAVE	},
	{	"sess-load",		NPFCTL_SESSIONS_LOAD	},
	/* --- */
	{	NULL,			0			}
};

__dead static void
usage(void)
{
	const char *progname = getprogname();

	fprintf(stderr,
	    "usage:\t%s [ start | stop | reload | flush | show | stats ]\n",
	    progname);
	fprintf(stderr,
	    "\t%s ( sess-save | sess-load )\n",
	    progname);
	fprintf(stderr,
	    "\t%s table <tid> { add | rem | test } <address/mask>\n",
	    progname);
	fprintf(stderr,
	    "\t%s table <tid> { list | flush }\n",
	    progname);

	exit(EXIT_FAILURE);
}

static void
npfctl_parsecfg(const char *cfg)
{
	FILE *fp;

	fp = fopen(cfg, "r");
	if (fp == NULL) {
		err(EXIT_FAILURE, "open '%s'", cfg);
	}
	yyrestart(fp);
	yylineno = 1;
	yycolumn = 0;
	yyfilename = cfg;
	yyparse();
	fclose(fp);
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
		{ NPF_STAT_PASS_SESSION,	"session pass"		},

		{ -1, "Packets blocked"					},
		{ NPF_STAT_BLOCK_DEFAULT,	"default block"		},
		{ NPF_STAT_BLOCK_RULESET,	"ruleset block"		},

		{ -1, "Session and NAT entries"				},
		{ NPF_STAT_SESSION_CREATE,	"session allocations"	},
		{ NPF_STAT_SESSION_DESTROY,	"session destructions"	},
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
		{ NPF_STAT_RACE_SESSION,	"duplicate session race"},

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
	static const char *ncode_errors[] = {
		[-NPF_ERR_OPCODE]	= "invalid instruction",
		[-NPF_ERR_JUMP]		= "invalid jump",
		[-NPF_ERR_REG]		= "invalid register",
		[-NPF_ERR_INVAL]	= "invalid argument value",
		[-NPF_ERR_RANGE]	= "processing out of range"
	};
	const int nc_err = ne->ne_ncode_error;
	const char *srcfile = ne->ne_source_file;

	if (srcfile) {
		warnx("source %s line %d", srcfile, ne->ne_source_line);
	}
	if (nc_err) {
		warnx("n-code error (%d): %s at offset 0x%x",
		    nc_err, ncode_errors[-nc_err], ne->ne_ncode_errat);
	}
	if (ne->ne_id) {
		warnx("object: %d", ne->ne_id);
	}
}

char *
npfctl_print_addrmask(int alen, npf_addr_t *addr, npf_netmask_t mask)
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
	if (mask) {
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
		{ "add",	NPF_IOCTL_TBLENT_ADD		},
		{ "rem",	NPF_IOCTL_TBLENT_REM		},
		{ "test",	NPF_IOCTL_TBLENT_LOOKUP		},
		{ "list",	NPF_IOCTL_TBLENT_LIST		},
		{ NULL,		0				}
	};
	npf_ioctl_table_t nct;
	fam_addr_mask_t fam;
	size_t buflen = 512;
	char *cmd, *arg = NULL; /* XXX gcc */
	int n, alen;

	/* Default action is list. */
	memset(&nct, 0, sizeof(npf_ioctl_table_t));
	nct.nct_tid = atoi(argv[0]);
	cmd = argv[1];

	for (n = 0; tblops[n].cmd != NULL; n++) {
		if (strcmp(cmd, tblops[n].cmd) != 0) {
			continue;
		}
		nct.nct_action = tblops[n].action;
		break;
	}
	if (tblops[n].cmd == NULL) {
		errx(EXIT_FAILURE, "invalid command '%s'", cmd);
	}
	if (nct.nct_action != NPF_IOCTL_TBLENT_LIST) {
		if (argc < 3) {
			usage();
		}
		arg = argv[2];
	}
again:
	if (nct.nct_action == NPF_IOCTL_TBLENT_LIST) {
		nct.nct_data.buf.buf = ecalloc(1, buflen);
		nct.nct_data.buf.len = buflen;
	} else {
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
		if (nct.nct_action == NPF_IOCTL_TBLENT_LIST) {
			/* XXX */
			free(nct.nct_data.buf.buf);
			buflen <<= 1;
			goto again;
		}
		/* FALLTHROUGH */
	default:
		err(EXIT_FAILURE, "ioctl");
	}

	if (nct.nct_action == NPF_IOCTL_TBLENT_LIST) {
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
		    nct.nct_action == NPF_IOCTL_TBLENT_LOOKUP ?
		    "matching entry found" : "success");
	}
	exit(EXIT_SUCCESS);
}

static void
npfctl(int action, int argc, char **argv)
{
	int fd, ret, ver, boolval;

	fd = open(NPF_DEV_PATH, O_RDONLY);
	if (fd == -1) {
		err(EXIT_FAILURE, "cannot open '%s'", NPF_DEV_PATH);
	}
	ret = ioctl(fd, IOC_NPF_VERSION, &ver);
	if (ret == -1) {
		err(EXIT_FAILURE, "ioctl");
	}
	if (ver != NPF_VERSION) {
		errx(EXIT_FAILURE,
		    "incompatible NPF interface version (%d, kernel %d)\n"
		    "Hint: update userland?", NPF_VERSION, ver);
	}
	switch (action) {
	case NPFCTL_START:
		boolval = true;
		ret = ioctl(fd, IOC_NPF_SWITCH, &boolval);
		break;
	case NPFCTL_STOP:
		boolval = false;
		ret = ioctl(fd, IOC_NPF_SWITCH, &boolval);
		break;
	case NPFCTL_RELOAD:
		npfctl_config_init(false);
		npfctl_parsecfg(argc < 3 ? NPF_CONF_PATH : argv[2]);
		ret = npfctl_config_send(fd, NULL);
		if (ret) {
			errx(EXIT_FAILURE, "ioctl: %s", strerror(ret));
		}
		break;
	case NPFCTL_SHOWCONF:
		ret = npfctl_config_show(fd);
		break;
	case NPFCTL_FLUSH:
		ret = npf_config_flush(fd);
		break;
	case NPFCTL_VALIDATE:
		npfctl_config_init(false);
		npfctl_parsecfg(argc < 3 ? NPF_CONF_PATH : argv[2]);
		ret = npfctl_config_show(0);
		break;
	case NPFCTL_TABLE:
		if ((argc -= 2) < 2) {
			usage();
		}
		argv += 2;
		npfctl_table(fd, argc, argv);
		break;
	case NPFCTL_STATS:
		ret = npfctl_print_stats(fd);
		break;
	case NPFCTL_SESSIONS_SAVE:
		if (npf_sessions_recv(fd, NPF_SESSDB_PATH) != 0) {
			errx(EXIT_FAILURE, "could not save sessions to '%s'",
			    NPF_SESSDB_PATH);
		}
		break;
	case NPFCTL_SESSIONS_LOAD:
		if (npf_sessions_send(fd, NPF_SESSDB_PATH) != 0) {
			errx(EXIT_FAILURE, "no sessions loaded from '%s'",
			    NPF_SESSDB_PATH);
		}
		break;
	}
	if (ret) {
		err(EXIT_FAILURE, "ioctl");
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
		npfctl_parsecfg(cfg);
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
