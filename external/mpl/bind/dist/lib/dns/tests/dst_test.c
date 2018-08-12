/*	$NetBSD: dst_test.c,v 1.2 2018/08/12 13:02:36 christos Exp $	*/

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

/* ! \file */

#include <config.h>

#include <atf-c.h>

#include <unistd.h>

#include <isc/file.h>
#include <isc/util.h>
#include <isc/stdio.h>
#include <isc/string.h>

#include <dst/dst.h>
#include <dst/result.h>

#include "../dst_internal.h"

#include "dnstest.h"

ATF_TC(sig);
ATF_TC_HEAD(sig, tc) {
	atf_tc_set_md_var(tc, "descr", "signature ineffability");
}

/*
 * Read sig in file at path to buf.
 */
static isc_result_t
sig_fromfile(const char *path, isc_buffer_t *buf) {
	isc_result_t result;
	size_t rval, len;
	FILE *fp = NULL;
	unsigned char val;
	char *p, *data;
	off_t size;

	result = isc_stdio_open(path, "rb", &fp);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_file_getsizefd(fileno(fp), &size);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	data = isc_mem_get(mctx, (size + 1));
	ATF_REQUIRE(data != NULL);

	len = (size_t)size;
	p = data;
	while (len != 0U) {
		result = isc_stdio_read(p, 1, len, fp, &rval);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
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
		} else if (len < 2U)
		       goto err;
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
	  dns_keytag_t id, dns_secalg_t alg, int type, isc_boolean_t expect)
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
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_file_getsizefd(fileno(fp), &size);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	data = isc_mem_get(mctx, (size + 1));
	ATF_REQUIRE(data != NULL);

	p = data;
	len = (size_t)size;
	do {
		result = isc_stdio_read(p, 1, len, fp, &rval);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
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
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = dst_key_fromfile(name, id, alg, type, "testdata/dst",
				  mctx, &key);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_buffer_init(&databuf, data, (unsigned int)size);
	isc_buffer_add(&databuf, (unsigned int)size);
	isc_buffer_usedregion(&databuf, &datareg);

	memset(sig, 0, sizeof(sig));
	isc_buffer_init(&sigbuf, sig, sizeof(sig));

	/*
	 * Read precomputed signature from file in a form usable by dst_verify.
	 */
	result = sig_fromfile(sigpath, &sigbuf);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Verify that the key signed the data.
	 */
	isc_buffer_remainingregion(&sigbuf, &sigreg);

	result = dst_context_create3(key, mctx, DNS_LOGCATEGORY_GENERAL,
				     ISC_FALSE, &ctx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = dst_context_adddata(ctx, &datareg);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = dst_context_verify(ctx, &sigreg);

	ATF_REQUIRE((expect && (result == ISC_R_SUCCESS)) ||
		    (!expect && (result != ISC_R_SUCCESS)));


	isc_mem_put(mctx, data, size + 1);
	dst_context_destroy(&ctx);
	dst_key_free(&key);

	return;
}

ATF_TC_BODY(sig, tc) {
	isc_result_t result;

	UNUSED(tc);

	result = dns_test_begin(NULL, ISC_FALSE);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	struct {
		const char *datapath;
		const char *sigpath;
		const char *keyname;
		dns_keytag_t keyid;
		dns_secalg_t alg;
		isc_boolean_t expect;
	} testcases[] = {
		{
			"testdata/dst/test1.data",
			"testdata/dst/test1.dsasig",
			"test.", 23616, DST_ALG_DSA, ISC_TRUE
		},
		{
			"testdata/dst/test1.data",
			"testdata/dst/test1.rsasig",
			"test.", 54622, DST_ALG_RSAMD5, ISC_TRUE
		},
		{
			/* wrong sig */
			"testdata/dst/test1.data",
			"testdata/dst/test1.dsasig",
			"test.", 54622, DST_ALG_RSAMD5, ISC_FALSE
		},
		{
			/* wrong data */
			"testdata/dst/test2.data",
			"testdata/dst/test1.dsasig",
			"test.", 23616, DST_ALG_DSA, ISC_FALSE
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

	dns_test_end();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, sig);

	return (atf_no_error());
}
