/*	$NetBSD: proxyheader_test.c,v 1.1.1.1 2025/01/26 16:12:36 christos Exp $	*/

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

#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/proxy2.h>
#include <isc/random.h>

#include "proxyheader_test_data.h"

#include <tests/isc.h>

typedef struct dummy_handler_cbarg {
	isc_proxy2_command_t cmd;
	int socktype;
	isc_sockaddr_t src_addr;
	isc_sockaddr_t dst_addr;
	size_t no_more_calls;
	size_t tlvs;
	size_t tls_subtlvs;
	uint8_t tls_client_flags;
	bool client_cert_verified;
	isc_region_t tlv_data;
	isc_region_t extra;
	isc_region_t tls_version;
	isc_region_t tls_common_name;
} dummy_handler_cbarg_t;

static bool
dummy_subtlv_iter_cb(const uint8_t client, const bool client_cert_verified,
		     const isc_proxy2_tlv_subtype_tls_t tls_subtlv_type,
		     const isc_region_t *restrict data, void *cbarg) {
	dummy_handler_cbarg_t *arg = (dummy_handler_cbarg_t *)cbarg;

	UNUSED(client);
	UNUSED(client_cert_verified);

	arg->tls_subtlvs++;

	switch (tls_subtlv_type) {
	case ISC_PROXY2_TLV_SUBTYPE_TLS_VERSION:
		arg->tls_version = *data;
		break;
	case ISC_PROXY2_TLV_SUBTYPE_TLS_CN:
		arg->tls_common_name = *data;
		break;
	default:
		break;
	};

	return true;
}

static bool
dummy_tlv_iter_cb(const isc_proxy2_tlv_type_t tlv_type,
		  const isc_region_t *restrict data, void *cbarg) {
	dummy_handler_cbarg_t *arg = (dummy_handler_cbarg_t *)cbarg;

	if (arg != NULL) {
		arg->tlvs++;
	}

	if (tlv_type == ISC_PROXY2_TLV_TYPE_TLS) {
		isc_result_t result = isc_proxy2_subtlv_tls_header_data(
			data, &arg->tls_client_flags,
			&arg->client_cert_verified);

		assert_true(result == ISC_R_SUCCESS);

		result = isc_proxy2_subtlv_tls_iterate(
			data, dummy_subtlv_iter_cb, cbarg);

		assert_true(result == ISC_R_SUCCESS);
	}
	return true;
}

static void
proxy2_handler_dummy(const isc_result_t result, const isc_proxy2_command_t cmd,
		     const int socktype,
		     const isc_sockaddr_t *restrict src_addr,
		     const isc_sockaddr_t *restrict dst_addr,
		     const isc_region_t *restrict tlv_blob,
		     const isc_region_t *restrict extra, void *cbarg) {
	dummy_handler_cbarg_t *arg = (dummy_handler_cbarg_t *)cbarg;

	UNUSED(extra);

	if (result == ISC_R_NOMORE && arg != NULL) {
		arg->no_more_calls++;
		return;
	} else if (result != ISC_R_SUCCESS) {
		return;
	}

	if (cmd == ISC_PROXY2_CMD_PROXY && socktype != 0 /* unspec */) {
		INSIST(src_addr != NULL);
		INSIST(dst_addr != NULL);
	} else if (cmd == ISC_PROXY2_CMD_LOCAL) {
		INSIST(tlv_blob == NULL);
		INSIST(src_addr == NULL);
		INSIST(dst_addr == NULL);
	}

	if (arg != NULL) {
		arg->cmd = cmd;
		arg->socktype = socktype;
		if (src_addr != NULL) {
			INSIST(dst_addr != NULL);
			arg->src_addr = *src_addr;
			arg->dst_addr = *dst_addr;
		}
	}

	if (tlv_blob) {
		assert_true(isc_proxy2_tlv_data_verify(tlv_blob) ==
			    ISC_R_SUCCESS);
		if (cbarg != NULL) {
			isc_proxy2_tlv_iterate(tlv_blob, dummy_tlv_iter_cb,
					       cbarg);
		}
	}
}

static int
setup_test_proxy(void **state) {
	isc_proxy2_handler_t **handler = (isc_proxy2_handler_t **)state;
	*handler = isc_proxy2_handler_new(mctx, 0, proxy2_handler_dummy, NULL);
	return 0;
}

static int
teardown_test_proxy(void **state) {
	isc_proxy2_handler_free((isc_proxy2_handler_t **)state);

	return 0;
}

static void
test_header_data(isc_proxy2_handler_t *handler, const void *data,
		 const size_t size, const bool tear_apart,
		 const bool tear_randomly) {
	isc_region_t region = { 0 };
	isc_result_t result;

	if (tear_apart) {
		isc_buffer_t databuf = { 0 };
		isc_buffer_init(&databuf, (void *)data, size);
		isc_buffer_add(&databuf, size);

		for (; isc_buffer_remaininglength(&databuf) > 0;) {
			isc_region_t remaining = { 0 };
			size_t sz = 1;

			if (tear_randomly) {
				sz = 1 + isc_random_uniform(
						 isc_buffer_remaininglength(
							 &databuf));
			}

			isc_buffer_remainingregion(&databuf, &remaining);
			remaining.length = sz;

			result = isc_proxy2_handler_push(handler, &remaining);
			assert_true(isc_proxy2_handler_result(handler) ==
				    result);

			isc_buffer_forward(&databuf, sz);
			if (result == ISC_R_SUCCESS) {
				break;
			}
		}

	} else {
		result = isc_proxy2_handler_push_data(handler, data, size);
		assert_true(isc_proxy2_handler_result(handler) == result);
	}

	assert_true(isc_proxy2_handler_result(handler) == ISC_R_SUCCESS);
	isc_proxy2_handler_header(handler, &region);
	assert_true(region.length == size);
	assert_true(memcmp(region.base, data, region.length) == 0);
}

static void
verify_proxy_v2_header(isc_proxy2_handler_t *handler,
		       dummy_handler_cbarg_t *cbarg) {
	char sabuf[ISC_SOCKADDR_FORMATSIZE] = { 0 };
	isc_sockaddr_t src_addr = { 0 }, dst_addr = { 0 };
	isc_result_t result;
	int socktype = -1;

	assert_true(cbarg->cmd == ISC_PROXY2_CMD_PROXY);
	assert_true(cbarg->socktype == SOCK_STREAM);
	assert_true(isc_sockaddr_pf(&cbarg->dst_addr) == AF_INET);
	assert_true(isc_sockaddr_pf(&cbarg->src_addr) == AF_INET);

	isc_sockaddr_format(&cbarg->dst_addr, sabuf, sizeof(sabuf));
	assert_true(strcmp(sabuf, "127.0.0.66#11883") == 0);
	isc_sockaddr_format(&cbarg->src_addr, sabuf, sizeof(sabuf));
	assert_true(strcmp(sabuf, "127.0.0.1#56784") == 0);

	if (handler != NULL) {
		result = isc_proxy2_handler_addresses(handler, &socktype,
						      &src_addr, &dst_addr);
		assert_true(result == ISC_R_SUCCESS);
		assert_true(isc_sockaddr_equal(&src_addr, &cbarg->src_addr));
		assert_true(isc_sockaddr_equal(&dst_addr, &cbarg->dst_addr));
		assert_true(socktype == cbarg->socktype);
	}

	assert_true(cbarg->tlvs == 0);
	assert_true(cbarg->tls_subtlvs == 0);
	assert_true(cbarg->tls_client_flags == 0);
	assert_true(cbarg->client_cert_verified == false);
}

static void
verify_proxy_v2_header_with_TLS(isc_proxy2_handler_t *handler,
				dummy_handler_cbarg_t *cbarg) {
	char sabuf[ISC_SOCKADDR_FORMATSIZE] = { 0 };
	isc_sockaddr_t src_addr = { 0 }, dst_addr = { 0 };
	isc_result_t result;
	int socktype = -1;

	assert_true(cbarg->cmd == ISC_PROXY2_CMD_PROXY);
	assert_true(cbarg->socktype == SOCK_STREAM);
	assert_true(isc_sockaddr_pf(&cbarg->dst_addr) == AF_INET);
	assert_true(isc_sockaddr_pf(&cbarg->src_addr) == AF_INET);

	isc_sockaddr_format(&cbarg->dst_addr, sabuf, sizeof(sabuf));
	assert_true(strcmp(sabuf, "127.0.0.67#11883") == 0);
	isc_sockaddr_format(&cbarg->src_addr, sabuf, sizeof(sabuf));
	assert_true(strcmp(sabuf, "127.0.0.1#39754") == 0);

	if (handler != NULL) {
		result = isc_proxy2_handler_addresses(handler, &socktype,
						      &src_addr, &dst_addr);
		assert_true(result == ISC_R_SUCCESS);
		assert_true(isc_sockaddr_equal(&src_addr, &cbarg->src_addr));
		assert_true(isc_sockaddr_equal(&dst_addr, &cbarg->dst_addr));
		assert_true(socktype == cbarg->socktype);
	}

	assert_true(cbarg->tlvs == 1);
	assert_true(cbarg->tls_subtlvs == 1);
	assert_true(cbarg->tls_client_flags == ISC_PROXY2_CLIENT_TLS);
	assert_true(cbarg->client_cert_verified == true);

	/* "TLSv1.2" (w/o trailing '\0') */
	assert_true(cbarg->tls_version.length == 7);
	assert_true(memcmp(cbarg->tls_version.base, "TLSv1.2", 7) == 0);
}

static void
verify_proxy_v2_header_with_TLS_CN(isc_proxy2_handler_t *handler,
				   dummy_handler_cbarg_t *cbarg) {
	char sabuf[ISC_SOCKADDR_FORMATSIZE] = { 0 };
	isc_sockaddr_t src_addr = { 0 }, dst_addr = { 0 };
	isc_result_t result;
	int socktype = -1;

	assert_true(cbarg->cmd == ISC_PROXY2_CMD_PROXY);
	assert_true(cbarg->socktype == SOCK_STREAM);
	assert_true(isc_sockaddr_pf(&cbarg->dst_addr) == AF_INET);
	assert_true(isc_sockaddr_pf(&cbarg->src_addr) == AF_INET);

	isc_sockaddr_format(&cbarg->dst_addr, sabuf, sizeof(sabuf));
	assert_true(strcmp(sabuf, "127.0.0.67#11883") == 0);
	isc_sockaddr_format(&cbarg->src_addr, sabuf, sizeof(sabuf));
	assert_true(strcmp(sabuf, "127.0.0.1#40402") == 0);

	if (handler != NULL) {
		result = isc_proxy2_handler_addresses(handler, &socktype,
						      &src_addr, &dst_addr);
		assert_true(result == ISC_R_SUCCESS);
		assert_true(isc_sockaddr_equal(&src_addr, &cbarg->src_addr));
		assert_true(isc_sockaddr_equal(&dst_addr, &cbarg->dst_addr));
		assert_true(socktype == cbarg->socktype);
	}

	assert_true(cbarg->tlvs == 1);
	assert_true(cbarg->tls_subtlvs == 2); /* version and common name */
	assert_true(cbarg->tls_client_flags ==
		    (ISC_PROXY2_CLIENT_TLS | ISC_PROXY2_CLIENT_CERT_SESS |
		     ISC_PROXY2_CLIENT_CERT_CONN));
	assert_true(cbarg->client_cert_verified == true);

	/* "TLSv1.2" (w/o trailing '\0') */
	assert_true(cbarg->tls_version.length == 7);
	assert_true(memcmp(cbarg->tls_version.base, "TLSv1.2", 7) == 0);

	/* "mqttuser1" (w/o trailing '\0') */
	assert_true(cbarg->tls_common_name.length == 9);
	assert_true(memcmp(cbarg->tls_common_name.base, "mqttuser1", 9) == 0);
}

static void
verify_proxy_v2_header_with_AF_UNIX(isc_proxy2_handler_t *handler,
				    dummy_handler_cbarg_t *cbarg) {
	assert_true(cbarg->cmd == ISC_PROXY2_CMD_PROXY);
	assert_true(cbarg->socktype == 0);

	if (handler != NULL) {
		int socktype = -1;
		isc_result_t result;

		result = isc_proxy2_handler_addresses(handler, &socktype, NULL,
						      NULL);

		assert_int_equal(result, ISC_R_SUCCESS);

		assert_int_equal(socktype, 0);
	}
}

ISC_RUN_TEST_IMPL(proxyheader_generic_test) {
	isc_proxy2_handler_t *handler = (isc_proxy2_handler_t *)*state;
	dummy_handler_cbarg_t cbarg = { 0 };

	isc_proxy2_handler_setcb(handler, proxy2_handler_dummy, &cbarg);

	test_header_data(handler, proxy_v2_header, sizeof(proxy_v2_header),
			 false, false);
	verify_proxy_v2_header(handler, &cbarg);

	cbarg = (dummy_handler_cbarg_t){ 0 };
	test_header_data(handler, (void *)proxy_v2_header_with_TLS,
			 sizeof(proxy_v2_header_with_TLS), false, false);
	verify_proxy_v2_header_with_TLS(handler, &cbarg);

	cbarg = (dummy_handler_cbarg_t){ 0 };
	test_header_data(handler, (void *)proxy_v2_header_with_TLS_CN,
			 sizeof(proxy_v2_header_with_TLS_CN), false, false);
	verify_proxy_v2_header_with_TLS_CN(handler, &cbarg);

	cbarg = (dummy_handler_cbarg_t){ 0 };
	test_header_data(handler, (void *)proxy_v2_header_with_AF_UNIX,
			 sizeof(proxy_v2_header_with_AF_UNIX), false, false);
	verify_proxy_v2_header_with_AF_UNIX(handler, &cbarg);
}

ISC_RUN_TEST_IMPL(proxyheader_generic_byte_by_byte_test) {
	isc_proxy2_handler_t *handler = (isc_proxy2_handler_t *)*state;
	dummy_handler_cbarg_t cbarg = { 0 };

	isc_proxy2_handler_setcb(handler, proxy2_handler_dummy, &cbarg);

	test_header_data(handler, proxy_v2_header, sizeof(proxy_v2_header),
			 true, false);
	verify_proxy_v2_header(handler, &cbarg);
	assert_true(cbarg.no_more_calls == sizeof(proxy_v2_header) - 1);

	cbarg = (dummy_handler_cbarg_t){ 0 };
	test_header_data(handler, (void *)proxy_v2_header_with_TLS,
			 sizeof(proxy_v2_header_with_TLS), true, false);
	verify_proxy_v2_header_with_TLS(handler, &cbarg);
	assert_true(cbarg.no_more_calls ==
		    sizeof(proxy_v2_header_with_TLS) - 1);

	cbarg = (dummy_handler_cbarg_t){ 0 };
	test_header_data(handler, (void *)proxy_v2_header_with_TLS_CN,
			 sizeof(proxy_v2_header_with_TLS_CN), true, false);
	verify_proxy_v2_header_with_TLS_CN(handler, &cbarg);
	assert_true(cbarg.no_more_calls ==
		    sizeof(proxy_v2_header_with_TLS_CN) - 1);

	cbarg = (dummy_handler_cbarg_t){ 0 };
	test_header_data(handler, (void *)proxy_v2_header_with_AF_UNIX,
			 sizeof(proxy_v2_header_with_AF_UNIX), true, false);
	verify_proxy_v2_header_with_AF_UNIX(handler, &cbarg);
	assert_true(cbarg.no_more_calls ==
		    sizeof(proxy_v2_header_with_AF_UNIX) - 1);
}

ISC_RUN_TEST_IMPL(proxyheader_generic_torn_apart_randomly_test) {
	isc_proxy2_handler_t *handler = (isc_proxy2_handler_t *)*state;
	dummy_handler_cbarg_t cbarg = { 0 };

	isc_proxy2_handler_setcb(handler, proxy2_handler_dummy, &cbarg);

	test_header_data(handler, proxy_v2_header, sizeof(proxy_v2_header),
			 true, true);
	verify_proxy_v2_header(handler, &cbarg);

	cbarg = (dummy_handler_cbarg_t){ 0 };
	test_header_data(handler, (void *)proxy_v2_header_with_TLS,
			 sizeof(proxy_v2_header_with_TLS), true, true);
	verify_proxy_v2_header_with_TLS(handler, &cbarg);

	cbarg = (dummy_handler_cbarg_t){ 0 };
	test_header_data(handler, (void *)proxy_v2_header_with_TLS_CN,
			 sizeof(proxy_v2_header_with_TLS_CN), true, true);
	verify_proxy_v2_header_with_TLS_CN(handler, &cbarg);

	cbarg = (dummy_handler_cbarg_t){ 0 };
	test_header_data(handler, (void *)proxy_v2_header_with_AF_UNIX,
			 sizeof(proxy_v2_header_with_AF_UNIX), true, true);
	verify_proxy_v2_header_with_AF_UNIX(handler, &cbarg);
}

ISC_RUN_TEST_IMPL(proxyheader_direct_test) {
	isc_result_t result;
	isc_region_t region = { 0 };
	dummy_handler_cbarg_t cbarg = { 0 };

	cbarg = (dummy_handler_cbarg_t){ 0 };
	region.base = (uint8_t *)proxy_v2_header;
	region.length = sizeof(proxy_v2_header);
	result = isc_proxy2_header_handle_directly(
		&region, proxy2_handler_dummy, &cbarg);
	assert_true(result == ISC_R_SUCCESS);
	assert_true(cbarg.no_more_calls == 0);
	verify_proxy_v2_header(NULL, &cbarg);

	cbarg = (dummy_handler_cbarg_t){ 0 };
	region.base = (uint8_t *)proxy_v2_header_with_TLS;
	region.length = sizeof(proxy_v2_header_with_TLS);
	result = isc_proxy2_header_handle_directly(
		&region, proxy2_handler_dummy, &cbarg);
	assert_true(result == ISC_R_SUCCESS);
	assert_true(cbarg.no_more_calls == 0);
	isc_proxy2_tlv_iterate(&cbarg.tlv_data, dummy_tlv_iter_cb, &cbarg);
	verify_proxy_v2_header_with_TLS(NULL, &cbarg);

	cbarg = (dummy_handler_cbarg_t){ 0 };
	region.base = (uint8_t *)proxy_v2_header_with_TLS_CN;
	region.length = sizeof(proxy_v2_header_with_TLS_CN);
	result = isc_proxy2_header_handle_directly(
		&region, proxy2_handler_dummy, &cbarg);
	assert_true(result == ISC_R_SUCCESS);
	assert_true(cbarg.no_more_calls == 0);
	isc_proxy2_tlv_iterate(&cbarg.tlv_data, dummy_tlv_iter_cb, &cbarg);
	verify_proxy_v2_header_with_TLS_CN(NULL, &cbarg);

	cbarg = (dummy_handler_cbarg_t){ 0 };
	region.base = (uint8_t *)proxy_v2_header_with_AF_UNIX;
	region.length = sizeof(proxy_v2_header_with_AF_UNIX);
	result = isc_proxy2_header_handle_directly(
		&region, proxy2_handler_dummy, &cbarg);
	assert_true(result == ISC_R_SUCCESS);
	assert_true(cbarg.no_more_calls == 0);
	verify_proxy_v2_header_with_AF_UNIX(NULL, &cbarg);
}

ISC_RUN_TEST_IMPL(proxyheader_detect_bad_signature_test) {
	isc_proxy2_handler_t *handler = (isc_proxy2_handler_t *)*state;

	for (size_t i = 0; i < ISC_PROXY2_HEADER_SIGNATURE_SIZE; i++) {
		isc_result_t result;
		uint8_t sig[ISC_PROXY2_HEADER_SIGNATURE_SIZE];
		memmove(sig, ISC_PROXY2_HEADER_SIGNATURE,
			ISC_PROXY2_HEADER_SIGNATURE_SIZE);

		sig[i] = 0x0C; /* it is not present in the valid signature */

		/*
		 * We are expected to detect bad signature as early as possible,
		 * so we are passing only a part of the header.
		 */
		result = isc_proxy2_handler_push_data(handler, sig, i + 1);
		assert_true(result == ISC_R_UNEXPECTED);
	}
}

ISC_RUN_TEST_IMPL(proxyheader_extra_data_test) {
	isc_result_t result;
	isc_buffer_t databuf;
	isc_region_t region = { 0 };
	size_t sz;
	isc_proxy2_handler_t *handler = (isc_proxy2_handler_t *)*state;
	uint8_t header[] = { 0x0d, 0x0a, 0x0d, 0x0a, 0x00, 0x0d, 0x0a, 0x51,
			     0x55, 0x49, 0x54, 0x0a, 0x21, 0x11, 0x00, 0x1e,
			     0x7f, 0x00, 0x00, 0x01, 0x7f, 0x00, 0x00, 0x43,
			     0x9b, 0x4a, 0x2e, 0x6b, 0x20, 0x00, 0x0f, 0x01,
			     0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x07, 0x54,
			     0x4c, 0x53, 0x76, 0x31, 0x2e, 0x32 };
	uint8_t extra_data[] = { 0x10, 0x1a, 0x00, 0x04, 0x4d, 0x51, 0x54,
				 0x54, 0x04, 0x02, 0x00, 0x3c, 0x00, 0x0e,
				 0x4d, 0x51, 0x54, 0x54, 0x5f, 0x46, 0x58,
				 0x5f, 0x43, 0x6c, 0x69, 0x65, 0x6e, 0x74 };
	uint8_t data[sizeof(header) + sizeof(extra_data)];

	isc_buffer_init(&databuf, (void *)data, sizeof(data));

	isc_buffer_putmem(&databuf, header, sizeof(header));
	isc_buffer_putmem(&databuf, extra_data, sizeof(extra_data));

	isc_buffer_remainingregion(&databuf, &region);

	result = isc_proxy2_handler_push(handler, &region);
	assert_true(result == ISC_R_SUCCESS);

	region = (isc_region_t){ 0 };
	sz = isc_proxy2_handler_header(handler, &region);
	assert_true(sz == sizeof(header));
	assert_true(sz == region.length);
	assert_true(memcmp(header, region.base, sz) == 0);

	region = (isc_region_t){ 0 };
	sz = isc_proxy2_handler_extra(handler, &region);
	assert_true(sz == sizeof(extra_data));
	assert_true(sz == region.length);
	assert_true(memcmp(extra_data, region.base, sz) == 0);
}

ISC_RUN_TEST_IMPL(proxyheader_max_size_test) {
	isc_result_t result;
	isc_proxy2_handler_t handler;

	UNUSED(state);

	isc_proxy2_handler_init(&handler, mctx, sizeof(proxy_v2_header),
				proxy2_handler_dummy, NULL);

	result = isc_proxy2_handler_push_data(&handler, proxy_v2_header,
					      sizeof(proxy_v2_header));

	assert_true(result == ISC_R_SUCCESS);

	isc_proxy2_handler_uninit(&handler);

	isc_proxy2_handler_init(&handler, mctx, sizeof(proxy_v2_header) - 1,
				proxy2_handler_dummy, NULL);

	result = isc_proxy2_handler_push_data(&handler, proxy_v2_header,
					      sizeof(proxy_v2_header));

	assert_true(result == ISC_R_RANGE);

	isc_proxy2_handler_uninit(&handler);
}

ISC_RUN_TEST_IMPL(proxyheader_make_header_test) {
	isc_result_t result;
	isc_proxy2_handler_t *handler = (isc_proxy2_handler_t *)*state;
	isc_buffer_t databuf;
	uint8_t data[ISC_PROXY2_MAX_SIZE];
	isc_buffer_t sslbuf;
	uint8_t ssldata[ISC_PROXY2_MAX_SIZE];
	isc_region_t region = { 0 };
	uint8_t extra[256] = { 0 };
	const char *tls_version = "TLSv1.3";
	const char *tls_cn = "name.test";
	dummy_handler_cbarg_t cbarg = { 0 };
	struct in_addr localhost4 = { 0 };
	isc_sockaddr_t src_addrv4 = { 0 }, dst_addrv4 = { 0 },
		       src_addrv6 = { 0 }, dst_addrv6 = { 0 };
	const uint16_t src_port = 1236;
	const uint16_t dst_port = 9582;

	localhost4.s_addr = htonl(INADDR_LOOPBACK);

	isc_sockaddr_fromin(&src_addrv4, &localhost4, src_port);
	isc_sockaddr_fromin(&dst_addrv4, &localhost4, dst_port);
	isc_sockaddr_fromin6(&src_addrv6, &in6addr_loopback, src_port);
	isc_sockaddr_fromin6(&dst_addrv6, &in6addr_loopback, dst_port);
	isc_proxy2_handler_setcb(handler, proxy2_handler_dummy, &cbarg);

	isc_buffer_init(&databuf, (void *)data, sizeof(data));
	isc_buffer_init(&sslbuf, (void *)ssldata, sizeof(ssldata));

	/* unspec */
	result = isc_proxy2_make_header(&databuf, ISC_PROXY2_CMD_LOCAL, 0, NULL,
					NULL, NULL);
	assert_true(result == ISC_R_SUCCESS);

	isc_buffer_usedregion(&databuf, &region);
	assert_true(region.length == ISC_PROXY2_HEADER_SIZE);

	region = (isc_region_t){ .base = extra, .length = sizeof(extra) };
	result = isc_proxy2_header_append_tlv(
		&databuf, ISC_PROXY2_TLV_TYPE_NOOP, &region);
	assert_true(result == ISC_R_SUCCESS);

	isc_buffer_usedregion(&databuf, &region);
	assert_true(region.length == ISC_PROXY2_HEADER_SIZE + sizeof(extra) +
					     ISC_PROXY2_TLV_HEADER_SIZE);

	result = isc_proxy2_handler_push(handler, &region);
	assert_true(result == ISC_R_SUCCESS);
	assert_true(cbarg.tlvs == 0); /* in unspec mode we ignore TLVs */

	/* AF_INET, SOCK_STREAM */
	cbarg = (dummy_handler_cbarg_t){ 0 };
	isc_buffer_clear(&databuf);

	result = isc_proxy2_make_header(&databuf, ISC_PROXY2_CMD_PROXY,
					SOCK_STREAM, &src_addrv4, &dst_addrv4,
					NULL);
	assert_true(result == ISC_R_SUCCESS);

	isc_buffer_usedregion(&databuf, &region);
	assert_true(region.length == ISC_PROXY2_MIN_AF_INET_SIZE);

	region = (isc_region_t){ .base = extra, .length = sizeof(extra) };
	result = isc_proxy2_header_append_tlv(
		&databuf, ISC_PROXY2_TLV_TYPE_NOOP, &region);
	assert_true(result == ISC_R_SUCCESS);

	isc_buffer_usedregion(&databuf, &region);
	assert_true(region.length == ISC_PROXY2_MIN_AF_INET_SIZE +
					     sizeof(extra) +
					     ISC_PROXY2_TLV_HEADER_SIZE);

	result = isc_proxy2_handler_push(handler, &region);
	assert_true(result == ISC_R_SUCCESS);
	assert_true(cbarg.tlvs == 1); /* ISC_PROXY2_TLV_TYPE_NOOP */

	assert_true(cbarg.socktype == SOCK_STREAM);
	assert_true(isc_sockaddr_pf(&cbarg.src_addr) == AF_INET);
	assert_true(isc_sockaddr_pf(&cbarg.dst_addr) == AF_INET);

	assert_true(isc_sockaddr_equal(&cbarg.src_addr, &src_addrv4));
	assert_true(isc_sockaddr_equal(&cbarg.dst_addr, &dst_addrv4));

	/* AF_INET6, SOCK_STREAM (+ TLS version and CN) */
	cbarg = (dummy_handler_cbarg_t){ 0 };
	isc_buffer_clear(&databuf);

	result = isc_proxy2_make_header(&databuf, ISC_PROXY2_CMD_PROXY,
					SOCK_STREAM, &src_addrv6, &dst_addrv6,
					NULL);
	assert_true(result == ISC_R_SUCCESS);

	isc_buffer_usedregion(&databuf, &region);
	assert_true(region.length == ISC_PROXY2_MIN_AF_INET6_SIZE);

	region = (isc_region_t){ .base = extra, .length = sizeof(extra) };
	result = isc_proxy2_header_append_tlv(
		&databuf, ISC_PROXY2_TLV_TYPE_NOOP, &region);
	assert_true(result == ISC_R_SUCCESS);

	result = isc_proxy2_make_tls_subheader(
		&sslbuf, ISC_PROXY2_CLIENT_TLS | ISC_PROXY2_CLIENT_CERT_CONN,
		true, NULL);
	assert_true(result == ISC_R_SUCCESS);
	result = isc_proxy2_append_tlv_string(
		&sslbuf, ISC_PROXY2_TLV_SUBTYPE_TLS_VERSION, tls_version);
	assert_true(result == ISC_R_SUCCESS);
	result = isc_proxy2_append_tlv_string(
		&sslbuf, ISC_PROXY2_TLV_SUBTYPE_TLS_CN, tls_cn);
	assert_true(result == ISC_R_SUCCESS);

	isc_buffer_usedregion(&sslbuf, &region);
	result = isc_proxy2_header_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_TLS,
					      &region);
	assert_true(result == ISC_R_SUCCESS);

	isc_buffer_usedregion(&databuf, &region);
	size_t expected = ISC_PROXY2_MIN_AF_INET6_SIZE + sizeof(extra) +
			  (4 * ISC_PROXY2_TLV_HEADER_SIZE) +
			  ISC_PROXY2_TLS_SUBHEADER_MIN_SIZE +
			  strlen(tls_version) + strlen(tls_cn);
	assert_true(region.length == expected);

	result = isc_proxy2_handler_push(handler, &region);
	assert_true(result == ISC_R_SUCCESS);

	assert_true(cbarg.socktype == SOCK_STREAM);
	assert_true(isc_sockaddr_pf(&cbarg.src_addr) == AF_INET6);
	assert_true(isc_sockaddr_pf(&cbarg.dst_addr) == AF_INET6);

	assert_true(isc_sockaddr_equal(&cbarg.src_addr, &src_addrv6));
	assert_true(isc_sockaddr_equal(&cbarg.dst_addr, &dst_addrv6));

	region = (isc_region_t){ 0 };
	(void)isc_proxy2_handler_tlvs(handler, &region);
	assert_true(isc_proxy2_tlv_data_verify(&region) == ISC_R_SUCCESS);
	/* ISC_PROXY2_TLV_TYPE_NOOP+ISC_PROXY2_TLV_TYPE_TLS */
	assert_true(cbarg.tlvs == 2);
	/* ISC_PROXY2_TLV_SUBTYPE_TLS_VERSION+ISC_PROXY2_TLV_SUBTYPE_TLS_CN */
	assert_true(cbarg.tls_subtlvs == 2);

	assert_true(cbarg.tls_version.length == strlen(tls_version));
	assert_true(memcmp(cbarg.tls_version.base, tls_version,
			   strlen(tls_version)) == 0);

	assert_true(cbarg.tls_common_name.length == strlen(tls_cn));
	assert_true(memcmp(cbarg.tls_common_name.base, tls_cn,
			   strlen(tls_cn)) == 0);
}

static bool
rebuild_subtlv_iter_cb(const uint8_t client, const bool client_cert_verified,
		       const isc_proxy2_tlv_subtype_tls_t tls_subtlv_type,
		       const isc_region_t *restrict data, void *cbarg) {
	isc_result_t result;
	isc_buffer_t *outbuf = (isc_buffer_t *)cbarg;

	UNUSED(client);
	UNUSED(client_cert_verified);

	result = isc_proxy2_append_tlv(outbuf, tls_subtlv_type, data);
	assert_true(result == ISC_R_SUCCESS);

	return true;
}

static bool
rebuild_tlv_iter_cb(const isc_proxy2_tlv_type_t tlv_type,
		    const isc_region_t *restrict data, void *cbarg) {
	isc_result_t result;
	isc_buffer_t *outbuf = (isc_buffer_t *)cbarg;

	if (tlv_type == ISC_PROXY2_TLV_TYPE_TLS) {
		uint8_t client_flags = 0;
		bool client_cert_verified = false;
		isc_buffer_t databuf = { 0 };
		isc_region_t region = { 0 };
		uint8_t storage[ISC_PROXY2_MAX_SIZE];

		isc_buffer_init(&databuf, (void *)storage, sizeof(storage));

		/* get flags values */
		result = isc_proxy2_subtlv_tls_header_data(
			data, &client_flags, &client_cert_verified);
		assert_true(result == ISC_R_SUCCESS);

		/* create header */
		result = isc_proxy2_make_tls_subheader(
			&databuf, client_flags, client_cert_verified, NULL);
		assert_true(result == ISC_R_SUCCESS);

		/* process and append values */
		result = isc_proxy2_subtlv_tls_iterate(
			data, rebuild_subtlv_iter_cb, &databuf);
		assert_true(result == ISC_R_SUCCESS);

		isc_buffer_usedregion(&databuf, &region);
		result = isc_proxy2_header_append_tlv(outbuf, tlv_type,
						      &region);
		assert_true(result == ISC_R_SUCCESS);
	} else {
		result = isc_proxy2_header_append_tlv(outbuf, tlv_type, data);
		assert_true(result == ISC_R_SUCCESS);
	}

	return true;
}

static void
proxy2_handler_rebuild_cb(const isc_result_t header_result,
			  const isc_proxy2_command_t cmd, const int socktype,
			  const isc_sockaddr_t *restrict src_addr,
			  const isc_sockaddr_t *restrict dst_addr,
			  const isc_region_t *restrict tlv_blob,
			  const isc_region_t *restrict extra, void *cbarg) {
	isc_result_t result;
	isc_buffer_t *outbuf = (isc_buffer_t *)cbarg;

	if (header_result != ISC_R_SUCCESS) {
		return;
	}

	result = isc_proxy2_make_header(outbuf, cmd, socktype, src_addr,
					dst_addr, NULL);
	assert_true(result == ISC_R_SUCCESS);

	if (tlv_blob != NULL) {
		isc_proxy2_tlv_iterate(tlv_blob, rebuild_tlv_iter_cb, outbuf);
	}

	if (extra != NULL) {
		result = isc_proxy2_tlv_data_verify(tlv_blob);
		assert_true(result == ISC_R_SUCCESS);
		isc_buffer_putmem(outbuf, extra->base, extra->length);
	}
}

static void
proxy2_handler_rebuild(isc_buffer_t *restrict outbuf, const void *data,
		       const size_t size) {
	isc_proxy2_handler_t handler = { 0 };

	isc_proxy2_handler_init(&handler, mctx, 0, proxy2_handler_rebuild_cb,
				outbuf);

	isc_proxy2_handler_push_data(&handler, data, size);

	isc_proxy2_handler_uninit(&handler);
}

static void
try_rebuild_header(const void *data, size_t size) {
	isc_buffer_t databuf = { 0 };
	isc_region_t region = { 0 };
	uint8_t storage[ISC_PROXY2_MAX_SIZE];

	isc_buffer_init(&databuf, (void *)storage, sizeof(storage));

	proxy2_handler_rebuild(&databuf, data, size);
	isc_buffer_usedregion(&databuf, &region);
	assert_true(region.length == size);
	assert_true(memcmp(region.base, data, size) == 0);
}

ISC_RUN_TEST_IMPL(proxyheader_rebuild_header_test) {
	try_rebuild_header(proxy_v2_header, sizeof(proxy_v2_header));
	try_rebuild_header(proxy_v2_header_with_TLS,
			   sizeof(proxy_v2_header_with_TLS));
	try_rebuild_header(proxy_v2_header_with_TLS_CN,
			   sizeof(proxy_v2_header_with_TLS_CN));
}

ISC_RUN_TEST_IMPL(proxyheader_bad_header_signature_test) {
	size_t i;
	isc_result_t result;
	isc_proxy2_handler_t *handler = (isc_proxy2_handler_t *)*state;

	for (i = 0; i < ISC_PROXY2_HEADER_SIGNATURE_SIZE; i++) {
		uint8_t sig[ISC_PROXY2_HEADER_SIGNATURE_SIZE];
		memmove(sig, ISC_PROXY2_HEADER_SIGNATURE,
			ISC_PROXY2_HEADER_SIGNATURE_SIZE);
		sig[i] = 0x0C; /* 0x0C cannot be found in the signature */
		result = isc_proxy2_handler_push_data(handler, sig,
						      sizeof(sig));
		assert_true(result == ISC_R_UNEXPECTED);
		isc_proxy2_handler_clear(handler);
	}

	result = isc_proxy2_handler_push_data(handler,
					      ISC_PROXY2_HEADER_SIGNATURE,
					      ISC_PROXY2_HEADER_SIGNATURE_SIZE);
	assert_true(result == ISC_R_NOMORE);
}

ISC_RUN_TEST_IMPL(proxyheader_bad_proto_version_command_test) {
	isc_proxy2_handler_t *handler = (isc_proxy2_handler_t *)*state;
	isc_result_t result;
	uint8_t *pver_cmd = NULL;
	uint8_t botched_header[sizeof(proxy_v2_header)] = { 0 };

	memmove(botched_header, proxy_v2_header, sizeof(proxy_v2_header));

	pver_cmd = &botched_header[ISC_PROXY2_HEADER_SIGNATURE_SIZE];

	assert_true(*pver_cmd == 0x21);

	*pver_cmd = 0x31; /* unexpected version (3) followed by PROXY command */

	result = isc_proxy2_handler_push_data(handler, botched_header,
					      sizeof(botched_header));
	assert_true(result == ISC_R_NOTIMPLEMENTED);

	*pver_cmd = 0x22; /* version two followed by unexpected command (2) */

	result = isc_proxy2_handler_push_data(handler, botched_header,
					      sizeof(botched_header));
	assert_true(result == ISC_R_UNEXPECTED);
}

ISC_RUN_TEST_IMPL(proxyheader_bad_family_socktype_test) {
	isc_proxy2_handler_t *handler = (isc_proxy2_handler_t *)*state;
	isc_result_t result;
	uint8_t *pfam = NULL;
	uint8_t botched_header[sizeof(proxy_v2_header)] = { 0 };

	memmove(botched_header, proxy_v2_header, sizeof(proxy_v2_header));

	pfam = &botched_header[ISC_PROXY2_HEADER_SIGNATURE_SIZE + 1];

	assert_true(*pfam == 0x11);

	*pfam = 0x41; /* unexpected family (4) followed by SOCK_STREAM (1)*/

	result = isc_proxy2_handler_push_data(handler, botched_header,
					      sizeof(botched_header));
	assert_true(result == ISC_R_UNEXPECTED);

	*pfam = 0x13; /* AF_INET (1) followed by unexpected sock type (3) */

	result = isc_proxy2_handler_push_data(handler, botched_header,
					      sizeof(botched_header));
	assert_true(result == ISC_R_UNEXPECTED);
}

static inline void
update_header_length(uint8_t *botched_header, uint16_t newlen) {
	newlen = htons(newlen);
	memmove(&botched_header[ISC_PROXY2_HEADER_SIGNATURE_SIZE + 2], &newlen,
		sizeof(newlen));
}

ISC_RUN_TEST_IMPL(proxyheader_bad_unexpected_not_enough_length_test) {
	isc_proxy2_handler_t *handler = (isc_proxy2_handler_t *)*state;
	isc_result_t result;
	uint8_t botched_header[sizeof(proxy_v2_header)] = { 0 };

	memmove(botched_header, proxy_v2_header, sizeof(proxy_v2_header));

	update_header_length(botched_header, 0);
	result = isc_proxy2_handler_push_data(handler, botched_header,
					      sizeof(botched_header));
	assert_true(result == ISC_R_RANGE);

	update_header_length(botched_header, 4); /* not enough */
	result = isc_proxy2_handler_push_data(handler, botched_header,
					      sizeof(botched_header));
	assert_true(result == ISC_R_RANGE);

	update_header_length(botched_header, UINT16_MAX); /* no more */
	result = isc_proxy2_handler_push_data(handler, botched_header,
					      sizeof(botched_header));
	assert_true(result == ISC_R_NOMORE);
	isc_proxy2_handler_clear(handler);
}

ISC_RUN_TEST_IMPL(proxyheader_tlv_data_test) {
	isc_result_t result;
	isc_buffer_t databuf = { 0 };
	isc_buffer_t tlsbuf = { 0 };
	uint8_t data[ISC_PROXY2_MAX_SIZE] = { 0 };
	uint8_t tlsdata[ISC_PROXY2_MAX_SIZE] = { 0 };
	uint8_t zerodata[0xff] = { 0 };
	isc_region_t region = { 0 };
	const char *alpn = "dot";
	const char *tls_version = "TLSv1.3";
	const char *tls_cn = "name.test";

	isc_buffer_init(&databuf, (void *)data, sizeof(data));
	isc_buffer_init(&tlsbuf, (void *)tlsdata, sizeof(tlsdata));

	/* zero filled data is not fine */
	region.base = zerodata;
	region.length = sizeof(zerodata);
	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_UNEXPECTED);

	/* crc32c must be 4 bytes long */
	isc_buffer_clear(&databuf);
	region.base = (uint8_t *)zerodata;
	region.length = sizeof(zerodata);
	result = isc_proxy2_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_CRC32C,
				       &region);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&databuf, &region);

	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_RANGE);

	isc_buffer_clear(&databuf);
	region.base = (uint8_t *)zerodata;
	region.length = 4;
	result = isc_proxy2_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_CRC32C,
				       &region);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&databuf, &region);

	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_SUCCESS);

	/* unique id must be <= 128 bytes long */
	isc_buffer_clear(&databuf);
	region.base = (uint8_t *)zerodata;
	region.length = sizeof(zerodata);
	result = isc_proxy2_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_UNIQUE_ID,
				       &region);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&databuf, &region);

	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_RANGE);

	isc_buffer_clear(&databuf);
	region.base = (uint8_t *)zerodata;
	region.length = 128;
	result = isc_proxy2_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_UNIQUE_ID,
				       &region);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&databuf, &region);

	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_SUCCESS);

	/* two noops is fine */
	isc_buffer_clear(&databuf);
	region = (isc_region_t){ 0 };
	result = isc_proxy2_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_NOOP,
				       &region);
	assert_true(result == ISC_R_SUCCESS);
	result = isc_proxy2_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_NOOP,
				       &region);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&databuf, &region);

	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_SUCCESS);

	/* one ALPN tag is fine */
	isc_buffer_clear(&databuf);
	result = isc_proxy2_append_tlv_string(&databuf,
					      ISC_PROXY2_TLV_TYPE_ALPN, alpn);
	assert_true(result == ISC_R_SUCCESS);

	isc_buffer_usedregion(&databuf, &region);
	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_SUCCESS);

	/* two ALPN tags is not fine */
	result = isc_proxy2_append_tlv_string(&databuf,
					      ISC_PROXY2_TLV_TYPE_ALPN, alpn);
	assert_true(result == ISC_R_SUCCESS);

	isc_buffer_usedregion(&databuf, &region);
	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_UNEXPECTED);

	/* empty TLS subheader is tolerable */
	isc_buffer_clear(&databuf);
	isc_buffer_clear(&tlsbuf);
	result = isc_proxy2_make_tls_subheader(&tlsbuf, 0, false, NULL);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&tlsbuf, &region);
	result = isc_proxy2_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_TLS,
				       &region);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&databuf, &region);
	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_SUCCESS);

	/* empty TLS subheader with no TLS version while one is expected */
	isc_buffer_clear(&databuf);
	isc_buffer_clear(&tlsbuf);
	result = isc_proxy2_make_tls_subheader(&tlsbuf, ISC_PROXY2_CLIENT_TLS,
					       false, NULL);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&tlsbuf, &region);
	result = isc_proxy2_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_TLS,
				       &region);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&databuf, &region);
	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_UNEXPECTED);

	/* TLS subheader with TLS version */
	isc_buffer_clear(&databuf);
	isc_buffer_clear(&tlsbuf);
	result = isc_proxy2_make_tls_subheader(&tlsbuf, ISC_PROXY2_CLIENT_TLS,
					       false, NULL);
	assert_true(result == ISC_R_SUCCESS);
	region.length = sizeof(tls_version);
	result = isc_proxy2_append_tlv_string(
		&tlsbuf, ISC_PROXY2_TLV_SUBTYPE_TLS_VERSION, tls_version);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&tlsbuf, &region);
	result = isc_proxy2_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_TLS,
				       &region);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&databuf, &region);
	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_SUCCESS);

	/* TLS subheader with multiple TLS versions is not fine */
	isc_buffer_clear(&databuf);
	isc_buffer_clear(&tlsbuf);
	result = isc_proxy2_make_tls_subheader(&tlsbuf, ISC_PROXY2_CLIENT_TLS,
					       false, NULL);
	assert_true(result == ISC_R_SUCCESS);
	result = isc_proxy2_append_tlv_string(
		&tlsbuf, ISC_PROXY2_TLV_SUBTYPE_TLS_VERSION, tls_version);
	assert_true(result == ISC_R_SUCCESS);
	result = isc_proxy2_append_tlv(
		&tlsbuf, ISC_PROXY2_TLV_SUBTYPE_TLS_VERSION, &region);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&tlsbuf, &region);
	result = isc_proxy2_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_TLS,
				       &region);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&databuf, &region);
	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_UNEXPECTED);

	/* TLS subheader with unexpected TLS version */
	isc_buffer_clear(&databuf);
	isc_buffer_clear(&tlsbuf);
	result = isc_proxy2_make_tls_subheader(&tlsbuf, 0, false, NULL);
	assert_true(result == ISC_R_SUCCESS);
	result = isc_proxy2_append_tlv_string(
		&tlsbuf, ISC_PROXY2_TLV_SUBTYPE_TLS_VERSION, tls_version);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&tlsbuf, &region);
	result = isc_proxy2_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_TLS,
				       &region);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&databuf, &region);
	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_UNEXPECTED);

	/* TLS subheader with no CN while expected */
	isc_buffer_clear(&databuf);
	isc_buffer_clear(&tlsbuf);
	result = isc_proxy2_make_tls_subheader(
		&tlsbuf, ISC_PROXY2_CLIENT_TLS | ISC_PROXY2_CLIENT_CERT_CONN,
		false, NULL);
	assert_true(result == ISC_R_SUCCESS);
	result = isc_proxy2_append_tlv_string(
		&tlsbuf, ISC_PROXY2_TLV_SUBTYPE_TLS_VERSION, tls_version);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&tlsbuf, &region);
	result = isc_proxy2_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_TLS,
				       &region);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&databuf, &region);
	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_UNEXPECTED);

	/* TLS subheader with unexpected CN */
	isc_buffer_clear(&databuf);
	isc_buffer_clear(&tlsbuf);
	result = isc_proxy2_make_tls_subheader(&tlsbuf, ISC_PROXY2_CLIENT_TLS,
					       false, NULL);
	assert_true(result == ISC_R_SUCCESS);
	result = isc_proxy2_append_tlv_string(
		&tlsbuf, ISC_PROXY2_TLV_SUBTYPE_TLS_CN, tls_cn);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&tlsbuf, &region);
	result = isc_proxy2_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_TLS,
				       &region);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&databuf, &region);
	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_UNEXPECTED);

	/* TLS subheader with CN unexpected (because TLS flag is not set) */
	isc_buffer_clear(&databuf);
	isc_buffer_clear(&tlsbuf);
	result = isc_proxy2_make_tls_subheader(
		&tlsbuf,
		ISC_PROXY2_CLIENT_CERT_CONN | ISC_PROXY2_CLIENT_CERT_SESS,
		false, NULL);
	assert_true(result == ISC_R_SUCCESS);
	result = isc_proxy2_append_tlv_string(
		&tlsbuf, ISC_PROXY2_TLV_SUBTYPE_TLS_CN, tls_cn);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&tlsbuf, &region);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&tlsbuf, &region);
	result = isc_proxy2_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_TLS,
				       &region);
	assert_true(result == ISC_R_SUCCESS);
	isc_buffer_usedregion(&databuf, &region);
	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_UNEXPECTED);

	/* botched TLV header */
	isc_buffer_clear(&databuf);
	region.base = (uint8_t *)zerodata;
	region.length = sizeof(zerodata);
	result = isc_proxy2_append_tlv(&databuf, ISC_PROXY2_TLV_TYPE_NOOP,
				       &region);
	isc_buffer_subtract(&databuf, region.length / 2);
	isc_buffer_usedregion(&databuf, &region);
	result = isc_proxy2_tlv_data_verify(&region);
	assert_true(result == ISC_R_RANGE);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(proxyheader_generic_test, setup_test_proxy,
		      teardown_test_proxy)
ISC_TEST_ENTRY_CUSTOM(proxyheader_generic_byte_by_byte_test, setup_test_proxy,
		      teardown_test_proxy)
ISC_TEST_ENTRY_CUSTOM(proxyheader_generic_torn_apart_randomly_test,
		      setup_test_proxy, teardown_test_proxy)
ISC_TEST_ENTRY_CUSTOM(proxyheader_direct_test, setup_test_proxy,
		      teardown_test_proxy)
ISC_TEST_ENTRY_CUSTOM(proxyheader_detect_bad_signature_test, setup_test_proxy,
		      teardown_test_proxy)
ISC_TEST_ENTRY_CUSTOM(proxyheader_extra_data_test, setup_test_proxy,
		      teardown_test_proxy)
ISC_TEST_ENTRY_CUSTOM(proxyheader_max_size_test, setup_test_proxy,
		      teardown_test_proxy)
ISC_TEST_ENTRY_CUSTOM(proxyheader_make_header_test, setup_test_proxy,
		      teardown_test_proxy)
ISC_TEST_ENTRY_CUSTOM(proxyheader_rebuild_header_test, setup_test_proxy,
		      teardown_test_proxy)
ISC_TEST_ENTRY_CUSTOM(proxyheader_bad_header_signature_test, setup_test_proxy,
		      teardown_test_proxy)
ISC_TEST_ENTRY_CUSTOM(proxyheader_bad_proto_version_command_test,
		      setup_test_proxy, teardown_test_proxy)
ISC_TEST_ENTRY_CUSTOM(proxyheader_bad_family_socktype_test, setup_test_proxy,
		      teardown_test_proxy)
ISC_TEST_ENTRY_CUSTOM(proxyheader_bad_unexpected_not_enough_length_test,
		      setup_test_proxy, teardown_test_proxy)
ISC_TEST_ENTRY_CUSTOM(proxyheader_tlv_data_test, setup_test_proxy,
		      teardown_test_proxy)
ISC_TEST_LIST_END

ISC_TEST_MAIN
