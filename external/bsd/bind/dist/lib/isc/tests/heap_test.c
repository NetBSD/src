/*	$NetBSD: heap_test.c,v 1.1.1.1 2018/04/07 21:44:11 christos Exp $	*/

/*
 * Copyright (C) 2017  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* ! \file */

#include <config.h>

#include <atf-c.h>

#include <stdio.h>
#include <string.h>

#include <isc/heap.h>
#include <isc/mem.h>

#include <isc/util.h>

struct e {
	unsigned int value;
	unsigned int index;
};

static isc_boolean_t
compare(void *p1, void *p2) {
	struct e *e1 = p1;
	struct e *e2 = p2;

	return (ISC_TF(e1->value < e2->value));
}

static void
idx(void *p, unsigned int i) {
	struct e *e = p;

	e->index = i;
}

ATF_TC(isc_heap_delete);
ATF_TC_HEAD(isc_heap_delete, tc) {
	atf_tc_set_md_var(tc, "descr", "test isc_heap_delete");
}
ATF_TC_BODY(isc_heap_delete, tc) {
	isc_mem_t *mctx = NULL;
	isc_heap_t *heap = NULL;
	isc_result_t result;
	struct e e1 = { 100, 0 };

	UNUSED(tc);

	result = isc_mem_create(0, 0, &mctx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_heap_create(mctx, compare, idx, 0, &heap);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE(heap != NULL);

	isc_heap_insert(heap, &e1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(e1.index, 1);

	isc_heap_delete(heap, e1.index);
	ATF_CHECK_EQ(e1.index, 0);

	isc_heap_destroy(&heap);
	ATF_REQUIRE_EQ(heap, NULL);

	isc_mem_detach(&mctx);
	ATF_REQUIRE_EQ(mctx, NULL);
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, isc_heap_delete);

	return (atf_no_error());
}
