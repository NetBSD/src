/*	$NetBSD: counter.c,v 1.1.2.2 2024/02/24 13:07:19 martin Exp $	*/

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

/*! \file */

#include <stdbool.h>
#include <stddef.h>

#include <isc/atomic.h>
#include <isc/counter.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/refcount.h>
#include <isc/util.h>

#define COUNTER_MAGIC	 ISC_MAGIC('C', 'n', 't', 'r')
#define VALID_COUNTER(r) ISC_MAGIC_VALID(r, COUNTER_MAGIC)

struct isc_counter {
	unsigned int magic;
	isc_mem_t *mctx;
	isc_refcount_t references;
	atomic_uint_fast32_t limit;
	atomic_uint_fast32_t used;
};

isc_result_t
isc_counter_create(isc_mem_t *mctx, int limit, isc_counter_t **counterp) {
	isc_counter_t *counter;

	REQUIRE(counterp != NULL && *counterp == NULL);

	counter = isc_mem_get(mctx, sizeof(*counter));

	counter->mctx = NULL;
	isc_mem_attach(mctx, &counter->mctx);

	isc_refcount_init(&counter->references, 1);
	atomic_init(&counter->limit, limit);
	atomic_init(&counter->used, 0);

	counter->magic = COUNTER_MAGIC;
	*counterp = counter;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_counter_increment(isc_counter_t *counter) {
	uint32_t used = atomic_fetch_add_relaxed(&counter->used, 1) + 1;
	uint32_t limit = atomic_load_acquire(&counter->limit);

	if (limit != 0 && used >= limit) {
		return (ISC_R_QUOTA);
	}

	return (ISC_R_SUCCESS);
}

unsigned int
isc_counter_used(isc_counter_t *counter) {
	REQUIRE(VALID_COUNTER(counter));

	return (atomic_load_acquire(&counter->used));
}

void
isc_counter_setlimit(isc_counter_t *counter, int limit) {
	REQUIRE(VALID_COUNTER(counter));

	atomic_store(&counter->limit, limit);
}

void
isc_counter_attach(isc_counter_t *source, isc_counter_t **targetp) {
	REQUIRE(VALID_COUNTER(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);

	*targetp = source;
}

static void
destroy(isc_counter_t *counter) {
	isc_refcount_destroy(&counter->references);
	counter->magic = 0;
	isc_mem_putanddetach(&counter->mctx, counter, sizeof(*counter));
}

void
isc_counter_detach(isc_counter_t **counterp) {
	isc_counter_t *counter;

	REQUIRE(counterp != NULL && *counterp != NULL);
	counter = *counterp;
	*counterp = NULL;
	REQUIRE(VALID_COUNTER(counter));

	if (isc_refcount_decrement(&counter->references) == 1) {
		destroy(counter);
	}
}
