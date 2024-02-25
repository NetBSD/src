/*	$NetBSD: dnstap_test.c,v 1.2.2.2 2024/02/25 15:47:39 martin Exp $	*/

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
#include <fstrm.h>

#include <protobuf-c/protobuf-c.h>

#include <isc/buffer.h>
#include <isc/file.h>
#include <isc/print.h>
#include <isc/stdio.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/dnstap.h>
#include <dns/view.h>

#include <tests/dns.h>

#define TAPFILE TESTS_DIR "/testdata/dnstap/dnstap.file"
#define TAPSOCK TESTS_DIR "/testdata/dnstap/dnstap.sock"

#define TAPSAVED TESTS_DIR "/testdata/dnstap/dnstap.saved"
#define TAPTEXT	 TESTS_DIR "/testdata/dnstap/dnstap.text"

static int
cleanup(void **state __attribute__((__unused__))) {
	(void)isc_file_remove(TAPFILE);
	(void)isc_file_remove(TAPSOCK);

	return (0);
}

static int
setup(void **state) {
	/*
	 * Make sure files are cleaned up before the test runs.
	 */
	cleanup(state);

	/*
	 * Make sure text conversions match the time zone in which
	 * the testdata was originally generated.
	 */
	setenv("TZ", "PDT8", 1);
	return (0);
}

/* set up dnstap environment */
ISC_RUN_TEST_IMPL(dns_dt_create) {
	isc_result_t result;
	dns_dtenv_t *dtenv = NULL;
	struct fstrm_iothr_options *fopt;

	fopt = fstrm_iothr_options_init();
	assert_non_null(fopt);
	fstrm_iothr_options_set_num_input_queues(fopt, 1);

	result = dns_dt_create(mctx, dns_dtmode_file, TAPFILE, &fopt, NULL,
			       &dtenv);
	assert_int_equal(result, ISC_R_SUCCESS);
	if (dtenv != NULL) {
		dns_dt_detach(&dtenv);
	}
	if (fopt != NULL) {
		fstrm_iothr_options_destroy(&fopt);
	}

	assert_true(isc_file_exists(TAPFILE));

	fopt = fstrm_iothr_options_init();
	assert_non_null(fopt);
	fstrm_iothr_options_set_num_input_queues(fopt, 1);

	result = dns_dt_create(mctx, dns_dtmode_unix, TAPSOCK, &fopt, NULL,
			       &dtenv);
	assert_int_equal(result, ISC_R_SUCCESS);
	if (dtenv != NULL) {
		dns_dt_detach(&dtenv);
	}
	if (fopt != NULL) {
		fstrm_iothr_options_destroy(&fopt);
	}

	/* 'create' should succeed, but the file shouldn't exist yet */
	assert_false(isc_file_exists(TAPSOCK));

	fopt = fstrm_iothr_options_init();
	assert_non_null(fopt);
	fstrm_iothr_options_set_num_input_queues(fopt, 1);

	result = dns_dt_create(mctx, 33, TAPSOCK, &fopt, NULL, &dtenv);
	assert_int_equal(result, ISC_R_FAILURE);
	assert_null(dtenv);
	if (dtenv != NULL) {
		dns_dt_detach(&dtenv);
	}
	if (fopt != NULL) {
		fstrm_iothr_options_destroy(&fopt);
	}
}

/* send dnstap messages */
ISC_RUN_TEST_IMPL(dns_dt_send) {
	isc_result_t result;
	dns_dtenv_t *dtenv = NULL;
	dns_dthandle_t *handle = NULL;
	uint8_t *data;
	size_t dsize;
	unsigned char zone[DNS_NAME_MAXWIRE];
	unsigned char qambuffer[4096], rambuffer[4096];
	unsigned char qrmbuffer[4096], rrmbuffer[4096];
	isc_buffer_t zb, qamsg, ramsg, qrmsg, rrmsg;
	size_t qasize, qrsize, rasize, rrsize;
	dns_fixedname_t zfname;
	dns_name_t *zname;
	dns_dtmsgtype_t dt;
	dns_view_t *view = NULL;
	dns_compress_t cctx;
	isc_region_t zr;
	isc_sockaddr_t qaddr;
	isc_sockaddr_t raddr;
	struct in_addr in;
	isc_stdtime_t now;
	isc_time_t p, f;
	struct fstrm_iothr_options *fopt;

	result = dns_test_makeview("test", false, &view);
	assert_int_equal(result, ISC_R_SUCCESS);

	fopt = fstrm_iothr_options_init();
	assert_non_null(fopt);
	fstrm_iothr_options_set_num_input_queues(fopt, 1);

	result = dns_dt_create(mctx, dns_dtmode_file, TAPFILE, &fopt, NULL,
			       &dtenv);
	assert_int_equal(result, ISC_R_SUCCESS);

	dns_dt_attach(dtenv, &view->dtenv);
	view->dttypes = DNS_DTTYPE_ALL;

	/*
	 * Set up some test data
	 */
	zname = dns_fixedname_initname(&zfname);
	isc_buffer_constinit(&zb, "example.com.", 12);
	isc_buffer_add(&zb, 12);
	result = dns_name_fromtext(zname, &zb, NULL, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	memset(&zr, 0, sizeof(zr));
	isc_buffer_init(&zb, zone, sizeof(zone));
	result = dns_compress_init(&cctx, -1, mctx);
	assert_int_equal(result, ISC_R_SUCCESS);
	dns_compress_setmethods(&cctx, DNS_COMPRESS_NONE);
	result = dns_name_towire(zname, &cctx, &zb);
	assert_int_equal(result, ISC_R_SUCCESS);
	dns_compress_invalidate(&cctx);
	isc_buffer_usedregion(&zb, &zr);

	in.s_addr = inet_addr("10.53.0.1");
	isc_sockaddr_fromin(&qaddr, &in, 2112);
	in.s_addr = inet_addr("10.53.0.2");
	isc_sockaddr_fromin(&raddr, &in, 2112);

	isc_stdtime_get(&now);
	isc_time_set(&p, now - 3600, 0); /* past */
	isc_time_set(&f, now + 3600, 0); /* future */

	result = dns_test_getdata(TESTS_DIR "/testdata/dnstap/query.auth",
				  qambuffer, sizeof(qambuffer), &qasize);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_buffer_init(&qamsg, qambuffer, qasize);
	isc_buffer_add(&qamsg, qasize);

	result = dns_test_getdata(TESTS_DIR "/testdata/dnstap/response.auth",
				  rambuffer, sizeof(rambuffer), &rasize);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_buffer_init(&ramsg, rambuffer, rasize);
	isc_buffer_add(&ramsg, rasize);

	result = dns_test_getdata(TESTS_DIR "/testdata/dnstap/query.recursive",
				  qrmbuffer, sizeof(qrmbuffer), &qrsize);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_buffer_init(&qrmsg, qrmbuffer, qrsize);
	isc_buffer_add(&qrmsg, qrsize);

	result = dns_test_getdata(TESTS_DIR
				  "/testdata/dnstap/response.recursive",
				  rrmbuffer, sizeof(rrmbuffer), &rrsize);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_buffer_init(&rrmsg, rrmbuffer, rrsize);
	isc_buffer_add(&rrmsg, rrsize);

	for (dt = DNS_DTTYPE_SQ; dt <= DNS_DTTYPE_TR; dt <<= 1) {
		isc_buffer_t *m;
		isc_sockaddr_t *q = &qaddr, *r = &raddr;

		switch (dt) {
		case DNS_DTTYPE_AQ:
			m = &qamsg;
			break;
		case DNS_DTTYPE_AR:
			m = &ramsg;
			break;
		default:
			m = &qrmsg;
			if ((dt & DNS_DTTYPE_RESPONSE) != 0) {
				m = &ramsg;
			}
			break;
		}

		dns_dt_send(view, dt, q, r, false, &zr, &p, &f, m);
		dns_dt_send(view, dt, q, r, false, &zr, NULL, &f, m);
		dns_dt_send(view, dt, q, r, false, &zr, &p, NULL, m);
		dns_dt_send(view, dt, q, r, false, &zr, NULL, NULL, m);
		dns_dt_send(view, dt, q, r, true, &zr, &p, &f, m);
		dns_dt_send(view, dt, q, r, true, &zr, NULL, &f, m);
		dns_dt_send(view, dt, q, r, true, &zr, &p, NULL, m);
		dns_dt_send(view, dt, q, r, true, &zr, NULL, NULL, m);
	}

	dns_dt_detach(&view->dtenv);
	dns_dt_detach(&dtenv);
	dns_view_detach(&view);

	result = dns_dt_open(TAPFILE, dns_dtmode_file, mctx, &handle);
	assert_int_equal(result, ISC_R_SUCCESS);

	while (dns_dt_getframe(handle, &data, &dsize) == ISC_R_SUCCESS) {
		dns_dtdata_t *dtdata = NULL;
		isc_region_t r;
		static dns_dtmsgtype_t expected = DNS_DTTYPE_SQ;
		static int n = 0;

		r.base = data;
		r.length = dsize;

		result = dns_dt_parse(mctx, &r, &dtdata);
		assert_int_equal(result, ISC_R_SUCCESS);
		if (result != ISC_R_SUCCESS) {
			n++;
			continue;
		}

		assert_int_equal(dtdata->type, expected);
		if (++n % 8 == 0) {
			expected <<= 1;
		}

		dns_dtdata_free(&dtdata);
	}

	if (fopt != NULL) {
		fstrm_iothr_options_destroy(&fopt);
	}
	if (handle != NULL) {
		dns_dt_close(&handle);
	}
}

/* dnstap message to text */
ISC_RUN_TEST_IMPL(dns_dt_totext) {
	isc_result_t result;
	dns_dthandle_t *handle = NULL;
	uint8_t *data;
	size_t dsize;
	FILE *fp = NULL;

	UNUSED(state);

	result = dns_dt_open(TAPSAVED, dns_dtmode_file, mctx, &handle);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_stdio_open(TAPTEXT, "r", &fp);
	assert_int_equal(result, ISC_R_SUCCESS);

	while (dns_dt_getframe(handle, &data, &dsize) == ISC_R_SUCCESS) {
		dns_dtdata_t *dtdata = NULL;
		isc_buffer_t *b = NULL;
		isc_region_t r;
		char s[BUFSIZ], *p;

		r.base = data;
		r.length = dsize;

		/* read the corresponding line of text */
		p = fgets(s, sizeof(s), fp);
		assert_ptr_equal(p, s);
		if (p == NULL) {
			break;
		}

		p = strchr(p, '\n');
		if (p != NULL) {
			*p = '\0';
		}

		/* parse dnstap frame */
		result = dns_dt_parse(mctx, &r, &dtdata);
		assert_int_equal(result, ISC_R_SUCCESS);
		if (result != ISC_R_SUCCESS) {
			continue;
		}

		isc_buffer_allocate(mctx, &b, 2048);
		assert_non_null(b);
		if (b == NULL) {
			break;
		}

		/* convert to text and compare */
		result = dns_dt_datatotext(dtdata, &b);
		assert_int_equal(result, ISC_R_SUCCESS);

		assert_string_equal((char *)isc_buffer_base(b), s);

		dns_dtdata_free(&dtdata);
		isc_buffer_free(&b);
	}

	if (handle != NULL) {
		dns_dt_close(&handle);
	}
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY_CUSTOM(dns_dt_create, setup, cleanup)
ISC_TEST_ENTRY_CUSTOM(dns_dt_send, setup, cleanup)
ISC_TEST_ENTRY_CUSTOM(dns_dt_totext, setup, cleanup)

ISC_TEST_LIST_END

ISC_TEST_MAIN
