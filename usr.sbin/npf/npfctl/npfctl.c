/*	$NetBSD: npfctl.c,v 1.3 2010/12/18 01:07:26 rmind Exp $	*/

/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: npfctl.c,v 1.3 2010/12/18 01:07:26 rmind Exp $");

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>

#include "npfctl.h"

#define	NPFCTL_START		1
#define	NPFCTL_STOP		2
#define	NPFCTL_RELOAD		3
#define	NPFCTL_FLUSH		4
#define	NPFCTL_TABLE		5
#define	NPFCTL_STATS		6
#define	NPFCTL_SESSIONS_SAVE	7
#define	NPFCTL_SESSIONS_LOAD	8

static struct operations_s {
	const char *		cmd;
	int			action;
} operations[] = {
	/* Start, stop, reload */
	{	"start",		NPFCTL_START		},
	{	"stop",			NPFCTL_STOP		},
	{	"reload",		NPFCTL_RELOAD		},
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
	void *p;

	p = malloc(sz);
	if (p == NULL) {
		perror("zalloc");
		exit(EXIT_FAILURE);
	}
	memset(p, 0, sz);
	return p;
}

char *
xstrdup(const char *s)
{
	char *p;

	p = strdup(s);
	if (p == NULL) {
		perror("xstrdup");
		exit(EXIT_FAILURE);
	}
	return p;
}

static void
usage(void)
{
	const char *progname = getprogname();

	fprintf(stderr,
	    "usage:\t%s [ start | stop | reload | flush | stats ]\n",
	    progname);
	fprintf(stderr,
	    "usage:\t%s [ sess-save | sess-load ]\n",
	    progname);
	fprintf(stderr,
	    "\t%s table <tid> [ flush ]\n",
	    progname);
	fprintf(stderr,
	    "\t%s table <tid> { add | rem } <address/mask>\n",
	    progname);

	exit(EXIT_FAILURE);
}

static void
npfctl_parsecfg(const char *cfg)
{
	char *buf, *p;
	FILE *fp;
	size_t n;
	int l;

	fp = fopen(cfg, "r");
	if (fp == NULL) {
		err(EXIT_FAILURE, "fopen");
	}
	l = 0;
	buf = NULL;
	while (getline(&buf, &n, fp) != -1) {
		l++;
		p = strpbrk(buf, "#\n");
		if (p != NULL) {
			*p = '\0';
		}
		if (npf_parseline(buf)) {
			fprintf(stderr, "invalid syntax at line %d\n", l);
			exit(EXIT_FAILURE);
		}
	}
	if (buf != NULL) {
		free(buf);
	}
}

static int
npfctl_print_stats(int fd)
{
	uint64_t *st = malloc(NPF_STATS_SIZE);

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
	    "%"PRIu64" duplicate session race\n", st[NPF_STAT_RACE_NAT],
	    st[NPF_STAT_RACE_SESSION]);

	free(st);
	return 0;
}

static void
npfctl(int action, int argc, char **argv)
{
	int fd, ret, ver, boolval;
	npf_ioctl_table_t tbl;
	char *arg;

#ifndef DEBUG
	fd = open(NPF_DEV_PATH, O_RDONLY);
	if (fd == -1) {
		err(EXIT_FAILURE, "cannot open " NPF_DEV_PATH);
	}
	ret = ioctl(fd, IOC_NPF_VERSION, &ver);
	if (ver != NPF_VERSION) {
		errx(EXIT_FAILURE, "incompatible npf interface version "
		    "(%d, kernel %d)", NPF_VERSION, ver);
	}
#endif
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
		npfctl_init_data();
#ifdef DEBUG
		npfctl_parsecfg("npf.conf");
		return npfctl_ioctl_send(0);
#endif
		npfctl_parsecfg(argc < 3 ? NPF_CONF_PATH : argv[2]);
		ret = npfctl_ioctl_send(fd);
		break;
	case NPFCTL_FLUSH:
		/* Pass empty configuration to flush. */
		npfctl_init_data();
		ret = npfctl_ioctl_send(fd);
		break;
	case NPFCTL_TABLE:
		if (argc < 5) {
			usage();
		}
		tbl.nct_tid = atoi(argv[2]);
		if (strcmp(argv[3], "add") == 0) {
			tbl.nct_action = NPF_IOCTL_TBLENT_ADD;
			arg = argv[4];
		} else if (strcmp(argv[3], "rem") == 0) {
			tbl.nct_action = NPF_IOCTL_TBLENT_REM;
			arg = argv[4];
		} else {
			tbl.nct_action = 0;
			arg = argv[3];
		}
		if (!npfctl_parse_v4mask(arg,
		    &tbl.nct_addr, &tbl.nct_mask)) {
			errx(EXIT_FAILURE, "invalid CIDR '%s'", arg);
		}
		ret = ioctl(fd, IOC_NPF_TABLE, &tbl);
		break;
	case NPFCTL_STATS:
		ret = npfctl_print_stats(fd);
		break;
	case NPFCTL_SESSIONS_SAVE:
		ret = npfctl_ioctl_recvse(fd);
		break;
	case NPFCTL_SESSIONS_LOAD:
		ret = npfctl_ioctl_sendse(fd);
		break;
	}
	if (ret == -1) {
		err(EXIT_FAILURE, "ioctl");
	}
	close(fd);
}

int
main(int argc, char **argv)
{
	char *cmd;
	int n;

	if (argc < 2) {
		usage();
	}
	cmd = argv[1];

	/* Find and call the subroutine */
	for (n = 0; operations[n].cmd != NULL; n++) {
		if (strcmp(cmd, operations[n].cmd) != 0)
			continue;
		npfctl(operations[n].action, argc, argv);
		return 0;
	}
	usage();
	return 0;
}
