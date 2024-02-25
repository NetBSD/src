/*	$NetBSD: name_test.c,v 1.2.2.2 2024/02/25 15:47:40 martin Exp $	*/

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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/print.h>
#include <isc/thread.h>
#include <isc/util.h>

#include <dns/compress.h>
#include <dns/fixedname.h>
#include <dns/name.h>

#include <tests/dns.h>

/* Set to true (or use -v option) for verbose output */
static bool verbose = false;

/* dns_name_fullcompare test */
ISC_RUN_TEST_IMPL(fullcompare) {
	dns_fixedname_t fixed1;
	dns_fixedname_t fixed2;
	dns_name_t *name1;
	dns_name_t *name2;
	dns_namereln_t relation;
	int i;
	isc_result_t result;
	struct {
		const char *name1;
		const char *name2;
		dns_namereln_t relation;
		int order;
		unsigned int nlabels;
	} data[] = {
		/* relative */
		{ "", "", dns_namereln_equal, 0, 0 },
		{ "foo", "", dns_namereln_subdomain, 1, 0 },
		{ "", "foo", dns_namereln_contains, -1, 0 },
		{ "foo", "bar", dns_namereln_none, 4, 0 },
		{ "bar", "foo", dns_namereln_none, -4, 0 },
		{ "bar.foo", "foo", dns_namereln_subdomain, 1, 1 },
		{ "foo", "bar.foo", dns_namereln_contains, -1, 1 },
		{ "baz.bar.foo", "bar.foo", dns_namereln_subdomain, 1, 2 },
		{ "bar.foo", "baz.bar.foo", dns_namereln_contains, -1, 2 },
		{ "foo.example", "bar.example", dns_namereln_commonancestor, 4,
		  1 },

		/* absolute */
		{ ".", ".", dns_namereln_equal, 0, 1 },
		{ "foo.", "bar.", dns_namereln_commonancestor, 4, 1 },
		{ "bar.", "foo.", dns_namereln_commonancestor, -4, 1 },
		{ "foo.example.", "bar.example.", dns_namereln_commonancestor,
		  4, 2 },
		{ "bar.foo.", "foo.", dns_namereln_subdomain, 1, 2 },
		{ "foo.", "bar.foo.", dns_namereln_contains, -1, 2 },
		{ "baz.bar.foo.", "bar.foo.", dns_namereln_subdomain, 1, 3 },
		{ "bar.foo.", "baz.bar.foo.", dns_namereln_contains, -1, 3 },
		{ NULL, NULL, dns_namereln_none, 0, 0 }
	};

	UNUSED(state);

	name1 = dns_fixedname_initname(&fixed1);
	name2 = dns_fixedname_initname(&fixed2);
	for (i = 0; data[i].name1 != NULL; i++) {
		int order = 3000;
		unsigned int nlabels = 3000;

		if (data[i].name1[0] == 0) {
			dns_fixedname_init(&fixed1);
		} else {
			result = dns_name_fromstring2(name1, data[i].name1,
						      NULL, 0, NULL);
			assert_int_equal(result, ISC_R_SUCCESS);
		}
		if (data[i].name2[0] == 0) {
			dns_fixedname_init(&fixed2);
		} else {
			result = dns_name_fromstring2(name2, data[i].name2,
						      NULL, 0, NULL);
			assert_int_equal(result, ISC_R_SUCCESS);
		}
		relation = dns_name_fullcompare(name1, name1, &order, &nlabels);
		assert_int_equal(relation, dns_namereln_equal);
		assert_int_equal(order, 0);
		assert_int_equal(nlabels, name1->labels);

		/* Some random initializer */
		order = 3001;
		nlabels = 3001;

		relation = dns_name_fullcompare(name1, name2, &order, &nlabels);
		assert_int_equal(relation, data[i].relation);
		assert_int_equal(order, data[i].order);
		assert_int_equal(nlabels, data[i].nlabels);
	}
}

static void
compress_test(dns_name_t *name1, dns_name_t *name2, dns_name_t *name3,
	      unsigned char *expected, unsigned int length,
	      dns_compress_t *cctx, dns_decompress_t *dctx) {
	isc_buffer_t source;
	isc_buffer_t target;
	dns_name_t name;
	unsigned char buf1[1024];
	unsigned char buf2[1024];

	isc_buffer_init(&source, buf1, sizeof(buf1));
	isc_buffer_init(&target, buf2, sizeof(buf2));

	assert_int_equal(dns_name_towire(name1, cctx, &source), ISC_R_SUCCESS);

	assert_int_equal(dns_name_towire(name2, cctx, &source), ISC_R_SUCCESS);
	assert_int_equal(dns_name_towire(name2, cctx, &source), ISC_R_SUCCESS);
	assert_int_equal(dns_name_towire(name3, cctx, &source), ISC_R_SUCCESS);

	isc_buffer_setactive(&source, source.used);

	dns_name_init(&name, NULL);
	RUNTIME_CHECK(dns_name_fromwire(&name, &source, dctx, 0, &target) ==
		      ISC_R_SUCCESS);
	RUNTIME_CHECK(dns_name_fromwire(&name, &source, dctx, 0, &target) ==
		      ISC_R_SUCCESS);
	RUNTIME_CHECK(dns_name_fromwire(&name, &source, dctx, 0, &target) ==
		      ISC_R_SUCCESS);
	RUNTIME_CHECK(dns_name_fromwire(&name, &source, dctx, 0, &target) ==
		      ISC_R_SUCCESS);
	dns_decompress_invalidate(dctx);

	assert_int_equal(target.used, length);
	assert_true(memcmp(target.base, expected, target.used) == 0);
}

/* name compression test */
ISC_RUN_TEST_IMPL(compression) {
	unsigned int allowed;
	dns_compress_t cctx;
	dns_decompress_t dctx;
	dns_name_t name1;
	dns_name_t name2;
	dns_name_t name3;
	isc_region_t r;
	unsigned char plain1[] = "\003yyy\003foo";
	unsigned char plain2[] = "\003bar\003yyy\003foo";
	unsigned char plain3[] = "\003xxx\003bar\003foo";
	unsigned char plain[] = "\003yyy\003foo\0\003bar\003yyy\003foo\0\003"
				"bar\003yyy\003foo\0\003xxx\003bar\003foo";

	UNUSED(state);

	dns_name_init(&name1, NULL);
	r.base = plain1;
	r.length = sizeof(plain1);
	dns_name_fromregion(&name1, &r);

	dns_name_init(&name2, NULL);
	r.base = plain2;
	r.length = sizeof(plain2);
	dns_name_fromregion(&name2, &r);

	dns_name_init(&name3, NULL);
	r.base = plain3;
	r.length = sizeof(plain3);
	dns_name_fromregion(&name3, &r);

	/* Test 1: NONE */
	allowed = DNS_COMPRESS_NONE;
	assert_int_equal(dns_compress_init(&cctx, -1, mctx), ISC_R_SUCCESS);
	dns_compress_setmethods(&cctx, allowed);
	dns_decompress_init(&dctx, -1, DNS_DECOMPRESS_STRICT);
	dns_decompress_setmethods(&dctx, allowed);

	compress_test(&name1, &name2, &name3, plain, sizeof(plain), &cctx,
		      &dctx);

	dns_compress_rollback(&cctx, 0);
	dns_compress_invalidate(&cctx);

	/* Test2: GLOBAL14 */
	allowed = DNS_COMPRESS_GLOBAL14;
	assert_int_equal(dns_compress_init(&cctx, -1, mctx), ISC_R_SUCCESS);
	dns_compress_setmethods(&cctx, allowed);
	dns_decompress_init(&dctx, -1, DNS_DECOMPRESS_STRICT);
	dns_decompress_setmethods(&dctx, allowed);

	compress_test(&name1, &name2, &name3, plain, sizeof(plain), &cctx,
		      &dctx);

	dns_compress_rollback(&cctx, 0);
	dns_compress_invalidate(&cctx);

	/* Test3: ALL */
	allowed = DNS_COMPRESS_ALL;
	assert_int_equal(dns_compress_init(&cctx, -1, mctx), ISC_R_SUCCESS);
	dns_compress_setmethods(&cctx, allowed);
	dns_decompress_init(&dctx, -1, DNS_DECOMPRESS_STRICT);
	dns_decompress_setmethods(&dctx, allowed);

	compress_test(&name1, &name2, &name3, plain, sizeof(plain), &cctx,
		      &dctx);

	dns_compress_rollback(&cctx, 0);
	dns_compress_invalidate(&cctx);

	/* Test4: NONE disabled */
	allowed = DNS_COMPRESS_NONE;
	assert_int_equal(dns_compress_init(&cctx, -1, mctx), ISC_R_SUCCESS);
	dns_compress_setmethods(&cctx, allowed);
	dns_compress_disable(&cctx);
	dns_decompress_init(&dctx, -1, DNS_DECOMPRESS_STRICT);
	dns_decompress_setmethods(&dctx, allowed);

	compress_test(&name1, &name2, &name3, plain, sizeof(plain), &cctx,
		      &dctx);

	dns_compress_rollback(&cctx, 0);
	dns_compress_invalidate(&cctx);

	/* Test5: GLOBAL14 disabled */
	allowed = DNS_COMPRESS_GLOBAL14;
	assert_int_equal(dns_compress_init(&cctx, -1, mctx), ISC_R_SUCCESS);
	dns_compress_setmethods(&cctx, allowed);
	dns_compress_disable(&cctx);
	dns_decompress_init(&dctx, -1, DNS_DECOMPRESS_STRICT);
	dns_decompress_setmethods(&dctx, allowed);

	compress_test(&name1, &name2, &name3, plain, sizeof(plain), &cctx,
		      &dctx);

	dns_compress_rollback(&cctx, 0);
	dns_compress_invalidate(&cctx);

	/* Test6: ALL disabled */
	allowed = DNS_COMPRESS_ALL;
	assert_int_equal(dns_compress_init(&cctx, -1, mctx), ISC_R_SUCCESS);
	dns_compress_setmethods(&cctx, allowed);
	dns_compress_disable(&cctx);
	dns_decompress_init(&dctx, -1, DNS_DECOMPRESS_STRICT);
	dns_decompress_setmethods(&dctx, allowed);

	compress_test(&name1, &name2, &name3, plain, sizeof(plain), &cctx,
		      &dctx);

	dns_compress_rollback(&cctx, 0);
	dns_compress_invalidate(&cctx);
}

/* is trust-anchor-telemetry test */
ISC_RUN_TEST_IMPL(istat) {
	dns_fixedname_t fixed;
	dns_name_t *name;
	isc_result_t result;
	size_t i;
	struct {
		const char *name;
		bool istat;
	} data[] = { { ".", false },
		     { "_ta-", false },
		     { "_ta-1234", true },
		     { "_TA-1234", true },
		     { "+TA-1234", false },
		     { "_fa-1234", false },
		     { "_td-1234", false },
		     { "_ta_1234", false },
		     { "_ta-g234", false },
		     { "_ta-1h34", false },
		     { "_ta-12i4", false },
		     { "_ta-123j", false },
		     { "_ta-1234-abcf", true },
		     { "_ta-1234-abcf-ED89", true },
		     { "_ta-12345-abcf-ED89", false },
		     { "_ta-.example", false },
		     { "_ta-1234.example", true },
		     { "_ta-1234-abcf.example", true },
		     { "_ta-1234-abcf-ED89.example", true },
		     { "_ta-12345-abcf-ED89.example", false },
		     { "_ta-1234-abcfe-ED89.example", false },
		     { "_ta-1234-abcf-EcD89.example", false } };

	UNUSED(state);

	name = dns_fixedname_initname(&fixed);

	for (i = 0; i < (sizeof(data) / sizeof(data[0])); i++) {
		result = dns_name_fromstring(name, data[i].name, 0, NULL);
		assert_int_equal(result, ISC_R_SUCCESS);
		assert_int_equal(dns_name_istat(name), data[i].istat);
	}
}

/* dns_nane_init */
ISC_RUN_TEST_IMPL(init) {
	dns_name_t name;
	unsigned char offsets[1];

	UNUSED(state);

	dns_name_init(&name, offsets);

	assert_null(name.ndata);
	assert_int_equal(name.length, 0);
	assert_int_equal(name.labels, 0);
	assert_int_equal(name.attributes, 0);
	assert_ptr_equal(name.offsets, offsets);
	assert_null(name.buffer);
}

/* dns_nane_invalidate */
ISC_RUN_TEST_IMPL(invalidate) {
	dns_name_t name;
	unsigned char offsets[1];

	UNUSED(state);

	dns_name_init(&name, offsets);
	dns_name_invalidate(&name);

	assert_null(name.ndata);
	assert_int_equal(name.length, 0);
	assert_int_equal(name.labels, 0);
	assert_int_equal(name.attributes, 0);
	assert_null(name.offsets);
	assert_null(name.buffer);
}

/* dns_nane_setbuffer/hasbuffer */
ISC_RUN_TEST_IMPL(buffer) {
	dns_name_t name;
	unsigned char buf[BUFSIZ];
	isc_buffer_t b;

	UNUSED(state);

	isc_buffer_init(&b, buf, BUFSIZ);
	dns_name_init(&name, NULL);
	dns_name_setbuffer(&name, &b);
	assert_ptr_equal(name.buffer, &b);
	assert_true(dns_name_hasbuffer(&name));
}

/* dns_nane_isabsolute */
ISC_RUN_TEST_IMPL(isabsolute) {
	struct {
		const char *namestr;
		bool expect;
	} testcases[] = { { "x", false },
			  { "a.b.c.d.", true },
			  { "x.z", false } };
	unsigned int i;

	UNUSED(state);

	for (i = 0; i < (sizeof(testcases) / sizeof(testcases[0])); i++) {
		isc_result_t result;
		dns_name_t name;
		unsigned char data[BUFSIZ];
		isc_buffer_t b, nb;
		size_t len;

		len = strlen(testcases[i].namestr);
		isc_buffer_constinit(&b, testcases[i].namestr, len);
		isc_buffer_add(&b, len);

		dns_name_init(&name, NULL);
		isc_buffer_init(&nb, data, BUFSIZ);
		dns_name_setbuffer(&name, &nb);
		result = dns_name_fromtext(&name, &b, NULL, 0, NULL);
		assert_int_equal(result, ISC_R_SUCCESS);

		assert_int_equal(dns_name_isabsolute(&name),
				 testcases[i].expect);
	}
}

/* dns_nane_hash */
ISC_RUN_TEST_IMPL(hash) {
	struct {
		const char *name1;
		const char *name2;
		bool expect;
		bool expecti;
	} testcases[] = {
		{ "a.b.c.d", "A.B.C.D", true, false },
		{ "a.b.c.d.", "A.B.C.D.", true, false },
		{ "a.b.c.d", "a.b.c.d", true, true },
		{ "A.B.C.D.", "A.B.C.D.", true, false },
		{ "x.y.z.w", "a.b.c.d", false, false },
		{ "x.y.z.w.", "a.b.c.d.", false, false },
	};
	unsigned int i;

	UNUSED(state);

	for (i = 0; i < (sizeof(testcases) / sizeof(testcases[0])); i++) {
		isc_result_t result;
		dns_fixedname_t f1, f2;
		dns_name_t *n1, *n2;
		unsigned int h1, h2;

		n1 = dns_fixedname_initname(&f1);
		n2 = dns_fixedname_initname(&f2);

		result = dns_name_fromstring2(n1, testcases[i].name1, NULL, 0,
					      NULL);
		assert_int_equal(result, ISC_R_SUCCESS);
		result = dns_name_fromstring2(n2, testcases[i].name2, NULL, 0,
					      NULL);
		assert_int_equal(result, ISC_R_SUCCESS);

		/* Check case-insensitive hashing first */
		h1 = dns_name_hash(n1, false);
		h2 = dns_name_hash(n2, false);

		if (verbose) {
			print_message("# %s hashes to %u, "
				      "%s to %u, case insensitive\n",
				      testcases[i].name1, h1,
				      testcases[i].name2, h2);
		}

		assert_int_equal((h1 == h2), testcases[i].expect);

		/* Now case-sensitive */
		h1 = dns_name_hash(n1, false);
		h2 = dns_name_hash(n2, false);

		if (verbose) {
			print_message("# %s hashes to %u, "
				      "%s to %u, case sensitive\n",
				      testcases[i].name1, h1,
				      testcases[i].name2, h2);
		}

		assert_int_equal((h1 == h2), testcases[i].expect);
	}
}

/* dns_nane_issubdomain */
ISC_RUN_TEST_IMPL(issubdomain) {
	struct {
		const char *name1;
		const char *name2;
		bool expect;
	} testcases[] = {
		{ "c.d", "a.b.c.d", false }, { "c.d.", "a.b.c.d.", false },
		{ "b.c.d", "c.d", true },    { "a.b.c.d.", "c.d.", true },
		{ "a.b.c", "a.b.c", true },  { "a.b.c.", "a.b.c.", true },
		{ "x.y.z", "a.b.c", false }
	};
	unsigned int i;

	UNUSED(state);

	for (i = 0; i < (sizeof(testcases) / sizeof(testcases[0])); i++) {
		isc_result_t result;
		dns_fixedname_t f1, f2;
		dns_name_t *n1, *n2;

		n1 = dns_fixedname_initname(&f1);
		n2 = dns_fixedname_initname(&f2);

		result = dns_name_fromstring2(n1, testcases[i].name1, NULL, 0,
					      NULL);
		assert_int_equal(result, ISC_R_SUCCESS);
		result = dns_name_fromstring2(n2, testcases[i].name2, NULL, 0,
					      NULL);
		assert_int_equal(result, ISC_R_SUCCESS);

		if (verbose) {
			print_message("# check: %s %s a subdomain of %s\n",
				      testcases[i].name1,
				      testcases[i].expect ? "is" : "is not",
				      testcases[i].name2);
		}

		assert_int_equal(dns_name_issubdomain(n1, n2),
				 testcases[i].expect);
	}
}

/* dns_nane_countlabels */
ISC_RUN_TEST_IMPL(countlabels) {
	struct {
		const char *namestr;
		unsigned int expect;
	} testcases[] = {
		{ "c.d", 2 },	  { "c.d.", 3 },  { "a.b.c.d.", 5 },
		{ "a.b.c.d", 4 }, { "a.b.c", 3 }, { ".", 1 },
	};
	unsigned int i;

	UNUSED(state);

	for (i = 0; i < (sizeof(testcases) / sizeof(testcases[0])); i++) {
		isc_result_t result;
		dns_fixedname_t fname;
		dns_name_t *name;

		name = dns_fixedname_initname(&fname);

		result = dns_name_fromstring2(name, testcases[i].namestr, NULL,
					      0, NULL);
		assert_int_equal(result, ISC_R_SUCCESS);

		if (verbose) {
			print_message("# %s: expect %u labels\n",
				      testcases[i].namestr,
				      testcases[i].expect);
		}

		assert_int_equal(dns_name_countlabels(name),
				 testcases[i].expect);
	}
}

/* dns_nane_getlabel */
ISC_RUN_TEST_IMPL(getlabel) {
	struct {
		const char *name1;
		unsigned int pos1;
		const char *name2;
		unsigned int pos2;
	} testcases[] = {
		{ "c.d", 1, "a.b.c.d", 3 },
		{ "a.b.c.d", 3, "c.d", 1 },
		{ "a.b.c.", 3, "A.B.C.", 3 },
	};
	unsigned int i;

	UNUSED(state);

	for (i = 0; i < (sizeof(testcases) / sizeof(testcases[0])); i++) {
		isc_result_t result;
		dns_fixedname_t f1, f2;
		dns_name_t *n1, *n2;
		dns_label_t l1, l2;
		unsigned int j;

		n1 = dns_fixedname_initname(&f1);
		n2 = dns_fixedname_initname(&f2);

		result = dns_name_fromstring2(n1, testcases[i].name1, NULL, 0,
					      NULL);
		assert_int_equal(result, ISC_R_SUCCESS);
		result = dns_name_fromstring2(n2, testcases[i].name2, NULL, 0,
					      NULL);
		assert_int_equal(result, ISC_R_SUCCESS);

		dns_name_getlabel(n1, testcases[i].pos1, &l1);
		dns_name_getlabel(n2, testcases[i].pos2, &l2);
		assert_int_equal(l1.length, l2.length);

		for (j = 0; j < l1.length; j++) {
			assert_int_equal(l1.base[j], l2.base[j]);
		}
	}
}

/* dns_nane_getlabelsequence */
ISC_RUN_TEST_IMPL(getlabelsequence) {
	struct {
		const char *name1;
		unsigned int pos1;
		const char *name2;
		unsigned int pos2;
		unsigned int range;
	} testcases[] = {
		{ "c.d", 1, "a.b.c.d", 3, 1 },
		{ "a.b.c.d.e", 2, "c.d", 0, 2 },
		{ "a.b.c", 0, "a.b.c", 0, 3 },
	};
	unsigned int i;

	UNUSED(state);

	for (i = 0; i < (sizeof(testcases) / sizeof(testcases[0])); i++) {
		isc_result_t result;
		dns_name_t t1, t2;
		dns_fixedname_t f1, f2;
		dns_name_t *n1, *n2;

		/* target names */
		dns_name_init(&t1, NULL);
		dns_name_init(&t2, NULL);

		/* source names */
		n1 = dns_fixedname_initname(&f1);
		n2 = dns_fixedname_initname(&f2);

		result = dns_name_fromstring2(n1, testcases[i].name1, NULL, 0,
					      NULL);
		assert_int_equal(result, ISC_R_SUCCESS);
		result = dns_name_fromstring2(n2, testcases[i].name2, NULL, 0,
					      NULL);
		assert_int_equal(result, ISC_R_SUCCESS);

		dns_name_getlabelsequence(n1, testcases[i].pos1,
					  testcases[i].range, &t1);
		dns_name_getlabelsequence(n2, testcases[i].pos2,
					  testcases[i].range, &t2);

		assert_true(dns_name_equal(&t1, &t2));
	}
}

#ifdef DNS_BENCHMARK_TESTS

/*
 * XXXMUKS: Don't delete this code. It is useful in benchmarking the
 * name parser, but we don't require it as part of the unit test runs.
 */

/* Benchmark dns_name_fromwire() implementation */

ISC_RUN_TEST_IMPL(fromwire_thread(void *arg) {
	unsigned int maxval = 32000000;
	uint8_t data[] = { 3,	'w', 'w', 'w', 7,   'e', 'x',
			   'a', 'm', 'p', 'l', 'e', 7,	 'i',
			   'n', 'v', 'a', 'l', 'i', 'd', 0 };
	unsigned char output_data[DNS_NAME_MAXWIRE];
	isc_buffer_t source, target;
	unsigned int i;
	dns_decompress_t dctx;

	UNUSED(arg);

	dns_decompress_init(&dctx, -1, DNS_DECOMPRESS_STRICT);
	dns_decompress_setmethods(&dctx, DNS_COMPRESS_NONE);

	isc_buffer_init(&source, data, sizeof(data));
	isc_buffer_add(&source, sizeof(data));
	isc_buffer_init(&target, output_data, sizeof(output_data));

	/* Parse 32 million names in each thread */
	for (i = 0; i < maxval; i++) {
		dns_name_t name;

		isc_buffer_clear(&source);
		isc_buffer_clear(&target);
		isc_buffer_add(&source, sizeof(data));
		isc_buffer_setactive(&source, sizeof(data));

		dns_name_init(&name, NULL);
		(void)dns_name_fromwire(&name, &source, &dctx, 0, &target);
	}

	return (NULL);
}

ISC_RUN_TEST_IMPL(benchmark) {
	isc_result_t result;
	unsigned int i;
	isc_time_t ts1, ts2;
	double t;
	unsigned int nthreads;
	isc_thread_t threads[32];

	UNUSED(state);

	debug_mem_record = false;

	result = isc_time_now(&ts1);
	assert_int_equal(result, ISC_R_SUCCESS);

	nthreads = ISC_MIN(isc_os_ncpus(), 32);
	nthreads = ISC_MAX(nthreads, 1);
	for (i = 0; i < nthreads; i++) {
		isc_thread_create(fromwire_thread, NULL, &threads[i]);
	}

	for (i = 0; i < nthreads; i++) {
		isc_thread_join(threads[i], NULL);
	}

	result = isc_time_now(&ts2);
	assert_int_equal(result, ISC_R_SUCCESS);

	t = isc_time_microdiff(&ts2, &ts1);

	printf("%u dns_name_fromwire() calls, %f seconds, %f calls/second\n",
	       nthreads * 32000000, t / 1000000.0,
	       (nthreads * 32000000) / (t / 1000000.0));
}

#endif /* DNS_BENCHMARK_TESTS */

ISC_TEST_LIST_START
ISC_TEST_ENTRY(fullcompare)
ISC_TEST_ENTRY(compression)
ISC_TEST_ENTRY(istat)
ISC_TEST_ENTRY(init)
ISC_TEST_ENTRY(invalidate)
ISC_TEST_ENTRY(buffer)
ISC_TEST_ENTRY(isabsolute)
ISC_TEST_ENTRY(hash)
ISC_TEST_ENTRY(issubdomain)
ISC_TEST_ENTRY(countlabels)
ISC_TEST_ENTRY(getlabel)
ISC_TEST_ENTRY(getlabelsequence)
#ifdef DNS_BENCHMARK_TESTS
ISC_TEST_ENTRY(benchmark)
#endif /* DNS_BENCHMARK_TESTS */
ISC_TEST_LIST_END

ISC_TEST_MAIN
