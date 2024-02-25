/*	$NetBSD: rndc-confgen.c,v 1.6.2.1 2024/02/25 15:43:00 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

/**
 * rndc-confgen generates configuration files for rndc. It can be used
 * as a convenient alternative to writing the rndc.conf file and the
 * corresponding controls and key statements in named.conf by hand.
 * Alternatively, it can be run with the -a option to set up a
 * rndc.key file and avoid the need for a rndc.conf file and a
 * controls statement altogether.
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#include <isc/assertions.h>
#include <isc/attributes.h>
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

#include <dns/keyvalues.h>
#include <dns/name.h>

#include <dst/dst.h>

#include <confgen/os.h>

#include "keygen.h"
#include "util.h"

#define DEFAULT_KEYNAME "rndc-key"
#define DEFAULT_SERVER	"127.0.0.1"
#define DEFAULT_PORT	953

static char program[256];
const char *progname;

bool verbose = false;

const char *keyfile, *keydef;

noreturn static void
usage(int status);

static void
usage(int status) {
	fprintf(stderr, "\
Usage:\n\
 %s [-a] [-b bits] [-c keyfile] [-k keyname] [-p port] \
[-s addr] [-t chrootdir] [-u user]\n\
  -a:		 generate just the key clause and write it to keyfile (%s)\n\
  -A alg:	 algorithm (default hmac-sha256)\n\
  -b bits:	 from 1 through 512, default 256; total length of the secret\n\
  -c keyfile:	 specify an alternate key file (requires -a)\n\
  -k keyname:	 the name as it will be used  in named.conf and rndc.conf\n\
  -p port:	 the port named will listen on and rndc will connect to\n\
  -q:		 suppress printing written key path\n\
  -s addr:	 the address to which rndc should connect\n\
  -t chrootdir:	 write a keyfile in chrootdir as well (requires -a)\n\
  -u user:	 set the keyfile owner to \"user\" (requires -a)\n",
		progname, keydef);

	exit(status);
}

int
main(int argc, char **argv) {
	bool show_final_mem = false;
	isc_buffer_t key_txtbuffer;
	char key_txtsecret[256];
	isc_mem_t *mctx = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	const char *keyname = NULL;
	const char *serveraddr = NULL;
	dns_secalg_t alg;
	const char *algname;
	char *p;
	int ch;
	int port;
	int keysize = -1;
	struct in_addr addr4_dummy;
	struct in6_addr addr6_dummy;
	char *chrootdir = NULL;
	char *user = NULL;
	bool keyonly = false;
	bool quiet = false;
	int len;

	keydef = keyfile = RNDC_KEYFILE;

	result = isc_file_progname(*argv, program, sizeof(program));
	if (result != ISC_R_SUCCESS) {
		memmove(program, "rndc-confgen", 13);
	}
	progname = program;

	keyname = DEFAULT_KEYNAME;
	alg = DST_ALG_HMACSHA256;
	serveraddr = DEFAULT_SERVER;
	port = DEFAULT_PORT;

	isc_commandline_errprint = false;

	while ((ch = isc_commandline_parse(argc, argv,
					   "aA:b:c:hk:Mmp:r:s:t:u:Vy")) != -1)
	{
		switch (ch) {
		case 'a':
			keyonly = true;
			break;
		case 'A':
			algname = isc_commandline_argument;
			alg = alg_fromtext(algname);
			if (alg == DST_ALG_UNKNOWN) {
				fatal("Unsupported algorithm '%s'", algname);
			}
			break;
		case 'b':
			keysize = strtol(isc_commandline_argument, &p, 10);
			if (*p != '\0' || keysize < 0) {
				fatal("-b requires a non-negative number");
			}
			break;
		case 'c':
			keyfile = isc_commandline_argument;
			break;
		case 'h':
			usage(0);
		case 'k':
		case 'y': /* Compatible with rndc -y. */
			keyname = isc_commandline_argument;
			break;
		case 'M':
			isc_mem_debugging = ISC_MEM_DEBUGTRACE;
			break;

		case 'm':
			show_final_mem = true;
			break;
		case 'p':
			port = strtol(isc_commandline_argument, &p, 10);
			if (*p != '\0' || port < 0 || port > 65535) {
				fatal("port '%s' out of range",
				      isc_commandline_argument);
			}
			break;
		case 'q':
			quiet = true;
			break;
		case 'r':
			fatal("The -r option has been deprecated.");
			break;
		case 's':
			serveraddr = isc_commandline_argument;
			if (inet_pton(AF_INET, serveraddr, &addr4_dummy) != 1 &&
			    inet_pton(AF_INET6, serveraddr, &addr6_dummy) != 1)
			{
				fatal("-s should be an IPv4 or IPv6 address");
			}
			break;
		case 't':
			chrootdir = isc_commandline_argument;
			break;
		case 'u':
			user = isc_commandline_argument;
			break;
		case 'V':
			verbose = true;
			break;
		case '?':
			if (isc_commandline_option != '?') {
				fprintf(stderr, "%s: invalid argument -%c\n",
					program, isc_commandline_option);
				usage(1);
			} else {
				usage(0);
			}
			break;
		default:
			fprintf(stderr, "%s: unhandled option -%c\n", program,
				isc_commandline_option);
			exit(1);
		}
	}

	argc -= isc_commandline_index;
	argv += isc_commandline_index;
	POST(argv);

	if (argc > 0) {
		usage(1);
	}

	if (alg == DST_ALG_HMACMD5) {
		fprintf(stderr, "warning: use of hmac-md5 for RNDC keys "
				"is deprecated; hmac-sha256 is now "
				"recommended.\n");
	}

	if (keysize < 0) {
		keysize = alg_bits(alg);
	}
	algname = dst_hmac_algorithm_totext(alg);

	isc_mem_create(&mctx);
	isc_buffer_init(&key_txtbuffer, &key_txtsecret, sizeof(key_txtsecret));

	generate_key(mctx, alg, keysize, &key_txtbuffer);

	if (keyonly) {
		write_key_file(keyfile, chrootdir == NULL ? user : NULL,
			       keyname, &key_txtbuffer, alg);
		if (!quiet) {
			printf("wrote key file \"%s\"\n", keyfile);
		}

		if (chrootdir != NULL) {
			char *buf;
			len = strlen(chrootdir) + strlen(keyfile) + 2;
			buf = isc_mem_get(mctx, len);
			snprintf(buf, len, "%s%s%s", chrootdir,
				 (*keyfile != '/') ? "/" : "", keyfile);

			write_key_file(buf, user, keyname, &key_txtbuffer, alg);
			if (!quiet) {
				printf("wrote key file \"%s\"\n", buf);
			}
			isc_mem_put(mctx, buf, len);
		}
	} else {
		printf("\
# Start of rndc.conf\n\
key \"%s\" {\n\
	algorithm %s;\n\
	secret \"%.*s\";\n\
};\n\
\n\
options {\n\
	default-key \"%s\";\n\
	default-server %s;\n\
	default-port %d;\n\
};\n\
# End of rndc.conf\n\
\n\
# Use with the following in named.conf, adjusting the allow list as needed:\n\
# key \"%s\" {\n\
# 	algorithm %s;\n\
# 	secret \"%.*s\";\n\
# };\n\
# \n\
# controls {\n\
# 	inet %s port %d\n\
# 		allow { %s; } keys { \"%s\"; };\n\
# };\n\
# End of named.conf\n",
		       keyname, algname,
		       (int)isc_buffer_usedlength(&key_txtbuffer),
		       (char *)isc_buffer_base(&key_txtbuffer), keyname,
		       serveraddr, port, keyname, algname,
		       (int)isc_buffer_usedlength(&key_txtbuffer),
		       (char *)isc_buffer_base(&key_txtbuffer), serveraddr,
		       port, serveraddr, keyname);
	}

	if (show_final_mem) {
		isc_mem_stats(mctx, stderr);
	}

	isc_mem_destroy(&mctx);

	return (0);
}
