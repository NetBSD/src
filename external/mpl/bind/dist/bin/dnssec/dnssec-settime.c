/*	$NetBSD: dnssec-settime.c,v 1.7 2023/01/25 21:43:23 christos Exp $	*/

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

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/file.h>
#include <isc/hash.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/result.h>

#include <dst/dst.h>

#if USE_PKCS11
#include <pk11/result.h>
#endif /* if USE_PKCS11 */

#include "dnssectool.h"

const char *program = "dnssec-settime";

static isc_mem_t *mctx = NULL;

ISC_PLATFORM_NORETURN_PRE static void
usage(void) ISC_PLATFORM_NORETURN_POST;

static void
usage(void) {
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "    %s [options] keyfile\n\n", program);
	fprintf(stderr, "Version: %s\n", VERSION);
	fprintf(stderr, "General options:\n");
#if USE_PKCS11
	fprintf(stderr,
		"    -E engine:          specify PKCS#11 provider "
		"(default: %s)\n",
		PK11_LIB_LOCATION);
#elif defined(USE_PKCS11)
	fprintf(stderr, "    -E engine:          specify OpenSSL engine "
			"(default \"pkcs11\")\n");
#else  /* if USE_PKCS11 */
	fprintf(stderr, "    -E engine:          specify OpenSSL engine\n");
#endif /* if USE_PKCS11 */
	fprintf(stderr, "    -f:                 force update of old-style "
			"keys\n");
	fprintf(stderr, "    -K directory:       set key file location\n");
	fprintf(stderr, "    -L ttl:             set default key TTL\n");
	fprintf(stderr, "    -v level:           set level of verbosity\n");
	fprintf(stderr, "    -V:                 print version information\n");
	fprintf(stderr, "    -h:                 help\n");
	fprintf(stderr, "Timing options:\n");
	fprintf(stderr, "    -P date/[+-]offset/none: set/unset key "
			"publication date\n");
	fprintf(stderr, "    -P ds date/[+-]offset/none: set/unset "
			"DS publication date\n");
	fprintf(stderr, "    -P sync date/[+-]offset/none: set/unset "
			"CDS and CDNSKEY publication date\n");
	fprintf(stderr, "    -A date/[+-]offset/none: set/unset key "
			"activation date\n");
	fprintf(stderr, "    -R date/[+-]offset/none: set/unset key "
			"revocation date\n");
	fprintf(stderr, "    -I date/[+-]offset/none: set/unset key "
			"inactivation date\n");
	fprintf(stderr, "    -D date/[+-]offset/none: set/unset key "
			"deletion date\n");
	fprintf(stderr, "    -D ds date/[+-]offset/none: set/unset "
			"DS deletion date\n");
	fprintf(stderr, "    -D sync date/[+-]offset/none: set/unset "
			"CDS and CDNSKEY deletion date\n");
	fprintf(stderr, "    -S <key>: generate a successor to an existing "
			"key\n");
	fprintf(stderr, "    -i <interval>: prepublication interval for "
			"successor key "
			"(default: 30 days)\n");
	fprintf(stderr, "Key state options:\n");
	fprintf(stderr, "    -s: update key state file (default no)\n");
	fprintf(stderr, "    -g state: set the goal state for this key\n");
	fprintf(stderr, "    -d state date/[+-]offset: set the DS state\n");
	fprintf(stderr, "    -k state date/[+-]offset: set the DNSKEY state\n");
	fprintf(stderr, "    -r state date/[+-]offset: set the RRSIG (KSK) "
			"state\n");
	fprintf(stderr, "    -z state date/[+-]offset: set the RRSIG (ZSK) "
			"state\n");
	fprintf(stderr, "Printing options:\n");
	fprintf(stderr, "    -p C/P/Psync/A/R/I/D/Dsync/all: print a "
			"particular time value or values\n");
	fprintf(stderr, "    -u:                 print times in unix epoch "
			"format\n");
	fprintf(stderr, "Output:\n");
	fprintf(stderr, "     K<name>+<alg>+<new id>.key, "
			"K<name>+<alg>+<new id>.private\n");

	exit(-1);
}

static void
printtime(dst_key_t *key, int type, const char *tag, bool epoch, FILE *stream) {
	isc_result_t result;
	isc_stdtime_t when;

	if (tag != NULL) {
		fprintf(stream, "%s: ", tag);
	}

	result = dst_key_gettime(key, type, &when);
	if (result == ISC_R_NOTFOUND) {
		fprintf(stream, "UNSET\n");
	} else if (epoch) {
		fprintf(stream, "%d\n", (int)when);
	} else {
		time_t now = when;
		struct tm t, *tm = localtime_r(&now, &t);
		unsigned int flen;
		char timebuf[80];

		if (tm == NULL) {
			fprintf(stream, "INVALID\n");
			return;
		}

		flen = strftime(timebuf, sizeof(timebuf),
				"%a %b %e %H:%M:%S %Y", tm);
		INSIST(flen > 0U && flen < sizeof(timebuf));
		fprintf(stream, "%s\n", timebuf);
	}
}

static void
writekey(dst_key_t *key, const char *directory, bool write_state) {
	char newname[1024];
	char keystr[DST_KEY_FORMATSIZE];
	isc_buffer_t buf;
	isc_result_t result;
	int options = DST_TYPE_PUBLIC | DST_TYPE_PRIVATE;

	if (write_state) {
		options |= DST_TYPE_STATE;
	}

	isc_buffer_init(&buf, newname, sizeof(newname));
	result = dst_key_buildfilename(key, DST_TYPE_PUBLIC, directory, &buf);
	if (result != ISC_R_SUCCESS) {
		fatal("Failed to build public key filename: %s",
		      isc_result_totext(result));
	}

	result = dst_key_tofile(key, options, directory);
	if (result != ISC_R_SUCCESS) {
		dst_key_format(key, keystr, sizeof(keystr));
		fatal("Failed to write key %s: %s", keystr,
		      isc_result_totext(result));
	}
	printf("%s\n", newname);

	isc_buffer_clear(&buf);
	result = dst_key_buildfilename(key, DST_TYPE_PRIVATE, directory, &buf);
	if (result != ISC_R_SUCCESS) {
		fatal("Failed to build private key filename: %s",
		      isc_result_totext(result));
	}
	printf("%s\n", newname);

	if (write_state) {
		isc_buffer_clear(&buf);
		result = dst_key_buildfilename(key, DST_TYPE_STATE, directory,
					       &buf);
		if (result != ISC_R_SUCCESS) {
			fatal("Failed to build key state filename: %s",
			      isc_result_totext(result));
		}
		printf("%s\n", newname);
	}
}

int
main(int argc, char **argv) {
	isc_result_t result;
	const char *engine = NULL;
	const char *filename = NULL;
	char *directory = NULL;
	char keystr[DST_KEY_FORMATSIZE];
	char *endp, *p;
	int ch;
	const char *predecessor = NULL;
	dst_key_t *prevkey = NULL;
	dst_key_t *key = NULL;
	dns_name_t *name = NULL;
	dns_secalg_t alg = 0;
	unsigned int size = 0;
	uint16_t flags = 0;
	int prepub = -1;
	int options;
	dns_ttl_t ttl = 0;
	isc_stdtime_t now;
	isc_stdtime_t dstime = 0, dnskeytime = 0;
	isc_stdtime_t krrsigtime = 0, zrrsigtime = 0;
	isc_stdtime_t pub = 0, act = 0, rev = 0, inact = 0, del = 0;
	isc_stdtime_t prevact = 0, previnact = 0, prevdel = 0;
	dst_key_state_t goal = DST_KEY_STATE_NA;
	dst_key_state_t ds = DST_KEY_STATE_NA;
	dst_key_state_t dnskey = DST_KEY_STATE_NA;
	dst_key_state_t krrsig = DST_KEY_STATE_NA;
	dst_key_state_t zrrsig = DST_KEY_STATE_NA;
	bool setgoal = false, setds = false, setdnskey = false;
	bool setkrrsig = false, setzrrsig = false;
	bool setdstime = false, setdnskeytime = false;
	bool setkrrsigtime = false, setzrrsigtime = false;
	bool setpub = false, setact = false;
	bool setrev = false, setinact = false;
	bool setdel = false, setttl = false;
	bool unsetpub = false, unsetact = false;
	bool unsetrev = false, unsetinact = false;
	bool unsetdel = false;
	bool printcreate = false, printpub = false;
	bool printact = false, printrev = false;
	bool printinact = false, printdel = false;
	bool force = false;
	bool epoch = false;
	bool changed = false;
	bool write_state = false;
	isc_log_t *log = NULL;
	isc_stdtime_t syncadd = 0, syncdel = 0;
	bool unsetsyncadd = false, setsyncadd = false;
	bool unsetsyncdel = false, setsyncdel = false;
	bool printsyncadd = false, printsyncdel = false;
	isc_stdtime_t dsadd = 0, dsdel = 0;
	bool unsetdsadd = false, setdsadd = false;
	bool unsetdsdel = false, setdsdel = false;
	bool printdsadd = false, printdsdel = false;

	options = DST_TYPE_PUBLIC | DST_TYPE_PRIVATE | DST_TYPE_STATE;

	if (argc == 1) {
		usage();
	}

	isc_mem_create(&mctx);

	setup_logging(mctx, &log);

#if USE_PKCS11
	pk11_result_register();
#endif /* if USE_PKCS11 */
	dns_result_register();

	isc_commandline_errprint = false;

	isc_stdtime_get(&now);

#define CMDLINE_FLAGS "A:D:d:E:fg:hI:i:K:k:L:P:p:R:r:S:suv:Vz:"
	while ((ch = isc_commandline_parse(argc, argv, CMDLINE_FLAGS)) != -1) {
		switch (ch) {
		case 'A':
			if (setact || unsetact) {
				fatal("-A specified more than once");
			}

			changed = true;
			act = strtotime(isc_commandline_argument, now, now,
					&setact);
			unsetact = !setact;
			break;
		case 'D':
			/* -Dsync ? */
			if (isoptarg("sync", argv, usage)) {
				if (unsetsyncdel || setsyncdel) {
					fatal("-D sync specified more than "
					      "once");
				}

				changed = true;
				syncdel = strtotime(isc_commandline_argument,
						    now, now, &setsyncdel);
				unsetsyncdel = !setsyncdel;
				break;
			}
			/* -Dds ? */
			if (isoptarg("ds", argv, usage)) {
				if (unsetdsdel || setdsdel) {
					fatal("-D ds specified more than once");
				}

				changed = true;
				dsdel = strtotime(isc_commandline_argument, now,
						  now, &setdsdel);
				unsetdsdel = !setdsdel;
				break;
			}
			/* -Ddnskey ? */
			(void)isoptarg("dnskey", argv, usage);
			if (setdel || unsetdel) {
				fatal("-D specified more than once");
			}

			changed = true;
			del = strtotime(isc_commandline_argument, now, now,
					&setdel);
			unsetdel = !setdel;
			break;
		case 'd':
			if (setds) {
				fatal("-d specified more than once");
			}

			ds = strtokeystate(isc_commandline_argument);
			setds = true;
			/* time */
			(void)isoptarg(isc_commandline_argument, argv, usage);
			dstime = strtotime(isc_commandline_argument, now, now,
					   &setdstime);
			break;
		case 'E':
			engine = isc_commandline_argument;
			break;
		case 'f':
			force = true;
			break;
		case 'g':
			if (setgoal) {
				fatal("-g specified more than once");
			}

			goal = strtokeystate(isc_commandline_argument);
			if (goal != DST_KEY_STATE_NA &&
			    goal != DST_KEY_STATE_HIDDEN &&
			    goal != DST_KEY_STATE_OMNIPRESENT)
			{
				fatal("-g must be either none, hidden, or "
				      "omnipresent");
			}
			setgoal = true;
			break;
		case '?':
			if (isc_commandline_option != '?') {
				fprintf(stderr, "%s: invalid argument -%c\n",
					program, isc_commandline_option);
			}
			FALLTHROUGH;
		case 'h':
			/* Does not return. */
			usage();
		case 'I':
			if (setinact || unsetinact) {
				fatal("-I specified more than once");
			}

			changed = true;
			inact = strtotime(isc_commandline_argument, now, now,
					  &setinact);
			unsetinact = !setinact;
			break;
		case 'i':
			prepub = strtottl(isc_commandline_argument);
			break;
		case 'K':
			/*
			 * We don't have to copy it here, but do it to
			 * simplify cleanup later
			 */
			directory = isc_mem_strdup(mctx,
						   isc_commandline_argument);
			break;
		case 'k':
			if (setdnskey) {
				fatal("-k specified more than once");
			}

			dnskey = strtokeystate(isc_commandline_argument);
			setdnskey = true;
			/* time */
			(void)isoptarg(isc_commandline_argument, argv, usage);
			dnskeytime = strtotime(isc_commandline_argument, now,
					       now, &setdnskeytime);
			break;
		case 'L':
			ttl = strtottl(isc_commandline_argument);
			setttl = true;
			break;
		case 'P':
			/* -Psync ? */
			if (isoptarg("sync", argv, usage)) {
				if (unsetsyncadd || setsyncadd) {
					fatal("-P sync specified more than "
					      "once");
				}

				changed = true;
				syncadd = strtotime(isc_commandline_argument,
						    now, now, &setsyncadd);
				unsetsyncadd = !setsyncadd;
				break;
			}
			/* -Pds ? */
			if (isoptarg("ds", argv, usage)) {
				if (unsetdsadd || setdsadd) {
					fatal("-P ds specified more than once");
				}

				changed = true;
				dsadd = strtotime(isc_commandline_argument, now,
						  now, &setdsadd);
				unsetdsadd = !setdsadd;
				break;
			}
			/* -Pdnskey ? */
			(void)isoptarg("dnskey", argv, usage);
			if (setpub || unsetpub) {
				fatal("-P specified more than once");
			}

			changed = true;
			pub = strtotime(isc_commandline_argument, now, now,
					&setpub);
			unsetpub = !setpub;
			break;
		case 'p':
			p = isc_commandline_argument;
			if (!strcasecmp(p, "all")) {
				printcreate = true;
				printpub = true;
				printact = true;
				printrev = true;
				printinact = true;
				printdel = true;
				printsyncadd = true;
				printsyncdel = true;
				printdsadd = true;
				printdsdel = true;
				break;
			}

			do {
				switch (*p++) {
				case 'A':
					printact = true;
					break;
				case 'C':
					printcreate = true;
					break;
				case 'D':
					if (!strncmp(p, "sync", 4)) {
						p += 4;
						printsyncdel = true;
						break;
					}
					if (!strncmp(p, "ds", 2)) {
						p += 2;
						printdsdel = true;
						break;
					}
					printdel = true;
					break;
				case 'I':
					printinact = true;
					break;
				case 'P':
					if (!strncmp(p, "sync", 4)) {
						p += 4;
						printsyncadd = true;
						break;
					}
					if (!strncmp(p, "ds", 2)) {
						p += 2;
						printdsadd = true;
						break;
					}
					printpub = true;
					break;
				case 'R':
					printrev = true;
					break;
				case ' ':
					break;
				default:
					usage();
					break;
				}
			} while (*p != '\0');
			break;
		case 'R':
			if (setrev || unsetrev) {
				fatal("-R specified more than once");
			}

			changed = true;
			rev = strtotime(isc_commandline_argument, now, now,
					&setrev);
			unsetrev = !setrev;
			break;
		case 'r':
			if (setkrrsig) {
				fatal("-r specified more than once");
			}

			krrsig = strtokeystate(isc_commandline_argument);
			setkrrsig = true;
			/* time */
			(void)isoptarg(isc_commandline_argument, argv, usage);
			krrsigtime = strtotime(isc_commandline_argument, now,
					       now, &setkrrsigtime);
			break;
		case 'S':
			predecessor = isc_commandline_argument;
			break;
		case 's':
			write_state = true;
			break;
		case 'u':
			epoch = true;
			break;
		case 'V':
			/* Does not return. */
			version(program);
		case 'v':
			verbose = strtol(isc_commandline_argument, &endp, 0);
			if (*endp != '\0') {
				fatal("-v must be followed by a number");
			}
			break;
		case 'z':
			if (setzrrsig) {
				fatal("-z specified more than once");
			}

			zrrsig = strtokeystate(isc_commandline_argument);
			setzrrsig = true;
			(void)isoptarg(isc_commandline_argument, argv, usage);
			zrrsigtime = strtotime(isc_commandline_argument, now,
					       now, &setzrrsigtime);
			break;

		default:
			fprintf(stderr, "%s: unhandled option -%c\n", program,
				isc_commandline_option);
			exit(1);
		}
	}

	if (argc < isc_commandline_index + 1 ||
	    argv[isc_commandline_index] == NULL)
	{
		fatal("The key file name was not specified");
	}
	if (argc > isc_commandline_index + 1) {
		fatal("Extraneous arguments");
	}

	if ((setgoal || setds || setdnskey || setkrrsig || setzrrsig) &&
	    !write_state)
	{
		fatal("Options -g, -d, -k, -r and -z require -s to be set");
	}

	result = dst_lib_init(mctx, engine);
	if (result != ISC_R_SUCCESS) {
		fatal("Could not initialize dst: %s",
		      isc_result_totext(result));
	}

	if (predecessor != NULL) {
		int major, minor;

		if (prepub == -1) {
			prepub = (30 * 86400);
		}

		if (setpub || unsetpub) {
			fatal("-S and -P cannot be used together");
		}
		if (setact || unsetact) {
			fatal("-S and -A cannot be used together");
		}

		result = dst_key_fromnamedfile(predecessor, directory, options,
					       mctx, &prevkey);
		if (result != ISC_R_SUCCESS) {
			fatal("Invalid keyfile %s: %s", filename,
			      isc_result_totext(result));
		}
		if (!dst_key_isprivate(prevkey) && !dst_key_isexternal(prevkey))
		{
			fatal("%s is not a private key", filename);
		}

		name = dst_key_name(prevkey);
		alg = dst_key_alg(prevkey);
		size = dst_key_size(prevkey);
		flags = dst_key_flags(prevkey);

		dst_key_format(prevkey, keystr, sizeof(keystr));
		dst_key_getprivateformat(prevkey, &major, &minor);
		if (major != DST_MAJOR_VERSION || minor < DST_MINOR_VERSION) {
			fatal("Predecessor has incompatible format "
			      "version %d.%d\n\t",
			      major, minor);
		}

		result = dst_key_gettime(prevkey, DST_TIME_ACTIVATE, &prevact);
		if (result != ISC_R_SUCCESS) {
			fatal("Predecessor has no activation date. "
			      "You must set one before\n\t"
			      "generating a successor.");
		}

		result = dst_key_gettime(prevkey, DST_TIME_INACTIVE,
					 &previnact);
		if (result != ISC_R_SUCCESS) {
			fatal("Predecessor has no inactivation date. "
			      "You must set one before\n\t"
			      "generating a successor.");
		}

		pub = previnact - prepub;
		act = previnact;

		if ((previnact - prepub) < now && prepub != 0) {
			fatal("Time until predecessor inactivation is\n\t"
			      "shorter than the prepublication interval.  "
			      "Either change\n\t"
			      "predecessor inactivation date, or use the -i "
			      "option to set\n\t"
			      "a shorter prepublication interval.");
		}

		result = dst_key_gettime(prevkey, DST_TIME_DELETE, &prevdel);
		if (result != ISC_R_SUCCESS) {
			fprintf(stderr,
				"%s: warning: Predecessor has no "
				"removal date;\n\t"
				"it will remain in the zone "
				"indefinitely after rollover.\n",
				program);
		} else if (prevdel < previnact) {
			fprintf(stderr,
				"%s: warning: Predecessor is "
				"scheduled to be deleted\n\t"
				"before it is scheduled to be "
				"inactive.\n",
				program);
		}

		changed = setpub = setact = true;
	} else {
		if (prepub < 0) {
			prepub = 0;
		}

		if (prepub > 0) {
			if (setpub && setact && (act - prepub) < pub) {
				fatal("Activation and publication dates "
				      "are closer together than the\n\t"
				      "prepublication interval.");
			}

			if (setpub && !setact) {
				setact = true;
				act = pub + prepub;
			} else if (setact && !setpub) {
				setpub = true;
				pub = act - prepub;
			}

			if ((act - prepub) < now) {
				fatal("Time until activation is shorter "
				      "than the\n\tprepublication interval.");
			}
		}
	}

	if (directory != NULL) {
		filename = argv[isc_commandline_index];
	} else {
		result = isc_file_splitpath(mctx, argv[isc_commandline_index],
					    &directory, &filename);
		if (result != ISC_R_SUCCESS) {
			fatal("cannot process filename %s: %s",
			      argv[isc_commandline_index],
			      isc_result_totext(result));
		}
	}

	result = dst_key_fromnamedfile(filename, directory, options, mctx,
				       &key);
	if (result != ISC_R_SUCCESS) {
		fatal("Invalid keyfile %s: %s", filename,
		      isc_result_totext(result));
	}

	if (!dst_key_isprivate(key) && !dst_key_isexternal(key)) {
		fatal("%s is not a private key", filename);
	}

	dst_key_format(key, keystr, sizeof(keystr));

	if (predecessor != NULL) {
		if (!dns_name_equal(name, dst_key_name(key))) {
			fatal("Key name mismatch");
		}
		if (alg != dst_key_alg(key)) {
			fatal("Key algorithm mismatch");
		}
		if (size != dst_key_size(key)) {
			fatal("Key size mismatch");
		}
		if (flags != dst_key_flags(key)) {
			fatal("Key flags mismatch");
		}
	}

	prevdel = previnact = 0;
	if ((setdel && setinact && del < inact) ||
	    (dst_key_gettime(key, DST_TIME_INACTIVE, &previnact) ==
		     ISC_R_SUCCESS &&
	     setdel && !setinact && !unsetinact && del < previnact) ||
	    (dst_key_gettime(key, DST_TIME_DELETE, &prevdel) == ISC_R_SUCCESS &&
	     setinact && !setdel && !unsetdel && prevdel < inact) ||
	    (!setdel && !unsetdel && !setinact && !unsetinact && prevdel != 0 &&
	     prevdel < previnact))
	{
		fprintf(stderr,
			"%s: warning: Key is scheduled to "
			"be deleted before it is\n\t"
			"scheduled to be inactive.\n",
			program);
	}

	if (force) {
		set_keyversion(key);
	} else {
		check_keyversion(key, keystr);
	}

	if (verbose > 2) {
		fprintf(stderr, "%s: %s\n", program, keystr);
	}

	/*
	 * Set time values.
	 */
	if (setpub) {
		dst_key_settime(key, DST_TIME_PUBLISH, pub);
	} else if (unsetpub) {
		dst_key_unsettime(key, DST_TIME_PUBLISH);
	}

	if (setact) {
		dst_key_settime(key, DST_TIME_ACTIVATE, act);
	} else if (unsetact) {
		dst_key_unsettime(key, DST_TIME_ACTIVATE);
	}

	if (setrev) {
		if ((dst_key_flags(key) & DNS_KEYFLAG_REVOKE) != 0) {
			fprintf(stderr,
				"%s: warning: Key %s is already "
				"revoked; changing the revocation date "
				"will not affect this.\n",
				program, keystr);
		}
		if ((dst_key_flags(key) & DNS_KEYFLAG_KSK) == 0) {
			fprintf(stderr,
				"%s: warning: Key %s is not flagged as "
				"a KSK, but -R was used.  Revoking a "
				"ZSK is legal, but undefined.\n",
				program, keystr);
		}
		dst_key_settime(key, DST_TIME_REVOKE, rev);
	} else if (unsetrev) {
		if ((dst_key_flags(key) & DNS_KEYFLAG_REVOKE) != 0) {
			fprintf(stderr,
				"%s: warning: Key %s is already "
				"revoked; removing the revocation date "
				"will not affect this.\n",
				program, keystr);
		}
		dst_key_unsettime(key, DST_TIME_REVOKE);
	}

	if (setinact) {
		dst_key_settime(key, DST_TIME_INACTIVE, inact);
	} else if (unsetinact) {
		dst_key_unsettime(key, DST_TIME_INACTIVE);
	}

	if (setdel) {
		dst_key_settime(key, DST_TIME_DELETE, del);
	} else if (unsetdel) {
		dst_key_unsettime(key, DST_TIME_DELETE);
	}

	if (setsyncadd) {
		dst_key_settime(key, DST_TIME_SYNCPUBLISH, syncadd);
	} else if (unsetsyncadd) {
		dst_key_unsettime(key, DST_TIME_SYNCPUBLISH);
	}

	if (setsyncdel) {
		dst_key_settime(key, DST_TIME_SYNCDELETE, syncdel);
	} else if (unsetsyncdel) {
		dst_key_unsettime(key, DST_TIME_SYNCDELETE);
	}

	if (setdsadd) {
		dst_key_settime(key, DST_TIME_DSPUBLISH, dsadd);
	} else if (unsetdsadd) {
		dst_key_unsettime(key, DST_TIME_DSPUBLISH);
	}

	if (setdsdel) {
		dst_key_settime(key, DST_TIME_DSDELETE, dsdel);
	} else if (unsetdsdel) {
		dst_key_unsettime(key, DST_TIME_DSDELETE);
	}

	if (setttl) {
		dst_key_setttl(key, ttl);
	}

	if (predecessor != NULL && prevkey != NULL) {
		dst_key_setnum(prevkey, DST_NUM_SUCCESSOR, dst_key_id(key));
		dst_key_setnum(key, DST_NUM_PREDECESSOR, dst_key_id(prevkey));
	}

	/*
	 * No metadata changes were made but we're forcing an upgrade
	 * to the new format anyway: use "-P now -A now" as the default
	 */
	if (force && !changed) {
		dst_key_settime(key, DST_TIME_PUBLISH, now);
		dst_key_settime(key, DST_TIME_ACTIVATE, now);
		changed = true;
	}

	/*
	 * Make sure the key state goals are written.
	 */
	if (write_state) {
		if (setgoal) {
			if (goal == DST_KEY_STATE_NA) {
				dst_key_unsetstate(key, DST_KEY_GOAL);
			} else {
				dst_key_setstate(key, DST_KEY_GOAL, goal);
			}
			changed = true;
		}
		if (setds) {
			if (ds == DST_KEY_STATE_NA) {
				dst_key_unsetstate(key, DST_KEY_DS);
				dst_key_unsettime(key, DST_TIME_DS);
			} else {
				dst_key_setstate(key, DST_KEY_DS, ds);
				dst_key_settime(key, DST_TIME_DS, dstime);
			}
			changed = true;
		}
		if (setdnskey) {
			if (dnskey == DST_KEY_STATE_NA) {
				dst_key_unsetstate(key, DST_KEY_DNSKEY);
				dst_key_unsettime(key, DST_TIME_DNSKEY);
			} else {
				dst_key_setstate(key, DST_KEY_DNSKEY, dnskey);
				dst_key_settime(key, DST_TIME_DNSKEY,
						dnskeytime);
			}
			changed = true;
		}
		if (setkrrsig) {
			if (krrsig == DST_KEY_STATE_NA) {
				dst_key_unsetstate(key, DST_KEY_KRRSIG);
				dst_key_unsettime(key, DST_TIME_KRRSIG);
			} else {
				dst_key_setstate(key, DST_KEY_KRRSIG, krrsig);
				dst_key_settime(key, DST_TIME_KRRSIG,
						krrsigtime);
			}
			changed = true;
		}
		if (setzrrsig) {
			if (zrrsig == DST_KEY_STATE_NA) {
				dst_key_unsetstate(key, DST_KEY_ZRRSIG);
				dst_key_unsettime(key, DST_TIME_ZRRSIG);
			} else {
				dst_key_setstate(key, DST_KEY_ZRRSIG, zrrsig);
				dst_key_settime(key, DST_TIME_ZRRSIG,
						zrrsigtime);
			}
			changed = true;
		}
	}

	if (!changed && setttl) {
		changed = true;
	}

	/*
	 * Print out time values, if -p was used.
	 */
	if (printcreate) {
		printtime(key, DST_TIME_CREATED, "Created", epoch, stdout);
	}

	if (printpub) {
		printtime(key, DST_TIME_PUBLISH, "Publish", epoch, stdout);
	}

	if (printact) {
		printtime(key, DST_TIME_ACTIVATE, "Activate", epoch, stdout);
	}

	if (printrev) {
		printtime(key, DST_TIME_REVOKE, "Revoke", epoch, stdout);
	}

	if (printinact) {
		printtime(key, DST_TIME_INACTIVE, "Inactive", epoch, stdout);
	}

	if (printdel) {
		printtime(key, DST_TIME_DELETE, "Delete", epoch, stdout);
	}

	if (printsyncadd) {
		printtime(key, DST_TIME_SYNCPUBLISH, "SYNC Publish", epoch,
			  stdout);
	}

	if (printsyncdel) {
		printtime(key, DST_TIME_SYNCDELETE, "SYNC Delete", epoch,
			  stdout);
	}

	if (printdsadd) {
		printtime(key, DST_TIME_DSPUBLISH, "DS Publish", epoch, stdout);
	}

	if (printdsdel) {
		printtime(key, DST_TIME_DSDELETE, "DS Delete", epoch, stdout);
	}

	if (changed) {
		writekey(key, directory, write_state);
		if (predecessor != NULL && prevkey != NULL) {
			writekey(prevkey, directory, write_state);
		}
	}

	if (prevkey != NULL) {
		dst_key_free(&prevkey);
	}
	dst_key_free(&key);
	dst_lib_destroy();
	if (verbose > 10) {
		isc_mem_stats(mctx, stdout);
	}
	cleanup_logging(&log);
	isc_mem_free(mctx, directory);
	isc_mem_destroy(&mctx);

	return (0);
}
