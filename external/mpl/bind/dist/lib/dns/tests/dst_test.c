/*	$NetBSD: dst_test.c,v 1.5 2019/02/24 20:01:31 christos Exp $	*/

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

#include <config.h>

#if HAVE_CMOCKA

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/util.h>

#include <isc/file.h>
#include <isc/hex.h>
#include <isc/util.h>
#include <isc/stdio.h>
#include <isc/string.h>

#include <dst/dst.h>
#include <dst/result.h>

#include "../dst_internal.h"

#include "dnstest.h"

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
	  dns_keytag_t id, dns_secalg_t alg, int type, bool expect)
{
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
	result = dst_key_fromfile(name, id, alg, type, "testdata/dst",
				  mctx, &key);
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

	result = dst_context_create(key, mctx, DNS_LOGCATEGORY_GENERAL,
				    false, 0, &ctx);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = dst_context_adddata(ctx, &datareg);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = dst_context_verify(ctx, &sigreg);

	if (expect && result != ISC_R_SUCCESS) {
		isc_result_t result2;
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

		fprintf(stderr, "%s\n", hexbuf);

		dst_context_destroy(&ctx);
	}

	assert_true((expect && (result == ISC_R_SUCCESS)) ||
		    (!expect && (result != ISC_R_SUCCESS)));

	isc_mem_put(mctx, data, size + 1);
	dst_context_destroy(&ctx);
	dst_key_free(&key);

	return;
}

static void
sig_test(void **state) {
	UNUSED(state);

	struct {
		const char *datapath;
		const char *sigpath;
		const char *keyname;
		dns_keytag_t keyid;
		dns_secalg_t alg;
		bool expect;
	} testcases[] = {
		{
			"testdata/dst/test1.data",
			"testdata/dst/test1.ecdsa256sig",
			"test.", 49130, DST_ALG_ECDSA256, true
		},
		{
			"testdata/dst/test1.data",
			"testdata/dst/test1.rsasha256sig",
			"test.", 11349, DST_ALG_RSASHA256, true
		},
		{
			/* wrong sig */
			"testdata/dst/test1.data",
			"testdata/dst/test1.ecdsa256sig",
			"test.", 11349, DST_ALG_RSASHA256, false
		},
		{
			/* wrong data */
			"testdata/dst/test2.data",
			"testdata/dst/test1.ecdsa256sig",
			"test.", 49130, DST_ALG_ECDSA256, false
		},
	};
	unsigned int i;

	for (i = 0; i < (sizeof(testcases)/sizeof(testcases[0])); i++) {
		if (!dst_algorithm_supported(testcases[i].alg)) {
			continue;
		}

		check_sig(testcases[i].datapath,
			  testcases[i].sigpath,
			  testcases[i].keyname,
			  testcases[i].keyid,
			  testcases[i].alg,
			  DST_TYPE_PRIVATE|DST_TYPE_PUBLIC,
			  testcases[i].expect);
	}
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(sig_test, _setup, _teardown),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif
