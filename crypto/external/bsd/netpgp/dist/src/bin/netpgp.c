/*
 * Copyright (c) 2005-2008 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer. The Contributors have asserted
 * their moral rights under the UK Copyright Design and Patents Act 1988 to
 * be recorded as the authors of this copyright work.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 \file Command line program to perform netpgp operations
*/
#include <sys/types.h>
#include <sys/param.h>

#include <getopt.h>
#include <libgen.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netpgp.h>

#define DEFAULT_NUMBITS 2048

#define MAXBUF 1024

static const char *usage =
	" --help OR\n"
	"\t--list-keys [options] OR\n"
	"\t--list-packets [options] OR\n"
	"\t--encrypt [options] files... OR\n"
	"\t--decrypt [options] files... OR\n"
	"\t--sign [--detach] [options] files... OR\n"
	"\t--clearsign [options] files... OR\n"
	"\t--verify [options] files...\n"
	"where options are:\n"
	"\t[--keyring=<keyring>] AND/OR\n"
	"\t[--userid=<userid>] AND/OR\n"
	"\t[--armour] AND/OR\n"
	"\t[--homedir=<homedir>]\n";
static const char *usage_find_key =
	"%s --find-key --userid=<userid> [--keyring=<keyring>] \n";
static const char *usage_export_key =
	"%s --export-key --userid=<userid> [--keyring=<keyring>] \n";
static const char *usage_encrypt =
"%s --encrypt --userid=<userid> [--armour] [--homedir=<homedir>] files...\n";
static const char *usage_sign =
"%s --sign --userid=<userid> [--armour] [--homedir=<homedir>] files...\n";
static const char *usage_clearsign =
"%s --clearsign --userid=<userid> [--homedir=<homedir>] files...\n";

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
	VERIFY_SHOW,
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

	/* debug */
	OPS_DEBUG

};


#define EXIT_ERROR	2

static struct option options[] = {
	/* commands */
	{"list-keys",	no_argument,		NULL,	LIST_KEYS},
	{"find-key",	no_argument,		NULL,	FIND_KEY},
	{"export-key",	no_argument,		NULL,	EXPORT_KEY},
	{"import-key",	no_argument,		NULL,	IMPORT_KEY},
	{"generate-key", no_argument,		NULL,	GENERATE_KEY},

	{"encrypt",	no_argument,		NULL,	ENCRYPT},
	{"decrypt",	no_argument,		NULL,	DECRYPT},
	{"sign",	no_argument,		NULL,	SIGN},
	{"clearsign",	no_argument,		NULL,	CLEARSIGN},
	{"verify",	no_argument,		NULL,	VERIFY},
	{"verify-show",	no_argument,		NULL,	VERIFY_SHOW},
	{"verifyshow",	no_argument,		NULL,	VERIFY_SHOW},

	{"list-packets", no_argument,		NULL,	LIST_PACKETS},

	{"help",	no_argument,		NULL,	HELP_CMD},
	{"version",	no_argument,		NULL,	VERSION_CMD},

	/* options */
	{"keyring",	required_argument, 	NULL,	KEYRING},
	{"userid",	required_argument, 	NULL,	USERID},
	{"home",	required_argument, 	NULL,	HOMEDIR},
	{"homedir",	required_argument, 	NULL,	HOMEDIR},
	{"armor",	no_argument,		NULL,	ARMOUR},
	{"armour",	no_argument,		NULL,	ARMOUR},
	{"numbits",	required_argument, 	NULL,	NUMBITS},
	{"detach",	no_argument,		NULL,	DETACHED},
	{"detached",	no_argument,		NULL,	DETACHED},

	/* debug */
	{"debug",	required_argument, 	NULL,	OPS_DEBUG},

	{ NULL,		0,			NULL,	0},
};

/* gather up program variables into one struct */
typedef struct prog_t {
	char	 keyring[MAXBUF + 1];		/* name of keyring */
	char	*userid;			/* user identifier */
	char	 myring_name[MAXBUF + 1];	/* myring filename */
	char	 pubring_name[MAXBUF + 1];	/* pubring filename */
	char	 secring_name[MAXBUF + 1];	/* secret ring file */
	char	*progname;			/* program name */
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
	switch (p->cmd) {
	case LIST_KEYS:
		return netpgp_list_keys(netpgp);
	case FIND_KEY:
		return netpgp_find_key(netpgp, p->userid);
	case EXPORT_KEY:
		return netpgp_export_key(netpgp, p->userid);
	case IMPORT_KEY:
		return netpgp_import_key(netpgp, f);
	case GENERATE_KEY:
		return netpgp_generate_key(netpgp, p->userid, p->numbits);
	case ENCRYPT:
		return netpgp_encrypt_file(netpgp, p->userid, f, NULL,
					p->armour);
	case DECRYPT:
		return netpgp_decrypt_file(netpgp, f, NULL, p->armour);
	case SIGN:
		return netpgp_sign_file(netpgp, p->userid, f, NULL, p->armour,
					0, p->detached);
	case CLEARSIGN:
		return netpgp_sign_file(netpgp, p->userid, f, NULL, p->armour,
					1, p->detached);
	case VERIFY:
		return netpgp_verify_file(netpgp, f, NULL, p->armour);
	case VERIFY_SHOW:
		return netpgp_verify_file(netpgp, f, "-", p->armour);
	case LIST_PACKETS:
		return netpgp_list_packets(netpgp, f, p->armour, NULL);
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
	char            homedir[MAXPATHLEN];
	int             optindex;
	int             ret;
	int             ch;
	int             i;

	(void) memset(&p, 0x0, sizeof(p));
	(void) memset(homedir, 0x0, sizeof(homedir));
	(void) memset(&netpgp, 0x0, sizeof(netpgp));
	p.progname = argv[0];
	p.numbits = DEFAULT_NUMBITS;
	p.overwrite = 1;
	if (argc < 2) {
		print_usage(usage, p.progname);
		exit(EXIT_ERROR);
	}

	/* set default homedir */
	(void) snprintf(homedir, sizeof(homedir), "%s/.gnupg", getenv("HOME"));

	optindex = 0;
	while ((ch = getopt_long(argc, argv, "", options, &optindex)) != -1) {
		switch (options[optindex].val) {
		case LIST_KEYS:
			p.cmd = options[optindex].val;
			break;

		case FIND_KEY:
		case EXPORT_KEY:
		case IMPORT_KEY:
		case GENERATE_KEY:
		case ENCRYPT:
		case DECRYPT:
		case SIGN:
		case CLEARSIGN:
		case VERIFY:
		case VERIFY_SHOW:
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
			if (netpgp_get_debug(__FILE__)) {
				(void) fprintf(stderr,
					"userid is '%s'\n", optarg);
			}
			p.userid = optarg;
			break;

		case ARMOUR:
			p.armour = 1;
			break;

		case DETACHED:
			p.detached = 1;
			break;

		case HOMEDIR:
			if (optarg == NULL) {
				(void) fprintf(stderr,
				"No home directory argument provided\n");
				exit(EXIT_ERROR);
			}
			(void) snprintf(homedir, sizeof(homedir), "%s", optarg);
			break;

		case NUMBITS:
			if (optarg == NULL) {
				(void) fprintf(stderr,
				"No number of bits argument provided\n");
				exit(EXIT_ERROR);
			}
			p.numbits = atoi(optarg);
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
	if (!netpgp_init(&netpgp, p.userid, NULL, NULL)) {
		printf("can't initialise\n");
		exit(EXIT_ERROR);
	}

	/*
	 * now do the required action for each of the files on the command
	 * line
	 */
	if (optind == argc) {
		ret = netpgp_cmd(&netpgp, &p, NULL);
	} else {
		for (ret = EXIT_SUCCESS, i = optind; i < argc; i++) {
			if (!netpgp_cmd(&netpgp, &p, argv[i])) {
				ret = EXIT_FAILURE;
			}
		}
	}
	netpgp_end(&netpgp);
	exit(ret);
}
