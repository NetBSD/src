/*	$NetBSD: dnsstream_utils_test.c,v 1.2 2025/01/26 16:25:49 christos Exp $	*/

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
#include <time.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/dnsstream.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/random.h>

#include "dnsstream_utils_test_data.h"

#include <tests/isc.h>

#define STATIC_BUFFER_SIZE  (512)
#define DYNAMIC_BUFFER_SIZE (STATIC_BUFFER_SIZE + ISC_BUFFER_INCR)

static int
setup_test_dnsbuf(void **state) {
	isc_buffer_t **pdnsbuf = (isc_buffer_t **)state;
	isc_buffer_allocate(mctx, pdnsbuf, STATIC_BUFFER_SIZE);

	return 0;
}

static int
teardown_test_dnsbuf(void **state) {
	isc_buffer_free((isc_buffer_t **)state);

	return 0;
}

static bool
dnsasm_dummy(isc_dnsstream_assembler_t *dnsasm, const isc_result_t result,
	     isc_region_t *restrict region, void *cbarg, void *userarg) {
	UNUSED(dnsasm);
	UNUSED(result);
	UNUSED(region);
	UNUSED(cbarg);
	UNUSED(userarg);
	return true;
}

static int
setup_test_dnsasm(void **state) {
	isc_dnsstream_assembler_t **pdnsasm =
		(isc_dnsstream_assembler_t **)state;
	*pdnsasm = isc_dnsstream_assembler_new(mctx, dnsasm_dummy, NULL);

	return 0;
}

static int
teardown_test_dnsasm(void **state) {
	isc_dnsstream_assembler_free((isc_dnsstream_assembler_t **)state);

	return 0;
}

ISC_RUN_TEST_IMPL(dnsbuffer_generic_test) {
	uint8_t buf[STATIC_BUFFER_SIZE / 2] = { 0 };
	isc_buffer_t *dnsbuf = (isc_buffer_t *)*state;
	isc_region_t reg = { 0 };
	size_t n = 0;

	for (size_t i = 0; i < sizeof(buf); i++) {
		buf[i] = (uint8_t)i;
	}

	/* sanity checks */
	assert_true(isc_buffer_length(dnsbuf) == STATIC_BUFFER_SIZE);
	assert_true(isc_buffer_usedlength(dnsbuf) == 0);
	assert_true(isc_buffer_remaininglength(dnsbuf) == 0);

	isc_buffer_putmem(dnsbuf, buf, sizeof(buf));

	assert_true(isc_buffer_usedlength(dnsbuf) == sizeof(buf));
	assert_true(isc_buffer_remaininglength(dnsbuf) == sizeof(buf));

	assert_true(isc_buffer_current(dnsbuf) == dnsbuf->base);

	isc_buffer_clear(dnsbuf);

	assert_true(isc_buffer_length(dnsbuf) == STATIC_BUFFER_SIZE);
	assert_true(isc_buffer_usedlength(dnsbuf) == 0);
	assert_true(isc_buffer_remaininglength(dnsbuf) == 0);

	isc_buffer_putmem(dnsbuf, buf, sizeof(buf));

	assert_true(isc_buffer_usedlength(dnsbuf) == sizeof(buf));
	assert_true(isc_buffer_remaininglength(dnsbuf) == sizeof(buf));

	assert_true(isc_buffer_current(dnsbuf) == dnsbuf->base);

	for (size_t i = 0; i < sizeof(buf); i++, isc_buffer_forward(dnsbuf, 1))
	{
		uint8_t *p = isc_buffer_current(dnsbuf);

		assert_true(*p == i);
	}

	assert_true(isc_buffer_remaininglength(dnsbuf) == 0);
	assert_true(isc_buffer_usedlength(dnsbuf) == sizeof(buf));

	isc_buffer_putmem(dnsbuf, buf, sizeof(buf));

	assert_true(isc_buffer_remaininglength(dnsbuf) == sizeof(buf));
	assert_true(isc_buffer_usedlength(dnsbuf) == sizeof(buf) * 2);

	assert_true(isc_buffer_length(dnsbuf) == STATIC_BUFFER_SIZE);

	for (size_t i = 0; i < sizeof(buf); i++, isc_buffer_forward(dnsbuf, 1))
	{
		uint8_t *p = isc_buffer_current(dnsbuf);

		assert_true(*p == i);
	}

	isc_buffer_putmem(dnsbuf, buf, sizeof(buf));

	assert_true(isc_buffer_length(dnsbuf) == DYNAMIC_BUFFER_SIZE);

	for (size_t i = 0; i < sizeof(buf); i++, isc_buffer_forward(dnsbuf, 1))
	{
		uint8_t *p = isc_buffer_current(dnsbuf);

		assert_true(*p == i);
	}

	assert_true(isc_buffer_remaininglength(dnsbuf) == 0);
	isc_buffer_trycompact(dnsbuf);

	assert_true(isc_buffer_length(dnsbuf) == DYNAMIC_BUFFER_SIZE);

	isc_buffer_putmem(dnsbuf, buf, sizeof(buf));

	isc_buffer_remainingregion(dnsbuf, &reg);
	assert_true(isc_buffer_remaininglength(dnsbuf) == reg.length);
	assert_true(reg.length == sizeof(buf));

	for (size_t i = 0; i < reg.length; i++) {
		uint8_t d = (uint8_t)reg.base[i];

		assert_true(d == i);
	}

	isc_buffer_forward(dnsbuf, reg.length);
	assert_true(isc_buffer_remaininglength(dnsbuf) == 0);

	isc_buffer_putmem(dnsbuf, buf, sizeof(buf));
	assert_true(isc_buffer_remaininglength(dnsbuf) == sizeof(buf));

	isc_buffer_clear(dnsbuf);

	assert_true(isc_buffer_length(dnsbuf) == DYNAMIC_BUFFER_SIZE);

	assert_true(isc_buffer_remaininglength(dnsbuf) == 0);

	n = DYNAMIC_BUFFER_SIZE / sizeof(buf) + 1;
	for (size_t i = 0; i < n; i++) {
		isc_buffer_putmem(dnsbuf, buf, sizeof(buf));
	}

	assert_true(isc_buffer_length(dnsbuf) > DYNAMIC_BUFFER_SIZE);
	assert_true(isc_buffer_length(dnsbuf) >= n * sizeof(buf));

	assert_true(isc_buffer_remaininglength(dnsbuf) == n * sizeof(buf));

	for (size_t i = 0; i < n; i++) {
		for (size_t k = 0; k < sizeof(buf);
		     k++, isc_buffer_forward(dnsbuf, 1))
		{
			uint8_t *p = isc_buffer_current(dnsbuf);

			assert_true(*p == k);
		}
	}
}

ISC_RUN_TEST_IMPL(dnsbuffer_resize_alloc_test) {
	uint8_t buf[STATIC_BUFFER_SIZE / 2] = { 0 };
	isc_buffer_t *dnsbuf = (isc_buffer_t *)*state;
	size_t i = 0, n = 0;

	for (i = 0; i < sizeof(buf); i++) {
		buf[i] = (uint8_t)i;
	}

	isc_buffer_putmem(dnsbuf, buf, sizeof(buf));

	for (i = 0; i < (sizeof(buf) / 3) * 2;
	     i++, isc_buffer_forward(dnsbuf, 1))
	{
		uint8_t *p = isc_buffer_current(dnsbuf);

		assert_true(*p == i);
	}

	assert_true(isc_buffer_length(dnsbuf) == STATIC_BUFFER_SIZE);

	n = DYNAMIC_BUFFER_SIZE / sizeof(buf) + 1;
	for (size_t k = 0; k < n; k++) {
		isc_buffer_putmem(dnsbuf, buf, sizeof(buf));
	}

	assert_true(isc_buffer_length(dnsbuf) >= STATIC_BUFFER_SIZE);

	for (; i < sizeof(buf); i++, isc_buffer_forward(dnsbuf, 1)) {
		uint8_t *p = isc_buffer_current(dnsbuf);

		assert_true(*p == i);
	}
}

ISC_RUN_TEST_IMPL(dnsbuffer_be_test) {
	isc_buffer_t *dnsbuf = (isc_buffer_t *)*state;
	const uint16_t u16 = 0xBEEF;
	uint16_t *pu16;
	uint16_t u16v;

	assert_true(isc_buffer_remaininglength(dnsbuf) == 0);

	isc_buffer_putuint16(dnsbuf, u16);

	assert_true(isc_buffer_remaininglength(dnsbuf) == sizeof(u16));

	pu16 = (uint16_t *)isc_buffer_current(dnsbuf);
	assert_true(*pu16 == htons(u16));

	assert_int_equal(isc_buffer_peekuint16(dnsbuf, &u16v), ISC_R_SUCCESS);
	assert_int_equal(u16v, u16);
	assert_true(isc_buffer_remaininglength(dnsbuf) == sizeof(u16));

	assert_true(isc_buffer_getuint16(dnsbuf) == u16);
	assert_true(isc_buffer_remaininglength(dnsbuf) == 0);
}

typedef struct verify_cbdata {
	uint8_t *verify_message;
	bool cont_on_success;
	bool clear_on_success;
} verify_cbdata_t;

static bool
verify_dnsmsg(isc_dnsstream_assembler_t *dnsasm, const isc_result_t result,
	      isc_region_t *restrict region, void *cbarg, void *userarg) {
	size_t *processed = (size_t *)userarg;
	verify_cbdata_t *vdata = (verify_cbdata_t *)cbarg;
	uint8_t *message = (uint8_t *)vdata->verify_message;

	UNUSED(dnsasm);

	assert_true(result == isc_dnsstream_assembler_result(dnsasm));

	if (vdata->verify_message != NULL) {
		message += sizeof(uint16_t);
	}

	if (result != ISC_R_SUCCESS) {
		return true;
	}

	if (vdata->verify_message != NULL &&
	    memcmp(message, region->base, region->length) == 0)
	{
		*processed += 1;
	} else {
		*processed += 1;
	}

	if (vdata->clear_on_success) {
		isc_dnsstream_assembler_clear(
			(isc_dnsstream_assembler_t *)dnsasm);
	}

	return vdata->cont_on_success;
}

typedef struct verify_regions_cbdata {
	isc_region_t *packets;
	bool cont_on_success;
} verify_regions_cbdata_t;

static bool
verify_dnsmsg_regions(isc_dnsstream_assembler_t *dnsasm,
		      const isc_result_t result, isc_region_t *restrict region,
		      void *cbarg, void *userarg) {
	size_t *processed = (size_t *)userarg;
	verify_regions_cbdata_t *vdata = (verify_regions_cbdata_t *)cbarg;
	uint8_t *message = (uint8_t *)vdata->packets[0].base;

	UNUSED(dnsasm);

	assert_true(result == isc_dnsstream_assembler_result(dnsasm));

	if (vdata->packets != NULL) {
		message += sizeof(uint16_t);
	}

	if (result != ISC_R_SUCCESS) {
		return true;
	}

	if (vdata->packets != NULL &&
	    memcmp(message, region->base, region->length) == 0)
	{
		*processed += 1;
	} else {
		*processed += 1;
	}

	vdata->packets++;

	return vdata->cont_on_success;
}

ISC_RUN_TEST_IMPL(dnsasm_sequence_test) {
	isc_dnsstream_assembler_t *dnsasm = (isc_dnsstream_assembler_t *)*state;
	verify_cbdata_t cbdata = { 0 };
	size_t verified = 0;

	cbdata.cont_on_success = true;

	cbdata.verify_message = (uint8_t *)request;
	isc_dnsstream_assembler_setcb(dnsasm, verify_dnsmsg, (void *)&cbdata);
	isc_dnsstream_assembler_incoming(dnsasm, &verified, (void *)request,
					 sizeof(request));
	assert_true(verified == 1);
	assert_true(isc_dnsstream_assembler_result(dnsasm) == ISC_R_SUCCESS);

	cbdata.verify_message = (uint8_t *)response;
	isc_dnsstream_assembler_setcb(dnsasm, verify_dnsmsg, (void *)&cbdata);
	isc_dnsstream_assembler_incoming(dnsasm, &verified, (void *)response,
					 sizeof(response));
	assert_true(verified == 2);
	assert_true(isc_dnsstream_assembler_result(dnsasm) == ISC_R_SUCCESS);

	cbdata.verify_message = (uint8_t *)request_large;
	isc_dnsstream_assembler_setcb(dnsasm, verify_dnsmsg, (void *)&cbdata);
	isc_dnsstream_assembler_incoming(dnsasm, &verified,
					 (void *)request_large,
					 sizeof(request_large));
	assert_true(verified == 3);
	assert_true(isc_dnsstream_assembler_result(dnsasm) == ISC_R_SUCCESS);

	cbdata.verify_message = (uint8_t *)response_large;
	isc_dnsstream_assembler_setcb(dnsasm, verify_dnsmsg, (void *)&cbdata);
	isc_dnsstream_assembler_incoming(dnsasm, &verified,
					 (void *)response_large,
					 sizeof(response_large));
	assert_true(verified == 4);
	assert_true(isc_dnsstream_assembler_result(dnsasm) == ISC_R_SUCCESS);
}

ISC_RUN_TEST_IMPL(dnsasm_multiple_messages_test) {
	isc_dnsstream_assembler_t *dnsasm = (isc_dnsstream_assembler_t *)*state;
	isc_buffer_t dnsbuf;
	verify_cbdata_t cbdata = { 0 };
	size_t verified = 0;

	isc_buffer_init(&dnsbuf, NULL, 0);
	isc_buffer_setmctx(&dnsbuf, mctx);
	isc_buffer_putmem(&dnsbuf, (void *)request, sizeof(request));
	isc_buffer_putmem(&dnsbuf, (void *)response, sizeof(response));
	isc_buffer_putmem(&dnsbuf, (void *)request_large,
			  sizeof(request_large));
	isc_buffer_putmem(&dnsbuf, (void *)response_large,
			  sizeof(response_large));

	cbdata.cont_on_success = false;

	/*
	 * feed the data to the message assembler and handle the first message
	 */
	cbdata.verify_message = (uint8_t *)request;
	isc_dnsstream_assembler_setcb(dnsasm, verify_dnsmsg, (void *)&cbdata);
	isc_dnsstream_assembler_incoming(dnsasm, &verified,
					 isc_buffer_current(&dnsbuf),
					 isc_buffer_remaininglength(&dnsbuf));

	isc_buffer_clearmctx(&dnsbuf);
	isc_buffer_invalidate(&dnsbuf);
	assert_true(verified == 1);
	assert_true(isc_dnsstream_assembler_result(dnsasm) == ISC_R_SUCCESS);

	/*
	 * handle the next message (and so on)
	 */
	cbdata.verify_message = (uint8_t *)response;
	isc_dnsstream_assembler_incoming(dnsasm, &verified, NULL, 0);
	assert_true(verified == 2);
	assert_true(isc_dnsstream_assembler_result(dnsasm) == ISC_R_SUCCESS);

	cbdata.verify_message = (uint8_t *)request_large;
	isc_dnsstream_assembler_incoming(dnsasm, &verified, NULL, 0);
	assert_true(verified == 3);
	assert_true(isc_dnsstream_assembler_result(dnsasm) == ISC_R_SUCCESS);

	cbdata.verify_message = (uint8_t *)response_large;
	isc_dnsstream_assembler_incoming(dnsasm, &verified, NULL, 0);
	assert_true(verified == 4);
	assert_true(isc_dnsstream_assembler_result(dnsasm) == ISC_R_SUCCESS);

	/*
	 * no unprocessed data left
	 */
	isc_dnsstream_assembler_incoming(dnsasm, &verified, NULL, 0);
	assert_true(verified == 4);
	assert_true(isc_dnsstream_assembler_result(dnsasm) == ISC_R_NOMORE);
}

ISC_RUN_TEST_IMPL(dnsasm_torn_apart_test) {
	isc_dnsstream_assembler_t *dnsasm = (isc_dnsstream_assembler_t *)*state;
	verify_cbdata_t cbdata = { 0 };
	size_t verified = 0;
	size_t left = 0;

	cbdata.verify_message = (uint8_t *)response_large;
	isc_dnsstream_assembler_setcb(dnsasm, verify_dnsmsg, (void *)&cbdata);
	isc_dnsstream_assembler_incoming(dnsasm, &verified, response_large,
					 sizeof(response_large) / 3 * 2);

	assert_true(verified == 0);
	assert_true(isc_dnsstream_assembler_result(dnsasm) == ISC_R_NOMORE);

	left = sizeof(response_large) -
	       isc_dnsstream_assembler_remaininglength(dnsasm);
	isc_dnsstream_assembler_incoming(
		dnsasm, &verified,
		&response_large[isc_dnsstream_assembler_remaininglength(dnsasm)],
		left);
	assert_true(verified == 1);
	assert_true(isc_dnsstream_assembler_result(dnsasm) == ISC_R_SUCCESS);
}

ISC_RUN_TEST_IMPL(dnsasm_error_data_test) {
	isc_dnsstream_assembler_t *dnsasm = (isc_dnsstream_assembler_t *)*state;
	verify_cbdata_t cbdata = { 0 };
	size_t verified = 0;
	isc_buffer_t dnsbuf;
	uint16_t bad_data = 0;

	isc_buffer_init(&dnsbuf, NULL, 0);
	isc_buffer_setmctx(&dnsbuf, mctx);

	isc_buffer_putmem(&dnsbuf, (void *)request, sizeof(request));
	isc_buffer_putmem(&dnsbuf, (void *)&bad_data, sizeof(bad_data));
	isc_buffer_putmem(&dnsbuf, (void *)&bad_data, sizeof(bad_data));
	isc_buffer_putmem(&dnsbuf, (void *)response_large,
			  sizeof(response_large));

	cbdata.cont_on_success = false;

	cbdata.verify_message = (uint8_t *)request;
	isc_dnsstream_assembler_setcb(dnsasm, verify_dnsmsg, (void *)&cbdata);
	isc_dnsstream_assembler_incoming(dnsasm, &verified,
					 isc_buffer_current(&dnsbuf),
					 isc_buffer_remaininglength(&dnsbuf));

	isc_buffer_clearmctx(&dnsbuf);
	isc_buffer_invalidate(&dnsbuf);

	assert_true(verified == 1);
	assert_true(isc_dnsstream_assembler_result(dnsasm) == ISC_R_SUCCESS);

	assert_true(isc_dnsstream_assembler_remaininglength(dnsasm) > 0);
	isc_dnsstream_assembler_incoming(dnsasm, &verified, NULL, 0);

	assert_true(verified == 1);
	assert_true(isc_dnsstream_assembler_result(dnsasm) == ISC_R_RANGE);
	assert_true(isc_dnsstream_assembler_remaininglength(dnsasm) == 0);

	isc_dnsstream_assembler_incoming(dnsasm, &verified, NULL, 0);
	assert_true(verified == 1);
	assert_true(isc_dnsstream_assembler_result(dnsasm) == ISC_R_NOMORE);
}

ISC_RUN_TEST_IMPL(dnsasm_torn_randomly_test) {
	isc_dnsstream_assembler_t *dnsasm = (isc_dnsstream_assembler_t *)*state;
	verify_cbdata_t cbdata = { 0 };
	verify_regions_cbdata_t cbdata_regions = { 0 };
	isc_buffer_t dnsbuf;
	size_t packetno;
	isc_region_t packets[] = {
		{ (void *)request, sizeof(request) },
		{ (void *)response, sizeof(response) },
		{ (void *)request_large, sizeof(request_large) },
		{ (void *)response_large, sizeof(response_large) },
		{ (void *)request, sizeof(request) },
		{ (void *)response_large, sizeof(response_large) },
		{ (void *)request_large, sizeof(request_large) },
		{ (void *)response_large, sizeof(response_large) },
		{ (void *)request, sizeof(request) },
	};
	const size_t npackets = sizeof(packets) / sizeof(packets[0]);

	isc_buffer_init(&dnsbuf, NULL, 0);
	isc_buffer_setmctx(&dnsbuf, mctx);

	for (size_t i = 0; i < npackets; i++) {
		isc_buffer_putmem(&dnsbuf, packets[i].base, packets[i].length);
	}

	/* process packet by packet */
	packetno = 0;
	cbdata.cont_on_success = false;
	isc_dnsstream_assembler_setcb(dnsasm, verify_dnsmsg, (void *)&cbdata);

	/* process random amount of data */
	for (; isc_buffer_remaininglength(&dnsbuf) > 0;) {
		size_t sz = 1 + isc_random_uniform(
					isc_buffer_remaininglength(&dnsbuf));

		for (bool start = true; packetno < npackets; start = false) {
			cbdata.verify_message =
				(uint8_t *)packets[packetno].base;

			if (start) {
				isc_dnsstream_assembler_incoming(
					dnsasm, &packetno,
					isc_buffer_current(&dnsbuf), sz);
			} else {
				isc_dnsstream_assembler_incoming(
					dnsasm, &packetno, NULL, 0);
			}

			if (isc_dnsstream_assembler_result(dnsasm) ==
			    ISC_R_NOMORE)
			{
				break;
			}
		}

		isc_buffer_forward(&dnsbuf, sz);
	}

	assert_true(packetno == npackets);
	assert_true(isc_dnsstream_assembler_remaininglength(dnsasm) == 0);
	assert_true(isc_buffer_remaininglength(&dnsbuf) == 0);

	for (size_t i = 0; i < npackets; i++) {
		isc_buffer_putmem(&dnsbuf, packets[i].base, packets[i].length);
	}

	/* try to process multiple packets at once, when possible */
	packetno = 0;
	cbdata_regions.cont_on_success = true;
	cbdata_regions.packets = packets;

	isc_dnsstream_assembler_setcb(dnsasm, verify_dnsmsg_regions,
				      (void *)&cbdata_regions);

	/* process random amount of data */
	for (; isc_buffer_remaininglength(&dnsbuf) > 0;) {
		size_t sz = 1 + isc_random_uniform(
					isc_buffer_remaininglength(&dnsbuf));

		isc_dnsstream_assembler_incoming(
			dnsasm, &packetno, isc_buffer_current(&dnsbuf), sz);

		isc_buffer_forward(&dnsbuf, sz);
	}

	assert_true(packetno == npackets);
	assert_true(isc_dnsstream_assembler_remaininglength(dnsasm) == 0);
	assert_true(isc_buffer_remaininglength(&dnsbuf) == 0);

	isc_buffer_clearmctx(&dnsbuf);
	isc_buffer_invalidate(&dnsbuf);
	dnsasm->cbarg = NULL; /* to make GCC happy about dangling pointers */
}

ISC_RUN_TEST_IMPL(dnsasm_clear_buffer_within_cb_test) {
	isc_dnsstream_assembler_t *dnsasm = (isc_dnsstream_assembler_t *)*state;
	verify_cbdata_t cbdata = { 0 };
	size_t verified = 0;
	isc_buffer_t dnsbuf;

	isc_buffer_init(&dnsbuf, NULL, 0);
	isc_buffer_setmctx(&dnsbuf, mctx);

	isc_buffer_putmem(&dnsbuf, (void *)request, sizeof(request));
	isc_buffer_putmem(&dnsbuf, (void *)&response, sizeof(response));
	isc_buffer_putmem(&dnsbuf, (void *)request, sizeof(request));
	cbdata.cont_on_success = true;
	cbdata.clear_on_success = true;

	cbdata.verify_message = (uint8_t *)request;
	isc_dnsstream_assembler_setcb(dnsasm, verify_dnsmsg, (void *)&cbdata);
	isc_dnsstream_assembler_incoming(dnsasm, &verified,
					 isc_buffer_current(&dnsbuf),
					 isc_buffer_remaininglength(&dnsbuf));

	isc_buffer_clearmctx(&dnsbuf);
	isc_buffer_invalidate(&dnsbuf);

	assert_true(verified == 1);
	assert_true(isc_dnsstream_assembler_result(dnsasm) == ISC_R_UNSET);

	assert_true(isc_dnsstream_assembler_remaininglength(dnsasm) == 0);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(dnsbuffer_generic_test, setup_test_dnsbuf,
		      teardown_test_dnsbuf)
ISC_TEST_ENTRY_CUSTOM(dnsbuffer_resize_alloc_test, setup_test_dnsbuf,
		      teardown_test_dnsbuf)
ISC_TEST_ENTRY_CUSTOM(dnsbuffer_be_test, setup_test_dnsbuf,
		      teardown_test_dnsbuf)
ISC_TEST_ENTRY_CUSTOM(dnsasm_sequence_test, setup_test_dnsasm,
		      teardown_test_dnsasm)
ISC_TEST_ENTRY_CUSTOM(dnsasm_multiple_messages_test, setup_test_dnsasm,
		      teardown_test_dnsasm)
ISC_TEST_ENTRY_CUSTOM(dnsasm_torn_apart_test, setup_test_dnsasm,
		      teardown_test_dnsasm)
ISC_TEST_ENTRY_CUSTOM(dnsasm_error_data_test, setup_test_dnsasm,
		      teardown_test_dnsasm)
ISC_TEST_ENTRY_CUSTOM(dnsasm_torn_randomly_test, setup_test_dnsasm,
		      teardown_test_dnsasm)
ISC_TEST_ENTRY_CUSTOM(dnsasm_clear_buffer_within_cb_test, setup_test_dnsasm,
		      teardown_test_dnsasm)
ISC_TEST_LIST_END

ISC_TEST_MAIN
