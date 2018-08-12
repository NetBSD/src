/*	$NetBSD: lex_test.c,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

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

#include <atf-c.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <isc/buffer.h>
#include <isc/lex.h>
#include <isc/mem.h>
#include <isc/util.h>

ATF_TC(lex_0xff);
ATF_TC_HEAD(lex_0xff, tc) {
	atf_tc_set_md_var(tc, "descr", "check handling of 0xff");
}
ATF_TC_BODY(lex_0xff, tc) {
	isc_mem_t *mctx = NULL;
	isc_result_t result;
	isc_lex_t *lex = NULL;
	isc_buffer_t death_buf;
	isc_token_t token;

	unsigned char death[] = { EOF, 'A' };

	UNUSED(tc);

	result = isc_mem_create(0, 0, &mctx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_lex_create(mctx, 1024, &lex);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_buffer_init(&death_buf, &death[0], sizeof(death));
	isc_buffer_add(&death_buf, sizeof(death));

	result = isc_lex_openbuffer(lex, &death_buf);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_lex_gettoken(lex, 0, &token);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
}

ATF_TC(lex_setline);
ATF_TC_HEAD(lex_setline, tc) {
	atf_tc_set_md_var(tc, "descr", "check setting of source line");
}
ATF_TC_BODY(lex_setline, tc) {
	isc_mem_t *mctx = NULL;
	isc_result_t result;
	isc_lex_t *lex = NULL;
	unsigned char text[] = "text\nto\nbe\nprocessed\nby\nlexer";
	isc_buffer_t buf;
	isc_token_t token;
	unsigned long line;
	int i;

	UNUSED(tc);

	result = isc_mem_create(0, 0, &mctx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_lex_create(mctx, 1024, &lex);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_buffer_init(&buf, &text[0], sizeof(text));
	isc_buffer_add(&buf, sizeof(text));

	result = isc_lex_openbuffer(lex, &buf);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_lex_setsourceline(lex, 100);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	for (i = 0; i < 6; i++) {
		result = isc_lex_gettoken(lex, 0, &token);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

		line = isc_lex_getsourceline(lex);
		ATF_REQUIRE_EQ(line, 100U + i);
	}

	result = isc_lex_gettoken(lex, 0, &token);
	ATF_REQUIRE_EQ(result, ISC_R_EOF);

	line = isc_lex_getsourceline(lex);
	ATF_REQUIRE_EQ(line, 105U);
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, lex_0xff);
	ATF_TP_ADD_TC(tp, lex_setline);
	return (atf_no_error());
}

