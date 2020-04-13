/*	$NetBSD: lex_test.c,v 1.4.2.3 2020/04/13 08:02:59 martin Exp $	*/

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

#include <sched.h> /* IWYU pragma: keep */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/lex.h>
#include <isc/mem.h>
#include <isc/util.h>

/* check handling of 0xff */
static void
lex_0xff(void **state) {
	isc_mem_t *mctx = NULL;
	isc_result_t result;
	isc_lex_t *lex = NULL;
	isc_buffer_t death_buf;
	isc_token_t token;

	unsigned char death[] = { EOF, 'A' };

	UNUSED(state);

	result = isc_mem_create(0, 0, &mctx);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_lex_create(mctx, 1024, &lex);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_buffer_init(&death_buf, &death[0], sizeof(death));
	isc_buffer_add(&death_buf, sizeof(death));

	result = isc_lex_openbuffer(lex, &death_buf);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_lex_gettoken(lex, 0, &token);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_lex_destroy(&lex);

	isc_mem_destroy(&mctx);
}

/* check setting of source line */
static void
lex_setline(void **state) {
	isc_mem_t *mctx = NULL;
	isc_result_t result;
	isc_lex_t *lex = NULL;
	unsigned char text[] = "text\nto\nbe\nprocessed\nby\nlexer";
	isc_buffer_t buf;
	isc_token_t token;
	unsigned long line;
	int i;

	UNUSED(state);

	result = isc_mem_create(0, 0, &mctx);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_lex_create(mctx, 1024, &lex);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_buffer_init(&buf, &text[0], sizeof(text));
	isc_buffer_add(&buf, sizeof(text));

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

	isc_mem_destroy(&mctx);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(lex_0xff),
		cmocka_unit_test(lex_setline),
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
