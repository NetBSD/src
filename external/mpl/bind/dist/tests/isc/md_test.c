/*	$NetBSD: md_test.c,v 1.2.2.2 2024/02/25 15:47:52 martin Exp $	*/

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
#include <string.h>

/* For FIPS_mode() */
#include <openssl/crypto.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/hex.h>
#include <isc/md.h>
#include <isc/region.h>
#include <isc/result.h>

#include "md.c"

#include <tests/isc.h>

#define TEST_INPUT(x) (x), sizeof(x) - 1

static int
_setup(void **state) {
	isc_md_t *md = isc_md_new();
	if (md == NULL) {
		return (-1);
	}
	*state = md;
	return (0);
}

static int
_teardown(void **state) {
	if (*state == NULL) {
		return (-1);
	}
	isc_md_free(*state);
	return (0);
}

static int
_reset(void **state) {
	if (*state == NULL) {
		return (-1);
	}
	if (isc_md_reset(*state) != ISC_R_SUCCESS) {
		return (-1);
	}
	return (0);
}

ISC_RUN_TEST_IMPL(isc_md_new) {
	isc_md_t *md = isc_md_new();
	assert_non_null(md);
	isc_md_free(md); /* Cleanup */
}

ISC_RUN_TEST_IMPL(isc_md_free) {
	isc_md_t *md = isc_md_new();
	assert_non_null(md);
	isc_md_free(md);   /* Test freeing valid message digest context */
	isc_md_free(NULL); /* Test freeing NULL argument */
}

static void
isc_md_test(isc_md_t *md, const isc_md_type_t *type, const char *buf,
	    size_t buflen, const char *result, const size_t repeats) {
	isc_result_t res;

	assert_non_null(md);
	assert_int_equal(isc_md_init(md, type), ISC_R_SUCCESS);

	for (size_t i = 0; i < repeats; i++) {
		assert_int_equal(
			isc_md_update(md, (const unsigned char *)buf, buflen),
			ISC_R_SUCCESS);
	}

	unsigned char digest[ISC_MAX_MD_SIZE];
	unsigned int digestlen;
	assert_int_equal(isc_md_final(md, digest, &digestlen), ISC_R_SUCCESS);

	char hexdigest[ISC_MAX_MD_SIZE * 2 + 3];
	isc_region_t r = { .base = digest, .length = digestlen };
	isc_buffer_t b;
	isc_buffer_init(&b, hexdigest, sizeof(hexdigest));

	res = isc_hex_totext(&r, 0, "", &b);

	assert_return_code(res, ISC_R_SUCCESS);

	assert_memory_equal(hexdigest, result, (result ? strlen(result) : 0));
	assert_int_equal(isc_md_reset(md), ISC_R_SUCCESS);
}

ISC_RUN_TEST_IMPL(isc_md_init) {
	isc_md_t *md = *state;
	assert_non_null(md);

	expect_assert_failure(isc_md_init(NULL, ISC_MD_MD5));

	assert_int_equal(isc_md_init(md, NULL), ISC_R_NOTIMPLEMENTED);

	assert_int_equal(isc_md_init(md, ISC_MD_MD5), ISC_R_SUCCESS);
	assert_int_equal(isc_md_reset(md), ISC_R_SUCCESS);

	assert_int_equal(isc_md_init(md, ISC_MD_SHA1), ISC_R_SUCCESS);
	assert_int_equal(isc_md_reset(md), ISC_R_SUCCESS);

	assert_int_equal(isc_md_init(md, ISC_MD_SHA224), ISC_R_SUCCESS);
	assert_int_equal(isc_md_reset(md), ISC_R_SUCCESS);

	assert_int_equal(isc_md_init(md, ISC_MD_SHA256), ISC_R_SUCCESS);
	assert_int_equal(isc_md_reset(md), ISC_R_SUCCESS);

	assert_int_equal(isc_md_init(md, ISC_MD_SHA384), ISC_R_SUCCESS);
	assert_int_equal(isc_md_reset(md), ISC_R_SUCCESS);

	assert_int_equal(isc_md_init(md, ISC_MD_SHA512), ISC_R_SUCCESS);
	assert_int_equal(isc_md_reset(md), ISC_R_SUCCESS);
}

ISC_RUN_TEST_IMPL(isc_md_update) {
	isc_md_t *md = *state;
	assert_non_null(md);

	/* Uses message digest context initialized in isc_md_init_test() */
	expect_assert_failure(isc_md_update(NULL, NULL, 0));

	assert_int_equal(isc_md_update(md, NULL, 100), ISC_R_SUCCESS);
	assert_int_equal(isc_md_update(md, (const unsigned char *)"", 0),
			 ISC_R_SUCCESS);
}

ISC_RUN_TEST_IMPL(isc_md_reset) {
	isc_md_t *md = *state;
#if 0
	unsigned char digest[ISC_MAX_MD_SIZE] __attribute((unused));
	unsigned int digestlen __attribute((unused));
#endif /* if 0 */

	assert_non_null(md);

	assert_int_equal(isc_md_init(md, ISC_MD_SHA512), ISC_R_SUCCESS);
	assert_int_equal(isc_md_update(md, (const unsigned char *)"a", 1),
			 ISC_R_SUCCESS);
	assert_int_equal(isc_md_update(md, (const unsigned char *)"b", 1),
			 ISC_R_SUCCESS);

	assert_int_equal(isc_md_reset(md), ISC_R_SUCCESS);

#if 0
	/*
	 * This test would require OpenSSL compiled with mock_assert(),
	 * so this could be only manually checked that the test will
	 * segfault when called by hand
	 */
	expect_assert_failure(isc_md_final(md,digest,&digestlen));
#endif /* if 0 */
}

ISC_RUN_TEST_IMPL(isc_md_final) {
	isc_md_t *md = *state;
	assert_non_null(md);

	unsigned char digest[ISC_MAX_MD_SIZE];
	unsigned int digestlen;

	/* Fail when message digest context is empty */
	expect_assert_failure(isc_md_final(NULL, digest, &digestlen));
	/* Fail when output buffer is empty */
	expect_assert_failure(isc_md_final(md, NULL, &digestlen));

	assert_int_equal(isc_md_init(md, ISC_MD_SHA512), ISC_R_SUCCESS);
	assert_int_equal(isc_md_final(md, digest, NULL), ISC_R_SUCCESS);
}

ISC_RUN_TEST_IMPL(isc_md_md5) {
	isc_md_t *md = *state;
	isc_md_test(md, ISC_MD_MD5, NULL, 0, NULL, 0);
	isc_md_test(md, ISC_MD_MD5, TEST_INPUT(""),
		    "D41D8CD98F00B204E9800998ECF8427E", 1);
	isc_md_test(md, ISC_MD_MD5, TEST_INPUT("a"),
		    "0CC175B9C0F1B6A831C399E269772661", 1);
	isc_md_test(md, ISC_MD_MD5, TEST_INPUT("abc"),
		    "900150983CD24FB0D6963F7D28E17F72", 1);
	isc_md_test(md, ISC_MD_MD5, TEST_INPUT("message digest"),
		    "F96B697D7CB7938D525A2F31AAF161D0", 1);
	isc_md_test(md, ISC_MD_MD5, TEST_INPUT("abcdefghijklmnopqrstuvwxyz"),
		    "C3FCD3D76192E4007DFB496CCA67E13B", 1);
	isc_md_test(md, ISC_MD_MD5,
		    TEST_INPUT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklm"
			       "nopqrstuvwxyz0123456789"),
		    "D174AB98D277D9F5A5611C2C9F419D9F", 1);
	isc_md_test(md, ISC_MD_MD5,
		    TEST_INPUT("123456789012345678901234567890123456789"
			       "01234567890123456789012345678901234567890"),
		    "57EDF4A22BE3C955AC49DA2E2107B67A", 1);
}

ISC_RUN_TEST_IMPL(isc_md_sha1) {
	isc_md_t *md = *state;
	isc_md_test(md, ISC_MD_SHA1, NULL, 0, NULL, 0);
	isc_md_test(md, ISC_MD_SHA1, TEST_INPUT(""),
		    "DA39A3EE5E6B4B0D3255BFEF95601890AFD80709", 1);
	isc_md_test(md, ISC_MD_SHA1, TEST_INPUT("abc"),
		    "A9993E364706816ABA3E25717850C26C9CD0D89D", 1);
	isc_md_test(md, ISC_MD_SHA1,
		    TEST_INPUT("abcdbcdecdefdefgefghfghighijhijkijk"
			       "ljklmklmnlmnomnopnopq"),
		    "84983E441C3BD26EBAAE4AA1F95129E5E54670F1", 1);
	isc_md_test(md, ISC_MD_SHA1, TEST_INPUT("a"),
		    "34AA973CD4C4DAA4F61EEB2BDBAD27316534016F", 1000000);
	isc_md_test(md, ISC_MD_SHA1,
		    TEST_INPUT("01234567012345670123456701234567"),
		    "DEA356A2CDDD90C7A7ECEDC5EBB563934F460452", 20);
	isc_md_test(md, ISC_MD_SHA1, TEST_INPUT("\x5e"),
		    "5E6F80A34A9798CAFC6A5DB96CC57BA4C4DB59C2", 1);
	isc_md_test(md, ISC_MD_SHA1,
		    TEST_INPUT("\x9a\x7d\xfd\xf1\xec\xea\xd0\x6e\xd6\x46"
			       "\xaa\x55\xfe\x75\x71\x46"),
		    "82ABFF6605DBE1C17DEF12A394FA22A82B544A35", 1);
	isc_md_test(md, ISC_MD_SHA1,
		    TEST_INPUT("\xf7\x8f\x92\x14\x1b\xcd\x17\x0a\xe8\x9b"
			       "\x4f\xba\x15\xa1\xd5\x9f\x3f\xd8\x4d\x22"
			       "\x3c\x92\x51\xbd\xac\xbb\xae\x61\xd0\x5e"
			       "\xd1\x15\xa0\x6a\x7c\xe1\x17\xb7\xbe\xea"
			       "\xd2\x44\x21\xde\xd9\xc3\x25\x92\xbd\x57"
			       "\xed\xea\xe3\x9c\x39\xfa\x1f\xe8\x94\x6a"
			       "\x84\xd0\xcf\x1f\x7b\xee\xad\x17\x13\xe2"
			       "\xe0\x95\x98\x97\x34\x7f\x67\xc8\x0b\x04"
			       "\x00\xc2\x09\x81\x5d\x6b\x10\xa6\x83\x83"
			       "\x6f\xd5\x56\x2a\x56\xca\xb1\xa2\x8e\x81"
			       "\xb6\x57\x66\x54\x63\x1c\xf1\x65\x66\xb8"
			       "\x6e\x3b\x33\xa1\x08\xb0\x53\x07\xc0\x0a"
			       "\xff\x14\xa7\x68\xed\x73\x50\x60\x6a\x0f"
			       "\x85\xe6\xa9\x1d\x39\x6f\x5b\x5c\xbe\x57"
			       "\x7f\x9b\x38\x80\x7c\x7d\x52\x3d\x6d\x79"
			       "\x2f\x6e\xbc\x24\xa4\xec\xf2\xb3\xa4\x27"
			       "\xcd\xbb\xfb"),
		    "CB0082C8F197D260991BA6A460E76E202BAD27B3", 1);
}

ISC_RUN_TEST_IMPL(isc_md_sha224) {
	isc_md_t *md = *state;

	isc_md_test(md, ISC_MD_SHA224, NULL, 0, NULL, 0);
	isc_md_test(md, ISC_MD_SHA224, TEST_INPUT(""),
		    "D14A028C2A3A2BC9476102BB288234C415A2B01F828EA62AC5B3E42F",
		    1);
	isc_md_test(md, ISC_MD_SHA224, TEST_INPUT("abc"),
		    "23097D223405D8228642A477BDA255B32AADBCE4BDA0B3F7"
		    "E36C9DA7",
		    1);
	isc_md_test(md, ISC_MD_SHA224,
		    TEST_INPUT("abcdbcdecdefdefgefghfghighijhijkijklj"
			       "klmklmnlmnomnopnopq"),
		    "75388B16512776CC5DBA5DA1FD890150B0C6455CB4F58B"
		    "1952522525",
		    1);
	isc_md_test(md, ISC_MD_SHA224, TEST_INPUT("a"),
		    "20794655980C91D8BBB4C1EA97618A4BF03F42581948B2"
		    "EE4EE7AD67",
		    1000000);
	isc_md_test(md, ISC_MD_SHA224,
		    TEST_INPUT("01234567012345670123456701234567"),
		    "567F69F168CD7844E65259CE658FE7AADFA25216E68ECA"
		    "0EB7AB8262",
		    20);
	isc_md_test(md, ISC_MD_SHA224, TEST_INPUT("\x07"),
		    "00ECD5F138422B8AD74C9799FD826C531BAD2FCABC7450"
		    "BEE2AA8C2A",
		    1);
	isc_md_test(md, ISC_MD_SHA224,
		    TEST_INPUT("\x18\x80\x40\x05\xdd\x4f\xbd\x15\x56\x29"
			       "\x9d\x6f\x9d\x93\xdf\x62"),
		    "DF90D78AA78821C99B40BA4C966921ACCD8FFB1E98AC38"
		    "8E56191DB1",
		    1);
	isc_md_test(md, ISC_MD_SHA224,
		    TEST_INPUT("\x55\xb2\x10\x07\x9c\x61\xb5\x3a\xdd\x52"
			       "\x06\x22\xd1\xac\x97\xd5\xcd\xbe\x8c\xb3"
			       "\x3a\xa0\xae\x34\x45\x17\xbe\xe4\xd7\xba"
			       "\x09\xab\xc8\x53\x3c\x52\x50\x88\x7a\x43"
			       "\xbe\xbb\xac\x90\x6c\x2e\x18\x37\xf2\x6b"
			       "\x36\xa5\x9a\xe3\xbe\x78\x14\xd5\x06\x89"
			       "\x6b\x71\x8b\x2a\x38\x3e\xcd\xac\x16\xb9"
			       "\x61\x25\x55\x3f\x41\x6f\xf3\x2c\x66\x74"
			       "\xc7\x45\x99\xa9\x00\x53\x86\xd9\xce\x11"
			       "\x12\x24\x5f\x48\xee\x47\x0d\x39\x6c\x1e"
			       "\xd6\x3b\x92\x67\x0c\xa5\x6e\xc8\x4d\xee"
			       "\xa8\x14\xb6\x13\x5e\xca\x54\x39\x2b\xde"
			       "\xdb\x94\x89\xbc\x9b\x87\x5a\x8b\xaf\x0d"
			       "\xc1\xae\x78\x57\x36\x91\x4a\xb7\xda\xa2"
			       "\x64\xbc\x07\x9d\x26\x9f\x2c\x0d\x7e\xdd"
			       "\xd8\x10\xa4\x26\x14\x5a\x07\x76\xf6\x7c"
			       "\x87\x82\x73"),
		    "0B31894EC8937AD9B91BDFBCBA294D9ADEFAA18E09305E"
		    "9F20D5C3A4",
		    1);
}

ISC_RUN_TEST_IMPL(isc_md_sha256) {
	isc_md_t *md = *state;

	isc_md_test(md, ISC_MD_SHA256, NULL, 0, NULL, 0);
	isc_md_test(md, ISC_MD_SHA256, TEST_INPUT(""),
		    "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B93"
		    "4CA495991B7852B855",
		    1);

	isc_md_test(md, ISC_MD_SHA256, TEST_INPUT("abc"),
		    "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A"
		    "9CB410FF61F20015AD",
		    1);
	isc_md_test(md, ISC_MD_SHA256,
		    TEST_INPUT("abcdbcdecdefdefgefghfghighijhijkijkljk"
			       "lmklmnlmnomnopnopq"),
		    "248D6A61D20638B8E5C026930C3E6039A33CE45964FF21"
		    "67F6ECEDD419DB06C1",
		    1);
	isc_md_test(md, ISC_MD_SHA256, TEST_INPUT("a"),
		    "CDC76E5C9914FB9281A1C7E284D73E67F1809A48A49720"
		    "0E046D39CCC7112CD0",
		    1000000);
	isc_md_test(md, ISC_MD_SHA256,
		    TEST_INPUT("01234567012345670123456701234567"),
		    "594847328451BDFA85056225462CC1D867D877FB388DF0"
		    "CE35F25AB5562BFBB5",
		    20);
	isc_md_test(md, ISC_MD_SHA256, TEST_INPUT("\x19"),
		    "68AA2E2EE5DFF96E3355E6C7EE373E3D6A4E17F75F9518"
		    "D843709C0C9BC3E3D4",
		    1);
	isc_md_test(md, ISC_MD_SHA256,
		    TEST_INPUT("\xe3\xd7\x25\x70\xdc\xdd\x78\x7c\xe3"
			       "\x88\x7a\xb2\xcd\x68\x46\x52"),
		    "175EE69B02BA9B58E2B0A5FD13819CEA573F3940A94F82"
		    "5128CF4209BEABB4E8",
		    1);
	isc_md_test(md, ISC_MD_SHA256,
		    TEST_INPUT("\x83\x26\x75\x4e\x22\x77\x37\x2f\x4f\xc1"
			       "\x2b\x20\x52\x7a\xfe\xf0\x4d\x8a\x05\x69"
			       "\x71\xb1\x1a\xd5\x71\x23\xa7\xc1\x37\x76"
			       "\x00\x00\xd7\xbe\xf6\xf3\xc1\xf7\xa9\x08"
			       "\x3a\xa3\x9d\x81\x0d\xb3\x10\x77\x7d\xab"
			       "\x8b\x1e\x7f\x02\xb8\x4a\x26\xc7\x73\x32"
			       "\x5f\x8b\x23\x74\xde\x7a\x4b\x5a\x58\xcb"
			       "\x5c\x5c\xf3\x5b\xce\xe6\xfb\x94\x6e\x5b"
			       "\xd6\x94\xfa\x59\x3a\x8b\xeb\x3f\x9d\x65"
			       "\x92\xec\xed\xaa\x66\xca\x82\xa2\x9d\x0c"
			       "\x51\xbc\xf9\x33\x62\x30\xe5\xd7\x84\xe4"
			       "\xc0\xa4\x3f\x8d\x79\xa3\x0a\x16\x5c\xba"
			       "\xbe\x45\x2b\x77\x4b\x9c\x71\x09\xa9\x7d"
			       "\x13\x8f\x12\x92\x28\x96\x6f\x6c\x0a\xdc"
			       "\x10\x6a\xad\x5a\x9f\xdd\x30\x82\x57\x69"
			       "\xb2\xc6\x71\xaf\x67\x59\xdf\x28\xeb\x39"
			       "\x3d\x54\xd6"),
		    "97DBCA7DF46D62C8A422C941DD7E835B8AD3361763F7E9"
		    "B2D95F4F0DA6E1CCBC",
		    1);
}

ISC_RUN_TEST_IMPL(isc_md_sha384) {
	isc_md_t *md = *state;

	isc_md_test(md, ISC_MD_SHA384, NULL, 0, NULL, 0);
	isc_md_test(md, ISC_MD_SHA384, TEST_INPUT(""),
		    "38B060A751AC96384CD9327EB1B1E36A21FDB71114BE07"
		    "434C0CC7BF63F6E1DA274EDEBFE76F65FBD51AD2F14898"
		    "B95B"
		    "",
		    1);
	isc_md_test(md, ISC_MD_SHA384, TEST_INPUT("abc"),
		    "CB00753F45A35E8BB5A03D699AC65007272C32AB0EDED1"
		    "631A8B605A43FF5BED8086072BA1E7CC2358BAEC"
		    "A134C825A7",
		    1);
	isc_md_test(md, ISC_MD_SHA384,
		    TEST_INPUT("abcdefghbcdefghicdefghijdefghijkefghijkl"
			       "fghijklmghijklmnhijklmnoijklmnopjklmnopq"
			       "klmnopqrlmnopqrsmnopqrstnopqrstu"),
		    "09330C33F71147E83D192FC782CD1B4753111B173B3B05"
		    "D22FA08086E3B0F712FCC7C71A557E2DB966C3E9"
		    "FA91746039",
		    1);
	isc_md_test(md, ISC_MD_SHA384, TEST_INPUT("a"),
		    "9D0E1809716474CB086E834E310A4A1CED149E9C00F248"
		    "527972CEC5704C2A5B07B8B3DC38ECC4EBAE97DD"
		    "D87F3D8985",
		    1000000);
	isc_md_test(md, ISC_MD_SHA384,
		    TEST_INPUT("01234567012345670123456701234567"),
		    "2FC64A4F500DDB6828F6A3430B8DD72A368EB7F3A8322A"
		    "70BC84275B9C0B3AB00D27A5CC3C2D224AA6B61A"
		    "0D79FB4596",
		    20);
	isc_md_test(md, ISC_MD_SHA384, TEST_INPUT("\xb9"),
		    "BC8089A19007C0B14195F4ECC74094FEC64F01F9092928"
		    "2C2FB392881578208AD466828B1C6C283D2722CF"
		    "0AD1AB6938",
		    1);
	isc_md_test(md, ISC_MD_SHA384,
		    TEST_INPUT("\xa4\x1c\x49\x77\x79\xc0\x37\x5f\xf1"
			       "\x0a\x7f\x4e\x08\x59\x17\x39"),
		    "C9A68443A005812256B8EC76B00516F0DBB74FAB26D665"
		    "913F194B6FFB0E91EA9967566B58109CBC675CC2"
		    "08E4C823F7",
		    1);
	isc_md_test(md, ISC_MD_SHA384,
		    TEST_INPUT("\x39\x96\x69\xe2\x8f\x6b\x9c\x6d\xbc\xbb"
			       "\x69\x12\xec\x10\xff\xcf\x74\x79\x03\x49"
			       "\xb7\xdc\x8f\xbe\x4a\x8e\x7b\x3b\x56\x21"
			       "\xdb\x0f\x3e\x7d\xc8\x7f\x82\x32\x64\xbb"
			       "\xe4\x0d\x18\x11\xc9\xea\x20\x61\xe1\xc8"
			       "\x4a\xd1\x0a\x23\xfa\xc1\x72\x7e\x72\x02"
			       "\xfc\x3f\x50\x42\xe6\xbf\x58\xcb\xa8\xa2"
			       "\x74\x6e\x1f\x64\xf9\xb9\xea\x35\x2c\x71"
			       "\x15\x07\x05\x3c\xf4\xe5\x33\x9d\x52\x86"
			       "\x5f\x25\xcc\x22\xb5\xe8\x77\x84\xa1\x2f"
			       "\xc9\x61\xd6\x6c\xb6\xe8\x95\x73\x19\x9a"
			       "\x2c\xe6\x56\x5c\xbd\xf1\x3d\xca\x40\x38"
			       "\x32\xcf\xcb\x0e\x8b\x72\x11\xe8\x3a\xf3"
			       "\x2a\x11\xac\x17\x92\x9f\xf1\xc0\x73\xa5"
			       "\x1c\xc0\x27\xaa\xed\xef\xf8\x5a\xad\x7c"
			       "\x2b\x7c\x5a\x80\x3e\x24\x04\xd9\x6d\x2a"
			       "\x77\x35\x7b\xda\x1a\x6d\xae\xed\x17\x15"
			       "\x1c\xb9\xbc\x51\x25\xa4\x22\xe9\x41\xde"
			       "\x0c\xa0\xfc\x50\x11\xc2\x3e\xcf\xfe\xfd"
			       "\xd0\x96\x76\x71\x1c\xf3\xdb\x0a\x34\x40"
			       "\x72\x0e\x16\x15\xc1\xf2\x2f\xbc\x3c\x72"
			       "\x1d\xe5\x21\xe1\xb9\x9b\xa1\xbd\x55\x77"
			       "\x40\x86\x42\x14\x7e\xd0\x96"),
		    "4F440DB1E6EDD2899FA335F09515AA025EE177A79F4B4A"
		    "AF38E42B5C4DE660F5DE8FB2A5B2FBD2A3CBFFD2"
		    "0CFF1288C0",
		    1);
}

ISC_RUN_TEST_IMPL(isc_md_sha512) {
	isc_md_t *md = *state;

	isc_md_test(md, ISC_MD_SHA512, NULL, 0, NULL, 0);
	isc_md_test(md, ISC_MD_SHA512, TEST_INPUT(""),
		    "CF83E1357EEFB8BDF1542850D66D8007D620E4050B5715"
		    "DC83F4A921D36CE9CE47D0D13C5D85F2B0FF8318D2877E"
		    "EC2F63B931BD47417A81A538327AF927DA3E",
		    1);
	isc_md_test(md, ISC_MD_SHA512, TEST_INPUT("abc"),
		    "DDAF35A193617ABACC417349AE20413112E6FA4E89A97E"
		    "A20A9EEEE64B55D39A2192992A274FC1A836BA3C"
		    "23A3FEEBBD454D4423643CE80E2A9AC94FA54CA49F",
		    1);
	isc_md_test(md, ISC_MD_SHA512,
		    TEST_INPUT("abcdefghbcdefghicdefghijdefghijkefghijkl"
			       "fghijklmghijklmnhijklmnoijklmnopjklmnopq"
			       "klmnopqrlmnopqrsmnopqrstnopqrstu"),
		    "8E959B75DAE313DA8CF4F72814FC143F8F7779C6EB9F7F"
		    "A17299AEADB6889018501D289E4900F7E4331B99"
		    "DEC4B5433AC7D329EEB6DD26545E96E55B874BE909",
		    1);
	isc_md_test(md, ISC_MD_SHA512, TEST_INPUT("a"),
		    "E718483D0CE769644E2E42C7BC15B4638E1F98B13B2044"
		    "285632A803AFA973EBDE0FF244877EA60A4CB043"
		    "2CE577C31BEB009C5C2C49AA2E4EADB217AD8CC09B",
		    1000000);
	isc_md_test(md, ISC_MD_SHA512,
		    TEST_INPUT("01234567012345670123456701234567"),
		    "89D05BA632C699C31231DED4FFC127D5A894DAD412C0E0"
		    "24DB872D1ABD2BA8141A0F85072A9BE1E2AA04CF"
		    "33C765CB510813A39CD5A84C4ACAA64D3F3FB7BAE9",
		    20);
	isc_md_test(md, ISC_MD_SHA512, TEST_INPUT("\xD0"),
		    "9992202938E882E73E20F6B69E68A0A7149090423D93C8"
		    "1BAB3F21678D4ACEEEE50E4E8CAFADA4C85A54EA"
		    "8306826C4AD6E74CECE9631BFA8A549B4AB3FBBA15",
		    1);
	isc_md_test(md, ISC_MD_SHA512,
		    TEST_INPUT("\x8d\x4e\x3c\x0e\x38\x89\x19\x14\x91\x81"
			       "\x6e\x9d\x98\xbf\xf0\xa0"),
		    "CB0B67A4B8712CD73C9AABC0B199E9269B20844AFB75AC"
		    "BDD1C153C9828924C3DDEDAAFE669C5FDD0BC66F"
		    "630F6773988213EB1B16F517AD0DE4B2F0C95C90F8",
		    1);
	isc_md_test(md, ISC_MD_SHA512,
		    TEST_INPUT("\xa5\x5f\x20\xc4\x11\xaa\xd1\x32\x80\x7a"
			       "\x50\x2d\x65\x82\x4e\x31\xa2\x30\x54\x32"
			       "\xaa\x3d\x06\xd3\xe2\x82\xa8\xd8\x4e\x0d"
			       "\xe1\xde\x69\x74\xbf\x49\x54\x69\xfc\x7f"
			       "\x33\x8f\x80\x54\xd5\x8c\x26\xc4\x93\x60"
			       "\xc3\xe8\x7a\xf5\x65\x23\xac\xf6\xd8\x9d"
			       "\x03\xe5\x6f\xf2\xf8\x68\x00\x2b\xc3\xe4"
			       "\x31\xed\xc4\x4d\xf2\xf0\x22\x3d\x4b\xb3"
			       "\xb2\x43\x58\x6e\x1a\x7d\x92\x49\x36\x69"
			       "\x4f\xcb\xba\xf8\x8d\x95\x19\xe4\xeb\x50"
			       "\xa6\x44\xf8\xe4\xf9\x5e\xb0\xea\x95\xbc"
			       "\x44\x65\xc8\x82\x1a\xac\xd2\xfe\x15\xab"
			       "\x49\x81\x16\x4b\xbb\x6d\xc3\x2f\x96\x90"
			       "\x87\xa1\x45\xb0\xd9\xcc\x9c\x67\xc2\x2b"
			       "\x76\x32\x99\x41\x9c\xc4\x12\x8b\xe9\xa0"
			       "\x77\xb3\xac\xe6\x34\x06\x4e\x6d\x99\x28"
			       "\x35\x13\xdc\x06\xe7\x51\x5d\x0d\x73\x13"
			       "\x2e\x9a\x0d\xc6\xd3\xb1\xf8\xb2\x46\xf1"
			       "\xa9\x8a\x3f\xc7\x29\x41\xb1\xe3\xbb\x20"
			       "\x98\xe8\xbf\x16\xf2\x68\xd6\x4f\x0b\x0f"
			       "\x47\x07\xfe\x1e\xa1\xa1\x79\x1b\xa2\xf3"
			       "\xc0\xc7\x58\xe5\xf5\x51\x86\x3a\x96\xc9"
			       "\x49\xad\x47\xd7\xfb\x40\xd2"),
		    "C665BEFB36DA189D78822D10528CBF3B12B3EEF7260399"
		    "09C1A16A270D48719377966B957A878E72058477"
		    "9A62825C18DA26415E49A7176A894E7510FD1451F5",
		    1);
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(isc_md_new)

ISC_TEST_ENTRY_CUSTOM(isc_md_init, _reset, _reset)

ISC_TEST_ENTRY_CUSTOM(isc_md_reset, _reset, _reset)

ISC_TEST_ENTRY(isc_md_md5)
ISC_TEST_ENTRY(isc_md_sha1)

ISC_TEST_ENTRY(isc_md_sha224)
ISC_TEST_ENTRY(isc_md_sha256)
ISC_TEST_ENTRY(isc_md_sha384)
ISC_TEST_ENTRY(isc_md_sha512)

ISC_TEST_ENTRY_CUSTOM(isc_md_update, _reset, _reset)
ISC_TEST_ENTRY_CUSTOM(isc_md_final, _reset, _reset)

ISC_TEST_ENTRY(isc_md_free)

ISC_TEST_LIST_END

ISC_TEST_MAIN_CUSTOM(_setup, _teardown)
