/*	$NetBSD: pool.c,v 1.3.4.1 2024/02/29 12:35:02 martin Exp $	*/

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

#include <string.h>

#include <isc/mem.h>
#include <isc/pool.h>
#include <isc/random.h>
#include <isc/util.h>

/***
 *** Types.
 ***/

struct isc_pool {
	isc_mem_t *mctx;
	unsigned int count;
	isc_pooldeallocator_t free;
	isc_poolinitializer_t init;
	void *initarg;
	void **pool;
};

/***
 *** Functions.
 ***/

static isc_result_t
alloc_pool(isc_mem_t *mctx, unsigned int count, isc_pool_t **poolp) {
	isc_pool_t *pool;

	pool = isc_mem_get(mctx, sizeof(*pool));
	pool->count = count;
	pool->free = NULL;
	pool->init = NULL;
	pool->initarg = NULL;
	pool->mctx = NULL;
	isc_mem_attach(mctx, &pool->mctx);
	pool->pool = isc_mem_get(mctx, count * sizeof(void *));
	memset(pool->pool, 0, count * sizeof(void *));

	*poolp = pool;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_pool_create(isc_mem_t *mctx, unsigned int count,
		isc_pooldeallocator_t release, isc_poolinitializer_t init,
		void *initarg, isc_pool_t **poolp) {
	isc_pool_t *pool = NULL;
	isc_result_t result;
	unsigned int i;

	INSIST(count > 0);

	/* Allocate the pool structure */
	result = alloc_pool(mctx, count, &pool);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	pool->free = release;
	pool->init = init;
	pool->initarg = initarg;

	/* Populate the pool */
	for (i = 0; i < count; i++) {
		result = init(&pool->pool[i], initarg);
		if (result != ISC_R_SUCCESS) {
			isc_pool_destroy(&pool);
			return (result);
		}
	}

	*poolp = pool;
	return (ISC_R_SUCCESS);
}

void *
isc_pool_get(isc_pool_t *pool) {
	return (pool->pool[isc_random_uniform(pool->count)]);
}

int
isc_pool_count(isc_pool_t *pool) {
	REQUIRE(pool != NULL);
	return (pool->count);
}

isc_result_t
isc_pool_expand(isc_pool_t **sourcep, unsigned int count,
		isc_pool_t **targetp) {
	isc_result_t result;
	isc_pool_t *pool;

	REQUIRE(sourcep != NULL && *sourcep != NULL);
	REQUIRE(targetp != NULL && *targetp == NULL);

	pool = *sourcep;
	*sourcep = NULL;
	if (count > pool->count) {
		isc_pool_t *newpool = NULL;
		unsigned int i;

		/* Allocate a new pool structure */
		result = alloc_pool(pool->mctx, count, &newpool);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}

		newpool->free = pool->free;
		newpool->init = pool->init;
		newpool->initarg = pool->initarg;

		/* Populate the new entries */
		for (i = pool->count; i < count; i++) {
			result = newpool->init(&newpool->pool[i],
					       newpool->initarg);
			if (result != ISC_R_SUCCESS) {
				isc_pool_destroy(&newpool);
				return (result);
			}
		}

		/* Copy over the objects from the old pool */
		for (i = 0; i < pool->count; i++) {
			newpool->pool[i] = pool->pool[i];
			pool->pool[i] = NULL;
		}

		isc_pool_destroy(&pool);
		pool = newpool;
	}

	*targetp = pool;
	return (ISC_R_SUCCESS);
}

void
isc_pool_destroy(isc_pool_t **poolp) {
	unsigned int i;
	isc_pool_t *pool = *poolp;
	*poolp = NULL;
	for (i = 0; i < pool->count; i++) {
		if (pool->free != NULL && pool->pool[i] != NULL) {
			pool->free(&pool->pool[i]);
		}
	}
	isc_mem_put(pool->mctx, pool->pool, pool->count * sizeof(void *));
	isc_mem_putanddetach(&pool->mctx, pool, sizeof(*pool));
}
