/*	$NetBSD: mutexblock.c,v 1.3.2.2 2019/06/10 22:04:43 christos Exp $	*/

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

#include <isc/mutexblock.h>
#include <isc/util.h>

void
isc_mutexblock_init(isc_mutex_t *block, unsigned int count) {
	unsigned int i;

	for (i = 0; i < count; i++) {
		isc_mutex_init(&block[i]);
	}
}

void
isc_mutexblock_destroy(isc_mutex_t *block, unsigned int count) {
	unsigned int i;

	for (i = 0; i < count; i++) {
		isc_mutex_destroy(&block[i]);
	}
}
