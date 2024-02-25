/*	$NetBSD: isc_lex_gettoken.c,v 1.2.2.2 2024/02/25 15:46:11 martin Exp $	*/

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

#include <stddef.h>
#include <stdint.h>

#include <isc/buffer.h>
#include <isc/lex.h>
#include <isc/mem.h>
#include <isc/util.h>

#include "fuzz.h"

bool debug = false;

static isc_mem_t *mctx = NULL;
static isc_lex_t *lex = NULL;

int
LLVMFuzzerInitialize(int *argc __attribute__((unused)),
		     char ***argv __attribute__((unused))) {
	isc_result_t result;

	isc_mem_create(&mctx);

	result = isc_lex_create(mctx, 1024, &lex);
	REQUIRE(result == ISC_R_SUCCESS);

	return (0);
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	isc_buffer_t buf;
	isc_result_t result;

	isc_buffer_constinit(&buf, data, size);
	isc_buffer_add(&buf, size);
	isc_buffer_setactive(&buf, size);

	CHECK(isc_lex_openbuffer(lex, &buf));

	do {
		isc_token_t token;
		result = isc_lex_gettoken(lex, 0, &token);
	} while (result == ISC_R_SUCCESS);

	return (0);
}
