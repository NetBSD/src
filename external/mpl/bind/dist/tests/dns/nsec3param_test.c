/*	$NetBSD: nsec3param_test.c,v 1.2.2.2 2024/02/25 15:47:40 martin Exp $	*/

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

#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/hex.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/nsec3.h>

#include "zone_p.h"

#include <tests/dns.h>

#define HASH	1
#define FLAGS	0
#define ITER	5
#define SALTLEN 4
#define SALT	"FEDCBA98"

/*%
 * Structures containing parameters for nsec3param_salttotext_test().
 */
typedef struct {
	dns_hash_t hash;
	unsigned char flags;
	dns_iterations_t iterations;
	unsigned char salt_length;
	const char *salt;
} nsec3param_rdata_test_params_t;

typedef struct {
	nsec3param_rdata_test_params_t lookup;
	nsec3param_rdata_test_params_t expect;
	bool resalt;
	isc_result_t expected_result;
} nsec3param_change_test_params_t;

static void
decode_salt(const char *string, unsigned char *salt, size_t saltlen) {
	isc_buffer_t buf;
	isc_result_t result;

	isc_buffer_init(&buf, salt, saltlen);
	result = isc_hex_decodestring(string, &buf);
	assert_int_equal(result, ISC_R_SUCCESS);
}

static void
copy_params(nsec3param_rdata_test_params_t from, dns_rdata_nsec3param_t *to,
	    unsigned char *saltbuf, size_t saltlen) {
	to->hash = from.hash;
	to->flags = from.flags;
	to->iterations = from.iterations;
	to->salt_length = from.salt_length;
	if (from.salt == NULL) {
		to->salt = NULL;
	} else if (strcmp(from.salt, "-") == 0) {
		DE_CONST("-", to->salt);
	} else {
		decode_salt(from.salt, saltbuf, saltlen);
		to->salt = saltbuf;
	}
}

static nsec3param_rdata_test_params_t
rdata_fromparams(uint8_t hash, uint8_t flags, uint16_t iter, uint8_t saltlen,
		 const char *salt) {
	nsec3param_rdata_test_params_t nsec3param;
	nsec3param.hash = hash;
	nsec3param.flags = flags;
	nsec3param.iterations = iter;
	nsec3param.salt_length = saltlen;
	nsec3param.salt = salt;
	return (nsec3param);
}

/*%
 * Check whether zone_lookup_nsec3param() finds the correct NSEC3PARAM
 * and sets the correct parameters to use in dns_zone_setnsec3param().
 */
static void
nsec3param_change_test(const nsec3param_change_test_params_t *test) {
	dns_zone_t *zone = NULL;
	dns_rdata_nsec3param_t param, lookup, expect;
	isc_result_t result;
	unsigned char lookupsalt[255];
	unsigned char expectsalt[255];
	unsigned char saltbuf[255];

	/*
	 * Prepare a zone along with its signing keys.
	 */
	result = dns_test_makezone("nsec3", &zone, NULL, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_zone_setfile(
		zone, TESTS_DIR "/testdata/nsec3param/nsec3.db.signed",
		dns_masterformat_text, &dns_master_style_default);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_zone_load(zone, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Copy parameters.
	 */
	copy_params(test->lookup, &lookup, lookupsalt, sizeof(lookupsalt));
	copy_params(test->expect, &expect, expectsalt, sizeof(expectsalt));

	/*
	 * Test dns__zone_lookup_nsec3param().
	 */
	result = dns__zone_lookup_nsec3param(zone, &lookup, &param, saltbuf,
					     test->resalt);
	assert_int_equal(result, test->expected_result);
	assert_int_equal(param.hash, expect.hash);
	assert_int_equal(param.flags, expect.flags);
	assert_int_equal(param.iterations, expect.iterations);
	assert_int_equal(param.salt_length, expect.salt_length);
	assert_non_null(param.salt);
	if (expect.salt != NULL) {
		int ret = memcmp(param.salt, expect.salt, expect.salt_length);
		assert_true(ret == 0);
	} else {
		/*
		 * We don't know what the new salt is, but we can compare it
		 * to the previous salt and test that it has changed.
		 */
		unsigned char salt[SALTLEN];
		int ret;
		decode_salt(SALT, salt, SALTLEN);
		ret = memcmp(param.salt, salt, SALTLEN);
		assert_false(ret == 0);
	}

	/*
	 * Detach.
	 */
	dns_zone_detach(&zone);
}

ISC_RUN_TEST_IMPL(nsec3param_change) {
	size_t i;

	/*
	 * Define tests.
	 */
	const nsec3param_change_test_params_t tests[] = {
		/*
		 * 1. Change nothing (don't care about salt).
		 * This should return ISC_R_SUCCESS because we are already
		 * using these NSEC3 parameters.
		 */
		{ rdata_fromparams(HASH, FLAGS, ITER, SALTLEN, NULL),
		  rdata_fromparams(HASH, FLAGS, ITER, SALTLEN, SALT), false,
		  ISC_R_SUCCESS },
		/*
		 * 2. Change nothing, but force a resalt.
		 * This should change the salt. Set 'expect.salt' to NULL to
		 * test a new salt has been generated.
		 */
		{ rdata_fromparams(HASH, FLAGS, ITER, SALTLEN, NULL),
		  rdata_fromparams(HASH, FLAGS, ITER, SALTLEN, NULL), true,
		  DNS_R_NSEC3RESALT },
		/*
		 * 3. Change iterations.
		 * The NSEC3 paarameters are not found, and there is no
		 * need to resalt because an explicit salt has been set,
		 * and resalt is not enforced.
		 */
		{ rdata_fromparams(HASH, FLAGS, 10, SALTLEN, SALT),
		  rdata_fromparams(HASH, FLAGS, 10, SALTLEN, SALT), false,
		  ISC_R_NOTFOUND },
		/*
		 * 4. Change iterations, don't care about the salt.
		 * We don't care about the salt. Since we need to change the
		 * NSEC3 parameters, we will also resalt.
		 */
		{ rdata_fromparams(HASH, FLAGS, 10, SALTLEN, NULL),
		  rdata_fromparams(HASH, FLAGS, 10, SALTLEN, NULL), false,
		  DNS_R_NSEC3RESALT },
		/*
		 * 5. Change salt length.
		 * Changing salt length means we need to resalt.
		 */
		{ rdata_fromparams(HASH, FLAGS, ITER, 16, NULL),
		  rdata_fromparams(HASH, FLAGS, ITER, 16, NULL), false,
		  DNS_R_NSEC3RESALT },
		/*
		 * 6. Set explicit salt.
		 * A different salt, so the NSEC3 parameters are not found.
		 * No need to resalt because an explicit salt is available.
		 */
		{ rdata_fromparams(HASH, FLAGS, ITER, 4, "12345678"),
		  rdata_fromparams(HASH, FLAGS, ITER, 4, "12345678"), false,
		  ISC_R_NOTFOUND },
		/*
		 * 7. Same salt.
		 * Nothing changed, so expect ISC_R_SUCCESS as a result.
		 */
		{ rdata_fromparams(HASH, FLAGS, ITER, SALTLEN, SALT),
		  rdata_fromparams(HASH, FLAGS, ITER, SALTLEN, SALT), false,
		  ISC_R_SUCCESS },
		/*
		 * 8. Same salt, and force resalt.
		 * Nothing changed, but a resalt is enforced.
		 */
		{ rdata_fromparams(HASH, FLAGS, ITER, SALTLEN, SALT),
		  rdata_fromparams(HASH, FLAGS, ITER, SALTLEN, NULL), true,
		  DNS_R_NSEC3RESALT },
		/*
		 * 9. No salt.
		 * Change parameters to use no salt. These parameters are
		 * not found, and no new salt needs to be generated.
		 */
		{ rdata_fromparams(HASH, FLAGS, ITER, 0, NULL),
		  rdata_fromparams(HASH, FLAGS, ITER, 0, "-"), true,
		  ISC_R_NOTFOUND },
		/*
		 * 10. No salt, explicit.
		 * Same as above, but set no salt explicitly.
		 */
		{ rdata_fromparams(HASH, FLAGS, ITER, 0, "-"),
		  rdata_fromparams(HASH, FLAGS, ITER, 0, "-"), true,
		  ISC_R_NOTFOUND },
	};

	UNUSED(state);

	/*
	 * Run tests.
	 */
	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		nsec3param_change_test(&tests[i]);
	}
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(nsec3param_change)
ISC_TEST_LIST_END

ISC_TEST_MAIN
