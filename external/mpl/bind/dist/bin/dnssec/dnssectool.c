/*	$NetBSD: dnssectool.c,v 1.8 2023/01/25 21:43:23 christos Exp $	*/

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

/*%
 * DNSSEC Support Routines.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef _WIN32
#include <Winsock2.h>
#endif /* ifdef _WIN32 */

#include <isc/base32.h>
#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/dir.h>
#include <isc/file.h>
#include <isc/heap.h>
#include <isc/list.h>
#include <isc/mem.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/dnssec.h>
#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/name.h>
#include <dns/nsec.h>
#include <dns/nsec3.h>
#include <dns/rdataclass.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/result.h>
#include <dns/secalg.h>
#include <dns/time.h>

#include "dnssectool.h"

#define KEYSTATES_NVALUES 4
static const char *keystates[KEYSTATES_NVALUES] = {
	"hidden",
	"rumoured",
	"omnipresent",
	"unretentive",
};

int verbose = 0;
bool quiet = false;
uint8_t dtype[8];

static fatalcallback_t *fatalcallback = NULL;

void
fatal(const char *format, ...) {
	va_list args;

	fprintf(stderr, "%s: fatal: ", program);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	if (fatalcallback != NULL) {
		(*fatalcallback)();
	}
	exit(1);
}

void
setfatalcallback(fatalcallback_t *callback) {
	fatalcallback = callback;
}

void
check_result(isc_result_t result, const char *message) {
	if (result != ISC_R_SUCCESS) {
		fatal("%s: %s", message, isc_result_totext(result));
	}
}

void
vbprintf(int level, const char *fmt, ...) {
	va_list ap;
	if (level > verbose) {
		return;
	}
	va_start(ap, fmt);
	fprintf(stderr, "%s: ", program);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void
version(const char *name) {
	fprintf(stderr, "%s %s\n", name, VERSION);
	exit(0);
}

void
sig_format(dns_rdata_rrsig_t *sig, char *cp, unsigned int size) {
	char namestr[DNS_NAME_FORMATSIZE];
	char algstr[DNS_NAME_FORMATSIZE];

	dns_name_format(&sig->signer, namestr, sizeof(namestr));
	dns_secalg_format(sig->algorithm, algstr, sizeof(algstr));
	snprintf(cp, size, "%s/%s/%d", namestr, algstr, sig->keyid);
}

void
setup_logging(isc_mem_t *mctx, isc_log_t **logp) {
	isc_logdestination_t destination;
	isc_logconfig_t *logconfig = NULL;
	isc_log_t *log = NULL;
	int level;

	if (verbose < 0) {
		verbose = 0;
	}
	switch (verbose) {
	case 0:
		/*
		 * We want to see warnings about things like out-of-zone
		 * data in the master file even when not verbose.
		 */
		level = ISC_LOG_WARNING;
		break;
	case 1:
		level = ISC_LOG_INFO;
		break;
	default:
		level = ISC_LOG_DEBUG(verbose - 2 + 1);
		break;
	}

	isc_log_create(mctx, &log, &logconfig);
	isc_log_setcontext(log);
	dns_log_init(log);
	dns_log_setcontext(log);
	isc_log_settag(logconfig, program);

	/*
	 * Set up a channel similar to default_stderr except:
	 *  - the logging level is passed in
	 *  - the program name and logging level are printed
	 *  - no time stamp is printed
	 */
	destination.file.stream = stderr;
	destination.file.name = NULL;
	destination.file.versions = ISC_LOG_ROLLNEVER;
	destination.file.maximum_size = 0;
	isc_log_createchannel(logconfig, "stderr", ISC_LOG_TOFILEDESC, level,
			      &destination,
			      ISC_LOG_PRINTTAG | ISC_LOG_PRINTLEVEL);

	RUNTIME_CHECK(isc_log_usechannel(logconfig, "stderr", NULL, NULL) ==
		      ISC_R_SUCCESS);

	*logp = log;
}

void
cleanup_logging(isc_log_t **logp) {
	isc_log_t *log;

	REQUIRE(logp != NULL);

	log = *logp;
	*logp = NULL;

	if (log == NULL) {
		return;
	}

	isc_log_destroy(&log);
	isc_log_setcontext(NULL);
	dns_log_setcontext(NULL);
}

static isc_stdtime_t
time_units(isc_stdtime_t offset, char *suffix, const char *str) {
	switch (suffix[0]) {
	case 'Y':
	case 'y':
		return (offset * (365 * 24 * 3600));
	case 'M':
	case 'm':
		switch (suffix[1]) {
		case 'O':
		case 'o':
			return (offset * (30 * 24 * 3600));
		case 'I':
		case 'i':
			return (offset * 60);
		case '\0':
			fatal("'%s' ambiguous: use 'mi' for minutes "
			      "or 'mo' for months",
			      str);
		default:
			fatal("time value %s is invalid", str);
		}
		UNREACHABLE();
		break;
	case 'W':
	case 'w':
		return (offset * (7 * 24 * 3600));
	case 'D':
	case 'd':
		return (offset * (24 * 3600));
	case 'H':
	case 'h':
		return (offset * 3600);
	case 'S':
	case 's':
	case '\0':
		return (offset);
	default:
		fatal("time value %s is invalid", str);
	}
	UNREACHABLE();
	return (0); /* silence compiler warning */
}

static bool
isnone(const char *str) {
	return ((strcasecmp(str, "none") == 0) ||
		(strcasecmp(str, "never") == 0));
}

dns_ttl_t
strtottl(const char *str) {
	const char *orig = str;
	dns_ttl_t ttl;
	char *endp;

	if (isnone(str)) {
		return ((dns_ttl_t)0);
	}

	ttl = strtol(str, &endp, 0);
	if (ttl == 0 && endp == str) {
		fatal("TTL must be numeric");
	}
	ttl = time_units(ttl, endp, orig);
	return (ttl);
}

dst_key_state_t
strtokeystate(const char *str) {
	if (isnone(str)) {
		return (DST_KEY_STATE_NA);
	}

	for (int i = 0; i < KEYSTATES_NVALUES; i++) {
		if (keystates[i] != NULL && strcasecmp(str, keystates[i]) == 0)
		{
			return ((dst_key_state_t)i);
		}
	}
	fatal("unknown key state %s", str);
}

isc_stdtime_t
strtotime(const char *str, int64_t now, int64_t base, bool *setp) {
	int64_t val, offset;
	isc_result_t result;
	const char *orig = str;
	char *endp;
	size_t n;

	if (isnone(str)) {
		if (setp != NULL) {
			*setp = false;
		}
		return ((isc_stdtime_t)0);
	}

	if (setp != NULL) {
		*setp = true;
	}

	if ((str[0] == '0' || str[0] == '-') && str[1] == '\0') {
		return ((isc_stdtime_t)0);
	}

	/*
	 * We accept times in the following formats:
	 *   now([+-]offset)
	 *   YYYYMMDD([+-]offset)
	 *   YYYYMMDDhhmmss([+-]offset)
	 *   [+-]offset
	 */
	n = strspn(str, "0123456789");
	if ((n == 8u || n == 14u) &&
	    (str[n] == '\0' || str[n] == '-' || str[n] == '+'))
	{
		char timestr[15];

		strlcpy(timestr, str, sizeof(timestr));
		timestr[n] = 0;
		if (n == 8u) {
			strlcat(timestr, "000000", sizeof(timestr));
		}
		result = dns_time64_fromtext(timestr, &val);
		if (result != ISC_R_SUCCESS) {
			fatal("time value %s is invalid: %s", orig,
			      isc_result_totext(result));
		}
		base = val;
		str += n;
	} else if (strncmp(str, "now", 3) == 0) {
		base = now;
		str += 3;
	}

	if (str[0] == '\0') {
		return ((isc_stdtime_t)base);
	} else if (str[0] == '+') {
		offset = strtol(str + 1, &endp, 0);
		offset = time_units((isc_stdtime_t)offset, endp, orig);
		val = base + offset;
	} else if (str[0] == '-') {
		offset = strtol(str + 1, &endp, 0);
		offset = time_units((isc_stdtime_t)offset, endp, orig);
		val = base - offset;
	} else {
		fatal("time value %s is invalid", orig);
	}

	return ((isc_stdtime_t)val);
}

dns_rdataclass_t
strtoclass(const char *str) {
	isc_textregion_t r;
	dns_rdataclass_t rdclass;
	isc_result_t result;

	if (str == NULL) {
		return (dns_rdataclass_in);
	}
	DE_CONST(str, r.base);
	r.length = strlen(str);
	result = dns_rdataclass_fromtext(&rdclass, &r);
	if (result != ISC_R_SUCCESS) {
		fatal("unknown class %s", str);
	}
	return (rdclass);
}

unsigned int
strtodsdigest(const char *str) {
	isc_textregion_t r;
	dns_dsdigest_t alg;
	isc_result_t result;

	DE_CONST(str, r.base);
	r.length = strlen(str);
	result = dns_dsdigest_fromtext(&alg, &r);
	if (result != ISC_R_SUCCESS) {
		fatal("unknown DS algorithm %s", str);
	}
	return (alg);
}

static int
cmp_dtype(const void *ap, const void *bp) {
	int a = *(const uint8_t *)ap;
	int b = *(const uint8_t *)bp;
	return (a - b);
}

void
add_dtype(unsigned int dt) {
	unsigned i, n;

	/* ensure there is space for a zero terminator */
	n = sizeof(dtype) / sizeof(dtype[0]) - 1;
	for (i = 0; i < n; i++) {
		if (dtype[i] == dt) {
			return;
		}
		if (dtype[i] == 0) {
			dtype[i] = dt;
			qsort(dtype, i + 1, 1, cmp_dtype);
			return;
		}
	}
	fatal("too many -a digest type arguments");
}

isc_result_t
try_dir(const char *dirname) {
	isc_result_t result;
	isc_dir_t d;

	isc_dir_init(&d);
	result = isc_dir_open(&d, dirname);
	if (result == ISC_R_SUCCESS) {
		isc_dir_close(&d);
	}
	return (result);
}

/*
 * Check private key version compatibility.
 */
void
check_keyversion(dst_key_t *key, char *keystr) {
	int major, minor;
	dst_key_getprivateformat(key, &major, &minor);
	INSIST(major <= DST_MAJOR_VERSION); /* invalid private key */

	if (major < DST_MAJOR_VERSION || minor < DST_MINOR_VERSION) {
		fatal("Key %s has incompatible format version %d.%d, "
		      "use -f to force upgrade to new version.",
		      keystr, major, minor);
	}
	if (minor > DST_MINOR_VERSION) {
		fatal("Key %s has incompatible format version %d.%d, "
		      "use -f to force downgrade to current version.",
		      keystr, major, minor);
	}
}

void
set_keyversion(dst_key_t *key) {
	int major, minor;
	dst_key_getprivateformat(key, &major, &minor);
	INSIST(major <= DST_MAJOR_VERSION);

	if (major != DST_MAJOR_VERSION || minor != DST_MINOR_VERSION) {
		dst_key_setprivateformat(key, DST_MAJOR_VERSION,
					 DST_MINOR_VERSION);
	}

	/*
	 * If the key is from a version older than 1.3, set
	 * set the creation date
	 */
	if (major < 1 || (major == 1 && minor <= 2)) {
		isc_stdtime_t now;
		isc_stdtime_get(&now);
		dst_key_settime(key, DST_TIME_CREATED, now);
	}
}

bool
key_collision(dst_key_t *dstkey, dns_name_t *name, const char *dir,
	      isc_mem_t *mctx, bool *exact) {
	isc_result_t result;
	bool conflict = false;
	dns_dnsseckeylist_t matchkeys;
	dns_dnsseckey_t *key = NULL;
	uint16_t id, oldid;
	uint32_t rid, roldid;
	dns_secalg_t alg;
	char filename[NAME_MAX];
	isc_buffer_t fileb;
	isc_stdtime_t now;

	if (exact != NULL) {
		*exact = false;
	}

	id = dst_key_id(dstkey);
	rid = dst_key_rid(dstkey);
	alg = dst_key_alg(dstkey);

	/*
	 * For Diffie Hellman just check if there is a direct collision as
	 * they can't be revoked.  Additionally dns_dnssec_findmatchingkeys
	 * only handles DNSKEY which is not used for HMAC.
	 */
	if (alg == DST_ALG_DH) {
		isc_buffer_init(&fileb, filename, sizeof(filename));
		result = dst_key_buildfilename(dstkey, DST_TYPE_PRIVATE, dir,
					       &fileb);
		if (result != ISC_R_SUCCESS) {
			return (true);
		}
		return (isc_file_exists(filename));
	}

	ISC_LIST_INIT(matchkeys);
	isc_stdtime_get(&now);
	result = dns_dnssec_findmatchingkeys(name, dir, now, mctx, &matchkeys);
	if (result == ISC_R_NOTFOUND) {
		return (false);
	}

	while (!ISC_LIST_EMPTY(matchkeys) && !conflict) {
		key = ISC_LIST_HEAD(matchkeys);
		if (dst_key_alg(key->key) != alg) {
			goto next;
		}

		oldid = dst_key_id(key->key);
		roldid = dst_key_rid(key->key);

		if (oldid == rid || roldid == id || id == oldid) {
			conflict = true;
			if (id != oldid) {
				if (verbose > 1) {
					fprintf(stderr,
						"Key ID %d could "
						"collide with %d\n",
						id, oldid);
				}
			} else {
				if (exact != NULL) {
					*exact = true;
				}
				if (verbose > 1) {
					fprintf(stderr, "Key ID %d exists\n",
						id);
				}
			}
		}

	next:
		ISC_LIST_UNLINK(matchkeys, key, link);
		dns_dnsseckey_destroy(mctx, &key);
	}

	/* Finish freeing the list */
	while (!ISC_LIST_EMPTY(matchkeys)) {
		key = ISC_LIST_HEAD(matchkeys);
		ISC_LIST_UNLINK(matchkeys, key, link);
		dns_dnsseckey_destroy(mctx, &key);
	}

	return (conflict);
}

bool
isoptarg(const char *arg, char **argv, void (*usage)(void)) {
	if (!strcasecmp(isc_commandline_argument, arg)) {
		if (argv[isc_commandline_index] == NULL) {
			fprintf(stderr, "%s: missing argument -%c %s\n",
				program, isc_commandline_option,
				isc_commandline_argument);
			usage();
		}
		isc_commandline_argument = argv[isc_commandline_index];
		/* skip to next argument */
		isc_commandline_index++;
		return (true);
	}
	return (false);
}

#ifdef _WIN32
void
InitSockets(void) {
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 0);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		fprintf(stderr, "WSAStartup() failed: %d\n", err);
		exit(1);
	}
}

void
DestroySockets(void) {
	WSACleanup();
}
#endif /* ifdef _WIN32 */
