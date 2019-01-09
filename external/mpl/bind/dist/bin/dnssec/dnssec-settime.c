/*	$NetBSD: dnssec-settime.c,v 1.1.1.2 2019/01/09 16:48:17 christos Exp $	*/

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

#include <config.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/file.h>
#include <isc/hash.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/keyvalues.h>
#include <dns/result.h>
#include <dns/log.h>

#include <dst/dst.h>

#if USE_PKCS11
#include <pk11/result.h>
#endif

#include "dnssectool.h"

const char *program = "dnssec-settime";
int verbose;

static isc_mem_t	*mctx = NULL;

ISC_PLATFORM_NORETURN_PRE static void
usage(void) ISC_PLATFORM_NORETURN_POST;

static void
usage(void) {
	fprintf(stderr, "Usage:\n");
	fprintf(stderr,	"    %s [options] keyfile\n\n", program);
	fprintf(stderr, "Version: %s\n", VERSION);
	fprintf(stderr, "General options:\n");
#if USE_PKCS11
	fprintf(stderr, "    -E engine:          specify PKCS#11 provider "
					"(default: %s)\n", PK11_LIB_LOCATION);
#elif defined(USE_PKCS11)
	fprintf(stderr, "    -E engine:          specify OpenSSL engine "
					   "(default \"pkcs11\")\n");
#else
	fprintf(stderr, "    -E engine:          specify OpenSSL engine\n");
#endif
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
	fprintf(stderr, "    -D sync date/[+-]offset/none: set/unset "
					"CDS and CDNSKEY deletion date\n");
	fprintf(stderr, "    -S <key>: generate a successor to an existing "
				      "key\n");
	fprintf(stderr, "    -i <interval>: prepublication interval for "
					   "successor key "
					   "(default: 30 days)\n");
	fprintf(stderr, "Printing options:\n");
	fprintf(stderr, "    -p C/P/Psync/A/R/I/D/Dsync/all: print a "
					"particular time value or values\n");
	fprintf(stderr, "    -u:                 print times in unix epoch "
						"format\n");
	fprintf(stderr, "Output:\n");
	fprintf(stderr, "     K<name>+<alg>+<new id>.key, "
			     "K<name>+<alg>+<new id>.private\n");

	exit (-1);
}

static void
printtime(dst_key_t *key, int type, const char *tag, bool epoch,
	  FILE *stream)
{
	isc_result_t result;
	const char *output = NULL;
	isc_stdtime_t when;

	if (tag != NULL)
		fprintf(stream, "%s: ", tag);

	result = dst_key_gettime(key, type, &when);
	if (result == ISC_R_NOTFOUND) {
		fprintf(stream, "UNSET\n");
	} else if (epoch) {
		fprintf(stream, "%d\n", (int) when);
	} else {
		time_t timet = when;
		output = ctime(&timet);
		fprintf(stream, "%s", output);
	}
}

int
main(int argc, char **argv) {
	isc_result_t	result;
	const char	*engine = NULL;
	const char 	*filename = NULL;
	char		*directory = NULL;
	char		newname[1024];
	char		keystr[DST_KEY_FORMATSIZE];
	char		*endp, *p;
	int		ch;
	const char	*predecessor = NULL;
	dst_key_t	*prevkey = NULL;
	dst_key_t	*key = NULL;
	isc_buffer_t	buf;
	dns_name_t	*name = NULL;
	dns_secalg_t 	alg = 0;
	unsigned int 	size = 0;
	uint16_t	flags = 0;
	int		prepub = -1;
	dns_ttl_t	ttl = 0;
	isc_stdtime_t	now;
	isc_stdtime_t	pub = 0, act = 0, rev = 0, inact = 0, del = 0;
	isc_stdtime_t	prevact = 0, previnact = 0, prevdel = 0;
	bool	setpub = false, setact = false;
	bool	setrev = false, setinact = false;
	bool	setdel = false, setttl = false;
	bool	unsetpub = false, unsetact = false;
	bool	unsetrev = false, unsetinact = false;
	bool	unsetdel = false;
	bool	printcreate = false, printpub = false;
	bool	printact = false,  printrev = false;
	bool	printinact = false, printdel = false;
	bool	force = false;
	bool   epoch = false;
	bool   changed = false;
	isc_log_t       *log = NULL;
	isc_stdtime_t	syncadd = 0, syncdel = 0;
	bool	unsetsyncadd = false, setsyncadd = false;
	bool	unsetsyncdel = false, setsyncdel = false;
	bool	printsyncadd = false, printsyncdel = false;

	if (argc == 1)
		usage();

	result = isc_mem_create(0, 0, &mctx);
	if (result != ISC_R_SUCCESS)
		fatal("Out of memory");

	setup_logging(mctx, &log);

#if USE_PKCS11
	pk11_result_register();
#endif
	dns_result_register();

	isc_commandline_errprint = false;

	isc_stdtime_get(&now);

#define CMDLINE_FLAGS "A:D:E:fhI:i:K:L:P:p:R:S:uv:V"
	while ((ch = isc_commandline_parse(argc, argv, CMDLINE_FLAGS)) != -1) {
		switch (ch) {
		case 'E':
			engine = isc_commandline_argument;
			break;
		case 'f':
			force = true;
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
				break;
			}

			do {
				switch (*p++) {
				case 'C':
					printcreate = true;
					break;
				case 'P':
					if (!strncmp(p, "sync", 4)) {
						p += 4;
						printsyncadd = true;
						break;
					}
					printpub = true;
					break;
				case 'A':
					printact = true;
					break;
				case 'R':
					printrev = true;
					break;
				case 'I':
					printinact = true;
					break;
				case 'D':
					if (!strncmp(p, "sync", 4)) {
						p += 4;
						printsyncdel = true;
						break;
					}
					printdel = true;
					break;
				case ' ':
					break;
				default:
					usage();
					break;
				}
			} while (*p != '\0');
			break;
		case 'u':
			epoch = true;
			break;
		case 'K':
			/*
			 * We don't have to copy it here, but do it to
			 * simplify cleanup later
			 */
			directory = isc_mem_strdup(mctx,
						   isc_commandline_argument);
			if (directory == NULL) {
				fatal("Failed to allocate memory for "
				      "directory");
			}
			break;
		case 'L':
			ttl = strtottl(isc_commandline_argument);
			setttl = true;
			break;
		case 'v':
			verbose = strtol(isc_commandline_argument, &endp, 0);
			if (*endp != '\0')
				fatal("-v must be followed by a number");
			break;
		case 'P':
			/* -Psync ? */
			if (isoptarg("sync", argv, usage)) {
				if (unsetsyncadd || setsyncadd)
					fatal("-P sync specified more than "
					      "once");

				changed = true;
				syncadd = strtotime(isc_commandline_argument,
						   now, now, &setsyncadd);
				unsetsyncadd = !setsyncadd;
				break;
			}
			(void)isoptarg("dnskey", argv, usage);
			if (setpub || unsetpub)
				fatal("-P specified more than once");

			changed = true;
			pub = strtotime(isc_commandline_argument,
					now, now, &setpub);
			unsetpub = !setpub;
			break;
		case 'A':
			if (setact || unsetact)
				fatal("-A specified more than once");

			changed = true;
			act = strtotime(isc_commandline_argument,
					now, now, &setact);
			unsetact = !setact;
			break;
		case 'R':
			if (setrev || unsetrev)
				fatal("-R specified more than once");

			changed = true;
			rev = strtotime(isc_commandline_argument,
					now, now, &setrev);
			unsetrev = !setrev;
			break;
		case 'I':
			if (setinact || unsetinact)
				fatal("-I specified more than once");

			changed = true;
			inact = strtotime(isc_commandline_argument,
					now, now, &setinact);
			unsetinact = !setinact;
			break;
		case 'D':
			/* -Dsync ? */
			if (isoptarg("sync", argv, usage)) {
				if (unsetsyncdel || setsyncdel)
					fatal("-D sync specified more than "
					      "once");

				changed = true;
				syncdel = strtotime(isc_commandline_argument,
						   now, now, &setsyncdel);
				unsetsyncdel = !setsyncdel;
				break;
			}
			/* -Ddnskey ? */
			(void)isoptarg("dnskey", argv, usage);
			if (setdel || unsetdel)
				fatal("-D specified more than once");

			changed = true;
			del = strtotime(isc_commandline_argument,
					now, now, &setdel);
			unsetdel = !setdel;
			break;
		case 'S':
			predecessor = isc_commandline_argument;
			break;
		case 'i':
			prepub = strtottl(isc_commandline_argument);
			break;
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

	if (argc < isc_commandline_index + 1 ||
	    argv[isc_commandline_index] == NULL)
		fatal("The key file name was not specified");
	if (argc > isc_commandline_index + 1)
		fatal("Extraneous arguments");

	result = dst_lib_init(mctx, engine);
	if (result != ISC_R_SUCCESS)
		fatal("Could not initialize dst: %s",
		      isc_result_totext(result));

	if (predecessor != NULL) {
		int major, minor;

		if (prepub == -1)
			prepub = (30 * 86400);

		if (setpub || unsetpub)
			fatal("-S and -P cannot be used together");
		if (setact || unsetact)
			fatal("-S and -A cannot be used together");

		result = dst_key_fromnamedfile(predecessor, directory,
					       DST_TYPE_PUBLIC |
					       DST_TYPE_PRIVATE,
					       mctx, &prevkey);
		if (result != ISC_R_SUCCESS)
			fatal("Invalid keyfile %s: %s",
			      filename, isc_result_totext(result));
		if (!dst_key_isprivate(prevkey) && !dst_key_isexternal(prevkey))
			fatal("%s is not a private key", filename);

		name = dst_key_name(prevkey);
		alg = dst_key_alg(prevkey);
		size = dst_key_size(prevkey);
		flags = dst_key_flags(prevkey);

		dst_key_format(prevkey, keystr, sizeof(keystr));
		dst_key_getprivateformat(prevkey, &major, &minor);
		if (major != DST_MAJOR_VERSION || minor < DST_MINOR_VERSION)
			fatal("Predecessor has incompatible format "
			      "version %d.%d\n\t", major, minor);

		result = dst_key_gettime(prevkey, DST_TIME_ACTIVATE, &prevact);
		if (result != ISC_R_SUCCESS)
			fatal("Predecessor has no activation date. "
			      "You must set one before\n\t"
			      "generating a successor.");

		result = dst_key_gettime(prevkey, DST_TIME_INACTIVE,
					 &previnact);
		if (result != ISC_R_SUCCESS)
			fatal("Predecessor has no inactivation date. "
			      "You must set one before\n\t"
			      "generating a successor.");

		pub = previnact - prepub;
		act = previnact;

		if ((previnact - prepub) < now && prepub != 0)
			fatal("Time until predecessor inactivation is\n\t"
			      "shorter than the prepublication interval.  "
			      "Either change\n\t"
			      "predecessor inactivation date, or use the -i "
			      "option to set\n\t"
			      "a shorter prepublication interval.");

		result = dst_key_gettime(prevkey, DST_TIME_DELETE, &prevdel);
		if (result != ISC_R_SUCCESS)
			fprintf(stderr, "%s: warning: Predecessor has no "
					"removal date;\n\t"
					"it will remain in the zone "
					"indefinitely after rollover.\n",
					program);
		else if (prevdel < previnact)
			fprintf(stderr, "%s: warning: Predecessor is "
					"scheduled to be deleted\n\t"
					"before it is scheduled to be "
					"inactive.\n", program);

		changed = setpub = setact = true;
	} else {
		if (prepub < 0)
			prepub = 0;

		if (prepub > 0) {
			if (setpub && setact && (act - prepub) < pub)
				fatal("Activation and publication dates "
				      "are closer together than the\n\t"
				      "prepublication interval.");

			if (setpub && !setact) {
				setact = true;
				act = pub + prepub;
			} else if (setact && !setpub) {
				setpub = true;
				pub = act - prepub;
			}

			if ((act - prepub) < now)
				fatal("Time until activation is shorter "
				      "than the\n\tprepublication interval.");
		}
	}

	if (directory != NULL) {
		filename = argv[isc_commandline_index];
	} else {
		result = isc_file_splitpath(mctx, argv[isc_commandline_index],
					    &directory, &filename);
		if (result != ISC_R_SUCCESS)
			fatal("cannot process filename %s: %s",
			      argv[isc_commandline_index],
			      isc_result_totext(result));
	}

	result = dst_key_fromnamedfile(filename, directory,
				       DST_TYPE_PUBLIC | DST_TYPE_PRIVATE,
				       mctx, &key);
	if (result != ISC_R_SUCCESS)
		fatal("Invalid keyfile %s: %s",
		      filename, isc_result_totext(result));

	if (!dst_key_isprivate(key) && !dst_key_isexternal(key))
		fatal("%s is not a private key", filename);

	dst_key_format(key, keystr, sizeof(keystr));

	if (predecessor != NULL) {
		if (!dns_name_equal(name, dst_key_name(key)))
			fatal("Key name mismatch");
		if (alg != dst_key_alg(key))
			fatal("Key algorithm mismatch");
		if (size != dst_key_size(key))
			fatal("Key size mismatch");
		if (flags != dst_key_flags(key))
			fatal("Key flags mismatch");
	}

	prevdel = previnact = 0;
	if ((setdel && setinact && del < inact) ||
	    (dst_key_gettime(key, DST_TIME_INACTIVE,
			     &previnact) == ISC_R_SUCCESS &&
	     setdel && !setinact && !unsetinact && del < previnact) ||
	    (dst_key_gettime(key, DST_TIME_DELETE,
			     &prevdel) == ISC_R_SUCCESS &&
	     setinact && !setdel && !unsetdel && prevdel < inact) ||
	    (!setdel && !unsetdel && !setinact && !unsetinact &&
	     prevdel != 0 && prevdel < previnact))
		fprintf(stderr, "%s: warning: Key is scheduled to "
				"be deleted before it is\n\t"
				"scheduled to be inactive.\n",
			program);

	if (force)
		set_keyversion(key);
	else
		check_keyversion(key, keystr);

	if (verbose > 2)
		fprintf(stderr, "%s: %s\n", program, keystr);

	/*
	 * Set time values.
	 */
	if (setpub)
		dst_key_settime(key, DST_TIME_PUBLISH, pub);
	else if (unsetpub)
		dst_key_unsettime(key, DST_TIME_PUBLISH);

	if (setact)
		dst_key_settime(key, DST_TIME_ACTIVATE, act);
	else if (unsetact)
		dst_key_unsettime(key, DST_TIME_ACTIVATE);

	if (setrev) {
		if ((dst_key_flags(key) & DNS_KEYFLAG_REVOKE) != 0)
			fprintf(stderr, "%s: warning: Key %s is already "
					"revoked; changing the revocation date "
					"will not affect this.\n",
					program, keystr);
		if ((dst_key_flags(key) & DNS_KEYFLAG_KSK) == 0)
			fprintf(stderr, "%s: warning: Key %s is not flagged as "
					"a KSK, but -R was used.  Revoking a "
					"ZSK is legal, but undefined.\n",
					program, keystr);
		dst_key_settime(key, DST_TIME_REVOKE, rev);
	} else if (unsetrev) {
		if ((dst_key_flags(key) & DNS_KEYFLAG_REVOKE) != 0)
			fprintf(stderr, "%s: warning: Key %s is already "
					"revoked; removing the revocation date "
					"will not affect this.\n",
					program, keystr);
		dst_key_unsettime(key, DST_TIME_REVOKE);
	}

	if (setinact)
		dst_key_settime(key, DST_TIME_INACTIVE, inact);
	else if (unsetinact)
		dst_key_unsettime(key, DST_TIME_INACTIVE);

	if (setdel)
		dst_key_settime(key, DST_TIME_DELETE, del);
	else if (unsetdel)
		dst_key_unsettime(key, DST_TIME_DELETE);

	if (setsyncadd)
		dst_key_settime(key, DST_TIME_SYNCPUBLISH, syncadd);
	else if (unsetsyncadd)
		dst_key_unsettime(key, DST_TIME_SYNCPUBLISH);

	if (setsyncdel)
		dst_key_settime(key, DST_TIME_SYNCDELETE, syncdel);
	else if (unsetsyncdel)
		dst_key_unsettime(key, DST_TIME_SYNCDELETE);

	if (setttl)
		dst_key_setttl(key, ttl);

	/*
	 * No metadata changes were made but we're forcing an upgrade
	 * to the new format anyway: use "-P now -A now" as the default
	 */
	if (force && !changed) {
		dst_key_settime(key, DST_TIME_PUBLISH, now);
		dst_key_settime(key, DST_TIME_ACTIVATE, now);
		changed = true;
	}

	if (!changed && setttl)
		changed = true;

	/*
	 * Print out time values, if -p was used.
	 */
	if (printcreate)
		printtime(key, DST_TIME_CREATED, "Created", epoch, stdout);

	if (printpub)
		printtime(key, DST_TIME_PUBLISH, "Publish", epoch, stdout);

	if (printact)
		printtime(key, DST_TIME_ACTIVATE, "Activate", epoch, stdout);

	if (printrev)
		printtime(key, DST_TIME_REVOKE, "Revoke", epoch, stdout);

	if (printinact)
		printtime(key, DST_TIME_INACTIVE, "Inactive", epoch, stdout);

	if (printdel)
		printtime(key, DST_TIME_DELETE, "Delete", epoch, stdout);

	if (printsyncadd)
		printtime(key, DST_TIME_SYNCPUBLISH, "SYNC Publish",
			  epoch, stdout);

	if (printsyncdel)
		printtime(key, DST_TIME_SYNCDELETE, "SYNC Delete",
			  epoch, stdout);

	if (changed) {
		isc_buffer_init(&buf, newname, sizeof(newname));
		result = dst_key_buildfilename(key, DST_TYPE_PUBLIC, directory,
					       &buf);
		if (result != ISC_R_SUCCESS) {
			fatal("Failed to build public key filename: %s",
			      isc_result_totext(result));
		}

		result = dst_key_tofile(key, DST_TYPE_PUBLIC|DST_TYPE_PRIVATE,
					directory);
		if (result != ISC_R_SUCCESS) {
			dst_key_format(key, keystr, sizeof(keystr));
			fatal("Failed to write key %s: %s", keystr,
			      isc_result_totext(result));
		}

		printf("%s\n", newname);

		isc_buffer_clear(&buf);
		result = dst_key_buildfilename(key, DST_TYPE_PRIVATE, directory,
					       &buf);
		if (result != ISC_R_SUCCESS) {
			fatal("Failed to build private key filename: %s",
			      isc_result_totext(result));
		}
		printf("%s\n", newname);
	}

	if (prevkey != NULL)
		dst_key_free(&prevkey);
	dst_key_free(&key);
	dst_lib_destroy();
	if (verbose > 10)
		isc_mem_stats(mctx, stdout);
	cleanup_logging(&log);
	isc_mem_free(mctx, directory);
	isc_mem_destroy(&mctx);

	return (0);
}
