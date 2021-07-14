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
__RCSID("$NetBSD: npfctl.c,v 1.65 2021/07/14 09:15:01 christos Exp $");

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/un.h>
#ifdef __NetBSD__
#include <sys/module.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#include "npfctl.h"

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
	NPFCTL_DEBUG,
	NPFCTL_CONN_LIST,
};

bool
join(char *buf, size_t buflen, int count, char **args, const char *sep)
{
	const unsigned seplen = strlen(sep);
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

__dead void
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
	    "\t%s table \"table-name\" { add | rem | test } <address/mask>\n",
	    progname);
	fprintf(stderr,
	    "\t%s table \"table-name\" { list | flush }\n",
	    progname);
	fprintf(stderr,
	    "\t%s table \"table-name\" replace [-n \"name\"]"
	    " [-t <type>] <table-file>\n",
	    progname);
	fprintf(stderr,
	    "\t%s save | load\n",
	    progname);
	fprintf(stderr,
	    "\t%s list [-46hNnw] [-i <ifname>]\n",
	    progname);
	fprintf(stderr,
	    "\t%s debug { -a | -b <binary-config> | -c <config> } "
	    "[ -o <outfile> ]\n",
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
npfctl_print_error(const npf_error_t *ne)
{
	const char *srcfile = ne->source_file;

	if (ne->error_msg) {
		errx(EXIT_FAILURE, "%s", ne->error_msg);
	}
	if (srcfile) {
		warnx("source %s line %d", srcfile, ne->source_line);
	}
	if (ne->id) {
		warnx("object: %" PRIi64, ne->id);
	}
}

char *
npfctl_print_addrmask(int alen, const char *fmt, const npf_addr_t *addr,
    npf_netmask_t mask)
{
	const unsigned buflen = 256;
	char *buf = ecalloc(1, buflen);
	struct sockaddr_storage ss;

	memset(&ss, 0, sizeof(ss));

	switch (alen) {
	case 4: {
		struct sockaddr_in *sin = (void *)&ss;
		sin->sin_family = AF_INET;
		memcpy(&sin->sin_addr, addr, sizeof(sin->sin_addr));
		break;
	}
	case 16: {
		struct sockaddr_in6 *sin6 = (void *)&ss;
		sin6->sin6_family = AF_INET6;
		memcpy(&sin6->sin6_addr, addr, sizeof(sin6->sin6_addr));
		break;
	}
	default:
		abort();
	}
	sockaddr_snprintf(buf, buflen, fmt, (const void *)&ss);
	if (mask && mask != NPF_NO_NETMASK) {
		const unsigned len = strlen(buf);
		snprintf(&buf[len], buflen - len, "/%u", mask);
	}
	return buf;
}

bool
npfctl_addr_iszero(const npf_addr_t *addr)
{
	static const npf_addr_t zero; /* must be static */
	return memcmp(addr, &zero, sizeof(npf_addr_t)) == 0;
}

static bool bpfjit = true;

void
npfctl_bpfjit(bool onoff)
{
	bpfjit = onoff;
}

static void
npfctl_preload_bpfjit(void)
{
#ifdef __NetBSD__
	modctl_load_t args = {
		.ml_filename = "bpfjit",
		.ml_flags = MODCTL_NO_PROP,
		.ml_props = NULL,
		.ml_propslen = 0
	};

	if (!bpfjit)
		return;

	if (modctl(MODCTL_LOAD, &args) != 0 && errno != EEXIST) {
		static const char *p = "; performance will be degraded";
		if (errno == ENOENT)
			warnx("the bpfjit module seems to be missing%s", p);
		else
			warn("error loading the bpfjit module%s", p);
		warnx("To disable this warning `set bpf.jit off' in "
		    "/etc/npf.conf");
	}
#endif
}

static nl_config_t *
npfctl_import(const char *path)
{
	nl_config_t *ncf;
	struct stat sb;
	size_t blen;
	void *blob;
	int fd;

	/*
	 * The file may change while reading - we are not handling this,
	 * just leaving this responsibility for the caller.
	 */
	if ((fd = open(path, O_RDONLY)) == -1) {
		err(EXIT_FAILURE, "open: '%s'", path);
	}
	if (fstat(fd, &sb) == -1) {
		err(EXIT_FAILURE, "stat: '%s'", path);
	}
	if ((blen = sb.st_size) == 0) {
		errx(EXIT_FAILURE,
		    "the binary configuration file '%s' is empty", path);
	}
	blob = mmap(NULL, blen, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0);
	if (blob == MAP_FAILED) {
		err(EXIT_FAILURE, "mmap: '%s'", path);
	}
	ncf = npf_config_import(blob, blen);
	munmap(blob, blen);
	return ncf;
}

static int
npfctl_load(int fd)
{
	nl_config_t *ncf;
	npf_error_t errinfo;

	/*
	 * Import the configuration, submit it and destroy.
	 */
	ncf = npfctl_import(NPF_DB_PATH);
	if (ncf == NULL) {
		err(EXIT_FAILURE, "npf_config_import: '%s'", NPF_DB_PATH);
	}
	if ((errno = npf_config_submit(ncf, fd, &errinfo)) != 0) {
		npfctl_print_error(&errinfo);
	}
	npf_config_destroy(ncf);
	return errno;
}

static int
npfctl_open_dev(const char *path)
{
	struct stat st;
	int fd;

	if (lstat(path, &st) == -1) {
		err(EXIT_FAILURE, "fstat: '%s'", path);
	}
	if ((st.st_mode & S_IFMT) == S_IFSOCK) {
		struct sockaddr_un addr;

		if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
			err(EXIT_FAILURE, "socket");
		}
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

		if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
			err(EXIT_FAILURE, "connect: '%s'", path);
		}
	} else {
		if ((fd = open(path, O_RDONLY)) == -1) {
			err(EXIT_FAILURE, "open: '%s'", path);
		}
	}
	return fd;
}

static void
npfctl_debug(int argc, char **argv)
{
	const char *conf = NULL, *bconf = NULL, *outfile = NULL;
	bool use_active = false;
	nl_config_t *ncf = NULL;
	int fd, c, optcount;

	argc--;
	argv++;

	npfctl_config_init(true);
	while ((c = getopt(argc, argv, "ab:c:o:")) != -1) {
		switch (c) {
		case 'a':
			use_active = true;
			break;
		case 'b':
			bconf = optarg;
			break;
		case 'c':
			conf = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		default:
			usage();
		}
	}

	/*
	 * Options -a, -b and -c are mutually exclusive, so allow only one.
	 * If no options were specified, then set the defaults.
	 */
	optcount = (int)!!use_active + (int)!!conf + (int)!!bconf;
	if (optcount != 1) {
		if (optcount > 1) {
			usage();
		}
		conf = NPF_CONF_PATH;
		outfile = outfile ? outfile : "npf.nvlist";
	}

	if (use_active) {
		puts("Loading the active configuration");
		fd = npfctl_open_dev(NPF_DEV_PATH);
		if ((ncf = npf_config_retrieve(fd)) == NULL) {
			err(EXIT_FAILURE, "npf_config_retrieve: '%s'",
			    NPF_DEV_PATH);
		}
	}

	if (conf) {
		printf("Loading %s\n", conf);
		npfctl_parse_file(conf);
		npfctl_config_build();
		ncf = npfctl_config_ref();
	}

	if (bconf) {
		printf("Importing %s\n", bconf);
		ncf = npfctl_import(bconf);
	}

	printf("Configuration:\n\n");
	_npf_config_dump(ncf, STDOUT_FILENO);
	if (outfile) {
		printf("\nSaving binary to %s\n", outfile);
		npfctl_config_save(ncf, outfile);
	}
	npf_config_destroy(ncf);
}

static void
npfctl(int action, int argc, char **argv)
{
	int fd, boolval, ret = 0;
	const char *fun = "";
	nl_config_t *ncf;

	switch (action) {
	case NPFCTL_VALIDATE:
	case NPFCTL_DEBUG:
		fd = 0;
		break;
	default:
		fd = npfctl_open_dev(NPF_DEV_PATH);
	}

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
		npfctl_preload_bpfjit();
		errno = ret = npfctl_config_send(fd);
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
	case NPFCTL_TABLE:
		if ((argc -= 2) < 2) {
			usage();
		}
		argv += 2;
		if (strcmp(argv[1], "replace") == 0) {
			npfctl_table_replace(fd, argc, argv);
		} else {
			npfctl_table(fd, argc, argv);
		}
		break;
	case NPFCTL_RULE:
		if ((argc -= 2) < 2) {
			usage();
		}
		argv += 2;
		npfctl_rule(fd, argc, argv);
		break;
	case NPFCTL_LOAD:
		npfctl_preload_bpfjit();
		ret = npfctl_load(fd);
		fun = "npfctl_config_load";
		break;
	case NPFCTL_SAVE:
		ncf = npf_config_retrieve(fd);
		if (ncf) {
			npfctl_config_save(ncf,
			    argc > 2 ? argv[2] : NPF_DB_PATH);
			npf_config_destroy(ncf);
		} else {
			ret = errno;
		}
		fun = "npfctl_config_save";
		break;
	case NPFCTL_STATS:
		ret = npfctl_print_stats(fd);
		fun = "npfctl_print_stats";
		break;
	case NPFCTL_CONN_LIST:
		ret = npfctl_conn_list(fd, argc, argv);
		fun = "npfctl_conn_list";
		break;
	case NPFCTL_VALIDATE:
		npfctl_config_init(false);
		npfctl_parse_file(argc > 2 ? argv[2] : NPF_CONF_PATH);
		ret = npfctl_config_show(0);
		fun = "npfctl_config_show";
		break;
	case NPFCTL_DEBUG:
		npfctl_debug(argc, argv);
		break;
	}
	if (ret) {
		err(EXIT_FAILURE, "%s", fun);
	}
	if (fd) {
		close(fd);
	}
}

int
main(int argc, char **argv)
{
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
		/* Table */
		{	"table",	NPFCTL_TABLE		},
		/* Rule */
		{	"rule",		NPFCTL_RULE		},
		/* Stats */
		{	"stats",	NPFCTL_STATS		},
		/* Full state save/load */
		{	"save",		NPFCTL_SAVE		},
		{	"load",		NPFCTL_LOAD		},
		{	"list",		NPFCTL_CONN_LIST	},
		/* Misc. */
		{	"valid",	NPFCTL_VALIDATE		},
		{	"debug",	NPFCTL_DEBUG		},
		/* --- */
		{	NULL,		0			}
	};
	char *cmd;

	if (argc < 2) {
		usage();
	}
	cmd = argv[1];

	/* Find and call the subroutine. */
	for (int n = 0; operations[n].cmd != NULL; n++) {
		const char *opcmd = operations[n].cmd;

		if (strncmp(cmd, opcmd, strlen(opcmd)) != 0) {
			continue;
		}
		npfctl(operations[n].action, argc, argv);
		return EXIT_SUCCESS;
	}
	usage();
}
