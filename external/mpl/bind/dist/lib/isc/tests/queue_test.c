/*	$NetBSD: queue_test.c,v 1.3.2.3 2020/04/13 08:02:59 martin Exp $	*/

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
#include <time.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/queue.h>
#include <isc/util.h>

#include "isctest.h"

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = isc_test_begin(NULL, true, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	isc_test_end();

	return (0);
}

typedef struct item item_t;
struct item {
	int 			value;
	ISC_QLINK(item_t)	qlink;
};

typedef ISC_QUEUE(item_t) item_queue_t;

static void
item_init(item_t *item, int value) {
	item->value = value;
	ISC_QLINK_INIT(item, qlink);
}

/* Test UDP sendto/recv (IPv4) */
static void
queue_valid(void **state) {
	item_queue_t queue;
	item_t one, two, three, four, five;
	item_t *p;

	UNUSED(state);

	ISC_QUEUE_INIT(queue, qlink);

	item_init(&one, 1);
	item_init(&two, 2);
	item_init(&three, 3);
	item_init(&four, 4);
	item_init(&five, 5);

	assert_true(ISC_QUEUE_EMPTY(queue));

	ISC_QUEUE_POP(queue, qlink, p);
	assert_null(p);

	assert_false(ISC_QLINK_LINKED(&one, qlink));
	ISC_QUEUE_PUSH(queue, &one, qlink);
	assert_true(ISC_QLINK_LINKED(&one, qlink));

	assert_false(ISC_QUEUE_EMPTY(queue));

	ISC_QUEUE_POP(queue, qlink, p);
	assert_non_null(p);
	assert_int_equal(p->value, 1);
	assert_true(ISC_QUEUE_EMPTY(queue));
	assert_false(ISC_QLINK_LINKED(p, qlink));

	ISC_QUEUE_PUSH(queue, p, qlink);
	assert_false(ISC_QUEUE_EMPTY(queue));
	assert_true(ISC_QLINK_LINKED(p, qlink));

	assert_false(ISC_QLINK_LINKED(&two, qlink));
	ISC_QUEUE_PUSH(queue, &two, qlink);
	assert_true(ISC_QLINK_LINKED(&two, qlink));

	assert_false(ISC_QLINK_LINKED(&three, qlink));
	ISC_QUEUE_PUSH(queue, &three, qlink);
	assert_true(ISC_QLINK_LINKED(&three, qlink));

	assert_false(ISC_QLINK_LINKED(&four, qlink));
	ISC_QUEUE_PUSH(queue, &four, qlink);
	assert_true(ISC_QLINK_LINKED(&four, qlink));

	assert_false(ISC_QLINK_LINKED(&five, qlink));
	ISC_QUEUE_PUSH(queue, &five, qlink);
	assert_true(ISC_QLINK_LINKED(&five, qlink));

	/* Test unlink by removing one item from the middle */
	ISC_QUEUE_UNLINK(queue, &three, qlink);

	ISC_QUEUE_POP(queue, qlink, p);
	assert_non_null(p);
	assert_int_equal(p->value, 1);

	ISC_QUEUE_POP(queue, qlink, p);
	assert_non_null(p);
	assert_int_equal(p->value, 2);

	ISC_QUEUE_POP(queue, qlink, p);
	assert_non_null(p);
	assert_int_equal(p->value, 4);

	ISC_QUEUE_POP(queue, qlink, p);
	assert_non_null(p);
	assert_int_equal(p->value, 5);

	assert_null(queue.head);
	assert_null(queue.tail);
	assert_true(ISC_QUEUE_EMPTY(queue));

	ISC_QUEUE_DESTROY(queue);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(queue_valid,
						_setup, _teardown),
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
