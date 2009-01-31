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
 \file Command line program to perform openpgp operations
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>
#include <unistd.h>

#include <openpgpsdk/keyring.h>
#include <openpgpsdk/crypto.h>
#include <openpgpsdk/signature.h>
#include <openpgpsdk/validate.h>
#include <openpgpsdk/readerwriter.h>
#include <openpgpsdk/std_print.h>
#include <openpgpsdk/packet-show.h>
#include <openpgpsdk/util.h>

#define DEFAULT_NUMBITS 2048

#define MAXBUF 1024

static const char *usage = "%s --list-keys | --list-packets | --encrypt | --decrypt | --sign | --clearsign | --verify [--keyring=<keyring>] [--userid=<userid>] [--armour] [--homedir=<homedir>] files...\n";
static const char *usage_find_key = "%s --find-key --userid=<userid> [--keyring=<keyring>] \n";
static const char *usage_export_key = "%s --export-key --userid=<userid> [--keyring=<keyring>] \n";
static const char *usage_encrypt = "%s --encrypt --userid=<userid> [--armour] [--homedir=<homedir>] files...\n";
static const char *usage_sign = "%s --sign --userid=<userid> [--armour] [--homedir=<homedir>] files...\n";
static const char *usage_clearsign = "%s --clearsign --userid=<userid> [--homedir=<homedir>] files...\n";

static char    *pname;

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
	LIST_PACKETS,

	/* options */
	KEYRING,
	USERID,
	PASSPHRASE,
	ARMOUR,
	HOMEDIR,
	NUMBITS,

	/* debug */
	OPS_DEBUG

};

enum {
	USERID_RAW_LEN		= OPS_KEY_ID_SIZE,
	USERID_FORMATTED	= USERID_RAW_LEN * 2
};

#define EXIT_ERROR	2

static struct option long_options[] = {
	/* commands */
	{"list-keys", no_argument, NULL, LIST_KEYS},
	{"find-key", no_argument, NULL, FIND_KEY},
	{"export-key", no_argument, NULL, EXPORT_KEY},
	{"import-key", no_argument, NULL, IMPORT_KEY},
	{"generate-key", no_argument, NULL, GENERATE_KEY},

	{"encrypt", no_argument, NULL, ENCRYPT},
	{"decrypt", no_argument, NULL, DECRYPT},
	{"sign", no_argument, NULL, SIGN},
	{"clearsign", no_argument, NULL, CLEARSIGN},
	{"verify", no_argument, NULL, VERIFY},

	{"list-packets", no_argument, NULL, LIST_PACKETS},

	/* options */
	{"keyring", required_argument, NULL, KEYRING},
	{"userid", required_argument, NULL, USERID},
	{"passphrase", required_argument, NULL, PASSPHRASE},
	{"homedir", required_argument, NULL, HOMEDIR},
	{"armor", no_argument, NULL, ARMOUR},
	{"armour", no_argument, NULL, ARMOUR},
	{"numbits", required_argument, NULL, NUMBITS},

	/* debug */
	{"debug", required_argument, NULL, OPS_DEBUG},

	{ NULL, 0, NULL, 0},
};

/* gather up program variables into one struct */
typedef struct prog_t {
	char            keyring[MAXBUF + 1];		/* name of keyring */
	char            userid[MAXBUF + 1];		/* user identifier */
	char            passphrase[MAXBUF + 1];		/* passphrase */
	char            myring_name[MAXBUF + 1];	/* myring filename */
	ops_keyring_t  *myring;				/* personal keyring */
	char            pubring_name[MAXBUF + 1];	/* pubring filename */
	ops_keyring_t  *pubring;			/* public keyring */
	char            secring_name[MAXBUF + 1];	/* secret ring file */
	ops_keyring_t  *secring;			/* secret keyring */
	ops_boolean_t   overwrite;			/* overwrite files? */
	int             numbits;			/* # of bits */
	int             armour;				/* ASCII armor */
	int             cmd;				/* openpgp command */
	int             ex;				/* exit code */
} prog_t;




/* print a usage message */
static void
print_usage(const char *usagemsg, char *progname)
{
	(void) fprintf(stderr, "\nUsage: ");
	(void) fprintf(stderr, usagemsg, basename(progname));
}

/* wrapper to get a pass phrase from the user */
static void
get_pass_phrase(char *phrase, size_t size)
{
	char           *p;

	if ((p = getpass("openpgp passphrase: ")) == NULL) {
		exit(EXIT_ERROR);
	}
	(void) snprintf(phrase, size, "%s", p);
}

/* small function to pretty print an 8-character raw userid */
static char    *
userid_to_id(const unsigned char *userid, char *id)
{
	static const char *hexes = "0123456789ABCDEF";
	int		   i;

	for (i = 0; i < USERID_RAW_LEN ; i++) {
		id[i * 2] = hexes[(userid[i] & 0xf0) >> 4];
		id[(i * 2) + 1] = hexes[userid[i] & 0xf];
	}
	id[USERID_FORMATTED] = 0x0;
	return id;
}

/* print out the successful signature information */
static void
psuccess(char *f, ops_validate_result_t * results, ops_keyring_t *pubring)
{
	const ops_keydata_t *pubkey;
	char            id[USERID_FORMATTED + 1];
	int             i;

	for (i = 0; i < results->valid_count; i++) {
		printf("Good signature for %s made %susing %s key %s\n",
		       f,
		       ctime(&results->valid_sigs[i].creation_time),
		       ops_show_pka(results->valid_sigs[i].key_algorithm),
		       userid_to_id(results->valid_sigs[i].signer_id, id));
		pubkey = ops_keyring_find_key_by_id(pubring,
						    (const unsigned char *)
					  results->valid_sigs[i].signer_id);
		ops_print_public_keydata(pubkey);
	}
}

/* do a command once for a specified file 'f' */
static void
openpgp(prog_t * p, char *f)
{
	const ops_keydata_t *keydata;
	ops_validate_result_t *validate_result;
	ops_create_info_t *cinfo;
	ops_keydata_t  *mykeydata;
	ops_memory_t   *mem;
	ops_secret_key_t *skey;
	ops_user_id_t   uid;
	const char     *suffix;
	char            outputfilename[MAXBUF + 1];
	int             fd;

	(void) memset(&uid, 0x0, sizeof(uid));
	switch (p->cmd) {
	case LIST_KEYS:
		ops_keyring_list((p->keyring[0]) ? p->myring : p->pubring);
		break;

	case FIND_KEY:
		if (p->userid[0] == 0x0) {
			print_usage(usage_find_key, pname);
			exit(EXIT_ERROR);
		}
		if (ops_get_debug_level(__FILE__)) {
			(void) fprintf(stderr, "userid: %s\n", p->userid);
		}
		keydata = ops_keyring_find_key_by_userid((p->keyring[0]) ?
					 p->myring : p->pubring, p->userid);
		p->ex = (keydata) ? EXIT_FAILURE : EXIT_SUCCESS;
		/* ops_keyring_free(&keyring); */
		break;

	case EXPORT_KEY:
		if (p->keyring[0] == 0x0 || p->userid[0] == 0x0) {
			print_usage(usage_export_key, pname);
			exit(EXIT_ERROR);
		}
		keydata = ops_keyring_find_key_by_userid((p->keyring[0]) ?
					 p->myring : p->pubring, p->userid);
		if (!keydata) {
			fprintf(stderr, "Cannot find key in keyring\n");
			exit(EXIT_ERROR);
		}
		ops_setup_memory_write(&cinfo, &mem, 128);
		if (ops_get_keydata_content_type(keydata) == OPS_PTAG_CT_PUBLIC_KEY) {
			ops_write_transferable_public_key(keydata,
							  ops_true, cinfo);
		} else {
			ops_write_transferable_secret_key(keydata,
					    (unsigned char *) p->passphrase,
				    strlen(p->passphrase), ops_true, cinfo);
		}
		printf("%s", (char *) ops_memory_get_data(mem));
		ops_teardown_memory_write(cinfo, mem);
		break;

	case IMPORT_KEY:
		(void) fprintf(stderr, "before:\n");
		ops_keyring_list(p->pubring);

		/* read new key */
		if (!ops_keyring_read_from_file(p->pubring, p->armour, f)) {
			fprintf(stderr, "Cannot import key from file %s\n", f);
			exit(EXIT_ERROR);
		}
		(void) fprintf(stderr, "after:\n");
		ops_keyring_list(p->pubring);
		break;

	case GENERATE_KEY:
		uid.user_id = (unsigned char *) p->userid;
		mykeydata = ops_rsa_create_selfsigned_keypair(p->numbits,
							    65537, &uid);
		if (!mykeydata) {
			fprintf(stderr, "Cannot generate key\n");
			exit(EXIT_ERROR);
		}
		/* write public key */
		/* append to keyrings */
		fd = ops_setup_file_append(&cinfo, p->pubring_name);
		ops_write_transferable_public_key(mykeydata, ops_false, cinfo);
		ops_teardown_file_write(cinfo, fd);

		ops_keyring_free(p->pubring);
		if (!ops_keyring_read_from_file(p->pubring, ops_false, p->pubring_name)) {
			(void) fprintf(stderr, "Cannot re-read keyring %s\n", p->pubring_name);
			exit(EXIT_ERROR);
		}
		fd = ops_setup_file_append(&cinfo, p->secring_name);
		ops_write_transferable_secret_key(mykeydata, NULL, 0, ops_false, cinfo);
		ops_teardown_file_write(cinfo, fd);
		ops_keyring_free(p->secring);
		if (!ops_keyring_read_from_file(p->secring, ops_false, p->secring_name)) {
			fprintf(stderr, "Cannot re-read keyring %s\n", p->secring_name);
			exit(EXIT_ERROR);
		}
		ops_keydata_free(mykeydata);
		break;

	case ENCRYPT:
		if (p->userid[0] == 0x0) {
			print_usage(usage_encrypt, pname);
			exit(EXIT_ERROR);
		}
		suffix = p->armour ? ".asc" : ".gpg";
		keydata = ops_keyring_find_key_by_userid(p->pubring, p->userid);
		if (!keydata) {
			fprintf(stderr, "Userid '%s' not found in keyring\n",
				p->userid);
			exit(EXIT_ERROR);
		}
		/* outputfilename */
		(void) snprintf(outputfilename, MAXBUF, "%s%s", f, suffix);
		p->overwrite = ops_true;
		ops_encrypt_file(f, outputfilename, keydata, p->armour,
				 p->overwrite);
		break;

	case DECRYPT:
		p->overwrite = ops_true;
		ops_decrypt_file(f, NULL, p->secring, p->armour,
		    p->overwrite, callback_cmd_get_passphrase_from_cmdline);
		break;

	case SIGN:
		if (p->userid[0] == 0x0) {
			print_usage(usage_sign, pname);
			exit(EXIT_ERROR);
		}
		/* get key with which to sign */
		keydata = ops_keyring_find_key_by_userid(p->secring, p->userid);
		if (!keydata) {
			fprintf(stderr, "Userid '%s' not found in keyring\n",
				p->userid);
			exit(EXIT_ERROR);
		}
		do {
			/* print out the user id */
			ops_print_public_keydata(keydata);
			/* get the passphrase */
			if (p->passphrase[0] == 0x0) {
				get_pass_phrase(p->passphrase, sizeof(p->passphrase));
			}
			/* now decrypt key */
			skey = ops_decrypt_secret_key_from_data(keydata, p->passphrase);
			if (skey == NULL) {
				(void) fprintf(stderr, "Bad passphrase\n");
				p->passphrase[0] = 0x0;
			}
		} while (skey == NULL);

		/* sign file */
		p->overwrite = ops_true;
		ops_sign_file(f, NULL, skey, p->armour, p->overwrite);
		break;

	case CLEARSIGN:
		if (p->userid[0] == 0x0) {
			print_usage(usage_clearsign, pname);
			exit(EXIT_ERROR);
		}
		/* get key with which to sign */
		keydata = ops_keyring_find_key_by_userid(p->secring, p->userid);
		if (!keydata) {
			fprintf(stderr, "Userid '%s' not found in keyring\n",
				p->userid);
			exit(EXIT_ERROR);
		}
		do {
			/* print out the user id */
			ops_print_public_keydata(keydata);
			/* get the passphrase */
			if (p->passphrase[0] == 0x0) {
				get_pass_phrase(p->passphrase,
						sizeof(p->passphrase));
			}
			/* now decrypt key */
			skey = ops_decrypt_secret_key_from_data(keydata,
							     p->passphrase);
			if (skey == NULL) {
				(void) fprintf(stderr, "Bad passphrase\n");
				p->passphrase[0] = 0x0;
			}
		} while (skey == NULL);

		/* sign file */
		p->overwrite = ops_true;
		ops_sign_file_as_cleartext(f, NULL, skey, p->overwrite);
		break;

	case VERIFY:

		validate_result = calloc(1, sizeof(ops_validate_result_t));

		if (ops_validate_file(validate_result, f, p->armour, p->pubring) == ops_true) {
			psuccess(f, validate_result, p->pubring);
		} else {
			printf("\"%s\": verification failure: %d invalid signatures, %d unknown signatures\n", f, validate_result->invalid_count, validate_result->unknown_signer_count);
			p->ex = EXIT_FAILURE;
		}
		ops_validate_result_free(validate_result);
		break;

	case LIST_PACKETS:
		ops_list_packets(f, p->armour, p->pubring,
				 callback_cmd_get_passphrase_from_cmdline);
		break;

	default:
		print_usage(usage, pname);
		exit(EXIT_ERROR);
	}
}

int
main(int argc, char **argv)
{
	prog_t          p;
	char            homedir[MAXBUF + 1];
	char            default_homedir[MAXBUF + 1];
	char           *dir;
	int		zeroargs;
	int             optindex = 0;
	int             ch = 0;
	int             i;

	pname = argv[0];
	(void) memset(&p, 0x0, sizeof(p));
	(void) memset(homedir, 0x0, sizeof(homedir));
	zeroargs = 0;
	p.numbits = DEFAULT_NUMBITS;
	p.overwrite = ops_true;
	if (argc < 2) {
		print_usage(usage, pname);
		exit(EXIT_ERROR);
	}
	/* what does the user want to do? */

	while ((ch = getopt_long(argc, argv, "", long_options, &optindex)) != -1) {

		/* read options and commands */

		switch (long_options[optindex].val) {
			/* commands */
		case LIST_KEYS:
			zeroargs = 1;
			p.cmd = long_options[optindex].val;
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
		case LIST_PACKETS:
			p.cmd = long_options[optindex].val;
			break;

			/* option */

		case KEYRING:
			if (optarg == NULL) {
				(void) fprintf(stderr, "No keyring argument provided\n");
				exit(EXIT_ERROR);
			}
			snprintf(p.keyring, MAXBUF, "%s", optarg);
			break;

		case USERID:
			if (optarg == NULL) {
				(void) fprintf(stderr, "No userid argument provided\n");
				exit(EXIT_ERROR);
			}
			if (ops_get_debug_level(__FILE__)) {
				(void) fprintf(stderr, "user_id is '%s'\n", optarg);
			}
			snprintf(p.userid, MAXBUF, "%s", optarg);
			break;

		case PASSPHRASE:
			if (optarg == NULL) {
				(void) fprintf(stderr, "No passphrase argument provided\n");
				exit(EXIT_ERROR);
			}
			snprintf(p.passphrase, MAXBUF, "%s", optarg);
			break;

		case ARMOUR:
			p.armour = 1;
			break;

		case HOMEDIR:
			if (optarg == NULL) {
				(void) fprintf(stderr, "No home directory argument provided\n");
				exit(EXIT_ERROR);
			}
			snprintf(homedir, MAXBUF, "%s", optarg);
			break;


		case NUMBITS:
			if (optarg == NULL) {
				(void) fprintf(stderr, "No number of bits argument provided\n");
				exit(EXIT_ERROR);
			}
			p.numbits = atoi(optarg);
			break;

		case OPS_DEBUG:
			ops_set_debug_level(optarg);
			break;

		default:
			printf("shouldn't be here: option=%d\n", long_options[optindex].val);
			break;
		}
	}

	/*
         * Read keyrings.
         * read public and secret from homedir.
         * (assumed names are pubring.gpg and secring.gpg).
         * Also read named keyring, if given.
         *
         * We will then have variables pubring, secring and myring.
         */

	if (homedir[0]) {
		dir = homedir;
	} else {
		(void) snprintf(default_homedir, MAXBUF, "%s/.gnupg",
			getenv("HOME"));
		if (ops_get_debug_level(__FILE__)) {
			printf("dir: %s\n", default_homedir);
		}
		dir = default_homedir;
	}

	(void) snprintf(p.pubring_name, MAXBUF, "%s/pubring.gpg", dir);
	p.pubring = calloc(1, sizeof(*p.pubring));
	if (!ops_keyring_read_from_file(p.pubring, ops_false, p.pubring_name)) {
		fprintf(stderr, "Cannot read keyring %s\n", p.pubring_name);
		exit(EXIT_ERROR);
	}
	snprintf(p.secring_name, MAXBUF, "%s/secring.gpg", dir);
	p.secring = calloc(1, sizeof(*p.secring));
	if (!ops_keyring_read_from_file(p.secring, ops_false, p.secring_name)) {
		fprintf(stderr, "Cannot read keyring %s\n", p.secring_name);
		exit(EXIT_ERROR);
	}
	if (p.keyring[0] != 0x0) {
		snprintf(p.myring_name, MAXBUF, "%s/%s", homedir, p.keyring);
		p.myring = calloc(1, sizeof(*p.myring));
		if (!ops_keyring_read_from_file(p.myring, ops_false, p.myring_name)) {
			fprintf(stderr, "Cannot read keyring %s\n", p.myring_name);
			exit(EXIT_ERROR);
		}
	}

	/*
	 * now do the required action for each of the files on the command
	 * line
	 */
	if (zeroargs) {
		openpgp(&p, NULL);
	} else {
		for (p.ex = EXIT_SUCCESS, i = optind; i < argc; i++) {
			openpgp(&p, argv[i]);
		}
	}

	if (p.pubring) {
		ops_keyring_free(p.pubring);
	}
	if (p.secring) {
		ops_keyring_free(p.secring);
	}
	if (p.myring) {
		ops_keyring_free(p.myring);
	}
	exit(p.ex);
}
