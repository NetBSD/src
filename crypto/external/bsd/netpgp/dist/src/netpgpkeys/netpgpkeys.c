/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alistair Crooks (agc@NetBSD.org)
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

/* Command line program to perform netpgp operations */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netpgp.h>

/*
 * 2048 is the absolute minimum, really - we should really look at
 * bumping this to 4096 or even higher - agc, 20090522
 */
#define DEFAULT_NUMBITS 2048

static const char *usage =
	" --help OR\n"
	"\t--export-keys [options] OR\n"
	"\t--find-key [options] OR\n"
	"\t--generate-key [options] OR\n"
	"\t--import-key [options] OR\n"
	"\t--list-keys [options] OR\n"
	"\t--get-key keyid [options] OR\n"
	"\t--version\n"
	"where options are:\n"
	"\t[--coredumps] AND/OR\n"
	"\t[--homedir=<homedir>] AND/OR\n"
	"\t[--keyring=<keyring>] AND/OR\n"
	"\t[--userid=<userid>] AND/OR\n"
	"\t[--verbose]\n";

enum optdefs {
	/* commands */
	LIST_KEYS = 1,
	FIND_KEY,
	EXPORT_KEY,
	IMPORT_KEY,
	GENERATE_KEY,
	VERSION_CMD,
	HELP_CMD,
	GET_KEY,

	/* options */
	SSHKEYS,
	KEYRING,
	USERID,
	HOMEDIR,
	NUMBITS,
	VERBOSE,
	COREDUMPS,
	PASSWDFD,
	RESULTS,
	SSHKEYFILE,

	/* debug */
	OPS_DEBUG

};

#define EXIT_ERROR	2

static struct option options[] = {
	/* key-management commands */
	{"list-keys",	no_argument,		NULL,	LIST_KEYS},
	{"find-key",	no_argument,		NULL,	FIND_KEY},
	{"export-key",	no_argument,		NULL,	EXPORT_KEY},
	{"import-key",	no_argument,		NULL,	IMPORT_KEY},
	{"generate-key", no_argument,		NULL,	GENERATE_KEY},
	{"get-key", 	no_argument,		NULL,	GET_KEY},
	/* debugging commands */
	{"help",	no_argument,		NULL,	HELP_CMD},
	{"version",	no_argument,		NULL,	VERSION_CMD},
	{"debug",	required_argument, 	NULL,	OPS_DEBUG},
	/* options */
	{"coredumps",	no_argument, 		NULL,	COREDUMPS},
	{"keyring",	required_argument, 	NULL,	KEYRING},
	{"userid",	required_argument, 	NULL,	USERID},
	{"home",	required_argument, 	NULL,	HOMEDIR},
	{"homedir",	required_argument, 	NULL,	HOMEDIR},
	{"numbits",	required_argument, 	NULL,	NUMBITS},
	{"ssh-keys",	no_argument, 		NULL,	SSHKEYS},
	{"sshkeyfile",	required_argument, 	NULL,	SSHKEYFILE},
	{"verbose",	no_argument, 		NULL,	VERBOSE},
	{"pass-fd",	required_argument, 	NULL,	PASSWDFD},
	{"results",	required_argument, 	NULL,	RESULTS},
	{ NULL,		0,			NULL,	0},
};

/* gather up program variables into one struct */
typedef struct prog_t {
	char	 keyring[MAXPATHLEN + 1];	/* name of keyring */
	char	*progname;			/* program name */
	int	 numbits;			/* # of bits */
	int	 cmd;				/* netpgpkeys command */
} prog_t;


/* print a usage message */
static void
print_usage(const char *usagemsg, char *progname)
{
	(void) fprintf(stderr,
	"%s\nAll bug reports, praise and chocolate, please, to:\n%s\n",
				netpgp_get_info("version"),
				netpgp_get_info("maintainer"));
	(void) fprintf(stderr, "Usage: %s COMMAND OPTIONS:\n%s %s",
		progname, progname, usagemsg);
}

/* do a command once for a specified file 'f' */
static int
netpgp_cmd(netpgp_t *netpgp, prog_t *p, char *f)
{
	char	*key;

	switch (p->cmd) {
	case LIST_KEYS:
		return (f == NULL) ? netpgp_list_keys(netpgp) : netpgp_match_list_keys(netpgp, f);
	case FIND_KEY:
		return netpgp_find_key(netpgp, netpgp_getvar(netpgp, "userid"));
	case EXPORT_KEY:
		return netpgp_export_key(netpgp,
				netpgp_getvar(netpgp, "userid"));
	case IMPORT_KEY:
		return netpgp_import_key(netpgp, f);
	case GENERATE_KEY:
		return netpgp_generate_key(netpgp,
				netpgp_getvar(netpgp, "userid"), p->numbits);
	case GET_KEY:
		key = netpgp_get_key(netpgp, f);
		if (key) {
			printf("%s", key);
			return 1;
		}
		(void) fprintf(stderr, "key '%s' not found\n", f);
		return 0;
	case HELP_CMD:
	default:
		print_usage(usage, p->progname);
		exit(EXIT_SUCCESS);
	}
}

int
main(int argc, char **argv)
{
	netpgp_t	netpgp;
	prog_t          p;
	int             optindex;
	int             ret;
	int             ch;
	int             i;

	(void) memset(&p, 0x0, sizeof(p));
	(void) memset(&netpgp, 0x0, sizeof(netpgp));
	p.progname = argv[0];
	p.numbits = DEFAULT_NUMBITS;
	if (argc < 2) {
		print_usage(usage, p.progname);
		exit(EXIT_ERROR);
	}
	/* set some defaults */
	netpgp_set_homedir(&netpgp, getenv("HOME"), "/.gnupg", 1);
	netpgp_setvar(&netpgp, "sshkeydir", "/etc/ssh");
	optindex = 0;
	while ((ch = getopt_long(argc, argv, "", options, &optindex)) != -1) {
		switch (options[optindex].val) {
		case LIST_KEYS:
			p.cmd = options[optindex].val;
			break;
		case COREDUMPS:
			netpgp_setvar(&netpgp, "coredumps", "allowed");
			p.cmd = options[optindex].val;
			break;
		case GENERATE_KEY:
			netpgp_setvar(&netpgp, "userid checks", "skip");
			p.cmd = options[optindex].val;
			break;
		case FIND_KEY:
		case EXPORT_KEY:
		case IMPORT_KEY:
		case GET_KEY:
		case HELP_CMD:
			p.cmd = options[optindex].val;
			break;
		case VERSION_CMD:
			printf(
"%s\nAll bug reports, praise and chocolate, please, to:\n%s\n",
				netpgp_get_info("version"),
				netpgp_get_info("maintainer"));
			exit(EXIT_SUCCESS);
			/* options */
		case SSHKEYS:
			netpgp_setvar(&netpgp, "ssh keys", "1");
			break;
		case KEYRING:
			if (optarg == NULL) {
				(void) fprintf(stderr,
					"%s: No keyring argument provided\n",
					*argv);
				exit(EXIT_ERROR);
			}
			snprintf(p.keyring, sizeof(p.keyring), "%s", optarg);
			break;
		case USERID:
			if (optarg == NULL) {
				(void) fprintf(stderr,
					"%s: no userid argument provided\n",
					*argv);
				exit(EXIT_ERROR);
			}
			netpgp_setvar(&netpgp, "userid", optarg);
			break;
		case VERBOSE:
			netpgp_incvar(&netpgp, "verbose", 1);
			break;
		case HOMEDIR:
			if (optarg == NULL) {
				(void) fprintf(stderr,
				"%s: no home directory argument provided\n",
				*argv);
				exit(EXIT_ERROR);
			}
			netpgp_set_homedir(&netpgp, optarg, NULL, 0);
			break;
		case NUMBITS:
			if (optarg == NULL) {
				(void) fprintf(stderr,
				"%s: no number of bits argument provided\n",
				*argv);
				exit(EXIT_ERROR);
			}
			p.numbits = atoi(optarg);
			break;
		case PASSWDFD:
			if (optarg == NULL) {
				(void) fprintf(stderr,
				"%s: no pass-fd argument provided\n", *argv);
				exit(EXIT_ERROR);
			}
			netpgp_setvar(&netpgp, "pass-fd", optarg);
			break;
		case RESULTS:
			if (optarg == NULL) {
				(void) fprintf(stderr,
				"No output filename argument provided\n");
				exit(EXIT_ERROR);
			}
			netpgp_setvar(&netpgp, "results", optarg);
			break;
		case SSHKEYFILE:
			netpgp_setvar(&netpgp, "sshkeyfile", optarg);
			break;
		case OPS_DEBUG:
			netpgp_set_debug(optarg);
			break;
		default:
			p.cmd = HELP_CMD;
			break;
		}
	}
	/* initialise, and read keys from file */
	if (!netpgp_init(&netpgp)) {
		printf("can't initialise\n");
		exit(EXIT_ERROR);
	}
	/* now do the required action for each of the command line args */
	ret = EXIT_SUCCESS;
	if (optind == argc) {
		if (!netpgp_cmd(&netpgp, &p, NULL)) {
			ret = EXIT_FAILURE;
		}
	} else {
		for (i = optind; i < argc; i++) {
			if (!netpgp_cmd(&netpgp, &p, argv[i])) {
				ret = EXIT_FAILURE;
			}
		}
	}
	netpgp_end(&netpgp);
	exit(ret);
}
