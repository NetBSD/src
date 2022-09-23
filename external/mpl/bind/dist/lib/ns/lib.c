/*	$NetBSD: lib.c,v 1.1.1.7 2022/09/23 12:09:23 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <stdbool.h>
#include <stddef.h>

#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/once.h>
#include <isc/refcount.h>
#include <isc/util.h>

#include <dns/name.h>

#include <ns/lib.h>

/***
 *** Globals
 ***/

LIBNS_EXTERNAL_DATA unsigned int ns_pps = 0U;

/***
 *** Private
 ***/

static isc_once_t init_once = ISC_ONCE_INIT;
static isc_mem_t *ns_g_mctx = NULL;
static bool initialize_done = false;
static isc_refcount_t references;

static void
initialize(void) {
	REQUIRE(!initialize_done);

	isc_mem_create(&ns_g_mctx);

	isc_refcount_init(&references, 0);
	initialize_done = true;
	return;
}

isc_result_t
ns_lib_init(void) {
	isc_result_t result;

	/*
	 * Since this routine is expected to be used by a normal application,
	 * it should be better to return an error, instead of an emergency
	 * abort, on any failure.
	 */
	result = isc_once_do(&init_once, initialize);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	if (!initialize_done) {
		return (ISC_R_FAILURE);
	}

	isc_refcount_increment0(&references);

	return (ISC_R_SUCCESS);
}

void
ns_lib_shutdown(void) {
	if (isc_refcount_decrement(&references) == 1) {
		isc_refcount_destroy(&references);
		if (ns_g_mctx != NULL) {
			isc_mem_detach(&ns_g_mctx);
		}
	}
}
