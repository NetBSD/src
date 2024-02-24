/*	$NetBSD: taskpool.c,v 1.1.2.2 2024/02/24 13:07:22 martin Exp $	*/

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

#include <isc/mem.h>
#include <isc/random.h>
#include <isc/taskpool.h>
#include <isc/util.h>

/***
 *** Types.
 ***/

struct isc_taskpool {
	isc_mem_t *mctx;
	isc_taskmgr_t *tmgr;
	unsigned int ntasks;
	unsigned int quantum;
	isc_task_t **tasks;
};

/***
 *** Functions.
 ***/

static void
alloc_pool(isc_taskmgr_t *tmgr, isc_mem_t *mctx, unsigned int ntasks,
	   unsigned int quantum, isc_taskpool_t **poolp) {
	isc_taskpool_t *pool;
	unsigned int i;

	pool = isc_mem_get(mctx, sizeof(*pool));

	pool->mctx = NULL;
	isc_mem_attach(mctx, &pool->mctx);
	pool->ntasks = ntasks;
	pool->quantum = quantum;
	pool->tmgr = tmgr;
	pool->tasks = isc_mem_get(mctx, ntasks * sizeof(isc_task_t *));
	for (i = 0; i < ntasks; i++) {
		pool->tasks[i] = NULL;
	}

	*poolp = pool;
}

isc_result_t
isc_taskpool_create(isc_taskmgr_t *tmgr, isc_mem_t *mctx, unsigned int ntasks,
		    unsigned int quantum, bool priv, isc_taskpool_t **poolp) {
	unsigned int i;
	isc_taskpool_t *pool = NULL;

	INSIST(ntasks > 0);

	/* Allocate the pool structure */
	alloc_pool(tmgr, mctx, ntasks, quantum, &pool);

	/* Create the tasks */
	for (i = 0; i < ntasks; i++) {
		isc_result_t result = isc_task_create_bound(tmgr, quantum,
							    &pool->tasks[i], i);
		if (result != ISC_R_SUCCESS) {
			isc_taskpool_destroy(&pool);
			return (result);
		}
		isc_task_setprivilege(pool->tasks[i], priv);
		isc_task_setname(pool->tasks[i], "taskpool", NULL);
	}

	*poolp = pool;
	return (ISC_R_SUCCESS);
}

void
isc_taskpool_gettask(isc_taskpool_t *pool, isc_task_t **targetp) {
	isc_task_attach(pool->tasks[isc_random_uniform(pool->ntasks)], targetp);
}

int
isc_taskpool_size(isc_taskpool_t *pool) {
	REQUIRE(pool != NULL);
	return (pool->ntasks);
}

isc_result_t
isc_taskpool_expand(isc_taskpool_t **sourcep, unsigned int size, bool priv,
		    isc_taskpool_t **targetp) {
	isc_taskpool_t *pool;

	REQUIRE(sourcep != NULL && *sourcep != NULL);
	REQUIRE(targetp != NULL && *targetp == NULL);

	pool = *sourcep;
	*sourcep = NULL;
	if (size > pool->ntasks) {
		isc_taskpool_t *newpool = NULL;
		unsigned int i;

		/* Allocate a new pool structure */
		alloc_pool(pool->tmgr, pool->mctx, size, pool->quantum,
			   &newpool);

		/* Copy over the tasks from the old pool */
		for (i = 0; i < pool->ntasks; i++) {
			newpool->tasks[i] = pool->tasks[i];
			pool->tasks[i] = NULL;
		}

		/* Create new tasks */
		for (i = pool->ntasks; i < size; i++) {
			isc_result_t result =
				isc_task_create_bound(pool->tmgr, pool->quantum,
						      &newpool->tasks[i], i);
			if (result != ISC_R_SUCCESS) {
				*sourcep = pool;
				isc_taskpool_destroy(&newpool);
				return (result);
			}
			isc_task_setprivilege(newpool->tasks[i], priv);
			isc_task_setname(newpool->tasks[i], "taskpool", NULL);
		}

		isc_taskpool_destroy(&pool);
		pool = newpool;
	}

	*targetp = pool;
	return (ISC_R_SUCCESS);
}

void
isc_taskpool_destroy(isc_taskpool_t **poolp) {
	unsigned int i;
	isc_taskpool_t *pool = *poolp;
	*poolp = NULL;
	for (i = 0; i < pool->ntasks; i++) {
		if (pool->tasks[i] != NULL) {
			isc_task_detach(&pool->tasks[i]);
		}
	}
	isc_mem_put(pool->mctx, pool->tasks,
		    pool->ntasks * sizeof(isc_task_t *));
	isc_mem_putanddetach(&pool->mctx, pool, sizeof(*pool));
}
