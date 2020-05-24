/*	$NetBSD: fuzz.h,v 1.3 2020/05/24 19:46:22 christos Exp $	*/

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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <isc/lang.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dst/dst.h>

ISC_LANG_BEGINDECLS

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

static isc_mem_t *mctx = NULL;

static void __attribute__((constructor)) init(void) {
	isc_mem_create(&mctx);
	RUNTIME_CHECK(dst_lib_init(mctx, NULL) == ISC_R_SUCCESS);
}

static void __attribute__((destructor)) deinit(void) {
	dst_lib_destroy();
	isc_mem_destroy(&mctx);
}

ISC_LANG_ENDDECLS
