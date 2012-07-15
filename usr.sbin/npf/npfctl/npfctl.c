/*	$NetBSD: npfctl.c,v 1.15 2012/07/15 00:22:59 rmind Exp $	*/

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
__RCSID("$NetBSD: npfctl.c,v 1.15 2012/07/15 00:22:59 rmind Exp $");

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

void *
zalloc(size_t sz)
{
	void *p = malloc(sz);

	if (p == NULL) {
		err(EXIT_FAILURE, "zalloc");
	}
	memset(p, 0, sz);
	return p;
}

void *
xrealloc(void *ptr, size_t size)
{
	void *p = realloc(ptr, size);

	if (p == NULL) {
		err(EXIT_FAILURE, "xrealloc");
	}
	return p;
}

char *
xstrdup(const char *s)
{
	char *p = strdup(s);

	if (p == NULL) {
		err(EXIT_FAILURE, "xstrdup");
	}
	return p;
}

char *
xstrndup(const char *s, size_t len)
{
	char *p;

	p = strndup(s, len);
	if (p == NULL) {
		err(EXIT_FAILURE, "xstrndup");
	}
	return p;
}

__dead static void
usage(void)
{
	const char *progname = getprogname();

	fprintf(stderr,
	    "usage:\t%s [ start | stop | reload | flush | show | stats ]\n",
	    progname);
	fprintf(stderr,
	    "usage:\t%s [ sess-save | sess-load ]\n",
	    progname);
	fprintf(stderr,
	    "\t%s table <tid> [ flush ]\n",
	    progname);
	fprintf(stderr,
	    "\t%s table <tid> { add | rem | test } <address/mask>\n",
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
	uint64_t *st = zalloc(NPF_STATS_SIZE);

	if (ioctl(fd, IOC_NPF_STATS, &st) != 0) {
		err(EXIT_FAILURE, "ioctl(IOC_NPF_STATS)");
	}

	printf("Packets passed:\n\t%"PRIu64" default pass\n\t"
	    "%"PRIu64 " ruleset pass\n\t%"PRIu64" session pass\n\n",
	    st[NPF_STAT_PASS_DEFAULT], st[NPF_STAT_PASS_RULESET],
	    st[NPF_STAT_PASS_SESSION]);

	printf("Packets blocked:\n\t%"PRIu64" default block\n\t"
	    "%"PRIu64 " ruleset block\n\n", st[NPF_STAT_BLOCK_DEFAULT],
	    st[NPF_STAT_BLOCK_RULESET]);

	printf("Session and NAT entries:\n\t%"PRIu64" session allocations\n\t"
	    "%"PRIu64" session destructions\n\t%"PRIu64" NAT entry allocations\n\t"
	    "%"PRIu64" NAT entry destructions\n\n", st[NPF_STAT_SESSION_CREATE],
	    st[NPF_STAT_SESSION_DESTROY], st[NPF_STAT_NAT_CREATE],
	    st[NPF_STAT_NAT_DESTROY]);

	printf("Invalid packet state cases:\n\t%"PRIu64" cases in total\n\t"
	    "%"PRIu64" TCP case I\n\t%"PRIu64" TCP case II\n\t%"PRIu64
	    " TCP case III\n\n", st[NPF_STAT_INVALID_STATE],
	    st[NPF_STAT_INVALID_STATE_TCP1], st[NPF_STAT_INVALID_STATE_TCP2],
	    st[NPF_STAT_INVALID_STATE_TCP3]);

	printf("Packet race cases:\n\t%"PRIu64" NAT association race\n\t"
	    "%"PRIu64" duplicate session race\n\n", st[NPF_STAT_RACE_NAT],
	    st[NPF_STAT_RACE_SESSION]);

	printf("Rule processing procedure cases:\n"
	    "\t%"PRIu64" packets logged\n\t%"PRIu64" packets normalized\n\n",
	    st[NPF_STAT_RPROC_LOG], st[NPF_STAT_RPROC_NORM]);

	printf("Fragmentation:\n"
	    "\t%"PRIu64" fragments\n\t%"PRIu64" reassembled\n"
	    "\t%"PRIu64" failed reassembly\n\n",
	    st[NPF_STAT_FRAGMENTS], st[NPF_STAT_REASSEMBLY],
	    st[NPF_STAT_REASSFAIL]);

	printf("Unexpected error cases:\n\t%"PRIu64"\n", st[NPF_STAT_ERROR]);

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

static void
npfctl_table(int fd, char **argv)
{
	static const struct tblops_s {
		const char *	cmd;
		int		action;
	} tblops[] = {
		{ "add",	NPF_IOCTL_TBLENT_ADD },
		{ "rem",	NPF_IOCTL_TBLENT_REM },
		{ "test",	0 },
		{ NULL,		0 }
	};
	npf_ioctl_table_t nct;
	fam_addr_mask_t fam;
	char *cmd = argv[3];
	char *arg = argv[3];
	int n, alen;

	memset(&nct, 0, sizeof(npf_ioctl_table_t));
	nct.nct_tid = atoi(argv[2]);

	for (n = 0; tblops[n].cmd != NULL; n++) {
		if (strcmp(cmd, tblops[n].cmd) == 0) {
			nct.nct_action = tblops[n].action;
			arg = argv[4];
			break;
		}
	}
	if (!npfctl_parse_cidr(arg, &fam, &alen)) {
		errx(EXIT_FAILURE, "invalid CIDR '%s'", arg);
	}
	memcpy(&nct.nct_addr, &fam.fam_addr, sizeof(npf_addr_t));
	nct.nct_mask = fam.fam_mask;
	nct.nct_alen = alen;

	if (ioctl(fd, IOC_NPF_TABLE, &nct) != -1) {
		errno = 0;
	}
	switch (errno) {
	case EEXIST:
		warnx("entry already exists or is conflicting");
		break;
	case ENOENT:
		warnx("no matching entry was not found");
		break;
	case EINVAL:
		warnx("invalid address, mask or table ID");
		break;
	case 0:
		printf("%s: %s\n", getprogname(), nct.nct_action == 0 ?
		    "matching entry found" : "success");
		break;
	default:
		warn("error");
	}
	exit(errno ? EXIT_FAILURE : EXIT_SUCCESS);
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
		ret = npfctl_config_send(fd);
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
	case NPFCTL_TABLE:
		if (argc < 5) {
			usage();
		}
		npfctl_table(fd, argv);
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
		npfctl_config_init(true);
		npfctl_parsecfg(cfg);
		npfctl_config_send(0);
		return EXIT_SUCCESS;
	}

	/* Find and call the subroutine. */
	for (int n = 0; operations[n].cmd != NULL; n++) {
		if (strcmp(cmd, operations[n].cmd) != 0)
			continue;
		npfctl(operations[n].action, argc, argv);
		return EXIT_SUCCESS;
	}
	usage();
}
