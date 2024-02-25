/*	$NetBSD: trampoline.c,v 1.2.2.1 2024/02/25 15:47:19 martin Exp $	*/

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

#include <inttypes.h>
#include <stdlib.h>
#include <uv.h>

#include <isc/mem.h>
#include <isc/once.h>
#include <isc/thread.h>
#include <isc/util.h>

#include "mem_p.h"
#include "trampoline_p.h"

#define ISC__TRAMPOLINE_UNUSED 0

struct isc__trampoline {
	int tid; /* const */
	uintptr_t self;
	isc_threadfunc_t start;
	isc_threadarg_t arg;
	void *jemalloc_enforce_init;
};

/*
 * We can't use isc_mem API here, because it's called too early and the
 * isc_mem_debugging flags can be changed later causing mismatch between flags
 * used for isc_mem_get() and isc_mem_put().
 *
 * Since this is a single allocation at library load and deallocation at library
 * unload, using the standard allocator without the tracking is fine for this
 * single purpose.
 *
 * We can't use isc_mutex API either, because we track whether the mutexes get
 * properly destroyed, and we intentionally leak the static mutex here without
 * destroying it to prevent data race between library destructor running while
 * thread is being still created.
 */

static uv_mutex_t isc__trampoline_lock;
static isc__trampoline_t **trampolines;
thread_local size_t isc_tid_v = SIZE_MAX;
static size_t isc__trampoline_min = 1;
static size_t isc__trampoline_max = 65;

static isc__trampoline_t *
isc__trampoline_new(int tid, isc_threadfunc_t start, isc_threadarg_t arg) {
	isc__trampoline_t *trampoline = calloc(1, sizeof(*trampoline));
	RUNTIME_CHECK(trampoline != NULL);

	*trampoline = (isc__trampoline_t){
		.tid = tid,
		.start = start,
		.arg = arg,
		.self = ISC__TRAMPOLINE_UNUSED,
	};

	return (trampoline);
}

void
isc__trampoline_initialize(void) {
	uv_mutex_init(&isc__trampoline_lock);

	trampolines = calloc(isc__trampoline_max, sizeof(trampolines[0]));
	RUNTIME_CHECK(trampolines != NULL);

	/* Get the trampoline slot 0 for the main thread */
	trampolines[0] = isc__trampoline_new(0, NULL, NULL);
	isc_tid_v = trampolines[0]->tid;
	trampolines[0]->self = isc_thread_self();

	/* Initialize the other trampolines */
	for (size_t i = 1; i < isc__trampoline_max; i++) {
		trampolines[i] = NULL;
	}
	isc__trampoline_min = 1;
}

void
isc__trampoline_shutdown(void) {
	/*
	 * When the program using the library exits abruptly and the library
	 * gets unloaded, there might be some existing trampolines from unjoined
	 * threads.  We intentionally ignore those and don't check whether all
	 * trampolines have been cleared before exiting, so we leak a little bit
	 * of resources here, including the lock.
	 */
	free(trampolines[0]);
}

isc__trampoline_t *
isc__trampoline_get(isc_threadfunc_t start, isc_threadarg_t arg) {
	isc__trampoline_t **tmp = NULL;
	isc__trampoline_t *trampoline = NULL;
	uv_mutex_lock(&isc__trampoline_lock);
again:
	for (size_t i = isc__trampoline_min; i < isc__trampoline_max; i++) {
		if (trampolines[i] == NULL) {
			trampoline = isc__trampoline_new(i, start, arg);
			trampolines[i] = trampoline;
			isc__trampoline_min = i + 1;
			goto done;
		}
	}
	tmp = calloc(2 * isc__trampoline_max, sizeof(trampolines[0]));
	RUNTIME_CHECK(tmp != NULL);
	for (size_t i = 0; i < isc__trampoline_max; i++) {
		tmp[i] = trampolines[i];
	}
	for (size_t i = isc__trampoline_max; i < 2 * isc__trampoline_max; i++) {
		tmp[i] = NULL;
	}
	free(trampolines);
	trampolines = tmp;
	isc__trampoline_max = isc__trampoline_max * 2;
	goto again;
done:
	INSIST(trampoline != NULL);
	uv_mutex_unlock(&isc__trampoline_lock);

	return (trampoline);
}

void
isc__trampoline_detach(isc__trampoline_t *trampoline) {
	uv_mutex_lock(&isc__trampoline_lock);
	REQUIRE(trampoline->self == isc_thread_self());
	REQUIRE(trampoline->tid > 0);
	REQUIRE((size_t)trampoline->tid < isc__trampoline_max);
	REQUIRE(trampolines[trampoline->tid] == trampoline);

	trampolines[trampoline->tid] = NULL;

	if (isc__trampoline_min > (size_t)trampoline->tid) {
		isc__trampoline_min = trampoline->tid;
	}

	isc__mem_free_noctx(trampoline->jemalloc_enforce_init, 8);
	free(trampoline);

	uv_mutex_unlock(&isc__trampoline_lock);
	return;
}

void
isc__trampoline_attach(isc__trampoline_t *trampoline) {
	uv_mutex_lock(&isc__trampoline_lock);
	REQUIRE(trampoline->self == ISC__TRAMPOLINE_UNUSED);
	REQUIRE(trampoline->tid > 0);
	REQUIRE((size_t)trampoline->tid < isc__trampoline_max);
	REQUIRE(trampolines[trampoline->tid] == trampoline);

	/* Initialize the trampoline */
	isc_tid_v = trampoline->tid;
	trampoline->self = isc_thread_self();

	/*
	 * Ensure every thread starts with a malloc() call to prevent memory
	 * bloat caused by a jemalloc quirk.  While this dummy allocation is
	 * not used for anything, free() must not be immediately called for it
	 * so that an optimizing compiler does not strip away such a pair of
	 * malloc() + free() calls altogether, as it would foil the fix.
	 */
	trampoline->jemalloc_enforce_init = isc__mem_alloc_noctx(8);
	uv_mutex_unlock(&isc__trampoline_lock);
}

isc_threadresult_t
isc__trampoline_run(isc_threadarg_t arg) {
	isc__trampoline_t *trampoline = (isc__trampoline_t *)arg;
	isc_threadresult_t result;

	isc__trampoline_attach(trampoline);

	/* Run the main function */
	result = (trampoline->start)(trampoline->arg);

	isc__trampoline_detach(trampoline);

	return (result);
}
