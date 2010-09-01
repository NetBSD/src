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
#include <regex.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mj.h>
#include <netpgp.h>

/*
 * 2048 is the absolute minimum, really - we should really look at
 * bumping this to 4096 or even higher - agc, 20090522
 */
#define DEFAULT_NUMBITS 2048

#define DEFAULT_HASH_ALG "SHA256"

static const char *usage =
	" --help OR\n"
	"\t--export-keys [options] OR\n"
	"\t--find-key [options] OR\n"
	"\t--generate-key [options] OR\n"
	"\t--import-key [options] OR\n"
	"\t--list-keys [options] OR\n"
	"\t--list-sigs [options] OR\n"
	"\t--get-key keyid [options] OR\n"
	"\t--version\n"
	"where options are:\n"
	"\t[--coredumps] AND/OR\n"
	"\t[--hash=<hash alg>] AND/OR\n"
	"\t[--homedir=<homedir>] AND/OR\n"
	"\t[--keyring=<keyring>] AND/OR\n"
	"\t[--userid=<userid>] AND/OR\n"
	"\t[--verbose]\n";

enum optdefs {
	/* commands */
	LIST_KEYS = 260,
	LIST_SIGS,
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
	HASH_ALG,
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
	{"list-sigs",	no_argument,		NULL,	LIST_SIGS},
	{"find-key",	no_argument,		NULL,	FIND_KEY},
	{"export",	no_argument,		NULL,	EXPORT_KEY},
	{"export-key",	no_argument,		NULL,	EXPORT_KEY},
	{"import",	no_argument,		NULL,	IMPORT_KEY},
	{"import-key",	no_argument,		NULL,	IMPORT_KEY},
	{"gen",		optional_argument,	NULL,	GENERATE_KEY},
	{"gen-key",	optional_argument,	NULL,	GENERATE_KEY},
	{"generate",	optional_argument,	NULL,	GENERATE_KEY},
	{"generate-key", optional_argument,	NULL,	GENERATE_KEY},
	{"get-key", 	no_argument,		NULL,	GET_KEY},
	/* debugging commands */
	{"help",	no_argument,		NULL,	HELP_CMD},
	{"version",	no_argument,		NULL,	VERSION_CMD},
	{"debug",	required_argument, 	NULL,	OPS_DEBUG},
	/* options */
	{"coredumps",	no_argument, 		NULL,	COREDUMPS},
	{"keyring",	required_argument, 	NULL,	KEYRING},
	{"userid",	required_argument, 	NULL,	USERID},
	{"hash-alg",	required_argument, 	NULL,	HASH_ALG},
	{"hash",	required_argument, 	NULL,	HASH_ALG},
	{"algorithm",	required_argument, 	NULL,	HASH_ALG},
	{"home",	required_argument, 	NULL,	HOMEDIR},
	{"homedir",	required_argument, 	NULL,	HOMEDIR},
	{"numbits",	required_argument, 	NULL,	NUMBITS},
	{"ssh",		no_argument, 		NULL,	SSHKEYS},
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

/* vararg print function */
static void
p(FILE *fp, char *s, ...)
{
	va_list	args;

	va_start(args, s);
	while (s != NULL) {
		(void) fprintf(fp, "%s", s);
		s = va_arg(args, char *);
	}
	va_end(args);
}

/* print a JSON object to the FILE stream */
static void
pobj(FILE *fp, mj_t *obj, int depth)
{
	int	i;

	if (obj == NULL) {
		(void) fprintf(stderr, "No object found\n");
		return;
	}
	for (i = 0 ; i < depth ; i++) {
		p(fp, " ", NULL);
	}
	switch(obj->type) {
	case MJ_NULL:
	case MJ_FALSE:
	case MJ_TRUE:
		p(fp, (obj->type == MJ_NULL) ? "null" : (obj->type == MJ_FALSE) ? "false" : "true", NULL);
		break;
	case MJ_NUMBER:
		p(fp, obj->value.s, NULL);
		break;
	case MJ_STRING:
		(void) fprintf(fp, "%.*s", (int)(obj->c), obj->value.s);
		break;
	case MJ_ARRAY:
		for (i = 0 ; i < obj->c ; i++) {
			pobj(fp, &obj->value.v[i], depth + 1);
			if (i < obj->c - 1) {
				(void) fprintf(fp, ", "); 
			}
		}
		(void) fprintf(fp, "\n"); 
		break;
	case MJ_OBJECT:
		for (i = 0 ; i < obj->c ; i += 2) {
			pobj(fp, &obj->value.v[i], depth + 1);
			p(fp, ": ", NULL); 
			pobj(fp, &obj->value.v[i + 1], 0);
			if (i < obj->c - 1) {
				p(fp, ", ", NULL); 
			}
		}
		p(fp, "\n", NULL); 
		break;
	default:
		break;
	}
}

/* return the time as a string */
static char * 
ptimestr(char *dest, size_t size, time_t t)
{
	struct tm      *tm;

	tm = gmtime(&t);
	(void) snprintf(dest, size, "%04d-%02d-%02d",
		tm->tm_year + 1900,
		tm->tm_mon + 1,
		tm->tm_mday);
	return dest;
}

/* format a JSON object */
static void
formatobj(FILE *fp, mj_t *obj, const int psigs)
{
	int64_t	 birthtime;
	int64_t	 duration;
	time_t	 now;
	char	 tbuf[32];
	char	*s;
	mj_t	*sub;
	int	 r;
	int	 i;

	if (__ops_get_debug_level(__FILE__)) {
		mj_asprint(&s, obj);
		(void) fprintf(stderr, "formatobj: json is '%s'\n", s);
		free(s);
	}
	pobj(fp, &obj->value.v[mj_object_find(obj, "header", 0, 2) + 1], 0);
	p(fp, " ", NULL);
	pobj(fp, &obj->value.v[mj_object_find(obj, "key bits", 0, 2) + 1], 0);
	p(fp, "/", NULL);
	pobj(fp, &obj->value.v[mj_object_find(obj, "pka", 0, 2) + 1], 0);
	p(fp, " ", NULL);
	pobj(fp, &obj->value.v[mj_object_find(obj, "key id", 0, 2) + 1], 0);
	birthtime = strtoll(obj->value.v[mj_object_find(obj, "birthtime", 0, 2) + 1].value.s, NULL, 10);
	p(fp, " ", ptimestr(tbuf, sizeof(tbuf), birthtime), NULL);
	duration = strtoll(obj->value.v[mj_object_find(obj, "duration", 0, 2) + 1].value.s, NULL, 10);
	if (duration > 0) {
		now = time(NULL);
		p(fp, " ", (birthtime + duration < now) ? "[EXPIRED " : "[EXPIRES ",
			ptimestr(tbuf, sizeof(tbuf), birthtime + duration), "]", NULL);
	}
	p(fp, "\n", "Key fingerprint: ", NULL);
	pobj(fp, &obj->value.v[mj_object_find(obj, "fingerprint", 0, 2) + 1], 0);
	p(fp, "\n", NULL);
	/* go to field after \"duration\" */
	for (i = mj_object_find(obj, "duration", 0, 2) + 2; i < mj_arraycount(obj) ; i += 2) {
		if (strcmp(obj->value.v[i].value.s, "uid") == 0) {
			sub = &obj->value.v[i + 1];
			p(fp, "uid", NULL);
			pobj(fp, &sub->value.v[0], (psigs) ? 4 : 14); /* human name */
			pobj(fp, &sub->value.v[1], 1); /* any revocation */
			p(fp, "\n", NULL);
		} else if (strcmp(obj->value.v[i].value.s, "encryption") == 0) {
			sub = &obj->value.v[i + 1];
			p(fp, "encryption", NULL);
			pobj(fp, &sub->value.v[0], 1);	/* size */
			p(fp, "/", NULL);
			pobj(fp, &sub->value.v[1], 0); /* alg */
			p(fp, " ", NULL);
			pobj(fp, &sub->value.v[2], 0); /* id */
			p(fp, " ", ptimestr(tbuf, sizeof(tbuf), strtoll(sub->value.v[3].value.s, NULL, 10)),
				"\n", NULL);
		} else if (strcmp(obj->value.v[i].value.s, "sig") == 0) {
			sub = &obj->value.v[i + 1];
			p(fp, "sig", NULL);
			pobj(fp, &sub->value.v[0], 8);	/* size */
			p(fp, "  ", ptimestr(tbuf, sizeof(tbuf), strtoll(sub->value.v[1].value.s, NULL, 10)),
				" ", NULL); /* time */
			pobj(fp, &sub->value.v[2], 0); /* human name */
			p(fp, "\n", NULL);
		} else {
			fprintf(stderr, "weird '%s'\n", obj->value.v[i].value.s);
			pobj(fp, &obj->value.v[i], 0); /* human name */
		}
	}
	p(fp, "\n", NULL);
}

/* match keys, decoding from json if we do find any */
static int
match_keys(netpgp_t *netpgp, FILE *fp, char *f, const int psigs)
{
	char	*json;
	mj_t	 ids;
	int	 from;
	int	 idc;
	int	 tok;
	int	 to;
	int	 i;

	if (f == NULL) {
		if (!netpgp_list_keys_json(netpgp, &json, psigs)) {
			return 0;
		}
	} else {
		if (netpgp_match_keys_json(netpgp, &json, f, "human", psigs) == 0) {
			return 0;
		}
	}
	if (__ops_get_debug_level(__FILE__)) {
		(void) fprintf(stderr, "match_keys: json is '%s'\n", json);
	}
	/* ids is an array of strings, each containing 1 entry */
	(void) memset(&ids, 0x0, sizeof(ids));
	from = to = tok = 0;
	/* convert from string into an mj structure */
	(void) mj_parse(&ids, json, &from, &to, &tok);
	if ((idc = mj_arraycount(&ids)) == 1 && strchr(json, '{') == NULL) {
		idc = 0;
	}
	(void) fprintf(fp, "%d key%s found\n", idc, (idc == 1) ? "" : "s");
	for (i = 0 ; i < idc ; i++) {
		formatobj(fp, &ids.value.v[i], psigs);
	}
	/* clean up */
	free(json);
	mj_delete(&ids);
	return idc;
}

/* do a command once for a specified file 'f' */
static int
netpgp_cmd(netpgp_t *netpgp, prog_t *p, char *f)
{
	char	*key;

	switch (p->cmd) {
	case LIST_KEYS:
	case LIST_SIGS:
		return match_keys(netpgp, stdout, f, (p->cmd == LIST_SIGS));
	case FIND_KEY:
		return netpgp_find_key(netpgp, netpgp_getvar(netpgp, "userid"));
	case EXPORT_KEY:
		key = netpgp_export_key(netpgp, netpgp_getvar(netpgp, "userid"));
		if (key) {
			printf("%s", key);
			return 1;
		}
		(void) fprintf(stderr, "key '%s' not found\n", f);
		return 0;
	case IMPORT_KEY:
		return netpgp_import_key(netpgp, f);
	case GENERATE_KEY:
		return netpgp_generate_key(netpgp, f, p->numbits);
	case GET_KEY:
		key = netpgp_get_key(netpgp, f, "human");
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

/* set the option */
static int
setoption(netpgp_t *netpgp, prog_t *p, int val, char *arg, int *homeset)
{
	switch (val) {
	case COREDUMPS:
		netpgp_setvar(netpgp, "coredumps", "allowed");
		break;
	case GENERATE_KEY:
		netpgp_setvar(netpgp, "userid checks", "skip");
		p->cmd = val;
		break;
	case LIST_KEYS:
	case LIST_SIGS:
	case FIND_KEY:
	case EXPORT_KEY:
	case IMPORT_KEY:
	case GET_KEY:
	case HELP_CMD:
		p->cmd = val;
		break;
	case VERSION_CMD:
		printf(
"%s\nAll bug reports, praise and chocolate, please, to:\n%s\n",
			netpgp_get_info("version"),
			netpgp_get_info("maintainer"));
		exit(EXIT_SUCCESS);
		/* options */
	case SSHKEYS:
		netpgp_setvar(netpgp, "ssh keys", "1");
		break;
	case KEYRING:
		if (arg == NULL) {
			(void) fprintf(stderr,
				"No keyring argument provided\n");
			exit(EXIT_ERROR);
		}
		snprintf(p->keyring, sizeof(p->keyring), "%s", arg);
		break;
	case USERID:
		if (optarg == NULL) {
			(void) fprintf(stderr,
				"no userid argument provided\n");
			exit(EXIT_ERROR);
		}
		netpgp_setvar(netpgp, "userid", arg);
		break;
	case VERBOSE:
		netpgp_incvar(netpgp, "verbose", 1);
		break;
	case HOMEDIR:
		if (arg == NULL) {
			(void) fprintf(stderr,
			"no home directory argument provided\n");
			exit(EXIT_ERROR);
		}
		netpgp_set_homedir(netpgp, arg, NULL, 0);
		*homeset = 1;
		break;
	case NUMBITS:
		if (arg == NULL) {
			(void) fprintf(stderr,
			"no number of bits argument provided\n");
			exit(EXIT_ERROR);
		}
		p->numbits = atoi(arg);
		break;
	case HASH_ALG:
		if (arg == NULL) {
			(void) fprintf(stderr,
			"No hash algorithm argument provided\n");
			exit(EXIT_ERROR);
		}
		netpgp_setvar(netpgp, "hash", arg);
		break;
	case PASSWDFD:
		if (arg == NULL) {
			(void) fprintf(stderr,
			"no pass-fd argument provided\n");
			exit(EXIT_ERROR);
		}
		netpgp_setvar(netpgp, "pass-fd", arg);
		break;
	case RESULTS:
		if (arg == NULL) {
			(void) fprintf(stderr,
			"No output filename argument provided\n");
			exit(EXIT_ERROR);
		}
		netpgp_setvar(netpgp, "res", arg);
		break;
	case SSHKEYFILE:
		netpgp_setvar(netpgp, "ssh keys", "1");
		netpgp_setvar(netpgp, "sshkeyfile", arg);
		break;
	case OPS_DEBUG:
		netpgp_set_debug(arg);
		break;
	default:
		p->cmd = HELP_CMD;
		break;
	}
	return 1;
}

/* we have -o option=value -- parse, and process */
static int
parse_option(netpgp_t *netpgp, prog_t *p, const char *s, int *homeset)
{
	static regex_t	 opt;
	struct option	*op;
	static int	 compiled;
	regmatch_t	 matches[10];
	char		 option[128];
	char		 value[128];

	if (!compiled) {
		compiled = 1;
		(void) regcomp(&opt, "([^=]{1,128})(=(.*))?", REG_EXTENDED);
	}
	if (regexec(&opt, s, 10, matches, 0) == 0) {
		(void) snprintf(option, sizeof(option), "%.*s",
			(int)(matches[1].rm_eo - matches[1].rm_so), &s[matches[1].rm_so]);
		if (matches[2].rm_so > 0) {
			(void) snprintf(value, sizeof(value), "%.*s",
				(int)(matches[3].rm_eo - matches[3].rm_so), &s[matches[3].rm_so]);
		} else {
			value[0] = 0x0;
		}
		for (op = options ; op->name ; op++) {
			if (strcmp(op->name, option) == 0) {
				return setoption(netpgp, p, op->val, value, homeset);
			}
		}
	}
	return 0;
}

int
main(int argc, char **argv)
{
	struct stat	st;
	netpgp_t	netpgp;
	prog_t          p;
	int             homeset;
	int             optindex;
	int             ret;
	int             ch;
	int             i;

	(void) memset(&p, 0x0, sizeof(p));
	(void) memset(&netpgp, 0x0, sizeof(netpgp));
	homeset = 0;
	p.progname = argv[0];
	p.numbits = DEFAULT_NUMBITS;
	if (argc < 2) {
		print_usage(usage, p.progname);
		exit(EXIT_ERROR);
	}
	/* set some defaults */
	netpgp_setvar(&netpgp, "sshkeydir", "/etc/ssh");
	netpgp_setvar(&netpgp, "res", "<stdout>");
	netpgp_setvar(&netpgp, "hash", DEFAULT_HASH_ALG);
	optindex = 0;
	while ((ch = getopt_long(argc, argv, "Vglo:s", options, &optindex)) != -1) {
		if (ch >= LIST_KEYS) {
			/* getopt_long returns 0 for long options */
			if (!setoption(&netpgp, &p, options[optindex].val, optarg, &homeset)) {
				(void) fprintf(stderr, "Bad setoption result %d\n", ch);
			}
		} else {
			switch (ch) {
			case 'V':
				printf(
	"%s\nAll bug reports, praise and chocolate, please, to:\n%s\n",
					netpgp_get_info("version"),
					netpgp_get_info("maintainer"));
				exit(EXIT_SUCCESS);
			case 'g':
				p.cmd = GENERATE_KEY;
				break;
			case 'l':
				p.cmd = LIST_KEYS;
				break;
			case 'o':
				if (!parse_option(&netpgp, &p, optarg, &homeset)) {
					(void) fprintf(stderr, "Bad parse_option\n");
				}
				break;
			case 's':
				p.cmd = LIST_SIGS;
				break;
			default:
				p.cmd = HELP_CMD;
				break;
			}
		}
	}
	if (!homeset) {
		netpgp_set_homedir(&netpgp, getenv("HOME"),
			netpgp_getvar(&netpgp, "ssh keys") ? "/.ssh" : "/.gnupg", 1);
	}
	/* initialise, and read keys from file */
	if (!netpgp_init(&netpgp)) {
		if (stat(netpgp_getvar(&netpgp, "homedir"), &st) < 0) {
			(void) mkdir(netpgp_getvar(&netpgp, "homedir"), 0700);
		}
		if (stat(netpgp_getvar(&netpgp, "homedir"), &st) < 0) {
			(void) fprintf(stderr, "can't create home directory '%s'\n",
				netpgp_getvar(&netpgp, "homedir"));
			exit(EXIT_ERROR);
		}
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
