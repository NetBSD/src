/*	$NetBSD: aes_test.c,v 1.2.2.2 2024/02/25 15:47:51 martin Exp $	*/

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/aes.h>
#include <isc/buffer.h>
#include <isc/hex.h>
#include <isc/region.h>
#include <isc/string.h>
#include <isc/util.h>

#include <tests/isc.h>

/*
 * Test data from NIST KAT
 */

isc_result_t
tohexstr(unsigned char *d, char *out);

size_t
fromhexstr(const char *in, unsigned char *d);

unsigned char plaintext[3 * ISC_AES_BLOCK_LENGTH];
unsigned char ciphertext[ISC_AES_BLOCK_LENGTH];
char str[2 * ISC_AES_BLOCK_LENGTH + 1];
unsigned char key[ISC_AES256_KEYLENGTH + 1];
size_t len;

isc_result_t
tohexstr(unsigned char *d, char *out) {
	isc_buffer_t b;
	isc_region_t r;

	isc_buffer_init(&b, out, 2 * ISC_AES_BLOCK_LENGTH + 1);
	r.base = d;
	r.length = ISC_AES_BLOCK_LENGTH;
	return (isc_hex_totext(&r, 0, "", &b));
}

size_t
fromhexstr(const char *in, unsigned char *d) {
	isc_buffer_t b;
	isc_result_t ret;

	isc_buffer_init(&b, d, ISC_AES256_KEYLENGTH + 1);
	ret = isc_hex_decodestring(in, &b);
	if (ret != ISC_R_SUCCESS) {
		return (0);
	}
	return (isc_buffer_usedlength(&b));
}

typedef struct aes_testcase {
	const char *key;
	const char *input;
	const char *result;
} aes_testcase_t;

/* AES 128 test vectors */
ISC_RUN_TEST_IMPL(isc_aes128_test) {
	aes_testcase_t testcases[] = { /* Test 1 (KAT ECBVarTxt128 #3) */
				       { "00000000000000000000000000000000",
					 "F0000000000000000000000000000000",
					 "96D9FD5CC4F07441727DF0F33E401A36" },
				       /* Test 2 (KAT ECBVarTxt128 #123) */
				       { "00000000000000000000000000000000",
					 "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0",
					 "F9B0FDA0C4A898F5B9E6F661C4CE4D07" },
				       /* Test 3 (KAT ECBVarKey128 #3) */
				       { "F0000000000000000000000000000000",
					 "00000000000000000000000000000000",
					 "970014D634E2B7650777E8E84D03CCD8" },
				       /* Test 4 (KAT ECBVarKey128 #123) */
				       { "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0",
					 "00000000000000000000000000000000",
					 "41C78C135ED9E98C096640647265DA1E" },
				       /* Test 5 (KAT ECBGFSbox128 #3) */
				       { "00000000000000000000000000000000",
					 "6A118A874519E64E9963798A503F1D35",
					 "DC43BE40BE0E53712F7E2BF5CA707209" },
				       /* Test 6 (KAT ECBKeySbox128 #3) */
				       { "B6364AC4E1DE1E285EAF144A2415F7A0",
					 "00000000000000000000000000000000",
					 "5D9B05578FC944B3CF1CCF0E746CD581" },
				       { NULL, NULL, NULL }
	};

	aes_testcase_t *testcase = testcases;

	UNUSED(state);

	while (testcase->key != NULL) {
		len = fromhexstr(testcase->key, key);
		assert_int_equal(len, ISC_AES128_KEYLENGTH);
		len = fromhexstr(testcase->input, plaintext);
		assert_int_equal(len, ISC_AES_BLOCK_LENGTH);
		isc_aes128_crypt(key, plaintext, ciphertext);
		assert_int_equal(tohexstr(ciphertext, str), ISC_R_SUCCESS);
		assert_string_equal(str, testcase->result);

		testcase++;
	}
}

/* AES 192 test vectors */
ISC_RUN_TEST_IMPL(isc_aes192_test) {
	aes_testcase_t testcases[] = {
		/* Test 1 (KAT ECBVarTxt192 #3) */
		{ "000000000000000000000000000000000000000000000000",
		  "F0000000000000000000000000000000",
		  "2A560364CE529EFC21788779568D5555" },
		/* Test 2 (KAT ECBVarTxt192 #123) */
		{ "000000000000000000000000000000000000000000000000",
		  "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0",
		  "2AABB999F43693175AF65C6C612C46FB" },
		/* Test 3 (KAT ECBVarKey192 #3) */
		{ "F00000000000000000000000000000000000000000000000",
		  "00000000000000000000000000000000",
		  "180B09F267C45145DB2F826C2582D35C" },
		/* Test 4 (KAT ECBVarKey192 #187) */
		{ "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0",
		  "00000000000000000000000000000000",
		  "EACF1E6C4224EFB38900B185AB1DFD42" },
		/* Test 5 (KAT ECBGFSbox192 #3) */
		{ "000000000000000000000000000000000000000000000000",
		  "51719783D3185A535BD75ADC65071CE1",
		  "4F354592FF7C8847D2D0870CA9481B7C" },
		/* Test 6 (KAT ECBKeySbox192 #3) */
		{ "CD62376D5EBB414917F0C78F05266433DC9192A1EC943300",
		  "00000000000000000000000000000000",
		  "7F6C25FF41858561BB62F36492E93C29" },
		{ NULL, NULL, NULL }
	};

	aes_testcase_t *testcase = testcases;

	while (testcase->key != NULL) {
		len = fromhexstr(testcase->key, key);
		assert_int_equal(len, ISC_AES192_KEYLENGTH);
		len = fromhexstr(testcase->input, plaintext);
		assert_int_equal(len, ISC_AES_BLOCK_LENGTH);
		isc_aes192_crypt(key, plaintext, ciphertext);
		assert_int_equal(tohexstr(ciphertext, str), ISC_R_SUCCESS);
		assert_string_equal(str, testcase->result);

		testcase++;
	}
}

/* AES 256 test vectors */
ISC_RUN_TEST_IMPL(isc_aes256_test) {
	aes_testcase_t testcases[] = { /* Test 1 (KAT ECBVarTxt256 #3) */
				       { "00000000000000000000000000000000"
					 "00000000000000000000000000000000",
					 "F0000000000000000000000000000000",
					 "7F2C5ECE07A98D8BEE13C51177395FF7" },
				       /* Test 2 (KAT ECBVarTxt256 #123) */
				       { "00000000000000000000000000000000"
					 "00000000000000000000000000000000",
					 "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0",
					 "7240E524BC51D8C4D440B1BE55D1062C" },
				       /* Test 3 (KAT ECBVarKey256 #3) */
				       { "F0000000000000000000000000000000"
					 "00000000000000000000000000000000",
					 "00000000000000000000000000000000",
					 "1C777679D50037C79491A94DA76A9A35" },
				       /* Test 4 (KAT ECBVarKey256 #251) */
				       { "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
					 "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0",
					 "00000000000000000000000000000000",
					 "03720371A04962EAEA0A852E69972858" },
				       /* Test 5 (KAT ECBGFSbox256 #3) */
				       { "00000000000000000000000000000000"
					 "00000000000000000000000000000000",
					 "8A560769D605868AD80D819BDBA03771",
					 "38F2C7AE10612415D27CA190D27DA8B4" },
				       /* Test 6 (KAT ECBKeySbox256 #3) */
				       { "984CA75F4EE8D706F46C2D98C0BF4A45"
					 "F5B00D791C2DFEB191B5ED8E420FD627",
					 "00000000000000000000000000000000",
					 "4307456A9E67813B452E15FA8FFFE398" },
				       { NULL, NULL, NULL }
	};

	aes_testcase_t *testcase = testcases;

	UNUSED(state);

	while (testcase->key != NULL) {
		len = fromhexstr(testcase->key, key);
		assert_int_equal(len, ISC_AES256_KEYLENGTH);
		len = fromhexstr(testcase->input, plaintext);
		assert_int_equal(len, ISC_AES_BLOCK_LENGTH);
		isc_aes256_crypt(key, plaintext, ciphertext);
		assert_int_equal(tohexstr(ciphertext, str), ISC_R_SUCCESS);
		assert_string_equal(str, testcase->result);

		testcase++;
	}
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(isc_aes128_test)
ISC_TEST_ENTRY(isc_aes192_test)
ISC_TEST_ENTRY(isc_aes256_test)

ISC_TEST_LIST_END

ISC_TEST_MAIN
