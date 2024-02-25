/*	$NetBSD: tsig_test.c,v 1.2.2.2 2024/02/25 15:47:40 martin Exp $	*/

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

#include <isc/mem.h>
#include <isc/print.h>
#include <isc/util.h>

#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/tsig.h>

#include "tsig_p.h"

#include <tests/dns.h>

#define TEST_ORIGIN "test"

static int debug = 0;

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

static isc_result_t
add_mac(dst_context_t *tsigctx, isc_buffer_t *buf) {
	dns_rdata_any_tsig_t tsig;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_buffer_t databuf;
	isc_region_t r;
	isc_result_t result;
	unsigned char tsigbuf[1024];

	isc_buffer_usedregion(buf, &r);
	dns_rdata_fromregion(&rdata, dns_rdataclass_any, dns_rdatatype_tsig,
			     &r);
	isc_buffer_init(&databuf, tsigbuf, sizeof(tsigbuf));
	CHECK(dns_rdata_tostruct(&rdata, &tsig, NULL));
	isc_buffer_putuint16(&databuf, tsig.siglen);
	isc_buffer_putmem(&databuf, tsig.signature, tsig.siglen);
	isc_buffer_usedregion(&databuf, &r);
	result = dst_context_adddata(tsigctx, &r);
	dns_rdata_freestruct(&tsig);
cleanup:
	return (result);
}

static isc_result_t
add_tsig(dst_context_t *tsigctx, dns_tsigkey_t *key, isc_buffer_t *target) {
	dns_compress_t cctx;
	dns_rdata_any_tsig_t tsig;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdatalist_t rdatalist;
	dns_rdataset_t rdataset;
	isc_buffer_t *dynbuf = NULL;
	isc_buffer_t databuf;
	isc_buffer_t sigbuf;
	isc_region_t r;
	isc_result_t result = ISC_R_SUCCESS;
	isc_stdtime_t now;
	unsigned char tsigbuf[1024];
	unsigned int count;
	unsigned int sigsize = 0;
	bool invalidate_ctx = false;

	memset(&tsig, 0, sizeof(tsig));

	CHECK(dns_compress_init(&cctx, -1, mctx));
	invalidate_ctx = true;

	tsig.common.rdclass = dns_rdataclass_any;
	tsig.common.rdtype = dns_rdatatype_tsig;
	ISC_LINK_INIT(&tsig.common, link);
	dns_name_init(&tsig.algorithm, NULL);
	dns_name_clone(key->algorithm, &tsig.algorithm);

	isc_stdtime_get(&now);
	tsig.timesigned = now;
	tsig.fudge = DNS_TSIG_FUDGE;
	tsig.originalid = 50;
	tsig.error = dns_rcode_noerror;
	tsig.otherlen = 0;
	tsig.other = NULL;

	isc_buffer_init(&databuf, tsigbuf, sizeof(tsigbuf));
	isc_buffer_putuint48(&databuf, tsig.timesigned);
	isc_buffer_putuint16(&databuf, tsig.fudge);
	isc_buffer_usedregion(&databuf, &r);
	CHECK(dst_context_adddata(tsigctx, &r));

	CHECK(dst_key_sigsize(key->key, &sigsize));
	tsig.signature = isc_mem_get(mctx, sigsize);
	isc_buffer_init(&sigbuf, tsig.signature, sigsize);
	CHECK(dst_context_sign(tsigctx, &sigbuf));
	tsig.siglen = isc_buffer_usedlength(&sigbuf);
	assert_int_equal(sigsize, tsig.siglen);

	isc_buffer_allocate(mctx, &dynbuf, 512);
	CHECK(dns_rdata_fromstruct(&rdata, dns_rdataclass_any,
				   dns_rdatatype_tsig, &tsig, dynbuf));
	dns_rdatalist_init(&rdatalist);
	rdatalist.rdclass = dns_rdataclass_any;
	rdatalist.type = dns_rdatatype_tsig;
	ISC_LIST_APPEND(rdatalist.rdata, &rdata, link);
	dns_rdataset_init(&rdataset);
	CHECK(dns_rdatalist_tordataset(&rdatalist, &rdataset));
	CHECK(dns_rdataset_towire(&rdataset, &key->name, &cctx, target, 0,
				  &count));

	/*
	 * Fixup additional record count.
	 */
	((unsigned char *)target->base)[11]++;
	if (((unsigned char *)target->base)[11] == 0) {
		((unsigned char *)target->base)[10]++;
	}
cleanup:
	if (tsig.signature != NULL) {
		isc_mem_put(mctx, tsig.signature, sigsize);
	}
	if (dynbuf != NULL) {
		isc_buffer_free(&dynbuf);
	}
	if (invalidate_ctx) {
		dns_compress_invalidate(&cctx);
	}

	return (result);
}

static void
printmessage(dns_message_t *msg) {
	isc_buffer_t b;
	char *buf = NULL;
	int len = 1024;
	isc_result_t result = ISC_R_SUCCESS;

	if (!debug) {
		return;
	}

	do {
		buf = isc_mem_get(mctx, len);

		isc_buffer_init(&b, buf, len);
		result = dns_message_totext(msg, &dns_master_style_debug, 0,
					    &b);
		if (result == ISC_R_NOSPACE) {
			isc_mem_put(mctx, buf, len);
			len *= 2;
		} else if (result == ISC_R_SUCCESS) {
			printf("%.*s\n", (int)isc_buffer_usedlength(&b), buf);
		}
	} while (result == ISC_R_NOSPACE);

	if (buf != NULL) {
		isc_mem_put(mctx, buf, len);
	}
}

static void
render(isc_buffer_t *buf, unsigned flags, dns_tsigkey_t *key,
       isc_buffer_t **tsigin, isc_buffer_t **tsigout, dst_context_t *tsigctx) {
	dns_message_t *msg = NULL;
	dns_compress_t cctx;
	isc_result_t result;

	dns_message_create(mctx, DNS_MESSAGE_INTENTRENDER, &msg);
	assert_non_null(msg);

	msg->id = 50;
	msg->rcode = dns_rcode_noerror;
	msg->flags = flags;

	/*
	 * XXXMPA: this hack needs to be replaced with use of
	 * dns_message_reply() at some point.
	 */
	if ((flags & DNS_MESSAGEFLAG_QR) != 0) {
		msg->verified_sig = 1;
	}

	if (tsigin == tsigout) {
		msg->tcp_continuation = 1;
	}

	if (tsigctx == NULL) {
		result = dns_message_settsigkey(msg, key);
		assert_int_equal(result, ISC_R_SUCCESS);

		result = dns_message_setquerytsig(msg, *tsigin);
		assert_int_equal(result, ISC_R_SUCCESS);
	}

	result = dns_compress_init(&cctx, -1, mctx);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_message_renderbegin(msg, &cctx, buf);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_message_renderend(msg);
	assert_int_equal(result, ISC_R_SUCCESS);

	if (tsigctx != NULL) {
		isc_region_t r;

		isc_buffer_usedregion(buf, &r);
		result = dst_context_adddata(tsigctx, &r);
		assert_int_equal(result, ISC_R_SUCCESS);
	} else {
		if (tsigin == tsigout && *tsigin != NULL) {
			isc_buffer_free(tsigin);
		}

		result = dns_message_getquerytsig(msg, mctx, tsigout);
		assert_int_equal(result, ISC_R_SUCCESS);
	}

	dns_compress_invalidate(&cctx);
	dns_message_detach(&msg);
}

/*
 * Test tsig tcp-continuation validation:
 * Check that a simulated three message TCP sequence where the first
 * and last messages contain TSIGs but the intermediate message doesn't
 * correctly verifies.
 */
ISC_RUN_TEST_IMPL(tsig_tcp) {
	const dns_name_t *tsigowner = NULL;
	dns_fixedname_t fkeyname;
	dns_message_t *msg = NULL;
	dns_name_t *keyname;
	dns_tsig_keyring_t *ring = NULL;
	dns_tsigkey_t *key = NULL;
	isc_buffer_t *buf = NULL;
	isc_buffer_t *querytsig = NULL;
	isc_buffer_t *tsigin = NULL;
	isc_buffer_t *tsigout = NULL;
	isc_result_t result;
	unsigned char secret[16] = { 0 };
	dst_context_t *tsigctx = NULL;
	dst_context_t *outctx = NULL;

	UNUSED(state);

	/* isc_log_setdebuglevel(lctx, 99); */

	keyname = dns_fixedname_initname(&fkeyname);
	result = dns_name_fromstring(keyname, "test", 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_tsigkeyring_create(mctx, &ring);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_tsigkey_create(keyname, dns_tsig_hmacsha256_name, secret,
				    sizeof(secret), false, NULL, 0, 0, mctx,
				    ring, &key);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(key);

	/*
	 * Create request.
	 */
	isc_buffer_allocate(mctx, &buf, 65535);
	render(buf, 0, key, &tsigout, &querytsig, NULL);
	isc_buffer_free(&buf);

	/*
	 * Create response message 1.
	 */
	isc_buffer_allocate(mctx, &buf, 65535);
	render(buf, DNS_MESSAGEFLAG_QR, key, &querytsig, &tsigout, NULL);
	assert_non_null(tsigout);

	/*
	 * Process response message 1.
	 */
	dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &msg);
	assert_non_null(msg);

	result = dns_message_settsigkey(msg, key);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_message_parse(msg, buf, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	printmessage(msg);

	result = dns_message_setquerytsig(msg, querytsig);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_tsig_verify(buf, msg, NULL, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(msg->verified_sig, 1);
	assert_int_equal(msg->tsigstatus, dns_rcode_noerror);

	/*
	 * Check that we have a TSIG in the first message.
	 */
	assert_non_null(dns_message_gettsig(msg, &tsigowner));

	result = dns_message_getquerytsig(msg, mctx, &tsigin);
	assert_int_equal(result, ISC_R_SUCCESS);

	tsigctx = msg->tsigctx;
	msg->tsigctx = NULL;
	isc_buffer_free(&buf);
	dns_message_detach(&msg);

	result = dst_context_create(key->key, mctx, DNS_LOGCATEGORY_DNSSEC,
				    false, 0, &outctx);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(outctx);

	/*
	 * Start digesting.
	 */
	result = add_mac(outctx, tsigout);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Create response message 2.
	 */
	isc_buffer_allocate(mctx, &buf, 65535);

	assert_int_equal(result, ISC_R_SUCCESS);
	render(buf, DNS_MESSAGEFLAG_QR, key, &tsigout, &tsigout, outctx);

	/*
	 * Process response message 2.
	 */
	dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &msg);
	assert_non_null(msg);

	msg->tcp_continuation = 1;
	msg->tsigctx = tsigctx;
	tsigctx = NULL;

	result = dns_message_settsigkey(msg, key);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_message_parse(msg, buf, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	printmessage(msg);

	result = dns_message_setquerytsig(msg, tsigin);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_tsig_verify(buf, msg, NULL, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(msg->verified_sig, 0);
	assert_int_equal(msg->tsigstatus, dns_rcode_noerror);

	/*
	 * Check that we don't have a TSIG in the second message.
	 */
	tsigowner = NULL;
	assert_true(dns_message_gettsig(msg, &tsigowner) == NULL);

	tsigctx = msg->tsigctx;
	msg->tsigctx = NULL;
	isc_buffer_free(&buf);
	dns_message_detach(&msg);

	/*
	 * Create response message 3.
	 */
	isc_buffer_allocate(mctx, &buf, 65535);
	render(buf, DNS_MESSAGEFLAG_QR, key, &tsigout, &tsigout, outctx);

	result = add_tsig(outctx, key, buf);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Process response message 3.
	 */
	dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &msg);
	assert_non_null(msg);

	msg->tcp_continuation = 1;
	msg->tsigctx = tsigctx;
	tsigctx = NULL;

	result = dns_message_settsigkey(msg, key);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_message_parse(msg, buf, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	printmessage(msg);

	/*
	 * Check that we had a TSIG in the third message.
	 */
	assert_non_null(dns_message_gettsig(msg, &tsigowner));

	result = dns_message_setquerytsig(msg, tsigin);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dns_tsig_verify(buf, msg, NULL, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(msg->verified_sig, 1);
	assert_int_equal(msg->tsigstatus, dns_rcode_noerror);

	if (tsigin != NULL) {
		isc_buffer_free(&tsigin);
	}

	result = dns_message_getquerytsig(msg, mctx, &tsigin);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_buffer_free(&buf);
	dns_message_detach(&msg);

	if (outctx != NULL) {
		dst_context_destroy(&outctx);
	}
	if (querytsig != NULL) {
		isc_buffer_free(&querytsig);
	}
	if (tsigin != NULL) {
		isc_buffer_free(&tsigin);
	}
	if (tsigout != NULL) {
		isc_buffer_free(&tsigout);
	}
	dns_tsigkey_detach(&key);
	if (ring != NULL) {
		dns_tsigkeyring_detach(&ring);
	}
}

/* Tests the dns__tsig_algvalid function */
ISC_RUN_TEST_IMPL(algvalid) {
	UNUSED(state);

	assert_true(dns__tsig_algvalid(DST_ALG_HMACMD5));

	assert_true(dns__tsig_algvalid(DST_ALG_HMACSHA1));
	assert_true(dns__tsig_algvalid(DST_ALG_HMACSHA224));
	assert_true(dns__tsig_algvalid(DST_ALG_HMACSHA256));
	assert_true(dns__tsig_algvalid(DST_ALG_HMACSHA384));
	assert_true(dns__tsig_algvalid(DST_ALG_HMACSHA512));

	assert_false(dns__tsig_algvalid(DST_ALG_GSSAPI));
}

/* Tests the dns__tsig_algfromname function */
ISC_RUN_TEST_IMPL(algfromname) {
	UNUSED(state);

	assert_int_equal(dns__tsig_algfromname(DNS_TSIG_HMACMD5_NAME),
			 DST_ALG_HMACMD5);
	assert_int_equal(dns__tsig_algfromname(DNS_TSIG_HMACSHA1_NAME),
			 DST_ALG_HMACSHA1);
	assert_int_equal(dns__tsig_algfromname(DNS_TSIG_HMACSHA224_NAME),
			 DST_ALG_HMACSHA224);
	assert_int_equal(dns__tsig_algfromname(DNS_TSIG_HMACSHA256_NAME),
			 DST_ALG_HMACSHA256);
	assert_int_equal(dns__tsig_algfromname(DNS_TSIG_HMACSHA384_NAME),
			 DST_ALG_HMACSHA384);
	assert_int_equal(dns__tsig_algfromname(DNS_TSIG_HMACSHA512_NAME),
			 DST_ALG_HMACSHA512);

	assert_int_equal(dns__tsig_algfromname(DNS_TSIG_GSSAPI_NAME),
			 DST_ALG_GSSAPI);
	assert_int_equal(dns__tsig_algfromname(DNS_TSIG_GSSAPIMS_NAME),
			 DST_ALG_GSSAPI);

	assert_int_equal(dns__tsig_algfromname(dns_rootname), 0);
}

/* Tests the dns__tsig_algnamefromname function */

/*
 * Helper function to create a dns_name_t from a string and see if
 * the dns__tsig_algnamefromname function can correctly match it against the
 * static table of known algorithms.
 */
static void
test_name(const char *name_string, const dns_name_t *expected) {
	dns_name_t name;
	dns_name_init(&name, NULL);
	assert_int_equal(dns_name_fromstring(&name, name_string, 0, mctx),
			 ISC_R_SUCCESS);
	assert_ptr_equal(dns__tsig_algnamefromname(&name), expected);
	dns_name_free(&name, mctx);
}

ISC_RUN_TEST_IMPL(algnamefromname) {
	UNUSED(state);

	/* test the standard algorithms */
	test_name("hmac-md5.sig-alg.reg.int", DNS_TSIG_HMACMD5_NAME);
	test_name("hmac-sha1", DNS_TSIG_HMACSHA1_NAME);
	test_name("hmac-sha224", DNS_TSIG_HMACSHA224_NAME);
	test_name("hmac-sha256", DNS_TSIG_HMACSHA256_NAME);
	test_name("hmac-sha384", DNS_TSIG_HMACSHA384_NAME);
	test_name("hmac-sha512", DNS_TSIG_HMACSHA512_NAME);

	test_name("gss-tsig", DNS_TSIG_GSSAPI_NAME);
	test_name("gss.microsoft.com", DNS_TSIG_GSSAPIMS_NAME);

	/* try another name that isn't a standard algorithm name */
	assert_null(dns__tsig_algnamefromname(dns_rootname));
}

/* Tests the dns__tsig_algallocated function */
ISC_RUN_TEST_IMPL(algallocated) {
	UNUSED(state);

	/* test the standard algorithms */
	assert_false(dns__tsig_algallocated(DNS_TSIG_HMACMD5_NAME));
	assert_false(dns__tsig_algallocated(DNS_TSIG_HMACSHA1_NAME));
	assert_false(dns__tsig_algallocated(DNS_TSIG_HMACSHA224_NAME));
	assert_false(dns__tsig_algallocated(DNS_TSIG_HMACSHA256_NAME));
	assert_false(dns__tsig_algallocated(DNS_TSIG_HMACSHA384_NAME));
	assert_false(dns__tsig_algallocated(DNS_TSIG_HMACSHA512_NAME));

	assert_false(dns__tsig_algallocated(DNS_TSIG_HMACSHA512_NAME));
	assert_false(dns__tsig_algallocated(DNS_TSIG_HMACSHA512_NAME));

	/* try another name that isn't a standard algorithm name */
	assert_true(dns__tsig_algallocated(dns_rootname));
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(tsig_tcp, setup_test, teardown_test)
ISC_TEST_ENTRY(algvalid)
ISC_TEST_ENTRY(algfromname)
ISC_TEST_ENTRY_CUSTOM(algnamefromname, setup_test, teardown_test)
ISC_TEST_ENTRY(algallocated)
ISC_TEST_LIST_END

ISC_TEST_MAIN
