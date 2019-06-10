/*	$NetBSD: ddns-confgen.c,v 1.3.2.2 2019/06/10 22:02:57 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

/**
 * ddns-confgen generates configuration files for dynamic DNS. It can
 * be used as a convenient alternative to writing the ddns.key file
 * and the corresponding key and update-policy statements in named.conf.
 */

#include <config.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#include <isc/assertions.h>
#include <isc/base64.h>
#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/file.h>
#include <isc/mem.h>
#include <isc/net.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>

#if USE_PKCS11
#include <pk11/result.h>
#endif

#include <dns/keyvalues.h>
#include <dns/name.h>
#include <dns/result.h>

#include <dst/dst.h>
#include <confgen/os.h>

#include "util.h"
#include "keygen.h"

#define KEYGEN_DEFAULT		"tsig-key"
#define CONFGEN_DEFAULT		"ddns-key"

static char program[256];
const char *progname;
static enum { progmode_keygen, progmode_confgen} progmode;
bool verbose = false; /* needed by util.c but not used here */

ISC_PLATFORM_NORETURN_PRE static void
usage(int status) ISC_PLATFORM_NORETURN_POST;

static void
usage(int status) {
	if (progmode == progmode_confgen) {
		fprintf(stderr, "\
Usage:\n\
 %s [-a alg] [-k keyname] [-q] [-s name | -z zone]\n\
  -a alg:        algorithm (default hmac-sha256)\n\
  -k keyname:    name of the key as it will be used in named.conf\n\
  -s name:       domain name to be updated using the created key\n\
  -z zone:       name of the zone as it will be used in named.conf\n\
  -q:            quiet mode: print the key, with no explanatory text\n",
			 progname);
	} else {
		fprintf(stderr, "\
Usage:\n\
 %s [-a alg] [keyname]\n\
  -a alg:        algorithm (default hmac-sha256)\n\n",
			 progname);
	}

	exit (status);
}

int
main(int argc, char **argv) {
	isc_result_t result = ISC_R_SUCCESS;
	bool show_final_mem = false;
	bool quiet = false;
	isc_buffer_t key_txtbuffer;
	char key_txtsecret[256];
	isc_mem_t *mctx = NULL;
	const char *keyname = NULL;
	const char *zone = NULL;
	const char *self_domain = NULL;
	char *keybuf = NULL;
	dns_secalg_t alg = DST_ALG_HMACSHA256;
	const char *algname;
	int keysize = 256;
	int len = 0;
	int ch;

#if USE_PKCS11
	pk11_result_register();
#endif
	dns_result_register();

	result = isc_file_progname(*argv, program, sizeof(program));
	if (result != ISC_R_SUCCESS)
		memmove(program, "tsig-keygen", 11);
	progname = program;

	/*
	 * Libtool doesn't preserve the program name prior to final
	 * installation.  Remove the libtool prefix ("lt-").
	 */
	if (strncmp(progname, "lt-", 3) == 0)
		progname += 3;

#define PROGCMP(X) \
	(strcasecmp(progname, X) == 0 || strcasecmp(progname, X ".exe") == 0)

	if (PROGCMP("tsig-keygen")) {
		progmode = progmode_keygen;
		quiet = true;
	} else if (PROGCMP("ddns-confgen")) {
		progmode = progmode_confgen;
	} else {
		INSIST(0);
		ISC_UNREACHABLE();
	}

	isc_commandline_errprint = false;

	while ((ch = isc_commandline_parse(argc, argv,
					   "a:hk:Mmr:qs:y:z:")) != -1) {
		switch (ch) {
		case 'a':
			algname = isc_commandline_argument;
			alg = alg_fromtext(algname);
			if (alg == DST_ALG_UNKNOWN)
				fatal("Unsupported algorithm '%s'", algname);
			keysize = alg_bits(alg);
			break;
		case 'h':
			usage(0);
		case 'k':
		case 'y':
			if (progmode == progmode_confgen)
				keyname = isc_commandline_argument;
			else
				usage(1);
			break;
		case 'M':
			isc_mem_debugging = ISC_MEM_DEBUGTRACE;
			break;
		case 'm':
			show_final_mem = true;
			break;
		case 'q':
			if (progmode == progmode_confgen)
				quiet = true;
			else
				usage(1);
			break;
		case 'r':
			fatal("The -r option has been deprecated.");
			break;
		case 's':
			if (progmode == progmode_confgen)
				self_domain = isc_commandline_argument;
			else
				usage(1);
			break;
		case 'z':
			if (progmode == progmode_confgen)
				zone = isc_commandline_argument;
			else
				usage(1);
			break;
		case '?':
			if (isc_commandline_option != '?') {
				fprintf(stderr, "%s: invalid argument -%c\n",
					program, isc_commandline_option);
				usage(1);
			} else
				usage(0);
			break;
		default:
			fprintf(stderr, "%s: unhandled option -%c\n",
				program, isc_commandline_option);
			exit(1);
		}
	}

	if (progmode == progmode_keygen)
		keyname = argv[isc_commandline_index++];

	POST(argv);

	if (self_domain != NULL && zone != NULL)
		usage(1);	/* -s and -z cannot coexist */

	if (argc > isc_commandline_index)
		usage(1);

	/* Use canonical algorithm name */
	algname = alg_totext(alg);

	DO("create memory context", isc_mem_create(0, 0, &mctx));

	if (keyname == NULL) {
		const char *suffix = NULL;

		keyname = ((progmode == progmode_keygen)
			?  KEYGEN_DEFAULT
			: CONFGEN_DEFAULT);
		if (self_domain != NULL)
			suffix = self_domain;
		else if (zone != NULL)
			suffix = zone;
		if (suffix != NULL) {
			len = strlen(keyname) + strlen(suffix) + 2;
			keybuf = isc_mem_get(mctx, len);
			if (keybuf == NULL)
				fatal("failed to allocate memory for keyname");
			snprintf(keybuf, len, "%s.%s", keyname, suffix);
			keyname = (const char *) keybuf;
		}
	}

	isc_buffer_init(&key_txtbuffer, &key_txtsecret, sizeof(key_txtsecret));

	generate_key(mctx, alg, keysize, &key_txtbuffer);


	if (!quiet)
		printf("\
# To activate this key, place the following in named.conf, and\n\
# in a separate keyfile on the system or systems from which nsupdate\n\
# will be run:\n");

	printf("\
key \"%s\" {\n\
	algorithm %s;\n\
	secret \"%.*s\";\n\
};\n",
	       keyname, algname,
	       (int)isc_buffer_usedlength(&key_txtbuffer),
	       (char *)isc_buffer_base(&key_txtbuffer));

	if (!quiet) {
		if (self_domain != NULL) {
			printf("\n\
# Then, in the \"zone\" statement for the zone containing the\n\
# name \"%s\", place an \"update-policy\" statement\n\
# like this one, adjusted as needed for your preferred permissions:\n\
update-policy {\n\
	  grant %s name %s ANY;\n\
};\n",
			       self_domain, keyname, self_domain);
		} else if (zone != NULL) {
			printf("\n\
# Then, in the \"zone\" definition statement for \"%s\",\n\
# place an \"update-policy\" statement like this one, adjusted as \n\
# needed for your preferred permissions:\n\
update-policy {\n\
	  grant %s zonesub ANY;\n\
};\n",
			       zone, keyname);
		} else {
			printf("\n\
# Then, in the \"zone\" statement for each zone you wish to dynamically\n\
# update, place an \"update-policy\" statement granting update permission\n\
# to this key.  For example, the following statement grants this key\n\
# permission to update any name within the zone:\n\
update-policy {\n\
	grant %s zonesub ANY;\n\
};\n",
			       keyname);
		}

		printf("\n\
# After the keyfile has been placed, the following command will\n\
# execute nsupdate using this key:\n\
nsupdate -k <keyfile>\n");

	}

	if (keybuf != NULL)
		isc_mem_put(mctx, keybuf, len);

	if (show_final_mem)
		isc_mem_stats(mctx, stderr);

	isc_mem_destroy(&mctx);

	return (0);
}
