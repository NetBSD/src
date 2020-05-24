/*	$NetBSD: mempool_test.c,v 1.4 2020/05/24 19:46:13 christos Exp $	*/

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

#include <isc/mem.h>
#include <isc/util.h>

isc_mem_t *mctx;

int
main(int argc, char *argv[]) {
	void *items1[50];
	void *items2[50];
	void *tmp;
	isc_mempool_t *mp1, *mp2;
	unsigned int i, j;
	isc_mutex_t lock;

	UNUSED(argc);
	UNUSED(argv);

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	isc_mutex_init(&lock);

	mctx = NULL;
	isc_mem_create(&mctx);

	mp1 = NULL;
	isc_mempool_create(mctx, 24, &mp1);

	mp2 = NULL;
	isc_mempool_create(mctx, 31, &mp2);

	isc_mempool_associatelock(mp1, &lock);
	isc_mempool_associatelock(mp2, &lock);

	isc_mem_stats(mctx, stderr);

	isc_mempool_setfreemax(mp1, 10);
	isc_mempool_setfillcount(mp1, 10);
	isc_mempool_setmaxalloc(mp1, 30);

	/*
	 * Allocate 30 items from the pool.  This is our max.
	 */
	for (i = 0; i < 30; i++) {
		items1[i] = isc_mempool_get(mp1);
		RUNTIME_CHECK(items1[i] != NULL);
	}

	/*
	 * Try to allocate one more.  This should fail.
	 */
	tmp = isc_mempool_get(mp1);
	RUNTIME_CHECK(tmp == NULL);

	/*
	 * Free the first 11 items.  Verify that there are 10 free items on
	 * the free list (which is our max).
	 */

	for (i = 0; i < 11; i++) {
		isc_mempool_put(mp1, items1[i]);
		items1[i] = NULL;
	}

	RUNTIME_CHECK(isc_mempool_getfreecount(mp1) == 10);
	RUNTIME_CHECK(isc_mempool_getallocated(mp1) == 19);

	isc_mem_stats(mctx, stderr);

	/*
	 * Now, beat up on mp2 for a while.  Allocate 50 items, then free
	 * them, then allocate 50 more, etc.
	 */
	isc_mempool_setfreemax(mp2, 25);
	isc_mempool_setfillcount(mp2, 25);
	for (j = 0; j < 5000; j++) {
		for (i = 0; i < 50; i++) {
			items2[i] = isc_mempool_get(mp2);
			RUNTIME_CHECK(items2[i] != NULL);
		}
		for (i = 0; i < 50; i++) {
			isc_mempool_put(mp2, items2[i]);
			items2[i] = NULL;
		}
	}

	/*
	 * Free all the other items and blow away this pool.
	 */
	for (i = 11; i < 30; i++) {
		isc_mempool_put(mp1, items1[i]);
		items1[i] = NULL;
	}

	isc_mempool_destroy(&mp1);

	isc_mem_stats(mctx, stderr);

	isc_mempool_destroy(&mp2);

	isc_mem_stats(mctx, stderr);

	isc_mem_destroy(&mctx);

	isc_mutex_destroy(&lock);

	return (0);
}
