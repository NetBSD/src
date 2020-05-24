/*	$NetBSD: wrap.c,v 1.2 2020/05/24 19:46:30 christos Exp $	*/

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

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/util.h>

#include <dns/view.h>

#include <ns/client.h>

/*
 * This overrides calls to isc_nmhandle_unref(), sending them to
 * __wrap_isc_nmhandle_unref(), when libtool is in use and LD_WRAP
 * can't be used.
 */

extern void
__wrap_isc_nmhandle_unref(isc_nmhandle_t *handle);

void
isc_nmhandle_unref(isc_nmhandle_t *handle) {
	__wrap_isc_nmhandle_unref(handle);
}
