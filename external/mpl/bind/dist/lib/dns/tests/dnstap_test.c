/*	$NetBSD: dnstap_test.c,v 1.2 2018/08/12 13:02:36 christos Exp $	*/

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

#include <atf-c.h>

#include <unistd.h>
#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/file.h>
#include <isc/stdio.h>
#include <isc/print.h>
#include <isc/types.h>

#include <dns/dnstap.h>
#include <dns/view.h>

#include "dnstest.h"

#ifdef HAVE_DNSTAP
#include <dns/dnstap.pb-c.h>
#include <protobuf-c/protobuf-c.h>

#define TAPFILE "testdata/dnstap/dnstap.file"
#define TAPSOCK "testdata/dnstap/dnstap.sock"

#define TAPSAVED "testdata/dnstap/dnstap.saved"
#define TAPTEXT "testdata/dnstap/dnstap.text"

/*
 * Helper functions
 */
static void
cleanup() {
	(void) isc_file_remove(TAPFILE);
	(void) isc_file_remove(TAPSOCK);
}

/*
 * Individual unit tests
 */

ATF_TC(create);
ATF_TC_HEAD(create, tc) {
	atf_tc_set_md_var(tc, "descr", "set up dnstap environment");
}
ATF_TC_BODY(create, tc) {
	isc_result_t result;
	dns_dtenv_t *dtenv = NULL;
	struct fstrm_iothr_options *fopt;

	UNUSED(tc);

	cleanup();

	result = dns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE(result == ISC_R_SUCCESS);

	fopt = fstrm_iothr_options_init();
	ATF_REQUIRE(fopt != NULL);
	fstrm_iothr_options_set_num_input_queues(fopt, 1);

	result = dns_dt_create(mctx, dns_dtmode_file, TAPFILE, &fopt, &dtenv);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	if (dtenv != NULL)
		dns_dt_detach(&dtenv);
	if (fopt != NULL)
		fstrm_iothr_options_destroy(&fopt);

	ATF_CHECK(isc_file_exists(TAPFILE));

	fopt = fstrm_iothr_options_init();
	ATF_REQUIRE(fopt != NULL);
	fstrm_iothr_options_set_num_input_queues(fopt, 1);

	result = dns_dt_create(mctx, dns_dtmode_unix, TAPSOCK, &fopt, &dtenv);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	if (dtenv != NULL)
		dns_dt_detach(&dtenv);
	if (fopt != NULL)
		fstrm_iothr_options_destroy(&fopt);

	/* 'create' should succeed, but the file shouldn't exist yet */
	ATF_CHECK(!isc_file_exists(TAPSOCK));

	fopt = fstrm_iothr_options_init();
	ATF_REQUIRE(fopt != NULL);
	fstrm_iothr_options_set_num_input_queues(fopt, 1);

	result = dns_dt_create(mctx, 33, TAPSOCK, &fopt, &dtenv);
	ATF_CHECK_EQ(result, ISC_R_FAILURE);
	ATF_CHECK_EQ(dtenv, NULL);
	if (dtenv != NULL)
		dns_dt_detach(&dtenv);
	if (fopt != NULL)
		fstrm_iothr_options_destroy(&fopt);

	cleanup();

	dns_dt_shutdown();
	dns_test_end();
}

ATF_TC(send);
ATF_TC_HEAD(send, tc) {
	atf_tc_set_md_var(tc, "descr", "send dnstap messages");
}
ATF_TC_BODY(send, tc) {
	isc_result_t result;
	dns_dtenv_t *dtenv = NULL;
	dns_dthandle_t *handle = NULL;
	isc_uint8_t *data;
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

	UNUSED(tc);

	cleanup();

	result = dns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE(result == ISC_R_SUCCESS);

	result = dns_test_makeview("test", &view);

	fopt = fstrm_iothr_options_init();
	ATF_REQUIRE(fopt != NULL);
	fstrm_iothr_options_set_num_input_queues(fopt, 1);

	result = dns_dt_create(mctx, dns_dtmode_file, TAPFILE, &fopt, &dtenv);
	ATF_REQUIRE(result == ISC_R_SUCCESS);

	dns_dt_attach(dtenv, &view->dtenv);
	view->dttypes = DNS_DTTYPE_ALL;

	/*
	 * Set up some test data
	 */
	zname = dns_fixedname_initname(&zfname);
	isc_buffer_constinit(&zb, "example.com.", 12);
	isc_buffer_add(&zb, 12);
	result = dns_name_fromtext(zname, &zb, NULL, 0, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	memset(&zr, 0, sizeof(zr));
	isc_buffer_init(&zb, zone, sizeof(zone));
	result = dns_compress_init(&cctx, -1, mctx);
	dns_compress_setmethods(&cctx, DNS_COMPRESS_NONE);
	result = dns_name_towire(zname, &cctx, &zb);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	dns_compress_invalidate(&cctx);
	isc_buffer_usedregion(&zb, &zr);

	in.s_addr = inet_addr("10.53.0.1");
	isc_sockaddr_fromin(&qaddr, &in, 2112);
	in.s_addr = inet_addr("10.53.0.2");
	isc_sockaddr_fromin(&raddr, &in, 2112);

	isc_stdtime_get(&now);
	isc_time_set(&p, now - 3600, 0); /* past */
	isc_time_set(&f, now + 3600, 0); /* future */

	result = dns_test_getdata("testdata/dnstap/query.auth",
				  qambuffer, sizeof(qambuffer), &qasize);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	isc_buffer_init(&qamsg, qambuffer, qasize);
	isc_buffer_add(&qamsg, qasize);

	result = dns_test_getdata("testdata/dnstap/response.auth",
				  rambuffer, sizeof(rambuffer), &rasize);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	isc_buffer_init(&ramsg, rambuffer, rasize);
	isc_buffer_add(&ramsg, rasize);

	result = dns_test_getdata("testdata/dnstap/query.recursive", qrmbuffer,
				  sizeof(qrmbuffer), &qrsize);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	isc_buffer_init(&qrmsg, qrmbuffer, qrsize);
	isc_buffer_add(&qrmsg, qrsize);

	result = dns_test_getdata("testdata/dnstap/response.recursive",
				  rrmbuffer, sizeof(rrmbuffer), &rrsize);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
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
			if ((dt & DNS_DTTYPE_RESPONSE) != 0)
				m = &ramsg;
			break;
		}

		dns_dt_send(view, dt, q, r, ISC_FALSE, &zr, &p, &f, m);
		dns_dt_send(view, dt, q, r, ISC_FALSE, &zr, NULL, &f, m);
		dns_dt_send(view, dt, q, r, ISC_FALSE, &zr, &p, NULL, m);
		dns_dt_send(view, dt, q, r, ISC_FALSE, &zr, NULL, NULL, m);
		dns_dt_send(view, dt, q, r, ISC_TRUE, &zr, &p, &f, m);
		dns_dt_send(view, dt, q, r, ISC_TRUE, &zr, NULL, &f, m);
		dns_dt_send(view, dt, q, r, ISC_TRUE, &zr, &p, NULL, m);
		dns_dt_send(view, dt, q, r, ISC_TRUE, &zr, NULL, NULL, m);
	}

	dns_dt_detach(&view->dtenv);
	dns_dt_detach(&dtenv);
	dns_dt_shutdown();
	dns_view_detach(&view);

	/*
	 * XXX now read back and check content.
	 */

	result = dns_dt_open(TAPFILE, dns_dtmode_file, mctx, &handle);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	while (dns_dt_getframe(handle, &data, &dsize) == ISC_R_SUCCESS) {
		dns_dtdata_t *dtdata = NULL;
		isc_region_t r;
		static dns_dtmsgtype_t expected = DNS_DTTYPE_SQ;
		static int n = 0;

		r.base = data;
		r.length = dsize;

		result = dns_dt_parse(mctx, &r, &dtdata);
		ATF_CHECK_EQ(result, ISC_R_SUCCESS);
		if (result != ISC_R_SUCCESS) {
			n++;
			continue;
		}

		ATF_CHECK_EQ(dtdata->type, expected);
		if (++n % 8 == 0)
			expected <<= 1;

		dns_dtdata_free(&dtdata);
	}

	if (fopt != NULL)
		fstrm_iothr_options_destroy(&fopt);
	if (handle != NULL)
		dns_dt_close(&handle);
	cleanup();

	dns_test_end();
}

ATF_TC(totext);
ATF_TC_HEAD(totext, tc) {
	atf_tc_set_md_var(tc, "descr", "dnstap message to text");
}
ATF_TC_BODY(totext, tc) {
	isc_result_t result;
	dns_dthandle_t *handle = NULL;
	isc_uint8_t *data;
	size_t dsize;
	FILE *fp = NULL;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_TRUE);
	ATF_REQUIRE(result == ISC_R_SUCCESS);

	result = dns_dt_open(TAPSAVED, dns_dtmode_file, mctx, &handle);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_stdio_open(TAPTEXT, "r", &fp);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* make sure text conversion gets the right local time */
	setenv("TZ", "PST8", 1);

	while (dns_dt_getframe(handle, &data, &dsize) == ISC_R_SUCCESS) {
		dns_dtdata_t *dtdata = NULL;
		isc_buffer_t *b = NULL;
		isc_region_t r;
		char s[BUFSIZ], *p;

		r.base = data;
		r.length = dsize;

		/* read the corresponding line of text */
		p = fgets(s, sizeof(s), fp);
		ATF_CHECK_EQ(p, s);
		if (p == NULL)
			break;

		p = strchr(p, '\n');
		if (p != NULL)
			*p = '\0';

		/* parse dnstap frame */
		result = dns_dt_parse(mctx, &r, &dtdata);
		ATF_CHECK_EQ(result, ISC_R_SUCCESS);
		if (result != ISC_R_SUCCESS)
			continue;

		isc_buffer_allocate(mctx, &b, 2048);
		ATF_CHECK(b != NULL);
		if (b == NULL)
			break;

		/* convert to text and compare */
		result = dns_dt_datatotext(dtdata, &b);
		ATF_CHECK_EQ(result, ISC_R_SUCCESS);

		ATF_CHECK_STREQ((char *) isc_buffer_base(b), s);

		dns_dtdata_free(&dtdata);
		isc_buffer_free(&b);
	}

	if (handle != NULL)
		dns_dt_close(&handle);
	cleanup();

	dns_test_end();
}

#else
ATF_TC(untested);
ATF_TC_HEAD(untested, tc) {
	atf_tc_set_md_var(tc, "descr", "skipping dnstap test");
}
ATF_TC_BODY(untested, tc) {
	UNUSED(tc);
	atf_tc_skip("dnstap not available");
}
#endif

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
#ifdef HAVE_DNSTAP
	ATF_TP_ADD_TC(tp, create);
	ATF_TP_ADD_TC(tp, send);
	ATF_TP_ADD_TC(tp, totext);
#else
	ATF_TP_ADD_TC(tp, untested);
#endif

	return (atf_no_error());
}
