/*	$NetBSD: lib.c,v 1.1.1.1 2018/08/12 12:08:06 christos Exp $	*/

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

#include <stddef.h>

#include <isc/mem.h>
#include <isc/msgcat.h>
#include <isc/mutex.h>
#include <isc/once.h>
#include <isc/util.h>

#include <dns/name.h>

#include <ns/lib.h>


/***
 *** Globals
 ***/

LIBNS_EXTERNAL_DATA unsigned int			ns_pps = 0U;


/***
 *** Private
 ***/

static isc_once_t init_once = ISC_ONCE_INIT;
static isc_mem_t *ns_g_mctx = NULL;
static isc_boolean_t initialize_done = ISC_FALSE;
static isc_mutex_t reflock;
static unsigned int references = 0;

static void
initialize(void) {
	isc_result_t result;

	REQUIRE(initialize_done == ISC_FALSE);

	result = isc_mem_create(0, 0, &ns_g_mctx);
	if (result != ISC_R_SUCCESS)
		return;

	result = isc_mutex_init(&reflock);
	if (result != ISC_R_SUCCESS)
		goto cleanup_mctx;

	initialize_done = ISC_TRUE;
	return;

  cleanup_mctx:
	if (ns_g_mctx != NULL)
		isc_mem_detach(&ns_g_mctx);
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
	if (result != ISC_R_SUCCESS)
		return (result);

	if (!initialize_done)
		return (ISC_R_FAILURE);

	LOCK(&reflock);
	references++;
	UNLOCK(&reflock);

	return (ISC_R_SUCCESS);
}

void
ns_lib_shutdown(void) {
	isc_boolean_t cleanup_ok = ISC_FALSE;

	LOCK(&reflock);
	if (--references == 0)
		cleanup_ok = ISC_TRUE;
	UNLOCK(&reflock);

	if (!cleanup_ok)
		return;

	if (ns_g_mctx != NULL)
		isc_mem_detach(&ns_g_mctx);
}
