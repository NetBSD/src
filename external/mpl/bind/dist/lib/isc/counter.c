/*	$NetBSD: counter.c,v 1.4 2019/02/24 20:01:31 christos Exp $	*/

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

/*! \file */

#include <config.h>

#include <stdbool.h>
#include <stddef.h>

#include <isc/atomic.h>
#include <isc/counter.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/util.h>

#define COUNTER_MAGIC			ISC_MAGIC('C', 'n', 't', 'r')
#define VALID_COUNTER(r)		ISC_MAGIC_VALID(r, COUNTER_MAGIC)

struct isc_counter {
	unsigned int	magic;
	isc_mem_t	*mctx;
	atomic_uint_fast32_t	references;
	atomic_uint_fast32_t	limit;
	atomic_uint_fast32_t	used;
};

isc_result_t
isc_counter_create(isc_mem_t *mctx, int limit, isc_counter_t **counterp) {
	isc_counter_t *counter;

	REQUIRE(counterp != NULL && *counterp == NULL);

	counter = isc_mem_get(mctx, sizeof(*counter));
	if (counter == NULL)
		return (ISC_R_NOMEMORY);

	counter->mctx = NULL;
	isc_mem_attach(mctx, &counter->mctx);

	atomic_store(&counter->references, 1);
	atomic_store(&counter->limit, limit);
	atomic_store(&counter->used, 0);

	counter->magic = COUNTER_MAGIC;
	*counterp = counter;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_counter_increment(isc_counter_t *counter) {
	isc_result_t result = ISC_R_SUCCESS;

	uint32_t used = atomic_fetch_add(&counter->used, 1) + 1;
	if (atomic_load(&counter->limit) != 0 &&
	    used >= atomic_load(&counter->limit)) {
		result = ISC_R_QUOTA;
	}

	return (result);
}

unsigned int
isc_counter_used(isc_counter_t *counter) {
	REQUIRE(VALID_COUNTER(counter));

	return (atomic_load(&counter->used));
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

	INSIST(atomic_fetch_add(&source->references, 1) > 0);

	*targetp = source;
}

static void
destroy(isc_counter_t *counter) {
	counter->magic = 0;
	isc_mem_putanddetach(&counter->mctx, counter, sizeof(*counter));
}

void
isc_counter_detach(isc_counter_t **counterp) {
	isc_counter_t *counter;
	uint32_t oldrefs;

	REQUIRE(counterp != NULL && *counterp != NULL);
	counter = *counterp;
	REQUIRE(VALID_COUNTER(counter));

	*counterp = NULL;

	oldrefs = atomic_fetch_sub(&counter->references, 1);
	INSIST(oldrefs > 0);

	if (oldrefs == 1) {
		destroy(counter);
	}
}
