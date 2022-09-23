/*	$NetBSD: dnstap_test.c,v 1.10 2022/09/23 12:15:32 christos Exp $	*/

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

#if HAVE_CMOCKA

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

#include <isc/buffer.h>
#include <isc/file.h>
#include <isc/print.h>
#include <isc/stdio.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/dnstap.h>
#include <dns/view.h>

#include "dnstest.h"

#ifdef HAVE_DNSTAP

#include <fstrm.h>

#include <protobuf-c/protobuf-c.h>

#define TAPFILE "testdata/dnstap/dnstap.file"
#define TAPSOCK "testdata/dnstap/dnstap.sock"

#define TAPSAVED "testdata/dnstap/dnstap.saved"
#define TAPTEXT	 "testdata/dnstap/dnstap.text"

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = dns_test_begin(NULL, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	dns_test_end();

	return (0);
}

static void
cleanup() {
	(void)isc_file_remove(TAPFILE);
	(void)isc_file_remove(TAPSOCK);
}

/* set up dnstap environment */
static void
create_test(void **state) {
	isc_result_t result;
	dns_dtenv_t *dtenv = NULL;
	struct fstrm_iothr_options *fopt;

	UNUSED(state);

	cleanup();

	fopt = fstrm_iothr_options_init();
	assert_non_null(fopt);
	fstrm_iothr_options_set_num_input_queues(fopt, 1);

	result = dns_dt_create(dt_mctx, dns_dtmode_file, TAPFILE, &fopt, NULL,
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

	result = dns_dt_create(dt_mctx, dns_dtmode_unix, TAPSOCK, &fopt, NULL,
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

	result = dns_dt_create(dt_mctx, 33, TAPSOCK, &fopt, NULL, &dtenv);
	assert_int_equal(result, ISC_R_FAILURE);
	assert_null(dtenv);
	if (dtenv != NULL) {
		dns_dt_detach(&dtenv);
	}
	if (fopt != NULL) {
		fstrm_iothr_options_destroy(&fopt);
	}

	cleanup();
}

/* send dnstap messages */
static void
send_test(void **state) {
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

	UNUSED(state);

	cleanup();

	result = dns_test_makeview("test", &view);
	assert_int_equal(result, ISC_R_SUCCESS);

	fopt = fstrm_iothr_options_init();
	assert_non_null(fopt);
	fstrm_iothr_options_set_num_input_queues(fopt, 1);

	result = dns_dt_create(dt_mctx, dns_dtmode_file, TAPFILE, &fopt, NULL,
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
	result = dns_compress_init(&cctx, -1, dt_mctx);
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

	result = dns_test_getdata("testdata/dnstap/query.auth", qambuffer,
				  sizeof(qambuffer), &qasize);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_buffer_init(&qamsg, qambuffer, qasize);
	isc_buffer_add(&qamsg, qasize);

	result = dns_test_getdata("testdata/dnstap/response.auth", rambuffer,
				  sizeof(rambuffer), &rasize);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_buffer_init(&ramsg, rambuffer, rasize);
	isc_buffer_add(&ramsg, rasize);

	result = dns_test_getdata("testdata/dnstap/query.recursive", qrmbuffer,
				  sizeof(qrmbuffer), &qrsize);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_buffer_init(&qrmsg, qrmbuffer, qrsize);
	isc_buffer_add(&qrmsg, qrsize);

	result = dns_test_getdata("testdata/dnstap/response.recursive",
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

	result = dns_dt_open(TAPFILE, dns_dtmode_file, dt_mctx, &handle);
	assert_int_equal(result, ISC_R_SUCCESS);

	while (dns_dt_getframe(handle, &data, &dsize) == ISC_R_SUCCESS) {
		dns_dtdata_t *dtdata = NULL;
		isc_region_t r;
		static dns_dtmsgtype_t expected = DNS_DTTYPE_SQ;
		static int n = 0;

		r.base = data;
		r.length = dsize;

		result = dns_dt_parse(dt_mctx, &r, &dtdata);
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
	cleanup();
}

/* dnstap message to text */
static void
totext_test(void **state) {
	isc_result_t result;
	dns_dthandle_t *handle = NULL;
	uint8_t *data;
	size_t dsize;
	FILE *fp = NULL;

	UNUSED(state);

	result = dns_dt_open(TAPSAVED, dns_dtmode_file, dt_mctx, &handle);
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
		result = dns_dt_parse(dt_mctx, &r, &dtdata);
		assert_int_equal(result, ISC_R_SUCCESS);
		if (result != ISC_R_SUCCESS) {
			continue;
		}

		isc_buffer_allocate(dt_mctx, &b, 2048);
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
	cleanup();
}
#endif /* HAVE_DNSTAP */

int
main(void) {
#if HAVE_DNSTAP
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(create_test, _setup, _teardown),
		cmocka_unit_test_setup_teardown(send_test, _setup, _teardown),
		cmocka_unit_test_setup_teardown(totext_test, _setup, _teardown),
	};

	/* make sure text conversion gets the right local time */
	setenv("TZ", "PST8", 1);

	return (cmocka_run_group_tests(tests, NULL, NULL));
#else  /* if HAVE_DNSTAP */
	print_message("1..0 # Skipped: dnstap not enabled\n");
	return (SKIPPED_TEST_EXIT_CODE);
#endif /* HAVE_DNSTAP */
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (SKIPPED_TEST_EXIT_CODE);
}

#endif /* HAVE_CMOCKA */
