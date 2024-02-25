/*	$NetBSD: dst_test.c,v 1.2.2.2 2024/02/25 15:47:39 martin Exp $	*/

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

/*
 * As a workaround, include an OpenSSL header file before including cmocka.h,
 * because OpenSSL 3.1.0 uses __attribute__(malloc), conflicting with a
 * redefined malloc in cmocka.h.
 */
#include <openssl/err.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/file.h>
#include <isc/hex.h>
#include <isc/result.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dst/dst.h>

#include "dst_internal.h"

#include <tests/dns.h>

static int
setup_test(void **state) {
	UNUSED(state);

	dst_lib_init(mctx, NULL);

	return (0);
}

static int
teardown_test(void **state) {
	UNUSED(state);

	dst_lib_destroy();

	return (0);
}

/* Read sig in file at path to buf. Check signature ineffability */
static isc_result_t
sig_fromfile(const char *path, isc_buffer_t *buf) {
	isc_result_t result;
	size_t rval, len;
	FILE *fp = NULL;
	unsigned char val;
	char *p, *data;
	off_t size;

	result = isc_stdio_open(path, "rb", &fp);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_file_getsizefd(fileno(fp), &size);
	assert_int_equal(result, ISC_R_SUCCESS);

	data = isc_mem_get(mctx, (size + 1));
	assert_non_null(data);

	len = (size_t)size;
	p = data;
	while (len != 0U) {
		result = isc_stdio_read(p, 1, len, fp, &rval);
		assert_int_equal(result, ISC_R_SUCCESS);
		len -= rval;
		p += rval;
	}
	isc_stdio_close(fp);

	p = data;
	len = size;
	while (len > 0U) {
		if ((*p == '\r') || (*p == '\n')) {
			++p;
			--len;
			continue;
		} else if (len < 2U) {
			goto err;
		}
		if (('0' <= *p) && (*p <= '9')) {
			val = *p - '0';
		} else if (('A' <= *p) && (*p <= 'F')) {
			val = *p - 'A' + 10;
		} else {
			result = ISC_R_BADHEX;
			goto err;
		}
		++p;
		val <<= 4;
		--len;
		if (('0' <= *p) && (*p <= '9')) {
			val |= (*p - '0');
		} else if (('A' <= *p) && (*p <= 'F')) {
			val |= (*p - 'A' + 10);
		} else {
			result = ISC_R_BADHEX;
			goto err;
		}
		++p;
		--len;
		isc_buffer_putuint8(buf, val);
	}

	result = ISC_R_SUCCESS;

err:
	isc_mem_put(mctx, data, size + 1);
	return (result);
}

static void
check_sig(const char *datapath, const char *sigpath, const char *keyname,
	  dns_keytag_t id, dns_secalg_t alg, int type, bool expect) {
	isc_result_t result;
	size_t rval, len;
	FILE *fp;
	dst_key_t *key = NULL;
	unsigned char sig[512];
	unsigned char *p;
	unsigned char *data;
	off_t size;
	isc_buffer_t b;
	isc_buffer_t databuf, sigbuf;
	isc_region_t datareg, sigreg;
	dns_fixedname_t fname;
	dns_name_t *name;
	dst_context_t *ctx = NULL;

	/*
	 * Read data from file in a form usable by dst_verify.
	 */
	result = isc_stdio_open(datapath, "rb", &fp);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_file_getsizefd(fileno(fp), &size);
	assert_int_equal(result, ISC_R_SUCCESS);

	data = isc_mem_get(mctx, (size + 1));
	assert_non_null(data);

	p = data;
	len = (size_t)size;
	do {
		result = isc_stdio_read(p, 1, len, fp, &rval);
		assert_int_equal(result, ISC_R_SUCCESS);
		len -= rval;
		p += rval;
	} while (len);
	isc_stdio_close(fp);

	/*
	 * Read key from file in a form usable by dst_verify.
	 */
	name = dns_fixedname_initname(&fname);
	isc_buffer_constinit(&b, keyname, strlen(keyname));
	isc_buffer_add(&b, strlen(keyname));
	result = dns_name_fromtext(name, &b, dns_rootname, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dst_key_fromfile(name, id, alg, type,
				  TESTS_DIR "/testdata/dst", mctx, &key);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_buffer_init(&databuf, data, (unsigned int)size);
	isc_buffer_add(&databuf, (unsigned int)size);
	isc_buffer_usedregion(&databuf, &datareg);

	memset(sig, 0, sizeof(sig));
	isc_buffer_init(&sigbuf, sig, sizeof(sig));

	/*
	 * Read precomputed signature from file in a form usable by dst_verify.
	 */
	result = sig_fromfile(sigpath, &sigbuf);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Verify that the key signed the data.
	 */
	isc_buffer_remainingregion(&sigbuf, &sigreg);

	result = dst_context_create(key, mctx, DNS_LOGCATEGORY_GENERAL, false,
				    0, &ctx);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dst_context_adddata(ctx, &datareg);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dst_context_verify(ctx, &sigreg);

	/*
	 * Compute the expected signature and emit it
	 * so the precomputed signature can be updated.
	 * This should only be done if the covered data
	 * is updated.
	 */
	if (expect && result != ISC_R_SUCCESS) {
		isc_result_t result2;

		dst_context_destroy(&ctx);
		result2 = dst_context_create(key, mctx, DNS_LOGCATEGORY_GENERAL,
					     false, 0, &ctx);
		assert_int_equal(result2, ISC_R_SUCCESS);

		result2 = dst_context_adddata(ctx, &datareg);
		assert_int_equal(result2, ISC_R_SUCCESS);

		char sigbuf2[4096];
		isc_buffer_t sigb;
		isc_buffer_init(&sigb, sigbuf2, sizeof(sigbuf2));

		result2 = dst_context_sign(ctx, &sigb);
		assert_int_equal(result2, ISC_R_SUCCESS);

		isc_region_t r;
		isc_buffer_usedregion(&sigb, &r);

		char hexbuf[4096] = { 0 };
		isc_buffer_t hb;
		isc_buffer_init(&hb, hexbuf, sizeof(hexbuf));

		isc_hex_totext(&r, 0, "", &hb);

		fprintf(stderr, "# %s:\n# %s\n", sigpath, hexbuf);
	}

	isc_mem_put(mctx, data, size + 1);
	dst_context_destroy(&ctx);
	dst_key_free(&key);

	assert_true((expect && (result == ISC_R_SUCCESS)) ||
		    (!expect && (result != ISC_R_SUCCESS)));

	return;
}

ISC_RUN_TEST_IMPL(sig_test) {
	struct {
		const char *datapath;
		const char *sigpath;
		const char *keyname;
		dns_keytag_t keyid;
		dns_secalg_t alg;
		bool expect;
	} testcases[] = {
		{ TESTS_DIR "/testdata/dst/test1.data",
		  TESTS_DIR "/testdata/dst/test1.ecdsa256sig", "test.", 49130,
		  DST_ALG_ECDSA256, true },
		{ TESTS_DIR "/testdata/dst/test1.data",
		  TESTS_DIR "/testdata/dst/test1.rsasha256sig", "test.", 11349,
		  DST_ALG_RSASHA256, true },
		{ /* wrong sig */
		  TESTS_DIR "/testdata/dst/test1.data",
		  TESTS_DIR "/testdata/dst/test1.ecdsa256sig", "test.", 11349,
		  DST_ALG_RSASHA256, false },
		{ /* wrong data */
		  TESTS_DIR "/testdata/dst/test2.data",
		  TESTS_DIR "/testdata/dst/test1.ecdsa256sig", "test.", 49130,
		  DST_ALG_ECDSA256, false },
	};
	unsigned int i;

	for (i = 0; i < (sizeof(testcases) / sizeof(testcases[0])); i++) {
		if (!dst_algorithm_supported(testcases[i].alg)) {
			continue;
		}

		check_sig(testcases[i].datapath, testcases[i].sigpath,
			  testcases[i].keyname, testcases[i].keyid,
			  testcases[i].alg, DST_TYPE_PRIVATE | DST_TYPE_PUBLIC,
			  testcases[i].expect);
	}
}

static void
check_cmp(const char *key1_name, dns_keytag_t key1_id, const char *key2_name,
	  dns_keytag_t key2_id, dns_secalg_t alg, int type, bool expect) {
	isc_result_t result;
	dst_key_t *key1 = NULL;
	dst_key_t *key2 = NULL;
	isc_buffer_t b1;
	isc_buffer_t b2;
	dns_fixedname_t fname1;
	dns_fixedname_t fname2;
	dns_name_t *name1;
	dns_name_t *name2;

	/*
	 * Read key1 from the file.
	 */
	name1 = dns_fixedname_initname(&fname1);
	isc_buffer_constinit(&b1, key1_name, strlen(key1_name));
	isc_buffer_add(&b1, strlen(key1_name));
	result = dns_name_fromtext(name1, &b1, dns_rootname, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dst_key_fromfile(name1, key1_id, alg, type,
				  TESTS_DIR "/comparekeys", mctx, &key1);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Read key2 from the file.
	 */
	name2 = dns_fixedname_initname(&fname2);
	isc_buffer_constinit(&b2, key2_name, strlen(key2_name));
	isc_buffer_add(&b2, strlen(key2_name));
	result = dns_name_fromtext(name2, &b2, dns_rootname, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dst_key_fromfile(name2, key2_id, alg, type,
				  TESTS_DIR "/comparekeys", mctx, &key2);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Compare the keys (for public-only keys).
	 */
	if ((type & DST_TYPE_PRIVATE) == 0) {
		assert_true(dst_key_pubcompare(key1, key2, false) == expect);
	}

	/*
	 * Compare the keys (for both public-only keys and keypairs).
	 */
	assert_true(dst_key_compare(key1, key2) == expect);

	/*
	 * Free the keys
	 */
	dst_key_free(&key2);
	dst_key_free(&key1);

	return;
}

ISC_RUN_TEST_IMPL(cmp_test) {
	struct {
		const char *key1_name;
		dns_keytag_t key1_id;
		const char *key2_name;
		dns_keytag_t key2_id;
		dns_secalg_t alg;
		int type;
		bool expect;
	} testcases[] = {
		/* RSA Keypair: self */
		{ "example.", 53461, "example.", 53461, DST_ALG_RSASHA256,
		  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE, true },

		/* RSA Keypair: different key */
		{ "example.", 53461, "example2.", 37993, DST_ALG_RSASHA256,
		  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE, false },

		/* RSA Keypair: different PublicExponent (e) */
		{ "example.", 53461, "example-e.", 53973, DST_ALG_RSASHA256,
		  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE, false },

		/* RSA Keypair: different Modulus (n) */
		{ "example.", 53461, "example-n.", 37464, DST_ALG_RSASHA256,
		  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE, false },

		/* RSA Keypair: different PrivateExponent (d) */
		{ "example.", 53461, "example-d.", 53461, DST_ALG_RSASHA256,
		  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE, false },

		/* RSA Keypair: different Prime1 (p) */
		{ "example.", 53461, "example-p.", 53461, DST_ALG_RSASHA256,
		  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE, false },

		/* RSA Keypair: different Prime2 (q) */
		{ "example.", 53461, "example-q.", 53461, DST_ALG_RSASHA256,
		  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE, false },

		/* RSA Public Key: self */
		{ "example.", 53461, "example.", 53461, DST_ALG_RSASHA256,
		  DST_TYPE_PUBLIC, true },

		/* RSA Public Key: different key */
		{ "example.", 53461, "example2.", 37993, DST_ALG_RSASHA256,
		  DST_TYPE_PUBLIC, false },

		/* RSA Public Key: different PublicExponent (e) */
		{ "example.", 53461, "example-e.", 53973, DST_ALG_RSASHA256,
		  DST_TYPE_PUBLIC, false },

		/* RSA Public Key: different Modulus (n) */
		{ "example.", 53461, "example-n.", 37464, DST_ALG_RSASHA256,
		  DST_TYPE_PUBLIC, false },

		/* ECDSA Keypair: self */
		{ "example.", 19786, "example.", 19786, DST_ALG_ECDSA256,
		  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE, true },

		/* ECDSA Keypair: different key */
		{ "example.", 19786, "example2.", 16384, DST_ALG_ECDSA256,
		  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE, false },

		/* ECDSA Public Key: self */
		{ "example.", 19786, "example.", 19786, DST_ALG_ECDSA256,
		  DST_TYPE_PUBLIC, true },

		/* ECDSA Public Key: different key */
		{ "example.", 19786, "example2.", 16384, DST_ALG_ECDSA256,
		  DST_TYPE_PUBLIC, false },

		/* EdDSA Keypair: self */
		{ "example.", 63663, "example.", 63663, DST_ALG_ED25519,
		  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE, true },

		/* EdDSA Keypair: different key */
		{ "example.", 63663, "example2.", 37529, DST_ALG_ED25519,
		  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE, false },

		/* EdDSA Public Key: self */
		{ "example.", 63663, "example.", 63663, DST_ALG_ED25519,
		  DST_TYPE_PUBLIC, true },

		/* EdDSA Public Key: different key */
		{ "example.", 63663, "example2.", 37529, DST_ALG_ED25519,
		  DST_TYPE_PUBLIC, false },

		/* DH Keypair: self */
		{ "example.", 65316, "example.", 65316, DST_ALG_DH,
		  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE | DST_TYPE_KEY, true },

		/* DH Keypair: different key */
		{ "example.", 65316, "example2.", 19823, DST_ALG_DH,
		  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE | DST_TYPE_KEY, false },

		/* DH Keypair: different key (with generator=5) */
		{ "example.", 65316, "example3.", 17187, DST_ALG_DH,
		  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE | DST_TYPE_KEY, false },

		/* DH Keypair: different private key */
		{ "example.", 65316, "example-private.", 65316, DST_ALG_DH,
		  DST_TYPE_PUBLIC | DST_TYPE_PRIVATE | DST_TYPE_KEY, false },

		/* DH Public Key: self */
		{ "example.", 65316, "example.", 65316, DST_ALG_DH,
		  DST_TYPE_PUBLIC | DST_TYPE_KEY, true },

		/* DH Public Key: different key */
		{ "example.", 65316, "example2.", 19823, DST_ALG_DH,
		  DST_TYPE_PUBLIC | DST_TYPE_KEY, false },

		/* DH Public Key: different key (with generator=5) */
		{ "example.", 65316, "example3.", 17187, DST_ALG_DH,
		  DST_TYPE_PUBLIC | DST_TYPE_KEY, false },
	};
	unsigned int i;

	for (i = 0; i < (sizeof(testcases) / sizeof(testcases[0])); i++) {
		if (!dst_algorithm_supported(testcases[i].alg)) {
			continue;
		}

		check_cmp(testcases[i].key1_name, testcases[i].key1_id,
			  testcases[i].key2_name, testcases[i].key2_id,
			  testcases[i].alg, testcases[i].type,
			  testcases[i].expect);
	}
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(sig_test, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(cmp_test, setup_test, teardown_test)
ISC_TEST_LIST_END

ISC_TEST_MAIN
