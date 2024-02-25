/*	$NetBSD: private_test.c,v 1.2.2.2 2024/02/25 15:47:40 martin Exp $	*/

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
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/util.h>

#include <dns/nsec3.h>
#include <dns/private.h>
#include <dns/rdataclass.h>
#include <dns/rdatatype.h>

#include <dst/dst.h>

#include <tests/dns.h>

static dns_rdatatype_t privatetype = 65534;

static int
setup_test(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = dst_lib_init(mctx, NULL);

	if (result != ISC_R_SUCCESS) {
		return (1);
	}

	return (0);
}

static int
teardown_test(void **state) {
	UNUSED(state);

	dst_lib_destroy();

	return (0);
}

typedef struct {
	unsigned char alg;
	dns_keytag_t keyid;
	bool remove;
	bool complete;
} signing_testcase_t;

typedef struct {
	unsigned char hash;
	unsigned char flags;
	unsigned int iterations;
	unsigned long salt;
	bool remove;
	bool pending;
	bool nonsec;
} nsec3_testcase_t;

static void
make_signing(signing_testcase_t *testcase, dns_rdata_t *private,
	     unsigned char *buf, size_t len) {
	dns_rdata_init(private);

	buf[0] = testcase->alg;
	buf[1] = (testcase->keyid & 0xff00) >> 8;
	buf[2] = (testcase->keyid & 0xff);
	buf[3] = testcase->remove;
	buf[4] = testcase->complete;
	private->data = buf;
	private->length = len;
	private->type = privatetype;
	private->rdclass = dns_rdataclass_in;
}

static void
make_nsec3(nsec3_testcase_t *testcase, dns_rdata_t *private,
	   unsigned char *pbuf) {
	dns_rdata_nsec3param_t params;
	dns_rdata_t nsec3param = DNS_RDATA_INIT;
	unsigned char bufdata[BUFSIZ];
	isc_buffer_t buf;
	uint32_t salt;
	unsigned char *sp;
	int slen = 4;

	/* for simplicity, we're using a maximum salt length of 4 */
	salt = htonl(testcase->salt);
	sp = (unsigned char *)&salt;
	while (slen > 0 && *sp == '\0') {
		slen--;
		sp++;
	}

	params.common.rdclass = dns_rdataclass_in;
	params.common.rdtype = dns_rdatatype_nsec3param;
	params.hash = testcase->hash;
	params.iterations = testcase->iterations;
	params.salt = sp;
	params.salt_length = slen;

	params.flags = testcase->flags;
	if (testcase->remove) {
		params.flags |= DNS_NSEC3FLAG_REMOVE;
		if (testcase->nonsec) {
			params.flags |= DNS_NSEC3FLAG_NONSEC;
		}
	} else {
		params.flags |= DNS_NSEC3FLAG_CREATE;
		if (testcase->pending) {
			params.flags |= DNS_NSEC3FLAG_INITIAL;
		}
	}

	isc_buffer_init(&buf, bufdata, sizeof(bufdata));
	dns_rdata_fromstruct(&nsec3param, dns_rdataclass_in,
			     dns_rdatatype_nsec3param, &params, &buf);

	dns_rdata_init(private);

	dns_nsec3param_toprivate(&nsec3param, private, privatetype, pbuf,
				 DNS_NSEC3PARAM_BUFFERSIZE + 1);
}

/* convert private signing records to text */
ISC_RUN_TEST_IMPL(private_signing_totext) {
	dns_rdata_t private;
	int i;

	signing_testcase_t testcases[] = { { DST_ALG_RSASHA512, 12345, 0, 0 },
					   { DST_ALG_RSASHA256, 54321, 1, 0 },
					   { DST_ALG_NSEC3RSASHA1, 22222, 0,
					     1 },
					   { DST_ALG_RSASHA1, 33333, 1, 1 } };
	const char *results[] = { "Signing with key 12345/RSASHA512",
				  "Removing signatures for key 54321/RSASHA256",
				  "Done signing with key 22222/NSEC3RSASHA1",
				  ("Done removing signatures for key "
				   "33333/RSASHA1") };
	int ncases = 4;

	UNUSED(state);

	for (i = 0; i < ncases; i++) {
		unsigned char data[5];
		char output[BUFSIZ];
		isc_buffer_t buf;

		isc_buffer_init(&buf, output, sizeof(output));

		make_signing(&testcases[i], &private, data, sizeof(data));
		dns_private_totext(&private, &buf);
		assert_string_equal(output, results[i]);
	}
}

/* convert private chain records to text */
ISC_RUN_TEST_IMPL(private_nsec3_totext) {
	dns_rdata_t private;
	int i;

	nsec3_testcase_t testcases[] = {
		{ 1, 0, 1, 0xbeef, 0, 0, 0 },
		{ 1, 1, 10, 0xdadd, 0, 0, 0 },
		{ 1, 0, 20, 0xbead, 0, 1, 0 },
		{ 1, 0, 30, 0xdeaf, 1, 0, 0 },
		{ 1, 0, 100, 0xfeedabee, 1, 0, 1 },
	};
	const char *results[] = { "Creating NSEC3 chain 1 0 1 BEEF",
				  "Creating NSEC3 chain 1 1 10 DADD",
				  "Pending NSEC3 chain 1 0 20 BEAD",
				  ("Removing NSEC3 chain 1 0 30 DEAF / "
				   "creating NSEC chain"),
				  "Removing NSEC3 chain 1 0 100 FEEDABEE" };
	int ncases = 5;

	UNUSED(state);

	for (i = 0; i < ncases; i++) {
		unsigned char data[DNS_NSEC3PARAM_BUFFERSIZE + 1];
		char output[BUFSIZ];
		isc_buffer_t buf;

		isc_buffer_init(&buf, output, sizeof(output));

		make_nsec3(&testcases[i], &private, data);
		dns_private_totext(&private, &buf);
		assert_string_equal(output, results[i]);
	}
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(private_signing_totext, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(private_nsec3_totext, setup_test, teardown_test)
ISC_TEST_LIST_END

ISC_TEST_MAIN
