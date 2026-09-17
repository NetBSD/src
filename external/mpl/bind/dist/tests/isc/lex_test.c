/*	$NetBSD: lex_test.c,v 1.4 2026/09/17 18:01:19 christos Exp $	*/

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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/lex.h>
#include <isc/mem.h>
#include <isc/util.h>

#include <tests/isc.h>

#define AS_STR(x) (x).value.as_textregion.base

/* check handling of 0x00 */
ISC_RUN_TEST_IMPL(lex_0x00) {
	isc_result_t result;
	isc_lex_t *lex = NULL;
	isc_buffer_t buf;
	isc_token_t token;

	unsigned char nul_then_A[] = { '\0', 'A' };
	unsigned char embedded_null[] = { '"', 'a', '\0', 'b', '"' };
	unsigned char escaped_null[] = { 'a', '\\', '\0', 'b' };

	UNUSED(state);

	isc_lex_create(mctx, 1024, &lex);

	isc_buffer_init(&buf, &nul_then_A[0], sizeof(nul_then_A));
	isc_buffer_add(&buf, sizeof(nul_then_A));

	result = isc_lex_openbuffer(lex, &buf);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_lex_gettoken(lex, 0, &token);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(token.type, isc_tokentype_unknown);

	result = isc_lex_gettoken(lex, 0, &token);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(token.type, isc_tokentype_string);

	isc_lex_close(lex);

	/*
	 * Check that an embedded NUL is preserved in a quoted string.
	 */
	isc_buffer_init(&buf, &embedded_null[0], sizeof(embedded_null));
	isc_buffer_add(&buf, sizeof(embedded_null));

	result = isc_lex_openbuffer(lex, &buf);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_lex_gettoken(lex, ISC_LEXOPT_QSTRING, &token);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(token.type, isc_tokentype_qstring);
	assert_int_equal(token.value.as_textregion.length, 3);
	assert_memory_equal(token.value.as_textregion.base, "a\0b", 3);

	isc_lex_close(lex);

	/*
	 * Check that an escaped NUL is preserved.
	 */
	isc_buffer_init(&buf, &escaped_null[0], sizeof(escaped_null));
	isc_buffer_add(&buf, sizeof(escaped_null));

	result = isc_lex_openbuffer(lex, &buf);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_lex_gettoken(lex, ISC_LEXOPT_ESCAPE, &token);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(token.type, isc_tokentype_string);
	assert_int_equal(token.value.as_textregion.length, 4);
	assert_memory_equal(token.value.as_textregion.base, "a\\\0b", 4);

	isc_lex_destroy(&lex);
}

/*
 * A NUL token must not preserve a stale beginning-of-line state:
 * whitespace following the NUL is not initial whitespace.
 */
ISC_RUN_TEST_IMPL(lex_0x00_initialws) {
	isc_result_t result;
	isc_lex_t *lex = NULL;
	isc_buffer_t buf;
	isc_token_t token;

	unsigned char nul_then_ws[] = { 'a', '\n', '\0', ' ', 'b' };

	UNUSED(state);

	isc_lex_create(mctx, 1024, &lex);

	isc_buffer_init(&buf, &nul_then_ws[0], sizeof(nul_then_ws));
	isc_buffer_add(&buf, sizeof(nul_then_ws));

	result = isc_lex_openbuffer(lex, &buf);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_lex_gettoken(lex, ISC_LEXOPT_INITIALWS, &token);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(token.type, isc_tokentype_string);

	result = isc_lex_gettoken(lex, ISC_LEXOPT_INITIALWS, &token);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(token.type, isc_tokentype_unknown);
	/*
	 * The unknown token must not leave the previous token's text
	 * region pointer behind for a caller to dereference.
	 */
	assert_null(token.value.as_textregion.base);
	assert_int_equal(token.value.as_textregion.length, 0);

	result = isc_lex_gettoken(lex, ISC_LEXOPT_INITIALWS, &token);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(token.type, isc_tokentype_string);
	assert_string_equal(AS_STR(token), "b");

	isc_lex_destroy(&lex);
}

/*
 * A NUL inside brace-delimited text is corruption and must yield an
 * unknown token instead of being embedded in the btext token.
 */
ISC_RUN_TEST_IMPL(lex_0x00_btext) {
	isc_result_t result;
	isc_lex_t *lex = NULL;
	isc_buffer_t buf;
	isc_token_t token;
	isc_lexspecials_t specials;

	unsigned char btext_null[] = { '{', 'a', '\0', 'b', '}' };

	UNUSED(state);

	isc_lex_create(mctx, 1024, &lex);

	memset(specials, 0, sizeof(specials));
	specials['{'] = 1;
	specials['}'] = 1;
	isc_lex_setspecials(lex, specials);

	isc_buffer_init(&buf, &btext_null[0], sizeof(btext_null));
	isc_buffer_add(&buf, sizeof(btext_null));

	result = isc_lex_openbuffer(lex, &buf);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_lex_gettoken(lex, ISC_LEXOPT_BTEXT, &token);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(token.type, isc_tokentype_unknown);

	isc_lex_destroy(&lex);
}

/* check handling of 0xff */
ISC_RUN_TEST_IMPL(lex_0xff) {
	isc_result_t result;
	isc_lex_t *lex = NULL;
	isc_buffer_t death_buf;
	isc_token_t token;

	unsigned char death[] = { EOF, 'A' };

	UNUSED(state);

	isc_lex_create(mctx, 1024, &lex);

	isc_buffer_init(&death_buf, &death[0], sizeof(death));
	isc_buffer_add(&death_buf, sizeof(death));

	result = isc_lex_openbuffer(lex, &death_buf);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_lex_gettoken(lex, 0, &token);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_lex_destroy(&lex);
}

/* check setting of source line */
ISC_RUN_TEST_IMPL(lex_setline) {
	isc_result_t result;
	isc_lex_t *lex = NULL;
	unsigned char text[] = "text\nto\nbe\nprocessed\nby\nlexer";
	isc_buffer_t buf;
	isc_token_t token;
	unsigned long line;
	int i;

	UNUSED(state);

	isc_lex_create(mctx, 1024, &lex);

	isc_buffer_init(&buf, &text[0], sizeof(text) - 1);
	isc_buffer_add(&buf, sizeof(text) - 1);

	result = isc_lex_openbuffer(lex, &buf);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_lex_setsourceline(lex, 100);
	assert_int_equal(result, ISC_R_SUCCESS);

	for (i = 0; i < 6; i++) {
		result = isc_lex_gettoken(lex, 0, &token);
		assert_int_equal(result, ISC_R_SUCCESS);

		line = isc_lex_getsourceline(lex);
		assert_int_equal(line, 100U + i);
	}

	result = isc_lex_gettoken(lex, 0, &token);
	assert_int_equal(result, ISC_R_EOF);

	line = isc_lex_getsourceline(lex);
	assert_int_equal(line, 105U);

	isc_lex_destroy(&lex);
}

static struct {
	const char *text;
	const char *string_value;
	isc_result_t string_result;
	isc_tokentype_t string_type;
	const char *qstring_value;
	isc_result_t qstring_result;
	isc_tokentype_t qstring_type;
	const char *qvpair_value;
	isc_result_t qvpair_result;
	isc_tokentype_t qvpair_type;
} parse_tests[] = {
	{ "", "", ISC_R_SUCCESS, isc_tokentype_eof, "", ISC_R_SUCCESS,
	  isc_tokentype_eof, "", ISC_R_SUCCESS, isc_tokentype_eof },
	{ "1234", "1234", ISC_R_SUCCESS, isc_tokentype_string, "1234",
	  ISC_R_SUCCESS, isc_tokentype_string, "1234", ISC_R_SUCCESS,
	  isc_tokentype_string },
	{ "1234=", "1234=", ISC_R_SUCCESS, isc_tokentype_string,
	  "1234=", ISC_R_SUCCESS, isc_tokentype_string, "1234=", ISC_R_SUCCESS,
	  isc_tokentype_vpair },
	{ "1234=foo", "1234=foo", ISC_R_SUCCESS, isc_tokentype_string,
	  "1234=foo", ISC_R_SUCCESS, isc_tokentype_string, "1234=foo",
	  ISC_R_SUCCESS, isc_tokentype_vpair },
	{ "1234=\"foo", "1234=\"foo", ISC_R_SUCCESS, isc_tokentype_string,
	  "1234=\"foo", ISC_R_SUCCESS, isc_tokentype_string, NULL,
	  ISC_R_UNEXPECTEDEND, 0 },
	{ "1234=\"foo\"", "1234=\"foo\"", ISC_R_SUCCESS, isc_tokentype_string,
	  "1234=\"foo\"", ISC_R_SUCCESS, isc_tokentype_string, "1234=foo",
	  ISC_R_SUCCESS, isc_tokentype_qvpair },
	{ "key", "key", ISC_R_SUCCESS, isc_tokentype_string, "key",
	  ISC_R_SUCCESS, isc_tokentype_string, "key", ISC_R_SUCCESS,
	  isc_tokentype_string },
	{ "\"key=", "\"key=", ISC_R_SUCCESS, isc_tokentype_string, NULL,
	  ISC_R_UNEXPECTEDEND, 0, "\"key=", ISC_R_SUCCESS,
	  isc_tokentype_vpair },
	{ "\"key=\"", "\"key=\"", ISC_R_SUCCESS, isc_tokentype_string, "key=",
	  ISC_R_SUCCESS, isc_tokentype_qstring, NULL, ISC_R_UNEXPECTEDEND, 0 },
	{ "key=\"\"", "key=\"\"", ISC_R_SUCCESS, isc_tokentype_string,
	  "key=\"\"", ISC_R_SUCCESS, isc_tokentype_string,
	  "key=", ISC_R_SUCCESS, isc_tokentype_qvpair },
	{ "key=\"a b\"", "key=\"a", ISC_R_SUCCESS, isc_tokentype_string,
	  "key=\"a", ISC_R_SUCCESS, isc_tokentype_string, "key=a b",
	  ISC_R_SUCCESS, isc_tokentype_qvpair },
	{ "key=\"a\tb\"", "key=\"a", ISC_R_SUCCESS, isc_tokentype_string,
	  "key=\"a", ISC_R_SUCCESS, isc_tokentype_string, "key=a\tb",
	  ISC_R_SUCCESS, isc_tokentype_qvpair },
	/* double quote not immediately after '=' is not special. */
	{ "key=c\"a b\"", "key=c\"a", ISC_R_SUCCESS, isc_tokentype_string,
	  "key=c\"a", ISC_R_SUCCESS, isc_tokentype_string, "key=c\"a",
	  ISC_R_SUCCESS, isc_tokentype_vpair },
	/* remove special meaning for '=' by escaping */
	{ "key\\=", "key\\=", ISC_R_SUCCESS, isc_tokentype_string,
	  "key\\=", ISC_R_SUCCESS, isc_tokentype_string,
	  "key\\=", ISC_R_SUCCESS, isc_tokentype_string },
	{ "key\\=\"a\"", "key\\=\"a\"", ISC_R_SUCCESS, isc_tokentype_string,
	  "key\\=\"a\"", ISC_R_SUCCESS, isc_tokentype_string, "key\\=\"a\"",
	  ISC_R_SUCCESS, isc_tokentype_string },
	{ "key\\=\"a \"", "key\\=\"a", ISC_R_SUCCESS, isc_tokentype_string,
	  "key\\=\"a", ISC_R_SUCCESS, isc_tokentype_string, "key\\=\"a",
	  ISC_R_SUCCESS, isc_tokentype_string },
	/* vpair with a key of 'key\=' (would need to be deescaped) */
	{ "key\\==", "key\\==", ISC_R_SUCCESS, isc_tokentype_string,
	  "key\\==", ISC_R_SUCCESS, isc_tokentype_string,
	  "key\\==", ISC_R_SUCCESS, isc_tokentype_vpair },
	{ "key\\==\"\"", "key\\==\"\"", ISC_R_SUCCESS, isc_tokentype_string,
	  "key\\==\"\"", ISC_R_SUCCESS, isc_tokentype_string,
	  "key\\==", ISC_R_SUCCESS, isc_tokentype_qvpair },
	{ "key=\\\\\\\\", "key=\\\\\\\\", ISC_R_SUCCESS, isc_tokentype_string,
	  "key=\\\\\\\\", ISC_R_SUCCESS, isc_tokentype_string, "key=\\\\\\\\",
	  ISC_R_SUCCESS, isc_tokentype_vpair },
	{ "key=\\\\\\\"", "key=\\\\\\\"", ISC_R_SUCCESS, isc_tokentype_string,
	  "key=\\\\\\\"", ISC_R_SUCCESS, isc_tokentype_string, "key=\\\\\\\"",
	  ISC_R_SUCCESS, isc_tokentype_vpair },
	/* incomplete escape sequence */
	{ "key=\\\"\\", NULL, ISC_R_UNEXPECTEDEND, isc_tokentype_string, NULL,
	  ISC_R_UNEXPECTEDEND, 0, NULL, ISC_R_UNEXPECTEDEND, 0 },
	/* incomplete escape sequence */
	{ "key=\\", NULL, ISC_R_UNEXPECTEDEND, isc_tokentype_string, NULL,
	  ISC_R_UNEXPECTEDEND, 0, NULL, ISC_R_UNEXPECTEDEND, 0 },
};

/*%
 * string
 */
ISC_RUN_TEST_IMPL(lex_string) {
	isc_buffer_t buf;
	isc_lex_t *lex = NULL;
	isc_result_t result;
	isc_token_t token;
	size_t i;

	UNUSED(state);

	for (i = 0; i < ARRAY_SIZE(parse_tests); i++) {
		isc_lex_create(mctx, 1024, &lex);

		isc_buffer_constinit(&buf, parse_tests[i].text,
				     strlen(parse_tests[i].text));
		isc_buffer_add(&buf, strlen(parse_tests[i].text));

		result = isc_lex_openbuffer(lex, &buf);
		assert_int_equal(result, ISC_R_SUCCESS);

		result = isc_lex_setsourceline(lex, 100);
		assert_int_equal(result, ISC_R_SUCCESS);

		memset(&token, 0, sizeof(token));
		result = isc_lex_getmastertoken(lex, &token,
						isc_tokentype_string, true);

		assert_int_equal(result, parse_tests[i].string_result);
		if (result == ISC_R_SUCCESS) {
			switch (token.type) {
			case isc_tokentype_string:
			case isc_tokentype_qstring:
			case isc_tokentype_vpair:
			case isc_tokentype_qvpair:
				assert_int_equal(token.type,
						 parse_tests[i].string_type);
				assert_string_equal(
					AS_STR(token),
					parse_tests[i].string_value);
				break;
			default:
				assert_int_equal(token.type,
						 parse_tests[i].string_type);
				break;
			}
		}

		isc_lex_destroy(&lex);
	}
}

/*%
 * qstring
 */
ISC_RUN_TEST_IMPL(lex_qstring) {
	isc_buffer_t buf;
	isc_lex_t *lex = NULL;
	isc_result_t result;
	isc_token_t token;
	size_t i;

	UNUSED(state);

	for (i = 0; i < ARRAY_SIZE(parse_tests); i++) {
		isc_lex_create(mctx, 1024, &lex);

		isc_buffer_constinit(&buf, parse_tests[i].text,
				     strlen(parse_tests[i].text));
		isc_buffer_add(&buf, strlen(parse_tests[i].text));

		result = isc_lex_openbuffer(lex, &buf);
		assert_int_equal(result, ISC_R_SUCCESS);

		result = isc_lex_setsourceline(lex, 100);
		assert_int_equal(result, ISC_R_SUCCESS);

		memset(&token, 0, sizeof(token));
		result = isc_lex_getmastertoken(lex, &token,
						isc_tokentype_qstring, true);

		assert_int_equal(result, parse_tests[i].qstring_result);
		if (result == ISC_R_SUCCESS) {
			switch (token.type) {
			case isc_tokentype_string:
			case isc_tokentype_qstring:
			case isc_tokentype_vpair:
			case isc_tokentype_qvpair:
				assert_int_equal(token.type,
						 parse_tests[i].qstring_type);
				assert_string_equal(
					AS_STR(token),
					parse_tests[i].qstring_value);
				break;
			default:
				assert_int_equal(token.type,
						 parse_tests[i].qstring_type);
				break;
			}
		}

		isc_lex_destroy(&lex);
	}
}

/*%
 * keypair is <string>=<qstring>.  This has implications double quotes
 * in key names.
 */
ISC_RUN_TEST_IMPL(lex_keypair) {
	isc_buffer_t buf;
	isc_lex_t *lex = NULL;
	isc_result_t result;
	isc_token_t token;
	size_t i;

	UNUSED(state);

	for (i = 0; i < ARRAY_SIZE(parse_tests); i++) {
		isc_lex_create(mctx, 1024, &lex);

		isc_buffer_constinit(&buf, parse_tests[i].text,
				     strlen(parse_tests[i].text));
		isc_buffer_add(&buf, strlen(parse_tests[i].text));

		result = isc_lex_openbuffer(lex, &buf);
		assert_int_equal(result, ISC_R_SUCCESS);

		result = isc_lex_setsourceline(lex, 100);
		assert_int_equal(result, ISC_R_SUCCESS);

		memset(&token, 0, sizeof(token));
		result = isc_lex_getmastertoken(lex, &token,
						isc_tokentype_qvpair, true);

		assert_int_equal(result, parse_tests[i].qvpair_result);
		if (result == ISC_R_SUCCESS) {
			switch (token.type) {
			case isc_tokentype_string:
			case isc_tokentype_qstring:
			case isc_tokentype_vpair:
			case isc_tokentype_qvpair:
				assert_int_equal(token.type,
						 parse_tests[i].qvpair_type);
				assert_string_equal(
					AS_STR(token),
					parse_tests[i].qvpair_value);
				break;
			default:
				assert_int_equal(token.type,
						 parse_tests[i].qvpair_type);
				break;
			}
		}

		isc_lex_destroy(&lex);
	}
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(lex_0x00)
ISC_TEST_ENTRY(lex_0x00_initialws)
ISC_TEST_ENTRY(lex_0x00_btext)
ISC_TEST_ENTRY(lex_0xff)
ISC_TEST_ENTRY(lex_keypair)
ISC_TEST_ENTRY(lex_setline)
ISC_TEST_ENTRY(lex_string)
ISC_TEST_ENTRY(lex_qstring)
ISC_TEST_LIST_END

ISC_TEST_MAIN
