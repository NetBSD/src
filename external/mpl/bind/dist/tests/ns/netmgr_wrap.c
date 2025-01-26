/*	$NetBSD: netmgr_wrap.c,v 1.1.1.1 2025/01/26 16:12:36 christos Exp $	*/

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

#include <isc/atomic.h>
#include <isc/netmgr.h>
#include <isc/util.h>

#include <dns/view.h>

#include <ns/client.h>

#if ISC_NETMGR_TRACE
#define FLARG                                                                 \
	, const char *func ISC_ATTR_UNUSED, const char *file ISC_ATTR_UNUSED, \
		unsigned int line ISC_ATTR_UNUSED
#else
#define FLARG
#endif

/*
 * We don't want to use netmgr-based client accounting, we need to emulate it.
 */
atomic_uint_fast32_t client_refs[32];
atomic_uintptr_t client_addrs[32];

#if ISC_NETMGR_TRACE
void
isc_nmhandle__attach(isc_nmhandle_t *source, isc_nmhandle_t **targetp FLARG) {
#else
void
isc_nmhandle_attach(isc_nmhandle_t *source, isc_nmhandle_t **targetp) {
#endif
	ns_client_t *client = (ns_client_t *)source;
	int i;

	for (i = 0; i < 32; i++) {
		if (atomic_load(&client_addrs[i]) == (uintptr_t)client) {
			break;
		}
	}
	INSIST(i < 32);
	INSIST(atomic_load(&client_refs[i]) > 0);

	atomic_fetch_add(&client_refs[i], 1);
#if 0
	fprintf(stderr, "%s:%s:%s:%d -> %ld\n", __func__, func, file, line,
		client_refs[i]);
#endif

	*targetp = source;
	return;
}

#if ISC_NETMGR_TRACE
void
isc_nmhandle__detach(isc_nmhandle_t **handlep FLARG) {
#else
void
isc_nmhandle_detach(isc_nmhandle_t **handlep) {
#endif
	isc_nmhandle_t *handle = *handlep;
	ns_client_t *client = (ns_client_t *)handle;
	int i;

	*handlep = NULL;

	for (i = 0; i < 32; i++) {
		if (atomic_load(&client_addrs[i]) == (uintptr_t)client) {
			break;
		}
	}
	INSIST(i < 32);

	if (atomic_fetch_sub(&client_refs[i], 1) == 1) {
		dns_view_detach(&client->view);
		client->state = 4;
		ns__client_reset_cb(client);
		ns__client_put_cb(client);
		atomic_store(&client_addrs[i], (uintptr_t)NULL);
	}
#if 0
	fprintf(stderr, "%s:%s:%s:%d -> %ld\n", __func__, func, file, line,
		client_refs[i]);
#endif

	return;
}
