/*	$NetBSD: master_test.c,v 1.1.1.1 2011/09/11 17:19:02 christos Exp $	*/

/*
 * Copyright (C) 2011  Internet Systems Consortium, Inc. ("ISC")
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

/* Id: master_test.c,v 1.4 2011-07-06 01:36:32 each Exp */

/*! \file */

#include <config.h>

#include <atf-c.h>

#include <unistd.h>

#include <dns/cache.h>
#include <dns/callbacks.h>
#include <dns/master.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>

#include "dnstest.h"

/*
 * Helper functions
 */

#define	BUFLEN		255
#define	BIGBUFLEN	(64 * 1024)
#define TEST_ORIGIN	"test"

static isc_result_t
add_callback(void *arg, dns_name_t *owner, dns_rdataset_t *dataset);

static isc_result_t
add_callback(void *arg, dns_name_t *owner, dns_rdataset_t *dataset) {
	char buf[BIGBUFLEN];
	isc_buffer_t target;
	isc_result_t result;

	UNUSED(arg);

	isc_buffer_init(&target, buf, BIGBUFLEN);
	result = dns_rdataset_totext(dataset, owner, ISC_FALSE, ISC_FALSE,
				     &target);
	return(result);
}

static int
test_master(const char *testfile) {
	isc_result_t		result;
	int			len;
	char			origin[sizeof(TEST_ORIGIN)];
	dns_name_t		dns_origin;
	isc_buffer_t		source;
	isc_buffer_t		target;
	unsigned char		name_buf[BUFLEN];
	dns_rdatacallbacks_t	callbacks;

	strcpy(origin, TEST_ORIGIN);
	len = strlen(origin);
	isc_buffer_init(&source, origin, len);
	isc_buffer_add(&source, len);
	isc_buffer_setactive(&source, len);
	isc_buffer_init(&target, name_buf, BUFLEN);
	dns_name_init(&dns_origin, NULL);

	result = dns_name_fromtext(&dns_origin, &source, dns_rootname,
				   0, &target);
	if (result != ISC_R_SUCCESS)
		return(result);

	dns_rdatacallbacks_init_stdio(&callbacks);
	callbacks.add = add_callback;

	/*
	 * atf-run changes us to a /tmp directory, so tests
	 * that access test data files must first chdir to the proper
	 * location.
	 */
	if (chdir(TESTS) == -1)
		return (ISC_R_FAILURE);

	result = dns_master_loadfile(testfile, &dns_origin, &dns_origin,
				     dns_rdataclass_in, ISC_TRUE,
				     &callbacks, mctx);
	return (result);
}

/*
 * Individual unit tests
 */

/* Successful load test */
ATF_TC(master_load);
ATF_TC_HEAD(master_load, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_master_loadfile() loads a "
				       "valid master file and returns success");
}
ATF_TC_BODY(master_load, tc) {
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = test_master("testdata/master/master1.data");
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	dns_test_end();
}


/* Unepxected end of file test */
ATF_TC(master_unexpected);
ATF_TC_HEAD(master_unexpected, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_master_loadfile() returns "
				       "DNS_R_UNEXPECTED when file ends "
				       "too soon");
}
ATF_TC_BODY(master_unexpected, tc) {
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = test_master("testdata/master/master2.data");
	ATF_REQUIRE_EQ(result, ISC_R_UNEXPECTEDEND);

	dns_test_end();
}


/* No owner test */
ATF_TC(master_noowner);
ATF_TC_HEAD(master_noowner, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_master_loadfile() accepts broken "
				       "zones with no TTL for first record "
				       "if it is an SOA");
}
ATF_TC_BODY(master_noowner, tc) {
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = test_master("testdata/master/master3.data");
	ATF_REQUIRE_EQ(result, DNS_R_NOOWNER);

	dns_test_end();
}


/* No TTL test */
ATF_TC(master_nottl);
ATF_TC_HEAD(master_nottl, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_master_loadfile() returns "
				       "DNS_R_NOOWNER when no owner name "
				       "is specified");
}

ATF_TC_BODY(master_nottl, tc) {
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = test_master("testdata/master/master4.data");
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	dns_test_end();
}


/* Bad class test */
ATF_TC(master_badclass);
ATF_TC_HEAD(master_badclass, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_master_loadfile() returns "
				       "DNS_R_BADCLASS when record class "
				       "doesn't match zone class");
}
ATF_TC_BODY(master_badclass, tc) {
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = test_master("testdata/master/master5.data");
	ATF_REQUIRE_EQ(result, DNS_R_BADCLASS);

	dns_test_end();
}

/* DNSKEY test */
ATF_TC(master_dnskey);
ATF_TC_HEAD(master_dnskey, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_master_loadfile() understands "
				       "DNSKEY with key material");
}
ATF_TC_BODY(master_dnskey, tc) {
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = test_master("testdata/master/master6.data");
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	dns_test_end();
}


/* DNSKEY with no key material test */
ATF_TC(master_dnsnokey);
ATF_TC_HEAD(master_dnsnokey, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_master_loadfile() understands "
				       "DNSKEY with no key material");
}
ATF_TC_BODY(master_dnsnokey, tc) {
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = test_master("testdata/master/master7.data");
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	dns_test_end();
}

/* Include test */
ATF_TC(master_include);
ATF_TC_HEAD(master_include, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_master_loadfile() understands "
				       "$INCLUDE");
}
ATF_TC_BODY(master_include, tc) {
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = test_master("testdata/master/master8.data");
	ATF_REQUIRE_EQ(result, DNS_R_SEENINCLUDE);

	dns_test_end();
}

/* Include failure test */
ATF_TC(master_includefail);
ATF_TC_HEAD(master_includefail, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_master_loadfile() understands "
				       "$INCLUDE failures");
}
ATF_TC_BODY(master_includefail, tc) {
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = test_master("testdata/master/master9.data");
	ATF_REQUIRE_EQ(result, DNS_R_BADCLASS);

	dns_test_end();
}


/* Non-empty blank lines test */
ATF_TC(master_blanklines);
ATF_TC_HEAD(master_blanklines, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_master_loadfile() handles "
				       "non-empty blank lines");
}
ATF_TC_BODY(master_blanklines, tc) {
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = test_master("testdata/master/master10.data");
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	dns_test_end();
}

/* SOA leading zeroes test */
ATF_TC(master_leadingzero);
ATF_TC_HEAD(master_leadingzero, tc) {
	atf_tc_set_md_var(tc, "descr", "dns_master_loadfile() allows "
				       "leading zeroes in SOA");
}
ATF_TC_BODY(master_leadingzero, tc) {
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = test_master("testdata/master/master11.data");
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	dns_test_end();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, master_load);
	ATF_TP_ADD_TC(tp, master_unexpected);
	ATF_TP_ADD_TC(tp, master_noowner);
	ATF_TP_ADD_TC(tp, master_nottl);
	ATF_TP_ADD_TC(tp, master_badclass);
	ATF_TP_ADD_TC(tp, master_dnskey);
	ATF_TP_ADD_TC(tp, master_dnsnokey);
	ATF_TP_ADD_TC(tp, master_include);
	ATF_TP_ADD_TC(tp, master_includefail);
	ATF_TP_ADD_TC(tp, master_blanklines);
	ATF_TP_ADD_TC(tp, master_leadingzero);

	return (atf_no_error());
}

