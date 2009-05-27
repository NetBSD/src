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

/*
 * Similraily, SHA1 is now looking as though it should not be used.
 * Let's pre-empt this by specifying SHA256 - gpg interoperates just
 * fine with SHA256 - agc, 20090522
 */
#define DEFAULT_HASH_ALG	"SHA256"

static const char *usage =
	" --help OR\n"
	"\t--encrypt [--output=file] [options] files... OR\n"
	"\t--decrypt [--output=file] [options] files... OR\n\n"
	"\t--sign [--armor] [--detach] [--hash=alg] [--output=file]\n"
		"\t\t[options] files... OR\n"
	"\t--verify [options] files... OR\n"
	"\t--cat [--output=file] [options] files... OR\n"
	"\t--clearsign [--output=file] [options] files... OR\n\n"
	"\t--export-keys [options] OR\n"
	"\t--find-key [options] OR\n"
	"\t--generate-key [options] OR\n"
	"\t--import-key [options] OR\n"
	"\t--list-keys [options] OR\n\n"
	"\t--list-packets [options] OR\n"
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
	ENCRYPT,
	DECRYPT,
	SIGN,
	CLEARSIGN,
	VERIFY,
	VERIFY_CAT,
	LIST_PACKETS,
	VERSION_CMD,
	HELP_CMD,

	/* options */
	KEYRING,
	USERID,
	ARMOUR,
	HOMEDIR,
	NUMBITS,
	DETACHED,
	HASH_ALG,
	OUTPUT,
	VERBOSE,
	COREDUMPS,

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
	/* file manipulation commands */
	{"encrypt",	no_argument,		NULL,	ENCRYPT},
	{"decrypt",	no_argument,		NULL,	DECRYPT},
	{"sign",	no_argument,		NULL,	SIGN},
	{"clearsign",	no_argument,		NULL,	CLEARSIGN},
	{"verify",	no_argument,		NULL,	VERIFY},
	{"cat",		no_argument,		NULL,	VERIFY_CAT},
	{"vericat",	no_argument,		NULL,	VERIFY_CAT},
	{"verify-cat",	no_argument,		NULL,	VERIFY_CAT},
	{"verify-show",	no_argument,		NULL,	VERIFY_CAT},
	{"verifyshow",	no_argument,		NULL,	VERIFY_CAT},
	/* file listing commands */
	{"list-packets", no_argument,		NULL,	LIST_PACKETS},
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
	{"armor",	no_argument,		NULL,	ARMOUR},
	{"armour",	no_argument,		NULL,	ARMOUR},
	{"numbits",	required_argument, 	NULL,	NUMBITS},
	{"detach",	no_argument,		NULL,	DETACHED},
	{"detached",	no_argument,		NULL,	DETACHED},
	{"hash-alg",	required_argument, 	NULL,	HASH_ALG},
	{"hash",	required_argument, 	NULL,	HASH_ALG},
	{"algorithm",	required_argument, 	NULL,	HASH_ALG},
	{"verbose",	no_argument, 		NULL,	VERBOSE},
	{"output",	required_argument, 	NULL,	OUTPUT},
	{ NULL,		0,			NULL,	0},
};

/* gather up program variables into one struct */
typedef struct prog_t {
	char	 keyring[MAXPATHLEN + 1];	/* name of keyring */
	char	*progname;			/* program name */
	char	*output;			/* output file name */
	int	 overwrite;			/* overwrite files? */
	int	 numbits;			/* # of bits */
	int	 armour;			/* ASCII armor */
	int	 detached;			/* use separate file */
	int	 cmd;				/* netpgp command */
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
	const int	cleartext = 1;

	switch (p->cmd) {
	case LIST_KEYS:
		return netpgp_list_keys(netpgp);
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
	case ENCRYPT:
		return netpgp_encrypt_file(netpgp,
					netpgp_getvar(netpgp, "userid"),
					f, p->output,
					p->armour);
	case DECRYPT:
		return netpgp_decrypt_file(netpgp, f, p->output, p->armour);
	case SIGN:
		return netpgp_sign_file(netpgp,
					netpgp_getvar(netpgp, "userid"),
					f, p->output,
					p->armour, !cleartext, p->detached);
	case CLEARSIGN:
		return netpgp_sign_file(netpgp,
					netpgp_getvar(netpgp, "userid"),
					f, p->output,
					p->armour, cleartext, p->detached);
	case VERIFY:
		return netpgp_verify_file(netpgp, f, NULL, p->armour);
	case VERIFY_CAT:
		return netpgp_verify_file(netpgp, f,
					(p->output) ? p->output : "-",
					p->armour);
	case LIST_PACKETS:
		return netpgp_list_packets(netpgp, f, p->armour, NULL);
	case HELP_CMD:
	default:
		print_usage(usage, p->progname);
		exit(EXIT_SUCCESS);
	}
}

/* get even more lippy */
static void
give_it_large(netpgp_t *netpgp)
{
	char	*cp;
	char	 num[16];
	int	 val;

	val = 0;
	if ((cp = netpgp_getvar(netpgp, "verbose")) != NULL) {
		val = atoi(cp);
	}
	(void) snprintf(num, sizeof(num), "%d", val + 1);
	netpgp_setvar(netpgp, "verbose", num);
}

/* set the home directory value to "home/subdir" */
static int
set_homedir(netpgp_t *netpgp, char *home, const char *subdir)
{
	struct stat	st;
	char		d[MAXPATHLEN];

	if (home == NULL) {
		(void) fprintf(stderr, "NULL HOME directory\n");
		return 0;
	}
	(void) snprintf(d, sizeof(d), "%s%s", home, (subdir) ? subdir : "");
	if (stat(d, &st) == 0) {
		if ((st.st_mode & S_IFMT) == S_IFDIR) {
			netpgp_setvar(netpgp, "homedir", d);
			return 1;
		}
		(void) fprintf(stderr, "netpgp: homedir \"%s\" is not a dir\n",
					d);
		return 0;
	}
	(void) fprintf(stderr, "netpgp: warning homedir \"%s\" not found\n", d);
	return 1;
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
	p.overwrite = 1;
	p.output = NULL;
	if (argc < 2) {
		print_usage(usage, p.progname);
		exit(EXIT_ERROR);
	}
	/* set some defaults */
	netpgp_setvar(&netpgp, "hash", DEFAULT_HASH_ALG);
	set_homedir(&netpgp, getenv("HOME"), "/.gnupg");
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
		case ENCRYPT:
		case DECRYPT:
		case SIGN:
		case CLEARSIGN:
		case VERIFY:
		case VERIFY_CAT:
		case LIST_PACKETS:
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
		case KEYRING:
			if (optarg == NULL) {
				(void) fprintf(stderr,
					"No keyring argument provided\n");
				exit(EXIT_ERROR);
			}
			snprintf(p.keyring, sizeof(p.keyring), "%s", optarg);
			break;
		case USERID:
			if (optarg == NULL) {
				(void) fprintf(stderr,
					"No userid argument provided\n");
				exit(EXIT_ERROR);
			}
			netpgp_setvar(&netpgp, "userid", optarg);
			break;
		case ARMOUR:
			p.armour = 1;
			break;
		case DETACHED:
			p.detached = 1;
			break;
		case VERBOSE:
			give_it_large(&netpgp);
			break;
		case HOMEDIR:
			if (optarg == NULL) {
				(void) fprintf(stderr,
				"No home directory argument provided\n");
				exit(EXIT_ERROR);
			}
			set_homedir(&netpgp, optarg, NULL);
			break;
		case NUMBITS:
			if (optarg == NULL) {
				(void) fprintf(stderr,
				"No number of bits argument provided\n");
				exit(EXIT_ERROR);
			}
			p.numbits = atoi(optarg);
			break;
		case HASH_ALG:
			if (optarg == NULL) {
				(void) fprintf(stderr,
				"No hash algorithm argument provided\n");
				exit(EXIT_ERROR);
			}
			netpgp_setvar(&netpgp, "hash", optarg);
			break;
		case OUTPUT:
			if (optarg == NULL) {
				(void) fprintf(stderr,
				"No output filename argument provided\n");
				exit(EXIT_ERROR);
			}
			if (p.output) {
				(void) free(p.output);
			}
			p.output = strdup(optarg);
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
