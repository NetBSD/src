/*	$NetBSD: master_test.c,v 1.2.2.2 2024/02/25 15:47:39 martin Exp $	*/

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
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/dir.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/cache.h>
#include <dns/callbacks.h>
#include <dns/db.h>
#include <dns/master.h>
#include <dns/masterdump.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>

#include <tests/dns.h>

static void
nullmsg(dns_rdatacallbacks_t *cb, const char *fmt, ...) {
	UNUSED(cb);
	UNUSED(fmt);
}

#define BUFLEN	    255
#define BIGBUFLEN   (70 * 1024)
#define TEST_ORIGIN "test"

static dns_masterrawheader_t header;
static bool headerset;

dns_name_t dns_origin;
char origin[sizeof(TEST_ORIGIN)];
unsigned char name_buf[BUFLEN];
dns_rdatacallbacks_t callbacks;
char *include_file = NULL;

static void
rawdata_callback(dns_zone_t *zone, dns_masterrawheader_t *header);

static isc_result_t
add_callback(void *arg, const dns_name_t *owner, dns_rdataset_t *dataset) {
	char buf[BIGBUFLEN];
	isc_buffer_t target;
	isc_result_t result;

	UNUSED(arg);

	isc_buffer_init(&target, buf, BIGBUFLEN);
	result = dns_rdataset_totext(dataset, owner, false, false, &target);
	return (result);
}

static void
rawdata_callback(dns_zone_t *zone, dns_masterrawheader_t *h) {
	UNUSED(zone);
	header = *h;
	headerset = true;
}

static isc_result_t
setup_master(void (*warn)(struct dns_rdatacallbacks *, const char *, ...),
	     void (*error)(struct dns_rdatacallbacks *, const char *, ...)) {
	isc_result_t result;
	int len;
	isc_buffer_t source;
	isc_buffer_t target;

	strlcpy(origin, TEST_ORIGIN, sizeof(origin));
	len = strlen(origin);
	isc_buffer_init(&source, origin, len);
	isc_buffer_add(&source, len);
	isc_buffer_setactive(&source, len);
	isc_buffer_init(&target, name_buf, BUFLEN);
	dns_name_init(&dns_origin, NULL);
	dns_master_initrawheader(&header);

	result = dns_name_fromtext(&dns_origin, &source, dns_rootname, 0,
				   &target);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	dns_rdatacallbacks_init_stdio(&callbacks);
	callbacks.add = add_callback;
	callbacks.rawdata = rawdata_callback;
	callbacks.zone = NULL;
	if (warn != NULL) {
		callbacks.warn = warn;
	}
	if (error != NULL) {
		callbacks.error = error;
	}
	headerset = false;
	return (result);
}

static isc_result_t
test_master(const char *workdir, const char *testfile,
	    dns_masterformat_t format,
	    void (*warn)(struct dns_rdatacallbacks *, const char *, ...),
	    void (*error)(struct dns_rdatacallbacks *, const char *, ...)) {
	isc_result_t result;

	result = setup_master(warn, error);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	dns_rdatacallbacks_init_stdio(&callbacks);
	callbacks.add = add_callback;
	callbacks.rawdata = rawdata_callback;
	callbacks.zone = NULL;
	if (warn != NULL) {
		callbacks.warn = warn;
	}
	if (error != NULL) {
		callbacks.error = error;
	}

	if (workdir != NULL) {
		result = isc_dir_chdir(workdir);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}

	result = dns_master_loadfile(testfile, &dns_origin, &dns_origin,
				     dns_rdataclass_in, true, 0, &callbacks,
				     NULL, NULL, mctx, format, 0);

	return (result);
}

static void
include_callback(const char *filename, void *arg) {
	char **argp = (char **)arg;
	*argp = isc_mem_strdup(mctx, filename);
}

/*
 * Successful load test:
 * dns_master_loadfile() loads a valid master file and returns success
 */
ISC_RUN_TEST_IMPL(load) {
	isc_result_t result;

	UNUSED(state);

	result = test_master(SRCDIR, TESTS_DIR "/testdata/master/master1.data",
			     dns_masterformat_text, nullmsg, nullmsg);
	assert_int_equal(result, ISC_R_SUCCESS);
}

/*
 * Unexpected end of file test:
 * dns_master_loadfile() returns DNS_R_UNEXPECTED when file ends too soon
 */
ISC_RUN_TEST_IMPL(unexpected) {
	isc_result_t result;

	UNUSED(state);

	result = test_master(SRCDIR, TESTS_DIR "/testdata/master/master2.data",
			     dns_masterformat_text, nullmsg, nullmsg);
	assert_int_equal(result, ISC_R_UNEXPECTEDEND);
}

/*
 * No owner test:
 * dns_master_loadfile() accepts broken zones with no TTL for first record
 * if it is an SOA
 */
ISC_RUN_TEST_IMPL(noowner) {
	isc_result_t result;

	UNUSED(state);

	result = test_master(SRCDIR, TESTS_DIR "/testdata/master/master3.data",
			     dns_masterformat_text, nullmsg, nullmsg);
	assert_int_equal(result, DNS_R_NOOWNER);
}

/*
 * No TTL test:
 * dns_master_loadfile() returns DNS_R_NOOWNER when no owner name is
 * specified
 */
ISC_RUN_TEST_IMPL(nottl) {
	isc_result_t result;

	UNUSED(state);

	result = test_master(SRCDIR, TESTS_DIR "/testdata/master/master4.data",
			     dns_masterformat_text, nullmsg, nullmsg);
	assert_int_equal(result, ISC_R_SUCCESS);
}

/*
 * Bad class test:
 * dns_master_loadfile() returns DNS_R_BADCLASS when record class doesn't
 * match zone class
 */
ISC_RUN_TEST_IMPL(badclass) {
	isc_result_t result;

	UNUSED(state);

	result = test_master(SRCDIR, TESTS_DIR "/testdata/master/master5.data",
			     dns_masterformat_text, nullmsg, nullmsg);
	assert_int_equal(result, DNS_R_BADCLASS);
}

/*
 * Too big rdata test:
 * dns_master_loadfile() returns ISC_R_NOSPACE when record is too big
 */
ISC_RUN_TEST_IMPL(toobig) {
	isc_result_t result;

	UNUSED(state);

	result = test_master(SRCDIR, TESTS_DIR "/testdata/master/master15.data",
			     dns_masterformat_text, nullmsg, nullmsg);
	assert_int_equal(result, ISC_R_NOSPACE);
}

/*
 * Maximum rdata test:
 * dns_master_loadfile() returns ISC_R_SUCCESS when record is maximum size
 */
ISC_RUN_TEST_IMPL(maxrdata) {
	isc_result_t result;

	UNUSED(state);

	result = test_master(SRCDIR, TESTS_DIR "/testdata/master/master16.data",
			     dns_masterformat_text, nullmsg, nullmsg);
	assert_int_equal(result, ISC_R_SUCCESS);
}

/*
 * DNSKEY test:
 * dns_master_loadfile() understands DNSKEY with key material
 */
ISC_RUN_TEST_IMPL(dnskey) {
	isc_result_t result;

	UNUSED(state);

	result = test_master(SRCDIR, TESTS_DIR "/testdata/master/master6.data",
			     dns_masterformat_text, nullmsg, nullmsg);
	assert_int_equal(result, ISC_R_SUCCESS);
}

/*
 * DNSKEY with no key material test:
 * dns_master_loadfile() understands DNSKEY with no key material
 *
 * RFC 4034 removed the ability to signal NOKEY, so empty key material should
 * be rejected.
 */
ISC_RUN_TEST_IMPL(dnsnokey) {
	isc_result_t result;

	UNUSED(state);

	result = test_master(SRCDIR, TESTS_DIR "/testdata/master/master7.data",
			     dns_masterformat_text, nullmsg, nullmsg);
	assert_int_equal(result, ISC_R_UNEXPECTEDEND);
}

/*
 * Include test:
 * dns_master_loadfile() understands $INCLUDE
 */
ISC_RUN_TEST_IMPL(include) {
	isc_result_t result;

	UNUSED(state);

	result = test_master(SRCDIR, TESTS_DIR "/testdata/master/master8.data",
			     dns_masterformat_text, nullmsg, nullmsg);
	assert_int_equal(result, DNS_R_SEENINCLUDE);
}

/*
 * Include file list test:
 * dns_master_loadfile4() returns names of included file
 */
ISC_RUN_TEST_IMPL(master_includelist) {
	isc_result_t result;
	char *filename = NULL;

	UNUSED(state);

	result = setup_master(nullmsg, nullmsg);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_dir_chdir(SRCDIR);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_master_loadfile(
		TESTS_DIR "/testdata/master/master8.data", &dns_origin,
		&dns_origin, dns_rdataclass_in, 0, true, &callbacks,
		include_callback, &filename, mctx, dns_masterformat_text, 0);
	assert_int_equal(result, DNS_R_SEENINCLUDE);
	assert_non_null(filename);
	if (filename != NULL) {
		assert_string_equal(filename, "testdata/master/master6.data");
		isc_mem_free(mctx, filename);
	}
}

/*
 * Include failure test:
 * dns_master_loadfile() understands $INCLUDE failures
 */
ISC_RUN_TEST_IMPL(includefail) {
	isc_result_t result;

	UNUSED(state);

	result = test_master(SRCDIR, TESTS_DIR "/testdata/master/master9.data",
			     dns_masterformat_text, nullmsg, nullmsg);
	assert_int_equal(result, DNS_R_BADCLASS);
}

/*
 * Non-empty blank lines test:
 * dns_master_loadfile() handles non-empty blank lines
 */
ISC_RUN_TEST_IMPL(blanklines) {
	isc_result_t result;

	UNUSED(state);

	result = test_master(SRCDIR, TESTS_DIR "/testdata/master/master10.data",
			     dns_masterformat_text, nullmsg, nullmsg);
	assert_int_equal(result, ISC_R_SUCCESS);
}

/*
 * SOA leading zeroes test:
 * dns_master_loadfile() allows leading zeroes in SOA
 */

ISC_RUN_TEST_IMPL(leadingzero) {
	isc_result_t result;

	UNUSED(state);

	result = test_master(SRCDIR, TESTS_DIR "/testdata/master/master11.data",
			     dns_masterformat_text, nullmsg, nullmsg);
	assert_int_equal(result, ISC_R_SUCCESS);
}

/* masterfile totext tests */
ISC_RUN_TEST_IMPL(totext) {
	isc_result_t result;
	dns_rdataset_t rdataset;
	dns_rdatalist_t rdatalist;
	isc_buffer_t target;
	unsigned char buf[BIGBUFLEN];

	UNUSED(state);

	/* First, test with an empty rdataset */
	dns_rdatalist_init(&rdatalist);
	rdatalist.rdclass = dns_rdataclass_in;
	rdatalist.type = dns_rdatatype_none;
	rdatalist.covers = dns_rdatatype_none;

	dns_rdataset_init(&rdataset);
	result = dns_rdatalist_tordataset(&rdatalist, &rdataset);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_buffer_init(&target, buf, BIGBUFLEN);
	result = dns_master_rdatasettotext(dns_rootname, &rdataset,
					   &dns_master_style_debug, NULL,
					   &target);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_buffer_usedlength(&target), 0);

	/*
	 * XXX: We will also need to add tests for dumping various
	 * rdata types, classes, etc, and comparing the results against
	 * known-good output.
	 */
}

/*
 * Raw load test:
 * dns_master_loadfile() loads a valid raw file and returns success
 */
ISC_RUN_TEST_IMPL(loadraw) {
	isc_result_t result;

	UNUSED(state);

	/* Raw format version 0 */
	result = test_master(BUILDDIR, "testdata/master/master12.data",
			     dns_masterformat_raw, nullmsg, nullmsg);
	assert_string_equal(isc_result_totext(result), "success");
	assert_true(headerset);
	assert_int_equal(header.flags, 0);

	/* Raw format version 1, no source serial  */
	result = test_master(BUILDDIR, "testdata/master/master13.data",
			     dns_masterformat_raw, nullmsg, nullmsg);
	assert_string_equal(isc_result_totext(result), "success");
	assert_true(headerset);
	assert_int_equal(header.flags, 0);

	/* Raw format version 1, source serial == 2011120101 */
	result = test_master(BUILDDIR, "testdata/master/master14.data",
			     dns_masterformat_raw, nullmsg, nullmsg);
	assert_string_equal(isc_result_totext(result), "success");
	assert_true(headerset);
	assert_true((header.flags & DNS_MASTERRAW_SOURCESERIALSET) != 0);
	assert_int_equal(header.sourceserial, 2011120101);
}

/*
 * Raw dump test:
 * dns_master_dump*() functions dump valid raw files
 */
ISC_RUN_TEST_IMPL(dumpraw) {
	isc_result_t result;
	dns_db_t *db = NULL;
	dns_dbversion_t *version = NULL;
	char myorigin[sizeof(TEST_ORIGIN)];
	dns_name_t dnsorigin;
	isc_buffer_t source, target;
	unsigned char namebuf[BUFLEN];
	int len;

	UNUSED(state);

	strlcpy(myorigin, TEST_ORIGIN, sizeof(myorigin));
	len = strlen(myorigin);
	isc_buffer_init(&source, myorigin, len);
	isc_buffer_add(&source, len);
	isc_buffer_setactive(&source, len);
	isc_buffer_init(&target, namebuf, BUFLEN);
	dns_name_init(&dnsorigin, NULL);
	result = dns_name_fromtext(&dnsorigin, &source, dns_rootname, 0,
				   &target);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_db_create(mctx, "rbt", &dnsorigin, dns_dbtype_zone,
			       dns_rdataclass_in, 0, NULL, &db);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_dir_chdir(SRCDIR);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_db_load(db, TESTS_DIR "/testdata/master/master1.data",
			     dns_masterformat_text, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_dir_chdir(BUILDDIR);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_db_currentversion(db, &version);

	result = dns_master_dump(mctx, db, version, &dns_master_style_default,
				 "test.dump", dns_masterformat_raw, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = test_master(NULL, "test.dump", dns_masterformat_raw, nullmsg,
			     nullmsg);
	assert_string_equal(isc_result_totext(result), "success");
	assert_true(headerset);
	assert_int_equal(header.flags, 0);

	dns_master_initrawheader(&header);
	header.sourceserial = 12345;
	header.flags |= DNS_MASTERRAW_SOURCESERIALSET;

	unlink("test.dump");
	result = dns_master_dump(mctx, db, version, &dns_master_style_default,
				 "test.dump", dns_masterformat_raw, &header);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = test_master(NULL, "test.dump", dns_masterformat_raw, nullmsg,
			     nullmsg);
	assert_string_equal(isc_result_totext(result), "success");
	assert_true(headerset);
	assert_true((header.flags & DNS_MASTERRAW_SOURCESERIALSET) != 0);
	assert_int_equal(header.sourceserial, 12345);

	unlink("test.dump");
	dns_db_closeversion(db, &version, false);
	dns_db_detach(&db);
}

static const char *warn_expect_value;
static bool warn_expect_result;

static void
warn_expect(struct dns_rdatacallbacks *mycallbacks, const char *fmt, ...) {
	char buf[4096];
	va_list ap;

	UNUSED(mycallbacks);

	warn_expect_result = false;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (warn_expect_value != NULL && strstr(buf, warn_expect_value) != NULL)
	{
		warn_expect_result = true;
	}
}

/*
 * Origin change test:
 * dns_master_loadfile() rejects zones with inherited name following $ORIGIN
 */
ISC_RUN_TEST_IMPL(neworigin) {
	isc_result_t result;

	UNUSED(state);

	warn_expect_value = "record with inherited owner";
	result = test_master(SRCDIR, TESTS_DIR "/testdata/master/master17.data",
			     dns_masterformat_text, warn_expect, nullmsg);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(warn_expect_result);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(load)
ISC_TEST_ENTRY(unexpected)
ISC_TEST_ENTRY(noowner)
ISC_TEST_ENTRY(nottl)
ISC_TEST_ENTRY(badclass)
ISC_TEST_ENTRY(dnskey)
ISC_TEST_ENTRY(dnsnokey)
ISC_TEST_ENTRY(include)
ISC_TEST_ENTRY(master_includelist)
ISC_TEST_ENTRY(includefail)
ISC_TEST_ENTRY(blanklines)
ISC_TEST_ENTRY(leadingzero)
ISC_TEST_ENTRY(totext)
ISC_TEST_ENTRY(loadraw)
ISC_TEST_ENTRY(dumpraw)
ISC_TEST_ENTRY(toobig)
ISC_TEST_ENTRY(maxrdata)
ISC_TEST_ENTRY(neworigin)
ISC_TEST_LIST_END

ISC_TEST_MAIN
