/*	$NetBSD: tsig_test.c,v 1.1.1.1 2018/08/12 12:08:21 christos Exp $	*/

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

#include <isc/mem.h>
#include <isc/print.h>

#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/tsig.h>

#include "../tsig_p.h"

#include "dnstest.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h> /* uintptr_t */
#endif

#define TEST_ORIGIN	"test"

/*
 * Individual unit tests
 */

static int debug = 0;

static isc_result_t
add_mac(dst_context_t *tsigctx, isc_buffer_t *buf) {
	dns_rdata_any_tsig_t tsig;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_buffer_t databuf;
	isc_region_t r;
	isc_result_t result;
	unsigned char tsigbuf[1024];

	isc_buffer_usedregion(buf, &r);
	dns_rdata_fromregion(&rdata, dns_rdataclass_any,
			     dns_rdatatype_tsig, &r);
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
	isc_boolean_t invalidate_ctx = ISC_FALSE;

	memset(&tsig, 0, sizeof(tsig));

	CHECK(dns_compress_init(&cctx, -1, mctx));
	invalidate_ctx = ISC_TRUE;

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
	tsig.signature = (unsigned char *) isc_mem_get(mctx, sigsize);
	if (tsig.signature == NULL)
		CHECK(ISC_R_NOMEMORY);
	isc_buffer_init(&sigbuf, tsig.signature, sigsize);
	CHECK(dst_context_sign(tsigctx, &sigbuf));
	tsig.siglen = isc_buffer_usedlength(&sigbuf);
	ATF_CHECK_EQ(sigsize, tsig.siglen);

	CHECK(isc_buffer_allocate(mctx, &dynbuf, 512));
	CHECK(dns_rdata_fromstruct(&rdata, dns_rdataclass_any,
				   dns_rdatatype_tsig, &tsig, dynbuf));
	dns_rdatalist_init(&rdatalist);
	rdatalist.rdclass = dns_rdataclass_any;
	rdatalist.type = dns_rdatatype_tsig;
	ISC_LIST_APPEND(rdatalist.rdata, &rdata, link);
	dns_rdataset_init(&rdataset);
	CHECK(dns_rdatalist_tordataset(&rdatalist, &rdataset));
	CHECK(dns_rdataset_towire(&rdataset, &key->name, &cctx,
				  target, 0, &count));

	/*
	 * Fixup additional record count.
	 */
	((unsigned char*)target->base)[11]++;
	if (((unsigned char*)target->base)[11] == 0)
		((unsigned char*)target->base)[10]++;
 cleanup:
	if (tsig.signature != NULL)
		isc_mem_put(mctx, tsig.signature, sigsize);
	if (dynbuf != NULL)
		isc_buffer_free(&dynbuf);
	if (invalidate_ctx)
		dns_compress_invalidate(&cctx);

	return (result);
}

static void
printmessage(dns_message_t *msg) {
	isc_buffer_t b;
	char *buf = NULL;
	int len = 1024;
	isc_result_t result = ISC_R_SUCCESS;

	if (!debug)
		return;

	do {
		buf = isc_mem_get(mctx, len);
		if (buf == NULL)
			return;

		isc_buffer_init(&b, buf, len);
		result = dns_message_totext(msg, &dns_master_style_debug,
					    0, &b);
		if (result == ISC_R_NOSPACE) {
			isc_mem_put(mctx, buf, len);
			len *= 2;
		} else if (result == ISC_R_SUCCESS)
			printf("%.*s\n", (int) isc_buffer_usedlength(&b), buf);
	} while (result == ISC_R_NOSPACE);

	if (buf != NULL)
		isc_mem_put(mctx, buf, len);
}

static void
render(isc_buffer_t *buf, unsigned flags, dns_tsigkey_t *key,
       isc_buffer_t **tsigin, isc_buffer_t **tsigout,
       dst_context_t *tsigctx)
{
	dns_message_t *msg = NULL;
	dns_compress_t cctx;
	isc_result_t result;

	result = dns_message_create(mctx, DNS_MESSAGE_INTENTRENDER, &msg);
	ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS,
			 "dns_message_create: %s",
			 dns_result_totext(result));
	ATF_REQUIRE(msg != NULL);

	msg->id = 50;
	msg->rcode = dns_rcode_noerror;
	msg->flags = flags;

	/*
	 * XXXMPA: this hack needs to be replaced with use of
	 * dns_message_reply() at some point.
	 */
	if ((flags & DNS_MESSAGEFLAG_QR) != 0)
		msg->verified_sig = 1;

	if (tsigin == tsigout)
		msg->tcp_continuation = 1;

	if (tsigctx == NULL) {
		result = dns_message_settsigkey(msg, key);
		ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS,
				 "dns_message_settsigkey: %s",
				 dns_result_totext(result));

		result = dns_message_setquerytsig(msg, *tsigin);
		ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS,
				 "dns_message_setquerytsig: %s",
				 dns_result_totext(result));
	}

	result = dns_compress_init(&cctx, -1, mctx);
	ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS,
			 "dns_compress_init: %s",
			 dns_result_totext(result));

	result = dns_message_renderbegin(msg, &cctx, buf);
	ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS,
			 "dns_message_renderbegin: %s",
			 dns_result_totext(result));

	result = dns_message_renderend(msg);
	ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS,
			 "dns_message_renderend: %s",
			 dns_result_totext(result));

	if (tsigctx != NULL) {
		isc_region_t r;

		isc_buffer_usedregion(buf, &r);
		result = dst_context_adddata(tsigctx, &r);
		ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS,
				 "dst_context_adddata: %s",
				 dns_result_totext(result));
	} else {
		if (tsigin == tsigout && *tsigin != NULL)
			isc_buffer_free(tsigin);

		result = dns_message_getquerytsig(msg, mctx, tsigout);
		ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS,
				 "dns_message_getquerytsig: %s",
				 dns_result_totext(result));
	}

	dns_compress_invalidate(&cctx);
	dns_message_destroy(&msg);
}

/*
 * Check that a simulated three message TCP sequence where the first
 * and last messages contain TSIGs but the intermediate message doesn't
 * correctly verifies.
 */
ATF_TC(tsig_tcp);
ATF_TC_HEAD(tsig_tcp, tc) {
	atf_tc_set_md_var(tc, "descr", "test tsig tcp-continuation validation");
}
ATF_TC_BODY(tsig_tcp, tc) {
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

	UNUSED(tc);

	result = dns_test_begin(stderr, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* isc_log_setdebuglevel(lctx, 99); */

	keyname = dns_fixedname_initname(&fkeyname);
	result = dns_name_fromstring(keyname, "test", 0, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_tsigkeyring_create(mctx, &ring);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dns_tsigkey_create(keyname, dns_tsig_hmacsha256_name,
				    secret, sizeof(secret), ISC_FALSE,
				    NULL, 0, 0, mctx, ring, &key);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE(key != NULL);

	/*
	 * Create request.
	 */
	result = isc_buffer_allocate(mctx, &buf, 65535);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	render(buf, 0, key, &tsigout, &querytsig, NULL);
	isc_buffer_free(&buf);

	/*
	 * Create response message 1.
	 */
	result = isc_buffer_allocate(mctx, &buf, 65535);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	render(buf, DNS_MESSAGEFLAG_QR, key, &querytsig, &tsigout, NULL);

	/*
	 * Process response message 1.
	 */
	result = dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &msg);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS, "dns_message_create: %s",
			   dns_result_totext(result));
	ATF_REQUIRE(msg != NULL);

	result = dns_message_settsigkey(msg, key);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS, "dns_message_settsigkey: %s",
			   dns_result_totext(result));

	result = dns_message_parse(msg, buf, 0);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS, "dns_message_parse: %s",
			   dns_result_totext(result));

	printmessage(msg);

	result = dns_message_setquerytsig(msg, querytsig);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS,
			   "dns_message_setquerytsig: %s",
			   dns_result_totext(result));

	result = dns_tsig_verify(buf, msg, NULL, NULL);
	ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS,
			 "dns_tsig_verify: %s",
			 dns_result_totext(result));
	ATF_CHECK_EQ(msg->verified_sig, 1);
	ATF_CHECK_EQ(msg->tsigstatus, dns_rcode_noerror);

	/*
	 * Check that we have a TSIG in the first message.
	 */
	ATF_REQUIRE(dns_message_gettsig(msg, &tsigowner) != NULL);

	result = dns_message_getquerytsig(msg, mctx, &tsigin);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS,
			   "dns_message_getquerytsig: %s",
			   dns_result_totext(result));

	tsigctx = msg->tsigctx;
	msg->tsigctx = NULL;
	isc_buffer_free(&buf);
	dns_message_destroy(&msg);

	result = dst_context_create3(key->key, mctx, DNS_LOGCATEGORY_DNSSEC,
				     ISC_FALSE, &outctx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE(outctx != NULL);

	/*
	 * Start digesting.
	 */
	result = add_mac(outctx, tsigout);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Create response message 2.
	 */
	result = isc_buffer_allocate(mctx, &buf, 65535);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	render(buf, DNS_MESSAGEFLAG_QR, key, &tsigout, &tsigout, outctx);

	/*
	 * Process response message 2.
	 */
	result = dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &msg);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS, "dns_message_create: %s",
			   dns_result_totext(result));
	ATF_REQUIRE(msg != NULL);

	msg->tcp_continuation = 1;
	msg->tsigctx = tsigctx;
	tsigctx = NULL;

	result = dns_message_settsigkey(msg, key);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS, "dns_message_settsigkey: %s",
			   dns_result_totext(result));

	result = dns_message_parse(msg, buf, 0);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS, "dns_message_parse: %s",
			   dns_result_totext(result));

	printmessage(msg);

	result = dns_message_setquerytsig(msg, tsigin);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS,
			   "dns_message_setquerytsig: %s",
			   dns_result_totext(result));

	result = dns_tsig_verify(buf, msg, NULL, NULL);
	ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS,
			 "dns_tsig_verify: %s",
			 dns_result_totext(result));
	ATF_CHECK_EQ(msg->verified_sig, 0);
	ATF_CHECK_EQ(msg->tsigstatus, dns_rcode_noerror);

	/*
	 * Check that we don't have a TSIG in the second message.
	 */
	tsigowner = NULL;
	ATF_REQUIRE(dns_message_gettsig(msg, &tsigowner) == NULL);

	tsigctx = msg->tsigctx;
	msg->tsigctx = NULL;
	isc_buffer_free(&buf);
	dns_message_destroy(&msg);

	/*
	 * Create response message 3.
	 */
	result = isc_buffer_allocate(mctx, &buf, 65535);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	render(buf, DNS_MESSAGEFLAG_QR, key, &tsigout, &tsigout, outctx);

	result = add_tsig(outctx, key, buf);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS, "add_tsig: %s",
			   dns_result_totext(result));

	/*
	 * Process response message 3.
	 */
	result = dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &msg);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS, "dns_message_create: %s",
			   dns_result_totext(result));
	ATF_REQUIRE(msg != NULL);

	msg->tcp_continuation = 1;
	msg->tsigctx = tsigctx;
	tsigctx = NULL;

	result = dns_message_settsigkey(msg, key);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS, "dns_message_settsigkey: %s",
			   dns_result_totext(result));

	result = dns_message_parse(msg, buf, 0);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS, "dns_message_parse: %s",
			   dns_result_totext(result));

	printmessage(msg);

	/*
	 * Check that we had a TSIG in the third message.
	 */
	ATF_REQUIRE(dns_message_gettsig(msg, &tsigowner) != NULL);

	result = dns_message_setquerytsig(msg, tsigin);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS,
			   "dns_message_setquerytsig: %s",
			   dns_result_totext(result));

	result = dns_tsig_verify(buf, msg, NULL, NULL);
	ATF_CHECK_EQ_MSG(result, ISC_R_SUCCESS,
			 "dns_tsig_verify: %s",
			 dns_result_totext(result));
	ATF_CHECK_EQ(msg->verified_sig, 1);
	ATF_CHECK_EQ(msg->tsigstatus, dns_rcode_noerror);

	if (tsigin != NULL)
		isc_buffer_free(&tsigin);

	result = dns_message_getquerytsig(msg, mctx, &tsigin);
	ATF_REQUIRE_EQ_MSG(result, ISC_R_SUCCESS,
			   "dns_message_getquerytsig: %s",
			   dns_result_totext(result));

	isc_buffer_free(&buf);
	dns_message_destroy(&msg);

	if (outctx != NULL)
		dst_context_destroy(&outctx);
	if (querytsig != NULL)
		isc_buffer_free(&querytsig);
	if (tsigin != NULL)
		isc_buffer_free(&tsigin);
	if (tsigout != NULL)
		isc_buffer_free(&tsigout);
	dns_tsigkey_detach(&key);
	if (ring != NULL)
		dns_tsigkeyring_detach(&ring);
	dns_test_end();
}

ATF_TC(algvalid);
ATF_TC_HEAD(algvalid, tc) {
	atf_tc_set_md_var(tc, "descr", "Tests the dns__tsig_algvalid function");
}
ATF_TC_BODY(algvalid, tc) {
	UNUSED(tc);

#ifndef PK11_MD5_DISABLE
	ATF_REQUIRE_EQ(dns__tsig_algvalid(DST_ALG_HMACMD5), ISC_TRUE);
#else
	ATF_REQUIRE_EQ(dns__tsig_algvalid(DST_ALG_HMACMD5), ISC_FALSE);
#endif

	ATF_REQUIRE_EQ(dns__tsig_algvalid(DST_ALG_HMACSHA1), ISC_TRUE);
	ATF_REQUIRE_EQ(dns__tsig_algvalid(DST_ALG_HMACSHA224), ISC_TRUE);
	ATF_REQUIRE_EQ(dns__tsig_algvalid(DST_ALG_HMACSHA256), ISC_TRUE);
	ATF_REQUIRE_EQ(dns__tsig_algvalid(DST_ALG_HMACSHA384), ISC_TRUE);
	ATF_REQUIRE_EQ(dns__tsig_algvalid(DST_ALG_HMACSHA512), ISC_TRUE);

	ATF_REQUIRE_EQ(dns__tsig_algvalid(DST_ALG_GSSAPI), ISC_FALSE);
}

ATF_TC(algfromname);
ATF_TC_HEAD(algfromname, tc) {
	atf_tc_set_md_var(tc, "descr", "Tests the dns__tsig_algfromname function");
}
ATF_TC_BODY(algfromname, tc) {
	UNUSED(tc);

#ifndef PK11_MD5_DISABLE
	ATF_REQUIRE_EQ(dns__tsig_algfromname(DNS_TSIG_HMACMD5_NAME), DST_ALG_HMACMD5);
#endif

	ATF_REQUIRE_EQ(dns__tsig_algfromname(DNS_TSIG_HMACSHA1_NAME), DST_ALG_HMACSHA1);
	ATF_REQUIRE_EQ(dns__tsig_algfromname(DNS_TSIG_HMACSHA224_NAME), DST_ALG_HMACSHA224);
	ATF_REQUIRE_EQ(dns__tsig_algfromname(DNS_TSIG_HMACSHA256_NAME), DST_ALG_HMACSHA256);
	ATF_REQUIRE_EQ(dns__tsig_algfromname(DNS_TSIG_HMACSHA384_NAME), DST_ALG_HMACSHA384);
	ATF_REQUIRE_EQ(dns__tsig_algfromname(DNS_TSIG_HMACSHA512_NAME), DST_ALG_HMACSHA512);

	ATF_REQUIRE_EQ(dns__tsig_algfromname(DNS_TSIG_GSSAPI_NAME), DST_ALG_GSSAPI);
	ATF_REQUIRE_EQ(dns__tsig_algfromname(DNS_TSIG_GSSAPIMS_NAME), DST_ALG_GSSAPI);

	ATF_REQUIRE_EQ(dns__tsig_algfromname(dns_rootname), 0);
}

ATF_TC(algnamefromname);
ATF_TC_HEAD(algnamefromname, tc) {
	atf_tc_set_md_var(tc, "descr", "Tests the dns__tsig_algnamefromname function");
}

/*
 * Helper function to create a dns_name_t from a string and see if
 * the dns__tsig_algnamefromname function can correctly match it against the
 * static table of known algorithms.
 */
static void test_name(const char *name_string, const dns_name_t *expected) {
	dns_name_t	name;
	dns_name_init(&name, NULL);
	ATF_CHECK_EQ(dns_name_fromstring(&name, name_string, 0, mctx), ISC_R_SUCCESS);
	ATF_REQUIRE_EQ_MSG(dns__tsig_algnamefromname(&name), expected, "%s", name_string);
	dns_name_free(&name, mctx);
}

ATF_TC_BODY(algnamefromname, tc) {
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(stderr, ISC_TRUE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* test the standard algorithms */
#ifndef PK11_MD5_DISABLE
	test_name("hmac-md5.sig-alg.reg.int", DNS_TSIG_HMACMD5_NAME);
#endif
	test_name("hmac-sha1", DNS_TSIG_HMACSHA1_NAME);
	test_name("hmac-sha224", DNS_TSIG_HMACSHA224_NAME);
	test_name("hmac-sha256", DNS_TSIG_HMACSHA256_NAME);
	test_name("hmac-sha384", DNS_TSIG_HMACSHA384_NAME);
	test_name("hmac-sha512", DNS_TSIG_HMACSHA512_NAME);

	test_name("gss-tsig", DNS_TSIG_GSSAPI_NAME);
	test_name("gss.microsoft.com", DNS_TSIG_GSSAPIMS_NAME);

	/* try another name that isn't a standard algorithm name */
	ATF_REQUIRE_EQ(dns__tsig_algnamefromname(dns_rootname), NULL);

	/* cleanup */
	dns_test_end();
}

ATF_TC(algallocated);
ATF_TC_HEAD(algallocated, tc) {
	atf_tc_set_md_var(tc, "descr", "Tests the dns__tsig_algallocated function");
}
ATF_TC_BODY(algallocated, tc) {

	/* test the standard algorithms */
#ifndef PK11_MD5_DISABLE
	ATF_REQUIRE_EQ(dns__tsig_algallocated(DNS_TSIG_HMACMD5_NAME), ISC_FALSE);
#endif

	ATF_REQUIRE_EQ(dns__tsig_algallocated(DNS_TSIG_HMACSHA1_NAME), ISC_FALSE);
	ATF_REQUIRE_EQ(dns__tsig_algallocated(DNS_TSIG_HMACSHA224_NAME), ISC_FALSE);
	ATF_REQUIRE_EQ(dns__tsig_algallocated(DNS_TSIG_HMACSHA256_NAME), ISC_FALSE);
	ATF_REQUIRE_EQ(dns__tsig_algallocated(DNS_TSIG_HMACSHA384_NAME), ISC_FALSE);
	ATF_REQUIRE_EQ(dns__tsig_algallocated(DNS_TSIG_HMACSHA512_NAME), ISC_FALSE);

	ATF_REQUIRE_EQ(dns__tsig_algallocated(DNS_TSIG_HMACSHA512_NAME), ISC_FALSE);
	ATF_REQUIRE_EQ(dns__tsig_algallocated(DNS_TSIG_HMACSHA512_NAME), ISC_FALSE);

	/* try another name that isn't a standard algorithm name */
	ATF_REQUIRE_EQ(dns__tsig_algallocated(dns_rootname), ISC_TRUE);
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, tsig_tcp);
	ATF_TP_ADD_TC(tp, algvalid);
	ATF_TP_ADD_TC(tp, algfromname);
	ATF_TP_ADD_TC(tp, algnamefromname);
	ATF_TP_ADD_TC(tp, algallocated);

	return (atf_no_error());
}
