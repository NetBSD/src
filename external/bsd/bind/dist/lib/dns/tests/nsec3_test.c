/*	$NetBSD: nsec3_test.c,v 1.1.1.6.4.1 2018/04/16 01:57:57 pgoyette Exp $	*/

/*
 * Copyright (C) 2012, 2014-2017  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id */

/*! \file */

#include <config.h>

#include <atf-c.h>

#include <unistd.h>

#include <dns/db.h>
#include <dns/nsec3.h>

#include "dnstest.h"

#if defined(OPENSSL) || defined(PKCS11CRYPTO)
/*
 * Helper functions
 */

static void
iteration_test(const char *file, unsigned int expected) {
	isc_result_t result;
	dns_db_t *db = NULL;
	unsigned int iterations;

	result = dns_test_loaddb(&db, dns_dbtype_zone, "test", file);
	ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS, "%s", file);

	result = dns_nsec3_maxiterations(db, NULL, mctx, &iterations);
	ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS, "%s", file);

	ATF_CHECK_EQ_MSG(iterations, expected, "%s", file);

	dns_db_detach(&db);
}

/*%
 * Structure containing parameters for nsec3param_salttotext_test().
 */
typedef struct {
	const char *nsec3param_text;	/* NSEC3PARAM RDATA in text form */
	const char *expected_salt;	/* string expected in target buffer */
} nsec3param_salttotext_test_params_t;

/*%
 * Check whether dns_nsec3param_salttotext() handles supplied text form
 * NSEC3PARAM RDATA correctly: test whether the result of calling the former is
 * as expected and whether it properly checks available buffer space.
 *
 * Assumes supplied text form NSEC3PARAM RDATA is valid as testing handling of
 * invalid NSEC3PARAM RDATA is out of scope of this unit test.
 */
static void
nsec3param_salttotext_test(const nsec3param_salttotext_test_params_t *params) {
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_nsec3param_t nsec3param;
	unsigned char buf[1024];
	isc_result_t result;
	char salt[64];
	size_t length;

	/*
	 * Prepare a dns_rdata_nsec3param_t structure for testing.
	 */
	result = dns_test_rdata_fromstring(&rdata, dns_rdataclass_in,
					   dns_rdatatype_nsec3param, buf,
					   sizeof(buf),
					   params->nsec3param_text);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = dns_rdata_tostruct(&rdata, &nsec3param, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Check typical use.
	 */
	result = dns_nsec3param_salttotext(&nsec3param, salt, sizeof(salt));
	ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS,
			 "\"%s\": expected success, got %s\n",
			 params->nsec3param_text, isc_result_totext(result));
	ATF_CHECK_EQ_MSG(strcmp(salt, params->expected_salt), 0,
			 "\"%s\": expected salt \"%s\", got \"%s\"",
			 params->nsec3param_text, params->expected_salt, salt);

	/*
	 * Ensure available space in the buffer is checked before the salt is
	 * printed to it and that the amount of space checked for includes the
	 * terminating NULL byte.
	 */
	length = strlen(params->expected_salt);
	ATF_REQUIRE(length < sizeof(salt) - 1);	/* prevent buffer overwrite */
	ATF_REQUIRE(length > 0U);		/* prevent length underflow */

	result = dns_nsec3param_salttotext(&nsec3param, salt, length - 1);
	ATF_CHECK_EQ_MSG(result, ISC_R_NOSPACE,
			 "\"%s\": expected a %lu-byte target buffer to be "
			 "rejected, got %s\n",
			 params->nsec3param_text, (unsigned long)(length - 1),
			 isc_result_totext(result));
	result = dns_nsec3param_salttotext(&nsec3param, salt, length);
	ATF_CHECK_EQ_MSG(result, ISC_R_NOSPACE,
			 "\"%s\": expected a %lu-byte target buffer to be "
			 "rejected, got %s\n",
			 params->nsec3param_text, (unsigned long)length,
			 isc_result_totext(result));
	result = dns_nsec3param_salttotext(&nsec3param, salt, length + 1);
	ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS,
			 "\"%s\": expected a %lu-byte target buffer to be "
			 "accepted, got %s\n",
			 params->nsec3param_text, (unsigned long)(length + 1),
			 isc_result_totext(result));
}

/*
 * Individual unit tests
 */

ATF_TC(max_iterations);
ATF_TC_HEAD(max_iterations, tc) {
	atf_tc_set_md_var(tc, "descr", "check that appropriate max iterations "
			  " is returned for different key size mixes");
}
ATF_TC_BODY(max_iterations, tc) {
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	iteration_test("testdata/nsec3/1024.db", 150);
	iteration_test("testdata/nsec3/2048.db", 500);
	iteration_test("testdata/nsec3/4096.db", 2500);
	iteration_test("testdata/nsec3/min-1024.db", 150);
	iteration_test("testdata/nsec3/min-2048.db", 500);

	dns_test_end();
}

ATF_TC(nsec3param_salttotext);
ATF_TC_HEAD(nsec3param_salttotext, tc) {
	atf_tc_set_md_var(tc, "descr", "check dns_nsec3param_salttotext()");
}
ATF_TC_BODY(nsec3param_salttotext, tc) {
	isc_result_t result;
	size_t i;

	const nsec3param_salttotext_test_params_t tests[] = {
		/*
		 * Tests with non-empty salts.
		 */
		{ "0 0 10 0123456789abcdef", "0123456789ABCDEF" },
		{ "0 1 11 0123456789abcdef", "0123456789ABCDEF" },
		{ "1 0 12 42", "42" },
		{ "1 1 13 42", "42" },
		/*
		 * Test with empty salt.
		 */
		{ "0 0 0 -", "-" },
	};

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		nsec3param_salttotext_test(&tests[i]);
	}

	dns_test_end();
}
#else
ATF_TC(untested);
ATF_TC_HEAD(untested, tc) {
	atf_tc_set_md_var(tc, "descr", "skipping nsec3 test");
}
ATF_TC_BODY(untested, tc) {
	UNUSED(tc);
	atf_tc_skip("DNSSEC not available");
}
#endif

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
#if defined(OPENSSL) || defined(PKCS11CRYPTO)
	ATF_TP_ADD_TC(tp, max_iterations);
	ATF_TP_ADD_TC(tp, nsec3param_salttotext);
#else
	ATF_TP_ADD_TC(tp, untested);
#endif

	return (atf_no_error());
}

