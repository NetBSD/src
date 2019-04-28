/*	$NetBSD: dnssec-keygen.c,v 1.5 2019/04/28 00:01:13 christos Exp $	*/

/*
 * Portions Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 *
 * Portions Copyright (C) Network Associates, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC AND NETWORK ASSOCIATES DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*! \file */

#include <config.h>

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/region.h>
#include <isc/string.h>
#include <isc/util.h>

#include <pk11/site.h>

#include <dns/dnssec.h>
#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/name.h>
#include <dns/rdataclass.h>
#include <dns/result.h>
#include <dns/secalg.h>

#include <dst/dst.h>

#if USE_PKCS11
#include <pk11/result.h>
#endif

#include "dnssectool.h"

#define MAX_RSA 4096 /* should be long enough... */

const char *program = "dnssec-keygen";
int verbose;

ISC_PLATFORM_NORETURN_PRE static void
usage(void) ISC_PLATFORM_NORETURN_POST;

static void progress(int p);

static void
usage(void) {
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "    %s [options] name\n\n", program);
	fprintf(stderr, "Version: %s\n", VERSION);
	fprintf(stderr, "    name: owner of the key\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "    -K <directory>: write keys into directory\n");
	fprintf(stderr, "    -a <algorithm>:\n");
	fprintf(stderr, "        RSASHA1 | NSEC3RSASHA1 |\n");
	fprintf(stderr, "        RSASHA256 | RSASHA512 |\n");
	fprintf(stderr, "        ECDSAP256SHA256 | ECDSAP384SHA384 |\n");
	fprintf(stderr, "        ED25519 | ED448 | DH\n");
	fprintf(stderr, "    -3: use NSEC3-capable algorithm\n");
	fprintf(stderr, "    -b <key size in bits>:\n");
	fprintf(stderr, "        RSASHA1:\t[1024..%d]\n", MAX_RSA);
	fprintf(stderr, "        NSEC3RSASHA1:\t[1024..%d]\n", MAX_RSA);
	fprintf(stderr, "        RSASHA256:\t[1024..%d]\n", MAX_RSA);
	fprintf(stderr, "        RSASHA512:\t[1024..%d]\n", MAX_RSA);
	fprintf(stderr, "        DH:\t\t[128..4096]\n");
	fprintf(stderr, "        ECDSAP256SHA256:\tignored\n");
	fprintf(stderr, "        ECDSAP384SHA384:\tignored\n");
	fprintf(stderr, "        ED25519:\tignored\n");
	fprintf(stderr, "        ED448:\tignored\n");
	fprintf(stderr, "        (key size defaults are set according to\n"
			"        algorithm and usage (ZSK or KSK)\n");
	fprintf(stderr, "    -n <nametype>: ZONE | HOST | ENTITY | "
					    "USER | OTHER\n");
	fprintf(stderr, "        (DNSKEY generation defaults to ZONE)\n");
	fprintf(stderr, "    -c <class>: (default: IN)\n");
	fprintf(stderr, "    -d <digest bits> (0 => max, default)\n");
	fprintf(stderr, "    -E <engine>:\n");
#if USE_PKCS11
	fprintf(stderr, "        path to PKCS#11 provider library "
				"(default is %s)\n", PK11_LIB_LOCATION);
#else
	fprintf(stderr, "        name of an OpenSSL engine to use\n");
#endif
	fprintf(stderr, "    -f <keyflag>: KSK | REVOKE\n");
	fprintf(stderr, "    -g <generator>: use specified generator "
			"(DH only)\n");
	fprintf(stderr, "    -L <ttl>: default key TTL\n");
	fprintf(stderr, "    -p <protocol>: (default: 3 [dnssec])\n");
	fprintf(stderr, "    -s <strength>: strength value this key signs DNS "
			"records with (default: 0)\n");
	fprintf(stderr, "    -T <rrtype>: DNSKEY | KEY (default: DNSKEY; "
			"use KEY for SIG(0))\n");
	fprintf(stderr, "    -t <type>: "
			"AUTHCONF | NOAUTHCONF | NOAUTH | NOCONF "
			"(default: AUTHCONF)\n");
	fprintf(stderr, "    -h: print usage and exit\n");
	fprintf(stderr, "    -m <memory debugging mode>:\n");
	fprintf(stderr, "       usage | trace | record | size | mctx\n");
	fprintf(stderr, "    -v <level>: set verbosity level (0 - 10)\n");
	fprintf(stderr, "    -V: print version information\n");
	fprintf(stderr, "Timing options:\n");
	fprintf(stderr, "    -P date/[+-]offset/none: set key publication date "
						"(default: now)\n");
	fprintf(stderr, "    -P sync date/[+-]offset/none: set CDS and CDNSKEY "
						"publication date\n");
	fprintf(stderr, "    -A date/[+-]offset/none: set key activation date "
						"(default: now)\n");
	fprintf(stderr, "    -R date/[+-]offset/none: set key "
						"revocation date\n");
	fprintf(stderr, "    -I date/[+-]offset/none: set key "
						"inactivation date\n");
	fprintf(stderr, "    -D date/[+-]offset/none: set key deletion date\n");
	fprintf(stderr, "    -D sync date/[+-]offset/none: set CDS and CDNSKEY "
						"deletion date\n");

	fprintf(stderr, "    -G: generate key only; do not set -P or -A\n");
	fprintf(stderr, "    -C: generate a backward-compatible key, omitting "
			"all dates\n");
	fprintf(stderr, "    -S <key>: generate a successor to an existing "
				      "key\n");
	fprintf(stderr, "    -i <interval>: prepublication interval for "
					   "successor key "
					   "(default: 30 days)\n");
	fprintf(stderr, "Output:\n");
	fprintf(stderr, "     K<name>+<alg>+<id>.key, "
			"K<name>+<alg>+<id>.private\n");

	exit (-1);
}

static void
progress(int p)
{
	char c = '*';

	switch (p) {
	case 0:
		c = '.';
		break;
	case 1:
		c = '+';
		break;
	case 2:
		c = '*';
		break;
	case 3:
		c = ' ';
		break;
	default:
		break;
	}
	(void) putc(c, stderr);
	(void) fflush(stderr);
}

int
main(int argc, char **argv) {
	char		*algname = NULL, *freeit = NULL;
	char		*nametype = NULL, *type = NULL;
	char		*classname = NULL;
	char		*endp;
	dst_key_t	*key = NULL;
	dns_fixedname_t	fname;
	dns_name_t	*name;
	uint16_t	flags = 0, kskflag = 0, revflag = 0;
	dns_secalg_t	alg;
	bool	conflict = false, null_key = false;
	bool	oldstyle = false;
	isc_mem_t	*mctx = NULL;
	int		ch, generator = 0, param = 0;
	int		protocol = -1, size = -1, signatory = 0;
	isc_result_t	ret;
	isc_textregion_t r;
	char		filename[255];
	const char	*directory = NULL;
	const char	*predecessor = NULL;
	dst_key_t	*prevkey = NULL;
	isc_buffer_t	buf;
	isc_log_t	*log = NULL;
	const char	*engine = NULL;
	dns_rdataclass_t rdclass;
	int		options = DST_TYPE_PRIVATE | DST_TYPE_PUBLIC;
	int		dbits = 0;
	dns_ttl_t	ttl = 0;
	bool	use_nsec3 = false;
	isc_stdtime_t	publish = 0, activate = 0, revokekey = 0;
	isc_stdtime_t	inactive = 0, deltime = 0;
	isc_stdtime_t	now;
	int		prepub = -1;
	bool	setpub = false, setact = false;
	bool	setrev = false, setinact = false;
	bool	setdel = false, setttl = false;
	bool	unsetpub = false, unsetact = false;
	bool	unsetrev = false, unsetinact = false;
	bool	unsetdel = false;
	bool	genonly = false;
	bool	quiet = false;
	bool	show_progress = false;
	unsigned char	c;
	isc_stdtime_t	syncadd = 0, syncdel = 0;
	bool	setsyncadd = false;
	bool	setsyncdel = false;

	if (argc == 1)
		usage();

#if USE_PKCS11
	pk11_result_register();
#endif
	dns_result_register();

	isc_commandline_errprint = false;

	/*
	 * Process memory debugging argument first.
	 */
#define CMDLINE_FLAGS "3A:a:b:Cc:D:d:E:eFf:Gg:hI:i:K:L:m:n:P:p:qR:r:S:s:T:t:" \
		      "v:V"
	while ((ch = isc_commandline_parse(argc, argv, CMDLINE_FLAGS)) != -1) {
		switch (ch) {
		case 'm':
			if (strcasecmp(isc_commandline_argument, "record") == 0)
				isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
			if (strcasecmp(isc_commandline_argument, "trace") == 0)
				isc_mem_debugging |= ISC_MEM_DEBUGTRACE;
			if (strcasecmp(isc_commandline_argument, "usage") == 0)
				isc_mem_debugging |= ISC_MEM_DEBUGUSAGE;
			if (strcasecmp(isc_commandline_argument, "size") == 0)
				isc_mem_debugging |= ISC_MEM_DEBUGSIZE;
			if (strcasecmp(isc_commandline_argument, "mctx") == 0)
				isc_mem_debugging |= ISC_MEM_DEBUGCTX;
			break;
		default:
			break;
		}
	}
	isc_commandline_reset = true;

	RUNTIME_CHECK(isc_mem_create(0, 0, &mctx) == ISC_R_SUCCESS);

	isc_stdtime_get(&now);

	while ((ch = isc_commandline_parse(argc, argv, CMDLINE_FLAGS)) != -1) {
	    switch (ch) {
		case '3':
			use_nsec3 = true;
			break;
		case 'a':
			algname = isc_commandline_argument;
			break;
		case 'b':
			size = strtol(isc_commandline_argument, &endp, 10);
			if (*endp != '\0' || size < 0)
				fatal("-b requires a non-negative number");
			break;
		case 'C':
			oldstyle = true;
			break;
		case 'c':
			classname = isc_commandline_argument;
			break;
		case 'd':
			dbits = strtol(isc_commandline_argument, &endp, 10);
			if (*endp != '\0' || dbits < 0)
				fatal("-d requires a non-negative number");
			break;
		case 'E':
			engine = isc_commandline_argument;
			break;
		case 'e':
			fprintf(stderr,
				"phased-out option -e "
				"(was 'use (RSA) large exponent')\n");
			break;
		case 'f':
			c = (unsigned char)(isc_commandline_argument[0]);
			if (toupper(c) == 'K')
				kskflag = DNS_KEYFLAG_KSK;
			else if (toupper(c) == 'R')
				revflag = DNS_KEYFLAG_REVOKE;
			else
				fatal("unknown flag '%s'",
				      isc_commandline_argument);
			break;
		case 'g':
			generator = strtol(isc_commandline_argument,
					   &endp, 10);
			if (*endp != '\0' || generator <= 0)
				fatal("-g requires a positive number");
			break;
		case 'K':
			directory = isc_commandline_argument;
			ret = try_dir(directory);
			if (ret != ISC_R_SUCCESS)
				fatal("cannot open directory %s: %s",
				      directory, isc_result_totext(ret));
			break;
		case 'L':
			ttl = strtottl(isc_commandline_argument);
			setttl = true;
			break;
		case 'n':
			nametype = isc_commandline_argument;
			break;
		case 'm':
			break;
		case 'p':
			protocol = strtol(isc_commandline_argument, &endp, 10);
			if (*endp != '\0' || protocol < 0 || protocol > 255)
				fatal("-p must be followed by a number "
				      "[0..255]");
			break;
		case 'q':
			quiet = true;
			break;
		case 'r':
			fatal("The -r option has been deprecated.\n"
			      "System random data is always used.\n");
			break;
		case 's':
			signatory = strtol(isc_commandline_argument,
					   &endp, 10);
			if (*endp != '\0' || signatory < 0 || signatory > 15)
				fatal("-s must be followed by a number "
				      "[0..15]");
			break;
		case 'T':
			if (strcasecmp(isc_commandline_argument, "KEY") == 0)
				options |= DST_TYPE_KEY;
			else if (strcasecmp(isc_commandline_argument,
				 "DNSKEY") == 0)
				/* default behavior */
				;
			else
				fatal("unknown type '%s'",
				      isc_commandline_argument);
			break;
		case 't':
			type = isc_commandline_argument;
			break;
		case 'v':
			endp = NULL;
			verbose = strtol(isc_commandline_argument, &endp, 0);
			if (*endp != '\0')
				fatal("-v must be followed by a number");
			break;
		case 'z':
			/* already the default */
			break;
		case 'G':
			genonly = true;
			break;
		case 'P':
			/* -Psync ? */
			if (isoptarg("sync", argv, usage)) {
				if (setsyncadd)
					fatal("-P sync specified more than "
					      "once");

				syncadd = strtotime(isc_commandline_argument,
						   now, now, &setsyncadd);
				break;
			}
			(void)isoptarg("dnskey", argv, usage);
			if (setpub || unsetpub)
				fatal("-P specified more than once");

			publish = strtotime(isc_commandline_argument,
					    now, now, &setpub);
			unsetpub = !setpub;
			break;
		case 'A':
			if (setact || unsetact)
				fatal("-A specified more than once");

			activate = strtotime(isc_commandline_argument,
					     now, now, &setact);
			unsetact = !setact;
			break;
		case 'R':
			if (setrev || unsetrev)
				fatal("-R specified more than once");

			revokekey = strtotime(isc_commandline_argument,
					   now, now, &setrev);
			unsetrev = !setrev;
			break;
		case 'I':
			if (setinact || unsetinact)
				fatal("-I specified more than once");

			inactive = strtotime(isc_commandline_argument,
					     now, now, &setinact);
			unsetinact = !setinact;
			break;
		case 'D':
			/* -Dsync ? */
			if (isoptarg("sync", argv, usage)) {
				if (setsyncdel)
					fatal("-D sync specified more than "
					      "once");

				syncdel = strtotime(isc_commandline_argument,
						   now, now, &setsyncdel);
				break;
			}
			(void)isoptarg("dnskey", argv, usage);
			if (setdel || unsetdel)
				fatal("-D specified more than once");

			deltime = strtotime(isc_commandline_argument,
					    now, now, &setdel);
			unsetdel = !setdel;
			break;
		case 'S':
			predecessor = isc_commandline_argument;
			break;
		case 'i':
			prepub = strtottl(isc_commandline_argument);
			break;
		case 'F':
			/* Reserved for FIPS mode */
			/* FALLTHROUGH */
		case '?':
			if (isc_commandline_option != '?')
				fprintf(stderr, "%s: invalid argument -%c\n",
					program, isc_commandline_option);
			/* FALLTHROUGH */
		case 'h':
			/* Does not return. */
			usage();

		case 'V':
			/* Does not return. */
			version(program);

		default:
			fprintf(stderr, "%s: unhandled option -%c\n",
				program, isc_commandline_option);
			exit(1);
		}
	}

	if (!isatty(0))
		quiet = true;

	ret = dst_lib_init(mctx, engine);
	if (ret != ISC_R_SUCCESS)
		fatal("could not initialize dst: %s",
		      isc_result_totext(ret));

	setup_logging(mctx, &log);

	if (predecessor == NULL) {
		if (prepub == -1)
			prepub = 0;

		if (argc < isc_commandline_index + 1)
			fatal("the key name was not specified");
		if (argc > isc_commandline_index + 1)
			fatal("extraneous arguments");

		name = dns_fixedname_initname(&fname);
		isc_buffer_init(&buf, argv[isc_commandline_index],
				strlen(argv[isc_commandline_index]));
		isc_buffer_add(&buf, strlen(argv[isc_commandline_index]));
		ret = dns_name_fromtext(name, &buf, dns_rootname, 0, NULL);
		if (ret != ISC_R_SUCCESS)
			fatal("invalid key name %s: %s",
			      argv[isc_commandline_index],
			      isc_result_totext(ret));

		if (algname == NULL) {
			fatal("no algorithm specified");
		}

		r.base = algname;
		r.length = strlen(algname);
		ret = dns_secalg_fromtext(&alg, &r);
		if (ret != ISC_R_SUCCESS) {
			fatal("unknown algorithm %s", algname);
		}
		if (alg == DST_ALG_DH) {
			options |= DST_TYPE_KEY;
		}

		if (!dst_algorithm_supported(alg)) {
			fatal("unsupported algorithm: %d", alg);
		}

		if (use_nsec3) {
			switch (alg) {
			case DST_ALG_RSASHA1:
				alg = DST_ALG_NSEC3RSASHA1;
				break;
			case DST_ALG_NSEC3RSASHA1:
			case DST_ALG_RSASHA256:
			case DST_ALG_RSASHA512:
			case DST_ALG_ECDSA256:
			case DST_ALG_ECDSA384:
			case DST_ALG_ED25519:
			case DST_ALG_ED448:
				break;
			default:
				fatal("%s is incompatible with NSEC3; "
				      "do not use the -3 option", algname);
			}
		}

		if (type != NULL && (options & DST_TYPE_KEY) != 0) {
			if (strcasecmp(type, "NOAUTH") == 0) {
				flags |= DNS_KEYTYPE_NOAUTH;
			} else if (strcasecmp(type, "NOCONF") == 0) {
				flags |= DNS_KEYTYPE_NOCONF;
			} else if (strcasecmp(type, "NOAUTHCONF") == 0) {
				flags |= (DNS_KEYTYPE_NOAUTH |
					  DNS_KEYTYPE_NOCONF);
				if (size < 0)
					size = 0;
			} else if (strcasecmp(type, "AUTHCONF") == 0) {
				/* nothing */;
			} else {
				fatal("invalid type %s", type);
			}
		}

		if (size < 0) {
			switch (alg) {
			case DST_ALG_RSASHA1:
			case DST_ALG_NSEC3RSASHA1:
			case DST_ALG_RSASHA256:
			case DST_ALG_RSASHA512:
				if ((kskflag & DNS_KEYFLAG_KSK) != 0) {
					size = 2048;
				} else {
					size = 1024;
				}
				if (verbose > 0) {
					fprintf(stderr, "key size not "
							"specified; defaulting"
							" to %d\n", size);
				}
				break;
			case DST_ALG_ECDSA256:
			case DST_ALG_ECDSA384:
			case DST_ALG_ED25519:
			case DST_ALG_ED448:
				break;
			default:
				fatal("key size not specified (-b option)");
			}
		}

		if (!oldstyle && prepub > 0) {
			if (setpub && setact && (activate - prepub) < publish)
				fatal("Activation and publication dates "
				      "are closer together than the\n\t"
				      "prepublication interval.");

			if (!setpub && !setact) {
				setpub = setact = true;
				publish = now;
				activate = now + prepub;
			} else if (setpub && !setact) {
				setact = true;
				activate = publish + prepub;
			} else if (setact && !setpub) {
				setpub = true;
				publish = activate - prepub;
			}

			if ((activate - prepub) < now)
				fatal("Time until activation is shorter "
				      "than the\n\tprepublication interval.");
		}
	} else {
		char keystr[DST_KEY_FORMATSIZE];
		isc_stdtime_t when;
		int major, minor;

		if (prepub == -1)
			prepub = (30 * 86400);

		if (algname != NULL)
			fatal("-S and -a cannot be used together");
		if (size >= 0)
			fatal("-S and -b cannot be used together");
		if (nametype != NULL)
			fatal("-S and -n cannot be used together");
		if (type != NULL)
			fatal("-S and -t cannot be used together");
		if (setpub || unsetpub)
			fatal("-S and -P cannot be used together");
		if (setact || unsetact)
			fatal("-S and -A cannot be used together");
		if (use_nsec3)
			fatal("-S and -3 cannot be used together");
		if (oldstyle)
			fatal("-S and -C cannot be used together");
		if (genonly)
			fatal("-S and -G cannot be used together");

		ret = dst_key_fromnamedfile(predecessor, directory,
					    DST_TYPE_PUBLIC | DST_TYPE_PRIVATE,
					    mctx, &prevkey);
		if (ret != ISC_R_SUCCESS)
			fatal("Invalid keyfile %s: %s",
			      predecessor, isc_result_totext(ret));
		if (!dst_key_isprivate(prevkey))
			fatal("%s is not a private key", predecessor);

		name = dst_key_name(prevkey);
		alg = dst_key_alg(prevkey);
		size = dst_key_size(prevkey);
		flags = dst_key_flags(prevkey);

		dst_key_format(prevkey, keystr, sizeof(keystr));
		dst_key_getprivateformat(prevkey, &major, &minor);
		if (major != DST_MAJOR_VERSION || minor < DST_MINOR_VERSION)
			fatal("Key %s has incompatible format version %d.%d\n\t"
			      "It is not possible to generate a successor key.",
			      keystr, major, minor);

		ret = dst_key_gettime(prevkey, DST_TIME_ACTIVATE, &when);
		if (ret != ISC_R_SUCCESS)
			fatal("Key %s has no activation date.\n\t"
			      "You must use dnssec-settime -A to set one "
			      "before generating a successor.", keystr);

		ret = dst_key_gettime(prevkey, DST_TIME_INACTIVE, &activate);
		if (ret != ISC_R_SUCCESS)
			fatal("Key %s has no inactivation date.\n\t"
			      "You must use dnssec-settime -I to set one "
			      "before generating a successor.", keystr);

		publish = activate - prepub;
		if (publish < now)
			fatal("Key %s becomes inactive\n\t"
			      "sooner than the prepublication period "
			      "for the new key ends.\n\t"
			      "Either change the inactivation date with "
			      "dnssec-settime -I,\n\t"
			      "or use the -i option to set a shorter "
			      "prepublication interval.", keystr);

		ret = dst_key_gettime(prevkey, DST_TIME_DELETE, &when);
		if (ret != ISC_R_SUCCESS)
			fprintf(stderr, "%s: WARNING: Key %s has no removal "
					"date;\n\t it will remain in the zone "
					"indefinitely after rollover.\n\t "
					"You can use dnssec-settime -D to "
					"change this.\n", program, keystr);

		setpub = setact = true;
	}

	switch (alg) {
	case DNS_KEYALG_RSASHA1:
	case DNS_KEYALG_NSEC3RSASHA1:
	case DNS_KEYALG_RSASHA256:
		if (size != 0 && (size < 1024 || size > MAX_RSA))
			fatal("RSA key size %d out of range", size);
		break;
	case DNS_KEYALG_RSASHA512:
		if (size != 0 && (size < 1024 || size > MAX_RSA))
			fatal("RSA key size %d out of range", size);
		break;
	case DNS_KEYALG_DH:
		if (size != 0 && (size < 128 || size > 4096))
			fatal("DH key size %d out of range", size);
		break;
	case DST_ALG_ECDSA256:
		size = 256;
		break;
	case DST_ALG_ECDSA384:
		size = 384;
		break;
	case DST_ALG_ED25519:
		size = 256;
		break;
	case DST_ALG_ED448:
		size = 456;
		break;
	}

	if (alg != DNS_KEYALG_DH && generator != 0)
		fatal("specified DH generator for a non-DH key");

	if (nametype == NULL) {
		if ((options & DST_TYPE_KEY) != 0) /* KEY */
			fatal("no nametype specified");
		flags |= DNS_KEYOWNER_ZONE;	/* DNSKEY */
	} else if (strcasecmp(nametype, "zone") == 0)
		flags |= DNS_KEYOWNER_ZONE;
	else if ((options & DST_TYPE_KEY) != 0)	{ /* KEY */
		if (strcasecmp(nametype, "host") == 0 ||
			 strcasecmp(nametype, "entity") == 0)
			flags |= DNS_KEYOWNER_ENTITY;
		else if (strcasecmp(nametype, "user") == 0)
			flags |= DNS_KEYOWNER_USER;
		else
			fatal("invalid KEY nametype %s", nametype);
	} else if (strcasecmp(nametype, "other") != 0) /* DNSKEY */
		fatal("invalid DNSKEY nametype %s", nametype);

	rdclass = strtoclass(classname);

	if (directory == NULL)
		directory = ".";

	if ((options & DST_TYPE_KEY) != 0)  /* KEY */
		flags |= signatory;
	else if ((flags & DNS_KEYOWNER_ZONE) != 0) { /* DNSKEY */
		flags |= kskflag;
		flags |= revflag;
	}

	if (protocol == -1)
		protocol = DNS_KEYPROTO_DNSSEC;
	else if ((options & DST_TYPE_KEY) == 0 &&
		 protocol != DNS_KEYPROTO_DNSSEC)
		fatal("invalid DNSKEY protocol: %d", protocol);

	if ((flags & DNS_KEYFLAG_TYPEMASK) == DNS_KEYTYPE_NOKEY) {
		if (size > 0)
			fatal("specified null key with non-zero size");
		if ((flags & DNS_KEYFLAG_SIGNATORYMASK) != 0)
			fatal("specified null key with signing authority");
	}

	if ((flags & DNS_KEYFLAG_OWNERMASK) == DNS_KEYOWNER_ZONE &&
	    alg == DNS_KEYALG_DH)
	{
		fatal("a key with algorithm '%s' cannot be a zone key",
		      algname);
	}

	switch(alg) {
	case DNS_KEYALG_RSASHA1:
	case DNS_KEYALG_NSEC3RSASHA1:
	case DNS_KEYALG_RSASHA256:
	case DNS_KEYALG_RSASHA512:
		show_progress = true;
		break;

	case DNS_KEYALG_DH:
		param = generator;
		break;

	case DST_ALG_ECDSA256:
	case DST_ALG_ECDSA384:
	case DST_ALG_ED25519:
	case DST_ALG_ED448:
		show_progress = true;
		break;
	}

	if ((flags & DNS_KEYFLAG_TYPEMASK) == DNS_KEYTYPE_NOKEY)
		null_key = true;

	isc_buffer_init(&buf, filename, sizeof(filename) - 1);

	do {
		conflict = false;

		if (!quiet && show_progress) {
			fprintf(stderr, "Generating key pair.");
			ret = dst_key_generate(name, alg, size, param, flags,
					       protocol, rdclass, mctx, &key,
					       &progress);
			putc('\n', stderr);
			fflush(stderr);
		} else {
			ret = dst_key_generate(name, alg, size, param, flags,
					       protocol, rdclass, mctx, &key,
					       NULL);
		}

		if (ret != ISC_R_SUCCESS) {
			char namestr[DNS_NAME_FORMATSIZE];
			char algstr[DNS_SECALG_FORMATSIZE];
			dns_name_format(name, namestr, sizeof(namestr));
			dns_secalg_format(alg, algstr, sizeof(algstr));
			fatal("failed to generate key %s/%s: %s\n",
			      namestr, algstr, isc_result_totext(ret));
			/* NOTREACHED */
			exit(-1);
		}

		dst_key_setbits(key, dbits);

		/*
		 * Set key timing metadata (unless using -C)
		 *
		 * Creation date is always set to "now".
		 *
		 * For a new key without an explicit predecessor, publish
		 * and activation dates are set to "now" by default, but
		 * can both be overridden.
		 *
		 * For a successor key, activation is set to match the
		 * predecessor's inactivation date.  Publish is set to 30
		 * days earlier than that (XXX: this should be configurable).
		 * If either of the resulting dates are in the past, that's
		 * an error; the inactivation date of the predecessor key
		 * must be updated before a successor key can be created.
		 */
		if (!oldstyle) {
			dst_key_settime(key, DST_TIME_CREATED, now);

			if (genonly && (setpub || setact))
				fatal("cannot use -G together with "
				      "-P or -A options");

			if (setpub)
				dst_key_settime(key, DST_TIME_PUBLISH, publish);
			else if (setact && !unsetpub)
				dst_key_settime(key, DST_TIME_PUBLISH,
						activate - prepub);
			else if (!genonly && !unsetpub)
				dst_key_settime(key, DST_TIME_PUBLISH, now);

			if (setact)
				dst_key_settime(key, DST_TIME_ACTIVATE,
						activate);
			else if (!genonly && !unsetact)
				dst_key_settime(key, DST_TIME_ACTIVATE, now);

			if (setrev) {
				if (kskflag == 0)
					fprintf(stderr, "%s: warning: Key is "
						"not flagged as a KSK, but -R "
						"was used. Revoking a ZSK is "
						"legal, but undefined.\n",
						program);
				dst_key_settime(key, DST_TIME_REVOKE, revokekey);
			}

			if (setinact)
				dst_key_settime(key, DST_TIME_INACTIVE,
						inactive);

			if (setdel) {
				if (setinact && deltime < inactive)
					fprintf(stderr, "%s: warning: Key is "
						"scheduled to be deleted "
						"before it is scheduled to be "
						"made inactive.\n",
						program);
				dst_key_settime(key, DST_TIME_DELETE, deltime);
			}

			if (setsyncadd)
				dst_key_settime(key, DST_TIME_SYNCPUBLISH,
						syncadd);

			if (setsyncdel)
				dst_key_settime(key, DST_TIME_SYNCDELETE,
						syncdel);

		} else {
			if (setpub || setact || setrev || setinact ||
			    setdel || unsetpub || unsetact ||
			    unsetrev || unsetinact || unsetdel || genonly ||
			    setsyncadd || setsyncdel)
				fatal("cannot use -C together with "
				      "-P, -A, -R, -I, -D, or -G options");
			/*
			 * Compatibility mode: Private-key-format
			 * should be set to 1.2.
			 */
			dst_key_setprivateformat(key, 1, 2);
		}

		/* Set the default key TTL */
		if (setttl)
			dst_key_setttl(key, ttl);

		/*
		 * Do not overwrite an existing key, or create a key
		 * if there is a risk of ID collision due to this key
		 * or another key being revoked.
		 */
		if (key_collision(key, name, directory, mctx, NULL)) {
			conflict = true;
			if (null_key) {
				dst_key_free(&key);
				break;
			}

			if (verbose > 0) {
				isc_buffer_clear(&buf);
				ret = dst_key_buildfilename(key, 0,
							    directory, &buf);
				if (ret == ISC_R_SUCCESS)
					fprintf(stderr,
						"%s: %s already exists, or "
						"might collide with another "
						"key upon revokation.  "
						"Generating a new key\n",
						program, filename);
			}

			dst_key_free(&key);
		}
	} while (conflict == true);

	if (conflict)
		fatal("cannot generate a null key due to possible key ID "
		      "collision");

	ret = dst_key_tofile(key, options, directory);
	if (ret != ISC_R_SUCCESS) {
		char keystr[DST_KEY_FORMATSIZE];
		dst_key_format(key, keystr, sizeof(keystr));
		fatal("failed to write key %s: %s\n", keystr,
		      isc_result_totext(ret));
	}

	isc_buffer_clear(&buf);
	ret = dst_key_buildfilename(key, 0, NULL, &buf);
	if (ret != ISC_R_SUCCESS)
		fatal("dst_key_buildfilename returned: %s\n",
		      isc_result_totext(ret));
	printf("%s\n", filename);
	dst_key_free(&key);
	if (prevkey != NULL)
		dst_key_free(&prevkey);

	cleanup_logging(&log);
	dst_lib_destroy();
	dns_name_destroy();
	if (verbose > 10)
		isc_mem_stats(mctx, stdout);
	isc_mem_destroy(&mctx);

	if (freeit != NULL)
		free(freeit);

	return (0);
}
